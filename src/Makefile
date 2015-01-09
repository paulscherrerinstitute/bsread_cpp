TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
# The following switch is required if compiling C++
# gensub modules, as the genSubRecord structure declares
# a member 'unsigned long    not;'
# However, 'not' has been made a non-ansi reserved word
# but can be switch off as such by this compiler switch :)
#USR_CXXFLAGS_DEFAULT += -fno-operator-names
#USR_CXXFLAGS_DEFAULT += -pedantic -I$(TOP)/json-cpp/include/
#=============================
# Build the IOC support library

LIBRARY_IOC += bsdaqSupport


bsreadSupport_SRCS += asub_routines.cc
bsreadSupport_SRCS += bsread.cc
bsreadSupport_SRCS += bunchData.pb.cc

bsreadSupport_SRCS += json.cc

bsreadSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

## External dependcies json, zmq, protobuf
USR_LIBS += zmq protobuf
USR_CXXFLAGS_DEFAULT += -fno-operator-names
USR_CXXFLAGS_DEFAULT += -pedantic


zmq_DIR += /usr/lib/x86_64-linux-gnu
protobuf_DIR = /usr/lib/x86_64-linux-gnu

PROD_IOC = bsread
# bsread.dbd will be created and installed
DBD += bsread.dbd

# bsread.dbd will be made up from these files:
bsread_DBD += base.dbd
bsread_DBD += bsread.dbd

# bsread_registerRecordDeviceDriver.cpp derives from bsread.dbd
##bsread_SRCS += bsread_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
##bsdaq_SRCS_DEFAULT += bsdaqMain.cpp
bsread_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#bsread_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
bsread_LIBS += bsreadSupport

# Finally link to the EPICS Base libraries
bsread_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE