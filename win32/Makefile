# Makefile for cross-compiling gpredict for win32 on Linux,
# using mingw compiler

# configure pkg-config, version from git
include config.mk

# version string in C code
VERSION_SHORT = $(GITMAJ).$(GITMIN).$(GITBLD)
VERSION_LONG = $(GITMAJ).$(GITMIN).0.$(GITBLD)

# cross-compiler prefix
MGW_PREFIX=i686-w64-mingw32-

# directories
topsrc = ..
gpreddir = $(topsrc)/src
sgpsdpdir = $(gpreddir)/sgpsdp
nxjsondir = $(gpreddir)/nxjson

# tools
CC = $(MGW_PREFIX)gcc -Wall -O2 -mms-bitfields -DWIN32 -m32
AS = $(MGW_PREFIX)as
DLLWRAP = $(MGW_PREFIX)dllwrap
DLLWRAP_FLAGS = --as=$(AS) --export-all --driver-name $(CC) -s
RC = $(MGW_PREFIX)windres
# NB: .rc file create various version formats from the numbers
RCFLAGS = --define __WIN32__ --define __WIN95__ --define MSRC --define __GNUWIN32__ \
	--define VERMAJ=$(GITMAJ) --define VERMIN=$(GITMIN) --define VERBLD=$(GITBLD)

# libraries
GTKLIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs gtk+-3.0)
GLIBLIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs glib-2.0 gthread-2.0)
GOOLIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs goocanvas-2.0)
GUI_LIBS = $(GTKLIBS) $(GLIBLIB) $(GOOLIBS) libsgpsdp.lib -lwinmm -lwininet -lws2_32
LIBS = -lm

# flags/defines
GTK_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags gtk+-3.0)
GOO_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags goocanvas-2.0)
CFLAGS = -DVERSION_SHORT=$(VERSION_SHORT) -DHAVE_CONFIG_H -DPACKAGE_LOCALE_DIR=\"/\" -I. -I.. -I$(gpreddir) $(GOO_CFLAGS)

# source paths
vpath %.c $(sgpsdpdir)
vpath %.c $(gpreddir)
vpath %.h $(gpreddir)
vpath %.h $(sgpsdpdir)
vpath %.c $(nxjsondir)
vpath %.h $(nxjsondir)
vpath %.rc $(topsrc)/win32
vpath %.c .

#### End of system configuration section. ####

SGPSDPSRC = \
	sgp4sdp4.c \
	sgp_in.c \
	sgp_math.c \
	sgp_obs.c \
	sgp_time.c \
	solar.c

SGPSDPOBJ = $(SGPSDPSRC:.c=.o)

NXJSONSRC = \
	nxjson.c

NXJSONOBJ = $(NXJSONSRC:.c=.o)

GOOCANVSRC = \
        goocanvasatk.c             \
        goocanvasellipse.c         \
	goocanvasenumtypes.c		\
	goocanvasgroup.c		\
	goocanvasimage.c		\
	goocanvasitem.c			\
	goocanvasitemmodel.c		\
	goocanvasitemsimple.c		\
	goocanvasmarshal.c		\
	goocanvaspolyline.c		\
	goocanvaspath.c			\
	goocanvasprivate.h		\
	goocanvasrect.c			\
	goocanvasstyle.c		\
	goocanvastable.c		\
	goocanvastext.c			\
	goocanvasutils.c		\
	goocanvaswidget.c		\
	goocanvas.c 


GOOCANVOBJ = $(GOOCANVSRC:.c=.o)


