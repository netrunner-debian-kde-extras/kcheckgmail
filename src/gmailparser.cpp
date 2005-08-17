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

GMailParser::GMailParser() :
	mVersion(gGMailVersion),
	mInvites(0)
{
	mSummary.inbox = 0;
	mSummary.starred = 0;
	mSummary.sent = 0;
	mSummary.all = 0;
	mSummary.spam = 0;
	mSummary.trash = 0;
}

GMailParser::~GMailParser()
{
}

void GMailParser::parse(const QString &data)
{
	QRegExp rx("D\\(\\[(.*)\\][\\s\\n]*\\);");
	int pos = 0;

	rx.setMinimal(true);

	if(!rx.isValid()) {
		kdDebug() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
	} 

	mCurMsgId = 0;
	unsigned int oldNewCount = getNewCount();
	QMap<QString,bool> *oldMap = getThreadList();
	freeThreadList();

	kdDebug() << k_funcinfo << "oldNewCount=" << oldNewCount << endl;

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
				parseThread(str, oldMap);
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
			}

		} 

		pos += rx.matchedLength();
	}

	if(oldMap)
		delete oldMap;

	kdDebug() << k_funcinfo << "getNewCount()=" << getNewCount() << endl;
	kdDebug() << k_funcinfo << "oldNewCount=" << oldNewCount << endl;
	if(oldNewCount != getNewCount())
		emit mailCountChanged();
}

void GMailParser::parseQuota(const QString &data)
{
	QStringList list = QStringList::split(",",data);
	if(list.size() == 4) {
		QStringList::Iterator iter = list.begin();
		int i = 0;
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
		kdDebug() << k_funcinfo << "Wrong number of elements in qu: "
			<< list.size() << ", should be: 4." << endl;
}

void GMailParser::parseDefaultSummary(const QString &data)
{
	QStringList list = QStringList::split(",",data);
	if(list.size() == 7) {
		QStringList::Iterator iter = list.begin();
		int i = 0;
		while(iter != list.end()) {
			QString str = *iter;
			int val = str.toUInt();
			switch(i) {
				case 0:
					mSummary.inbox = val;
					break;
				case 1:
					mSummary.starred = val;
					break;
				case 2:
					mSummary.sent = val;
					break;
				case 3:
					mSummary.all = val;
					break;
				case 4:
					mSummary.spam = val;
					break;
				case 5:
					mSummary.trash = val;
					break;
				default:
					break;
			}
			iter++;
			i++;
		}

		kdDebug() << k_funcinfo << endl  
			<< "inbox=" << mSummary.inbox << "\n"
			<< "starred=" << mSummary.starred << "\n"
			<< "sent=" << mSummary.sent << "\n"
			<< "all=" << mSummary.all << "\n"
			<< "spam=" << mSummary.spam << "\n"
			<< "trash=" << mSummary.trash << endl;

	} else
		kdDebug() << k_funcinfo << "Wrong number of elements in ds: "
			<< list.size() << ", should be: 7." << endl;
}

void GMailParser::parseLabel(const QString &data)
{
	kdDebug() << k_funcinfo 
		<< "\n+++Data++\n" << data 
		 << "\n---Data---\n" << endl;
}

void GMailParser::parseThread(const QString &_data, const QMap<QString,bool>* oldMap)
{
	static QRegExp killEscapes("\\\\\"");
	static QRegExp rx(
		"\\[\"([a-fA-F0-9]+)\"\\s*,"	// replyId 
		"\\s*([0-9]+)\\s*," 		// isNew
		"\\s*([0-9]+)\\s*,"		// unknown1
		"\\s*\"([^\"]*)\"\\s*,"		// date
		"\\s*\"([^\"]*)\",\\s*"		// senders
		"\\s*\"([^\"]*)\"\\s*,"		// chevron
		"\\s*\"([^\"]*)\"\\s*,"		// subject
		"\\s*\"([^\"]*)\"\\s*,"		// snippet
		"\\s*\\[([^\\]]*)\\]\\s*,"	// labels
		"\\s*\"([^\"]*)\"\\s*,"		// attachments
		"\\s*\"([a-fA-F0-9]+)\"\\s*,"	// msgId
		"\\s*([0-9]+)\\s*,"              // unknown3
		"\\s*\"([^\"]*)\"\\s*\\]"		// unknown3
		);

	QString data = _data;
	data.replace(killEscapes, "");

	int pos = 0;

	rx.setMinimal(true);

	if(!rx.isValid()) {
		kdDebug() << k_funcinfo << "Invalid RX!\n"
			<< rx.errorString() << endl;
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
		t->date = rx.cap(4);
		t->senders = rx.cap(5);
		t->chevron = rx.cap(6);
		t->subject = rx.cap(7);
		t->snippet = rx.cap(8);
		t->labels = rx.cap(9);
		t->attachments = rx.cap(10);
		t->msgId = rx.cap(11);
		t->unknown3 = rx.cap(12).toUInt();
		t->isNull = false;

		// truly a new msg? inc counter
		kdDebug() << k_funcinfo << "truly a new msg? inc counter" << endl;
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

	kdDebug() << k_funcinfo << "Finished searching for threads in: " << endl;
	kdDebug() << data << endl;

	if(newMsgCount > 0)
		emit mailArrived(newMsgCount);
}

void GMailParser::parseVersion(const QString &data)
{
	mVersion = data;
	mVersion.remove('"');

	kdDebug() << k_funcinfo << "Version=" << mVersion << endl;

	if(mVersion != gGMailVersion)
		emit versionMismatch();
}

void GMailParser::parseInvite(const QString &data)
{
	bool ok = true;
	mInvites = data.toUInt(&ok);
	if(!ok)
		mInvites = 0;
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

unsigned int GMailParser::getNewCount() const
{
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
