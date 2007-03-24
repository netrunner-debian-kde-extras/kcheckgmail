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
#ifndef GMAIL_H
#define GMAIL_H

#include <qobject.h>
#include <qstring.h>

#include "gmailwalletmanager.h"

namespace KIO { class Job; }

class QTimer;
class QMutex;
template<class Key, class Value> class QMap;

/**
@author Matthew Wlazlo
*/
class GMail : public QObject
{
	Q_OBJECT
public:
	GMail();
	virtual ~GMail();

	void checkLoginParams();
	void setInterval(unsigned int i);

	bool isLoggedIn(bool lockCheck);
	bool isLoggedIn();
	bool isChecking();
	
	QString getURLPart(bool refresh);
	QString getURLPart();
	
	void gotWalletPassword(QString str);

protected:
	void login();
	void postLogin();
	void checkGMail();
	void logOut();
	
	void dump2File(const QString filename, const QString data);
	
	bool cookieExists(QString cookieName, QString url);
	bool cookieExists(QString cookieName);

private:
	unsigned int mInterval;
	QMutex *mCheckLock;
	QMutex *mLoginLock;

	bool mLoginParamsChanged;

	// true if timer spawned this check/login attempt.
	bool mLoginFromTimer;
	bool mCheckFromTimer;
	
	QString useDomain;
	QString useUsername;
	bool isGAP4D;
	
	QString sessionCookie;
	
	QString mUsername;
	QString mPasswordHash;
	unsigned int mLoginToken;
	QString mPageBuffer;
	QString mLoginBuffer;
	
	QString findCookies(QString url);
	QString mCookiesCache;
	
	QTimer *mTimer;
	
	//Normal GMail
	QString gGMailLoginURL, gGMailLoginPostFormat,
	 			gGMailCheckURL, gGMailPostLoginURLFormat, gGMailPostLoginURL, gGMailLogOut;

	//GAP4D: Google Applications for Domains
	QString gGAP4DLoginURL, gGAP4DLoginPostFormat,
				gGAP4DCheckURL, gGAP4DPostLoginURLFormat, gGAP4DLogOut;

public slots:
	void slotCheckGmail();
	void slotGetWalletPassword(const QString&);
	void slotSetWalletPassword(bool);
	void slotLogOut();

protected slots:
	void slotLoginResult(KIO::Job*);
	void slotLoginData(KIO::Job*, const QByteArray&);

	void slotPostLoginResult(KIO::Job*);
	void slotPostLoginData(KIO::Job*, const QByteArray&);

	void slotTimeout();

	void slotCheckResult(KIO::Job*);
	void slotCheckData(KIO::Job*, const QByteArray&);
	
private slots:
	void slotSessionChanged();

signals:
	void loginStart();
	void loginDone(bool success, bool spawnedFromTimer, const QString &why = QString::null);

	void checkStart();
	void checkDone(const QString &data);
	
	void sessionChanged();
};

#endif
