cmake_minimum_required(VERSION 3.10)
project(bsread_test)
set(CMAKE_CXX_STANDARD 11)

include_directories("//anaconda/include")
include_directories("../external")
link_directories("//anaconda/lib")

add_executable(run_tests test_main.cpp)
target_link_libraries(run_tests bsread gtest_main gtest)

add_test(NAME tests COMMAND run_tests)