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

#undef DUMP_HTML

#include "gmail.h"
#include "prefs.h"

#include <kio/job.h>
#include <kio/global.h>

#include <kdebug.h>

#include <qtimer.h>
#include <qregexp.h>
#include <qmutex.h>

static const QString gGMailLoginFormat = "https://gmail.google.com/accounts"
	"/ServiceLoginBoxAuth?Email=%s&Passwd=%s&null=Sign%%20in&service=mail";
static const QString gGMailCheckURL = "http://gmail.google.com/gmail?search=inbox&"
	"as_subset=unread&view=tl&start=0";

#define MILLISECS(x) (x * 1000)


#ifdef DUMP_HTML
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static FILE *gFP = 0;
#endif

GMail::GMail()
{
	mInterval = Prefs::interval();
	mCheckLock = new QMutex();
	mLoginLock = new QMutex();
	mCookieGV = 0;
	mMailCount = 0;
	mLoginParamsChanged = false;

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(slotTimeout()));

	mTimer->start(MILLISECS(mInterval));
}

GMail::~GMail()
{
	delete mCheckLock;
	delete mLoginLock;
	if(mCookieGV) {
		delete mCookieGV;
		mCookieGV = 0;
	}
#ifdef DUMP_HTML
	fclose(gFP);
#endif
}

void GMail::setLoginParams(const QString &username, const QString &password)
{
	if(mUsername != username || mPassword != password) {
		kdDebug() << k_funcinfo << "Login params changed.." << endl;
		mUsername = username;
		mPassword = password;
		
		if(mLoginLock->tryLock()) {
			if(mCookieGV)
				delete mCookieGV;
			mCookieGV = 0;
			mLoginLock->unlock();
			mLoginFromTimer = false;
			login();
		} else {
			kdDebug() << k_funcinfo << "scheduling login..." << endl;
			mLoginParamsChanged = true;
		}
	} else
		kdDebug() << k_funcinfo << "Login params have NOT changed" << endl;
}

void GMail::login()
{
	if(mLoginLock->tryLock()) {
		kdDebug() << k_funcinfo << "Starting Login.." << endl;
		emit startLogin();

		QString str;
		str.sprintf(gGMailLoginFormat.ascii(), 
					mUsername.ascii(), 
					mPassword.ascii());

		KIO::TransferJob *job = KIO::get(str, true, false);
		job->addMetaData("referrer", "http://www.google.com");
		job->addMetaData("cookies", "manual");
		job->addMetaData("cache", "reload");

		connect(job, SIGNAL(result(KIO::Job*)),
			SLOT(slotLoginResult(KIO::Job*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotLoginData(KIO::Job*, const QByteArray&)));
	}
}

void GMail::slotLoginResult(KIO::Job *job)
{
	kdDebug() << k_funcinfo << "Login process finished." << endl;
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "Login result: failed: " << job->errorString() << endl;
		mLoginLock->unlock();
		emit loginDone(false, mLoginFromTimer, job->errorString());
	} else {
		// from kopete/protocols/msn/msnnotifysocket.cpp
		QStringList cookielist = QStringList::split("\n", job->queryMetaData("setcookies"));
		mCookie = "Cookie: ";
		for(QStringList::Iterator it = cookielist.begin(); it != cookielist.end(); ++it) {
			QRegExp rx("Set-Cookie: ([^=]*)=([^;]*)");
			rx.search(*it);
			QString key = rx.cap(1);
			QString val = rx.cap(2);

			if(key.length() > 0) {
				mCookie += key + "=" + val + ";";
			}
		}
		kdDebug() << k_funcinfo << "Set-Cookie: " << mCookie << endl;

		if(mCookieGV) {
			mCookie += "GV=" + *mCookieGV + ";";
		}

		mLoginLock->unlock();

		kdDebug() << k_funcinfo << "Login ok=[" << (mCookieGV != 0) << "]" << endl;

		if(isLoggedIn()) {
			emit loginDone(true, mLoginFromTimer);
			mLoginParamsChanged = false;
			// Dont set mCheckFrom timer here, as we just logged in.
			checkGMail();
		} else
			emit loginDone(false, mLoginFromTimer, "Invalid username or password");
	}
}

