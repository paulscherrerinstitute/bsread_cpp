cmake_minimum_required(VERSION 3.10)
project(lib_bsread)

set(CMAKE_CXX_STANDARD 11)

include_directories("//anaconda/include")

add_library(lib_bsread SHARED src/BSDataSenderZmq.cpp src/BSDataSenderZmq.h src/BSDataMessage.cpp src/BSDataMessage.h src/constants.h)