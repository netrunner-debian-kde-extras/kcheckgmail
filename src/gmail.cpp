/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo                                  *
 *   mwlazlo@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// define this symbol if you want to write the fetched data to disc
#undef DUMP_PAGES
#define DUMP_DIR "/tmp/"

#include "gmail.h"
#include "gmailwalletmanager.h"
#include "prefs.h"
#include "gmail_constants.h"

#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <kdebug.h>

#include <qmutex.h>
#include <qregexp.h>
#include <qtimer.h>

#include <kapp.h>
#include <dcopclient.h>

#ifdef DUMP_PAGES
#include <qfile.h>
#endif

#define MILLISECS(x) (x * 1000)


GMail::GMail() : QObject(0, "GMailNetwork")
{
	mInterval = Prefs::interval();
	mCheckLock = new QMutex();
	mLoginLock = new QMutex();
	mLoginToken = 0;
	mLoginParamsChanged = false;
	isGAP4D = false;
	
	/** GMail/Google cookies:
		TZ: (timezone offset)
		GMAIL_LOGIN: page_timer/page_timer/timer_at_login
		RTT(Round Trip Time): (end - start)
	Details:
	RTT: checks how many seconds it takes to load the image (https://mail.google.com/mail/images/c.gif?t=timestamp) and result is set to RTT
	Pinging user: https://mail.google.com/mail?gxlu=(user)&zx=(timestamp) is used to detect whether the user exists or not, if so, focus on the password field
	
	Other stuff:
	hl: lang code, two characters
	gl: country code, two characters
	ltmpl: possible values: ca_tlsosm, ca_tlsosm_video
	*/
	
	//Any % should be replaced with @ due to a problem with QString not looking for escaped %
	gGMailLoginURL = "https://www.google.com/accounts/ServiceLoginAuth";
	gGMailLoginPostFormat = "Email=%1&Passwd=%2&null=Sign+in&service=mail"
			"&continue=http@3A@2F@2Fmail.google.com@2Fmail@3F"
			"&ltmpl=ca_tlsosm&ltmplcache=2&rm=false&PersistentCookie=false";
	gGMailPostLoginURLFormat = "%1://mail.google.com/mail/?auth=";
	gGMailCheckURL = "%1://mail.google.com/mail/?search=query"
			"&q=@20is@3Aunread@20in@3A%2&as_subset=unread&view=tl&start=0";
	gGMailLogOut = "https://mail.google.com/mail/?logout";
	
	gGAP4DLoginURL = "https://www.google.com/a/%1/LoginAction";
	gGAP4DLoginPostFormat = "userName=%1&password=%2&at=null&service=mail";
	gGAP4DCheckURL = "%1://mail.google.com/a/%2/?search=query"
			"&q=@20is@3Aunread@20in@3A%3&as_subset=unread&view=tl&start=0";
	gGAP4DPostLoginURLFormat = "%1://mail.google.com/a/%2/";
	gGAP4DLogOut = "https://mail.google.com/a/%1/?logout";
	
	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(slotTimeout()));

	mTimer->start(MILLISECS(mInterval));
}

GMail::~GMail()
{
	delete mCheckLock;
	delete mLoginLock;
	if(mLoginToken) 
		mLoginToken = 0;
}

void GMail::slotSetWalletPassword(bool)
{
	kdDebug() << k_funcinfo << "now, check login params." << endl;
	checkLoginParams();
}

void GMail::checkLoginParams()
{
	QString username = Prefs::gmailUsername();
	const QString& password = GMailWalletManager::instance()->getHash();
	if(mUsername != username || mPasswordHash != password) {
		mUsername = username;
		mPasswordHash = password;
		
		useUsername = mUsername;
		useDomain = "";
		isGAP4D = false;
		
		if( mUsername.find("@") != -1 ) {
			
			if ( QString::compare(mUsername.section("@",1),"gmail.com") != 0 &&
				QString::compare(mUsername.section("@",1),"googlemail.com") != 0) {
				kdDebug() << k_funcinfo << mUsername << " seems to be a GAP4D account" << endl;
				isGAP4D = true;
				useDomain = mUsername.section("@",1);
				
				/*****             REMOVE THIS TO RE-ENABLE GAP4D SUPPORT                  *****/
				
				emit loginDone(false, mLoginFromTimer, "GAP4D support is disabled because it needs to be fixed!");
				return;
			}
			useUsername = mUsername.section("@",0,0);
		}
		
		kdDebug() << k_funcinfo << "Using " << useUsername << " as username and " << useDomain << " as domain" << endl;
		
		getURLPart(true);
		
		if(mLoginLock->tryLock()) {
			if(mLoginToken)
			    mLoginToken = 0;
			mLoginLock->unlock();
			
			//Try to log out if a session already exists (because it might be from an other address)
			if(isLoggedIn(false)) {
				kdDebug() << k_funcinfo << "A gmail session was already open, logging out from it" << endl;
				logOut();
			}
			
			mLoginFromTimer = false;
			login();
		} else {
			kdDebug() << k_funcinfo << "Login in process. "
				<< "scheduling login for next timeout." << endl;
			mLoginParamsChanged = true;
		}
	}
}

