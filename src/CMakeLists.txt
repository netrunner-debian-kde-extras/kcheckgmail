add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kcheckgmail_SRCS
    knotification.cpp
    configdialog.cpp
    gmail.cpp
    gmailparser.cpp
    gmailwalletmanager.cpp
    jsprotocol.cpp
    kcheckgmailapp.cpp
    kcheckgmailcore.cpp
    kcheckgmailtray.cpp
    mailcounter.cpp
    main.cpp)

kde4_add_ui_files(kcheckgmail_SRCS
    advancedsettingswidget.ui
    appearancesettingswidget.ui
    appletsettingswidget.ui
    loginsettingswidget.ui
    netsettingswidget.ui)

message(STATUS "DCOP has been removed in KDE 4; port code to D-Bus. kde4_add_dcop_skels macro does not exist anymore. We keep it just to remember to port to D-Bus.")
#kde4_add_dcop_skels(kcheckgmail_SRCS kcheckgmailiface.h )

kde4_add_kcfg_files(kcheckgmail_SRCS prefs.kcfgc)

kde4_add_executable(kcheckgmail ${kcheckgmail_SRCS})

target_link_libraries(kcheckgmail ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kcheckgmail ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES  kcheckgmail.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES  eventsrc DESTINATION ${DATA_INSTALL_DIR}/kcheckgmail)



#original Makefile.am contents follow:

#INCLUDES = $(all_includes)
#
#bin_PROGRAMS = kcheckgmail
#
#kcheckgmail_SOURCES = advancedsettingswidget.ui appearancesettingswidget.ui \
#	appletsettingswidget.ui configdialog.cpp gmail.cpp gmailparser.cpp gmailwalletmanager.cpp \
#	jsprotocol.cpp kcheckgmailapp.cpp kcheckgmailcore.cpp kcheckgmailiface.skel \
#	kcheckgmailtray.cpp loginsettingswidget.ui mailcounter.cpp main.cpp netsettingswidget.ui \
#	prefs.kcfgc
#kcheckgmail_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
#kcheckgmail_LDADD = $(LIB_KDEUI) $(LIB_KIO) -lkwalletclient
#
#METASOURCES = AUTO
#
#noinst_HEADERS = advancedsettingswidget.h appletsettingswidget.h configdialog.h \
#	gmail.h gmail_constants.h gmailparser.h gmailwalletmanager.h jsprotocol.h \
#	kcheckgmailapp.h kcheckgmailcore.h kcheckgmailcore_p.h kcheckgmailiface.h \
#	kcheckgmailtray.h loginsettingswidget.h mailcounter.h netsettingswidget.h prefs.h
#
#xdg_apps_DATA = kcheckgmail.desktop
#
#srcdoc:
#	kdoc -a -p -H -d $$HOME/web/src/kcheckgmail kcheckgmail *.h -lqt -lkdecore -lkdeui -lkfile
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/kcheckgmail.pot
#
#rcdir = $(kde_datadir)/kcheckgmail
#rc_DATA = eventsrc
#
#
#KDE_OPTIONS=nofinal
