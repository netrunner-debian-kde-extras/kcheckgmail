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

#include "kcheckgmailcore.h"
#include "kcheckgmailcore_p.h"
#include "kcheckgmailtray.h"
#include "jsprotocol.h"
#include "configdialog.h"
#include "gmailwalletmanager.h"
#include "prefs.h"
#include "kcheckgmailadaptor.h"

#include <cstdlib>
#include <kapplication.h>
#include <klocale.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kmenu.h>
#include <kconfigdialog.h>
#include <kconfig.h>
#include <kiconeffect.h>
#include <kmimetype.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kmacroexpander.h>
#include <krun.h>
#include <k3aboutapplication.h>
#include <knotifyconfigwidget.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <knotification.h>
#include <QDBusConnection>

#include <QFile>
#include <QToolTip>
#include <QHash>
#include <QSignalMapper>
//Added by qt3to4:
#include <QList>
#include <QPixmap>
#include <ktoolinvocation.h>


KCheckGmailCore::KCheckGmailCore(QObject* parent)
	: QObject(parent),
	  d(new Private)
{
	d->mJSP = new JSProtocol(this);
	d->actions = new KActionCollection(this);

	initTray();
	initActions();
	buildTrayPopupMenu();
	initConfigDialog();
	makeConnections(d->mJSP, d->mTray);

	// D-Bus
	(void) new KcheckgmailAdaptor(this);
	QDBusConnection dbus = QDBusConnection::sessionBus();
	dbus.registerObject("/kcheckgmail", this, QDBusConnection::ExportScriptableSlots);

	d->mTray->showIcon();
	start();
}

KCheckGmailCore::~KCheckGmailCore()
{
	if (d->mConfigDialog)
		d->mConfigDialog->deleteLater();

	if (d->mTray)
		d->mTray->deleteLater();

	delete d;
	d = 0;
}


KCheckGmailCore& KCheckGmailCore::instance()
{
	static KCheckGmailCore object(0);
	return object;
}


void KCheckGmailCore::initTray()
{
	d->mTray = new KCheckGmailTray(0);
	connect(d->mTray, SIGNAL(quitSelected()), kapp, SLOT(quit()));

	connect(d->mTray, SIGNAL(leftButtonClicked()),
		this, SLOT(slotLeftButtonClicked()));

	connect(this, SIGNAL(countColorChanged(QColor)),
		d->mTray, SLOT(changeCountColor(QColor)));
}


void KCheckGmailCore::initActions()
{
	d->actionShowKNotifyDialog = new KAction(KIcon("preferences-desktop-notification"), i18n("Configure &Notifications"), this);
	d->actions->addAction("configure-notifications", d->actionShowKNotifyDialog);
	connect(d->actionShowKNotifyDialog, SIGNAL(triggered(bool)),
		this, SLOT(slotShowKNotifyDialog()));

	d->actionShowPrefsDialog = new KAction(KIcon("configure"), i18n("&Configure KCheckGMail..."), this);
	d->actions->addAction("configure", d->actionShowPrefsDialog);
	connect(d->actionShowPrefsDialog, SIGNAL(triggered(bool)),
		this, SLOT(slotShowPrefsDialog()));

	d->mLoginCheckMailAction = new KAction(KIcon("mail-receive"), i18n("Login and Chec&k Mail"), this);
	d->actions->addAction("check-mail", d->mLoginCheckMailAction);
	connect(d->mLoginCheckMailAction, SIGNAL(triggered(bool)),
		d->mJSP->retriever(), SLOT(slotCheckGmail()));

	d->actionLaunchBrowser = new KAction(KIcon("konqueror"), i18n("&Launch Browser"), this);
	d->actions->addAction("launch-browser", d->actionLaunchBrowser);
	connect(d->actionLaunchBrowser, SIGNAL(triggered(bool)),
		this, SLOT(slotLaunchBrowser()));

	d->actionComposeMail = new KAction(KIcon("email"), i18n("Co&mpose Mail"), this);
	d->actions->addAction("compose-mail", d->actionComposeMail);
	connect(d->actionComposeMail, SIGNAL(triggered(bool)),
		this, SLOT(slotComposeMail()));

	d->threadsMapper = new QSignalMapper(this);
	connect(d->threadsMapper, SIGNAL(mapped(int)),
		this, SLOT(slotThreadActivated(int)));
}


