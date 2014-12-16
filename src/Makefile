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


bsdaqSupport_SRCS += asubBsdaq.cc
bsdaqSupport_SRCS += bsdaq.cpp
bsdaqSupport_SRCS += bunchData.pb.cc

bsdaqSupport_SRCS += jsoncpp.cpp

bsdaqSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

## External dependcies jsoncpp, zmq, protobuf
USR_LIBS += zmq protobuf
USR_CXXFLAGS_DEFAULT += -fno-operator-names
USR_CXXFLAGS_DEFAULT += -pedantic


zmq_DIR += /usr/lib/x86_64-linux-gnu
protobuf_DIR = /usr/lib/x86_64-linux-gnu

PROD_IOC = bsdaq
# bsdaq.dbd will be created and installed
DBD += bsdaq.dbd

# bsdaq.dbd will be made up from these files:
bsdaq_DBD += base.dbd
bsdaq_DBD += bsread.dbd

# bsdaq_registerRecordDeviceDriver.cpp derives from bsdaq.dbd
bsdaq_SRCS += bsdaq_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
bsdaq_SRCS_DEFAULT += bsdaqMain.cpp
bsdaq_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#bsdaq_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
bsdaq_LIBS += bsdaqSupport

# Finally link to the EPICS Base libraries
bsdaq_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

