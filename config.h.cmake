/* Define to 1 if you have the <Carbon/Carbon.h> header file. */
#cmakedefine HAVE_CARBON_CARBON_H 1

/* Define if you have the CoreAudio API */
#cmakedefine HAVE_COREAUDIO 1

/* Define to 1 if you have the <crt_externs.h> header file. */
#cmakedefine HAVE_CRT_EXTERNS_H 1

/* Defines if your system has the crypt function */
#cmakedefine HAVE_CRYPT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define if you have libjpeg */
#cmakedefine HAVE_LIBJPEG 1

/* Define if you have libpng */
#cmakedefine HAVE_LIBPNG 1

/* Define if you have a working libpthread (will enable threaded code) */
#cmakedefine HAVE_LIBPTHREAD 1

/* Define if you have libz */
#cmakedefine HAVE_LIBZ 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define if your system needs _NSGetEnviron to set up the environment */
#cmakedefine HAVE_NSGETENVIRON 1

/* Define if you have res_init */
#cmakedefine HAVE_RES_INIT 1

/* Define if you have the res_init prototype */
#cmakedefine HAVE_RES_INIT_PROTO 1

/* Define if you have a STL implementation by SGI */
#cmakedefine HAVE_SGI_STL 1

/* Define to 1 if you have the `snprintf' function. */
#cmakedefine HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define if you have strlcat */
#cmakedefine HAVE_STRLCAT 1

/* Define if you have the strlcat prototype */
#cmakedefine HAVE_STRLCAT_PROTO 1

/* Define if you have strlcpy */
#cmakedefine HAVE_STRLCPY 1

/* Define if you have the strlcpy prototype */
#cmakedefine HAVE_STRLCPY_PROTO 1

/* Define to 1 if you have the <sys/bitypes.h> header file. */
#cmakedefine HAVE_SYS_BITYPES_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the `vsnprintf' function. */
#cmakedefine HAVE_VSNPRINTF 1

#if 0
/* Suffix for lib directories */
#define KDELIBSUFF

/* Define a safe value for MAXPATHLEN */
#define KDEMAXPATHLEN

/* Name of package */
#define PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#define PACKAGE_NAME

/* Define to the full name and version of this package. */
#define PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME

/* Define to the version of this package. */
#define PACKAGE_VERSION
#endif /* #if 0 */

/* The size of `char *', as computed by sizeof. */
#define SIZEOF_CHAR_P ${SIZEOF_CHAR_P}

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT ${SIZEOF_INT}

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG ${SIZEOF_LONG}

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT ${SIZEOF_SHORT}

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T ${SIZEOF_SIZE_T}

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG ${SIZEOF_UNSIGNED_LONG}

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define the application version */
#define VERSION "${KCHECKGMAIL_VERSION}"

/* Defined if compiling without arts */
#cmakedefine WITHOUT_ARTS

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#cmakedefine WORDS_BIGENDIAN 1

/*
 * jpeg.h needs HAVE_BOOLEAN, when the system uses boolean in system
 * headers and I'm too lazy to write a configure test as long as only
 * unixware is related
 */
#ifdef _UNIXWARE
#define HAVE_BOOLEAN
#endif



/*
 * AIX defines FD_SET in terms of bzero, but fails to include <strings.h>
 * that defines bzero.
 */

#if defined(_AIX)
#include <strings.h>
#endif



#if defined(HAVE_NSGETENVIRON) && defined(HAVE_CRT_EXTERNS_H)
# include <sys/time.h>
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#endif



#if !defined(HAVE_RES_INIT_PROTO)
#ifdef __cplusplus
extern "C" {
#endif
int res_init(void);
#ifdef __cplusplus
}
#endif
#endif



#if !defined(HAVE_STRLCAT_PROTO)
#ifdef __cplusplus
extern "C" {
#endif
unsigned long strlcat(char*, const char*, unsigned long);
#ifdef __cplusplus
}
#endif
#endif



#if !defined(HAVE_STRLCPY_PROTO)
#ifdef __cplusplus
extern "C" {
#endif
unsigned long strlcpy(char*, const char*, unsigned long);
#ifdef __cplusplus
}
#endif
#endif



/*
 * On HP-UX, the declaration of vsnprintf() is needed every time !
 */

#if !defined(HAVE_VSNPRINTF) || defined(hpux)
#if __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif
#ifdef __cplusplus
extern "C"
#endif
int vsnprintf(char *str, size_t n, char const *fmt, va_list ap);
#ifdef __cplusplus
extern "C"
#endif
int snprintf(char *str, size_t n, char const *fmt, ...);
#endif



#if defined(__SVR4) && !defined(__svr4__)
#define __svr4__ 1
#endif


/* type to use in place of socklen_t if not defined */
#undef kde_socklen_t

/* type to use in place of socklen_t if not defined (deprecated, use
   kde_socklen_t) */
#undef ksize_t
