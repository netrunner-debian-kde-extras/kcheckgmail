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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

// define this symbol if you want to try to detect the language of your account
#undef DETECT_GLANGUAGE

#include "gmailparser.h"
#include "gmail_constants.h"
#include "prefs.h"

#include <kdebug.h>
#include <QRegExp>
//Added by qt3to4:
#include <QList>
#include <klocale.h>
#include <kcharsets.h>

/**
 * Gmail's response parser object constructor.
 *
 * This class parses the resulting data of a call to
 * Gmail's JavaScript interface.
*/
GMailParser::GMailParser(QObject* parent) :
		QObject(parent),
		mInvites(0)
{
	mSummary.inbox = 0;
// 	mSummary.starred = 0;
	mSummary.drafts = 0;
// 	mSummary.sent = 0;
// 	mSummary.all = 0;
	mSummary.spam = 0;
// 	mSummary.trash = 0;
	
	previousLatestThread = "0";
	
	//Gmail versions kcheckgmail works with.
	gGMailVersion.append("7sck6ul8cinq");
	gGMailVersion.append("zu7a2n462w17"); // new Gmail version, ui=1
	gGMailVersion.append("1exl39kx7mipo");
	gGMailVersion.append("1x4nkpwjfkc8x");
	gGMailVersion.append("1ddh9n6glzd1c");
	gGMailVersion.append("11qm1wldxu1ww");
	gGMailVersion.append("5vnvxev1uvtq");

	// TODO: read this from a file
#ifdef DETECT_GLANGUAGE
	// Gmail language identifiers:
	gGMailLanguageCode.insert("7fba835ed0312d54",i18n("Spanish"));
	gGMailLanguageCode.insert("7530096a84569c0b",i18n("French"));
	gGMailLanguageCode.insert("21208aa200ae6920",i18n("Italian"));
	gGMailLanguageCode.insert("cd21242a38a63f0",i18n("German"));
	gGMailLanguageCode.insert("9930dc54804b344a",i18n("English (US)"));
	gGMailLanguageCode.insert("f59adb920ee42615",i18n("English (UK)"));
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
	gGMailLanguageCode.insert("88595fc43e710562",i18n("Chinese (Simplified)"));
	gGMailLanguageCode.insert("61a2f8a31f62e658",i18n("Chinese (Traditional)"));
	gGMailLanguageCode.insert("c368aa1b815d1a8a",i18n("Czech"));
	gGMailLanguageCode.insert("a680d09b1f097e52",i18n("Croatian"));
	gGMailLanguageCode.insert("e35d4c2af8d5feba",i18n("Catalan"));
	gGMailLanguageCode.insert("b8e15ea37ed4f16",i18n("Arabic"));
	gGMailLanguageCode.insert("1a0f6d9fde5216d",i18n("Indonesian"));
	gGMailLanguageCode.insert("eda8b40a2ecad8c4",i18n("Japanese"));
	gGMailLanguageCode.insert("4e86fd3e11e3c97f",i18n("Korean"));
	gGMailLanguageCode.insert("22aa46bbafe71b1c",i18n("Hindi"));
	gGMailLanguageCode.insert("41525b3ab51fbde0",i18n("Thai"));
	gGMailLanguageCode.insert("2a874ffaa00d8aef",i18n("Danish"));
	gGMailLanguageCode.insert("f7b8471d482333cb",i18n("Estonian"));
	gGMailLanguageCode.insert("2b1540853e61b819",i18n("Icelandic"));
	gGMailLanguageCode.insert("b8245e71d2838794",i18n("Latvian"));
	gGMailLanguageCode.insert("68b96b309bad80f3",i18n("Romanian"));
	gGMailLanguageCode.insert("5597c0ee7cb73b7f",i18n("Slovak"));
	gGMailLanguageCode.insert("54b802d9a90ad926",i18n("Slovenian"));
	gGMailLanguageCode.insert("2afe8d7fe1757459",i18n("Finnish"));
	gGMailLanguageCode.insert("2381ac21a233a92c",i18n("Tagalog"));
	gGMailLanguageCode.insert("5592d8feddc3e8da",i18n("Vietnamese"));
	gGMailLanguageCode.insert("ba76ef5e9ef44145",i18n("Ukrainian"));
	gGMailLanguageCode.insert("ffa2983afaf20325",i18n("Bulgarian"));
#endif
}

