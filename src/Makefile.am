INCLUDES = $(all_includes)

bin_PROGRAMS = kcheckgmail

kcheckgmail_SOURCES = advancedsettingswidget.ui appearancesettingswidget.ui \
	appletsettingswidget.ui configdialog.cpp gmail.cpp gmailparser.cpp gmailwalletmanager.cpp \
	knotification.cpp \
	jsprotocol.cpp kcheckgmailapp.cpp kcheckgmailcore.cpp kcheckgmailiface.skel \
	kcheckgmailtray.cpp loginsettingswidget.ui mailcounter.cpp main.cpp netsettingswidget.ui \
	prefs.kcfgc
kcheckgmail_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
kcheckgmail_LDADD = $(LIB_KDEUI) $(LIB_KIO) -lkwalletclient

METASOURCES = AUTO

noinst_HEADERS = advancedsettingswidget.h appletsettingswidget.h configdialog.h \
	gmail.h gmail_constants.h gmailparser.h gmailwalletmanager.h jsprotocol.h \
	knotification.h \
	kcheckgmailapp.h kcheckgmailcore.h kcheckgmailcore_p.h kcheckgmailiface.h \
	kcheckgmailtray.h loginsettingswidget.h mailcounter.h netsettingswidget.h prefs.h

xdg_apps_DATA = kcheckgmail.desktop

srcdoc:
	kdoc -a -p -H -d $$HOME/web/src/kcheckgmail kcheckgmail *.h -lqt -lkdecore -lkdeui -lkfile

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kcheckgmail.pot

rcdir = $(kde_datadir)/kcheckgmail
rc_DATA = eventsrc


KDE_OPTIONS=nofinal
