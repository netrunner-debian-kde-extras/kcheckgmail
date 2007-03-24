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
 
#include "gmailparser.h"
#include "gmail_constants.h"

#include <kdebug.h>
#include <qregexp.h>
#include <klocale.h>
#include <kcharsets.h>

GMailParser::GMailParser() :
	mInvites(0)
{
	mSummary.inbox = 0;
// 	mSummary.starred = 0;
	mSummary.drafts = 0;
// 	mSummary.sent = 0;
// 	mSummary.all = 0;
	mSummary.spam = 0;
// 	mSummary.trash = 0;
	
	// NOTE: when adding more supported gmail versions resize()'s value MUST be changed
	gGMailVersion.resize(3);
	//gmail versions kcheckgmail works with:
	gGMailVersion[0] = "1exl39kx7mipo";
	gGMailVersion[1] = "1x4nkpwjfkc8x";
	gGMailVersion[2] = "1ddh9n6glzd1c";

	//gmail language identifiers:
	gGMailLanguageCode.insert("7fba835ed0312d54",i18n("Spanish"));
	gGMailLanguageCode.insert("7530096a84569c0b",i18n("French"));
	gGMailLanguageCode.insert("21208aa200ae6920",i18n("Italian"));
	gGMailLanguageCode.insert("cd21242a38a63f0",i18n("German"));
	gGMailLanguageCode.insert("9930dc54804b344a",i18n("English (US)"));
	gGMailLanguageCode.insert("93a8e3f857e8a529",i18n("English (UK)"));
	gGMailLanguageCode.insert("d414bf5ecc193e94",i18n("Portuguese"));
	gGMailLanguageCode.insert("421a229c26e5115",i18n("Turkish"));
	gGMailLanguageCode.insert("f8c7fb73ac445a2f",i18n("Polish"));
	gGMailLanguageCode.insert("d880d89755cacbab",i18n("Russian"));
	gGMailLanguageCode.insert("a680d09b1f097e52",i18n("Croatian"));
	gGMailLanguageCode.insert("690643eba4fb5b28",i18n("Dutch"));
	gGMailLanguageCode.insert("b6a1b7dea1a8a18",i18n("Hungarian"));
	gGMailLanguageCode.insert("fa2444e0ab7696ed",i18n("Swedish"));
	gGMailLanguageCode.insert("cb207eb0643c6e51",i18n("Norwegian"));
	gGMailLanguageCode.insert("bee0f0eace8c0ee8",i18n("Lithuanian"));
	gGMailLanguageCode.insert("e7b392f9cad18fbb",i18n("Hebrew"));
	gGMailLanguageCode.insert("62efb853bef926",i18n("Greek"));
	gGMailLanguageCode.insert("88595fc43e710562",i18n("Chinese Simplified"));
	gGMailLanguageCode.insert("c368aa1b815d1a8a",i18n("Czech"));
	gGMailLanguageCode.insert("a680d09b1f097e52",i18n("Croatian"));
	gGMailLanguageCode.insert("e35d4c2af8d5feba",i18n("Catalan"));
	gGMailLanguageCode.insert("b8e15ea37ed4f16",i18n("Arabic"));
}

GMailParser::~GMailParser()
{
}

void GMailParser::parse(const QString &_data)
{
	static QRegExp rx("D\\(\\[(.*)\\][\\s\\n]*\\);");
	int pos = 0;

	rx.setMinimal(true);

	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
	} 

	mCurMsgId = 0;
	unsigned int oldNewCount = getNewCount(), NewCount = 0;
	QMap<QString,bool> *oldMap = getThreadList();
	freeThreadList();

	kdDebug() << k_funcinfo << "oldNewCount=" << oldNewCount << endl;

	QString data = QString::fromUtf8(_data);

	while((pos = rx.search(data, pos)) != -1) {
		QString str = rx.cap(1);
		QRegExp rxType("^\"([a-z]+)\",");

		int tokPos = -1;
		if((tokPos = rxType.search(str)) >= 0) {
			QString tok = rxType.cap(1);
			int tokLen = rxType.matchedLength();

			// strip token
			str.remove(tokPos, tokLen);
			
			if(tok == D_THREAD) {
				NewCount += parseThread(str, oldMap);
			} else if(tok == D_VERSION) {
				parseVersion(str);
			} else if(tok == D_QUOTA) {
				parseQuota(str);
			} else if(tok == D_DEFAULTSEARCH_SUMMARY) {
				parseDefaultSummary(str);
			} else if(tok == D_CATEGORIES) {
				parseLabel(str);
			} else if(tok == D_INVITE_STATUS) {
				parseInvite(str);
			} else if(tok == D_GAIA_NAME) {
				parseGName(str);
			}

		}

		pos += rx.matchedLength();
	}

	if(oldMap)
		delete oldMap;

	kdDebug() << k_funcinfo << "NewCount=" << NewCount << endl;
	kdDebug() << k_funcinfo << "oldNewCount=" << oldNewCount << endl;
	
	if(NewCount > 0)
		emit mailArrived(NewCount);
	if(oldNewCount != NewCount)
		emit mailCountChanged();
}