/**
 * Object destructor.
*/
GMailParser::~GMailParser()
{
}

/**
 * Main parser.
 *
 * The main parsing process starts here.
 * It splits the content of _data into blocks
 * which are later passed to sub-parsers.
 *
 * @param _data Gmail's JavaScript response
*/
void GMailParser::parse(const QString &data)
{
	static QRegExp rx("D\\(\\[(.*)\\][\\s\\n]*\\);");
	int pos = 0;

	rx.setMinimal(true);

	if(!rx.isValid()) {
		kWarning() << "Invalid RX!\n"
			<< rx.errorString();
	} 

	mCurMsgId = 0;

	QMap<QString,bool> *oldMap = getThreadList();
	freeThreadList();
	
	if(oldMap) {
		kDebug() << "oldmap.size=" << oldMap->size();
		if (oldMap->begin().key() > previousLatestThread) {
			previousLatestThread = oldMap->begin().key();
		}
	} else {
		kDebug() << "no oldmap";
	}
	
	kDebug() << "previousLatestThread=" << previousLatestThread;


	/*
	 * mailsArrived refers to new messages in the parsed threads
	 */
	unsigned int arrivedMails = 0;
	while((pos = rx.indexIn(data, pos)) != -1) {
		QString str = rx.cap(1);
		QRegExp rxType("^\"([a-z]+)\",");

		int tokPos = -1;
		if((tokPos = rxType.indexIn(str)) >= 0) {
			QString tok = rxType.cap(1);
			int tokLen = rxType.matchedLength();

			// strip token
			str.remove(tokPos, tokLen);
			
			if(tok == D_THREAD) {
				arrivedMails += parseThread(str, oldMap);
			} else if(tok == D_VERSION) {
				parseVersion(str);
			} else if(tok == D_QUOTA) {
				parseQuota(str);
			} else if(tok == D_DEFAULT_SUMMARY) {
				parseDefaultSummary(str);
			} else if(tok == D_CATEGORIES) {
				parseLabel(str);
			} else if(tok == D_INVITE_STATUS) {
				parseInvite(str);
			} else if(tok == D_GAIA_NAME) {
				parseGName(str);
			}/* else if(tok == D_THREADLIST_SUMMARY) {
				parseThreadSummary(str);
			}*///D(["ts",0,20/*max shown*/,4/*total results*/,0,"Search results for: in:inbox is:unread","in:inbox is:unread","113e12c0cc4"/*search id?*/,9,,"",""]

		}

		pos += rx.matchedLength();
	}

	delete oldMap;
	oldMap = 0;

	emit countUpdate(arrivedMails);
}

///////////////////////////////////////////////////////////////////////////
// Parsers
///////////////////////////////////////////////////////////////////////////


