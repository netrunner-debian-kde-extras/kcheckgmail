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
#include "knotification.h"
#include "jsprotocol.h"
#include "configdialog.h"
#include "gmailwalletmanager.h"
#include "prefs.h"
#include "kcheckgmailadaptor.h"

#include <cstdlib>
#include <kapplication.h>
#include <klocale.h>
#include <kactioncollection.h>
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

#include <qfile.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QPixmap>
#include <ktoolinvocation.h>


KCheckGmailCore::KCheckGmailCore(QObject* parent, const char* name)
	: QObject(parent),
	  d(new Private)
{
	d->mJSP = new JSProtocol(this, "JSProtocol");
	d->actions = new KActionCollection(this);

	initTray();
	initActions();
	buidTrayPopupMenu();
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
	static KCheckGmailCore object(0, "KCheckGmailCore");
	return object;
}


void KCheckGmailCore::initTray()
{
	d->mTray = new KCheckGmailTray(0, "KCheckGmailTray");
	connect(d->mTray, SIGNAL(quitSelected()), kapp, SLOT(quit()));

	connect(d->mTray, SIGNAL(leftButtonClicked()),
		this, SLOT(slotLeftButtonClicked()));

	connect(this, SIGNAL(countColorChanged(QColor)),
		d->mTray, SLOT(changeCountColor(QColor)));
}


void KCheckGmailCore::initActions()
{
	d->actionShowKNotifyDialog = new KAction(i18n("Configure &Notifications"), "knotify", "", this, SLOT(slotShowKNotifyDialog()), d->actions);

	d->actionShowPrefsDialog = new KAction(i18n("&Configure KCheckGMail..."), "configure", "",
		this, SLOT( slotShowPrefsDialog()), d->actions);

	d->mLoginCheckMailAction = new KAction(i18n("Login and Chec&k Mail"), "launch", "",
		d->mJSP->retriever(), SLOT(slotCheckGmail()), d->actions);

	d->actionLaunchBrowser = new KAction(i18n("&Launch Browser"), "konqueror", "",
		this, SLOT(slotLaunchBrowser()), d->actions);

	d->actionComposeMail = new KAction(i18n("Co&mpose Mail"), "email", "",
		this, SLOT(slotComposeMail()), d->actions );
}