void GMailParser::parseQuota(const QString &data)
{	
	QStringList list = QStringList::split(",",data);
	if(list.size() == 4) {
		QStringList::Iterator iter = list.begin();
		unsigned int i = 0;
		while(iter != list.end()) {
			QString val = *iter;
			val.remove('"');
			switch(i) {
				case 0:
					mQuota.used = val;
					break;
				case 1:
					mQuota.total = val;
					break;
				case 2:
					mQuota.percent = val;
					break;
				case 3:
					mQuota.colour = val;
					break;
				default:
					break;
			}
			iter++;
			i++;
		}
	} else
		kdWarning() << k_funcinfo << "Wrong number of elements in qu: "
			<< list.size() << ", should be: 4." << endl;
}

void GMailParser::parseDefaultSummary(const QString &_data)
{
	static QRegExp rx("\"([a-z]+)\",([0-9]+)");

	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
	}
	QString data = _data;
	int pos = 0;

	while((pos = rx.search(data, pos)) != -1) {
		QString str_name = rx.cap(1), str_val = rx.cap(2);
		int val = str_val.toUInt();

		// TODO: replace this with a nicer switch/case code
		if( QString::compare(str_name,"inbox") == 0)
			mSummary.inbox = val;
		else if( QString::compare(str_name,"drafts") == 0)
			mSummary.drafts = val;
		else if( QString::compare(str_name,"spam") == 0)
			mSummary.spam = val;
		else kdDebug() << k_funcinfo << "unkown identifier " << str_name << endl;

		pos += rx.matchedLength();
	}
	kdDebug() << k_funcinfo << endl  
		<< "inbox=" << mSummary.inbox << "\n"
		<< "drafts=" << mSummary.drafts << "\n"
		<< "spam=" << mSummary.spam << "\n" << endl;
}

void GMailParser::parseLabel(const QString &data)
{
	static QRegExp rx(
		"\\[\"([^\"]+)\""	// label name
		",([0-9]+)\\]"		// unread
		);

	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
	}
	int pos = 0;
	
	kdDebug() << k_funcinfo << endl;

	while((pos = rx.search(data, pos)) != -1) {
		kdDebug() << rx.cap(1) << " has " << rx.cap(2) << " unread messages" << endl;
		pos += rx.matchedLength();
	}
}

void GMailParser::parseGName(const QString &data)
{
	QString newName = data;
	
	newName.remove('"');
	
	if(newName != gName) {
		gName = newName;
		kdDebug() << "Gaia name: " << gName << endl;
		emit gNameChanged(gName);
	}
}

