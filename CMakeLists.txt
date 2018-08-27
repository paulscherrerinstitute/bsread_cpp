cmake_minimum_required(VERSION 3.10)
project(bsread)
set(CMAKE_CXX_STANDARD 11)

if(testing_enabled)
    add_subdirectory(tests)
endif()

file(GLOB SOURCES src/*.cpp)
file(GLOB EXTERNAL_SOURCES external/*.c*)

include_directories("//anaconda/include")
include_directories("external")

add_library(external STATIC ${EXTERNAL_SOURCES})

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} external zmq)