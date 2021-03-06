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

#include <QMutex>
#include <QRegExp>
#include <QTimer>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusError>
//Added by qt3to4:
#include <QTextStream>
#include <QByteArray>

#include <kapplication.h>
#include <kcharsets.h>

#ifdef DUMP_PAGES
#include <QFile>
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


GMail::GMail(QObject* parent)
	: QObject(parent),
	  d(new Private())
{
	mInterval = Prefs::interval();
	mCheckLock = new QMutex();
	mLoginLock = new QMutex();
	mLoginParamsChanged = false;
	isGAP4D = false;
	
	//Any % should be replaced with @ due to a problem with QString not looking for escaped %
	gGMailLoginURL = "https://www.google.com/accounts/ServiceLogin?"
			"service=mail";
	gGMailAuthURL = "https://www.google.com/accounts/ServiceLoginAuth?"
			"service=mail";
	gGMailLoginPOSTFormat = "ltmpl=default&ltmplcache=2&continue="
			"http@3A@2F@2Fmail.google.com@2Fmail@2F@3F?ui@3Dhtml"
			"&service=mail&rm=false&scc=1&GALX=%1&Email=%2"
			"&Passwd=%3&PersistentCookie=yes&rmShown=1"
			"&signIn=Sign+in&asts=";
	gGMailCheckURL = "%1://mail.google.com/mail/?search=query"
			"&q=%2&as_subset=unread&view=tl&start=0&init=1&ui=1";
	gGMailLogOut = "https://mail.google.com/mail/?logout";
	
	gGAP4DLoginURL = "https://www.google.com/a/%1/ServiceLogin?"
			"service=mail";
	gGAP4DAuthURL = "https://www.google.com/a/%1/LoginAction?service=mail";
	gGAP4DLoginPOSTFormat = "ltmpl=default&ltmplcache=2&continue="
			"https@3A@2F@2Fmail.google.com@2Fa@2F%4@2F&service=mail"
			"&rm=false&ltmpl=default&hl=en&ltmpl=default&ss=1"
			"&GALX=%1&Email=%2&Passwd=%3&rmShown=1&signIn=Sign+in"
			"&asts=";
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
	bool isLocked;

	isLocked = true;
	if (mCheckLock->tryLock()) {
		mCheckLock->unlock();
		isLocked = false;
	}
	if (isLocked)
		mCheckLock->unlock();
	delete mCheckLock;
	mCheckLock = 0;

	isLocked = true;
	if (mLoginLock->tryLock()) {
		mLoginLock->unlock();
		isLocked = false;
	}
	if (isLocked)
		mLoginLock->unlock();
	delete mLoginLock;
	mLoginLock = 0;

	delete d;
	d = 0;
}

void GMail::slotSetWalletPassword(bool)
{
	kDebug() << k_funcinfo << "now, check login params.";
	checkLoginParams(true);
}

