cmake_minimum_required (VERSION 2.6)
project (vdbconv)

enable_language(CXX)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -fPIC -ffast-math -fvisibility-inlines-hidden -fvisibility=hidden")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
	  "Choose the type of build, options are: None Debug Release"
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
	message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-install_name @executable_path")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#include_directories(src)
FILE(GLOB_RECURSE vdbc_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

include_directories("/opt/OpenVDB/include")

ADD_EXECUTABLE(vdbconv MACOSX_BUNDLE ${vdbc_SOURCES})

LINK_DIRECTORIES("/opt/OpenVDB/lib")

TARGET_LINK_LIBRARIES(vdbconv "/opt/OpenVDB/lib/libopenvdb.so" "/opt/hfs13.0.314/dsolib/libtbb.so.2" "/opt/hfs13.0.314/dsolib/libHalf.so.10")

