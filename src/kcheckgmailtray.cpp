/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>              *
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>            *
 *                                                                         *
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

#include <cstdlib>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <kiconeffect.h>

#include <qpainter.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qbitmap.h>
//Added by qt3to4:
#include <QPixmap>
#include <QMouseEvent>
#include <QX11Info>

#include "kcheckgmailtray.h"
#include "prefs.h"




KCheckGmailTray::KCheckGmailTray(QWidget *parent, const char *name)
	: KSystemTray(parent, name),
	mMailCount(0)
{
	mPixGmail = KSystemTray::loadIcon("kcheckgmail");
	mLightIconImage = mIconEffect.apply(mPixGmail,
						KIconEffect::ToGamma,
						0.90,
						Qt::red,
						false);
	setPixmap(mPixGmail);

	mLoginAnim = new QTimer(this, "KCheckGmail::login");
	connect(mLoginAnim, SIGNAL(timeout()), 
		this, SLOT(slotToggleLoginAnim()));

	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	QToolTip::add(this, i18n("KCheckGMail"));
	
	iconDisplayed = false;
}


KCheckGmailTray::~KCheckGmailTray()
{
    QToolTip::remove(this);
}


void KCheckGmailTray::mousePressEvent(QMouseEvent *ev)
{
	if(ev->button() == Qt::LeftButton) {
		emit leftButtonClicked();
	} else
		KSystemTray::mousePressEvent(ev);

}


///////////////////////////////////////////////////////////////////////////
// Check/Login - related slots/functions
///////////////////////////////////////////////////////////////////////////


void KCheckGmailTray::slotNoUnreadMail()
{
	KNotifyClient::event(winId(), "no-unread-gmail", i18n("There are no unread messages"));
}

void KCheckGmailTray::slotMailCountChanged(int n)
{
	mMailCount = n;
	updateCountImage(Prefs::trayIconUnreadMessagesColor());
}

void KCheckGmailTray::showIcon()
{
	iconDisplayed = true;
	show();
}

void KCheckGmailTray::hideIcon()
{
	iconDisplayed = false;
	hide();
}


void KCheckGmailTray::slotVersionMismatch()
{
	static bool warned = false;
	
	if(Prefs::alertVersionChange() && !warned) {
		warned = true;
		KMessageBox::information(0,
					 i18n("Gmail has been upgraded since this version of KCheckGMail was released. This may cause all sort of strange errors. Please check for an upgrade to KCheckGMail soon."),
					 i18n("Version changed"),
					 "IgnoreVersionChange");
	}
}

///////////////////////////////////////////////////////////////////////////
// Tray icon - related functions
///////////////////////////////////////////////////////////////////////////

/**
 * Update the number of unread messages in the tray icon
 */
void KCheckGmailTray::updateCountImage(QColor color)
{
	kdDebug() << k_funcinfo << "Count=" << mMailCount << endl;

	if(mMailCount == 0)
		setPixmapEmpty();
	else {
		// based on code from kmail
		int w = mPixGmail.width();
		int h = mPixGmail.height();

		QString countString = QString::number(mMailCount);
		QFont countFont = KGlobalSettings::generalFont();
		countFont.setBold(true);

		// decrease the size of the font for the number of unread messages if the
		// number doesn't fit into the available space
		float countFontSize = countFont.pointSizeFloat();
		QFontMetrics qfm(countFont);
		int width = qfm.width(countString);

		if(width > w) {
			countFontSize *= float(w) / float(width);
			countFont.setPointSizeFloat( countFontSize );
		}

		QPixmap numberPixmap(w, h);
		numberPixmap.fill(Qt::lightGray);
		QPainter p(&numberPixmap);
		p.setFont(countFont);
		p.setPen(color);
		p.drawText(numberPixmap.rect(), Qt::AlignCenter, countString);
		numberPixmap.setMask(numberPixmap.createHeuristicMask());
		QImage numberImage = numberPixmap.convertToImage();

		// do the overlay
		QImage iconWithNumberImage = mLightIconImage.copy();
		KIconEffect::overlay(iconWithNumberImage, numberImage);

		// convert from QImage to QPixmap
		QPixmap iconWithNumber;
		iconWithNumber.convertFromImage(iconWithNumberImage);
		setPixmap(iconWithNumber);
	}
}

