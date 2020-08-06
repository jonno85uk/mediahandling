cmake_minimum_required(VERSION 3.5)

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

#FIXME: these find_path never work

find_path(DATE_INCLUDE_DIR dependencies/date)
if ((NOT DATE_INCLUDE_DIR) OR (NOT EXISTS ${DATE_INCLUDE_DIR}))
	message("Unable to find date")
	set(DATE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/date)
	execute_process(COMMAND git submodule update --init -- dependencies/date
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	set(DATE_INCLUDE_DIR ${DATE_DIR}/include/)
endif()

find_path(GSL_INCLUDE_DIR dependencies/gsl-lite)
if ((NOT GSL_INCLUDE_DIR) OR (NOT EXISTS ${GSL_INCLUDE_DIR}))
	message("Unable to find gsl")
	set(GSL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/gsl-lite)
	execute_process(COMMAND git submodule update --init -- dependencies/gsl-lite
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	set(GSL_INCLUDE_DIR ${GSL_DIR}/include/gsl)
endif()

if (WIN32)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	find_path(FMT_INCLUDE_DIR dependencies/fmt)
	if ((NOT FMT_INCLUDE_DIR) OR (NOT EXISTS ${FMT_INCLUDE_DIR}))
		message("Unable to find fmt")
		set(FMT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fmt)
		execute_process(COMMAND git submodule update --init -- dependencies/fmt
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		# This can be built with ninja + clang statically. Fails dynamic build with undefined __udivti3
		# Build in Visual Studio and move the installed items to dependencies include + lib
	endif()
	# Move ffmpeg (zeranoe) static + shared contents to dependencies include + lib
	target_link_directories(mediaHandling 
		PRIVATE
			dependencies/lib)	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(mediaHandling fmtd)
	else()		
		target_link_libraries(mediaHandling fmt)
	endif()
	target_link_libraries(mediaHandling avformat avcodec avutil swscale swresample)
else()
    find_package(ffmpeg REQUIRED)
endif()

target_compile_features(mediaHandling PRIVATE cxx_std_17)

target_include_directories(mediaHandling
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
	dependencies/include
	${GSL_INCLUDE_DIR}
	${DATE_INCLUDE_DIR}
)


install(TARGETS mediaHandling DESTINATION lib)
install(DIRECTORY Include/ DESTINATION include/mediahandling)