GPREDICTSRC = \
	about.c \
	compat.c \
	first-time.c \
	gpredict-help.c \
	gpredict-utils.c \
	gtk-azel-plot.c \
    gtk-event-list.c \
    gtk-event-list-popup.c \
	gtk-freq-knob.c \
	gtk-polar-plot.c \
	gtk-polar-view.c \
	gtk-polar-view-popup.c \
	gtk-rig-ctrl.c \
	gtk-rot-ctrl.c \
	gtk-rot-knob.c \
	gtk-sat-data.c \
	gtk-sat-list.c \
	gtk-sat-list-popup.c \
	gtk-sat-map.c \
	gtk-sat-map-ground-track.c \
	gtk-sat-map-popup.c \
	gtk-sat-module.c \
	gtk-sat-module-popup.c \
	gtk-sat-module-tmg.c \
    gtk-sat-popup-common.c \
	gtk-sat-selector.c \
	gtk-single-sat.c \
	gtk-sky-glance.c \
	gui.c \
	locator.c \
	loc-tree.c \
	main.c \
	map-selector.c \
	map-tools.c \
	menubar.c \
	mod-cfg.c \
	mod-cfg-get-param.c \
	mod-mgr.c \
	orbit-tools.c \
	pass-popup-menu.c \
	pass-to-txt.c \
	predict-tools.c \
	print-pass.c \
	qth-data.c \
	qth-editor.c \
	radio-conf.c \
	rotor-conf.c \
	sat-cfg.c \
	sat-info.c \
	sat-log.c \
	sat-log-browser.c \
	sat-monitor.c \
	sat-pass-dialogs.c \
	sat-pref.c \
	sat-pref-conditions.c \
	sat-pref-debug.c \
	sat-pref-formats.c \
	sat-pref-general.c \
	sat-pref-interfaces.c \
	sat-pref-layout.c \
	sat-pref-list-view.c \
	sat-pref-map-view.c \
	sat-pref-modules.c \
	sat-pref-multi-pass.c \
	sat-pref-polar-view.c \
	sat-pref-predict.c \
	sat-pref-qth.c \
	sat-pref-qth-editor.c \
	sat-pref-refresh.c \
	sat-pref-rig.c \
	sat-pref-rig-editor.c \
	sat-pref-rot.c \
	sat-pref-rot-editor.c \
	sat-pref-single-pass.c \
	sat-pref-single-sat.c \
	sat-pref-sky-at-glance.c \
	sat-pref-tle.c \
	sat-vis.c \
	save-pass.c \
	strnatcmp.c \
	time-tools.c \
	tle-tools.c \
	tle-update.c \
	trsp-conf.c \
	trsp-update.c \
	win32-fetch.c

GPREDICTOBJ = $(GPREDICTSRC:.c=.o)

OBJS = $(SGPSDPOBJ) $(NXJSONOBJ) $(GPREDICTOBJ)

%.o: %.c
	$(CC) -c $(CFLAGS) $(GTK_CFLAGS) $<
	$(CC) -MM $(CFLAGS) $(GTK_CFLAGS) $< > $*.d 


# targets begin

all: libsgpsdp.dll gpredict.exe gpredict-con.exe

dist: gpredict-$(GITMAJ).$(GITMIN).$(GITBLD).zip

gpredict-$(GITMAJ).$(GITMIN).$(GITBLD).zip: all
	mkdir -p gpredict/share/gpredict
	cp -p libsgpsdp.dll gpredict.exe gpredict-con.exe $(BINDEPS) gpredict
	cp -rp ../data gpredict/share/gpredict
	cp -rp ../pixmaps gpredict/share/gpredict
	cp -rp ../po gpredict/share/gpredict
	mkdir -p gpredict/$(LOADERS)
	cp -p loaders.cache gpredict/$(LOADERS)
	cp -rp $(GTKETC) gpredict
	mkdir -p gpredict/share/glib-2.0
	cp -rp $(SCHEMAS) gpredict/share/glib-2.0
	zip -r $@ gpredict
	rm -rf gpredict

# Use -mconsole to always open a console window when gpredicxt is started
# Use -mwindows for no console
gpredict.exe: TARGET:=-mwindows
gpredict-con.exe: TARGET:=-mconsole

gpredict.exe gpredict-con.exe: $(OBJS) gpredict_res.o
	$(CC) $(TARGET) -mthreads -o $@ $^ $(CFLAGS) $(GTK_CFLAGS) $(LIBS) $(GUI_LIBS) -lmingwex -s

gpredict_res.o: gpredict.rc gpredict.exp.manifest
	$(RC) $(RCFLAGS) gpredict.rc $@

gpredict.exp.manifest: gpredict.exe.manifest
	sed -e 's/VERSION_STRL/$(VERSION_LONG)/' < $< > $@

libgoocanvas.dll: $(GOOCANVOBJ)
	$(CC) -shared $(CFLAGS) $(GTK_CFLAGS) $(GOOCANVOBJ) $(GTKLIBS) -lm -lmingwex \
	-Wl,--enable-auto-image-base -Wl,--out-implib,libgoocanvas.lib -o libgoocanvas.dll

libsgpsdp.dll:  $(SGPSDPOBJ)
	$(CC) -shared $(CFLAGS) $(GTK_CFLAGS) $(SGPSDPOBJ) $(GLIBLIB) -lm -lmingwex \
	-Wl,--enable-auto-image-base -Wl,--out-implib,libsgpsdp.lib -o libsgpsdp.dll

clean:
	rm -f *.o *.d *.def *.a *.lib *.dll *.exe *.exp.manifest *.zip
	rm -rf gpredict

