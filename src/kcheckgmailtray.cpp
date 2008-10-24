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
#include <kmimetype.h>
#include <kurl.h>

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
#include "advancedsettingswidget.h"
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
	setPixmap(mPixGmail);

	mLoginAnim = new QTimer(this, "KCheckGmail::login");
	connect(mLoginAnim, SIGNAL(timeout()), 
		this, SLOT(slotToggleLoginAnim()));

	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(this, SIGNAL(quitSelected()), kapp, SLOT(quit()));
	
	QToolTip::add(this, i18n("KCheckGMail"));
	
	iconDisplayed = true;

	// initialise and hook up the parser
	mParser = new GMailParser();

	connect(mParser, SIGNAL(mailArrived(unsigned int)), 
		this, SLOT(slotMailArrived(unsigned int)));

	connect(mParser, SIGNAL(mailCountChanged()), 
		this, SLOT(slotMailCountChanged()));

	connect(mParser, SIGNAL(versionMismatch()), 
		this, SLOT(slotVersionMismatch()));

	connect(mParser, SIGNAL(gNameUpdate(QString)), 
		this, SLOT(slotgNameUpdate(QString)));

	connect(mParser, SIGNAL(noUnreadMail()), 
		this, SLOT(slotNoUnreadMail()));
	

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
	menu->insertTitle(SmallIcon("kcheckgmail"), i18n("KCheckGMail"));
	menu->insertItem(SmallIcon("knotify"), 
		i18n("Configure &Notications..."), CONTEXT_NOTIFY);
	menu->insertItem(SmallIcon("configure"), 
		i18n("&Configure KCheckGMail..."), CONTEXT_CONFIGURE);
	menu->insertSeparator();
	mCheckNowId = menu->insertItem(SmallIcon("launch"), 
		i18n("Login and Chec&k Mail"), 
		mGmail, SLOT(slotCheckGmail()));
	menu->insertItem(SmallIcon("konqueror"), 
		i18n("&Launch Browser"), CONTEXT_LAUNCHBROWSER);
	menu->insertItem(SmallIcon("email"),
			 i18n("Co&mpose Mail"), CONTEXT_COMPOSE);

	mThreadsMenuId = menu->insertItem(SmallIcon("kcheckgmail"), i18n("Th&reads"),
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

	// register with dcop
	if(!kapp->dcopClient()->isRegistered()) {
		kapp->dcopClient()->registerAs(kapp->name());
	}
	kapp->dcopClient()->setDefaultObject(objId());
}


KCheckGmailTray::~KCheckGmailTray()
{
    QToolTip::remove(this);
}

void KCheckGmailTray::start()
{
	static bool started = false;
	
	if(started) {
		kdWarning() << k_funcinfo << "Unexpected call!" << endl;
	}
	
	//From RSIBreak
	if(KMessageBox::shouldBeShownContinue("welcome_to_kcheckgmail")) {
		takeScreenshotOfTrayIcon();
		KMessageBox::information(0,
					 i18n("<p><center>Welcome to KCheckGMail!</center></p>"
							 "<p>You can locate KCheckGMail here: </p>"
							 "<p><center><img source=\"systray_shot\"></center></p>"
							 "When you right-click on that icon you will see "
							 "a menu, from which you can see the Threads "
							 "menu containing the newest email of your account."),
					 i18n("Welcome"));

		KMessageBox::saveDontShowAgainContinue("welcome_to_kcheckgmail");
	}
	
	if(KMessageBox::shouldBeShownContinue("kcheckgmail_continue_legal")) {
		
		int legalCont;
		
		legalCont = KMessageBox::warningContinueCancel(0,
				i18n("Google, Gmail and Google Mail are registered trademarks of Google Inc.\n"
						"KCheckGMail nor its authors are in any way affiliated nor endorsed by Google Inc.\n"
						"By using this application you may or may not be violating "
						"the Terms of Use and/or the Program Policies "
						"of Gmail or Google Mail.\n"
						"Are you sure you want to use KCheckGMail?"),
				i18n("Legal Information"));
		
		if(legalCont == KMessageBox::Continue)
			KMessageBox::saveDontShowAgainContinue("kcheckgmail_continue_legal");
		else {
			//NOTE: kapp->quit(); doesn't quit immediately
			//There should be no harm on doing this because _nothing_ special has been loaded yet
			exit(0);
		}
	}
	
	if(Prefs::gmailUsername().length() == 0) {
		mLoginSettings->gmailPassword->erase();
		showPrefsDialog();
	}
	
	//Set interval and force the timer to start
	mGmail->setInterval(Prefs::interval(), true);
	mGmail->checkLoginParams();
	
	started = true;
}

///////////////////////////////////////////////////////////////////////////
// Menu functions
///////////////////////////////////////////////////////////////////////////

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

void KCheckGmailTray::showPrefsDialog()
{
	if(!KConfigDialog::showDialog("KCheckGmailSettingsDialog"))
		mConfigDialog->show();
}

