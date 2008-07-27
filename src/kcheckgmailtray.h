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

#include <ksystemtray.h>
#include <kiconeffect.h>
#include <qpixmap.h>
#include <qstring.h>

#include "kcheckgmailiface.h"

class GMail;
class GMailParser;
class QMouseEvent;
class KHelpMenu;
class KConfigDialog;
class LoginSettingsWidget;
class QTimer;

class KCheckGmailTray : public KSystemTray, virtual public KCheckGmailIface
{
	Q_OBJECT
public:
	KCheckGmailTray(QWidget *parent = 0, const char *name = 0);
	void start();

protected:
	void mousePressEvent(QMouseEvent*);

	void setPixmapAuth();
	void setPixmapSnooze();
	void setPixmapEmpty();
	
	void toggleAnim(bool restoreToState);

protected slots:
	// KPopupMenu
	void slotContextMenuActivated(int);
	void slotThreadsMenuActivated(int);
	void slotThreadsItemHighlighted(int);

	// KConfigDialog
	void slotSettingsChanged();
	void showPrefsDialog();

	// GMail
	void slotLoginDone(bool success, bool spawnedFromTimer, const QString &errmsg);
	void slotLoginStart();
	void slotCheckStart();
	void slotSessionChanged();
	void slotCheckDone(const QString &data);

	// GMailParser
	void slotMailArrived(unsigned int n);
	void slotMailCountChanged();
	void slotVersionMismatch();
	void slotgNameChanged(QString name);

	// login "animation"
	void slotToggleLoginAnim();

private:
	void launchBrowser(const QString &url = QString::null);
	void showKNotifyDialog();
	void updateCountImage();
	void updateThreadMenu();
	void composeMail();
	void snooze();
	void initConfigDialog();
	QString getUrlBase();

	// dcop call implementations
	int mailCount() const { return mMailCount; };
	void checkMailNow();
	void whereAmI();
	
	void takeScreenshotOfTrayIcon();

	QPixmap		mPixGmail,
			mPixCount;
	GMail		*mGmail;
	GMailParser	*mParser;
	KHelpMenu	*mHelpMenu;
	KPopupMenu	*mThreadsMenu;
	LoginSettingsWidget* mLoginSettings;
	KConfigDialog* mConfigDialog;
	KIconEffect mIconEffect;
	QTimer *mLoginAnim;

	// menu ids
	int	mCheckNowId;
	int	mThreadsMenuId;

	// mail count for dcop interface
	int	mMailCount;
	
	bool isSnoozing;
};
