SUBDIRS = TR

##noinst_LIBRARIES = libsgp4sdp4.a

AM_CPPFLAGS = $(all_includes) @PACKAGE_CFLAGS@

##libsgp4sdp4_a_METASOURCES = AUTO

##libsgp4sdp4_a_SOURCES = \
##	solar.c \
##	sgp_time.c \
##	sgp_obs.c \
##	sgp_math.c \
##	sgp_in.c \
##	sgp4sdp4.c 

##libsgp4sdp4_a_LDFLAGS = `pkg-config --libs glib-2.0`

noinst_PROGRAMS = test-001 test-002

test_001_SOURCES = \
	solar.c \
	sgp_time.c \
	sgp_obs.c \
	sgp_math.c \
	sgp_in.c \
	sgp4sdp4.c \
	test-001.c

test_001_LDADD = @PACKAGE_LIBS@
##test_001_LDFLAGS = `pkg-config --libs glib-2.0`

test_002_SOURCES = \
	solar.c \
	sgp_time.c \
	sgp_obs.c \
	sgp_math.c \
	sgp_in.c \
	sgp4sdp4.c \
	test-002.c

test_002_LDADD = @PACKAGE_LIBS@
##test_002_LDFLAGS = `pkg-config --libs glib-2.0`

EXTRA_DIST = \
	1_COPYING \
	2_README \
	README \
	sgp4sdp4.c \
	sgp4sdp4.h \
	sgp_in.c \
	sgp_math.c \
	sgp_obs.c \
	sgp_time.c \
	solar.c \
	test-001.c \
	test-001.tle \
	test-002.c \
	test-002.tle


