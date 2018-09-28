cmake_minimum_required(VERSION 2.8)
project(bsread)
set(CMAKE_CXX_STANDARD 11)

# Do not include ZMQ tests.
set(ZMQ_BUILD_TESTS OFF)
add_subdirectory(external/zmq/libzmq)

add_subdirectory(tests)

include_directories("external/zmq/")
include_directories("external/zmq/libzmq/include")
include_directories("external")

file(GLOB EXTERNAL_SOURCES external/*.c*)
add_library(external STATIC ${EXTERNAL_SOURCES})

file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} external libzmq-static)