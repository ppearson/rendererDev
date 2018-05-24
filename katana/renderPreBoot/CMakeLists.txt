cmake_minimum_required (VERSION 2.6)
project (renderPreBoot)

SET(CMAKE_C_FLAGS_RELEASE "-O2")
SET(CMAKE_C_FLAGS_DEBUG "-g")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
	  "Choose the type of build, options are: None Debug Release"
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
	message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()


FILE(GLOB_RECURSE renderPreBoot_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

ADD_EXECUTABLE(renderPreBoot MACOSX_BUNDLE ${renderPreBoot_SOURCES})

#LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu")

