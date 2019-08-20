cmake_minimum_required(VERSION 2.8)
project(media_handling_regression_tests)

add_definitions(-Wall -g3 -O0 -std=c++17)
include_directories(../Include ../ffmpeg)
cmake_policy(SET CMP0015 NEW) # to allow linking by relative paths
link_directories(../build)
file(GLOB SOURCES "*.cpp")
add_executable(mh_regression ${SOURCES})


target_link_libraries(mh_regression -lgtest -lmediaHandling -lavcodec -lavfilter)