//from rsibreak: rsiwidget.cpp
void KCheckGmailTray::whereAmI()
{
	if (!iconDisplayed)
		showIcon();
	
	takeScreenshotOfTrayIcon();
	
	KMessageBox::information(0,
				 i18n("<p>KCheckGMail is already running</p><p>You can find it here:</p><p><center><img source=\"systray_shot\"></center></p>"),
				 i18n("Already Running"));
}

//from rsibreak: rsiwidget.cpp
void KCheckGmailTray::takeScreenshotOfTrayIcon()
{
	
        // Process the events else the icon will not be there and the screenie will fail!
	kapp->processEvents();

        // ********************************************************************************
        // This block is copied from Konversation - KonversationMainWindow::queryClose()
        // The part about the border is copied from  KSystemTray::displayCloseMessage()
	//
        // Compute size and position of the pixmap to be grabbed:
	QPoint g = this-> pos();

        //Catch invalid positions (2007 - Raphael Geissert)
        if (g.x() < 0) {
            g.setX(0);
        }
        if (g.y() < 0) {
            g.setY(0);
        }
        g = this->mapToGlobal( g );

	int desktopWidth  = kapp->desktop()->width();
	int desktopHeight = kapp->desktop()->height();
	int tw = this->width();
	int th = this->height();
	int w = desktopWidth / 4;
	int h = desktopHeight / 9;
	
	int x = g.x() + tw/2 - w/2;               // Center the rectange in the systray icon
	int y = g.y() + th/2 - h/2;
	if ( x < 0 )                 x = 0;       // Move the rectangle to stay in the desktop limits
	if ( y < 0 )                 y = 0;
	if ( x + w > desktopWidth )  x = desktopWidth - w;
	if ( y + h > desktopHeight ) y = desktopHeight - h;

        // Grab the desktop and draw a circle around the icon:
	QPixmap shot = QPixmap::grabWindow( QX11Info::appRootWindow(),  x,  y,  w,  h );
	QPainter painter( &shot );
	const int MARGINS = 6;
	const int WIDTH   = 3;
	int ax = g.x() - x - MARGINS -1;
	int ay = g.y() - y - MARGINS -1;
	painter.setPen(  QPen( Qt::red,  WIDTH ) );
	painter.drawArc( ax,  ay,  tw + 2*MARGINS,  th + 2*MARGINS,  0,  16*360 );
	painter.end();

        // Then, we add a border around the image to make it more visible:
	QPixmap finalShot(w + 2, h + 2);
	finalShot.fill(KApplication::palette().active().foreground());
	painter.begin(&finalShot);
	painter.drawPixmap(1, 1, shot);
	painter.end();

        // Associate source to image and show the dialog:
	Q3MimeSourceFactory::defaultFactory()->setPixmap( "systray_shot", finalShot );

        // End copied block
        // ********************************************************************************
}


void KCheckGmailTray::slotgNameUpdate(QString name)
{
	static QString sname;
	kdDebug() << k_funcinfo << "Updating tooltip" << endl;
	
	//Trick to restore the tooltip
	if(name == QString::null)
		name = sname;
	else
		sname = name;
	
	QToolTip::remove( this );
	QToolTip::add(this, i18n("KCheckGMail - Notifying about new email for %1").arg(name));
}

void KCheckGmailTray::setPixmapAuth()
{
	setPixmap(mIconEffect.apply(mPixGmail, 
		  KIconEffect::Colorize,
		  0.60,
		  Qt::lightGray,
		  false));
}

void KCheckGmailTray::setPixmapEmpty()
{
	setPixmap(mPixGmail);
}

void KCheckGmailTray::toggleAnim(bool restoreToState)
{
	static bool state = false;
	if(state)
		setPixmapEmpty();
	else 
		setPixmapAuth();
	
	if(!restoreToState)
		state = !state;
}

void KCheckGmailTray::slotToggleLoginAnim()
{
	toggleAnim(false);
}


void KCheckGmailTray::changeCountColor(QColor color)
{
	updateCountImage(color);
}


void KCheckGmailTray::stopAnim()
{
    mLoginAnim->stop();
}


void KCheckGmailTray::startAnim(unsigned int t)
{
    mLoginAnim->start(t);
}

#include "kcheckgmailtray.moc"
