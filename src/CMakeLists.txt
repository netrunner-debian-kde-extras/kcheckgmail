include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kcheckgmail_SRCS
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
    advancedsettingsbase.ui
    appearancesettingsbase.ui
    appletsettingsbase.ui
    loginsettingsbase.ui
    netsettingsbase.ui)

qt4_add_dbus_adaptor(kcheckgmail_SRCS org.kcheckgmail.kcheckgmail.xml kcheckgmailcore.h KCheckGmailCore)

kde4_add_kcfg_files(kcheckgmail_SRCS prefs.kcfgc)

kde4_add_executable(kcheckgmail ${kcheckgmail_SRCS})

target_link_libraries(kcheckgmail ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} )

install(TARGETS kcheckgmail ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES  kcheckgmail.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES  kcheckgmail.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kcheckgmail )
install(FILES  org.kcheckgmail.kcheckgmail.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
