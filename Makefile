
include /ioc/tools/driver.makefile

BUILDCLASSES=Linux
EXCLUDE_VERSIONS=3.13 3.14.8
EXCLUDE_ARCHS=T2 V67 ppc603 ppc405 embeddedlinux-xscale_be 

USR_CXXFLAGS += -fno-operator-names
#USR_LDFLAGS += -lprotobuf
#USR_LDFLAGS += -static -lprotobuf -L/psi/ioc/BSDAQ/PROTOBUF/src/.libs/ -dynami

SOURCES += src/asub_routines.cc
SOURCES += src/bsread.cc
#SOURCES += src/bunchData.pb.cc
SOURCES += src/json.cc


DBD += src/bsread.dbd
