AC_INIT([Galactic Turtle], [1.1.0], [foutrelis@gmail.com])
AC_CONFIG_SRCDIR([src/galacticd.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_CXX

dnl sqlite3
AC_CHECK_HEADERS([sqlite3.h], [SQLITE_LIBS=-lsqlite3], [AC_MSG_ERROR("sqlite3 required")])
AC_CHECK_LIB([sqlite3], [sqlite3_prepare_v2], [], [AC_MSG_ERROR(sqlite3 >= 3.3.9 required)])
AC_SUBST(SQLITE_LIBS)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
