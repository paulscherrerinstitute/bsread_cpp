include /ioc/tools/driver.makefile

BUILDCLASSES=Linux
EXCLUDE_VERSIONS=3.13 3.14.8
EXCLUDE_ARCHS=T2 V67 ppc603 ppc405 embeddedlinux-xscale_be 
#ARCH_FILTER=SL6%
SOURCES += src/asub_routines.cc
SOURCES += src/bsread.cc
SOURCES += src/bsdata.cc
SOURCES += src/epics_bsread.cc
SOURCES += src/json.cc
SOURCES += src/md5.cc

DBDS += src/bsread.dbd

USR_CXXFLAGS += -fno-operator-names
USR_CXXFLAGS += -DDEBUG
USR_CFLAGS += -DDEBUG

PROJECT = bsread

TEMPLATES += templates/bsread.template
TEMPLATES += templates/bsread_sim.template
TEMPLATES += templates/bsread_evr.template


