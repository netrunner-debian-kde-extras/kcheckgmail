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
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusError>
//Added by qt3to4:
#include <Q3TextStream>
#include <QByteArray>

#include <kapplication.h>
#include <kcharsets.h>

#ifdef DUMP_PAGES
#include <qfile.h>
#endif

#define MILLISECS(x) (x * 1000)

class GMail::Private {
public:
	Private()
	  : buffer(0)
	{
	}

	~Private()
	{
		delete buffer;
		buffer = 0;
	}

	QBuffer *buffer;
};


GMail::GMail(QObject* parent, const char* name)
	: QObject(parent),
	  d(new Private())
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

	delete d;
	d = 0;
}

void GMail::slotSetWalletPassword(bool)
{
	kDebug() << k_funcinfo << "now, check login params." << endl;
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
	
	if( mUsername.contains("@") ) {
		
		if ( QString::compare(mUsername.section("@",1),"gmail.com") != 0 &&
			QString::compare(mUsername.section("@",1),"googlemail.com") != 0) {
			kDebug() << k_funcinfo << mUsername << " seems to be a GAP4D account" << endl;
			isGAP4D = true;
			useDomain = mUsername.section("@",1);
		}
		useUsername = mUsername.section("@",0,0);
	}
	
	kDebug() << k_funcinfo << "Using " << useUsername << " as username and " << useDomain << " as domain" << endl;
	
	if(!mLoginLock->locked()) {
		
		//Try to log out if a session already exists (because it might be from another address)
		if(isLoggedIn(false)) {
			kDebug() << k_funcinfo << "A gmail session was already open, logging out from it" << endl;
			logOut(true);
		}
		
		mLoginFromTimer = false;
		login();
	} else {
		kDebug() << k_funcinfo << "Login in process. "
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
		
		kDebug() << k_funcinfo << "Waiting for wallet..." << endl;
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
			QLatin1String(QUrl::toPercentEncoding(useUsername)),
			QLatin1String(QUrl::toPercentEncoding(pass)));
	str.replace('@','%');

	kDebug() << k_funcinfo << "Requesting login URL" << endl;

	loginRedirection = "";
	
	QByteArray postData(str.toUtf8());

	// get rid of terminating 0x0
	postData.truncate(postData.length() - 1);

	
	if (d->buffer) {
		kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen" << endl;
		return;
	}
	d->buffer = new QBuffer;
	d->buffer->open(QIODevice::WriteOnly);

	KIO::TransferJob *job = KIO::http_post(
			LoginURL,
			postData,
			KIO::HideProgressInfo);
	job->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	
	connect(job, SIGNAL(result(KJob*)),
		SLOT(slotLoginResult(KJob*)));
	
	connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
		SLOT(slotLoginData(KIO::Job*, const QByteArray&)));
	
	connect(job, SIGNAL(redirection(KIO::Job*, const KUrl&)),
		SLOT(slotLoginRedirection(KIO::Job*, const KUrl&)));
}

void GMail::slotLoginData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

void GMail::slotLoginRedirection(KIO::Job *job, const KUrl &url)
{
	kDebug() << k_funcinfo << url.url() << endl;

	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		loginRedirection = url;
	}
}

