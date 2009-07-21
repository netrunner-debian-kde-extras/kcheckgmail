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

#ifndef KCHECKGMAIL_TRAY_H
#define KCHECKGMAIL_TRAY_H

#include <ksystemtrayicon.h>
#include <kiconeffect.h>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <qmap.h>
//Added by qt3to4:
#include <QMouseEvent>



namespace KCheckGmail { class ConfigDialog; }

class GMail;
class GMailParser;
class QMouseEvent;
class KHelpMenu;
class KConfigDialog;
class LoginSettingsWidget;
class QTimer;
class KAction;

class KCheckGmailTray : public KSystemTrayIcon
{
	Q_OBJECT
public:
	KCheckGmailTray(QWidget *parent = 0);
	virtual ~KCheckGmailTray();

	void takeScreenshotOfTrayIcon();

	void setPixmapAuth();
	void setPixmapEmpty();
    void stopAnim();
    void startAnim(unsigned int t);
	void toggleAnim(bool restoreToState);
	void showIcon();
	void hideIcon();
	void whereAmI();

signals:
	void leftButtonClicked();

protected:
	void mousePressEvent(QMouseEvent*);

public slots:

	// GMailParser
	void slotMailCountChanged(int n);
	void slotVersionMismatch();
	void slotgNameUpdate(QString name);
	void slotNoUnreadMail();

	// login "animation"
	void slotToggleLoginAnim();

	void changeCountColor(QColor color);

private slots:
	void slotActivated(QSystemTrayIcon::ActivationReason reason);

private:
	void updateCountImage(QColor color);

	QPixmap		mPixGmail;
	QImage		mLightIconImage;

	KIconEffect mIconEffect;
	QTimer *mLoginAnim;

	int	mMailCount;
	
	bool iconDisplayed;
};

#endif // KCHECKGMAIL_TRAY_H
