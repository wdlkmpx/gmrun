# Process this file with autoconf to produce a configure script.

AC_INIT([gmrun], [0.9.2])
AM_CONFIG_HEADER([config.h])
#AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE
AM_PROG_AR([ar])
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

AC_CHECK_FUNC( poptGetContext,
              ,
              AC_CHECK_LIB(popt,
                            poptGetContext,
                           LIBS="${LIBS} -lpopt",
                           echo "*** You need to get libpopt ***";exit
                           )
              )

AC_CHECK_FUNC( XQueryPointer,
              ,
              AC_CHECK_LIB(X11,
                           XQueryPointer,
                           LIBS="${LIBS} -lX11",
                           echo "*** You need to get libX11 ***";exit
                           )
              )

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.4
                        gobject-2.0 >= 2.0.4
                        gthread-2.0 >= 2.0.4)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.5)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}", "PACKAGE_DATA_DIR")
  PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}", "PACKAGE_DATA_DIR")
  PACKAGE_DATA_DIR="${datadir}/${PACKAGE}"
fi
AC_SUBST(PACKAGE_DATA_DIR)

PREFIX=${prefix}
AC_SUBST(PREFIX)

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl


AC_OUTPUT([
Makefile
src/Makefile
config/Makefile
gmrun.spec
])

