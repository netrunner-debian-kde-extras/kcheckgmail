project(kcheckgmail)

# Check for cmake minimum for KDE 4.0.0
cmake_minimum_required(VERSION 2.4.5)

set(KCHECKGMAIL_VERSION "0.5.8-dev")

find_package(KDE4 REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include(KDE4Defaults)

include(MacroLibrary)

include(ConfigureChecks.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(TOPSUBDIRS)")

add_subdirectory( src )
add_subdirectory( icons )
add_subdirectory( po )
add_subdirectory( doc )

########### install files ###############

kde4_create_manpage(man-kcheckgmail.1.docbook 1 INSTALL_DESTINATION ${MAN_INSTALL_DIR})