/**
 * Threads/emails parser.
 *
 * This parser takes care of extracting the available data from the emails block.
 *
 * @param _data The messages data block
 * @param oldMap The old messages map, used to detect whether a message was already reported as new or not
 * @return The number of unread messages that were found in _data
*/
uint GMailParser::parseThread(const QString &_data, const QMap<QString,bool>* oldMap)
{
	//Matches messages when snippets are on
	static QRegExp rx(
			"\\[\"([a-fA-F0-9]+)\"\\s*,"	// replyID
			"\\s*([0-9]+)\\s*,"		// isNew
			"\\s*([0-9]+)\\s*,"		// isStarred
			"\\s*\"([^\"]*)\"\\s*,"		// date_short
			"\\s*\"([^\"]*)\"\\s*,"		// senders
			"\\s*\"([^\"]*)\"\\s*,"		// chevron
			"\\s*\"([^\"]*)\"\\s*,"		// subject
			"\\s*\"([^\"]*)\"\\s*,"		// snippet
			"\\s*\\[((?:\\s*\"[^\"]+\")?(?:,\\s*\"[^\"]+\")*)\\]\\s*,"	// labels
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
			"\\s*([0-9]+)\\s*,"		// isStarred
			"\\s*\"([^\"]*)\"\\s*,"		// date_short
			"\\s*\"([^\"]*)\"\\s*,"		// senders
			"\\s*\"([^\"]*)\"\\s*,"		// chevron
			"\\s*\"([^\"]*)\"\\s*,"		// subject
			"(\\s*),"			// snippet
			"\\s*\\[((?:\\s*\"[^\"]+\")?(?:,\\s*\"[^\"]+\")*)\\]\\s*,"	// labels
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
		kWarning() << "Invalid RX!\n"
				<< rx.errorString();
	}

	if(!rx2.isValid()) {
		kWarning() << "Invalid RX2!\n"
				<< rx2.errorString();
	}
	
	/*
	replyId == msgId if latest message on this 
	thread is not from you
	*/

	unsigned int newMsgCount = 0;

	while((pos = rx.indexIn(data, pos)) != -1) {
		Thread *t = new Thread;
		t->id = mCurMsgId ++;
		t->replyId = rx.cap(1);
		t->isNew = rx.cap(2).toInt();
		t->isStarred = rx.cap(3).toInt();
		t->date_short = rx.cap(4);
		t->senders = cleanUpData(rx.cap(5));
		t->chevron = rx.cap(6);
		t->subject = cleanUpData(rx.cap(7));
		t->snippet = cleanUpData(rx.cap(8));
		t->labels = rx.cap(9);
		t->attachments = rx.cap(10).split(",", QString::SkipEmptyParts);
		t->msgId = rx.cap(11);
		t->unknown2 = rx.cap(12).toUInt();
		t->date_long = rx.cap(13);
		t->unknown3 = rx.cap(14).toUInt();
		t->isNull = false;

		if(t->isNew && (t->msgId > previousLatestThread || t->replyId > previousLatestThread) && (!oldMap || 
				 (oldMap->find(t->msgId) == oldMap->end()))) {
			kDebug() << "Message [" << t->msgId << "] is new.";
			t->isNew = true;
			newMsgCount ++;
		} else {
			t->isNew = false;
			kDebug() << "Message [" << t->msgId << "] is NOT new.";
		}

		// (re-)insert
		mThreads.insert(t->msgId, t);

		pos += rx.matchedLength();
	}

	pos = 0;
	
	while((pos = rx2.indexIn(data, pos)) != -1) {
		Thread *t = new Thread;
		t->id = mCurMsgId ++;
		t->replyId = rx2.cap(1);
		t->isNew = rx2.cap(2).toInt();
		t->isStarred = rx2.cap(3).toInt();
		t->date_short = rx2.cap(4);
		t->senders = cleanUpData(rx2.cap(5));
		t->chevron = rx2.cap(6);
		t->subject = cleanUpData(rx2.cap(7));
		t->snippet = cleanUpData(rx2.cap(8));
		t->labels = rx2.cap(9);
		t->attachments = rx.cap(10).split(",", QString::SkipEmptyParts);
		t->msgId = rx2.cap(11);
		t->unknown2 = rx2.cap(12).toUInt();
		t->date_long = rx2.cap(13);
		t->unknown3 = rx2.cap(14).toUInt();
		t->isNull = false;

		if(t->isNew && (t->msgId > previousLatestThread || t->replyId > previousLatestThread) && (!oldMap || 
				 (oldMap->find(t->msgId) == oldMap->end()))) {
			kDebug() << "Message [" << t->msgId << "] is new.";
			t->isNew = true;
			newMsgCount ++;
		} else {
			kDebug() << "Message [" << t->msgId << "] is NOT new.";
			t->isNew = false;
		}

		// (re-)insert
		mThreads.insert(t->msgId, t);

		pos += rx2.matchedLength();
	}

	kDebug() << "Finished searching for threads in: ";
	kDebug() << data;
	kDebug() << "newMsgCount: " << newMsgCount;

	return newMsgCount;
}

