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

#include <cstdlib>
#include <iostream>

#include "config.h"

static const char kcheckgmailVersion[] = VERSION;

int main(int argc, char **argv)
{
	KAboutData about("kcheckgmail",
		0,
		ki18n("KCheckGMail"),
		kcheckgmailVersion, 
		ki18n(
		"System tray application to display how many\nnew email "
		"messages you have in your Gmail account."),
		KAboutData::License_GPL, 
		ki18n("(C) 2004 Matthew Wlazlo\n(C) 2007 Raphael Geissert\n(C) 2007 Luís Pereira"),
		KLocalizedString(), // text
		"http://kcheckgmail.sf.net",
		"kcheckgmail-development@lists.sourceforge.net");

	// needed to get org.kcheckgmail.kcheckgmail used for D-BUS
	about.setOrganizationDomain("kcheckgmail.org");
	
	about.addAuthor(ki18n("Matthew Wlazlo"), ki18n("Original author"), "mwlazlo@gmail.com");
	about.addAuthor(ki18n("Raphael Geissert"), ki18n("Maintainer"), "atomo64@gmail.com");
	about.addAuthor(ki18n("Luís Pereira"), ki18n("Developer"), "luis.artur.pereira@gmail.com", "http://kcheckgmail-lpereira.blogspot.com");
	about.addCredit(ki18n("Everybody who helped testing and translating KCheckGMail"));


	KGlobal::locale()->setMainCatalog("kcheckgmail");

	KCmdLineArgs::init(argc, argv, &about);
	KCmdLineOptions options;

	options.add("legal", ki18n("Display legal information"));
	KCmdLineArgs::addCmdLineOptions(options);
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
