cmake_minimum_required(VERSION 2.8.9)
project(mediahandling)
set(CMAKE_BUILD_TYPE Release)

add_definitions(-Wall -std=c++17)

include_directories(ffmpeg Include)

file(GLOB SOURCES "ffmpeg/*.cpp")

add_library(mediaHandling SHARED ${SOURCES})

install(TARGETS mediaHandling DESTINATION /usr/lib)
