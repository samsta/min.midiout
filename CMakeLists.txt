cmake_minimum_required(VERSION 3.19)
project(Min-Midiout)

if (NOT APPLE)
	message(FATAL_ERROR "Soz, bol, only works for Macs")
endif()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "macOS architecture" FORCE)

set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# Misc setup and subroutines
include(${CMAKE_CURRENT_SOURCE_DIR}/min-devkit/source/min-api/script/min-package.cmake)

# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/min-devkit/source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/min-devkit/source/min-lib)
endif ()

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/min-devkit/source/min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)
find_library(FRAMEWORK_COREMIDI CoreMidi REQUIRED)
find_library(FRAMEWORK_COREFOUNDATION CoreFoundation REQUIRED)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
	"${C74_INCLUDES}"
)

add_library( 
	min.midiout 
	MODULE
	min.midiout.cpp
)

target_link_libraries(
	min.midiout 
	PUBLIC
	${FRAMEWORK_COREMIDI}
	${FRAMEWORK_COREFOUNDATION}
)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)