void KCheckGmailTray::launchBrowser(const QString &url)
{
	QString loadURL;

	if(url == QString::null) {
		loadURL = getUrlBase();
		
		if (Prefs::gMailSimpleInterface())
			loadURL.append("h/");
	} else
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

void KCheckGmailTray::showKNotifyDialog()
{
	KNotifyDialog::configure(this);
}

void KCheckGmailTray::composeMail()
{
	QString url = getUrlBase();
	
	if (Prefs::gMailSimpleInterface()) {
		url.append("h/?v=b&pv=tl&cs=b");
	} else {
		url.append("?view=cm&fs=1&tearoff=1");
	}
	launchBrowser(url);
}

///////////////////////////////////////////////////////////////////////////
// Menu - related slots
///////////////////////////////////////////////////////////////////////////

void KCheckGmailTray::slotThreadsItemHighlighted(int n)
{
	// NOTE: the mime-type/icon code isn't enabled because KNotify won't display the icon
	GMailParser::Thread t = mParser->getThread(n);
	
	if (t.subject.isEmpty()) {
		return;
	}

	QStringList::Iterator it = t.attachments.begin();
	QStringList attachments;
	QString message = t.snippet,/* iconURL, format, */fileName;
	unsigned int attachmentsCount = 0;
	
	/*format = i 1 8 n("format used to display the attachments (%1 is the icon, %2 is the file name)",
		      "<img src=\"%1\"> %2");*/
		
	for (; it != t.attachments.end(); ++it ) {
		attachmentsCount++;
		fileName = *it;
		/*iconURL = KMimeType::iconForURL(KURL(fileName));
		kdDebug() << "Attachment name: " << fileName << ", iconURL: " << iconURL << endl;
		attachments.append(format.arg(iconURL, fileName));*/
		attachments.append(fileName);
	}
	
	if (attachmentsCount > 0) {
		// NOTE: %1 is the mail snippet and %2 is the attachments list
		message = i18n("%1\nAttachment: %2", "%1\nAttachments: %2", attachmentsCount)
				.arg(message, attachments.join(", "));
	}
	
	KNotifyClient::event(mThreadsMenu->winId(), "gmail-mail-snippet", message);
}

void KCheckGmailTray::slotThreadsMenuActivated(int n)
{
	const GMailParser::Thread &t = mParser->getThread(n);

	// make sure the thread does exist
	if(!t.isNull) {
		QString url = getUrlBase();
		
		if (Prefs::gMailSimpleInterface()) {
			url.append("h/?v=c&th=");
		} else {
			url.append("?view=cv&search=inbox&tearoff=1");
			url.append("&lvp=-1&cvp=1&fs=1&tf=1&fs=1&th=");
		}
		
		url.append(t.msgId);
		launchBrowser(url);
	}
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

///////////////////////////////////////////////////////////////////////////
// Settings - related slots/functions
///////////////////////////////////////////////////////////////////////////

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

	AdvancedSettingsWidget *cwid = new AdvancedSettingsWidget(0, "AdvancedSettings");
	mConfigDialog->addPage(cwid, i18n("Advanced"), "package_settings", i18n("Advanced Settings"));

	mLoginSettings->gmailPassword->erase();
	mLoginSettings->gmailPassword->insert("\007\007\007");
}

void KCheckGmailTray::slotSettingsChanged()
{
	bool loginOk = true;
	const char *passwd = mLoginSettings->gmailPassword->password();
	const QString user = mLoginSettings->kcfg_GmailUsername->originalText();
	int res;

	kdDebug() << k_funcinfo << passwd << endl;
	
	if(strlen(passwd) == 0 ) {
		kdDebug() << k_funcinfo << "user: " << user << endl;
		if(user.length() == 0) {
			res = KMessageBox::warningYesNo(0, i18n("No account information has been entered. Do you want to quit?"));

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
		
		if (Prefs::searchFor().length() == 0) {
			Prefs::setSearchFor("in:inbox is:unread");
			Prefs::writeConfig();
		}
		
		if (Prefs::searchFor().contains("in:") == 0 && Prefs::searchFor().contains("label:") == 0) {
			res = KMessageBox::questionYesNo(0, 
					i18n("<p>The search string you provided doesn't specify where to search for unread emails.</p>"
							"<p>A search without an <i>in:</i> and <i>label:</i> will return all unread emails.</p>"
							"<p>If what you want is to show the new emails in your inbox use <i>in:inbox</i> or leave empty.</p>"
							"<p>Are you sure you want to proceed without specifying location?</p>"),
       					QString::null,
       					KStdGuiItem::yes(),
					KStdGuiItem::no(),
					"no_location_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(showPrefsDialog()));
			}
		}
		if (Prefs::searchFor().contains("is:unread") == 0) {
			res = KMessageBox::questionYesNo(0, 
					i18n("<p>The search string you provided doesn't contain <i>is:unread</i>"
							".</p>"
							"<p>It should be set to ensure more unread messages are retrieved.</p>"
							"<p>Are you sure you want to proceed?</p>"),
					QString::null,
					KStdGuiItem::yes(),
					KStdGuiItem::no(),
					"is_unread_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(showPrefsDialog()));
			}
		}
	}
}

QString KCheckGmailTray::getUrlBase()
{
	QString base = "%1://mail.google.com/%2/";
	
	return base.arg((Prefs::useHTTPS())? "https" : "http", mGmail->getURLPart());
}

///////////////////////////////////////////////////////////////////////////
// Check/Login - related slots/functions
///////////////////////////////////////////////////////////////////////////

void KCheckGmailTray::slotLoginStart()
{
	kdDebug() << k_funcinfo << endl;
	setPixmapAuth();
	contextMenu()->setItemEnabled(mCheckNowId, false);
	mLoginAnim->start(200);
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
		QToolTip::remove( this );
		QToolTip::add(this, i18n("KCheckGMail"));
		
	} else {
		setPixmapEmpty();
		KNotifyClient::event(winId(), "gmail-login-yes", i18n("Now logged in to Gmail!"));
		contextMenu()->changeItem(mCheckNowId, i18n("Chec&k Mail Now"));
	}
	contextMenu()->setItemEnabled(mCheckNowId, true);

	slotMailCountChanged();
}

void KCheckGmailTray::slotLogingOut()
{
	// simulate a failed login attempt
	slotLoginDone(false, true, QString::null);
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

void KCheckGmailTray::slotMailArrived(unsigned int n)
{
	if (n == 1 && Prefs::displaySubjectOnSingleMail()) {
		GMailParser::Thread t;
		t = mParser->getLastThread();
		if (!t.isNull) {
			slotMailArrived(t.subject);
			return;
		}
	}
	QString str;

	str = i18n("There is <b>1</b> new message",
		   "There are <b>%n</b> new messages", n);

	KNotifyClient::event(winId(), "new-gmail-arrived", str);
	slotMailCountChanged();
}

void KCheckGmailTray::slotMailArrived(QString subject)
{
	QString str;
	
	str = i18n("New mail arrived: <i>%1</i>").arg(subject);
	
	KNotifyClient::event(winId(), "new-gmail-arrived", str);
	slotMailCountChanged();
}

void KCheckGmailTray::slotNoUnreadMail()
{
	KNotifyClient::event(winId(), "no-unread-gmail", i18n("There are no unread messages"));
}

void KCheckGmailTray::slotMailCountChanged()
{
	mMailCount = mParser->getNewCount(true);
	updateCountImage();
	updateThreadMenu();
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
					numItems ++;
				}
			}
			iter ++;
		}
		delete threads;
	}

	contextMenu()->setItemEnabled(mThreadsMenuId, (numItems > 0));
}