void GMail::slotGetWalletPassword(const QString& pass)
{
	QString str, LoginPostFormat, LoginURL;
	
	if(isGAP4D) {
		LoginPostFormat = gGAP4DLoginPostFormat;
		LoginURL = QString(gGAP4DLoginURL).arg(useDomain).replace('@','%');
	} else {
		LoginPostFormat = gGMailLoginPostFormat;
		LoginURL = gGMailLoginURL;
	}
	
	str = QString(LoginPostFormat).arg(useUsername,pass).replace('@','%');
	kdDebug() << k_funcinfo << "Sending login: " << str << endl;

	QCString b(str.utf8());
	QByteArray postData(b);

	// get rid of terminating 0x0
	postData.truncate(b.length());
	
	KIO::TransferJob *job = KIO::http_post(
		LoginURL,
		postData, false);
	job->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	
	connect(job, SIGNAL(result(KIO::Job*)),
		SLOT(slotLoginResult(KIO::Job*)));
	
	connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
		SLOT(slotLoginData(KIO::Job*, const QByteArray&)));
}

///////////////////////////////////////////////////////////////////////////
// Initial login exchange methods
///////////////////////////////////////////////////////////////////////////
void GMail::login()
{
	if(mLoginLock->tryLock()) {
		emit loginStart();
		
		kdDebug() << k_funcinfo << "Waiting for wallet..." << endl;
		// this will call back to gotWalletPassword().
		// we will continue the process from there.
		GMailWalletManager::instance()->get();
	}
}

void GMail::slotLoginResult(KIO::Job *job)
{	
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {

		if(mLoginToken) {
			if(isGAP4D) {
				mLoginLock->unlock();
				if(!isLoggedIn()) {
					emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
				} else {
					emit loginDone(true, mLoginFromTimer);
					checkGMail();
				}
				return;
			}
			
			static QRegExp rx("http[s]?://mail.google.com/mail\\?auth=([_\\d\\w\\-]+)");
			int found;
			
			kdDebug() << k_funcinfo << "mLoginToken: true" << endl;

			if(!rx.isValid()) {
				kdWarning() << k_funcinfo << "Invalid RX!\n"
						<< rx.errorString() << endl;
			}
			
			found = rx.search(mLoginBuffer);
			
			if( found == -1 ) {
				if(mLoginBuffer.find("onload") != -1 && (
							mLoginBuffer.find("FixForm") != -1 ||
							mLoginBuffer.find("start_time") != -1)) {
				
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
				} else {
					kdWarning() << k_funcinfo << " auth info couldn't be found!" << endl;
					dump2File("gmail_login.html",mLoginBuffer);
					
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
				}
			} else {
			
				//gGMailPostLoginURLFormat is the _format_, gGMailPostLoginURL is the one to be used
				gGMailPostLoginURL = gGMailPostLoginURLFormat + rx.cap(1);
				
				mLoginBuffer = "";
				postLogin();
				// NOTE: LoginLock is still locked()
			}
		} else {
			mLoginLock->unlock();
			emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
		}
	}
}

void GMail::slotLoginData(KIO::Job *job, const QByteArray &data)
{
	kdDebug() << k_funcinfo << endl;

	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		mLoginBuffer.append(str);

		// auth cookie no longer exists, no need to check for it
		mLoginToken = 1;
	}
}

