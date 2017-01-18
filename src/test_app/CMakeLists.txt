# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)

set(CMAKE_BUILD_TYPE Debug)

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
#add_library (Hello hello.cxx)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
#target_include_directories (Hello PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(EPICS_BASE /opt/epics/base)
set(EPICS_ARCH_OS Linux)

LINK_DIRECTORIES(/opt/epics/base/lib/linux-x86_64)

add_executable (test_app ./test_app.cpp ../json.cc ../json.h ../bsread.cc ../bsdata.cc ../md5.cc)
target_link_libraries(test_app zmq Com)

include_directories(bsread_example .. ${EPICS_BASE}/include ${EPICS_BASE}/include/os/${EPICS_ARCH_OS})
