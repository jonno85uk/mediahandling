cmake_minimum_required(VERSION 3.5)

project(mediahandling VERSION 0.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual -Wconversion -pedantic-errors -g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O2")

file(GLOB SOURCES "ffmpeg/*.cpp" "Src/*.cpp" "*.cpp")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(mediaHandling SHARED ${SOURCES})

find_package(ffmpeg REQUIRED)

target_compile_features(mediaHandling PRIVATE cxx_std_17)

target_include_directories(mediaHandling
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
)

install(TARGETS mediaHandling DESTINATION lib)
install(DIRECTORY Include/ DESTINATION include/mediahandling)
