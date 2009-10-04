AC_INIT(src/main.c)

AM_CONFIG_HEADER(build-config.h)

AM_INIT_AUTOMAKE(gpredict, 1.1)

AM_MAINTAINER_MODE
AC_PROG_INTLTOOL([0.21])


dnl check for programs
AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/time.h unistd.h getopt.h])

if test "${ac_cv_c_compiler_gnu}" = "yes"; then
  CFLAGS="${CFLAGS} -Wall"
fi

dnl check for glib, gtk, and goocanvas libraries
pkg_modules="gtk+-2.0 >= 2.12.0 glib-2.0 >= 2.16.0 gthread-2.0 >= 2.14.0 goocanvas >= 0.9 libcurl >= 7.16.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)


dnl Hamlib not needed if we go for the TCP based I/F
dnl check for hamlib
dnl PKG_CHECK_MODULES(HAMLIB, hamlib, [
dnl        CFLAGS="$CFLAGS $HAMLIB_CFLAGS";
dnl        LIBS="$LIBS $HAMLIB_LIBS"; 
dnl        havehamlib=true;            
dnl        AC_DEFINE(HAVE_HAMLIB, 1, [Define if hamlib is available])
dnl   ], [
dnl        havehamlib=false;            
dnl        AC_DEFINE(HAVE_HAMLIB, 0, [Define if hamlib is unvailable])

dnl])

dnl Add the languages which your application supports here.
dnl Note that other progs only have ALL_LINGUAS and AM_GLIB_GNU_GETTEXT
ALL_LINGUAS="en_GB en_US fr"
GETTEXT_PACKAGE=gpredict
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,
                   "$GETTEXT_PACKAGE",
                   "definition of GETTEXT_PACKAGE")


dnl compiler flags to enable generating coverage report
dnl using gcov
AC_ARG_ENABLE(coverage, [  --enable-coverage       enable coverge reports],enable_coerage=yes,enable_coverage=no)
if test "$enable_coverage" = yes ; then
        CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage";
	AC_DEFINE(ENABLE_COV, 1, [Define if code coverage should be enabled.])
fi


GLIB_V=`pkg-config --modversion glib-2.0`
GIO_V=`pkg-config --modversion gio-2.0`
GTHR_V=`pkg-config --modversion gthread-2.0`
GDK_V=`pkg-config --modversion gdk-2.0`
GTK_V=`pkg-config --modversion gtk+-2.0`
GOOC_V=`pkg-config --modversion goocanvas`
CURL_V=`pkg-config --modversion libcurl`
dnl SQL_V=`pkg-config --modversion sqlite3`
dnl if test "$havehamlib" = true ; then
dnl    HAML_V=`pkg-config --modversion hamlib`
dnl fi
 

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
echo Liburl version..... : $CURL_V
dnl echo SQLite version..... : $SQL_V
dnl if test "$havehamlib" = true ; then
dnl echo Hamlib version..... : $HAML_V
dnl else
dnl echo Hamlib version..... : none
dnl fi
dnl echo Enable coverage.... : $enable_coverage
dnl echo

