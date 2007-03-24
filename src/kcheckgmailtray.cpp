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
#include <kiconeffect.h>
#include <dcopclient.h>

#include <qfile.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <kpassdlg.h>
#include <klineedit.h>

#include "appletsettingswidget.h"
#include "kcheckgmailtray.h"
#include "loginsettingswidget.h"
#include "netsettingswidget.h"
#include "prefs.h"

#include "config.h"
#include "gmail.h"
#include "gmailparser.h"
#include "gmailwalletmanager.h"

// if catchAccidentalClick is true, wait 3 seconds before opening another
// browser window
#define ACCIDENTAL_CLICK_TIMEOUT (3 * 1000) 

#define CONTEXT_CONFIGURE 100
#define CONTEXT_LAUNCHBROWSER 101
#define CONTEXT_NOTIFY 102
#define CONTEXT_CHECKNOW 103
#define CONTEXT_COMPOSE 104

KCheckGmailTray::KCheckGmailTray(QWidget *parent, const char *name)
	: DCOPObject("KCheckGmailIface"),
	KSystemTray(parent, name),
	mHelpMenu(new KHelpMenu(this, KGlobal::instance()->aboutData(), 
		false, actionCollection())),
	mMailCount(-1)
{
	mPixGmail = KSystemTray::loadIcon("kcheckgmail");
	mPixCount = KSystemTray::loadIcon("kcheckgmail");
	setPixmapAuth();

	mLoginAnim = new QTimer(this, "KCheckGmail::login");
	connect(mLoginAnim, SIGNAL(timeout()), 
		this, SLOT(slotToggleLoginAnim()));

	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(this, SIGNAL(quitSelected()), kapp, SLOT(quit()));
	
	QToolTip::add(this, i18n("KCheckGMail"));

	// initialise and hook up the parser
	mParser = new GMailParser();

	connect(mParser, SIGNAL(mailArrived(unsigned int)), 
		this, SLOT(slotMailArrived(unsigned int)));

	connect(mParser, SIGNAL(mailCountChanged()), 
		this, SLOT(slotMailCountChanged()));

	connect(mParser, SIGNAL(versionMismatch()), 
		this, SLOT(slotVersionMismatch()));

	connect(mParser, SIGNAL(gNameChanged(QString)), 
		this, SLOT(slotgNameChanged(QString)));
	

	// initialise and hook up the GMail object
	mGmail = new GMail();

	connect(mGmail, SIGNAL(loginStart()), 
		this, SLOT(slotLoginStart()));

	connect(mGmail, SIGNAL(loginDone(bool, bool, const QString&)), 
		this, SLOT(slotLoginDone(bool, bool, const QString&)));

	connect(mGmail, SIGNAL(checkStart()), 
		this, SLOT(slotCheckStart()));

	connect(mGmail, SIGNAL(checkDone(const QString&)), 
		this, SLOT(slotCheckDone(const QString&)));

	connect(mGmail, SIGNAL(sessionChanged()), 
		this, SLOT(slotSessionChanged()));
	
	connect(kapp, SIGNAL(shutDown()),
		 mGmail, SLOT(slotLogOut()));

	// initialise the threads menu
	mThreadsMenu = new KPopupMenu(this, "KCheckGmail Threads menu");
	connect(mThreadsMenu, SIGNAL(activated(int)),
		SLOT(slotThreadsMenuActivated(int)));
	connect(mThreadsMenu, SIGNAL(highlighted(int)),
		SLOT(slotThreadsItemHighlighted(int)));
	
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
	menu->insertItem(SmallIcon("konqueror"), 
		i18n("&Launch Browser"), CONTEXT_LAUNCHBROWSER);
	menu->insertItem(SmallIcon("email"),
		i18n("Co&mpose Mail"), CONTEXT_COMPOSE);

	mThreadsMenuId = menu->insertItem(SmallIcon("kcheckgmail"), i18n("Threads"),
		mThreadsMenu);
	contextMenu()->setItemEnabled(mThreadsMenuId, false);

	menu->insertSeparator();

	menu->insertItem(SmallIcon("help"),KStdGuiItem::help().text(),
		mHelpMenu->menu());

	connect(menu, SIGNAL(activated(int)), SLOT(slotContextMenuActivated(int)));

	connect(GMailWalletManager::instance(), SIGNAL(getWalletPassword(const QString&)),
		mGmail, SLOT(slotGetWalletPassword(const QString&)));
	connect(GMailWalletManager::instance(), SIGNAL(setWalletPassword(bool)),
		mGmail, SLOT(slotSetWalletPassword(bool)));

	initConfigDialog();

	if(Prefs::gmailUsername().length() == 0) {
		mLoginSettings->gmailPassword->erase();
		showPrefsDialog();
	} else
		mGmail->checkLoginParams();
	mGmail->setInterval(Prefs::interval());

	// register with dcop
	if(!kapp->dcopClient()->isRegistered()) {
		kapp->dcopClient()->registerAs(kapp->name());
	}
	kapp->dcopClient()->setDefaultObject(objId());
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
		case CONTEXT_COMPOSE:
			composeMail();
			break;
	}
}

