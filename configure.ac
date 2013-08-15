AC_INIT([gpredict],
        m4_esyscmd([./git-version-gen .tarball-version]),
        [gpredict-discussion@lists.sourceforge.net])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/main.c)
AM_CONFIG_HEADER(build-config.h)
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip 1.6])

AM_MAINTAINER_MODE
AC_PROG_INTLTOOL([0.21])

# kernel style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# check for programs
AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/time.h unistd.h getopt.h])

if test "${ac_cv_c_compiler_gnu}" = "yes"; then
  CFLAGS="${CFLAGS} -Wall"
fi

# check for libm
AC_CHECK_LIB([m], [sin],, AC_MSG_ERROR([Can not find libm. Check your libc installation]))

# check for libcurl
if pkg-config --atleast-version=7.19 libcurl; then
    CFLAGS="$CFLAGS `pkg-config --cflags libcurl`"
    LIBS="$LIBS `pkg-config --libs libcurl`"
else
    AC_MSG_ERROR(Gpredict requires libcurl-dev 7.19 or later)
fi

# check for glib >2.32
if pkg-config --atleast-version=2.32 glib-2.0; then
    CFLAGS="$CFLAGS"
    LIBS="$LIBS"
else
    AC_MSG_ERROR(Gpredict requires libglib-dev 2.32 or later)
fi

# check for goocanvas (depends on gtk and glib)
if pkg-config --atleast-version=0.15 goocanvas; then
    CFLAGS="$CFLAGS `pkg-config --cflags goocanvas`"
    LIBS="$LIBS `pkg-config --libs goocanvas`"
else
    AC_MSG_ERROR(Gpredict requires libgoocanvas-dev 0.15 or later)
fi

# check for libgps (optional)
if pkg-config --atleast-version=2.90 libgps; then
    CFLAGS="$CFLAGS `pkg-config --cflags libgps`"
    LIBS="$LIBS `pkg-config --libs libgps`"
    havelibgps=true;
    AC_DEFINE(HAS_LIBGPS, 1, [Define if libgps is available])
else
    havelibgps=false;
fi

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

# Add the languages which your application supports here.
# Note that other progs only have ALL_LINGUAS and AM_GLIB_GNU_GETTEXT
ALL_LINGUAS="cs da de en_GB en_US es fi fr id it lt ru th uk"
GETTEXT_PACKAGE=gpredict
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
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


GLIB_V=`pkg-config --modversion glib-2.0`
GIO_V=`pkg-config --modversion gio-2.0`
GTHR_V=`pkg-config --modversion gthread-2.0`
GDK_V=`pkg-config --modversion gdk-2.0`
GTK_V=`pkg-config --modversion gtk+-2.0`
GOOC_V=`pkg-config --modversion goocanvas`
CURL_V=`pkg-config --modversion libcurl`
if test "$havelibgps" = true ; then
   GPS_V=`pkg-config --modversion libgps`
fi
 

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)


AC_OUTPUT([
Makefile
doc/Makefile
doc/man/gpredict.1
doc/man/Makefile
src/Makefile
src/sgpsdp/Makefile
src/sgpsdp/TR/Makefile
pixmaps/Makefile
pixmaps/maps/Makefile
pixmaps/icons/Makefile
data/Makefile
data/desktop/Makefile
data/satdata/Makefile
data/trsp/Makefile
po/Makefile.in
])

echo
echo SUMMARY:
echo
echo Gpredict version... : $VERSION
echo Glib version....... : $GLIB_V
echo Gio version........ : $GIO_V
echo Gthread version.... : $GTHR_V
echo Gdk version........ : $GDK_V
echo Gtk+ version....... : $GTK_V
echo GooCanvas version.. : $GOOC_V
echo Libcurl version..... : $CURL_V
if test "$havelibgps" = true ; then
   echo Libgps version..... : $GPS_V
fi
# echo Enable coverage.... : $enable_coverage
# echo

