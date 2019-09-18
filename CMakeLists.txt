cmake_minimum_required(VERSION 2.8.9)
project(mediahandling)
set(CMAKE_BUILD_TYPE Release)

add_definitions(-Wall -Werror -pedantic -O0 -g3 -std=c++17)

include_directories(ffmpeg Include ./)

file(GLOB SOURCES "ffmpeg/*.cpp" "Src/*.cpp" "*.cpp")

add_library(mediaHandling SHARED ${SOURCES})

install(TARGETS mediaHandling DESTINATION /usr/lib)
