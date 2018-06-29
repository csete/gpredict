# Configuration variables governing the build of gpredict for win32

# pkgconfig path, assumes goocanvas-2.0.2 and gtk+-3.10.4 win32 packages
# unpacked and paths adjusted (pkg-config files) at the same folder level
# as gpredict. Downloads used:
# 
# http://ftp.gnome.org/pub/GNOME/binaries/win32/goocanvas/2.0/
# download both: goocanvas-2.0.2-win32.zip, goocanvas-dev-2.0.2-win32.zip
# 
# http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win32.zip
# NB: I had to create new .pc files for both gtk+-3.0.pc and gdk.pc
# (thanks GNOME for missing crucial bits out of builds). My versions
# are in this folder, and will need unpack paths inserting.

MINGW_ROOT=../../mingw32
#MINGW_ROOT=../../../tmp/mingw32

PKG_CONFIG_PATH = "$(abspath $(MINGW_ROOT)/lib/pkgconfig)"

# binary dependencies to be deployed with gpredict.exe
BINDEPS = \
	$(wildcard $(MINGW_ROOT)/bin/*.dll) \
	$(MINGW_ROOT)/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-svg.dll

# where to put the loaders.cache file
LOADERS = lib/gdk-pixbuf-2.0/2.10.0

# other miscellaneous folders to deploy with the binary
GTKETC  = $(MINGW_ROOT)/etc
SCHEMAS = $(MINGW_ROOT)/share/glib-2.0/schemas

# Autoversioning from nearest git tag, assumes v<x>.<y> tag format.

GITVER := $(shell git describe)
GITSEP := $(subst -, ,$(GITVER))
GITTAG := $(word 1,$(GITSEP))
GITBLD := $(word 2,$(GITSEP))
GITCOM := $(word 3,$(GITSEP))
GITMAJ := $(subst v,,$(word 1,$(subst ., ,$(GITTAG))))
GITMIN := $(word 2,$(subst ., ,$(GITTAG)))
