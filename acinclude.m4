#
# $Id: acinclude.m4,v 1.1 2002/06/05 19:39:18 sonofkojak Exp $
#

#
# STLPort definitions
#
AC_DEFUN(AC_PATH_STLPORT_INC,
[
	AC_REQUIRE_CPP()
	AC_MSG_CHECKING(for stlport headers)

	ac_stlport_includes="no"
	ac_enable_stlport="yes"

	AC_ARG_ENABLE(stlport,
	[ --enable-stlport		do we need stlport ],
	[ ac_enable_stlport="$enableval" ])
	if test "$ac_enable_stlport" = "no"; then
		AC_SUBST(STLPORT_INCDIR)
		AC_SUBST(STLPORT_CXXFLAGS)
		have_stlport_inc="no"
	else
			
		AC_ARG_WITH(stlport-includes,
		[ --with-stlport-includes        where stlport headers are located],
		[ ac_stlport_includes="$withval"])


		AC_CACHE_VAL(ac_cv_header_stlport_includes, [

		dnl did user give --with-stlport-includes?
		
		if test "$ac_stlport_includes" = "no"; then
			
			stlport_include_dirs="\
				$LIBCONFIGDIR/include/stlport \
				/usr/include/stlport \
				/usr/local/include/stlport \
				/usr/local/include/ 
				/usr/local/stlport/include"

			for dir in $stlport_include_dirs; do
				if test -r "$dir/algorithm"; then
					ac_stlport_includes=$dir
					break
				fi
			done
		fi
		
		dnl caching
		ac_cv_header_stlport_includes=$ac_stlport_includes
		])

		if test "$ac_cv_header_stlport_includes" = "no"; then
			have_stlport_inc="no"
			STLPORT_INCDIR=""
			STLPORT_CXXFLAGS=""
		else
			have_stlport_inc="yes"
			STLPORT_INCDIR="$ac_cv_header_stlport_includes"
			STLPORT_CXXFLAGS="-I$STLPORT_INCDIR"
		fi
			

		AC_SUBST(STLPORT_INCDIR)
		AC_SUBST(STLPORT_CXXFLAGS)
	fi

	AC_MSG_RESULT([$ac_cv_header_stlport_includes])
])


AC_DEFUN(AC_PATH_STLPORT_LIB,
[
	AC_REQUIRE_CPP()
	AC_MSG_CHECKING(for stlport libraries)

	ac_stlport_lib="no"

	ac_enable_stlport="yes"

	AC_ARG_ENABLE(stlport,
	[ --enable-stlport		do we need stlport ],
	[ ac_enable_stlport="$enableval" ])
	if test "$ac_enable_stlport" = "no"; then
		AC_SUBST(STLPORT_INCDIR)
		AC_SUBST(STLPORT_INCLUDES)
		have_stlport_inc="no"
	else
		
		AC_ARG_WITH(stlport-lib,
		[ --with-stlport-lib        where stlport library is located],
		[ ac_stlport_lib="$withval"])

		
		AC_CACHE_VAL(ac_cv_lib_stlport_lib, [

		dnl did user give --with-stlport-lib?
		
		if test "$ac_stlport_lib" = "no"; then
			
			stlport_lib_dirs="\
				$STLPORTDIR/lib \
				/usr/lib \
				/usr/local/lib \
				/usr/local/tiit/lib"

			for dir in $stlport_lib_dirs; do
				if test -r "$dir/libstlport_gcc.so"; then
					ac_stlport_lib=$dir
					break
				fi
			done
		fi
		
		dnl caching
		ac_cv_lib_stlport_lib=$ac_stlport_lib
		])

		if test "$ac_cv_lib_stlport_lib" = "no"; then
			have_stlport_lib="no"
			STLPORT_LIBDIR=""
			STLPORT_LDFLAGS=""
		else
			have_stlport_lib="yes"
			STLPORT_LIBDIR="$ac_cv_lib_stlport_lib"
			STLPORT_LDFLAGS="-L$STLPORT_LIBDIR -lstlport_gcc -lpthread"
		fi

		AC_SUBST(STLPORT_LIBDIR)
		AC_SUBST(STLPORT_LDFLAGS)
	fi

	AC_MSG_RESULT([$ac_cv_lib_stlport_lib])	
])

