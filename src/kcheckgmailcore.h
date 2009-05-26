/***************************************************************************
 *   Copyright (C) 2008 by Luís Pereira <luis.artur.pereira@gmail.com>     *
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

#ifndef KCHECKGMAIL_CORE_H
#define KCHECKGMAIL_CORE_H

#include <qobject.h>

#include <kdebug.h>


class KCheckGmailCore;
class KCheckGmailTray;

namespace KCheckGmail {
	class JSProtocol;
};

class KMenu;
class KActionCollection;
using KCheckGmail::JSProtocol;


/*
 * This class contains code moved (and sometimes modified) from the
 * following classes:
 *
 * KCheckGmailTray
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>
 *
 * KCheckGmailIface
 *   Copyright (C) 2005 by James Stembridge <jstembridge@gmail.com>
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>
 */
class KCheckGmailCore : public QObject {
	Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kcheckgmail.kcheckgmail")

public:
	static KCheckGmailCore& instance();

signals:
	void quitSelected();
	void countColorChanged(QColor color);

private slots:
	void slotShowKNotifyDialog();
	void slotShowPrefsDialog();

	void slotLaunchBrowser(const QString &url = QString::null);
	void slotComposeMail();

	void slotLeftButtonClicked();

	void slotThreadActivated(int);
	void slotThreadsItemHighlighted(int);
	void updateThreadMenu();

	void slotMailArrived(unsigned int);

	void slotSettingsChanged();	
	void slotLoginDone(bool ok, bool isExcuseNeeded, const QString& message);
	void slotLoginStart();
	void slotCheckStart();
	void slotSessionChanged();
	void slotCheckDone();
	void slotLogingOut();
	void slotOpenButtonClicked();

private:
	KCheckGmailCore(QObject* parent = 0, const char* name = 0);
	virtual ~KCheckGmailCore();
	KCheckGmailCore(KCheckGmailCore&);
	KCheckGmailCore& operator=(const KCheckGmailCore&);

	void initTray();
	void initActions();
	void buidTrayPopupMenu();
	void initConfigDialog();
	void makeConnections(JSProtocol* mJSP, KCheckGmailTray* mTray);
	void start();

	QString getUrlBase();

	QString newEmailNotifyMessage(unsigned int n, bool showSender, bool showSubject, bool showSnippet);

	// dcop call implementations
	int mailCount() const;
	void checkMailNow();
	void showIcon();
	void hideIcon();
	void whereAmI();
	QStringList getThreads();
	QString getThreadSubject(QString msgId);
	QString getThreadSender(QString msgId);
	QString getThreadSnippet(QString msgId);
	QStringList getThreadAttachments(QString msgId);
	bool isNewThread(QString msgId);
	QMap<QString, unsigned int> getLabels();
	QString getGaiaName();

private:
	class Private;
	Private* d;
};

#endif
