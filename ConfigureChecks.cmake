include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(CheckCXXSourceCompiles)

include(TestBigEndian)

include(MacroBoolTo01)

if (APPLE)
   find_package(Carbon REQUIRED)
endif (APPLE)
macro_bool_to_01(CARBON_FOUND HAVE_CARBON)

#TODO: handle COREAUDIO API
#cmakedefine HAVE_COREAUDIO 1

check_include_files(crt_externs.h    HAVE_CRT_EXTERNS_H)

check_library_exists(crypt crypt ""  HAVE_CRYPT)

check_include_files(dlfcn.h          HAVE_DLFCN_H)
check_include_files(inttypes.h       HAVE_INTTYPES_H)

find_package(JPEG)
macro_bool_to_01(JPEG_FOUND HAVE_LIBJPEG)

find_package(PNG)
macro_bool_to_01(PNG_FOUND HAVE_LIBPNG)

# Threads is already included
macro_bool_to_01(CMAKE_USE_PTHREADS_INIT HAVE_LIB_PTHREAD)

find_package(ZLIB)
macro_bool_to_01(ZLIB_FOUND HAVE_LIBZ)

check_include_files(memory.h            HAVE_MEMORY_H)
check_function_exists(_NSGetEnviron   HAVE_NSGETENVIRON)


#
# Check for libresolv was copied from kdelibs ConfigureChecks.cmake
#

# Check for libresolv
# e.g. on slackware 9.1 res_init() is only a define for __res_init, so we check both, Alex
check_library_exists(resolv res_init "" HAVE_RES_INIT_IN_RESOLV_LIBRARY)
check_library_exists(resolv __res_init "" HAVE___RES_INIT_IN_RESOLV_LIBRARY)
if (HAVE___RES_INIT_IN_RESOLV_LIBRARY OR HAVE_RES_INIT_IN_RESOLV_LIBRARY)
    set(HAVE_RES_INIT TRUE)
endif (HAVE___RES_INIT_IN_RESOLV_LIBRARY OR HAVE_RES_INIT_IN_RESOLV_LIBRARY)
check_prototype_exists(res_init "sys/types.h;netinet/in.h;arpa/nameser.h;resolv.h" HAVE_RES_INIT_PROTO)

#TODO: How to check for SGI STL ?
#/* Define if you have a STL implementation by SGI */
#cmakedefine HAVE_SGI_STL 1

check_symbol_exists(snprintf      "stdio.h"       HAVE_SNPRINTF)

check_include_files(stdint.h      HAVE_STDINT_H)
check_include_files(stdlib.h      HAVE_STDLIB_H)
check_include_files(strings.h     HAVE_STRINGS_H)
check_include_files(string.h      HAVE_STRING_H)

check_function_exists(strlcat     HAVE_STRLCAT)
check_prototype_exists(strlcat    "string.h"     HAVE_STRLCAT_PROTO)

check_function_exists(strlcpy     HAVE_STRLCPY)
check_prototype_exists(strlcpy    "string.h"     HAVE_STRLCPY_PROTO)

check_include_files(sys/bitypes.h HAVE_SYS_BITYPES_H)
check_include_files(sys/stat.h    HAVE_SYS_STAT_H)
check_include_files(sys/types.h   HAVE_SYS_TYPES_H)
check_include_files(unistd.h      HAVE_UNISTD_H)

check_symbol_exists(vsnprintf     "stdio.h"      HAVE_VSNPRINTF)

#/* Suffix for lib directories */
##cmakedefine KDELIBSUFF
#
#/* Define a safe value for MAXPATHLEN */
##cmakedefine KDEMAXPATHLEN
#
#/* Name of package */
##cmakedefine PACKAGE
#
#/* Define to the address where bug reports for this package should be sent. */
##cmakedefine PACKAGE_BUGREPORT
#
#/* Define to the full name of this package. */
##cmakedefine PACKAGE_NAME
#
#/* Define to the full name and version of this package. */
##cmakedefine PACKAGE_STRING
#
#/* Define to the one symbol short name of this package. */
##cmakedefine PACKAGE_TARNAME
#
#/* Define to the version of this package. */
##cmakedefine PACKAGE_VERSION

check_type_size("char *"  SIZEOF_CHAR_P)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("short" SIZEOF_SHORT)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)

# There is a better way ?
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

test_big_endian(WORDS_BIGENDIAN)
