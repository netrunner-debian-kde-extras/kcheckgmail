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
	KMD5 md5(p);
		
	kDebug() << k_funcinfo << "Password=" << p.length()
			 << " hash=" << md5.hexDigest() << endl;
	
	mHash = md5.hexDigest();
	
	mPassword = p;

	if(Prefs::passwordFromWallet()) {
		kDebug() << k_funcinfo << "PasswordFromWallet" << endl;
		Prefs::setGmailPassword("");
		Prefs::self()->writeConfig();
		ret = storeWallet();
	} else {
		kDebug() << k_funcinfo << "PasswordFromKConfig" << endl;
		ret = storeKConfig();
	}

	return ret;
}

bool GMailWalletManager::get()
{
	bool ret = true;

	kDebug() << k_funcinfo << endl;

	if(Prefs::passwordFromWallet()) {
		kDebug() << k_funcinfo << "yeah, from wallet" << endl;
		
		Prefs::setGmailPassword("");
		Prefs::self()->writeConfig();
		if(mWallet) {
			kDebug() << k_funcinfo << "wallet exists" << endl;
			if(mWallet->isOpen()) {
				kDebug() << k_funcinfo << "wallet open" << endl;
				QString ret;
				mWallet->readPassword("gmailPassword", ret);
				kDebug() << k_funcinfo << "Got password" << endl;
				KMD5 md5(ret);
				mHash = md5.hexDigest();
				emit getWalletPassword(ret);
			}
		} else {
			kDebug() << k_funcinfo << "wallet NOT open, callback" << endl;
			ret = getWallet();
		}
	} else {
		kDebug() << k_funcinfo << "from kconfig" << endl;
		ret = getKConfig();
	}


	kDebug() << k_funcinfo << "return " << ret << endl;
	
	return ret;

}

void GMailWalletManager::openWallet()
{
	if(!mWallet) {
		kDebug() << k_funcinfo << "calling openWallet" << endl;
		mWallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
		                    0, KWallet::Wallet::Asynchronous);
		if(!mWallet)
			KMessageBox::error(0, i18n("KCheckGMail could not open "
				"the wallet. Please check your preferences."));
		else {
			kDebug() << k_funcinfo << "connecting wallet" << endl;
			connect(mWallet, SIGNAL(walletOpened(bool)), SLOT(slotWalletChangedStatus()));
		}
	}
}

// Kopete is a strong influence here. Cheers to those guys!
void GMailWalletManager::slotWalletChangedStatus()
{
	kDebug() << k_funcinfo << endl;

	if(!mWallet)
		kDebug() << k_funcinfo << "status changed but mWallet == 0" << endl;
	else
	if(mWallet->isOpen()) {
		kDebug() << k_funcinfo << "Wallet Open!" << endl;
		if(!mWallet->hasFolder(QString::fromLatin1("KCheckGmail"))) {
			kDebug() << k_funcinfo << "Creating folder" << endl;
			mWallet->createFolder(QString::fromLatin1("KCheckGmail"));
		}

		if(mWallet->setFolder(QString::fromLatin1("KCheckGmail"))) {
			kDebug() << k_funcinfo << "Setting folder" << endl;
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
			kDebug() << k_funcinfo << "Got pass: " << ret << endl;
			KMD5 md5(ret);
			mHash = md5.hexDigest();
			emit getWalletPassword(ret);
			clearPassword();

		} else {
			// opened OK, but we can't use it
			kDebug() << k_funcinfo << "Could not set folder" << endl;
			delete mWallet;
			mWallet = 0;
		}


	} else {
		kDebug() << k_funcinfo << "Wallet not open!" << endl;
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
	kDebug() << k_funcinfo << endl;

	if(KWallet::Wallet::isEnabled()) {
		if(mWallet && mWallet->isOpen()) {
			kDebug() << k_funcinfo << "Wallet open. Setting immediately." << endl;
			mWallet->writePassword("gmailPassword", mPassword);
			clearPassword();
			emit setWalletPassword(true);
		} else {
			kDebug() << k_funcinfo << "Wallet Not Open.." << endl;
			openWallet();
		}
	} else {
		if(KMessageBox::warningContinueCancel(0, 
			i18n("KCheckGMail cannot store your password securely in your wallet. "
			"Do you want to save the password in the unsafe configuration file instead?"),
			i18n("Unable to store secure password"),
			KGuiItem(i18n("Store unsafe"), "unlock"),
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
	kDebug() << k_funcinfo << endl;
	Prefs::setGmailPassword(mPassword);
	Prefs::self()->writeConfig();
	clearPassword();
	emit setWalletPassword(true);
	return ret;
}

bool GMailWalletManager::getWallet()
{
	bool ret = true;
	kDebug() << k_funcinfo << endl;

	if(KWallet::Wallet::isEnabled()) {
		kDebug() << k_funcinfo << "it's enabled" << endl;
		// just have to rely on not calling this method twice for now. No checks
		// are in place.
		if(mWallet && mWallet->isOpen()) {
			kDebug() << k_funcinfo << "Wallet open." << endl;
			QString p;
			mWallet->readPassword("gmailPassword", p);
			kDebug() << k_funcinfo << "p=" << p << endl;
			KMD5 md5(p);
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
			"FallbackToKConfig") != KMessageBox::Continue) {
			ret = false;
		} else
			ret = getKConfig();
		
		kDebug() << k_funcinfo << "Returned from getKConfig()" << endl;
		
	}
	return ret;
}

bool GMailWalletManager::getKConfig()
{
	bool ret = true;
	kDebug() << k_funcinfo << endl;

	KMD5 md5(Prefs::gmailPassword());
	mHash = md5.hexDigest();
	emit getWalletPassword(Prefs::gmailPassword());

	return ret;
}

void GMailWalletManager::clearPassword()
{
	kDebug() << k_funcinfo << endl;

	mPassword.fill('0');
	mPassword = "";
}

#include "gmailwalletmanager.moc"
