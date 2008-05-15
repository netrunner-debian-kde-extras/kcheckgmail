/***************************************************************************
 *   Copyright (C) 2008 by Luis Pereira <luis.artur.pereira@gmail.com>     *
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
#include "configdialog.h"
#include "prefs.h"

#include "appletsettingswidget.h"
#include "loginsettingswidget.h"
#include "netsettingswidget.h"
#include "advancedsettingswidget.h"

#include <kpassdlg.h>
#include <klocale.h>

namespace KCheckGmail {

ConfigDialog::ConfigDialog(QWidget* parent, const char* name,  KConfigSkeleton* config, DialogType dialogType, int dialogButtons)
 : KConfigDialog(parent, name, config, dialogType, dialogButtons)
{

	// Copied from KCheckGmailTray::initConfigDialog();

	mLoginSettings = new LoginSettingsWidget(this, "LoginSettings");
	addPage(mLoginSettings, i18n("Login"), "kcheckgmail", i18n("Login Settings"));

	NetworkSettingsWidget *nwid = new NetworkSettingsWidget(this, "NetworkSettings");
	addPage(nwid, i18n("Network"), "www", i18n("Network Settings"));

	AppletSettingsWidget *awid = new AppletSettingsWidget(this, "AppletSettings");
	addPage(awid, i18n("Behavior"), "configure", i18n("Behavior"));

	AdvancedSettingsWidget *cwid = new AdvancedSettingsWidget(this, "AdvancedSettings");
	addPage(cwid, i18n("Advanced"), "package_settings", i18n("Advanced Settings"));

	mLoginSettings->gmailPassword->erase();
	mLoginSettings->gmailPassword->insert("\007\007\007");
}


ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::erasePassword()
{
	mLoginSettings->gmailPassword->erase();
}

void ConfigDialog::insertPassword(const char* passwd)
{
	mLoginSettings->gmailPassword->insert(passwd);
}

const char* ConfigDialog::password() const
{
	return mLoginSettings->gmailPassword->password();
}

QString ConfigDialog::username() const
{
	return mLoginSettings->kcfg_GmailUsername->originalText();
}

} // namespace KCheckGmail

#include "configdialog.moc"
