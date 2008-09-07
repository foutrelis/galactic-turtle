AC_INIT([Galactic Turtle], [1.1.1], [foutrelis@gmail.com])
AC_CONFIG_SRCDIR([src/galacticd.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_CXX

dnl Check for sqlite3
PKG_CHECK_MODULES(SQLITE3, sqlite3 >= 3.3.9, , AC_MSG_ERROR([SQLite 3.3.9 or greater is required.]))
AC_SUBST(SQLITE3_CFLAGS)
AC_SUBST(SQLITE3_LIBS)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
