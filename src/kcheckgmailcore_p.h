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

#ifndef KCHECKGMAILCORE_P_H
#define KCHECKGMAILCORE_P_H


// if catchAccidentalClick is true, wait 3 seconds before opening another
// browser window
#define ACCIDENTAL_CLICK_TIMEOUT (3 * 1000)

namespace KCheckGmail {
	class JSProtocol;
	class ConfigDialog;
}

using KCheckGmail::JSProtocol;
using KCheckGmail::ConfigDialog;

class KActioncollection;
class KAction;
class KPopupmenu;
class KConfig;
class KIconeffect;
class KMimetype;
class KGlobal;
class KHelpMenu;
class KIconloader;


/**
 * This class cointains code copied and/or moved from several locations
 * within the KCheckGmail project.
 *
 * @author Luis Pereira <luis.artur.pereira@gmail.com>
 */
class KCheckGmailCore::Private {

public:
	KActionCollection* actions;

	KAction* actionShowKNotifyDialog;
	KAction* actionShowPrefsDialog;
	KAction* mLoginCheckMailAction;
	KAction* actionLaunchBrowser;
	KAction* actionComposeMail;

	KPopupMenu* menu;
	KPopupMenu* mThreadsMenu;
	int mThreadsMenuId;
	KHelpMenu* mHelpMenu;

	ConfigDialog* mConfigDialog;

	KCheckGmailTray* mTray;
	JSProtocol* mJSP;

	Private();
	~Private();

};


KCheckGmailCore::Private::Private()
	: actions(0),

	  actionShowKNotifyDialog(0),
	  actionShowPrefsDialog(0),
	  mLoginCheckMailAction(0),
	  actionLaunchBrowser(0),
	  actionComposeMail(0),

	  menu(0),
	  mThreadsMenu(0),
	  mThreadsMenuId(0),
	  mHelpMenu(0),

	  mConfigDialog(0),

	  mTray(0),
	  mJSP(0)
{
}


KCheckGmailCore::Private::~Private()
{
}


#endif