void KCheckGmailCore::buildTrayPopupMenu()
{
	d->menu = qobject_cast<KMenu *>(d->mTray->contextMenu());
	d->menu->clear();

	d->mThreadsMenu = new KMenu(d->menu);

#if 0
	connect(d->mThreadsMenu, SIGNAL(highlighted(int)),
		SLOT(slotThreadsItemHighlighted(int)));
#endif
	d->menu->addTitle(SmallIcon("kcheckgmail"), i18n("KCheckGMail"));

	d->menu->addAction(d->actionShowKNotifyDialog);
	d->menu->addAction(d->actionShowPrefsDialog);

	d->menu->addSeparator();

	d->menu->addAction(d->mLoginCheckMailAction);
	d->mLoginCheckMailAction->setEnabled(false);

	d->menu->addAction(d->actionLaunchBrowser);
	d->menu->addAction(d->actionComposeMail);

	d->mThreadsMenuAction = d->menu->addMenu(d->mThreadsMenu);
	d->mThreadsMenuAction->setIcon(SmallIcon("kcheckgmail"));
	d->mThreadsMenuAction->setText(i18n("Th&reads"));
	d->mThreadsMenuAction->setEnabled(false);

	d->menu->addSeparator();

	d->mHelpMenu = new KHelpMenu(d->menu, KGlobal::mainComponent().aboutData(), false, d->actions);
	d->mHelpMenu->menu()->setIcon(SmallIcon("help-contents"));
	d->mHelpMenu->menu()->setTitle(KStandardGuiItem::help().text());
	d->menu->addMenu(d->mHelpMenu->menu());
}


void KCheckGmailCore::initConfigDialog()
{
	d->mConfigDialog = new ConfigDialog(0,
					  "KCheckGmailSettingsDialog",
					  Prefs::self());

	connect(d->mConfigDialog, SIGNAL(finished()),
		this, SLOT(slotSettingsChanged()));
}


void KCheckGmailCore::makeConnections(JSProtocol* mJSP, KCheckGmailTray* mTray)
{
	// hook up JSProtocol
	connect(mJSP, SIGNAL(mailArrived(unsigned int)),
		this, SLOT(slotMailArrived(unsigned int)));

	connect(mJSP, SIGNAL(mailCountChanged(int)),
		mTray, SLOT(slotMailCountChanged(int)));

	connect(mJSP, SIGNAL(threadsChanged()),
		this, SLOT(updateThreadMenu()));

	connect(mJSP, SIGNAL(noUnreadMail()),
		mTray, SLOT(slotNoUnreadMail()));

	connect(mJSP, SIGNAL(checkDone()),
		this, SLOT(slotCheckDone()));

	connect(mJSP, SIGNAL(loginDone(bool, bool, const QString&)),
		this, SLOT(slotLoginDone(bool, bool, const QString&)));


	// hook up the GMailParser object
	connect(mJSP->parser(), SIGNAL(versionMismatch()),
		mTray, SLOT(slotVersionMismatch()));

	connect(mJSP->parser(), SIGNAL(gNameUpdate(QString)),
		mTray, SLOT(slotgNameUpdate(QString)));


	// hook up the GMail object
	connect(mJSP->retriever(), SIGNAL(loginStart()),
		this, SLOT(slotLoginStart()));

	connect(mJSP->retriever(), SIGNAL(checkStart()),
		this, SLOT(slotCheckStart()));

	connect(mJSP->retriever(), SIGNAL(sessionChanged()),
		this, SLOT(slotSessionChanged()));
	
	connect(mJSP->retriever(), SIGNAL(loggedOut()),
		mJSP, SLOT(slotLoggedOut()));

	connect(kapp, SIGNAL(aboutToQuit()),
		mJSP->retriever(), SLOT(slotLogOut()));


	// hook up GMailWalletManager
	connect(GMailWalletManager::instance(), SIGNAL(getWalletPassword(const QString&)),
		mJSP->retriever(), SLOT(slotGetWalletPassword(const QString&)));
	connect(GMailWalletManager::instance(), SIGNAL(setWalletPassword(bool)),
		mJSP->retriever(), SLOT(slotSetWalletPassword(bool)));
}


