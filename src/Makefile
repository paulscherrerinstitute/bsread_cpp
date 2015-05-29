TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += bsread

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install bsread.dbd into <top>/dbd
DBD += bsread.dbd

# specify all source files to be compiled and added to the library
bsread_SRCS += asub_routines.cc
bsread_SRCS += bsread.cc
bsread_SRCS += json.cc

bsread_LIBS += $(EPICS_BASE_IOC_LIBS)
bsread_SYS_LIBS += zmq

CXXFLAGS+=-fno-operator-names

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

