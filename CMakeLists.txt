cmake_minimum_required(VERSION 3.10)
project(bsread)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(tests)

file(GLOB SOURCES src/*.cpp)
file(GLOB EXTERNAL_SOURCES external/*.c*)

include_directories("external")
include_directories("//anaconda/include")

link_directories("//anaconda/lib")

add_library(external STATIC ${EXTERNAL_SOURCES})

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} external zmq)