void KCheckGmailCore::start()
{
	static bool started = false;
	
	if(started) {
		kWarning() << k_funcinfo << "Unexpected call!";
	}
	
	//From RSIBreak
	if(KMessageBox::shouldBeShownContinue("welcome_to_kcheckgmail")) {
#if 0
		d->mTray->takeScreenshotOfTrayIcon();
		KMessageBox::information(0,
					 i18n("<p><center>Welcome to KCheckGMail!</center></p>"
							 "<p>You can locate KCheckGMail here: </p>"
							 "<p><center><img source=\"systray_shot\"></center></p>"
							 "When you right-click on that icon you will see "
							 "a menu, from which you can see the Threads "
							 "menu containing the newest email of your account."),
					 i18n("Welcome"));

		KMessageBox::saveDontShowAgainContinue("welcome_to_kcheckgmail");

		Prefs::self()->setTrayIconUnreadMessagesColor(QColor("blue"));
#endif
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
			exit(EXIT_SUCCESS);
		}
	}
	
	if(Prefs::gmailUsername().length() == 0) {
		d->mConfigDialog->erasePassword();
		slotShowPrefsDialog();
	}
	
	//Set interval and force the timer to start
	d->mJSP->retriever()->setInterval(Prefs::interval(), true);
	d->mJSP->retriever()->checkLoginParams(true);
	
	started = true;
}


QString KCheckGmailCore::getUrlBase()
{
	QString base = "%1://mail.google.com/%2/";
	
	return base.arg((Prefs::useHTTPS())? "https" : "http", d->mJSP->retriever()->getURLPart());
}


void KCheckGmailCore::updateThreadMenu()
{
	QMap<QString, int> entries;
	QMap<QString,bool> *threads = d->mJSP->parser()->getThreadList();
	int numItems = 0;

	/*
	 * There is no need to unmap the mThreadsMenu actions. They are
	 * automatically unmapped when they are destroyed.
	 * QMenu::clear() destroys them.
	 */
	d->mThreadsMenu->clear();
	if(threads) {
		QList<QString> klist = threads->keys();
		QList<QString>::iterator iter = klist.begin();

		kDebug() << k_funcinfo << "number of threads=" << klist.size();
		while(iter != klist.end()) {
			const GMailParser::Thread &t = d->mJSP->parser()->getThread(*iter);
			if(!t.isNull) {
				QString str = t.senders;
				str += " - ";
				str += t.subject;
				str.replace("&","&&");

				QAction* action = new QAction(str, d->mThreadsMenu);

				// TODO: Add an hasAttach method
				if (!t.attachments.isEmpty())
					action->setIcon(SmallIcon("mail-attachment"));

				connect(action, SIGNAL(triggered(bool)),
					d->threadsMapper, SLOT(map()));

				d->threadsMapper->setMapping(action, t.id);
				d->mThreadsMenu->addAction(action);
				numItems++;
			}
			iter++;
		}
		delete threads;
		threads = 0;
	}
	d->mThreadsMenuAction->setEnabled(numItems > 0);
}


/**
 * Creates the notifying message displayed when email arrives.
 *
 * @param n Number of arrived emails
 * @param showSender If true shows the email sender
 * @param showSubject If true shows the email subject
 * @param showSnippet If true shows the email message snippet
 * @return The (i18n) string that will be displayed
 */
