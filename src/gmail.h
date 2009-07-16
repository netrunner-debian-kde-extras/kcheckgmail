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
#ifndef GMAIL_H
#define GMAIL_H

#include <qobject.h>
#include <qstring.h>
#include <kurl.h>

#include "gmailwalletmanager.h"

namespace KIO { class Job; }
class KJob;

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
	GMail(QObject* parent = 0);
	virtual ~GMail();

	void checkLoginParams();
	void setInterval(unsigned int i);
	void setInterval(unsigned int i, bool forceStart);

	bool isLoggedIn(bool lockCheck);
	bool isLoggedIn();
	bool isChecking();
	
	QString getURLPart();
	
	void gotWalletPassword(QString str);

protected:
	void login();
	void postLogin(QString url);
	void postLogin();
	void checkGMail();
	QString getRedirectURL(QString buffer);
	void logOut(bool force);
	void logOut();
	
	void dump2File(const QString filename,const QString data);
	
	bool cookieExists(QString, QString);
	bool cookieExists(QString);
	bool areCookiesAllowed(QString);
	QString getDomainAdvice(QString);
	bool setDomainAdvice(QString, QString);

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
	QString mPageBuffer;
	QString mLoginBuffer;
	QString mPostLoginBuffer;
	
	QString findCookies(QString url);
	QString mCookiesCache;
	
	QTimer *mTimer;
	
	//Normal GMail
	QString gGMailLoginURL, gGMailLoginPOSTFormat, gGMailCheckURL, gGMailLogOut;

	//GAP4D: Google Applications for Domains
	QString gGAP4DLoginURL, gGAP4DLoginPOSTFormat, gGAP4DCheckURL, gGAP4DLogOut;

	KUrl loginRedirection;

	class Private;
	Private* d;
	
public slots:
	void slotCheckGmail();
	void slotGetWalletPassword(const QString&);
	void slotSetWalletPassword(bool);
	void slotLogOut();

protected slots:
	void slotLoginResult(KJob*);
	void slotLoginData(KIO::Job*, const QByteArray&);
	void slotLoginRedirection(KIO::Job *job, const KUrl &url);

	void slotPostLoginResult(KJob*);
	void slotPostLoginData(KIO::Job*, const QByteArray&);

	void slotTimeout();

	void slotCheckResult(KJob*);
	void slotCheckData(KIO::Job*, const QByteArray&);
	
private slots:
	void slotSessionChanged();

signals:
	void loginStart();
	void loginDone(bool success, bool spawnedFromTimer, const QString &why = QString());

	void checkStart();
	void checkDone(const QString &data);
	
	void sessionChanged();
};

#endif
