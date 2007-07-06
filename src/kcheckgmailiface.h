/***************************************************************************
 *   Copyright (C) 2005 by James Stembridge <jstembridge@gmail.com>        *
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

#include "dcopobject.h"
#include <qstringlist.h>

class KCheckGmailIface : virtual public DCOPObject
{
	K_DCOP
	k_dcop:
	
	virtual int mailCount() const = 0;
	virtual void checkMailNow() = 0;
	virtual void whereAmI() = 0;
	virtual void showIcon() = 0;
	virtual void hideIcon() = 0;
	virtual QStringList getThreads() = 0;
	virtual QString getThreadSubject(QString msgId) = 0;
	virtual QString getThreadSender(QString msgId) = 0;
	virtual QString getThreadSnippet(QString msgId) = 0;
	virtual QStringList getThreadAttachments(QString msgId) = 0;
	virtual bool isNewThread(QString msgId) = 0;
};

