INCLUDES = $(all_includes)

bin_PROGRAMS = kcheckgmail

kcheckgmail_SOURCES = advancedsettingswidget.ui appletsettingswidget.ui \
	gmail.cpp gmailparser.cpp gmailwalletmanager.cpp kcheckgmailapp.cpp \
	kcheckgmailiface.skel kcheckgmailtray.cpp loginsettingswidget.ui main.cpp \
	netsettingswidget.ui prefs.kcfgc
kcheckgmail_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
kcheckgmail_LDADD = $(LIB_KDEUI) $(LIB_KIO) -lkwalletclient

METASOURCES = AUTO

noinst_HEADERS = gmail.h \
	gmailparser.h \
	kcheckgmailapp.h \
	kcheckgmailtray.h \
	kcheckgmailiface.h \
	gmailwalletmanager.h \
	loginsettingswidget.h \
	netsettingswidget.h \
	prefs.h \
	gmail_constants.h \
	appletsettingswidget.h \
	advancedsettingswidget.h

xdg_apps_DATA = kcheckgmail.desktop

srcdoc:
	kdoc -a -p -H -d $$HOME/web/src/kcheckgmail kcheckgmail *.h -lqt -lkdecore -lkdeui -lkfile

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kcheckgmail.pot

rcdir = $(kde_datadir)/kcheckgmail
rc_DATA = eventsrc


KDE_OPTIONS=nofinal