/**
 * Gmail version information parser.
 *
 * This parser extracts some information from the version string.
 *
 * @param _data The data block
*/
void GMailParser::parseVersion(const QString &_data)
{
	QString data = _data;
	data.remove('"');
	
	kDebug() << "Version string: " << data;
	
	QStringList list = data.split(",", QString::SkipEmptyParts);
	if(list.size() != 5)
		kWarning() << "Wrong number of elements: "
				<< list.size() << ", should be: 5.";
	
	QStringList::Iterator iter = list.begin();
	int i = 0;
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
				kWarning() << "Unknown version token: " << str << "(" << i <<")";
				break;
		}
		iter++;
		i++;
	}
	kDebug() << "Gmail version " << mVersion.version;
	
	bool ok = false;
	
	for( i = 0; i < gGMailVersion.size() ; i++ ) {
		if( gGMailVersion[i] == mVersion.version )
			ok = true;
	}
	
#ifdef DETECT_GLANGUAGE
	if(gGMailLanguageCode.contains(mVersion.language))
		kDebug() << "Gmail language: " << gGMailLanguageCode[mVersion.language];
	else
		kWarning() << "Unknown language code: " << mVersion.language;
#endif
	
	if(!ok) {
		kWarning() << "Gmail version " << mVersion.version << " is not supported, check for updates!";
		emit versionMismatch();
	}
}

