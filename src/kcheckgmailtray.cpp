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

#include <kaction.h>
#include <kapplication.h>
#include <kconfigdialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmacroexpander.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <knotifydialog.h>
#include <kpopupmenu.h>
#include <krun.h>

#include <qfile.h>
#include <qpainter.h>
#include <qtimer.h>

#include "appletsettingswidget.h"
#include "kcheckgmailtray.h"
#include "loginsettingswidget.h"
#include "netsettingswidget.h"
#include "prefs.h"

#include "config.h"
#include "gmail.h"

// if catchAccidentalClick is true, wait 3 seconds before opening another
// browser window
#define ACCIDENTAL_CLICK_TIMEOUT (3 * 1000) 

#define CONTEXT_CONFIGURE 100
#define CONTEXT_LAUNCHBROWSER 101
#define CONTEXT_NOTIFY 102
#define CONTEXT_CHECKNOW 103

KCheckGmailTray::KCheckGmailTray(QWidget *parent, const char *name)
	: KSystemTray(parent, name),
	mHelpMenu(new KHelpMenu(this, KGlobal::instance()->aboutData(), 
		false, actionCollection()))
{
	mPixGmail = KSystemTray::loadIcon("kcheckgmail");
	mPixLight = KSystemTray::loadIcon("kcheckgmaillight");
	mPixAuth = KSystemTray::loadIcon("kcheckgmailauth");

	setPixmap(mPixAuth);
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(this, SIGNAL(quitSelected()), kapp, SLOT(quit()));
	//QToolTip::add(this, i18n("KCheckGmail"));


	// initialise and hook up the GMail object
	mGmail = new GMail();

	connect(mGmail, SIGNAL(startLogin()), 
		this, SLOT(slotLoginStart()));

	connect(mGmail, SIGNAL(loginDone(bool, bool, const QString&)), 
		this, SLOT(slotLoginDone(bool, bool, const QString&)));

	connect(mGmail, SIGNAL(newMail(unsigned int)), 
		this, SLOT(slotNewMail(unsigned int)));

	connect(mGmail, SIGNAL(mailCountChanged(unsigned int)), 
		this, SLOT(slotMailCountChanged(unsigned int)));

	connect(mGmail, SIGNAL(startCheck()), 
		this, SLOT(slotStartCheck()));

	connect(mGmail, SIGNAL(stopCheck()), 
		this, SLOT(slotStopCheck()));

	// initialise the menu
	KPopupMenu *menu = contextMenu();
	menu->clear();
	menu->insertTitle(SmallIcon("kcheckgmail"), i18n("KCheckGmail"));
	menu->insertItem(SmallIcon("knotify"), 
		i18n("Configure &Notications..."), CONTEXT_NOTIFY);
	menu->insertItem(SmallIcon("configure"), 
		i18n("&Configure KCheckGmail..."), CONTEXT_CONFIGURE);
	menu->insertSeparator();
	mCheckNowId = menu->insertItem(SmallIcon("launch"), 
		i18n("Login and Chec&k Mail"), 
		mGmail, SLOT(slotCheckGmail()));
	menu->insertItem(SmallIcon("mozilla"), 
		i18n("&Launch Browser"), CONTEXT_LAUNCHBROWSER);

	menu->insertSeparator();

	menu->insertItem(SmallIcon("help"),KStdGuiItem::help().text(),
		mHelpMenu->menu());
//	KAction *quitAction = actionCollection()->action(KStdAction::name(KStdAction::Quit));
//	quitAction->plug(menu);

	connect(menu, SIGNAL(activated(int)), SLOT(slotContextMenuActivated(int)));

	mGmail->setLoginParams(Prefs::gmailUsername(), Prefs::gmailPassword());
	mGmail->setInterval(Prefs::interval());

}

void KCheckGmailTray::slotContextMenuActivated(int n)
{
	kdDebug() << k_funcinfo << "context=" << n << endl;

	switch(n) {
		case CONTEXT_CHECKNOW:
			break;
		case CONTEXT_CONFIGURE:
			showPrefsDialog();
			break;
		case CONTEXT_LAUNCHBROWSER:
			launchBrowser();
			break;
		case CONTEXT_NOTIFY:
			showKNotifyDialog();
			break;
	}
}

void KCheckGmailTray::launchBrowser()
{
	if(Prefs::useDefaultBrowser())
		kapp->invokeBrowser("http://gmail.google.com");
	else {
		QString s = Prefs::customBrowser();
		QMap<QChar,QString> map;
		map.insert('u', "http://gmail.google.com");
		s = KMacroExpander::expandMacrosShellQuote(s, map);
		KRun::runCommand(QFile::encodeName(s));
	}
}

void KCheckGmailTray::showKNotifyDialog()
{
	KNotifyDialog::configure(this);
}

void KCheckGmailTray::slotSettingsChanged()
{
	mGmail->setLoginParams(Prefs::gmailUsername(), Prefs::gmailPassword());
	mGmail->setInterval(Prefs::interval());
}

