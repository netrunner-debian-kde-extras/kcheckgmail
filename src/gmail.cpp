/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>              *
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>            *
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
#include "gmailparser.h"
#include "gmailwalletmanager.h"
#include "prefs.h"
#include "gmail_constants.h"

#include <kio/job.h>
#include <kio/global.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kdebug.h>

#include <qmutex.h>
#include <qregexp.h>
#include <qtimer.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <dcopclient.h>
#include <kcharsets.h>

#ifdef DUMP_PAGES
#include <qfile.h>
#endif

#define MILLISECS(x) (x * 1000)


GMail::GMail(QObject* parent, const char* name) : QObject(parent, name)
{
	mInterval = Prefs::interval();
	mCheckLock = new QMutex();
	mLoginLock = new QMutex();
	mLoginParamsChanged = false;
	isGAP4D = false;
	
	//Any % should be replaced with @ due to a problem with QString not looking for escaped %
	gGMailLoginURL = "https://www.google.com/accounts/ServiceLoginAuth?service=mail";
	gGMailLoginPOSTFormat = "Email=%1&Passwd=%2&signIn=Sign+in&service=mail"
			"&continue=http@3A@2F@2Fmail.google.com@2Fmail@3F"
			"&ltmpl=default&ltmplcache=2&rm=false&rmShown=1";
	gGMailCheckURL = "%1://mail.google.com/mail/?search=query"
			"&q=%2&as_subset=unread&view=tl&start=0&init=1&ui=1";
	gGMailLogOut = "https://mail.google.com/mail/?logout";
	
	gGAP4DLoginURL = "https://www.google.com/a/%1/LoginAction";
	gGAP4DLoginPOSTFormat = "Email=%1&Passwd=%2&signIn=Sign+in&service=mail"
			"&continue=http@3A@2F@2Fmail.google.com@2Fa@2F%3"
			"&ltmpl=default&ltmplcache=2&rm=false&rmShown=1";
	gGAP4DCheckURL = "%1://mail.google.com/a/%2/?search=query"
			"&q=%3&as_subset=unread&view=tl&start=0&init=1&ui=1";
	gGAP4DLogOut = "https://mail.google.com/a/%1/?logout";
	
	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
	connect(this, SIGNAL(sessionChanged()), 
		this, SLOT(slotSessionChanged()));
}

GMail::~GMail()
{
	if (mCheckLock->locked())
		mCheckLock->unlock();
	delete mCheckLock;
	mCheckLock = 0;

	if (mLoginLock->locked())
		mLoginLock->unlock();
	delete mLoginLock;
	mLoginLock = 0;
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
	
	if(mUsername == username && mPasswordHash == password
		  || username.length() == 0)
		return;
	
	mUsername = username;
	mPasswordHash = password;
	sessionCookie = QString::null;
	
	useUsername = mUsername;
	useDomain = "";
	isGAP4D = false;
	
	if( mUsername.find("@") != -1 ) {
		
		if ( QString::compare(mUsername.section("@",1),"gmail.com") != 0 &&
			QString::compare(mUsername.section("@",1),"googlemail.com") != 0) {
			kdDebug() << k_funcinfo << mUsername << " seems to be a GAP4D account" << endl;
			isGAP4D = true;
			useDomain = mUsername.section("@",1);
		}
		useUsername = mUsername.section("@",0,0);
	}
	
	kdDebug() << k_funcinfo << "Using " << useUsername << " as username and " << useDomain << " as domain" << endl;
	
	if(!mLoginLock->locked()) {
		
		//Try to log out if a session already exists (because it might be from an other address)
		if(isLoggedIn(false)) {
			kdDebug() << k_funcinfo << "A gmail session was already open, logging out from it" << endl;
			logOut(true);
		}
		
		mLoginFromTimer = false;
		login();
	} else {
		kdDebug() << k_funcinfo << "Login in process. "
			<< "scheduling login for next timeout." << endl;
		mLoginParamsChanged = true;
	}
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

void GMail::slotGetWalletPassword(const QString& pass)
{
	QString str, LoginPOSTFormat, LoginURL;
	
	if(isGAP4D) {
		LoginPOSTFormat = QString(gGAP4DLoginPOSTFormat).replace("%3",useDomain);
		LoginURL = QString(gGAP4DLoginURL).arg(useDomain).replace('@','%');
	} else {
		LoginPOSTFormat = gGMailLoginPOSTFormat;
		LoginURL = gGMailLoginURL;
	}
	
	str = QString(LoginPOSTFormat).arg(
			KURL::encode_string(useUsername),
			KURL::encode_string(pass)
					  ).replace('@','%');
	kdDebug() << k_funcinfo << "Requesting login URL" << endl;

	loginRedirection = "";
	
	QCString b(str.utf8());
	QByteArray postData(b);

	// get rid of terminating 0x0
	postData.truncate(b.length());
	
	KIO::TransferJob *job = KIO::http_post(
			LoginURL,
			postData,
   			false);
	job->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	
	connect(job, SIGNAL(result(KIO::Job*)),
		SLOT(slotLoginResult(KIO::Job*)));
	
	connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
		SLOT(slotLoginData(KIO::Job*, const QByteArray&)));
	
	connect(job, SIGNAL(redirection(KIO::Job*, const KURL&)),
		SLOT(slotLoginRedirection(KIO::Job*, const KURL&)));
}

