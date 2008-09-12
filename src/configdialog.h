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

#ifndef KCHECKGMAIL_CONFIGDIALOG_H
#define KCHECKGMAIL_CONFIGDIALOG_H

#include <kconfigdialog.h>
#include <klineedit.h>

class LoginSettingsWidget;
class NetworkSettingsWidget;
class AppletSettingsWidget;
class AdvancedSettingsWidget;

namespace KCheckGmail {

class ConfigDialog : public KConfigDialog {
	Q_OBJECT
public:
	ConfigDialog(QWidget *parent, const char *name, KConfigSkeleton *config, DialogType dialogType = IconList, int dialogButtons = Ok|Cancel);
	virtual ~ConfigDialog();

	void erasePassword();
	void insertPassword(const char* passw);
	const char* password() const;
	QString username() const;

private:
	LoginSettingsWidget* mLoginSettings;
};

} // namespace KCheckGmail

#endif