///////////////////////////////////////////////////////////////////////////
// Post login procedure: Gather cookies
///////////////////////////////////////////////////////////////////////////
void GMail::postLogin()
{
	// this is expected to be locked.
	if(!mLoginLock->tryLock()) {
		
		QString url;
		
		if(!isGAP4D)
			url = QString(gGMailPostLoginURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" )).replace('@','%');
		else
		{
			/*kdDebug() << k_funcinfo << " isGAP4D: " << isGAP4D << "; " ;
			
			if( !mCookieMap->contains("GMAIL_AT") && mCookieMap->contains("HID")) {
				kdDebug() << " First " << k_funcinfo << " call seems to have finished, now using HID to get GMAIL_AT "<< endl;
				url = QString(gGAP4DGetGMAIL_ATURL).arg(
					(Prefs::useHTTPS()
						? "https" 
						: "http" ),
					useDomain,
					mCookieMap->find("HID").data()).replace('@','%');
			} else {
				kdDebug() << " This seems to be the first call to " << k_funcinfo << ", we are expecting a HID "<< endl;
				url = QString(gGAP4DPostLoginURLFormat).arg(
						(Prefs::useHTTPS()
							? "https" 
							: "http" ),
						useDomain).replace('@','%');
			}*/
		}
		kdDebug() << k_funcinfo << "Starting job to " << url << endl;

		KIO::TransferJob *job = KIO::get(url, true, false);
		job->addMetaData("cookies", "auto");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KIO::Job*)),
			SLOT(slotPostLoginResult(KIO::Job*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotPostLoginData(KIO::Job*, const QByteArray&)));
	} else {
		kdWarning() << k_funcinfo << "mLoginLock is not locked!" << endl;
		mLoginLock->unlock();
	}
}

void GMail::slotPostLoginResult(KIO::Job *job)
{

	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		mLoginLock->unlock();
		
		if(isLoggedIn()) {
			if(true/*mCookieMap->contains("GMAIL_AT")*/) {
				//kdDebug() << k_funcinfo << "We got a GMAIL_AT, emiting signal" << endl;
				
				emit loginDone(true, mLoginFromTimer);
				checkGMail();
			} else { //If we don't have a GMAIL_AT it means we got a HID
				kdDebug() << k_funcinfo << "No GMAIL_AT, but we got a HID, calling postLogin() again; and keeping mLoginLock locked()" << endl;
				mLoginLock->tryLock();
				postLogin();
			}
		} else {
			emit loginDone(false, mLoginFromTimer, 
				       i18n("Unknown error retrieving cookies"));
		}
	}
}

void GMail::slotPostLoginData(KIO::Job *job, const QByteArray &)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	}
}

///////////////////////////////////////////////////////////////////////////
// Email checking methods
///////////////////////////////////////////////////////////////////////////
void GMail::checkGMail()
{
	if(isLoggedIn() && mCheckLock->tryLock()) {
		kdDebug() << k_funcinfo << "Starting check..." << endl;
		// stop timer. start again when we have some sort of result.
		mTimer->stop();
		emit checkStart();

		QString url;

		if(!isGAP4D)
			url = QString(gGMailCheckURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" ),"inbox"/*,
				(Prefs::SearchOnlyInInbox()
					? "inbox" 
					: "anywhere" )*/).replace('@','%');
		else
			url = QString(gGAP4DCheckURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" ),
				useDomain,"inbox"/*,
				(Prefs::SearchOnlyInInbox()
					? "inbox" 
					: "anywhere" )*/).replace('@','%');

		kdDebug() << k_funcinfo << "GET: " << url << endl;

		KIO::TransferJob *job = KIO::get(url, true, false);
		job->addMetaData("cookies", "auto");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KIO::Job*)),
			SLOT(slotCheckResult(KIO::Job*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotCheckData(KIO::Job*, const QByteArray&)));
	}
}

void GMail::slotCheckResult(KIO::Job *job)
{
	if(job->error() != 0) 
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;

	kdDebug() << k_funcinfo << "Check finished." << endl;

	dump2File("gmail_data.html",mPageBuffer);
	
	static QRegExp rx("top.location=[\"|\']http[s]?://www.google.com/accounts/ServiceLogin");
	int found;
	
	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
				<< rx.errorString() << endl;
	}
			
	found = rx.search(mPageBuffer);
			
	if( found != -1 || !isLoggedIn() ) {
		kdWarning() << k_funcinfo << "User is not logged in!" << endl;
		
		mPageBuffer = "";
		mCheckLock->unlock();
		
		//Clearing values will force login
		mUsername = "";
		mPasswordHash = "";
		checkLoginParams();
	} else {
		
		mTimer->start(MILLISECS(mInterval));
		emit checkDone(mPageBuffer);
		mPageBuffer = "";
	
		mCheckLock->unlock();
	}
}

void GMail::slotCheckData(KIO::Job *job, const QByteArray &data)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		mPageBuffer.append(str);

	}
}

///////////////////////////////////////////////////////////////////////////
// Other methods...
///////////////////////////////////////////////////////////////////////////
void GMail::slotTimeout()
{
	if((!mLoginLock->locked() && !isLoggedIn()) || mLoginParamsChanged) {
		mLoginFromTimer = true;
		login();
	} else
		if(!mLoginLock->locked() && isLoggedIn()) {
			// do the check
			mCheckFromTimer = true;
			checkGMail();
		}
}

