/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo                                  *
 *   mwlazlo@gmail.com                                                          *
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

#include <stdlib.h>

static const char kcheckgmailVersion[] = "0.2";
static const KCmdLineOptions gOptions[] =
{
	{ "login", I18N_NOOP("Application is being auto-started at KDE session start"), 0L },
	KCmdLineLastOption
};

 

int main(int argc, char **argv)
{
	KAboutData about("kcheckgmail",
		I18N_NOOP("KCheckGmail"), 
		kcheckgmailVersion, 
		"Kicker applet to display how many email messages you have in your Gmail account.",
		KAboutData::License_GPL, 
		"(C) 2004 Matthew Wlazlo", 
		0, // text
		0, // homePageAddress
		"mwlazlo@gmail.com");
	about.addAuthor("Matthew Wlazlo", 0, "mwlazlo@gmail.com");

        KGlobal::locale()->setMainCatalogue("kcheckgmail");

        KCmdLineArgs::init(argc, argv, &about);
        KCmdLineArgs::addCmdLineOptions(gOptions);
        KApplication::addCmdLineOptions();

        KCheckGmailApp app;

        return app.exec();


	return EXIT_SUCCESS;
}
