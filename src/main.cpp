/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo <mwlazlo@gmail.com>              *
 *   Copyright (C) 2007 by Raphael Geissert <atomo64@gmail.com>            *
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

#include "kcheckgmailapp.h"

#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kstartupinfo.h>

#include <iostream>

#include "config.h"

static const char kcheckgmailVersion[] = VERSION;
static const KCmdLineOptions gOptions[] =
{
	{ "legal", I18N_NOOP("Display legal information"), 0 },
	KCmdLineLastOption
};

int main(int argc, char **argv)
{
	KAboutData about("kcheckgmail",
		I18N_NOOP("KCheckGMail"), 
		kcheckgmailVersion, 
		I18N_NOOP(
		"System tray application to display how many\nnew email "
		"messages you have in your Gmail account."),
		KAboutData::License_GPL, 
		"(C) 2004 Matthew Wlazlo\n(C) 2007 Raphael Geissert\n(C) 2007 Luis Pereira", 
		0, // text
		"http://kcheckgmail.sf.net",
		"kcheckgmail-development@lists.sourceforge.net");
	
	about.addAuthor("Matthew Wlazlo", I18N_NOOP("Original author"), "mwlazlo@gmail.com");
	about.addAuthor("Raphael Geissert", I18N_NOOP("Maintainer"), "atomo64@gmail.com");
	about.addAuthor("Luis Pereira", I18N_NOOP("Developer"), "luis.artur.pereira@gmail.com");
	about.addCredit(I18N_NOOP("Everybody who helped testing and translating KCheckGMail"), 0, 0, 0);


        KGlobal::locale()->setMainCatalogue("kcheckgmail");

        KCmdLineArgs::init(argc, argv, &about);
        KCmdLineArgs::addCmdLineOptions(gOptions);
        KUniqueApplication::addCmdLineOptions();
	
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	
	if(args->isSet("legal")) {
		std::cout << (i18n("Legal Information:\nGoogle, Gmail and Google Mail are registered trademarks of Google Inc.\nKCheckGMail nor its authors are in any way affiliated nor endorsed by Google Inc.") + "\n");
		return EXIT_SUCCESS;
	}
	
	if (!KUniqueApplication::start()) {
		KStartupInfo::handleAutoAppStartedSending();
		std::cerr << i18n("KCheckGMail is already running!\n");
		return EXIT_SUCCESS;
	}
	
	KCheckGmailApp app;

        return app.exec();
}