void GMail::slotLoginResult(KJob *job)
{	
	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;

		delete d->buffer;
		d->buffer = 0;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		QString redirection;
		
		mLoginBuffer = QString(d->buffer->data());
		mLoginBuffer.detach();

		delete d->buffer;
		d->buffer = 0;

		redirection = getRedirectURL(mLoginBuffer);
		dump2File("gmail_login.html", mLoginBuffer);
		
		if( redirection == QString::null ) {
			
			if(!isLoggedIn(false)) {
				if(mLoginBuffer.contains("onload") && (
						mLoginBuffer.contains("FixForm") ||
						mLoginBuffer.contains("start_time") )) {
				
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
					return;
				} else {
					kWarning() << k_funcinfo << " Redirection couldn't be found!" << endl;
					
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
			} else if (mLoginBuffer.contains("?ui=html") && (
						mLoginBuffer.contains("nocheckbrowser") ||
						mLoginBuffer.contains("noscript") )) {
				kDebug() << k_funcinfo << "Google is performing dirty JS check, bypassing it" << endl;
				
				if (loginRedirection.isEmpty()) {
					kWarning() << k_funcinfo << "loginRedirection is empty!" << endl;
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				mLoginBuffer = "";
				
				KUrl _url;
				
				_url = loginRedirection;
				_url.setQuery("ui=html&zy=n");
				
				if (!_url.isValid()) {
					kWarning() << k_funcinfo << "New _url is invalid!:" << _url.url() << endl;
					mLoginLock->unlock();
					// let's show a nice error message to the user instead
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				postLogin(_url.url());
				
				
			} else {
				//no more redirections?
				kWarning() << k_funcinfo << "No redirection was found, but seems like we are logged in!" << endl;
				
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
			kWarning() << k_funcinfo << "Invalid RX!\n"
					<< rx.errorString() << endl;
		}
		
		found = rx.search(url);
		
		if(found == -1) {
			kWarning() <<  "This can't be a valid url!: " << url << endl;
			if (!KUrl(url).isValid()) {
				kError() <<  "This is absolutely a non-valid URL!: " << url << endl;
			}
		}
		
		if(rx.cap(1).compare("https://") != 0) {
			url = (Prefs::useHTTPS()? "https" : "http" );
			url.append("://");
			url.append(rx.cap(2));
		}
		
		mPostLoginBuffer = "";
		
		if (d->buffer) {
			kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen" << endl;
			return;
		}

		d->buffer = new QBuffer;
		d->buffer->open(QIODevice::WriteOnly);

		kDebug() << k_funcinfo << "Starting job to " << url << endl;

		KIO::TransferJob *job = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
		job->addMetaData("cookies", "auto");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KJob*)),
			SLOT(slotPostLoginResult(KJob*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotPostLoginData(KIO::Job*, const QByteArray&)));
	} else {
		kWarning() << k_funcinfo << "mLoginLock is not locked!" << endl;
	}
}

void GMail::slotPostLoginData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

void GMail::slotPostLoginResult(KJob *job)
{
	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;

		delete d->buffer;
		d->buffer = 0;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		mLoginLock->unlock();
		
		if(isLoggedIn()) {
			delete d->buffer;
			d->buffer = 0;

			mPostLoginBuffer = "";
			emit loginDone(true, mLoginFromTimer);
			checkGMail();
			
		} else {
			mPostLoginBuffer = QString(d->buffer->data());
			mPostLoginBuffer.detach();

			delete d->buffer;
			d->buffer = 0;

			QString url = getRedirectURL(mPostLoginBuffer);
			
			if(url == QString::null) {
				dump2File("gmail_postlogin.html", mPostLoginBuffer);
				mPostLoginBuffer = "";
				
				emit loginDone(false, mLoginFromTimer, 
				       i18n("Unknown error retrieving cookies"));
			} else {
				kDebug() << k_funcinfo << "Found another redirect!: " << url << endl;
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
		kDebug() << k_funcinfo << "Starting check..." << endl;
		// stop timer. start again when we have some sort of result.
		mTimer->stop();
		emit checkStart();

		QString url;
		QLatin1String prefsUrl = QLatin1String(
				QUrl::toPercentEncoding(Prefs::searchFor()));

		if(!isGAP4D) {
			url = QString(gGMailCheckURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" ),
				prefsUrl);
		} else {
			url = QString(gGAP4DCheckURL).arg(
				(Prefs::useHTTPS()
					? "https" 
					: "http" ),
				useDomain,
				prefsUrl);
		}
		url.replace('@','%');

		if (d->buffer) {
			kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen" << endl;
		return;
		}
		d->buffer = new QBuffer;
		d->buffer->open(QIODevice::WriteOnly);

		kDebug() << k_funcinfo << "GET: " << url << endl;

		KIO::TransferJob *job = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
		job->addMetaData("cookies", "auto");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KJob*)),
			SLOT(slotCheckResult(KJob*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotCheckData(KIO::Job*, const QByteArray&)));
	}
}

