# configure.ac

AC_PREREQ(2.68)
AC_INIT([Iota Metrics Collection System], [0.0.1], [bugs@niftylogic.com])

AC_CONFIG_SRCDIR([src/iota.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AX_TEST_COVERAGE

AC_PROG_CC

AC_HAVE_LIBRARY(pthread,,  AC_MSG_ERROR(Missing pthread library))
AC_HAVE_LIBRARY(ctap,,     AC_MSG_ERROR(Missing ctap testing library - see http://github.com/filefrog/ctap))
AC_HAVE_LIBRARY(vigor,,    AC_MSG_ERROR(Missing libvigor - see http://github.com/filefrog/libvigor))
AC_HAVE_LIBRARY(m,,        AC_MSG_ERROR(Missing -lm math library))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
