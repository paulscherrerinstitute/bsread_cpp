# ProtocolBuffers port
Created 2015-01-08

##Step-by-step guide to porting eldk52 cross compilation of protocol buffers

### protocol buffers 2.4.1 (last version that works) 

1. Fetch the latest version: 
        
        wget https://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz

1. untar/ungzip

        tar xzvf protobuf-2.4.1.tar.gz

1. Configure the protocol buffers to use ELDK52

        ./configure --host=powerpc-linux --prefix=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux CC=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-e500v2-gcc CXX=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-e500v2-g++

 Breakdown of flags passed to ./confiugre script: 
- host: tells the config system that the target system will be of powerpc-linux flavour
- prefix: tells the config system where the system root of target architecture (that is used to find basic libraries like libc/libc++) are located
- CC: is passed directly to generated makefile, it specifies which compiler to use (gcc)
- CXX: same as CC but for (g++) 

4. cross-compile the protocol buffers: 

        make 

note, observing the makefile output you will notice that the eldk52 toolchain is actually beeing used

5. If the compiler finishes without warnings you can test the library by copying it to the appropriate /ioc/ folder and modifying its LD_LIBRARY_PATH. Note that this approach should only be used for testing. For production use the output of ./configure script would have to be wrapped into a driver.makefile makefile and than compiled and installed


### protocol buffers > 2.6 (latest) 

1. Start in the same way as 2.4.1, just fetch the 2.6 version: 

        wget https://protobuf.googlecode.com/svn/rc/protobuf-2.6.0.tar.gz
        tar xzvf protobuf-2.6.0.tar.gz
        cd protobuf-2.6.0.tar.gz
         ./configure --host=powerpc-linux --prefix=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux CC=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-e500v2-gcc CXX=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-    e500v2-g++


1. trying to compile will result in an error: 

        make 

        ./google/protobuf/stubs/platform_macros.h:77:2: error: #error Host architecture was not detected as supported by protobuf 


1. This happens sicne our version of GCC reports powerpc architecture by defining "__powerpc__" and not "__ppc__" as expected by proto bufs (this is a known discrepancy in PowerPC GCC port). In order to fix this, we simply replace line 65 in platform_macros.h with: 

        #elif defined(__powerpc__)


1. Running make again results in the next error: 

        ./google/protobuf/stubs/atomicops.h:198:1: error: stray '#' in program 

This error occurs because the atomic operations for PPC are missing. If eldk would provide a gcc > 4.7.4 we could potentially use the builtin generic atomic operations. 

# Notes

Cross Compile Protocol Buffers for IFC board:

```
./configure \
--host=powerpc-linux \
--prefix=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux \
CFLAGS='-fPIC -g -O0' CXXFLAGS='-fPIC -g -O0' \
CC=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-e500v2-gcc \
CXX=/opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/powerpc-e500v2-g++  \
LDFLAGS='-L/opt/eldk-5.2/powerpc-e500v2/sysroots/ppce500v2-linux-gnuspe/lib'
```

Comment: [Tom] It configures fine, but as mentioned can not be compiled due to missing atomic_ops. Furthermore there is
an issue [1] opened on protobuffers to add support for PPC that was marked by developers as 'wont fix'. So I guess even
if we spend some time and fix the current release we will not have any support down the line. I suggest against it...
[1] https://code.google.com/p/protobuf/issues/detail?id=512
[2] https://code.google.com/p/protobuf/issues/detail?id=488




