desktop_in_files = gpredict.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files) $(desktop_DATA)

DISTCLEANFILES = gpredict.desktop

UPDATE_DESKTOP = \
	if which update-desktop-database>/dev/null 2>&1 ; then \
		update-desktop-database; \
	fi

install-data-hook:
if UPDATE_CACHES
	$(UPDATE_DESKTOP)
else
	@true
endif

uninstall-hook:
if UPDATE_CACHES
	$(UPDATE_DESKTOP)
else
	@true
endif