void GMail::slotLoginData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		mLoginBuffer.append(str);
	}
}

void GMail::slotLoginRedirection(KIO::Job *job, const KURL &url)
{
	kdDebug() << k_funcinfo << url.url() << endl;

	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		loginRedirection = url;
	}
}

void GMail::slotLoginResult(KIO::Job *job)
{	
	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;
		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		QString redirection;
		
		redirection = getRedirectURL(mLoginBuffer);
		dump2File("gmail_login.html", mLoginBuffer);
		
		if( redirection == QString::null ) {
			
			if(!isLoggedIn(false)) {
				if(mLoginBuffer.find("onload") != -1 && (
						mLoginBuffer.find("FixForm") != -1 ||
						mLoginBuffer.find("start_time") != -1)) {
				
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
					return;
				} else {
					kdWarning() << k_funcinfo << " Redirection couldn't be found!" << endl;
					
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
			} else if (mLoginBuffer.find("?ui=html") != -1 && (
				   		mLoginBuffer.find("nocheckbrowser") != -1 || 
						mLoginBuffer.find("noscript") != -1 )) { 
				kdDebug() << k_funcinfo << "Google is performing dirty JS check, bypassing it" << endl;
				
				if (loginRedirection.isEmpty()) {
					kdWarning() << k_funcinfo << "loginRedirection is empty!" << endl;
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				mLoginBuffer = "";
				
				KURL _url;
				
				_url = loginRedirection;
				_url.setQuery("ui=html&zy=n");
				
				if (!_url.isValid()) {
					kdWarning() << k_funcinfo << "New _url is invalid!:" << _url.url() << endl;
					mLoginLock->unlock();
					// let's show a nice error message to the user instead
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				postLogin(_url.url());
				
				
			} else {
				//no more redirections?
				kdWarning() << k_funcinfo << "No redirection was found, but seems like we are logged in!" << endl;
				
				mLoginBuffer = "";
				slotPostLoginResult(job);
				return;
			}
		} else {
			mLoginBuffer = "";
			postLogin(redirection);
			// NOTE: LoginLock is still locked()
			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////////
// Post login procedure: Gather cookies
///////////////////////////////////////////////////////////////////////////
void GMail::postLogin(QString url)
{
	// this is expected to be locked.
	if(mLoginLock->locked()) {
		
		static QRegExp rx("^(http[s]?://)(.*)$");
		int found;
		
		if(!rx.isValid()) {
			kdWarning() << k_funcinfo << "Invalid RX!\n"
					<< rx.errorString() << endl;
		}
		
		found = rx.search(url);
		
		if(found == -1) {
			kdWarning() <<  "This can't be a valid url!: " << url << endl;
			if (!KURL(url).isValid()) {
				kdError() <<  "This is absolutely a non-valid URL!: " << url << endl;
			}
		}
		
		if(rx.cap(1).compare("https://") != 0) {
			url = (Prefs::useHTTPS()? "https" : "http" );
			url.append("://");
			url.append(rx.cap(2));
		}
		
		mPostLoginBuffer = "";
		
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
	}
}

void GMail::slotPostLoginData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		mPostLoginBuffer.append(str);
	}
}

void GMail::slotPostLoginResult(KIO::Job *job)
{
	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		mLoginLock->unlock();
		
		if(isLoggedIn()) {
			
			mPostLoginBuffer = "";
			emit loginDone(true, mLoginFromTimer);
			checkGMail();
			
		} else {
			
			QString url = getRedirectURL(mPostLoginBuffer);
			
			if(url == QString::null) {
				dump2File("gmail_postlogin.html", mPostLoginBuffer);
				mPostLoginBuffer = "";
				
				emit loginDone(false, mLoginFromTimer, 
				       i18n("Unknown error retrieving cookies"));
			} else {
				kdDebug() << k_funcinfo << "Found an other redirect!: " << url << endl;
				mLoginLock->tryLock();
				postLogin(url);
			}
			
		}
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
					: "http" ),
				KURL::encode_string(Prefs::searchFor())
							 ).replace('@','%');
		else
			url = QString(gGAP4DCheckURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" ),
				useDomain,
				KURL::encode_string(Prefs::searchFor())
							 ).replace('@','%');

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

void GMail::slotCheckData(KIO::Job *job, const QByteArray &data)
{
	if(job->error() != 0) {
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		mPageBuffer.append(str);
	}
}

void GMail::slotCheckResult(KIO::Job *job)
{
	if(job->error() != 0) 
		kdWarning() << k_funcinfo << "error: " << job->errorString() << endl;

	kdDebug() << k_funcinfo << "Check finished." << endl;

	dump2File("gmail_data.html", mPageBuffer);
	
	static QRegExp rx("top\\.location=[\"\']http[s]?://www\\.google\\.com/accounts/ServiceLogin");
	static QRegExp rx2("gmail_error=[0-9]*;");
	int found;
	
	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
				<< rx.errorString() << endl;
	}
	
	if(!rx2.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX2!\n"
				<< rx2.errorString() << endl;
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
		
		return;
	} 
			
	found = rx2.search(mPageBuffer);
	
	if( found != -1 ) {
		kdWarning() << k_funcinfo << "Gmail is unavailable because of server-side errors!" << endl;
		
		mPageBuffer = "";
		mCheckLock->unlock();
		
		// let's try again in 60 seconds
		setInterval(60, true);
		return;
	}
	
	setInterval(mInterval, true);
	emit checkDone(mPageBuffer);
	mPageBuffer = "";
	
	mCheckLock->unlock();
}

///////////////////////////////////////////////////////////////////////////
// Other methods...
///////////////////////////////////////////////////////////////////////////
void GMail::slotTimeout()
{
	if(!isLoggedIn() || mLoginParamsChanged) {
		mLoginFromTimer = true;
		login();
	} else {
		// do the check
		mCheckFromTimer = true;
		checkGMail();
	}
}

void GMail::setInterval(unsigned int i, bool forceStart)
{
	bool running;
	
	if(i > Prefs::self()->intervalItem()->minValue().toUInt()) {
		mInterval = i;
		
		if (forceStart)
			running = true;
		else
			running = mTimer->isActive();
		
		mTimer->changeInterval(MILLISECS(mInterval));
		
		//Prevent starting the timer when it wasn't needed to
		if(!running)
			mTimer->stop();
	}
}

void GMail::setInterval(unsigned int i)
{
	setInterval(i, false);
}

bool GMail::isLoggedIn(bool lockCheck)
{
	bool ret = false;
	
	if( !lockCheck || ( lockCheck && !mLoginLock->locked() ) ) {
		if(cookieExists(ACTION_TOKEN_COOKIE))
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
	mCheckFromTimer = false;
	if(!isLoggedIn()) {
		login();
	} else {
		checkGMail();
	}
}

QString GMail::getURLPart()
{
	QString part;
	
	if(isGAP4D) {
		part = QString("a/%1").arg(useDomain);
	} else {
		part = "mail";
	}
	
	return part;
}

void GMail::logOut(bool force)
{
	static bool alreadyRunning = false;
	bool result;

	if(alreadyRunning || (!force && !isLoggedIn()))
		return;

	alreadyRunning = true;
	sessionCookie = QString::null;
	mTimer->stop();
	
	QString logoutUrl = (!isGAP4D)? gGMailLogOut : QString(gGAP4DLogOut).arg(useDomain);
	
	KIO::Job *job = KIO::get(logoutUrl, true, false);
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	kdDebug() << "Loging out! " << logoutUrl << endl;

	// we really don't want async jobs here
	result= KIO::NetAccess::synchronousRun(job, 0);
	kdDebug() << "Log out job done, with result: " << result << endl;
	alreadyRunning = false;
}

void GMail::logOut()
{
	logOut(false);
}

void GMail::slotLogOut()
{
	kdDebug() << k_funcinfo << endl;
	logOut(true);
}

void GMail::dump2File(const QString filename, const QString data)
{
#ifdef DUMP_PAGES
	QString dump_dir = DUMP_DIR;
		
	dump_dir += filename;
	
	kdDebug() << k_funcinfo << "Dumping data to file " << dump_dir << endl;
		
	QFile f(dump_dir);
		
	f.open( IO_WriteOnly );
		
	QTextStream stream(&f);
	stream << data;

	f.close();
#endif
}

bool GMail::setDomainAdvice(QString url, QString advice)
{
	QByteArray params;
	QDataStream stream(params, IO_WriteOnly);
	stream << url;
	stream << advice;
	
	if (!kapp->dcopClient()->send("kcookiejar", "kcookiejar",
	     "setDomainAdvice(QString,QString)", params))
	{
		kdWarning() << k_funcinfo << "There was some error using DCOP!" << endl;
		return false;
	}
	
	if (QString::compare(advice.lower(),getDomainAdvice(url).lower()) == 0) {
		return true;
	} else {
		return false;
	}
}

QString GMail::getDomainAdvice(QString url)
{
	QCString replyType;
	QByteArray params, reply;
	QDataStream stream(params, IO_WriteOnly);
	stream << url;
	
	if (!kapp->dcopClient()->call("kcookiejar", "kcookiejar",
	     "getDomainAdvice(QString)", params, replyType, reply))
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

bool GMail::areCookiesAllowed(QString url)
{
	QString advice;
	
	advice = getDomainAdvice(url);
	
	if (advice.compare("Accept") == 0) {
		return true;
	} else {
		return false;
	}
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
	
	if(ret && cookieName.compare(ACTION_TOKEN_COOKIE) == 0) {
		if(sessionCookie.compare(search.cap(2)) != 0) {
			QString oldSessionCookie;
			
			oldSessionCookie = sessionCookie;
			sessionCookie = search.cap(2);
			
			if(oldSessionCookie.length() != 0)
				emit sessionChanged();
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

QString GMail::getRedirectURL(QString buffer)
{	
	static QRegExp metaRX("<meta[ ]+.*url='(http[s]?://[^']+)'.*>");
	static QRegExp jsRX  ("location\\.replace[ ]*\\([ ]*['\"](http[s]?://[^'\"]+)['\"][ ]*\\)");
	int found;
	QString url, jsurl;
	
	kdDebug() << k_funcinfo << endl;

	if(!metaRX.isValid()) {
		kdWarning() << k_funcinfo << "Invalid metaRX!\n"
				<< metaRX.errorString() << endl;
	}

	if(!jsRX.isValid()) {
		kdWarning() << k_funcinfo << "Invalid jsRX!\n"
				<< jsRX.errorString() << endl;
	}
	
	found = metaRX.search(buffer);
			
	if( found == -1 ) {
		return QString::null;
	}
	
	url = KCharsets::resolveEntities(metaRX.cap(1));
	
	// now let's check if there's a JS redirection (location.replace)
	found = jsRX.search(buffer);
	
	if( found == -1 ) {
		return url;
	}
	jsurl = GMailParser::cleanUpData(jsRX.cap(1));
	
	// if both match it's ok
	if (url.compare(jsurl) == 0) {
		kdDebug() << k_funcinfo << "Found redirection to " << url << endl;
		return url;
	} else {
		// otherwise use JS redirection
		kdDebug() << k_funcinfo << "META and JS redirections do not match! META: " << url << " JS: " << jsurl << endl;
		return /*js*/url;
	}
	
}

#include "gmail.moc"
