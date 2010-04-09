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

#include "jsprotocol.h"

#include <kdebug.h>

namespace KCheckGmail {

JSProtocol::JSProtocol(QObject* parent)
	: QObject(parent),
	  mParser(new GMailParser(this)),
	  mGmail(new GMail(this)),
	  firstTime(true)
{
	connect(mParser, SIGNAL(countUpdate(unsigned int)),
		this, SLOT(slotCountUpdate(unsigned int)));

	connect(mGmail, SIGNAL(loginDone(bool, bool, const QString&)),
		this, SLOT(slotLoginDone(bool, bool, const QString&)));

	connect(mGmail, SIGNAL(checkDone(const QString&)),
		this, SLOT(slotCheckDone(const QString&)));
}


JSProtocol::~JSProtocol()
{
}

GMailParser* JSProtocol::parser()
{
	return mParser;
}

GMail* JSProtocol::retriever()
{
	return mGmail;
}


void JSProtocol::slotCheckDone(const QString& data)
{
	mParser->parse(data);
	emit checkDone();
}


void JSProtocol::slotLoginDone(bool ok, bool evtFromTimer, const QString &why)
{
	static QString lastExcuse = "";
	bool excuseNeeded = false;

	kDebug() << k_funcinfo << endl << "ok=" << ok << " evtFromTimer=" <<
			evtFromTimer << " why=" << why << endl << endl;

	if(!ok) {
		slotLoggedOut();
		if(why != lastExcuse || !evtFromTimer) {
			lastExcuse = why;
			excuseNeeded = true;
		}
	}
	emit loginDone(ok, excuseNeeded, why);
}


void JSProtocol::slotLoggedOut()
{
	firstTime = true;
	mCount.reset();
}


void JSProtocol::slotCountUpdate(unsigned int currentParsed)
{
	int currentTotal = mParser->unread(GMailParser::TotalCount);
	mCount.setCount(currentTotal, currentParsed);

	kDebug() << k_funcinfo;
	kDebug() << "firstTime: " << firstTime;
	kDebug() << "previousParsed: " << mCount.previousParsed();
	kDebug() << "currentParsed: " << currentParsed;
	kDebug() << "previousTotal: " << mCount.previousTotal();
	kDebug() << "currentTotal: " << currentTotal;

	if (firstTime) {
		firstTime = false;
		if (currentTotal != 0) {
			emit mailArrived(currentTotal);
			emit mailCountChanged(currentTotal);
		}
	} else {
		if (mCount.previousTotal() != currentTotal)
			emit mailCountChanged(currentTotal);

		if (mCount.previousParsed() != currentParsed && currentParsed != 0)
			emit mailArrived(currentParsed);
	}

	if (currentTotal == 0) {
		emit noUnreadMail();
	}
	emit threadsChanged();
}

int JSProtocol::unread() const
{
	return mCount.currentTotal();
}

} // namespace KCheckGmail

#include "jsprotocol.moc"
