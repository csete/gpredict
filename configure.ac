AC_INIT([gpredict],[m4_esyscmd(./git-version-gen .tarball-version)],[https://community.libre.space/c/gpredict])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADERS(build-config.h)
AM_INIT_AUTOMAKE([subdir-objects dist-bzip2 no-dist-gzip 1.6])

# ensure Makefiles are updated when Makefile.am is modified
AM_MAINTAINER_MODE([enable])

# kernel style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# check for programs
AC_PROG_CC
AC_SEARCH_LIBS([strerror],[cposix])

IT_PROG_INTLTOOL([0.21])
LT_INIT

AC_CHECK_HEADERS([sys/time.h unistd.h getopt.h])

if test "${ac_cv_c_compiler_gnu}" = "yes"; then
  CFLAGS="${CFLAGS} -Wall -Wextra -std=c11 -pedantic"
fi

# check for libm
AC_CHECK_LIB([m], [sin],, AC_MSG_ERROR([Can not find libm. Check your libc installation]))

PKG_PROG_PKG_CONFIG
if test "x$PKG_CONFIG" = x; then
    AC_MSG_ERROR(Gpredict requires pkg-config)
fi

# check for libcurl
if $PKG_CONFIG --atleast-version=7.19 libcurl; then
    CFLAGS="$CFLAGS `$PKG_CONFIG --cflags libcurl`"
    LIBS="$LIBS `$PKG_CONFIG --libs libcurl`"
else
    AC_MSG_ERROR(Gpredict requires libcurl-dev 7.19 or later)
fi

# check for glib 2.40 or later
if $PKG_CONFIG --atleast-version=2.40 glib-2.0; then
    CFLAGS="$CFLAGS"
    LIBS="$LIBS"
else
    AC_MSG_ERROR(Gpredict requires libglib-dev 2.40 or later)
fi

# check for gtk+ 3.0 or later
if $PKG_CONFIG --atleast-version=3.0 gtk+-3.0; then
    CFLAGS="$CFLAGS `$PKG_CONFIG --cflags gtk+-3.0`"
    LIBS="$LIBS `$PKG_CONFIG --libs gtk+-3.0`"
else
    AC_MSG_ERROR(Gpredict requires libgtk-3-dev)
fi

# check for libgps (optional)
if $PKG_CONFIG --atleast-version=2.90 libgps; then
    CFLAGS="$CFLAGS `$PKG_CONFIG --cflags libgps`"
    LIBS="$LIBS `$PKG_CONFIG --libs libgps`"
    havelibgps=true;
    AC_DEFINE(HAS_LIBGPS, 1, [Define if libgps is available])
else
    havelibgps=false;
fi

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

# Add the languages which your application supports here.
# Note that other progs only have ALL_LINGUAS and AM_GLIB_GNU_GETTEXT
ALL_LINGUAS="cs da de el en_GB en_US es fi fr id it lt ru th uk"
GETTEXT_PACKAGE=gpredict
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,
                   "$GETTEXT_PACKAGE",
                   "definition of GETTEXT_PACKAGE")


# compiler flags to enable generating coverage report
# using gcov
AC_ARG_ENABLE(coverage, [  --enable-coverage       enable coverge reports],enable_coerage=yes,enable_coverage=no)
if test "$enable_coverage" = yes ; then
        CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage";
	AC_DEFINE(ENABLE_COV, 1, [Define if code coverage should be enabled.])
fi

AC_ARG_ENABLE(caches,[  --enable-caches	  Run update-* to update desktop and icon caches when installing (disable if you install as not root)],,[enable_caches="no"])
AM_CONDITIONAL(UPDATE_CACHES, test x"$enable_caches" = "xyes")


GLIB_V=`$PKG_CONFIG --modversion glib-2.0`
GIO_V=`$PKG_CONFIG --modversion gio-2.0`
GTHR_V=`$PKG_CONFIG --modversion gthread-2.0`
GDK_V=`$PKG_CONFIG --modversion gdk-3.0`
GTK_V=`$PKG_CONFIG --modversion gtk+-3.0`
CURL_V=`$PKG_CONFIG --modversion libcurl`
if test "$havelibgps" = true ; then
   GPS_V=`$PKG_CONFIG --modversion libgps`
fi
 

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)


AC_CONFIG_FILES([
Makefile
doc/Makefile
doc/man/gpredict.1
doc/man/Makefile
src/Makefile
src/sgpsdp/Makefile
src/sgpsdp/TR/Makefile
icons/Makefile
pixmaps/Makefile
pixmaps/maps/Makefile
pixmaps/logos/Makefile
pixmaps/icons/Makefile
data/Makefile
data/desktop/Makefile
data/metainfo/Makefile
data/satdata/Makefile
po/Makefile.in
])
AC_OUTPUT

echo
echo SUMMARY:
echo
echo Gpredict version... : $VERSION
echo Glib version....... : $GLIB_V
echo Gio version........ : $GIO_V
echo Gthread version.... : $GTHR_V
echo Gdk version........ : $GDK_V
echo Gtk+ version....... : $GTK_V
echo Libcurl version.... : $CURL_V
if test "$havelibgps" = true ; then
   echo Libgps version..... : $GPS_V
fi
# echo Enable coverage.... : $enable_coverage
# echo

