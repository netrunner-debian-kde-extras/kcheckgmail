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

	void setLoginParams(const QString &username, const QString &password);
	void setInterval(unsigned int i);

	unsigned int getMailCount() { return mMailCount; }
	bool isLoggedIn();
	bool isChecking();

	// generate Cookie: string from mCookie
	QString cookieString();

protected:
	void login();
	void postLogin();
	void checkGMail();

	void parseCookies(const QString &str);

private:
	unsigned int mInterval;
	QMutex *mCheckLock;
	QMutex *mLoginLock;
	unsigned int mMailCount;

	bool mLoginParamsChanged;

	// true if timer spawned this check/login attempt.
	bool mLoginFromTimer;
	bool mCheckFromTimer;

	QString mUsername;
	QString mPassword;
	QMap<QString,QString> *mCookieMap;
	QString *mLoginToken;
	
	QTimer *mTimer;

public slots:
	void slotCheckGmail();

protected slots:
	void slotLoginResult(KIO::Job*);
	void slotLoginData(KIO::Job*, const QByteArray&);

	void slotPostLoginResult(KIO::Job*);
	void slotPostLoginData(KIO::Job*, const QByteArray&);

	void slotTimeout();

	void slotCheckResult(KIO::Job*);
	void slotCheckData(KIO::Job*, const QByteArray&);

signals:
	void startLogin();
	void loginDone(bool success, bool spawnedFromTimer, const QString &why = QString::null);
	void newMail(unsigned int count);
	void mailCountChanged(unsigned int count);

	void startCheck();
	void stopCheck();
};

#endif
