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
#include <qpixmap.h>
#include <qstring.h>

class GMail;
class QMouseEvent;
class KHelpMenu;

class KCheckGmailTray : public KSystemTray
{
	Q_OBJECT
public:
	KCheckGmailTray(QWidget *parent = 0, const char *name = 0);

protected:
	void mousePressEvent(QMouseEvent*);

protected slots:
	void slotContextMenuActivated(int);
	void slotLoginDone(bool success, bool spawnedFromTimer, const QString &errmsg);
	void slotLoginStart();
	void slotStartCheck();
	void slotStopCheck();
	void slotMailCountChanged(unsigned int count);
	void slotNewMail(unsigned int count);
	void slotSettingsChanged();

private:
	void launchBrowser();
	void showKNotifyDialog();
	void showPrefsDialog();
	void updateCountImage();

	QPixmap		mPixGmail,
			mPixLight,
			mPixAuth,
			mPixCount;
	GMail		*mGmail;
	KHelpMenu	*mHelpMenu;

	// menu id for the check now button
	int 		mCheckNowId;
};
