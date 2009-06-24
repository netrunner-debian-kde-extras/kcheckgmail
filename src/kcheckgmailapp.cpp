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
#include "kcheckgmailcore.h"

#include <QDBusInterface>

KCheckGmailApp::KCheckGmailApp()
{
}

int KCheckGmailApp::newInstance()
{
	static bool secondMe=false;
	if (secondMe) {
		QDBusInterface dbus("org.kcheckgmail.kcheckgmail",
				    "/kcheckgmail",
				    "org.kcheckgmail.kcheckgmail");
		dbus.call("whereAmI");
	} else {
		const KCheckGmailCore& kcgmCore = KCheckGmailCore::instance();

		// Avoid compiler warning about unused kcgmCore
		Q_UNUSED(kcgmCore);
		secondMe = true;
	}
	return 0;
}

#include "kcheckgmailapp.moc"