uint GMailParser::parseThread(const QString &_data, const QMap<QString,bool>* oldMap)
{
	//Matches messages when snippets are on
	static QRegExp rx(
		"\\[\"([a-fA-F0-9]+)\"\\s*,"	// replyID
		"\\s*([0-9]+)\\s*,"		// isNew
		"\\s*([0-9]+)\\s*,"		// unknown1
		"\\s*\"([^\"]*)\"\\s*,"		// date_short
		"\\s*\"([^\"]*)\"\\s*,"		// senders
		"\\s*\"([^\"]*)\"\\s*,"		// chevron
		"\\s*\"([^\"]*)\"\\s*,"		// subject
		"\\s*\"([^\"]*)\"\\s*,"		// snippet
		"\\s*\\[([^\\]]*)\\]\\s*,"	// labels
		"\\s*\"([^\"]*)\"\\s*,"		// attachments
		"\\s*\"([a-fA-F0-9]+)\"\\s*,"	// msgID
		"\\s*([0-9]+)\\s*,"		// unknown2
		"\\s*\"([^\"]*)\"\\s*"		// date_long
		"(,\\s*([0-9]+)\\s*,)?"		// unknown3
		"(\\s*\"([^\"]*)\"\\s*,)?"	// unknown4
		"(\\s*([0-9]+)\\s*\\])?"	// unknown5
	 );

	//Matches messages when snippets are off
	static QRegExp rx2(
		"\\[\"([a-fA-F0-9]+)\"\\s*,"	// replyID
		"\\s*([0-9]+)\\s*,"		// isNew
		"\\s*([0-9]+)\\s*,"		// unknown1
		"\\s*\"([^\"]*)\"\\s*,"		// date_short
		"\\s*\"([^\"]*)\"\\s*,"		// senders
		"\\s*\"([^\"]*)\"\\s*,"		// chevron
		"\\s*\"([^\"]*)\"\\s*,"		// subject
		"(\\s*),"			// snippet
		"\\s*\\[([^\\]]*)\\]\\s*,"	// labels
		"\\s*\"([^\"]*)\"\\s*,"		// attachments
		"\\s*\"([a-fA-F0-9]+)\"\\s*,"	// msgID
		"\\s*([0-9]+)\\s*,"		// unknown2
		"\\s*\"([^\"]*)\"\\s*"		// date_long
		"(,\\s*([0-9]+)\\s*,)?"		// unknown3
		"(\\s*\"([^\"]*)\"\\s*,)?"	// unknown4
		"(\\s*([0-9]+)\\s*\\])?"	// unknown5
	 );

	QString data = _data;
	data.replace("\\\"","&quot;");

	int pos = 0;

	rx.setMinimal(true);
	rx2.setMinimal(true);

	if(!rx.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
	}

	if(!rx2.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX2!\n"
				<< rx2.errorString() << endl;
	}
	
	/*
	replyId == msgId if latest message on this 
	thread is not from you
	*/

	unsigned int newMsgCount = 0;

	if(oldMap)
		kdDebug() << k_funcinfo << "oldmap.size=" << oldMap->size() << endl;
	else
		kdDebug() << k_funcinfo << "no oldmap" << endl;

	while((pos = rx.search(data, pos)) != -1) {
		Thread *t = new Thread;
		t->id = mCurMsgId ++;
		t->replyId = rx.cap(1);
		t->isNew = rx.cap(2).toInt();
		t->unknown1 = rx.cap(3).toUInt();
		t->date_short = rx.cap(4);
		t->senders = cleanUpData(rx.cap(5));
		t->chevron = rx.cap(6);
		t->subject = cleanUpData(rx.cap(7));
		t->snippet = cleanUpData(rx.cap(8));
		t->labels = rx.cap(9);
		t->attachments = rx.cap(10);
		t->msgId = rx.cap(11);
		t->unknown2 = rx.cap(12).toUInt();
		t->date_long = rx.cap(13);
		t->unknown3 = rx.cap(14).toUInt();
		t->isNull = false;

		if(t->isNew && (!oldMap || 
			(oldMap->find(t->msgId) == oldMap->end()))) {
			kdDebug() << "Message [" << t->msgId << "] is new." << endl;
			newMsgCount ++;
		} else
			kdDebug() << "Message [" << t->msgId << "] is NOT new." << endl;
		
		// (re-)insert
		mThreads.insert(t->msgId, t);

		pos += rx.matchedLength();
	}

	pos = 0;
	
	// TODO: only perform this if not all msgs were parsed (by checking counters)
	while((pos = rx2.search(data, pos)) != -1) {
		Thread *t = new Thread;
		t->id = mCurMsgId ++;
		t->replyId = rx2.cap(1);
		t->isNew = rx2.cap(2).toInt();
		t->unknown1 = rx2.cap(3).toUInt();
		t->date_short = rx2.cap(4);
		t->senders = cleanUpData(rx2.cap(5));
		t->chevron = rx2.cap(6);
		t->subject = cleanUpData(rx2.cap(7));
		t->snippet = cleanUpData(rx2.cap(8));
		t->labels = rx2.cap(9);
		t->attachments = rx2.cap(10);
		t->msgId = rx2.cap(11);
		t->unknown2 = rx2.cap(12).toUInt();
		t->date_long = rx2.cap(13);
		t->unknown3 = rx2.cap(14).toUInt();
		t->isNull = false;

		if(t->isNew && (!oldMap || 
			(oldMap->find(t->msgId) == oldMap->end()))) {
			kdDebug() << "Message [" << t->msgId << "] is new." << endl;
			newMsgCount ++;
		} else
			kdDebug() << "Message [" << t->msgId << "] is NOT new." << endl;
		
		// (re-)insert
		mThreads.insert(t->msgId, t);

		pos += rx2.matchedLength();
	}

	kdDebug() << k_funcinfo << "Finished searching for threads in: " << endl;
	kdDebug() << data << endl;
	kdDebug() << k_funcinfo << "newMsgCount: " << newMsgCount << endl;

	return newMsgCount;
}