void KCheckGmailTray::slotSessionChanged()
{
	KNotifyClient::event(winId(), "gmail-session-changed", i18n("An other account has been opened, logging out from it!"));
}

///////////////////////////////////////////////////////////////////////////
// Other slots
///////////////////////////////////////////////////////////////////////////

//Used by the DCOP interface
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

QStringList KCheckGmailTray::getThreads()
{
	QStringList out;
	QMap<QString,bool> *threads = mParser->getThreadList();

	if(threads) {

		QValueList<QString> klist = threads->keys();
		QValueList<QString>::iterator iter = klist.begin();
		
		while(iter != klist.end()) {
			const GMailParser::Thread &t = mParser->getThread(*iter);
			if(!t.isNull) {
				out.append(*iter);
			}
			iter ++;
		}
		delete threads;
	}
	return out;
}

QString KCheckGmailTray::getThreadSubject(QString msgId)
{
	GMailParser::Thread t = mParser->getThread(msgId);
	
	if (t.subject.isEmpty()) {
		return QString::null;
	}
	
	return t.subject;
}

QString KCheckGmailTray::getThreadSender(QString msgId)
{
	GMailParser::Thread t = mParser->getThread(msgId);
	
	if (t.senders.isEmpty()) {
		return QString::null;
	}
	
	return t.senders;
}

QString KCheckGmailTray::getThreadSnippet(QString msgId)
{
	GMailParser::Thread t = mParser->getThread(msgId);
	
	if (t.snippet.isEmpty()) {
		return QString::null;
	}
	
	return t.snippet;
}

QStringList KCheckGmailTray::getThreadAttachments(QString msgId)
{
	GMailParser::Thread t = mParser->getThread(msgId);
	
	return t.attachments;
}

bool KCheckGmailTray::isNewThread(QString msgId)
{
	GMailParser::Thread t = mParser->getThread(msgId);
	
	return t.isNew;
}

QMap<QString, unsigned int> KCheckGmailTray::getLabels()
{
	QMap<QString, unsigned int> labels = mParser->getLabels();
	
	return labels;
}

QString KCheckGmailTray::getGaiaName()
{
	return mParser->getGaiaName();
}

void KCheckGmailTray::checkMailNow()
{
	mGmail->slotCheckGmail();
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
	QPixmap shot = QPixmap::grabWindow( qt_xrootwin(),  x,  y,  w,  h );
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
	QMimeSourceFactory::defaultFactory()->setPixmap( "systray_shot", finalShot );

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
