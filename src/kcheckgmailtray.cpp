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
#include <knotification.h>
#include <kcolorscheme.h>
#include <kiconeffect.h>
#include <kglobalsettings.h>
#include <KTemporaryFile>


#include <QPainter>
#include <QTimer>
#include <QToolTip>
#include <QBitmap>
//#include <Q3MimeSourceFactory>
#include <QDesktopWidget>
//Added by qt3to4:
#include <QPixmap>
#include <QMouseEvent>
#include <QX11Info>

#include "kcheckgmailtray.h"
#include "prefs.h"




KCheckGmailTray::KCheckGmailTray(QWidget *parent)
	: KSystemTrayIcon(parent),
	mMailCount(0)
{
	mPixGmail = KSystemTrayIcon::loadIcon("kcheckgmail").pixmap(KIconLoader::SizeSmallMedium);
	mLightIconImage = mIconEffect.apply(mPixGmail,
						KIconEffect::ToGamma,
						0.90,
						Qt::red,
						false).toImage();
	setIcon(mPixGmail);

	mLoginAnim = new QTimer(this);
	connect(mLoginAnim, SIGNAL(timeout()), 
		this, SLOT(slotToggleLoginAnim()));

	setToolTip(i18n("KCheckGMail"));
	
	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(slotActivated(QSystemTrayIcon::ActivationReason)));
}


KCheckGmailTray::~KCheckGmailTray()
{
}

void KCheckGmailTray::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
		emit leftButtonClicked();
}


///////////////////////////////////////////////////////////////////////////
// Check/Login - related slots/functions
///////////////////////////////////////////////////////////////////////////


void KCheckGmailTray::slotNoUnreadMail()
{
	KNotification::event(QLatin1String("no-unread-gmail"), i18n("There are no unread messages"));
}

void KCheckGmailTray::slotMailCountChanged(int n)
{
	mMailCount = n;
	updateCountImage(Prefs::trayIconUnreadMessagesColor());
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
	kDebug() << k_funcinfo << "Count=" << mMailCount;

	if(mMailCount == 0)
		setPixmapEmpty();
	else {
		// adapted from KMSystemTray::updateCount()

		int oldPixmapWidth = mPixGmail.size().width();

		QString countString = QString::number( mMailCount );
		QFont countFont = KGlobalSettings::generalFont();
		countFont.setBold(true);

		// decrease the size of the font for the number of unread messages if the
		// number doesn't fit into the available space
		float countFontSize = countFont.pointSizeF();
		QFontMetrics qfm( countFont );
		int width = qfm.width( countString );
		if( width > (oldPixmapWidth - 2) )
		{
		  countFontSize *= float( oldPixmapWidth - 2 ) / float( width );
		  countFont.setPointSizeF( countFontSize );
		}

		// Overlay the light KCheckGmail image with the number image
		QImage iconWithNumberImage = mLightIconImage.copy();
		QPainter p( &iconWithNumberImage );
		p.setFont( countFont );
		KColorScheme scheme( QPalette::Active, KColorScheme::View );

		qfm = QFontMetrics( countFont );
		QRect boundingRect = qfm.tightBoundingRect( countString );
		boundingRect.adjust( 0, 0, 0, 2 );
		boundingRect.setHeight( qMin( boundingRect.height(), oldPixmapWidth ) );
		boundingRect.moveTo( (oldPixmapWidth - boundingRect.width()) / 2,
				    ((oldPixmapWidth - boundingRect.height()) / 2) - 1 );
		p.setOpacity( 0.7 );
		p.setBrush( scheme.background( KColorScheme::LinkBackground ) );
		p.setPen( scheme.background( KColorScheme::LinkBackground ).color() );
		p.drawRoundedRect( boundingRect, 2.0, 2.0 );

		p.setBrush( Qt::NoBrush );
//		p.setPen( scheme.foreground( KColorScheme::LinkText ).color() );
		p.setPen(color);
		p.setOpacity( 1.0 );
		p.drawText( iconWithNumberImage.rect(), Qt::AlignCenter, countString );

		setIcon( QPixmap::fromImage( iconWithNumberImage ) );
	}
}

//from rsibreak: rsiwidget.cpp
void KCheckGmailTray::whereAmI()
{
	show();

	QString systray_shot = takeScreenshotOfTrayIcon();
	const QString imgTag = QString::fromLatin1("<img src=\"%1\"/>").arg(systray_shot);
	KMessageBox::information(0,
				 i18n("<p>KCheckGMail is already running</p><p>You can find it here:</p><p><p><center>%1</center></p></p>", imgTag),
				 i18n("Already Running"));
}

QString KCheckGmailTray::takeScreenshotOfTrayIcon()
{
        // Process the events else the icon will not be there and the screenie will fail!
	kapp->processEvents();

	// Taken from Akregator TrayIcon::takeScreenshot()
	const QRect rect = geometry();
	const QPoint g = rect.topLeft();
	int desktopWidth  = kapp->desktop()->width();
	int desktopHeight = kapp->desktop()->height();
	int tw = rect.width();
	int th = rect.height();
	int w = desktopWidth / 4;
	int h = desktopHeight / 9;
	int x = g.x() + tw/2 - w/2; // Center the rectange in the systray icon
	int y = g.y() + th/2 - h/2;
	if (x < 0)
		x = 0; // Move the rectangle to stay in the desktop limits
	if (y < 0)
		y = 0;
	if (x + w > desktopWidth)
		x = desktopWidth - w;
	if (y + h > desktopHeight)
		y = desktopHeight - h;

	// Grab the desktop and draw a circle around the icon:
	QPixmap shot = QPixmap::grabWindow(QApplication::desktop()->winId(), x, y, w, h);
	QPainter painter(&shot);
	painter.setRenderHint( QPainter::Antialiasing );
	const int MARGINS = 6;
	const int WIDTH   = 3;
	int ax = g.x() - x - MARGINS -1;
	int ay = g.y() - y - MARGINS -1;
	painter.setPen( QPen(Qt::red/*KApplication::palette().active().highlight()*/, WIDTH) );
	painter.drawArc(ax, ay, tw + 2*MARGINS, th + 2*MARGINS, 0, 16*360);
	painter.end();

	// Paint the border
	const int BORDER = 1;
	QPixmap finalShot(w + 2*BORDER, h + 2*BORDER);
	finalShot.fill( KApplication::palette().color( QPalette::Foreground ));
	painter.begin(&finalShot);
	painter.drawPixmap(BORDER, BORDER, shot);
	painter.end();
//	return shot; // not finalShot?? -fo

	// End of Taken from Akregator

	QString filename;
	KTemporaryFile* tmpfile = new KTemporaryFile;
	tmpfile->setAutoRemove(false);
	if (tmpfile->open()) {
		filename = tmpfile->fileName();
		shot.save(tmpfile, "png");
		tmpfile->close();
	}
	return filename;
}


void KCheckGmailTray::slotgNameUpdate(QString name)
{
	static QString sname;
	kDebug() << k_funcinfo << "Updating tooltip";
	
	//Trick to restore the tooltip
	if(name == QString())
		name = sname;
	else
		sname = name;
	
	setToolTip(i18n("KCheckGMail - Notifying about new email for %1", name));
}

void KCheckGmailTray::setPixmapAuth()
{
	setIcon(mIconEffect.apply(mPixGmail,
		  KIconEffect::Colorize,
		  0.60,
		  Qt::lightGray,
		  false));
}

void KCheckGmailTray::setPixmapEmpty()
{
	setIcon(mPixGmail);
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
