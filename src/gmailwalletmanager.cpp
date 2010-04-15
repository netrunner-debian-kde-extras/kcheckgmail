#include "gmailwalletmanager.h"
#include "prefs.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kcodecs.h>


GMailWalletManager *GMailWalletManager::mInstance = 0;

GMailWalletManager::GMailWalletManager()
{
	mWallet = 0;
	mUseWallet = Prefs::passwordFromWallet();
}

GMailWalletManager::~GMailWalletManager()
{
	delete mWallet;
	mWallet = 0;
}

GMailWalletManager *GMailWalletManager::instance()
{
	if(!mInstance)
		mInstance = new GMailWalletManager();
	return mInstance;
}

bool GMailWalletManager::set(const QString &p)
{
	bool ret = true;
	KMD5 md5(p.toUtf8());
		
	kDebug() << "Password=" << p.length()
			 << " hash=" << md5.hexDigest();
	
	mHash = md5.hexDigest();
	
	mPassword = p;

	if(Prefs::passwordFromWallet()) {
		kDebug() << "PasswordFromWallet";
		Prefs::setGmailPassword("");
		Prefs::self()->writeConfig();
		ret = storeWallet();
	} else {
		kDebug() << "PasswordFromKConfig";
		ret = storeKConfig();
	}

	return ret;
}

bool GMailWalletManager::get()
{
	bool ret = true;

	kDebug();

	if(Prefs::passwordFromWallet()) {
		kDebug() << "yeah, from wallet";
		
		Prefs::setGmailPassword("");
		Prefs::self()->writeConfig();
		if(mWallet) {
			kDebug() << "wallet exists";
			if(mWallet->isOpen()) {
				kDebug() << "wallet open";
				QString ret;
				mWallet->readPassword("gmailPassword", ret);
				kDebug() << "Got password";
				KMD5 md5(ret.toUtf8());
				mHash = md5.hexDigest();
				emit getWalletPassword(ret);
			}
		} else {
			kDebug() << "wallet NOT open, callback";
			ret = getWallet();
		}
	} else {
		kDebug() << "from kconfig";
		ret = getKConfig();
	}


	kDebug() << "return " << ret;
	
	return ret;

}

void GMailWalletManager::openWallet()
{
	if(!mWallet) {
		kDebug() << "calling openWallet";
		mWallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
		                    0, KWallet::Wallet::Asynchronous);
		if(!mWallet)
			KMessageBox::error(0, i18n("KCheckGMail could not open "
				"the wallet. Please check your preferences."));
		else {
			kDebug() << "connecting wallet";
			connect(mWallet, SIGNAL(walletOpened(bool)), SLOT(slotWalletChangedStatus()));
		}
	}
}

// Kopete is a strong influence here. Cheers to those guys!
void GMailWalletManager::slotWalletChangedStatus()
{
	kDebug();

	if(!mWallet)
		kDebug() << "status changed but mWallet == 0";
	else
	if(mWallet->isOpen()) {
		kDebug() << "Wallet Open!";
		if(!mWallet->hasFolder(QLatin1String("KCheckGmail"))) {
			kDebug() << "Creating folder";
			mWallet->createFolder(QLatin1String("KCheckGmail"));
		}

		if(mWallet->setFolder(QLatin1String("KCheckGmail"))) {
			kDebug() << "Setting folder";
			// success!
			QObject::connect(mWallet, SIGNAL(walletClosed()), 
				this, SLOT(slotCloseWallet()));

			if(mPassword.length() > 0) {
				mWallet->writePassword("gmailPassword", mPassword);
				clearPassword();
				emit setWalletPassword(true);
			}

			QString ret;
			mWallet->readPassword("gmailPassword", ret);
			kDebug() << "Got pass: " << ret;
			KMD5 md5(ret.toUtf8());
			mHash = md5.hexDigest();
			emit getWalletPassword(ret);
			clearPassword();

		} else {
			// opened OK, but we can't use it
			kDebug() << "Could not set folder";
			delete mWallet;
			mWallet = 0;
		}


	} else {
		kDebug() << "Wallet not open!";
		delete mWallet;
		mWallet = 0;
	}
}

void GMailWalletManager::slotCloseWallet()
{
	delete mWallet;
	mWallet = 0;
}

bool GMailWalletManager::storeWallet()
{
	bool ret = true;
	kDebug();

	if(KWallet::Wallet::isEnabled()) {
		if(mWallet && mWallet->isOpen()) {
			kDebug() << "Wallet open. Setting immediately.";
			mWallet->writePassword("gmailPassword", mPassword);
			clearPassword();
			emit setWalletPassword(true);
		} else {
			kDebug() << "Wallet Not Open..";
			openWallet();
		}
	} else {
		if(KMessageBox::warningContinueCancel(0, 
			i18n("KCheckGMail cannot store your password securely in your wallet. "
			"Do you want to save the password in the unsafe configuration file instead?"),
			i18n("Unable to store secure password"),
			KGuiItem(i18n("Store unsafe"), "unlock"),
			KStandardGuiItem::cancel(),
			"FallbackToKConfig") != KMessageBox::Continue) {
			ret = false;
		} else {
			Prefs::setPasswordFromWallet(false);
			storeKConfig();
		}
	}

	return ret;
}

bool GMailWalletManager::storeKConfig()
{
	bool ret = true;
	kDebug();
	Prefs::setGmailPassword(mPassword);
	Prefs::self()->writeConfig();
	clearPassword();
	emit setWalletPassword(true);
	return ret;
}

bool GMailWalletManager::getWallet()
{
	bool ret = true;
	kDebug();

	if(KWallet::Wallet::isEnabled()) {
		kDebug() << "it's enabled";
		// just have to rely on not calling this method twice for now. No checks
		// are in place.
		if(mWallet && mWallet->isOpen()) {
			kDebug() << "Wallet open.";
			QString p;
			mWallet->readPassword("gmailPassword", p);
			kDebug() << "p=" << p;
			KMD5 md5(p.toUtf8());
			mHash = md5.hexDigest();
			emit getWalletPassword(p);
		} else
			openWallet();
	} else {
		if(KMessageBox::warningContinueCancel(0, 
			i18n("KCheckGMail cannot retrieve your password from your wallet. "
			"Do you want to save the password in the unsafe configuration file instead?"),
			i18n("Unable to retrieve secure password"),
			KGuiItem(i18n("Store unsafe"), "unlock"),
			KStandardGuiItem::cancel(),
			"FallbackToKConfig") != KMessageBox::Continue) {
			ret = false;
		} else
			ret = getKConfig();
		
		kDebug() << "Returned from getKConfig()";
		
	}
	return ret;
}

bool GMailWalletManager::getKConfig()
{
	bool ret = true;
	kDebug();

	KMD5 md5(Prefs::gmailPassword().toUtf8());
	mHash = md5.hexDigest();
	emit getWalletPassword(Prefs::gmailPassword());

	return ret;
}

void GMailWalletManager::clearPassword()
{
	kDebug();

	mPassword.fill('0');
	mPassword = "";
}

#include "gmailwalletmanager.moc"
