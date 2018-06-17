cmake_minimum_required (VERSION 2.6)
project (Instancer)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
	  "Choose the type of build, options are: None Debug Release"
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
	message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()

IF(${MODERN_GCC} MATCHES "1")
	SET(ARCH "corei7")
	SET(SSE_FLAGS "-msse -msse2 -msse3 -mssse3 -msse4")
ELSE()
	SET(ARCH "core2")
	SET(SSE_FLAGS "-msse -msse2 -msse3 -mssse3 -msse4")
ENDIF(${MODERN_GCC} MATCHES "1")

# -fvisibility-inlines-hidden -fvisibility=hidden

# CFLAGS = -ffunction-sections
# LDFLAGS = -Wl,-gc-sections

SET(CMAKE_CXX_FLAGS_DEBUG "-g -mfpmath=sse -fPIC -ffast-math -msse -msse2 -msse3 -mssse3 -msse4")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -mfpmath=sse -fPIC -ffast-math -msse -msse2 -msse3 -mssse3 -msse4")

SET(KATANA_PLUGIN_API_PATH "/opt/Katana3.0v1/plugin_apis/")

FILE(GLOB_RECURSE katanaAPI_SOURCES_ORIGINAL "${KATANA_PLUGIN_API_PATH}/src/*.cpp")

SET(katanaAPI_SOURCES "")
# strip out stuff we don't want
FOREACH(file ${katanaAPI_SOURCES_ORIGINAL})
	STRING(REGEX MATCH "([/]FnDisplayDriver[/]|[/]FnAsset[/]|[/]FnRender[/]|[/]FnRendererInfo[/]|[/]FnViewer[/])" test1 ${file})
	IF(test1)
	ELSE(test1)
		LIST(APPEND katanaAPI_SOURCES ${file})
	ENDIF(test1)
ENDFOREACH(file ${katanaAPI_SOURCES_ORIGINAL})


INCLUDE_DIRECTORIES("${KATANA_PLUGIN_API_PATH}/include/")

ADD_LIBRARY(katanaAPI ${katanaAPI_SOURCES})


####

## katanaImagineRendererInfo
FILE(GLOB_RECURSE instancer_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

ADD_LIBRARY(Instancer SHARED ${instancer_SOURCES})

ADD_DEPENDENCIES(Instancer katanaAPI)
SET_TARGET_PROPERTIES(Instancer PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(Instancer katanaAPI)