void KCheckGmailCore::buidTrayPopupMenu()
{
	d->mThreadsMenu = new KMenu(d->mTray, "KCheckGmail Threads menu");

	connect(d->mThreadsMenu, SIGNAL(activated(int)),
		SLOT(slotThreadActivated(int)));

	connect(d->mThreadsMenu, SIGNAL(highlighted(int)),
		SLOT(slotThreadsItemHighlighted(int)));

	d->menu = d->mTray->contextMenu();
	d->menu->clear();

	d->menu->insertTitle(SmallIcon("kcheckgmail"), i18n("KCheckGMail"));

	d->actionShowKNotifyDialog->plug( d->menu );
	d->actionShowPrefsDialog->plug(d->menu);

	d->menu->insertSeparator();

	d->mLoginCheckMailAction->plug(d->menu);
	d->mLoginCheckMailAction->setEnabled(false);

	d->actionLaunchBrowser->plug(d->menu);
	d->actionComposeMail->plug(d->menu);

	d->mThreadsMenuId = d->menu->insertItem(SmallIcon("kcheckgmail"),
		i18n("Th&reads"), d->mThreadsMenu);
	d->mTray->contextMenu()->setItemEnabled(d->mThreadsMenuId, false);

	d->menu->insertSeparator();

	d->mHelpMenu = new KHelpMenu(d->mTray, KGlobal::instance()->aboutData(), false, d->actions);
	d->menu->insertItem(SmallIcon("help"), KStandardGuiItem::help().text(), d->mHelpMenu->menu());
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
	
	connect(kapp, SIGNAL(shutDown()),
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
		kWarning() << k_funcinfo << "Unexpected call!" << endl;
	}
	
	//From RSIBreak
	if(KMessageBox::shouldBeShownContinue("welcome_to_kcheckgmail")) {
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
	d->mJSP->retriever()->checkLoginParams();
	
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
	int id = 0;

	d->mThreadsMenu->clear();
        if(threads) {
                Q3ValueList<QString> klist = threads->keys();
                Q3ValueList<QString>::iterator iter = klist.begin();

                kDebug() << k_funcinfo << "number of threads=" << klist.size() << endl;

                while(iter != klist.end()) {
                        const GMailParser::Thread &t = d->mJSP->parser()->getThread(*iter);
                        if(!t.isNull) {
                                QString str = t.senders;
                                str += " - ";
                                str += t.subject;
                                str.replace("&","&&");

				// TODO: Add an hasAttach method
				if (t.attachments.isEmpty())
					id = d->mThreadsMenu->insertItem(str, t.id);
				else
					id = d->mThreadsMenu->insertItem(SmallIcon("attach"), str, t.id);
				numItems ++;
                        }
			iter ++;
                }
                delete threads;
                threads = 0;
        }

	d->mTray->contextMenu()->setItemEnabled(d->mThreadsMenuId, (numItems > 0));
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
				str.append(i18n("<b>Sender:</b> <i>%1</i>").arg(t.senders));
				newLine = true;
			}
			if (showSubject) {
				if (newLine) {
					str.append("<br>");
				}
				str.append( i18n("<b>Subject:</b> <i>%1</i>").arg(t.subject));
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
	return i18n("There is <b>1</b> new mail message",
		   "There are <b>%n</b> new mail messages", n);
}


void KCheckGmailCore::slotShowKNotifyDialog()
{
	KNotifyConfigWidget::configure(d->mTray);
}


void KCheckGmailCore::slotShowPrefsDialog()
{
	if(!KConfigDialog::showDialog("KCheckGmailSettingsDialog"))
		d->mConfigDialog->show();
}


void KCheckGmailCore::slotLaunchBrowser(const QString &url)
{
	QString loadURL;

	if(url == QString::null) {
		loadURL = getUrlBase();
		
		if (Prefs::gMailSimpleInterface())
			loadURL.append("h/");
	} else
		loadURL = url;

	if(Prefs::useDefaultBrowser())
		KToolInvocation::invokeBrowser(loadURL);
	else {
		QString s = Prefs::customBrowser();
		QMap<QChar,QString> map;
		map.insert('u', loadURL);
		s = KMacroExpander::expandMacrosShellQuote(s, map);
		KRun::runCommand(QFile::encodeName(s));
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
				t->start(ACCIDENTAL_CLICK_TIMEOUT, true);
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
	QString message = t.snippet, iconType, iconFileName, format, fileName;
	unsigned int attachmentsCount = 0;
	KMimeType::Ptr mimetype;

	format = i18n("format used to display the attachments (%1 is the icon, %2 is the file name)", "<img src=\"%1\"> %2");

	for (; it != t.attachments.end(); ++it ) {
		attachmentsCount++;
		fileName = *it; // attachment filename

		mimetype = KMimeType::findByPath(fileName, 0, true); // no disk access
		iconType = mimetype->icon(QString::null, false);
		iconFileName = KIconLoader::global()->iconPath(iconType, KIcon::Small);
		kDebug() << "Attachment name: " << fileName << ", iconType: " << iconType << endl;
		attachments.append(format.arg(iconFileName, fileName));
	}

	if (attachmentsCount > 0) {
		message.append("<br>" + i18n("Attachment: %1", "Attachments: %1", attachmentsCount)
				.arg(attachments.join(", ")));
	}

	KNotification::event(d->mThreadsMenu->winId(), "gmail-mail-snippet", message);
	kDebug() << k_funcinfo << "Notification:" << "gmail-mail-snippet" << endl;
}


void KCheckGmailCore::slotMailArrived(unsigned int n)
{
	QString str = newEmailNotifyMessage(n, Prefs::displaySenderOnSingleMail(), Prefs::displaySubjectOnSingleMail(), Prefs::displaySnippetOnSingleMail());

	if (n == 1) {
		connect(KNotification::event("new-gmail-arrived", str, QPixmap(), 
					d->mTray, i18n("Open")), SIGNAL(activated(unsigned int )),
					SLOT(slotOpenButtonClicked()));
	} else {
		KNotification::event("new-gmail-arrived", str, QPixmap(), d->mTray);
	}
	kDebug() << k_funcinfo << "Notification: new-gmail-arrived" \
				<< " number of emails:" << n << endl;
}


void KCheckGmailCore::slotSettingsChanged()
{
	bool loginOk = true;
	QString passwd = d->mConfigDialog->password();
	const QString user = d->mConfigDialog->username();
	int res;

	kDebug() << k_funcinfo << passwd << endl;
	
	if(passwd.length() == 0 ) {
		kDebug() << k_funcinfo << "user: " << user << endl;
		if(user.length() == 0) {
			res = KMessageBox::warningYesNo(0, i18n("No account information has been entered. Do you want to quit?"));

			if( res == KMessageBox::Yes ) {
				emit quitSelected();
				kapp->quit();
			} else {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
			}
		}
	} else {
		
		kDebug() << k_funcinfo << " strncmp: " << (passwd == QString("\007\007\007"))  << endl;

		if(passwd != QString("\007\007\007")) {
			kDebug() << k_funcinfo << "setting wallet" << endl;
			loginOk = GMailWalletManager::instance()->set(d->mConfigDialog->password());
			d->mConfigDialog->erasePassword();
			d->mConfigDialog->insertPassword("\007\007\007");
		} else
			kDebug() << k_funcinfo << "passwd unchanged: " << passwd << endl;
		
		d->mJSP->retriever()->setInterval(Prefs::interval());
		
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
       					KStandardGuiItem::yes(),
					KStandardGuiItem::no(),
					"no_location_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
			}
		}
		if (Prefs::searchFor().contains("is:unread") == 0) {
			res = KMessageBox::questionYesNo(0, 
					i18n("<p>The search string you provided doesn't contain <i>is:unread</i>"
							".</p>"
							"<p>It should be set to ensure more unread messages are retrieved.</p>"
							"<p>Are you sure you want to proceed?</p>"),
					QString::null,
					KStandardGuiItem::yes(),
					KStandardGuiItem::no(),
					"is_unread_check");

			if( res == KMessageBox::No ) {
				QTimer::singleShot(100, this, SLOT(slotShowPrefsDialog()));
			}
		}
	}

	emit countColorChanged(Prefs::trayIconUnreadMessagesColor());
}


