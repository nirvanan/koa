#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([koa],[0.01],[nirvanan@live.cn])
AC_CONFIG_SRCDIR([src/str.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE([1.15 -Wall no-define tar-ustar no-dist-gzip dist-xz])

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.
AC_CHECK_LIB(m, isnan)
AC_CHECK_LIB(m, isfinite)

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h limits.h stddef.h stdint.h stdlib.h string.h])

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

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset modf strtol])

# Enable debug or not. 
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable DEBUG mode(default=no)]),[], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="-g2 -O0 -DDEBUG -Wall -pg"], 
	  [test "x$enable_debug" = "xno"], [CFLAGS="-O3 -Wall"], [])

AC_CONFIG_FILES([Makefile
src/Makefile
])

AC_OUTPUT
