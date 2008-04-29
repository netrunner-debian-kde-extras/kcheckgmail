/***************************************************************************
 *   Copyright (C) 2008 by Luis Pereira <luis.artur.pereira@gmail.com>     *
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

#ifndef KCHECKGMAIL_JSPROTOCOL_H
#define KCHECKGMAIL_JSPROTOCOL_H

#include <qobject.h>

#include "gmailparser.h"
#include "gmail.h"
#include "mailcounter.h"


namespace KCheckGmail {

/*
 * This class contains code moved (and sometimes modified) from the
 * following classes:
 *
 * KCheckGmailTray
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>
 *
 * GMailParser
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>
 */
class JSProtocol : public QObject {
	Q_OBJECT

public:
	JSProtocol(QObject* parent, const char* name = 0);
	~JSProtocol();

	GMailParser* parser();
	GMail* retriever();
    int unread() const;
    QMap<QString, int> threadMenuEntries();

signals:
	void mailArrived(unsigned int count);
	void threadsChanged(QMap<QString, int> menuEntries);
	void mailCountChanged(int count);
	void noUnreadMail();
	void checkDone();
	void loginDone(bool success, bool isExcuseNeeded, const QString& errmsg);

private slots:
	void slotCountUpdate(unsigned int parsed);
	void slotCheckDone(const QString& data);
	void slotLoginDone(bool success, bool spawnedFromTimer, const QString &errmsg);

private:
	GMailParser* mParser;
	GMail* mGmail;
	MailCounter mCount;
	bool firstTime;
};

}

#endif
