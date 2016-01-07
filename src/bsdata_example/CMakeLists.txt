# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.8)
project (bsdata_example)

set(CMAKE_BUILD_TYPE Release)
# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
#add_library (Hello hello.cxx)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
#target_include_directories (Hello PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.


INCLUDE_DIRECTORIES(/ioc/modules/ZMQ/404.0.1/R3.14.12/include/)
LINK_DIRECTORIES(/ioc/modules/ZMQ/404.0.1/R3.14.12/lib/eldk52-e500v2/)


add_executable (bsdata_example ./example.cpp ../json.cc ../json.h ../bsdata.h ../bsdata.cc ../md5.cc)
target_link_libraries(bsdata_example ZMQ rt)
include_directories(bsdata_example ..)


add_executable (bsdata_perf ./performance_test.cpp ../json.cc ../json.h ../bsdata.h ../bsdata.cc ../md5.cc)
target_link_libraries(bsdata_perf ZMQ rt)
include_directories(bsdata_perf ..)


