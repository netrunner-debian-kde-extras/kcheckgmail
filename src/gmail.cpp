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

// define this symbol if you want to write incoming HTML to disc
#undef DUMP_HTML
#define DUMP_HTML_FILE "/tmp/gmail.html"

#include "gmail.h"
#include "gmailwalletmanager.h"
#include "prefs.h"

#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <kdebug.h>

#include <qmap.h>
#include <qmutex.h>
#include <qregexp.h>
#include <qtimer.h>

#include <time.h>

#ifdef DUMP_HTML
#include <qfile.h>
#endif

static const QString 
gGMailLoginURL = "https://www.google.com/accounts/ServiceLoginBoxAuth",
gGMailLoginPostFormat = "Email=%s&Passwd=%s&null=Sign%%20in&service=mail"
	"&continue=https://gmail.google.com/gmail",
	
gGMailCheckURL = "http://gmail.google.com/gmail?search=inbox"
	"&as_subset=unread&view=tl&start=0",
gGMailPostLoginURLFormat = "http://gmail.google.com/gmail?_sgh=%s";


#define MILLISECS(x) (x * 1000)


GMail::GMail() : QObject(0, "GMailNetwork")
{
	mInterval = Prefs::interval();
	mCheckLock = new QMutex();
	mLoginLock = new QMutex();
	mCookieMap = new QMap<QString,QString>();
	mLoginToken = 0;
	mLoginParamsChanged = false;

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(slotTimeout()));

	mTimer->start(MILLISECS(mInterval));
}

GMail::~GMail()
{
	delete mCheckLock;
	delete mLoginLock;
	if(mLoginToken) {
		delete mLoginToken;
		mLoginToken = 0;
	}
	delete mCookieMap;
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
		
		if(mLoginLock->tryLock()) {
			if(mLoginToken)
				delete mLoginToken;
			mLoginToken = 0;
			mLoginLock->unlock();
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
	QString str;
	str.sprintf(gGMailLoginPostFormat.ascii(), 
				mUsername.ascii(), 
				pass.ascii());
	kdDebug() << "Sending login: " << str << endl;

	QCString b(str.utf8());
	QByteArray postData(b);

	// get rid of terminating 0x0
	postData.truncate(b.length());

	KIO::TransferJob *job = KIO::http_post(
		gGMailLoginURL,
		postData, false);
	job->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
	job->addMetaData("cookies", "manual");
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

		mCookieMap->clear();
		
		QString cookie;
		long int t = time(NULL);
		cookie.sprintf("T%ld/%ld/%ld", t - 2, t - 1, t);

		parseCookies("Set-Cookie: GMAIL_LOGIN="+cookie+";");

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
		parseCookies(job->queryMetaData("setcookies"));

		if(mLoginToken) {
			postLogin();
			// NOTE: LoginLock is still locked()
		} else {
			mLoginLock->unlock();
			kdDebug() << "Cookies=" << cookieString() << endl;
			emit loginDone(false, mLoginFromTimer, i18n("Invalid username or password"));
		}
	}
}

void GMail::slotLoginData(KIO::Job *job, const QByteArray &data)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		parseCookies(job->queryMetaData("setcookies"));
		QRegExp rx("_sgh%3[Dd](.*)&service=mail");
		if(rx.search(str) >= 0) {
			if(mLoginToken)
				delete mLoginToken;
			mLoginToken = new QString(rx.cap(1));
		}
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
		url.sprintf(gGMailPostLoginURLFormat.ascii(), mLoginToken->ascii());

		KIO::TransferJob *job = KIO::get(url, true, false);
		job->addMetaData("cookies", "manual");
		job->addMetaData("setcookies", cookieString());
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KIO::Job*)),
			SLOT(slotPostLoginResult(KIO::Job*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotPostLoginData(KIO::Job*, const QByteArray&)));
	} else
		mLoginLock->unlock();
}

void GMail::slotPostLoginResult(KIO::Job *job)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;

		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		parseCookies(job->queryMetaData("setcookies"));
		kdDebug() << k_funcinfo << "Finally, " << cookieString() << endl;
		
		mLoginLock->unlock();

		if(isLoggedIn()) {
			emit loginDone(true, mLoginFromTimer);
			checkGMail();
		} else
			emit loginDone(false, mLoginFromTimer, 
				i18n("Unknown error retrieving cookies"));
	}
}

void GMail::slotPostLoginData(KIO::Job *job, const QByteArray &)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		parseCookies(job->queryMetaData("setcookies"));
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

		KIO::TransferJob *job = KIO::get(gGMailCheckURL, true, false);
		job->addMetaData("cookies", "manual");
		job->addMetaData("setcookies", cookieString());
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

#ifdef DUMP_HTML
		QFile f(DUMP_HTML_FILE);
		
		f.open( IO_WriteOnly | IO_Append );
		
		QTextStream stream(&f);
		stream << mPageBuffer;
		stream << endl << "##################### END DUMP" << endl 
			<< endl;

		f.close();
#endif

		
	mTimer->start(MILLISECS(mInterval));
	emit checkDone(mPageBuffer);
	mPageBuffer = "";

	mCheckLock->unlock();

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

bool GMail::isLoggedIn()
{
	bool ret = false;

	if(!mLoginLock->locked()) {
		if(!mCookieMap->isEmpty() && mCookieMap->find("GMAIL_AT") != mCookieMap->end())
			ret = true;
	}

	return ret;
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

QString GMail::cookieString()
{
	QString ret = "Cookie: ";
	QValueList<QString> klist = mCookieMap->keys();
	
	QValueList<QString>::iterator iter = klist.begin();

	while(iter != klist.end()) {
		ret += *iter + "=" + (*mCookieMap)[*iter];
		iter ++;
		if(iter != klist.end())
			ret += "; ";
	}

	return ret;
}

void GMail::parseCookies(const QString &str)
{
	// from kopete/protocols/msn/msnnotifysocket.cpp
	QStringList cookielist = QStringList::split("\n", str);
	QRegExp rx("Set-Cookie: ([^=]*)=([^;]*)");
	for(QStringList::Iterator it = cookielist.begin(); it != cookielist.end(); ++it) {
		rx.search(*it);
		QString key = rx.cap(1);
		QString val = rx.cap(2);

		if(key.length() > 0) {
			mCookieMap->insert(key, val);
		}
	}
}


#include "gmail.moc"
