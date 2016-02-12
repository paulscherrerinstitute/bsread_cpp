TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#-------------------------------------------------------
# Release Debug Symbols

USR_CFLAGS_WIN32 += /Z7 /Zo
USR_CXXFLAGS_WIN32 += /Z7 /Zo
USR_LDFLAGS_WIN32 += /DEBUG /OPT:REF /OPT:ICF
#==================================================
# build a support library

LIBRARY_IOC += bsread

## Fix for NOT
USR_CXXFLAGS_LINUX += -fno-operator-names

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install bsread.dbd into <top>/dbd
DBD += bsread.dbd

# specify all source files to be compiled and added to the library
INC += bsread.h
INC += bsdata.h
INC += zmq.hpp
INC += json.h
INC += md5.h
bsread_SRCS += bsread_registerRecordDeviceDriver.cpp
bsread_SRCS += asub_routines.cc
bsread_SRCS += bsread.cc
bsread_SRCS += bsdata.cc
bsread_SRCS += epics_bsread.cc
bsread_SRCS += json.cc
bsread_SRCS += md5.cc

bsread_LIBS += $(EPICS_BASE_IOC_LIBS)
bsread_SYS_LIBS_Linux += zmq
bsread_LIBS_WIN32 += libzmq



#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

