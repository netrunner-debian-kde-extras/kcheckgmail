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
#ifndef GMAIL_PARSER_H
#define GMAIL_PARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <qmap.h>
#include <QVector>
//#include <vector>

/**
@author Matthew Wlazlo
*/
class GMailParser : public QObject
{
	Q_OBJECT
public:
	// "qu"
	typedef struct {
		QString used;
		QString total;
		QString percent;
		QString colour;
	} Quota;

	// "ds"
	typedef struct {
		unsigned int inbox;
// 		unsigned int starred;
		unsigned int drafts;
// 		unsigned int sent;
// 		unsigned int all;
		unsigned int spam;
// 		unsigned int trash;
	} DefaultSearchSummary;	

	// "t"
	typedef struct {
		int id;
		QString replyId;
		bool isNew;
		bool isStarred;
		QString date_short;
		QString senders;
		QString chevron;
		QString subject;
		QString snippet;
		QString labels;
		QStringList attachments;
		QString msgId;
		unsigned int unknown2;
		QString date_long;
		unsigned int unknown3;
		QString unknown4;
		unsigned int unknown5;
		bool isNull; 
	} Thread;
	
	// "v"
	typedef struct {
		QString unknown1;
		QString language;
		unsigned int unknown2;
		unsigned int unknown3;
		QString version;
	} Version;
	
	// "ts"
	/*typedef struct {
		int fromPos;
		int toPos;
		int showing;
		int pages; //?
		QString humanQuery;
		QString query;
		QString searchId;
		int unknown1;
		QString unknown2; // sometimes empty, sometimes filled
		QString dottedQuery; // ?
		QString unknown3;
	} ThreadSummary;*/
public:
	enum CountMode {
		TotalCount,
		ParsedOnlyCount
	};
		
	GMailParser(QObject* parent = 0);
	virtual ~GMailParser();

	void parse(const QString &data);
	

	unsigned int unread(CountMode mode ) const;
	unsigned int unread(CountMode mode, QString box) const;

// 	const QString &getVersion() const { return mVersion; }
	unsigned int getInvites() const { return mInvites; }
	const QString getGaiaName() const;

	const DefaultSearchSummary &getSummary() const { return mSummary; }

	const Quota& getQuota() const { return mQuota; }
	const QMap<QString, unsigned int> getLabels() const { return mLabels; }

	// key = msgId, bool = isNew
	QMap<QString,bool> *getThreadList() const;
	const Thread &getThread(const QString &msgId) const;
	const Thread &getThread(int id) const;
	const Thread &getLastArrivedThread() const;
	
	static QString stripTags(QString data);
	static QString convertEntities(QString data);
	static QString cleanUpData(QString data);

signals:
	void versionMismatch();
	void gNameUpdate(QString name);
	void countUpdate(unsigned int arrivedMails);

protected:
	void parseQuota(const QString&);
	void parseDefaultSummary(const QString&);
	void parseLabel(const QString&);
	uint parseThread(const QString&, const QMap<QString,bool>*);
	void parseVersion(const QString&);
	void parseInvite(const QString&);
	void parseGName(const QString&);
	void freeThreadList();

private:
	Version mVersion;
	unsigned int mInvites;
	unsigned int mCurMsgId;
	Quota mQuota;
	DefaultSearchSummary mSummary;
	QMap<QString, unsigned int> mLabels; //<name, count>
	QMap<QString, QString> eLabels; // <escaped name, name>
	QMap<QString, Thread*> mThreads;
	QVector<QString> gGMailVersion;
#ifdef DETECT_GLANGUAGE
	QMap<QString, QString> gGMailLanguageCode;
#endif
	QString gName;
	QString previousLatestThread;
};
#endif
