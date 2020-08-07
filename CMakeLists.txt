cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")


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

# TODO: figure out how to properly find packages
#find_package(date CONFIG REQUIRED)
#find_package(gsl-lite CONFIG REQUIRED)
#find_package(fmt CONFIG REQUIRED)
#find_package(ffmpeg CONFIG REQUIRED)


file(GLOB SOURCES "ffmpeg/*.cpp" "Src/*.cpp" "*.cpp")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
add_library(mediaHandling SHARED ${SOURCES})
target_compile_features(mediaHandling PRIVATE cxx_std_17)

if (WIN32)
	# FIXME: hardcoded directories that depend on building in a subdir 'build'
    target_include_directories(mediaHandling
        PRIVATE
            build/vcpkg_installed/x64-windows/include)

    target_link_directories(mediaHandling 
		PRIVATE
			build/vcpkg_installed/x64-windows/lib)	
    target_link_libraries(mediaHandling avformat avcodec avutil swscale swresample fmt)
endif()

target_include_directories(mediaHandling
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
)


install(TARGETS mediaHandling DESTINATION lib)
install(DIRECTORY Include/ DESTINATION include/mediahandling)