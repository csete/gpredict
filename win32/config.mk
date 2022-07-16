# Configuration variables governing the build of gpredict for win32

# pkgconfig path, assumes goocanvas-3.0.0 and gtk+-3.10.4 win32 packages
# unpacked and paths adjusted (pkg-config files) at the same folder level
# as gpredict. Downloads used:
# 
# http://ftp.gnome.org/pub/GNOME/binaries/win32/goocanvas/3.0/
# download both: goocanvas-3.0.0-win32.zip, goocanvas-dev-3.0.0-win32.zip
# 
# http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win32.zip
# NB: I had to create new .pc files for both gtk+-3.0.pc and gdk.pc
# (thanks GNOME for missing crucial bits out of builds). My versions
# are in this folder, and will need unpack paths inserting.

# Change the path below to select the appropriate install path and compiler 
# for 32 and 64 bit cross-compiles.  Note that if you have expicitly changed
# the PATH environment variable, you may need to make additional changes.
#
# Windows hosts will require a configuration like this for 32 bit compiles:
# MINGW_ROOT=/mingw32
# or this, for 64 bit compiles:
# MINGW_ROOT=/mingw64
#
# Linux or other hosts require a path that is dependent upon the installation.
# For 32 bit:
# MINGW_ROOT=../../msys64/mingw32
# For 64 bit:
# MINGW_ROOT=../../msys64/mingw64
#
MINGW_ROOT=/mingw64

PKG_CONFIG_PATH = "$(abspath $(MINGW_ROOT)/lib/pkgconfig)"

# binary dependencies to be deployed with gpredict.exe
BINDEPS = \
	$(wildcard $(MINGW_ROOT)/bin/*.dll) \
	$(MINGW_ROOT)/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-svg.dll \
	$(MINGW_ROOT)/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-png.dll \
	$(MINGW_ROOT)/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-jpeg.dll

# where to put the loaders.cache file
LOADERS = lib/gdk-pixbuf-2.0/2.10.0

# other miscellaneous folders to deploy with the binary
GTKETC  = $(MINGW_ROOT)/etc
SCHEMAS = $(MINGW_ROOT)/share/glib-2.0/schemas
ADWAITA = $(MINGW_ROOT)/share/icons/Adwaita

# Autoversioning from nearest git tag, assumes v<x>.<y> tag format.

GITVER := $(shell git describe)
GITSEP := $(subst -, ,$(GITVER))
GITTAG := $(word 1,$(GITSEP))
GITBLD := $(word 2,$(GITSEP))
GITCOM := $(word 3,$(GITSEP))
GITMAJ := $(subst v,,$(word 1,$(subst ., ,$(GITTAG))))
GITMIN := $(word 2,$(subst ., ,$(GITTAG)))
