/***************************************************************************
 *   Copyright (C) 2004 by Matthew Wlazlo                                  *
 *   mwlazlo@gmail.com                                                     *
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

#include <stdlib.h>

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
		I18N_NOOP("KCheckGmail"), 
		kcheckgmailVersion, 
		I18N_NOOP(
		"System tray application to display how many\nnew email "
		"messages you have in your Gmail account."),
		KAboutData::License_GPL, 
		"(C) 2004 Matthew Wlazlo", 
		0, // text
		"http://kcheckgmail.sf.net",
		"atomo64@gmail.com");//http://sf.net/tracker/?group_id=116095&atid=673717
	
	about.addAuthor("Matthew Wlazlo", I18N_NOOP("Original author"), "mwlazlo@gmail.com");
	about.addAuthor("Raphael Geissert", I18N_NOOP("Maintainer"), "atomo64@gmail.com");
	
	about.addCredit("Rogério Pereira Araújo", I18N_NOOP("Brazilian Portuguese Translation"), "rogerio.araujo@gmail.com");
	about.addCredit("Samuele Kaplun", I18N_NOOP("Italian Translation"), "kaplun@aliceposta.it");
	about.addCredit("Felipe Morales", I18N_NOOP("Spanish Translation"), "felipe.morales@wanadoo.es");
	about.addCredit("Marcus Thiesen", I18N_NOOP("German Translation"), "marcus@thiesen.org");
	about.addCredit("Dudalev Michael", I18N_NOOP("Russian Translation"), "dudalev@gmail.com");
	about.addCredit("Patrick Trettenbrein", I18N_NOOP("German Translation Updates"), "patrick.trettenbrein@gmx.net");
	about.addCredit("Alexis Bunel", I18N_NOOP("French Translation"), "alexisbunel@gmail.com");
	about.addCredit("Jarosław Kamper", I18N_NOOP("Polish Translation"), "jaroslawkamper@gmail.com");
	about.addCredit("Uğur Çetin", I18N_NOOP("Turkish Translation"), "jnmbk@users.sourceforge.net");
	about.addCredit("Peter Avramucz", I18N_NOOP("Hungarian Translation"), "muczy@freestart.hu");
	about.addCredit("Daniel Nylander", I18N_NOOP("Swedish Translation"), "po@danielnylander.se");
	about.addCredit("Andrius Štikonas", I18N_NOOP("Lithuanian Translation"), "stikonas@gmail.com");
	about.addCredit("Henrik Pihl", I18N_NOOP("Estonian Translation"), "ahvenas@gmail.com");


        KGlobal::locale()->setMainCatalogue("kcheckgmail");

        KCmdLineArgs::init(argc, argv, &about);
        KCmdLineArgs::addCmdLineOptions(gOptions);
        KApplication::addCmdLineOptions();
	
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	
	if(args->isSet("legal")) {
		printf (i18n("Legal Information:\nGoogle, Gmail and Google Mail are registered trademarks of Google Inc.\nKCheckGMail nor it's authors are in any way affiliated nor endorsed by Google Inc.") + "\n");
		return EXIT_SUCCESS;
	}
	
	if (!KUniqueApplication::start()) {
		KStartupInfo::handleAutoAppStartedSending();
		fprintf(stderr, i18n("KCheckGMail is already running!\n"));
		return EXIT_SUCCESS;
	}
	
	KCheckGmailApp app;

        return app.exec();
}