void GMail::slotCheckData(KIO::Job *job, const QByteArray &data)
{
	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

// TODO: Refactor
void GMail::slotCheckResult(KJob *job)
{
	if(job->error() != 0) {
		// TODO: We should notify the user
		kWarning() << k_funcinfo << "error: " << job->errorString() << endl;

		delete d->buffer;
		d->buffer = 0;
		mCheckLock->unlock();

		// let's try again in 60 seconds
		setInterval(60, true);
	} else {
		mPageBuffer = QString::fromUtf8(d->buffer->data());
		mPageBuffer.detach();

		delete d->buffer;
		d->buffer = 0;

		kDebug() << k_funcinfo << "Check finished." << endl;

		dump2File("gmail_data.html", mPageBuffer);

		static QRegExp rx("top\\.location=[\"\']http[s]?://www\\.google\\.com/accounts/ServiceLogin");
		static QRegExp rx2("gmail_error=[0-9]*;");
		int found;

		if(!rx.isValid()) {
			kWarning() << k_funcinfo << "Invalid RX!\n"
					<< rx.errorString() << endl;
		}

		if(!rx2.isValid()) {
			kWarning() << k_funcinfo << "Invalid RX2!\n"
					<< rx2.errorString() << endl;
		}

		found = rx.search(mPageBuffer);

		if( found != -1 || !isLoggedIn() ) {
			kWarning() << k_funcinfo << "User is not logged in!" << endl;

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
			kWarning() << k_funcinfo << "Gmail is unavailable because of server-side errors!" << endl;

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
		else kDebug() << k_funcinfo << ACTION_TOKEN_COOKIE << " wasn't found!" << endl;
	} else kDebug() << "mLoginLock is locked" << endl;

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
	
	KIO::Job *job = KIO::get(logoutUrl, KIO::Reload, KIO::HideProgressInfo);
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	kDebug() << "Loging out! " << logoutUrl << endl;

	// we really don't want async jobs here
	result= KIO::NetAccess::synchronousRun(job, 0);
	kDebug() << "Log out job done, with result: " << result << endl;
	alreadyRunning = false;
}

void GMail::logOut()
{
	logOut(false);
}

void GMail::slotLogOut()
{
	kDebug() << k_funcinfo << endl;
	logOut(true);
}

void GMail::dump2File(const QString filename, const QString data)
{
#ifdef DUMP_PAGES
	QString dump_dir = DUMP_DIR;
		
	dump_dir += filename;
	
	kDebug() << k_funcinfo << "Dumping data to file " << dump_dir << endl;
		
	QFile f(dump_dir);
		
	f.open( QIODevice::WriteOnly );
		
	Q3TextStream stream(&f);
	stream << data;

	f.close();
#endif
}

bool GMail::setDomainAdvice(QString url, QString advice)
{
	QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
	QDBusReply<void> reply = kded.call("setDomainAdvice", url, advice);
	if (!reply.isValid()) {
		QDBusError error = reply.error();
		kWarning() << k_funcinfo << "D-BUS error while calling setDomainAdvice" << endl;
		kDebug() << "Error description:" << error.message() << endl;
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
	QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
	QDBusReply<QString> reply = kded.call("getDomainAdvice", url);
	
	if (!reply.isValid())
	{
		QDBusError error = reply.error();
		kWarning() << k_funcinfo << "D-BUS error while calling getDomainAdvice" << endl;
		kDebug() << "Error description:" << error.message() << endl;
		return QString::null;
	}

	return reply.value();
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
	QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
	QDBusReply<QString> reply = kded.call("findCookies", url, qlonglong(0));
	if (!reply.isValid())
	{
		QDBusError error = reply.error();
		kWarning() << k_funcinfo << "D-BUS error while calling findCookies" << endl;
		kWarning() << "Error description:" << error.message() << endl;
		return QString::null;
	}
	return reply.value();
}

bool GMail::cookieExists(QString cookieName,QString url)
{
	kDebug() << k_funcinfo << "Searching for cookie " << cookieName << " at " << url << endl;;
	
	QString cookies;
	int found;
	bool ret = false;
	
	cookies = findCookies(url);
	
	if(cookies.length() == 0)
		return false;
	
	cookies += ";";
	
	QRegExp search(" (" + QRegExp::escape(cookieName) + ")=([^;]*)");
	
	if(!search.isValid()) {
		kWarning() << k_funcinfo << "Invalid RX!\n"
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
	
	kDebug() << cookieName << " was " << (ret? "FOUND":"NOT FOUND") << endl;
	
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
	static QRegExp metaRX("<meta[ ]+.*url=('|\\\"|&#39;|&#34;)(http[s]?://.+)\\1.*>");
	static QRegExp jsRX  ("location\\.replace[ ]*\\([ ]*(['\\\"])(http[s]?://.+)\\1[ ]*\\)");
	int found;
	QString url, jsurl;
	
	kDebug() << k_funcinfo << endl;

	metaRX.setMinimal(true);
	if(!metaRX.isValid()) {
		kWarning() << k_funcinfo << "Invalid metaRX!\n"
				<< metaRX.errorString() << endl;
	}

	jsRX.setMinimal(true);
	if(!jsRX.isValid()) {
		kWarning() << k_funcinfo << "Invalid jsRX!\n"
				<< jsRX.errorString() << endl;
	}
	
	found = metaRX.search(buffer);
			
	if( found == -1 ) {
		return QString::null;
	}
	
	url = KCharsets::resolveEntities(metaRX.cap(2));
	
	// now let's check if there's a JS redirection (location.replace)
	found = jsRX.search(buffer);
	
	if( found == -1 ) {
		return url;
	}
	jsurl = GMailParser::cleanUpData(jsRX.cap(2));
	
	// if both match it's ok
	if (url.compare(jsurl) == 0) {
		kDebug() << k_funcinfo << "Found redirection to " << url << endl;
		return url;
	} else {
		// otherwise use JS redirection
		kDebug() << k_funcinfo << "META and JS redirections do not match! META: " << url << " JS: " << jsurl << endl;
		return /*js*/url;
	}
	
}

#include "gmail.moc"
