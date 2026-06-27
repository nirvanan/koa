#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([koa],[0.01],[nirvanan@live.cn])
AC_CONFIG_SRCDIR([src/str.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE([1.16 -Wall no-define tar-ustar no-dist-gzip dist-xz])

# Checks for programs.
user_CFLAGS=$CFLAGS
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [isnan])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h limits.h stddef.h stdint.h stdlib.h string.h pthread.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset modf strtol])

# Checks for pthread
AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CC="$PTHREAD_CC"

# Enable debug or not.
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [enable DEBUG mode (default=no)])],
	[enable_debug=$enableval],
	[enable_debug=no])

AS_IF([test "x$user_CFLAGS" != "x"],
	[CFLAGS="$user_CFLAGS $PTHREAD_CFLAGS"],
	[AS_IF([test "x$enable_debug" = "xyes"],
		[CFLAGS="-g2 -O0 -DDEBUG -Wall -pg $PTHREAD_CFLAGS"],
		[CFLAGS="-O3 -Wall $PTHREAD_CFLAGS"])])

AC_CONFIG_FILES([Makefile
src/Makefile
])

AC_OUTPUT