void KCheckGmailTray::updateCountImage()
{
	unsigned int count = mGmail->getMailCount();

	kdDebug() << k_funcinfo << "Count=" << count << endl;

	if(count == 0)
		setPixmap(mPixGmail);
	else {
		// based on code from kmail
		int w = mPixLight.width();
		int h = mPixLight.height();

		QString countString = QString::number(count);
		QFont countFont = KGlobalSettings::generalFont();
		countFont.setBold(true);

		// decrease the size of the font for the number of unread messages if the
		// number doesn't fit into the available space
		float countFontSize = countFont.pointSizeFloat();
		QFontMetrics qfm(countFont);
		int width = qfm.width(countString);

		kdDebug() << "------- countFontSize=" << countFontSize 
			<< " width=" << width << " w=" << w << endl;

		kdDebug() << "pixelSize="<<countFont.pixelSize()<<endl;
		if(width > w) {
			countFontSize *= float(w) / float(width);
			countFont.setPointSizeFloat( countFontSize );
		}

		mPixCount.resize(w, h);
		copyBlt(&mPixCount, 0, 0,
			&mPixLight, 0, 0, w, h);

		QPainter p(&mPixCount);
		p.setFont(countFont);
		p.setPen(Qt::black);
		p.drawText(mPixCount.rect(), Qt::AlignCenter, countString);

		setPixmap(mPixCount);
	}
}

void KCheckGmailTray::slotNewMail(unsigned int n)
{
	QString str;

	if(n == 1)
		str = i18n("There is <b>1</b> new message");
	else
		str = i18n("There are <b>%1</b> new messages").arg(n);

	KNotifyClient::event(winId(), "new-gmail-arrived", str);
	updateCountImage();
}

void KCheckGmailTray::slotMailCountChanged(unsigned int)
{
	updateCountImage();
}

void KCheckGmailTray::slotLoginDone(bool ok, bool evtFromTimer, const QString &why)
{
	kdDebug() << k_funcinfo << endl << "ok=" << ok << " evtFromTimer=" <<
		evtFromTimer << " why=" << why << endl << endl;
	if(!ok) {
		static QString lastExcuse = "";
		if(why != lastExcuse || !evtFromTimer) {
			KNotifyClient::event(winId(), "gmail-login-no",
				i18n("An error occurred logging in to Gmail<br><b>%1</b>")
				.arg(why));
			lastExcuse = why;
		}
		
		setPixmap(mPixAuth);
		contextMenu()->changeItem(mCheckNowId, i18n("Login and Chec&k Mail"));
		
	} else {
		setPixmap(mPixGmail);
		KNotifyClient::event(winId(), "gmail-login-yes", i18n("Now logged in to Gmail!"));
		contextMenu()->changeItem(mCheckNowId, i18n("Chec&k Mail Now"));
	}
	contextMenu()->setItemEnabled(mCheckNowId, true);
}

void KCheckGmailTray::slotLoginStart()
{
	kdDebug() << k_funcinfo << endl;
	setPixmap(mPixAuth);
	contextMenu()->setItemEnabled(mCheckNowId, false);
}

void KCheckGmailTray::mousePressEvent(QMouseEvent *ev)
{
	if(ev->button() == QMouseEvent::LeftButton) {
		if(Prefs::allowLeftClickOpen()) {
			if(Prefs::catchAccidentalClick()) {
				static QTimer *t = new QTimer(this);
				if(!t->isActive()) {
					t->start(ACCIDENTAL_CLICK_TIMEOUT, true);
					launchBrowser();
				}
			} else
				launchBrowser();
		}
	} else
		KSystemTray::mousePressEvent(ev);

}

void KCheckGmailTray::showPrefsDialog()
{
	if(KConfigDialog::showDialog("KCheckGmailSettingsDialog"))
		return;
	
	KConfigDialog *dialog = new KConfigDialog(this,
					"KCheckGmailSettingsDialog",
					Prefs::self(),
					KDialogBase::IconList,
					KDialogBase::Ok | KDialogBase::Cancel);
	connect(dialog, SIGNAL(settingsChanged()),
		this, SLOT(slotSettingsChanged()));

	LoginSettingsWidget *lwid = new LoginSettingsWidget(0, "LoginSettings");
	dialog->addPage(lwid, i18n("Login"), "kcheckgmail", i18n("Login Settings"));

	NetworkSettingsWidget *nwid = new NetworkSettingsWidget(0, "NetworkSettings");
	dialog->addPage(nwid, i18n("Network"), "www", i18n("Network Settings"));

	AppletSettingsWidget *awid = new AppletSettingsWidget(0, "AppletSettings");
	dialog->addPage(awid, i18n("Applet"), "configure", i18n("Applet Settings"));

	dialog->show();
}

void KCheckGmailTray::slotStartCheck()
{
	kdDebug() << k_funcinfo << endl;
	contextMenu()->setItemEnabled(mCheckNowId, false);
}

void KCheckGmailTray::slotStopCheck()
{
	kdDebug() << k_funcinfo << endl;
	contextMenu()->setItemEnabled(mCheckNowId, true);
}
