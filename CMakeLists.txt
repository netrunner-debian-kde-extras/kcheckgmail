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


#original Makefile.am contents follow:

#SUBDIRS = $(TOPSUBDIRS)
#
#dist_man_MANS = kcheckgmail.1
#man_MANS = kcheckgmail.1
#
#$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
#	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in ;
#
#$(top_srcdir)/subdirs:
#	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs
#
#$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
#	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4
#
#MAINTAINERCLEANFILES = subdirs configure.in acinclude.m4 configure.files 
#
#package-messages:
#	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common package-messages
#	$(MAKE) -C po merge
#
#EXTRA_DIST = admin $LICENSEFILE$ configure.in.in
#
#dist-hook:
#	cd $(top_distdir) && perl admin/am_edit -padmin
#	cd $(top_distdir) && $(MAKE) -f admin/Makefile.common subdirs