QString KCheckGmailCore::newEmailNotifyMessage(unsigned int n, bool showSender, bool showSubject, bool showSnippet)
{
	bool newLine = false;

	if  ( (n == 1) && (showSender || showSubject || showSnippet) ) {
		GMailParser::Thread t;
		t = d->mJSP->parser()->getLastArrivedThread();
		if (!t.isNull) {
			QString str;
			str = i18n("<center><b>New mail arrived</b></center>");
			if (showSender) {
				str.append(i18n("<b>Sender:</b> <i>%1</i>", t.senders));
				newLine = true;
			}
			if (showSubject) {
				if (newLine) {
					str.append("<br>");
				}
				str.append( i18n("<b>Subject:</b> <i>%1</i>", t.subject));
				newLine = true;
			}
			if (showSnippet) {
				if (newLine) {
					str.append("<br>");
				}
				str.append(t.snippet);
			}
			return str;
		}
	}
	return i18np("There is <b>1</b> new mail message",
		   "There are <b>%1</b> new mail messages", n);
}


void KCheckGmailCore::slotShowKNotifyDialog()
{
	KNotifyConfigWidget::configure(0);
}


void KCheckGmailCore::slotShowPrefsDialog()
{
	if(!KConfigDialog::showDialog("KCheckGmailSettingsDialog"))
		d->mConfigDialog->show();
}


void KCheckGmailCore::slotLaunchBrowser(const QString &url)
{
	QString loadURL;

	if(url == QString()) {
		loadURL = getUrlBase();
		
		if (Prefs::gMailSimpleInterface())
			loadURL.append("h/");
	} else
		loadURL = url;

	if(Prefs::useDefaultBrowser())
		KToolInvocation::invokeBrowser(loadURL);
	else {
		QString s = Prefs::customBrowser();
		QHash<QChar,QString> hash;
		hash.insert('u', loadURL);
		s = KMacroExpander::expandMacrosShellQuote(s, hash);
		KRun::runCommand(QFile::encodeName(s), 0);
	}
}


void KCheckGmailCore::slotComposeMail()
{
	QString url = getUrlBase();
	
	if (Prefs::gMailSimpleInterface()) {
		url.append("h/?v=b&pv=tl&cs=b");
	} else {
		url.append("?view=cm&fs=1&tearoff=1");
	}
	slotLaunchBrowser(url);
}


void KCheckGmailCore::slotLeftButtonClicked()
{
	if(Prefs::allowLeftClickOpen()) {
		if(Prefs::catchAccidentalClick()) {
			static QTimer *t = new QTimer(this);
			if(!t->isActive()) {
				t->setSingleShot(true);
				t->start(ACCIDENTAL_CLICK_TIMEOUT);
				slotLaunchBrowser();
			}
		} else
			slotLaunchBrowser();
	}
}


void KCheckGmailCore::slotThreadActivated(int n)
{
	const GMailParser::Thread &t = d->mJSP->parser()->getThread(n);

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
		slotLaunchBrowser(url);
	}
}


void KCheckGmailCore::slotThreadsItemHighlighted(int n)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(n);
	
	if (t.subject.isEmpty()) {
		return;
	}

	QStringList::ConstIterator it = t.attachments.begin();
	QStringList attachments;
	QString message = t.snippet, iconFileName, format, fileName;
	unsigned int attachmentsCount = 0;
	KMimeType::Ptr mimetype;

	format = i18nc("format used to display the attachments (%1 is the icon, %2 is the file name)", "<img src=\"%1\"> %2");

	for (; it != t.attachments.end(); ++it ) {
		attachmentsCount++;
		fileName = *it; // attachment filename

		//TODO check mimetype return value
		mimetype = KMimeType::findByPath(fileName, 0, true); // no disk access
		iconFileName = KIconLoader::global()->iconPath(mimetype->iconName(), KIconLoader::Small);
		kDebug() << "Attachment name: " << fileName << ", iconFileName: " << iconFileName;
		attachments.append(format.arg(iconFileName, fileName));
	}

	if (attachmentsCount > 0) {
		message.append("<br>" + i18np("Attachment: %2", "Attachments: %2", attachmentsCount,
				 attachments.join(", ")));
	}

	KNotification::event(QLatin1String("gmail-mail-snippet"), message, QPixmap(), d->mThreadsMenu);
	kDebug() << k_funcinfo << "Notification:" << "gmail-mail-snippet";
}