void GMailParser::parseVersion(const QString &_data)
{
	QString data = _data;
	data.remove('"');
	
	kdDebug() << k_funcinfo << "Version string: " << data << endl;
	
	QStringList list = QStringList::split(",",data);
	if(list.size() != 5)
		kdWarning() << k_funcinfo << "Wrong number of elements: "
				<< list.size() << ", should be: 5." << endl;
	QStringList::Iterator iter = list.begin();
	unsigned int i = 0;
	while(iter != list.end()) {
		QString str = *iter;
		switch(i) {
			case 0:
				mVersion.unknown1 = str;
				break;
			case 1:
				mVersion.language = str;
				break;
			case 2:
				mVersion.unknown2 = str.toUInt();
				break;
			case 3:
				mVersion.unknown3 = str.toUInt();
				break;
			case 4:
				mVersion.version = str;
				break;
			default:
				kdWarning() << k_funcinfo << "Unknown version token: " << str << "(" << i <<")" << endl;
				break;
		}
		iter++;
		i++;
	}
	kdDebug() << "GMail version " << mVersion.version << endl;
	
	bool ok = false;
	
	for( i = 0; i < gGMailVersion.size() ; i++ ) {
		if( gGMailVersion[i] == mVersion.version )
			ok = true;
	}
	
	if(gGMailLanguageCode.contains(mVersion.language))
		kdDebug() << "GMail language: " << gGMailLanguageCode[mVersion.language] << endl;
	else
		kdWarning() << k_funcinfo << "Unknown language code: " << mVersion.language << endl;
	
	if(!ok)
	{
		kdWarning() << k_funcinfo << "GMail version " << mVersion.version << " is not supported, check for updates!" << endl;
		emit versionMismatch();
	}
}

void GMailParser::parseInvite(const QString &data)
{
	bool ok = true;
	mInvites = data.toUInt(&ok);
	if(!ok) {
		mInvites = 0;
	}
	kdDebug() << k_funcinfo << "Invites=" << mInvites << endl;
}

QMap<QString, bool> *GMailParser::getThreadList() const
{
	QMap<QString, bool> *ret = 0;

	if(!mThreads.isEmpty()) {
		ret = new QMap<QString, bool>();

		QValueList<QString> klist = mThreads.keys();
		QValueList<QString>::iterator iter = klist.begin();

		while(iter != klist.end()) {
			Thread *t = mThreads[*iter];
			ret->insert(t->msgId, t->isNew);
			iter ++;
		}
	}

	return ret;
}


void GMailParser::freeThreadList()
{
	if(!mThreads.isEmpty()) {

		QValueList<QString> klist = mThreads.keys();
		QValueList<QString>::iterator iter = klist.begin();

		while(iter != klist.end()) {
			Thread *t = mThreads[*iter];
			delete t;
			iter ++;
		}
	}

	mThreads.clear();
}

const GMailParser::Thread& GMailParser::getThread(const QString &msgId) const
{
	static Thread nullThread;

	QMap<QString, Thread*>::const_iterator iter = mThreads.find(msgId);
	
	if(iter == mThreads.end()) {
		nullThread.isNull = true;
		return nullThread;
	} else
		return *(*iter);
}

const GMailParser::Thread& GMailParser::getThread(int id) const
{
	static Thread nullThread;
	Thread *ret = &nullThread;
	ret->isNull = true;

	QMap<QString, Thread*>::const_iterator iter = mThreads.begin();
	
	while(ret->isNull == true && iter != mThreads.end()) {
		Thread *t = *iter;
		if(t->id == id)
			ret = t;
		iter ++;
	}
	
	return *ret;
}

unsigned int GMailParser::getNewCount(bool realCount) const
{
	if(realCount == true)
		return mSummary.inbox;
	
	unsigned int ret = 0;
	QMap<QString, bool> *lst = getThreadList();
	
	if(lst) {
		QMap<QString,bool>::iterator iter;
		iter = lst->begin();
		while(iter != lst->end()) {
			if(*iter == true)
				ret ++;
			iter ++;
		}
	}
	
	return ret;
}

unsigned int GMailParser::getNewCount() const
{
	return getNewCount(false);
}

QString GMailParser::stripTags(QString data)
{
	static QRegExp tags("<[^>]+>|</[^>]+>|<[^>]+/>");
	
	if(!tags.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
				<< tags.errorString() << endl;
	}
	
	data.remove(tags);
	
	return data;
}

QString GMailParser::convertEntities(QString data)
{
	QChar c;
	QString found;
	static QRegExp format("\\\\u([0-9a-zA-Z]{4})");
	
	if(!format.isValid()) {
		kdWarning() << k_funcinfo << "Invalid RX!\n"
				<< format.errorString() << endl;
	}
	
	while(format.search(data) != -1) {
		found = format.cap(1);
		c = QChar(found.toUInt(0,16));
		data.replace("\\u"+format.cap(1),c);
	}
	return data;
}

QString GMailParser::cleanUpData(QString data)
{
	data = convertEntities(data);
	data = stripTags(data);
	data = KCharsets::resolveEntities(data);
	return data;
}

