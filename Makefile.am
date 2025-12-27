ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc icons pixmaps data po

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

install-data-local:
	@$(NORMAL_INSTALL)
	$(INSTALL_DATA) $(top_srcdir)/AUTHORS $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(top_srcdir)/NEWS $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(top_srcdir)/README $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(DESTDIR)$(pkgdatadir)

EXTRA_DIST = \
	autogen.sh            \
	git-version-gen       \
	intltool-extract.in   \
	intltool-merge.in     \
	intltool-update.in    \
	snap/snapcraft.yaml   \
	win32/build-config.h  \
	win32/config.mk       \
	win32/gpredict.rc     \
	win32/Makefile        \
	win32/README          \
	win32/resource.h      \
	win32/win32-fetch.c   \
	win32/win32-fetch.h   \
	win32/icons/gpredict-icon.ico