void KCheckGmailCore::slotMailArrived(unsigned int n)
{
	QString str = newEmailNotifyMessage(n, Prefs::displaySenderOnSingleMail(), Prefs::displaySubjectOnSingleMail(), Prefs::displaySnippetOnSingleMail());

	KNotification *notify = new KNotification("new-gmail-arrived");
	notify->setText(str);

	if (n == 1) {
		notify->setActions(QStringList(i18n("Open")));
		connect(notify, SIGNAL(activated(unsigned int)),
			this, SLOT(slotOpenButtonClicked()));
	}
	notify->sendEvent();
	kDebug() << k_funcinfo << "Notification: new-gmail-arrived" \
				<< " number of emails:" << n << endl;
}


void KCheckGmailCore::slotSettingsChanged()
{
	bool loginOk = true;
	QString passwd = d->mConfigDialog->password();
	const QString user = d->mConfigDialog->username();
	int res;

	kDebug() << k_funcinfo << passwd;
	
	if(passwd.length() == 0 ) {
		kDebug() << k_funcinfo << "user: " << user;
		if(user.length() == 0) {
			res = KMessageBox::warningYesNo(0, i18n("No account information has been entered. Do you want to quit?"));

			if( res == KMessageBox::Yes ) {
				emit quitSelected();
				kapp->quit();
			} else {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
				return;
			}
		}
	} else {
		
		kDebug() << k_funcinfo << " strncmp: " << (passwd == QString("\007\007\007"));

		if(passwd != QString("\007\007\007")) {
			kDebug() << k_funcinfo << "setting wallet";
			loginOk = GMailWalletManager::instance()->set(d->mConfigDialog->password());
			d->mConfigDialog->erasePassword();
			d->mConfigDialog->insertPassword("\007\007\007");
		} else {
			kDebug() << k_funcinfo << "passwd unchanged: " << passwd;
			// force a params check only if password was not changed as
			// GMailWalletManager will take care of triggering the check if it was changed
			d->mJSP->retriever()->checkLoginParams(false);
		}
		
		if (Prefs::searchFor().length() == 0) {
			Prefs::setSearchFor("in:inbox is:unread");
			Prefs::self()->writeConfig();
		}
		
		if (Prefs::searchFor().contains("in:") == 0 && Prefs::searchFor().contains("label:") == 0) {
			res = KMessageBox::questionYesNo(0, 
					i18n("<p>The search string you provided doesn't specify where to search for unread emails.</p>"
							"<p>A search without an <i>in:</i> and <i>label:</i> will return all unread emails.</p>"
							"<p>If what you want is to show the new emails in your inbox use <i>in:inbox</i> or leave empty.</p>"
							"<p>Are you sure you want to proceed without specifying location?</p>"),
       					QString(),
       					KStandardGuiItem::yes(),
					KStandardGuiItem::no(),
					"no_location_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
				return;
			}
		}
		if (Prefs::searchFor().contains("is:unread") == 0) {
			res = KMessageBox::questionYesNo(0, 
					i18n("<p>The search string you provided doesn't contain <i>is:unread</i>"
							".</p>"
							"<p>It should be set to ensure more unread messages are retrieved.</p>"
							"<p>Are you sure you want to proceed?</p>"),
					QString(),
					KStandardGuiItem::yes(),
					KStandardGuiItem::no(),
					"is_unread_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
				return;
			}
		}
		d->mJSP->retriever()->setInterval(Prefs::interval());
	}

	emit countColorChanged(Prefs::trayIconUnreadMessagesColor());
}


