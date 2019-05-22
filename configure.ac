# Process this file with autoconf to produce a configure script.

AC_INIT([gmrun], [0.9.5w])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE
LT_INIT

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_PROG_CXX

AC_ARG_ENABLE(debug,
	[  --enable-debug=[no/yes]	Debugging information == no <--> optimization == yes],[],
	enable_debug=no)

if [ ! test "x$enable_debug" != "xyes"]; then
	CFLAGS="-g3 -O0 -DDEBUG"
	CXXFLAGS="-g3 -O0 -DDEBUG"
fi

AC_ARG_ENABLE(mtrace,
    [  --enable-mtrace=[no/yes]  Trace memory (de)allocation calls to detect memory leaks ],[],
    enable_mtrace=no)

if [ ! test "x$enable_mtrace" != "xyes"]; then
   CFLAGS="$CFLAGS -DMTRACE"
   CXXFLAGS="$CXXFLAGS -DMTRACE"
fi

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.10)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PREFIX=${prefix}
AC_SUBST(PREFIX)

AC_OUTPUT([
Makefile
src/Makefile
])

