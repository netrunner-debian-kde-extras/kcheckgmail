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
#ifndef GMAIL_PARSER_H
#define GMAIL_PARSER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <vector>

// version of Gmail KCheckGmail works with
static const QString gGMailVersion = "509dedf8dd775d9b";

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
		unsigned int starred;
		unsigned int sent;
		unsigned int all;
		unsigned int spam;
		unsigned int trash;
	} DefaultSearchSummary;
	
	// "ct" : array
	typedef struct {
		QString name;
		unsigned int count;
	} Label;

	// "t"
	typedef struct {
		int id;
		QString replyId;
		bool isNew;
		unsigned int unknown1;
		QString date;
		QString senders;
		QString chevron;
		QString subject;
		QString snippet;
		QString labels;
		QString attachments;
		QString msgId;
		unsigned int unknown3;
		bool isNull; 
	} Thread;
public:
	GMailParser();
	virtual ~GMailParser();

	void parse(const QString &data);
	
	unsigned int getNewCount() const;

	const QString &getVersion() const { return mVersion; }
	unsigned int getInvites() const { return mInvites; }

	const DefaultSearchSummary &getSummary() const { return mSummary; }

	const Quota& getQuota() const { return mQuota; }
	const std::vector<Label>& getLabel() const { return mLabels; }

	// key = msgId, bool = isNew
	QMap<QString,bool> *getThreadList() const;
	const Thread &getThread(const QString &msgId) const;
	const Thread &getThread(int id) const;

signals:
	void mailArrived(unsigned int count);
	void mailCountChanged();
	void versionMismatch();

protected:
	void parseQuota(const QString&);
	void parseDefaultSummary(const QString&);
	void parseLabel(const QString&);
	void parseThread(const QString&);
	void parseVersion(const QString&);
	void parseInvite(const QString&);
	void freeThreadList();

private:
	QString mVersion;
	unsigned int mInvites;
	Quota mQuota;
	DefaultSearchSummary mSummary;
	std::vector<Label> mLabels;
	QMap<QString,Thread*> mThreads;
};
#endif