void KCheckGmailCore::slotLoginDone(bool ok, bool isExcuseNeeded, const QString& message)
{
	d->mTray->stopAnim();

	if (!ok) {
		if (isExcuseNeeded) {
			KNotification::event(d->mTray->winId(), "gmail-login-no",
					i18n("An error occurred logging in to Gmail<br><b>%1</b>")
						.arg(message));

			kDebug() << k_funcinfo << "Notification: gmail-login-no" \
						<< " error message:" << message << endl;
		}

		d->mTray->setPixmapAuth();
		d->mLoginCheckMailAction->setText(i18n("Login and Chec&k Mail"));
		QToolTip::remove( d->mTray );
		QToolTip::add(d->mTray, i18n("KCheckGMail"));
		
	} else {
		d->mTray->setPixmapEmpty();
		KNotification::event(d->mTray->winId(), "gmail-login-yes", i18n("Now logged in to Gmail!"));

		kDebug() << k_funcinfo << "Notification: gmail-login-yes" << endl;

		d->mLoginCheckMailAction->setText(i18n("Chec&k Mail Now"));
	}
	d->mLoginCheckMailAction->setEnabled(true);
	d->mTray->slotMailCountChanged(0);
}


void KCheckGmailCore::slotLoginStart()
{
	kDebug() << k_funcinfo << endl;
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
	KNotification::event(d->mTray->winId(), "gmail-session-changed", i18n("Another account has been opened, logging out from it!"));
	kDebug() << k_funcinfo << "Notification: gmail-session-changed" << endl;
}


void KCheckGmailCore::slotCheckDone()
{	
	d->mLoginCheckMailAction->setEnabled(true);
}


void KCheckGmailCore::slotLogingOut()
{
	d->mTray->setPixmapEmpty();
	KNotification::event(d->mTray->winId(), "gmail-login-yes", i18n("Now logged in to Gmail!"));
	kDebug() << k_funcinfo << "Notification: gmail-login-yes" << endl;

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

		Q3ValueList<QString> klist = threads->keys();
		Q3ValueList<QString>::iterator iter = klist.begin();
		
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
		return QString::null;
	}
	
	return t.subject;
}


QString KCheckGmailCore::getThreadSender(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	if (t.senders.isEmpty()) {
		return QString::null;
	}
	
	return t.senders;
}


QString KCheckGmailCore::getThreadSnippet(QString msgId)
{
	GMailParser::Thread t = d->mJSP->parser()->getThread(msgId);
	
	if (t.snippet.isEmpty()) {
		return QString::null;
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
