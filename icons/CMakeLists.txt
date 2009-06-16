
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install(FILES  DESTINATION ${DATA_INSTALL_DIR}/kcheckgmail/icons)

kde4_install_icons(${ICON_INSTALL_DIR})
