# Makefile for cross-compiling gpredict for win32 on Linux,
# using mingw compiler

include config.mk

# directories
topsrc = ..
gcincdir = c:/mingw/include/goocanvas-0.10/goocanvas/
gcsrcdir = $(topsrc)/goocanv8/src
gpreddir = $(topsrc)/src
sgpsdpdir = $(gpreddir)/sgpsdp

# tools
CC = $(MGW_PREFIX)gcc -Wall -O2 -mms-bitfields -DWIN32
AS = $(MGW_PREFIX)as
DLLWRAP = $(MGW_PREFIX)dllwrap
DLLWRAP_FLAGS = --as=$(AS) --export-all --driver-name $(CC) -s
RC = $(MGW_PREFIX)windres
RCFLAGS = --define __WIN32__ --define __WIN95__ --define MSRC \
	--define __GNUWIN32__

# libraries
GTKLIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs gtk+-win32-2.0)
GLIBLIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs glib-2.0 gthread-2.0)
GUI_LIBS = $(GTKLIBS) $(GLIBLIB) -lgoocanvas libsgpsdp.lib -lcurldll -lwinmm -lws2_32
LIBS = -lm


# flags/defines
CFLAGS = -DHAVE_CONFIG_H -DHAVE_LIBCURL -DPACKAGE_LOCALE_DIR=\"/\"  \
	 -I. -I.. -I$(gpreddir) -I$(gcincdir) -I$(gcsrcdir) -I$(CROSSDIR)/include
GTK_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags gtk+-win32-2.0)

# source paths
vpath %.c $(gcsrcdir)
vpath %.c $(sgpsdpdir)
vpath %.c $(gpreddir)
vpath %.h $(gpreddir)
vpath %.h $(sgpsdpdir)
vpath %.h $(gcsrcdir)
vpath %.rc $(topsrc)/win32

#### End of system configuration section. ####

SGPSDPSRC = \
	sgp4sdp4.c \
	sgp_in.c \
	sgp_math.c \
	sgp_obs.c \
	sgp_time.c \
	solar.c

SGPSDPOBJ = $(SGPSDPSRC:.c=.o)


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
	gpredict-url-hook.c \
	gpredict-utils.c \
	gtk-azel-plot.c \
        gtk-event-list.c \
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
	gtk-sat-list-col-sel.c \
	gtk-sat-map.c \
	gtk-sat-map-ground-track.c \
	gtk-sat-map-popup.c \
	gtk-sat-module.c \
	gtk-sat-module-popup.c \
	gtk-sat-module-tmg.c \
	gtk-sat-selector.c \
	gtk-single-sat.c \
	gtk-sky-glance.c \
	gui.c \
	locator.c \
	loc-tree.c \
	main.c \
	map-selector.c \
	menubar.c \
	mod-cfg.c \
	mod-cfg-get-param.c \
	mod-mgr.c \
	orbit-tools.c \
	pass-popup-menu.c \
	pass-to-txt.c \
	predict-tools.c \
	qth-data.c \
	qth-editor.c \
	radio-conf.c \
	rotor-conf.c \
	sat-cfg.c \
	sat-debugger.c \
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
	sat-pref-help.c \
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
	time-tools.c \
	tle-tools.c \
	tle-update.c \
	trsp-conf.c \

GPREDICTOBJ = $(GPREDICTSRC:.c=.o)

OBJS = $(SGPSDPOBJ) $(GPREDICTOBJ)

%.o: %.c
	$(CC) -c $(CFLAGS) $(GTK_CFLAGS) $<
	$(CC) -MM $(CFLAGS) $(GTK_CFLAGS) $< > $*.d 


# targets begin

all: libsgpsdp.dll gpredict.exe


# Use -mconsole to always open a console window when gpredicxt is started
# Use -mwindows for no copnsole
gpredict.exe: $(OBJS) gpredict_res.o
	$(CC) -mconsole -mthread -o $@ $^ $(CFLAGS) $(GTK_CFLAGS) $(LIBS) $(GUI_LIBS) -lmingwex -s

gpredict_res.o: gpredict.rc 
	$(RC) $(RCFLAGS) $< $@

libgoocanvas.dll: $(GOOCANVOBJ)
	$(CC) -shared $(CFLAGS) $(GTK_CFLAGS) $(GOOCANVOBJ) $(GTKLIBS) -lm -lmingwex \
	-Wl,--enable-auto-image-base -Wl,--out-implib,libgoocanvas.lib -o libgoocanvas.dll

libsgpsdp.dll:  $(SGPSDPOBJ)
	$(CC) -shared $(CFLAGS) $(GTK_CFLAGS) $(SGPSDPOBJ) $(GLIBLIB) -lm -lmingwex \
	-Wl,--enable-auto-image-base -Wl,--out-implib,libsgpsdp.lib -o libsgpsdp.dll

clean:
	rm -f *.o *.d *.def *.a *.lib *.dll *.exe



