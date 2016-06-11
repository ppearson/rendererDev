cmake_minimum_required (VERSION 2.6)
project (ImagineRender)

enable_language(CXX)

#SET(NDK_DIR "/Applications/Nuke6.3v6/Nuke6.3v6.app/Contents/MacOS")
SET(NDK_DIR "/usr/local/Nuke7.0v4")

SET(ARCH "corei7")
SET(SSE_FLAGS "-msse -msse2 -msse3 -mssse3 -msse4")
#SET(SSE_FLAGS "-msse -msse2 -msse3")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -mfpmath=sse -fPIC -ffast-math ${SSE_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -fPIC -ffast-math ${SSE_FLAGS}")

include(ExternalProject)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release"
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()

FIND_PACKAGE(Threads)


# Imagine LIB

ADD_DEFINITIONS(-DIMAGINE_NO_GUI -DIMAGINE_EMBEDDED_MODE)

#SET(IMAGINE_SRC_PATH "${CMAKE_SOURCE_DIR}/../imagine/src")
SET(IMAGINE_SRC_PATH "/home/peter/Coding/Imagine_part/src")
#SET(IMAGINE_SRC_PATH "/Users/peter/Documents/Coding/imagine/src")

FILE(GLOB_RECURSE imagine_SOURCES "${IMAGINE_SRC_PATH}/*.cpp")


INCLUDE_DIRECTORIES("${IMAGINE_SRC_PATH}")
ADD_LIBRARY(imagineCore ${imagine_SOURCES})
ADD_DEPENDENCIES(imagineCore OPENEXR_LOCAL)

## Main Nuke Op

SET(EXTERNAL_LIBRARIES "DDImage")
LINK_DIRECTORIES(${NDK_DIR})

include_directories(src)
include_directories(${NDK_DIR}/include)
FILE(GLOB_RECURSE imagine_render_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

ADD_LIBRARY(ImagineRender SHARED ${imagine_render_SOURCES})

ADD_DEPENDENCIES(ImagineRender imagineCore)
SET_TARGET_PROPERTIES(ImagineRender PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(ImagineRender imagineCore ${CMAKE_THREAD_LIBS_INIT} ${EXTERNAL_LIBRARIES})
