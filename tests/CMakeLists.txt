cmake_minimum_required(VERSION 2.8)
project(bsread_test)
set(CMAKE_CXX_STANDARD 11)

include(GoogleTest)

include_directories("//anaconda/include")
include_directories("../external")
link_directories("//anaconda/lib")

file(GLOB SOURCES *.cpp)

add_executable(run_tests ${SOURCES})
target_link_libraries(run_tests bsread gtest_main gtest)

gtest_discover_tests(run_tests)