void GMail::checkLoginParams(bool passwordChanged)
{
	QString username = Prefs::gmailUsername();
	const QString& password = GMailWalletManager::instance()->getHash();
	
	if((mUsername == username && !passwordChanged)
		  || username.length() == 0)
		return;
	
	mUsername = username;
	mPasswordHash = password;
	sessionCookie = QString();
	
	useUsername = mUsername;
	useDomain = "";
	isGAP4D = false;
	
	if( mUsername.contains("@") ) {
		
		if ( QString::compare(mUsername.section("@",1),"gmail.com") != 0 &&
			QString::compare(mUsername.section("@",1),"googlemail.com") != 0) {
			kDebug() << k_funcinfo << mUsername << " seems to be a GAP4D account";
			isGAP4D = true;
			useDomain = mUsername.section("@",1);
		}
		useUsername = mUsername.section("@",0,0);
	}
	
	kDebug() << k_funcinfo << "Using " << useUsername << " as username and " << useDomain << " as domain";

	bool isLocked = true;
	if (mLoginLock->tryLock()) {
		mLoginLock->unlock();
		isLocked = false;
	}
	if(!isLocked) {
		
		//Try to log out if a session already exists (because it might be from another address)
		if(isLoggedIn(false)) {
			kDebug() << k_funcinfo << "A gmail session was already open, logging out from it";
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
		
		kDebug() << k_funcinfo << "Waiting for wallet...";
		// this will call back to gotWalletPassword().
		// we will continue the process from there.
		GMailWalletManager::instance()->get();
	}
}

QString GMail::findGALXCookie()
{
	bool result;
	int pos;
	QString cookies, cookieValue, loginURL;

	if (isGAP4D) {
		loginURL = QString(gGAP4DLoginURL).arg(useDomain);
	} else {
		loginURL = gGMailLoginURL;
	}

	KIO::Job *job = KIO::get(loginURL, KIO::Reload, KIO::HideProgressInfo);
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");

	result = KIO::NetAccess::synchronousRun(job, 0);
	if (result) {
		cookies = findCookies(loginURL);
		cookies += ";";

		QRegExp search(" (" + QRegExp::escape("GALX") + ")=([^;]*)");
		if(!search.isValid()) {
			kWarning() << "Invalid RX!\n" << search.errorString();
		}

		pos = search.indexIn(cookies);
		if (pos != -1) {
			cookieValue = search.cap(2);
			return cookieValue;
		}
	}
	return QString();
}

void GMail::slotGetWalletPassword(const QString& pass)
{
	QString str, LoginPOSTFormat, AuthURL;
	QString GALXValue;

	GALXValue = findGALXCookie();
	
	if(isGAP4D) {
		LoginPOSTFormat = QString(gGAP4DLoginPOSTFormat).replace("%4",useDomain);
		AuthURL = QString(gGAP4DAuthURL).arg(useDomain).replace('@','%');
	} else {
		LoginPOSTFormat = gGMailLoginPOSTFormat;
		AuthURL = gGMailAuthURL;
	}
	
	str = QString(LoginPOSTFormat).arg(
			QLatin1String(QUrl::toPercentEncoding(GALXValue)),
			QLatin1String(QUrl::toPercentEncoding(useUsername)),
			QLatin1String(QUrl::toPercentEncoding(pass)));
	str.replace('@','%');

	kDebug() << k_funcinfo << "Requesting login URL";

	loginRedirection = "";
	
	QByteArray postData(str.toUtf8());

	// get rid of terminating 0x0
	postData.truncate(postData.length() - 1);

	
	if (d->buffer) {
		kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen";
		return;
	}
	d->buffer = new QBuffer;
	d->buffer->open(QIODevice::WriteOnly);

	KIO::TransferJob *job = KIO::http_post(
			AuthURL,
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
		kWarning() << k_funcinfo << "error: " << job->errorString();
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

void GMail::slotLoginRedirection(KIO::Job *job, const KUrl &url)
{
	kDebug() << k_funcinfo << url.url();

	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString();
	} else {
		loginRedirection = url;
	}
}

void GMail::slotLoginResult(KJob *job)
{	
	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString();

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
		
		if( redirection == QString() ) {
			
			if(!isLoggedIn(false)) {
				if(mLoginBuffer.contains("onload") && (
						mLoginBuffer.contains("FixForm") ||
						mLoginBuffer.contains("start_time") )) {
				
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
					return;
				} else {
					kWarning() << k_funcinfo << " Redirection couldn't be found!";
					
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
			} else if (mLoginBuffer.contains("?ui=html") && (
						mLoginBuffer.contains("nocheckbrowser") ||
						mLoginBuffer.contains("noscript") )) {
				kDebug() << k_funcinfo << "Google is performing dirty JS check, bypassing it";
				
				if (loginRedirection.isEmpty()) {
					kWarning() << k_funcinfo << "loginRedirection is empty!";
					mLoginLock->unlock();
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				mLoginBuffer = "";
				
				KUrl _url;
				
				_url = loginRedirection;
				_url.setQuery("ui=html&zy=n");
				
				if (!_url.isValid()) {
					kWarning() << k_funcinfo << "New _url is invalid!:" << _url.url();
					mLoginLock->unlock();
					// let's show a nice error message to the user instead
					emit loginDone(false, mLoginFromTimer, i18n("GMail's login procedure has changed, check for new version"));
					return;
				}
				
				postLogin(_url.url());
				
				
			} else {
				//no more redirections?
				kWarning() << k_funcinfo << "No redirection was found, but seems like we are logged in!";
				
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
	bool isLocked = true;
	if (mLoginLock->tryLock()) {
		mLoginLock->unlock();
		isLocked = false;
	}
	// this is expected to be locked.
	if(isLocked) {
		
		KUrl _url(url);
		QRegExp rx("google\\..+");
		QString host;

		if (!_url.isValid()) {
			kError() <<  "tried to go to an invalid url!: " << url << endl;
		}

		if(_url.protocol().compare("https://") != 0 && Prefs::useHTTPS()) {
			kDebug() << k_funcinfo << "Forcing https on " << _url << endl;
			_url.setProtocol("https");
			host = _url.host();
			// avoid SSL cert errors by forcing the .com domain
			host.replace(rx, "google.com");
			_url.setHost(host);
		}
		
		mPostLoginBuffer = "";
		
		if (d->buffer) {
			kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen";
			return;
		}

		d->buffer = new QBuffer;
		d->buffer->open(QIODevice::WriteOnly);

		kDebug() << k_funcinfo << "Starting job to " << _url;


		KIO::TransferJob *job = KIO::get(_url, KIO::Reload, KIO::HideProgressInfo);
		job->addMetaData("cookies", "auto");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KJob*)),
			SLOT(slotPostLoginResult(KJob*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotPostLoginData(KIO::Job*, const QByteArray&)));
	} else {
		kWarning() << k_funcinfo << "mLoginLock is not locked!";
	}
}

void GMail::slotPostLoginData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString();
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

void GMail::slotPostLoginResult(KJob *job)
{
	if(job->error() != 0) {
		kWarning() << k_funcinfo << "error: " << job->errorString();

		delete d->buffer;
		d->buffer = 0;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		
		mLoginLock->unlock();
		
		mPostLoginBuffer = QString(d->buffer->data());
		mPostLoginBuffer.detach();

		delete d->buffer;
		d->buffer = 0;

		if(isLoggedIn()) {
			mPostLoginBuffer = "";
			emit loginDone(true, mLoginFromTimer);
			checkGMail();
		} else {
			QString url = getRedirectURL(mPostLoginBuffer);
			
			if(url == QString()) {
				dump2File("gmail_postlogin.html", mPostLoginBuffer);
				mPostLoginBuffer = "";
				
				emit loginDone(false, mLoginFromTimer, 
				       i18n("Unknown error retrieving cookies"));
			} else {
				kDebug() << k_funcinfo << "Found another redirect!: " << url;
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
		kDebug() << k_funcinfo << "Starting check...";
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
			kDebug() << k_funcinfo << "d->buffer isn't empty. Shouldn't happen";
		return;
		}
		d->buffer = new QBuffer;
		d->buffer->open(QIODevice::WriteOnly);

		kDebug() << k_funcinfo << "GET: " << url;

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
		kWarning() << k_funcinfo << "error: " << job->errorString();
	} else {
		d->buffer->write(data.data(), data.size());
	}
}

// TODO: Refactor
void GMail::slotCheckResult(KJob *job)
{
	if(job->error() != 0) {
		// TODO: We should notify the user
		kWarning() << k_funcinfo << "error: " << job->errorString();

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

		kDebug() << k_funcinfo << "Check finished.";

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

		found = rx.indexIn(mPageBuffer);

		if( found != -1 || !isLoggedIn() ) {
			kWarning() << k_funcinfo << "User is not logged in!";

			mPageBuffer = "";
			mCheckLock->unlock();

			//Clearing values will force login
			mUsername = "";
			mPasswordHash = "";
			checkLoginParams(true);

			return;
		}

		found = rx2.indexIn(mPageBuffer);

		if( found != -1 ) {
			kWarning() << k_funcinfo << "Gmail is unavailable because of server-side errors!";

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
		mLoginParamsChanged = false;
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
		
		mTimer->start(MILLISECS(mInterval));
		
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
	
	bool isLocked = true;
	if (mLoginLock->tryLock()) {
		mLoginLock->unlock();
		isLocked = false;
	}
	if( !lockCheck || ( lockCheck && !isLocked ) ) {
		if(cookieExists(ACTION_TOKEN_COOKIE))
			ret = true;
		else kDebug() << k_funcinfo << ACTION_TOKEN_COOKIE << " wasn't found!";
	} else kDebug() << "mLoginLock is locked";

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
	sessionCookie = QString();
	mTimer->stop();
	
	QString logoutUrl = (!isGAP4D)? gGMailLogOut : QString(gGAP4DLogOut).arg(useDomain);
	
	KIO::Job *job = KIO::get(logoutUrl, KIO::Reload, KIO::HideProgressInfo);
	job->addMetaData("cookies", "auto");
	job->addMetaData("cache", "reload");
	kDebug() << "Loging out! " << logoutUrl;

	// we really don't want async jobs here
	result= KIO::NetAccess::synchronousRun(job, 0);
	kDebug() << "Log out job done, with result: " << result;
	alreadyRunning = false;
	emit loggedOut();
}

void GMail::logOut()
{
	logOut(false);
}

void GMail::slotLogOut()
{
	kDebug() << k_funcinfo;
	logOut(true);
}

void GMail::dump2File(const QString filename, const QString data)
{
#ifdef DUMP_PAGES
	QString dump_dir = DUMP_DIR;
		
	dump_dir += filename;
	
	kDebug() << k_funcinfo << "Dumping data to file " << dump_dir;
		
	QFile f(dump_dir);
		
	f.open( QIODevice::WriteOnly );
		
	QTextStream stream(&f);
	stream << data;

	f.close();
#else
	// Avoid compiler warnings about unused variables
	Q_UNUSED (filename);
	Q_UNUSED (data);
#endif
}

bool GMail::setDomainAdvice(QString url, QString advice)
{
	QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
	QDBusReply<void> reply = kded.call("setDomainAdvice", url, advice);
	if (!reply.isValid()) {
		QDBusError error = reply.error();
		kWarning() << k_funcinfo << "D-BUS error while calling setDomainAdvice";
		kDebug() << "Error description:" << error.message();
		return false;
	}
	
	if (QString::compare(advice.toLower(),getDomainAdvice(url).toLower()) == 0) {
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
		kWarning() << k_funcinfo << "D-BUS error while calling getDomainAdvice";
		kDebug() << "Error description:" << error.message();
		return QString();
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
		kWarning() << k_funcinfo << "D-BUS error while calling findCookies";
		kWarning() << "Error description:" << error.message();
		return QString();
	}
	return reply.value();
}

bool GMail::cookieExists(QString cookieName,QString url)
{
	kDebug() << k_funcinfo << "Searching for cookie " << cookieName << " at " << url;
	
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
	
	 
	found = search.indexIn(cookies);
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
	
	kDebug() << cookieName << " was " << (ret? "FOUND":"NOT FOUND");
	
	return ret;
}

bool GMail::cookieExists(QString cookieName)
{
	return cookieExists(cookieName, QString("https://mail.google.com/%1/").arg(getURLPart()));
}

void GMail::slotSessionChanged()
{
	logOut();
	
	bool isLocked = true;
	if (mCheckLock->tryLock()) {
		mCheckLock->unlock();
		isLocked = false;
	}
	if (isLocked)
		mCheckLock->unlock();
	
	//Clearing values will force login
	mUsername = "";
	mPasswordHash = "";
	checkLoginParams(true);
}

QString GMail::getRedirectURL(QString buffer)
{	
	static QRegExp metaRX("<meta[ ]+.*url=('|\\\"|&#39;|&#34;)(http[s]?://.+)\\1.*>");
	static QRegExp jsRX  ("location\\.replace[ ]*\\([ ]*(['\\\"])(http[s]?://.+)\\1[ ]*\\)");
	int found;
	QString url, jsurl;
	
	kDebug() << k_funcinfo;

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
	
	found = metaRX.indexIn(buffer);
			
	if( found == -1 ) {
		return QString();
	}
	
	url = KCharsets::resolveEntities(metaRX.cap(2));
	
	// now let's check if there's a JS redirection (location.replace)
	found = jsRX.indexIn(buffer);
	
	if( found == -1 ) {
		return url;
	}
	jsurl = GMailParser::cleanUpData(jsRX.cap(2));
	
	// if both match it's ok
	if (url.compare(jsurl) == 0) {
		kDebug() << k_funcinfo << "Found redirection to " << url;
		return url;
	} else {
		// otherwise use JS redirection
		kDebug() << k_funcinfo << "META and JS redirections do not match! META: " << url << " JS: " << jsurl;
		return /*js*/url;
	}
	
}

#include "gmail.moc"
