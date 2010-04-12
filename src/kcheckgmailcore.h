/***************************************************************************
 *   Copyright (C) 2008 by Luís Pereira <luis.artur.pereira@gmail.com>     *
 *                                                                         *
 *   This class contains code moved (and sometimes modified) from the      *
 *   following classes:                                                    *
 *                                                                         *
 * KCheckGmailTray                                                         *
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>              *
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>            *
 *                                                                         *
 * KCheckGmailIface                                                        *
 *   Copyright (C) 2005 by James Stembridge <jstembridge@gmail.com>        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KCHECKGMAIL_CORE_H
#define KCHECKGMAIL_CORE_H

#include <QObject>

#include <kdebug.h>


class KCheckGmailCore;
class KCheckGmailTray;

namespace KCheckGmail {
	class JSProtocol;
};

class KMenu;
class KActionCollection;
class QColor;
using KCheckGmail::JSProtocol;


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

	void slotLaunchBrowser(const QString &url = QString());
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

	void slotStart();

private:
	KCheckGmailCore(QObject* parent = 0);
	virtual ~KCheckGmailCore();
	KCheckGmailCore(KCheckGmailCore&);
	KCheckGmailCore& operator=(const KCheckGmailCore&);

	void initTray();
	void initActions();
	void buildTrayPopupMenu();
	void initConfigDialog();
	void makeConnections(JSProtocol* mJSP, KCheckGmailTray* mTray);

	QString getUrlBase();

	QString newEmailNotifyMessage(unsigned int n, bool showSender, bool showSubject, bool showSnippet);

public Q_SLOTS:
	// D-Bus callable implementations
	Q_SCRIPTABLE int mailCount() const;
	Q_SCRIPTABLE void checkMailNow();
	Q_SCRIPTABLE void showIcon();
	Q_SCRIPTABLE void hideIcon();
	Q_SCRIPTABLE void whereAmI();
	Q_SCRIPTABLE QStringList getThreads();
	Q_SCRIPTABLE QString getThreadSubject(QString msgId);
	Q_SCRIPTABLE QString getThreadSender(QString msgId);
	Q_SCRIPTABLE QString getThreadSnippet(QString msgId);
	Q_SCRIPTABLE QStringList getThreadAttachments(QString msgId);
	Q_SCRIPTABLE bool isNewThread(QString msgId);
	Q_SCRIPTABLE QMap<QString, unsigned int> getLabels();
	Q_SCRIPTABLE QString getGaiaName();

private:
	class Private;
	Private* d;
};

#endif