void KCheckGmailTray::slotThreadsMenuActivated(int n)
{
	kdDebug() << k_funcinfo << "n=" << n << endl;
	const GMailParser::Thread &t = mParser->getThread(n);

	if(!t.isNull) {
		QString url=getUrlBase();
		url.append("?view=cv&search=inbox&tearoff=1");
		url.append("&lvp=-1&cvp=1&fs=1&tf=1&fs=1&th=");
		url.append(t.msgId);
		launchBrowser(url);
	}
}

void KCheckGmailTray::slotThreadsItemHighlighted(int n)
{
	KNotifyClient::userEvent(mThreadsMenu->winId(),mThreadsMenu->whatsThis(n),16);
}

void KCheckGmailTray::launchBrowser(const QString &url)
{
	QString loadURL;

	if(url == QString::null)
		loadURL = getUrlBase();
	else
		loadURL = url;

	if(Prefs::useDefaultBrowser())
		kapp->invokeBrowser(loadURL);
	else {
		QString s = Prefs::customBrowser();
		QMap<QChar,QString> map;
		map.insert('u', loadURL);
		s = KMacroExpander::expandMacrosShellQuote(s, map);
		KRun::runCommand(QFile::encodeName(s));
	}
}

void KCheckGmailTray::composeMail()
{
	QString url = getUrlBase();
	url.append("?view=cm&fs=1&tearoff=1");
	launchBrowser(url);
}

void KCheckGmailTray::showKNotifyDialog()
{
	KNotifyDialog::configure(this);
}

void KCheckGmailTray::slotSettingsChanged()
{
	bool loginOk = true;
	const char *passwd = mLoginSettings->gmailPassword->password();
	const QString user = mLoginSettings->kcfg_GmailUsername->originalText();

	kdDebug() << k_funcinfo << passwd << endl;
	
	if(strlen(passwd) == 0 ) {
		kdDebug() << k_funcinfo << "user: " << user << endl;
		if(user.length() == 0) {
			int res = KMessageBox::warningYesNo(0, i18n("No account information has been entered. Do you want to quit?"));

			if( res == KMessageBox::Yes ) {
				emit quitSelected();
				kapp->quit();
			} else {
				QTimer::singleShot(100, this, SLOT(showPrefsDialog()));
			}
		}
	} else {
		kdDebug() << k_funcinfo << " strncmp: " << strncmp(passwd, "\007\007\007", 3) << endl;

		if( strncmp(passwd, "\007\007\007", 3) != 0) {
			kdDebug() << k_funcinfo << "setting wallet" << endl;
			loginOk = GMailWalletManager::instance()->set(mLoginSettings->gmailPassword->password());
			mLoginSettings->gmailPassword->erase();
			mLoginSettings->gmailPassword->insert("\007\007\007");
		} else
			kdDebug() << k_funcinfo << "passwd unchanged: " << passwd << endl;
		
		mGmail->setInterval(Prefs::interval());
	}
}

void KCheckGmailTray::updateCountImage()
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

		kdDebug() << "------- countFontSize=" << countFontSize 
			<< " width=" << width << " w=" << w << endl;

		kdDebug() << "pixelSize="<<countFont.pixelSize()<<endl;
		if(width > w) {
			countFontSize *= float(w) / float(width);
			countFont.setPointSizeFloat( countFontSize );
		}

		mPixCount.resize(w, h);

		mPixCount = mIconEffect.apply(mPixGmail, 
			KIconEffect::ToGamma,
			0.80,
			Qt::red,
			false);

		QPainter p(&mPixCount);
		p.setFont(countFont);
		p.setPen(Qt::black);
		p.drawText(mPixCount.rect(), Qt::AlignCenter, countString);

		setPixmap(mPixCount);
	}
}

void KCheckGmailTray::updateThreadMenu()
{
	mThreadsMenu->clear();

	QMap<QString,bool> *threads = mParser->getThreadList();
	int numItems = 0, id;

	if(threads) {

		QValueList<QString> klist = threads->keys();
		QValueList<QString>::iterator iter = klist.begin();

		kdDebug() << k_funcinfo << "number of threads=" << klist.size() << endl;
		
		while(iter != klist.end()) {
			bool isNew = (*threads)[*iter];
			if(isNew) {
				const GMailParser::Thread &t = mParser->getThread(*iter);
				if(!t.isNull) {
					QString str = t.senders;
					str += " - ";
					str += t.subject;
					str.replace("&","&&");
					
					id = mThreadsMenu->insertItem(str, t.id);
					mThreadsMenu->setWhatsThis(id, t.snippet);
					numItems ++;
				}
			}
			iter ++;
		}
	}

	contextMenu()->setItemEnabled(mThreadsMenuId, (numItems > 0));
}

