# Configuration variables governing the build of gpredict for win32 (or win64)

# Choose between 32-bit or 64-bit compiler
MINGW_ROOT=/mingw32
MGW_PREFIX=i686-w64-mingw32-
#MINGW_ROOT=/mingw64
#MGW_PREFIX=x86_64-w64-mingw32-

# binary dependencies to be deployed with gpredict.exe
BINDEPS = \
	$(wildcard $(MINGW_ROOT)/bin/*.dll)

# where to put the loaders.cache file and dlls
LOADERS = lib/gdk-pixbuf-2.0/2.10.0
LOADER_DLLS =  $(MINGW_ROOT)/$(LOADERS)/loaders/

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
