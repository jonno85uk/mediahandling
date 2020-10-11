cmake_minimum_required(VERSION 3.5)

if (WIN32)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")

  set(SEARCH_PATHS
      vcpkg/
      build/)

  # if this is found, so are the others 
  find_path(VCPKG_INCLUDE_DIR date/date.h
      PATH_SUFFIXES include
      PATHS ${SEARCH_PATHS}
  )

  find_path(VCPKG_LIB_DIR libavcodec.a
  PATH_SUFFIXES lib
  PATHS ${SEARCH_PATHS}
)

endif (WIN32)

project(mediahandling VERSION 0.0 LANGUAGES CXX)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual -Wconversion -pedantic-errors -g3 -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O2")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options("$<$<CONFIG:RELEASE>:/w;/O2>")
	add_compile_options("$<$<CONFIG:DEBUG>:/Wall;/W4;/Zi;/Od>")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# None
endif()



file(GLOB SOURCES "ffmpeg/*.cpp" "Src/*.cpp" "*.cpp")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
add_library(mediaHandling SHARED ${SOURCES})
target_compile_features(mediaHandling PRIVATE cxx_std_17)

if (WIN32)
  target_link_directories(mediaHandling 
    PRIVATE
      ${VCPKG_LIB_DIR}
  )
  target_link_libraries(mediaHandling avformat avcodec avutil swscale swresample fmtd)

  target_include_directories(mediaHandling
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
      ${VCPKG_INCLUDE_DIR}
  )
else()
  target_link_libraries(mediaHandling avformat avcodec avutil swscale swresample)
    target_include_directories(mediaHandling
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/External/date/include/>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/External/gsl-lite/include/>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
  )
endif (WIN32)


install(TARGETS mediaHandling DESTINATION lib)
install(DIRECTORY Include/ DESTINATION include/mediahandling)