void KCheckGmailTray::slotMailArrived(unsigned int n)
{
	QString str;

	str = i18n("There is <b>1</b> new message",
		   "There are <b>%n</b> new messages", n);

	KNotifyClient::event(winId(), "new-gmail-arrived", str);
	slotMailCountChanged();
}

void KCheckGmailTray::slotMailCountChanged()
{
	mMailCount = mParser->getNewCount(true);
	updateCountImage();
	updateThreadMenu();
}

void KCheckGmailTray::slotLoginDone(bool ok, bool evtFromTimer, const QString &why)
{
	mLoginAnim->stop();

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
		
		setPixmapAuth();
		contextMenu()->changeItem(mCheckNowId, i18n("Login and Chec&k Mail"));
		
	} else {
		setPixmapEmpty();
		KNotifyClient::event(winId(), "gmail-login-yes", i18n("Now logged in to Gmail!"));
		contextMenu()->changeItem(mCheckNowId, i18n("Chec&k Mail Now"));
	}
	contextMenu()->setItemEnabled(mCheckNowId, true);

	slotMailCountChanged();
}

void KCheckGmailTray::slotLoginStart()
{
	kdDebug() << k_funcinfo << endl;
	setPixmapAuth();
	contextMenu()->setItemEnabled(mCheckNowId, false);
	mLoginAnim->start(200);
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

void KCheckGmailTray::initConfigDialog()
{
	mConfigDialog = new KConfigDialog(this,
					"KCheckGmailSettingsDialog",
					Prefs::self(),
					KDialogBase::IconList,
					KDialogBase::Ok | KDialogBase::Cancel);

	connect(mConfigDialog, SIGNAL(finished()),
		this, SLOT(slotSettingsChanged()));

	mLoginSettings = new LoginSettingsWidget(0, "LoginSettings");
	mConfigDialog->addPage(mLoginSettings, i18n("Login"), "kcheckgmail", i18n("Login Settings"));

	NetworkSettingsWidget *nwid = new NetworkSettingsWidget(0, "NetworkSettings");
	mConfigDialog->addPage(nwid, i18n("Network"), "www", i18n("Network Settings"));

	AppletSettingsWidget *awid = new AppletSettingsWidget(0, "AppletSettings");
	mConfigDialog->addPage(awid, i18n("Behavior"), "configure", i18n("Behavior"));

	mLoginSettings->gmailPassword->erase();
	mLoginSettings->gmailPassword->insert("\007\007\007");
}

void KCheckGmailTray::showPrefsDialog()
{
	if(!KConfigDialog::showDialog("KCheckGmailSettingsDialog")) 
		mConfigDialog->show();
}

void KCheckGmailTray::slotCheckStart()
{
	contextMenu()->setItemEnabled(mCheckNowId, false);
}

void KCheckGmailTray::slotCheckDone(const QString &data)
{
	mParser->parse(data);
	contextMenu()->setItemEnabled(mCheckNowId, true);
}

void KCheckGmailTray::slotVersionMismatch()
{
	static bool warned = false;
	
	if(Prefs::alertVersionChange() && !warned) {
		warned = true;
		KMessageBox::information(0,
			i18n("Gmail has been upgraded since this version of KCheckGmail was released. This may cause all sort of strange errors. Please check for an upgrade to KCheckGmail soon."),
			i18n("Version changed"),
			"IgnoreVersionChange");
	}
}

void KCheckGmailTray::setPixmapAuth()
{
	setPixmap(mIconEffect.apply(mPixGmail, 
		KIconEffect::Colorize,
		0.80,
		Qt::red,
		false));
}

void KCheckGmailTray::setPixmapEmpty()
{
	setPixmap(mPixGmail);
}

void KCheckGmailTray::slotToggleLoginAnim()
{
	static bool state = false;
	if(state) 
		setPixmapEmpty();
	else 
		setPixmapAuth();
	state = !state;
}

void KCheckGmailTray::checkMailNow()
{
	mGmail->slotCheckGmail();
}

QString KCheckGmailTray::getUrlBase()
{
	QString base = "%1://mail.google.com/%2/";
	
	return base.arg((Prefs::useHTTPS())? "https" : "http", mGmail->getURLPart());
}

void KCheckGmailTray::slotgNameChanged(QString name)
{
	kdDebug() << k_funcinfo << "Updating tooltip" << endl;
	QToolTip::remove( this );
	QToolTip::add(this, i18n("KCheckGMail - Notifying about new email for %1").arg(name));
}

void KCheckGmailTray::slotSessionChanged()
{
	KNotifyClient::event(winId(), "gmail-session-changed", i18n("An other session has been opened, logging out from it!"));
}
