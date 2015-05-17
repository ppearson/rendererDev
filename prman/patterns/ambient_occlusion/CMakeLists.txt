cmake_minimum_required (VERSION 2.6)
project (AmbientOcclusion)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
	  "Choose the type of build, options are: None Debug Release"
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
	message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()

SET(CMAKE_CXX_FLAGS_DEBUG "-g -mfpmath=sse -fPIC")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -fPIC")

SET(RMAN_INCLUDE_PATH "/opt/pixar/RenderManProServer-19.0/include")

SET(shader_SOURCES "ambient_occlusion.cpp")

INCLUDE_DIRECTORIES("${RMAN_INCLUDE_PATH}")

ADD_LIBRARY(AmbientOcclusion SHARED ${shader_SOURCES})

SET_TARGET_PROPERTIES(AmbientOcclusion PROPERTIES PREFIX "")