void GMail::slotLoginData(KIO::Job *job, const QByteArray &data)
{
	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);
		QRegExp rx("cookieVal\\s*=\\s*\"([^\"]*)");
		if(rx.search(str) >= 0) {
			if(mCookieGV)
				delete mCookieGV;
			mCookieGV = new QString(rx.cap(1));
			kdDebug() << k_funcinfo << "GV=" << mCookieGV << endl;
		} else
			kdDebug() << k_funcinfo << "GV not found in [" << str << "]\n";
	}
}

#ifdef DUMP_HTML
bool gDumpStarted = false;
#endif

void GMail::slotCheckResult(KIO::Job *job)
{
	if(job->error() != 0) 
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;

	kdDebug() << k_funcinfo << "Check finished." << endl;
		
	mCheckLock->unlock();
	mTimer->start(MILLISECS(mInterval));
	emit stopCheck();
#ifdef DUMP_HTML
	gDumpStarted = false;
#endif
}

#ifdef DUMP_HTML
#include <qfile.h>
#endif


void GMail::slotCheckData(KIO::Job *job, const QByteArray &data)
{

	if(job->error() != 0) {
		kdDebug() << k_funcinfo << "error: " << job->errorString() << endl;
	} else {
		QCString str(data, data.size() + 1);

#ifdef DUMP_HTML
		QString myString(str);
		
		QFile f("/tmp/gmail.html");
		
		if(!gDumpStarted) {
			gDumpStarted = true;
			f.open( IO_WriteOnly | IO_Truncate );

		} else
			f.open( IO_WriteOnly | IO_Append );
		
		QTextStream stream( &f );
		stream << myString;

		f.close();

		sync();
#endif
		
		// TODO: collate all data, then do more intelligent parsing in
		// slotCheckResult.

		//D(["ds",NEW_EMAIL,0,0,0,0,0]
		QRegExp rx("D\\(\\[\"ds\",([0-9]*),");
		if(rx.search(str) >= 0) {
			unsigned int n = rx.cap(1).toUInt();
			if(n > mMailCount) {
				mMailCount = n;
				emit newMail(n);
			} else if(n != mMailCount) {
				mMailCount = n;
				emit mailCountChanged(n);
			}
			kdDebug() << k_funcinfo << "count=" << n << endl;
		}
	}
}

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
		kdDebug() << k_funcinfo << "Interval has changed" << endl;
		mInterval = i;
		mTimer->changeInterval(MILLISECS(mInterval));
	} else
		kdDebug() << k_funcinfo << "Interval has NOT changed" << endl;
}

void GMail::checkGMail()
{
	if(isLoggedIn() && mCheckLock->tryLock()) {
		kdDebug() << k_funcinfo << "Starting check..." << endl;
		// stop timer. start again when we have some sort of result.
		mTimer->stop();
		emit startCheck();

		KIO::TransferJob *job = KIO::get(gGMailCheckURL, true, false);
		job->addMetaData("cookies", "manual");
		job->addMetaData("setcookies", mCookie);
		job->addMetaData("cache", "reload");
		job->addMetaData("referrer", "http://gmail.google.com/gmail/html/hist2.html");

		connect(job, SIGNAL(result(KIO::Job*)),
			SLOT(slotCheckResult(KIO::Job*)));

		connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotCheckData(KIO::Job*, const QByteArray&)));
	}
}

bool GMail::isLoggedIn()
{
	bool ret = false;

	if(mLoginLock->tryLock()) {
		if(mCookieGV)
			ret = true;
		mLoginLock->unlock();
	}

	return ret;
}

void GMail::slotCheckGmail()
{
	kdDebug() << k_funcinfo << endl;
	if(!isLoggedIn()) {
		mLoginFromTimer = false;
		login();
	} else {
		mCheckFromTimer = false;
		checkGMail();
	}
}

#include "gmail.moc"
