INCLUDES = $(all_includes)

bin_PROGRAMS = kcheckgmail

kcheckgmail_SOURCES = main.cpp \
	kcheckgmailapp.cpp \
	kcheckgmailtray.cpp \
	gmail.cpp \
	prefs.kcfgc \
	netsettingswidget.ui \
	loginsettingswidget.ui \
	appletsettingswidget.ui
kcheckgmail_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
kcheckgmail_LDADD = $(LIB_KDEUI) $(LIB_KIO)

METASOURCES = AUTO

noinst_HEADERS = gmail.h kcheckgmailapp.h kcheckgmailtray.h loginsettingswidget.h \
	netsettingswidget.h  prefs.h appletsettingswidget.h

xdg_apps_DATA = kcheckgmail.desktop

srcdoc:
	kdoc -a -p -H -d $$HOME/web/src/kcheckgmail kcheckgmail *.h -lqt -lkdecore -lkdeui -lkfile

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kcheckgmail.pot

rcdir = $(kde_datadir)/kcheckgmail
rc_DATA = eventsrc


KDE_OPTIONS=nofinal