void GMail::setInterval(unsigned int i)
{
	if(i > Prefs::self()->intervalItem()->minValue().toUInt() && mInterval != i) {
		mInterval = i;
		mTimer->changeInterval(MILLISECS(mInterval));
	} 
}

bool GMail::isLoggedIn(bool lockCheck)
{
	bool ret = false;
	
	if( !lockCheck || ( lockCheck && !mLoginLock->locked() ) ) {
		if(cookieExists(ACTION_TOKEN_COOKIE) /*|| mCookieMap->contains("HID") */)
			ret = true;
		else kdDebug() << k_funcinfo << ACTION_TOKEN_COOKIE << " wasn't found!" << endl;
	} else kdDebug() << "mLoginLock is locked" << endl;

	return ret;
}

bool GMail::isLoggedIn()
{
	return isLoggedIn(true);
}

void GMail::slotCheckGmail()
{
	if(!isLoggedIn()) {
		mLoginFromTimer = false;
		login();
	} else {
		mCheckFromTimer = false;
		checkGMail();
	}
}

QString GMail::getURLPart(bool refresh)
{
	static QString part;
	
	if(!refresh && part.length() > 0) {
		return part;
	}
	
	if(isGAP4D) {
		part = QString("a/%1").arg(useDomain);
	} else {
		part = "mail";
	}
	
	return part;
}

QString GMail::getURLPart()
{
	return getURLPart(false);
}

void GMail::logOut()
{
	if(!isLoggedIn())
		return;
	
	sessionCookie = QString::null;
	
	QString logout_url = (!isGAP4D)? gGMailLogOut : QString(gGAP4DLogOut).arg(useDomain);
	
	KIO::TransferJob *job = KIO::get(logout_url, true, false);
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	kdDebug() << "Loging out! " << logout_url << endl;
}

void GMail::slotLogOut()
{
	logOut();
}

void GMail::dump2File(const QString filename, const QString data)
{
#ifdef DUMP_PAGES
	QString dump_dir = DUMP_DIR;
		
	dump_dir += filename;
		
	QFile f(dump_dir);
		
	f.open( IO_WriteOnly );
		
	QTextStream stream(&f);
	stream << data;

	f.close();
#endif
}

//From kcookiejartest.cpp
QString GMail::findCookies(QString url)
{
	QCString replyType;
	QByteArray params, reply;
	QDataStream stream(params, IO_WriteOnly);
	stream << url;
	if (!kapp->dcopClient()->call("kcookiejar", "kcookiejar",
	     "findCookies(QString)", params, replyType, reply))
	{
		kdWarning() << k_funcinfo << "There was some error using DCOP!" << endl;
		return QString::null;
	}

	QDataStream stream2(reply, IO_ReadOnly);
	if(replyType != "QString")
	{
		kdWarning() << k_funcinfo << "DCOP function findCookies(...) return " << replyType.data() << ", expected QString" << endl;
		return QString::null;
	}

	QString result;
	stream2 >> result;
	return result;
}

bool GMail::cookieExists(QString cookieName,QString url)
{
	kdDebug() << k_funcinfo << "Searching for cookie " << cookieName << " at " << url << endl;;
	
	QString cookies;
	int found;
	bool ret = false;
	
	cookies = findCookies(url);
	
	if(cookies.length() == 0)
		return false;
	
	cookies += ";";
	
	QRegExp search(" (" + QRegExp::escape(cookieName) + ")=([^;]*)");
	
	if(!search.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
				<< search.errorString() << endl;
	}
			
	
	 
	found = search.search(cookies);
	ret = ( found != -1 );
	
	if(ret && QString::compare(cookieName,ACTION_TOKEN_COOKIE) == 0) {
		if(sessionCookie.compare(search.cap(2)) != 0) {
			if(sessionCookie.length() != 0)
				emit sessionChanged();
			
			sessionCookie = search.cap(2);
		}
	}
	
	kdDebug() << cookieName << " was " << (ret? "FOUND":"NOT FOUND") << endl;
	
	return ret;
}

bool GMail::cookieExists(QString cookieName)
{
	return cookieExists(cookieName, QString("https://mail.google.com/%1/").arg(getURLPart()));
}

void GMail::slotSessionChanged()
{
	logOut();
	
	if(mCheckLock->locked())
	mCheckLock->unlock();
	
	//Clearing values will force login
	mUsername = "";
	mPasswordHash = "";
	checkLoginParams();
}

#include "gmail.moc"