void KCheckGmailCore::slotLoginDone(bool ok, bool isExcuseNeeded, const QString& message)
{
	d->mTray->stopAnim();

	if (!ok) {
		if (isExcuseNeeded) {
			KNotification::event(QLatin1String("gmail-login-no"),
					i18n("An error occurred logging in to Gmail<br><b>%1</b>",
						 message));

			kDebug() << k_funcinfo << "Notification: gmail-login-no" \
						<< " error message:" << message << endl;
		}

		d->mTray->setPixmapAuth();
		d->mLoginCheckMailAction->setText(i18n("Login and Chec&k Mail"));
		d->mTray->setToolTip(i18n("KCheckGMail"));
		
	} else {
		d->mTray->setPixmapEmpty();
		KNotification::event(QLatin1String("gmail-login-yes"), i18n("Now logged in to Gmail!"));

		kDebug() << k_funcinfo << "Notification: gmail-login-yes";

		d->mLoginCheckMailAction->setText(i18n("Chec&k Mail Now"));
	}
	d->mLoginCheckMailAction->setEnabled(true);
}


void KCheckGmailCore::slotLoginStart()
{
	kDebug() << k_funcinfo;
	d->mTray->setPixmapAuth();
	d->mLoginCheckMailAction->setEnabled(false);
	d->mTray->startAnim(200);
}


void KCheckGmailCore::slotCheckStart()
{
	d->mLoginCheckMailAction->setEnabled(false);
}


void KCheckGmailCore::slotSessionChanged()
{
	KNotification::event(QLatin1String("gmail-session-changed"), i18n("Another account has been opened, logging out from it!"));
	kDebug() << k_funcinfo << "Notification: gmail-session-changed";
}


void KCheckGmailCore::slotCheckDone()
{	
	d->mLoginCheckMailAction->setEnabled(true);
}


void KCheckGmailCore::slotLogingOut()
{
	d->mTray->setPixmapEmpty();
	KNotification::event(QLatin1String("gmail-login-yes"), i18n("Now logged in to Gmail!"));
	kDebug() << k_funcinfo << "Notification: gmail-login-yes";

	d->mLoginCheckMailAction->setText(i18n("Chec&k Mail Now"));

	d->mLoginCheckMailAction->setEnabled(true);
}


void KCheckGmailCore::slotOpenButtonClicked()
{
	GMailParser::Thread t = d->mJSP->parser()->getLastArrivedThread();
	if(!t.isNull) {
		slotThreadActivated(t.id);
	}
}


int KCheckGmailCore::mailCount() const
{
	return d->mJSP->unread();
}


void KCheckGmailCore::checkMailNow()
{
	d->mJSP->retriever()->slotCheckGmail();
}


void KCheckGmailCore::showIcon()
{
	d->mTray->showIcon();
}


void KCheckGmailCore::hideIcon()
{
	d->mTray->hideIcon();
}


void KCheckGmailCore::whereAmI()
{
	d->mTray->whereAmI();
}


QStringList KCheckGmailCore::getThreads()
{
	QStringList out;
	QMap<QString,bool> *threads = d->mJSP->parser()->getThreadList();

	if(threads) {

		QList<QString> klist = threads->keys();
		QList<QString>::iterator iter = klist.begin();
		
		while(iter != klist.end()) {
			const GMailParser::Thread &t =d->mJSP->parser()->getThread(*iter);
			if(!t.isNull) {
				out.append(*iter);
			}
			iter ++;
		}
	}

	delete threads;
	threads = 0;

	return out;
}


QString KCheckGmailCore::getThreadSubject(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	if (t.subject.isEmpty()) {
		return QString();
	}
	
	return t.subject;
}


QString KCheckGmailCore::getThreadSender(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	if (t.senders.isEmpty()) {
		return QString();
	}
	
	return t.senders;
}


QString KCheckGmailCore::getThreadSnippet(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	if (t.snippet.isEmpty()) {
		return QString();
	}
	
	return t.snippet;
}


QStringList KCheckGmailCore::getThreadAttachments(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	return t.attachments;
}


bool KCheckGmailCore::isNewThread(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	return t.isNew;
}


QMap<QString, unsigned int> KCheckGmailCore::getLabels()
{
	QMap<QString, unsigned int> labels = d->mJSP->parser()->getLabels();
	
	return labels;
}


QString KCheckGmailCore::getGaiaName()
{
	return d->mJSP->parser()->getGaiaName();
}

#include "kcheckgmailcore.moc"