/**
 * Quota information parser.
 *
 * This parser extracts quota information like 
 * the amount of space used, available, the used percentage, etc.
 *
 * @param data The data block
*/
void GMailParser::parseQuota(const QString &data)
{	
	QStringList list = data.split(",", QString::SkipEmptyParts);
	if(list.size() == 4 || list.size() == 9) {
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
		kWarning() << "Wrong number of elements in qu: "
			<< list.size() << ", should be 4 or 9.";
}

/**
 * Default summary parser.
 *
 * This parser extracts the number of unread messages in the inbox, drafts and spam.
 *
 * @param _data The data block
*/
void GMailParser::parseDefaultSummary(const QString &_data)
{
	static QRegExp rx("\"([a-z]+)\",([0-9]+)");

	if(!rx.isValid()) {
		kWarning() << "Invalid RX!\n"
			<< rx.errorString();
	}
	QString data = _data;
	int pos = 0;

	while((pos = rx.indexIn(data, pos)) != -1) {
		QString str_name = rx.cap(1), str_val = rx.cap(2);
		int val = str_val.toUInt();

		if( QString::compare(str_name,"inbox") == 0)
			mSummary.inbox = val;
		else if( QString::compare(str_name,"drafts") == 0)
			mSummary.drafts = val;
		else if( QString::compare(str_name,"spam") == 0)
			mSummary.spam = val;
		else kWarning() << "unknown identifier " << str_name;

		pos += rx.matchedLength();
	}
	kDebug() << endl
		 << "inbox=" << mSummary.inbox << endl
		 << "drafts=" << mSummary.drafts << endl
		 << "spam=" << mSummary.spam;
}

/**
 * Lables parser.
 *
 * This parser extracts the number of unread messages per label.
 *
 * @param data The data block
 * @todo Store a QMap with the labels information
*/
void GMailParser::parseLabel(const QString &data)
{
	static QRegExp rx(
		"\\[\"([^\"]+)\""	// label name
		",([0-9]+)\\]"		// unread count
		);

	if(!rx.isValid()) {
		kWarning() << "Invalid RX!\n"
			<< rx.errorString();
	}
	int pos = 0;
	
	mLabels.clear();
	eLabels.clear();
	
	kDebug();

	while((pos = rx.indexIn(data, pos)) != -1) {
		mLabels.insert(rx.cap(1), rx.cap(2).toUInt());
		
		QString k = rx.cap(1);
		k.replace(" ", "-");
		eLabels.insert(k, rx.cap(1));
		
		kDebug() << rx.cap(1) << " has " << rx.cap(2) << " unread messages";
		pos += rx.matchedLength();
	}
}

/**
 * Invites information parser.
 *
 * This parser extracts the number of available invites.
 *
 * @param data The data block
*/
void GMailParser::parseInvite(const QString &data)
{
	bool ok = true;
	mInvites = data.toUInt(&ok);
	if(!ok) {
		mInvites = 0;
	}
	kDebug() << "Invites=" << mInvites;
}

/**
 * Gaia Name parser.
 *
 * This parser extracts the account's owner name (a.k.a. Gaia Name)
 *
 * @param data The data block
*/
void GMailParser::parseGName(const QString &data)
{
	QString newName = data;
	
	newName.remove('"');
	
	if(newName != gName) {
		gName = newName;
		kDebug() << "Gaia name: " << gName;
		emit gNameUpdate(gName);
	}
}

///////////////////////////////////////////////////////////////////////////
// Data accessors
///////////////////////////////////////////////////////////////////////////

/**
 * Return the list of parsed messages together with their isNew value
 *
 * @return A list with the msgId's as the keys and isNew as the value
 */
QMap<QString, bool> *GMailParser::getThreadList() const
{
	QMap<QString, bool> *ret = 0;

	if(!mThreads.isEmpty()) {
		ret = new QMap<QString, bool>();

		QList<QString> klist = mThreads.keys();
		QList<QString>::iterator iter = klist.begin();

		while(iter != klist.end()) {
			Thread *t = mThreads[*iter];
			ret->insert(t->msgId, t->isNew);
			iter ++;
		}
	}

	return ret;
}

/**
 * Return the thread information of the thread specified by msgId
 *
 * @param msgId The message Id of the thread
 * @return A copy of the Thread
*/
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

/**
 * Return the thread information of the thread specified by id.
 *
 * @param id The numerical id of the thread
 * @return A copy of the Thread
*/
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

/**
 * Return the thread information of last arrived thread in the map
 *
 * @return A copy of the Thread
 */
const GMailParser::Thread& GMailParser::getLastArrivedThread() const
{
	static Thread nullThread;

	QMap<QString, Thread*>::const_iterator iter = mThreads.constEnd();
	QMap<QString, Thread*>::const_iterator begin = mThreads.constBegin();

	while (iter != begin) {
		--iter;
		if ((*iter)->isNew)
			return *(*iter);
	}

	nullThread.isNull = true;
	return nullThread;
}

const QString GMailParser::getGaiaName() const
{	
	return gName;
}

/**
 * Retrieve the number of unread messages.
 *
 * If mode is ParsedOnlyCount the box parameter is ignored.
 *
 * @param mode If the number of unread messages should be taken from the totals or only from the parsed messages
 * @param box The name of the box (inbox, drafts, spam; or in the future: label) from where the real number of unread messages should be taken from
 * @return The number of unread messages
 * @example unread(GMailParser::TotalCount,"inbox") Get the real number of unread messages in the inbox
*/
unsigned int GMailParser::unread(CountMode mode, QString box) const
{
	unsigned int ret = 0;
	
	if(mode == TotalCount) {
		if(box.compare("inbox") == 0)
			return mSummary.inbox;
		else if(box.compare("drafts") == 0)
			return mSummary.drafts;
		else if(box.compare("spam") == 0)
			return mSummary.spam;
		else {
			if (mLabels.contains(box))
				return mLabels[box];
		}
		kWarning() << "The box " << box << " doesn't exist! returning value as if mode=ParsedOnlyCount";
	}
	
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
	
	delete lst;
	lst = 0;

	return ret;
}

/**
 * Retrieve the number of unread messages.
 *
 * @param mode If the number of unread messages should be taken from the totals or only from the parsed messages
 * @return The number of unread messages
*/
unsigned int GMailParser::unread(CountMode mode) const
{
	static QRegExp rx ("in:([^ ]+)");
	static QRegExp rx2("label:([^ ]+)");
	QString box;
	
	int pos;
	if (mode == TotalCount) {
		if (rx.indexIn(Prefs::searchFor()) == -1 && rx2.indexIn(Prefs::searchFor()) == -1) {
			// If none are specified gmail will return any unread mail (except spam and drafts)
			// TODO: to fix this we need to count all messages (!drafts,!spam, inbox + labels)
			mode = ParsedOnlyCount;
			//box = "inbox";
		} else if (rx.indexIn(Prefs::searchFor()) != -1 && rx2.indexIn(Prefs::searchFor()) != -1) {
			//there's no other way to know how many emails are in:inbox and in specified label:LABEL
			mode = ParsedOnlyCount;
		} else if ((pos = rx.indexIn(Prefs::searchFor())) != -1) {
			box = rx.cap(1);

			pos += rx.matchedLength();
			// make sure there is only one in: in the search string
			if (rx.indexIn(Prefs::searchFor(), pos) != -1) {
				mode = ParsedOnlyCount;
			}
		} else if ((pos = rx2.indexIn(Prefs::searchFor())) != -1) {
			box = rx2.cap(1);

			pos += rx2.matchedLength();
			// make sure there is only one label: in the search string
			if (rx2.indexIn(Prefs::searchFor(), pos) != -1) {
				mode = ParsedOnlyCount;
			} else if (eLabels.contains(box)) {
				box = eLabels[box];
			}
		}
	}
	
	return unread(mode, box);
}


///////////////////////////////////////////////////////////////////////////
// Clean up functions
///////////////////////////////////////////////////////////////////////////

void GMailParser::freeThreadList()
{
	if(!mThreads.isEmpty()) {

		QList<QString> klist = mThreads.keys();
		QList<QString>::iterator iter = klist.begin();

		while(iter != klist.end()) {
			Thread *t = mThreads[*iter];
			delete t;
			t = 0;
			iter ++;
		}
	}

	mThreads.clear();
}

/**
 * Tags stripper.
 *
 * This function removes all tags from data.
 *
 * @param data The data to be processed
 * @return The content of data without tags
*/
QString GMailParser::stripTags(QString data)
{
	static QRegExp tags("<[^>]+>|</[^>]+>|<[^>]+/>");
	
	if(!tags.isValid()) {
		kWarning() << "Invalid RX!\n"
				<< tags.errorString();
	}
	
	data.remove(tags);
	
	return data;
}

/**
 * JavaScript entities converter.
 *
 * This function converts all \uXXXX to their right representation.
 *
 * @param data The data to be processed
 * @return The content of data with the converted entities
*/
QString GMailParser::convertEntities(QString data)
{
	QChar c;
	QString found, id;
	static QRegExp format("\\\\((u)([0-9a-zA-Z]{4})|(x)([0-9a-zA-Z]{2}))");
	
	if(!format.isValid()) {
		kWarning() << "Invalid RX!\n"
				<< format.errorString();
	}
	
	while(format.indexIn(data) != -1) {
		id = format.cap(2);
		found = format.cap(3);
		if (found.length() == 0) {
			id = format.cap(4);
			found = format.cap(5);
		}
		c = QChar(found.toUInt(0,16));
		data.replace("\\"+id+found,c);
	}
	return data;
}

/**
 * All-in-one data cleaner.
 *
 * This function passes the data to 
 * convertEntities, stripTags and KCharsets::resolveEntities
 *
 * @see convertEntities
 * @see stripTags
 * @param data The data to be cleaned up
 * @return The cleaned up data
*/
QString GMailParser::cleanUpData(QString data)
{
	data = convertEntities(data);
	data = stripTags(data);
	data = KCharsets::resolveEntities(data);
	return data;
}

#include "gmailparser.moc"
