#! /bin/sh
#
# Copyright (c) 2002  Daniel Elstner  <daniel.elstner@gmx.net>,
#               2003  Murray Cumming  <murrayc@usa.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License VERSION 2 as
# published by the Free Software Foundation.  You are not allowed to
# use any other version of the license; unless you got the explicit
# permission from the author to do so.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


# This is meant to be a well-documented, good example of an autogen.sh script
# Please email gnome-devel-list@gnome.org if you think it isn't.


dir=`echo "$0" | sed 's,[^/]*$,,'`
test "x${dir}" = "x" && dir='.'

if test "x`cd "${dir}" 2>/dev/null && pwd`" != "x`pwd`"
then
    echo "This script must be executed directly from the source directory."
    exit 1
fi

# This might not be necessary with newer autotools:
rm -f config.cache

# We use glib-gettextize, which apparently does not add the intl directory 
# (containing a local copy of libintl code), and therefore has a slightly different Makefile.
echo "- glib-gettextize."	&& \
  glib-gettextize --copy --force 	&& \
echo "- libtoolize."		&& \
  libtoolize --force	&& \
echo "- intltoolize."		&& \
  intltoolize --copy --force	&& \
echo "- aclocal"		&& \
  aclocal 			&& \
echo "- autoheader"		&& \
  autoheader			&& \
echo "- autoconf."		&& \
  autoconf			&& \
echo "- automake."		&& \
  automake --add-missing --gnu	&& \
echo				&& \
  ./configure "$@"		&& exit 0

exit 1

