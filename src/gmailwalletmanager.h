#ifndef __GMAIL_WALLET_MANAGER_H
#define __GMAIL_WALLET_MANAGER_H

#include <qobject.h>
#include <qmutex.h>
#include <kwallet.h>


class GMailWalletManager : public QObject
{
	Q_OBJECT
private:
	GMailWalletManager();
public:
	~GMailWalletManager();
	static GMailWalletManager *instance();

	// set he password
	bool set(const QString& str);

	// put in a request to get the password
	bool get();

	const QString& getHash() const { return mHash; }

protected:
	void openWallet();
	bool storeWallet();
	bool storeKConfig();
	bool getWallet();
	bool getKConfig();
	void clearPassword();

protected slots:
	void slotWalletChangedStatus();
	void slotCloseWallet();
	
signals:
	void setWalletPassword(bool);
	void getWalletPassword(const QString&);

private:
	static GMailWalletManager *mInstance;

	KWallet::Wallet *mWallet;
	QString mPassword;
	QString mHash;
	QMutex mMux;
	bool mUseWallet;
};

#endif
