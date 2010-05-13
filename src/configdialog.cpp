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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#include "configdialog.h"
#include "prefs.h"

#include <kpassworddialog.h>
#include <klocale.h>
#include <KMessageBox>

namespace KCheckGmail {

ConfigDialog::ConfigDialog(QWidget* parent, const char* name,  KConfigSkeleton* config)
 : KConfigDialog(parent, name, config)
{

	// Copied from KCheckGmailTray::initConfigDialog();

	setFaceType(KPageDialog::List);
	setButtons(Ok | Cancel);

	QWidget* lwid = new QWidget();
	mLoginSettings.setupUi(lwid);
	addPage(lwid, i18n("Login"), "user-properties", i18n("Login Settings"), true);

	QWidget* nwid = new QWidget();
	mNetworkSettings.setupUi(nwid);
	addPage(nwid, i18n("Network"), "preferences-system-network", i18n("Network Settings"), true);

	QWidget* awid = new QWidget();
	mAppletSettings.setupUi(awid);
	addPage(awid, i18n("Behavior"), "configure", i18n("Behavior Settings"), true);

	QWidget* apwid = new QWidget();
	mAppearanceSettings.setupUi(apwid);
	addPage(apwid, i18n("Appearance"), "preferences-desktop-theme", i18n("Appearance Settings"), true);

	QWidget* adwid = new QWidget();
	mAdvancedSettings.setupUi(adwid);
	addPage(adwid, i18n("Advanced"), "preferences-system-settings", i18n("Advanced Settings"), true);

	mLoginSettings.gmailPassword->clear();
	mLoginSettings.gmailPassword->setText("\007\007\007");
}


ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::erasePassword()
{
	mLoginSettings.gmailPassword->clear();
}

void ConfigDialog::insertPassword(const char* passwd)
{
	mLoginSettings.gmailPassword->insert(passwd);
}

QString ConfigDialog::password() const
{
	return mLoginSettings.gmailPassword->text();
}

QString ConfigDialog::username() const
{
	return mLoginSettings.kcfg_GmailUsername->originalText();
}

void ConfigDialog::slotButtonClicked(int button)
{
       if (button == KDialog::Ok) {
               if(mAppletSettings.kcfg_CustomBrowser->text().isEmpty()) {
                       KMessageBox::error(0,
                       i18n("You have to specify a console command to launch the alternative browser"));
                       return;
               }
	}
	KConfigDialog::slotButtonClicked(button);
}


} // namespace KCheckGmail

#include "configdialog.moc"
