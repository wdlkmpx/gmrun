# Process this file with autoconf to produce a configure script.

AC_INIT([gmrun], [1.0w])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AM_MAINTAINER_MODE], [AM_MAINTAINER_MODE])
LT_INIT

AC_PROG_CC

AC_ARG_ENABLE(debug,
	[  --enable-debug=[no/yes]	Debugging information == no <--> optimization == yes],[],
	enable_debug=no)

if [ ! test "x$enable_debug" != xyes]; then
	CFLAGS="-g -DDEBUG"
fi

AC_ARG_ENABLE(mtrace,
    [  --enable-mtrace=[no/yes]  Trace memory (de)allocation calls to detect memory leaks ],[],
    enable_mtrace=no)

if [ ! test "x$enable_mtrace" != "xyes"]; then
   CFLAGS="$CFLAGS -DMTRACE"
fi

#========================================================================

GTK3_CHECK="gtk+-3.0 >= 3.4.0"
GTK2_CHECK="gtk+-2.0 >= 2.12.0"

AC_ARG_ENABLE(gtk3,
   AS_HELP_STRING([--disable-gtk3],[build with GTK3 (Autodetect)]),[],
   [enable_gtk3=check])

AC_ARG_ENABLE(gtk2,
   AS_HELP_STRING([--enable-gtk2],[build with GTK2 (Autodetect)]),[],
   [enable_gtk2=check])

AS_IF([test "x$enable_gtk3" = xyes],
   [enable_gtk2=no])
AS_IF([test "x$enable_gtk2" = xyes],
   [enable_gtk3=no])

AS_IF([test "x$enable_gtk3" = xcheck],
   [PKG_CHECK_MODULES([GTK],[$GTK3_CHECK],
      [enable_gtk3=yes],[enable_gtk3=no])])

AS_IF([test "x$enable_gtk3" = xyes],
   [
   gtk_modules="$GTK3_CHECK"
   gtk_version="gtk+-3.0"
   errmsg="GTK3 is not installed, use --enable-gtk2 to build with GTK2"
   ],
   [
   gtk_modules="$GTK2_CHECK"
   gtk_version="gtk+-2.0"
   errmsg="GTK2 is not installed, use --enable-gtk3 to build with GTK3"
   CPPFLAGS="$CPPFLAGS -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"
   ])

PKG_CHECK_MODULES([GTK], [$gtk_modules], [], [AC_ERROR([$errmsg])])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

#========================================================================

case "$CC" in
   gcc*) CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers" ;;
   *) ;;
esac

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
])

echo
echo "GTK Version: $(pkg-config --modversion $gtk_version)"
echo
