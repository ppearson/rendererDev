cmake_minimum_required (VERSION 2.6)
project (imagine_render)

enable_language(CXX)

#SET(NDK_DIR "/Applications/Nuke6.3v6/Nuke6.3v6.app/Contents/MacOS")
SET(NDK_DIR "/usr/local/Nuke7.0v4")

SET(ARCH "corei7")
SET(SSE_FLAGS "-msse -msse2 -msse3 -mssse3 -msse4")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -mfpmath=sse -fPIC -ffast-math -msse -msse2 -msse3 -mssse3 -msse4")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -fPIC -ffast-math -msse -msse2 -msse3 -mssse3 -msse4")

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

FIND_PACKAGE(ZLIB REQUIRED) # for OpenEXR and compression within Imagine

set(ILMBASE_DIST ${PROJECT_BINARY_DIR}/external/dist/ilmbase)
set(OPENEXR_DIST ${PROJECT_BINARY_DIR}/external/dist/openexr)

SET(OPENEXR_SHARED_LIBS_FLAGS "")

set(ILMBASE_VERSION 2.0.1)
ExternalProject_Add(ILMBASE_LOCAL
	URL ${CMAKE_SOURCE_DIR}/external/ilmbase-${ILMBASE_VERSION}.tar.bz2
	PATCH_COMMAND patch -d ../ILMBASE_LOCAL/IlmThread/ < ${CMAKE_SOURCE_DIR}/external/ilmthread.patch
	BINARY_DIR external/build/ilmbase
	INSTALL_DIR external/dist/ilmbase
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external/dist/ilmbase ${OPENEXR_SHARED_LIBS_FLAGS}
)



#

set(OPENEXR_VERSION 2.0.1)
ExternalProject_Add(OPENEXR_LOCAL
	URL ${CMAKE_SOURCE_DIR}/external/openexr-${OPENEXR_VERSION}.tar.bz2
	BINARY_DIR external/build/openexr
	INSTALL_DIR ${OPENEXR_DIST}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${OPENEXR_DIST}
	CMAKE_ARGS -DILMBASE_PACKAGE_PREFIX=${ILMBASE_DIST}
	CMAKE_ARGS  ${OPENEXR_SHARED_LIBS_FLAGS}
)

ADD_DEPENDENCIES(OPENEXR_LOCAL ILMBASE_LOCAL)

set(ILMBASE_LIB ${ILMBASE_DIST}/lib)
set(OPENEXR_LIB ${OPENEXR_DIST}/lib)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	# As above for OpenEXR, debug on OS X should use shared, but release static libs
	IF (CMAKE_BUILD_TYPE MATCHES "Release")
		set(EXTERNAL_OPENEXR_LIBRARIES ${ILMBASE_LIB}/libHalf.a ${ILMBASE_LIB}/libIex-2_0.a ${ILMBASE_LIB}/libIlmThread-2_0.a ${ILMBASE_LIB}/libImath-2_0.a ${OPENEXR_LIB}/libImf-2_0.a)
	ELSE()
		set(EXTERNAL_OPENEXR_LIBRARIES ${ILMBASE_LIB}/libHalf.dylib ${ILMBASE_LIB}/libIex-2_0.dylib ${ILMBASE_LIB}/libIlmThread-2_0.dylib ${ILMBASE_LIB}/libImath-2_0.dylib ${OPENEXR_LIB}/libImf-2_0.dylib)
	ENDIF(CMAKE_BUILD_TYPE MATCHES "Release")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(EXTERNAL_OPENEXR_LIBRARIES ${ILMBASE_LIB}/libHalf.so ${ILMBASE_LIB}/libIex-2_0.so ${ILMBASE_LIB}/libIlmThread-2_0.so ${ILMBASE_LIB}/libImath-2_0.so ${OPENEXR_LIB}/libImf-2_0.so)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(EXTERNAL_INCLUDE_DIRS ${ILMBASE_DIST}/include/OpenEXR ${OPENEXR_DIST}/include/OpenEXR)
set(EXTERNAL_COMPILE_FLAGS "")
set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARY_DIRS ${ILMBASE_DIST}/lib ${OPENEXR_DIST}/lib)
set(EXTERNAL_LIBRARIES ${EXTERNAL_OPENEXR_LIBRARIES})

# Imagine LIB

ADD_DEFINITIONS(-DIMAGINE_NO_GUI -DIMAGINE_EMBEDDED_MODE)

#SET(IMAGINE_SRC_PATH "${CMAKE_SOURCE_DIR}/../imagine/src")
SET(IMAGINE_SRC_PATH "/home/peter/Coding/Imagine/src")
#SET(IMAGINE_SRC_PATH "/Users/peter/Documents/Coding/imagine/src")

FILE(GLOB_RECURSE imagine_SOURCES_ORIGINAL "${IMAGINE_SRC_PATH}/*.cpp")

#MESSAGE( "${imagine_SOURCES_ORIGINAL}" )

# Remove UI items and other stuff so that we can use Imagine's src directory directly without having
# a modified copy
SET(imagine_SOURCES "")
FOREACH(file ${imagine_SOURCES_ORIGINAL})
	STRING(REGEX MATCH "([/]ui[/]|[/]app[/]|[/]io[/]geometry[/]|[/]io[/]scene[/]|[/]scene_builders[/])" test1 ${file})
	IF(test1)

	ELSE(test1)
		LIST(APPEND imagine_SOURCES ${file})
	ENDIF(test1)
ENDFOREACH(file ${imagine_SOURCES_ORIGINAL})

# remove some other stuff
LIST(REMOVE_ITEM imagine_SOURCES ${IMAGINE_SRC_PATH}/io/geo_reader.cpp ${IMAGINE_SRC_PATH}/io/geometry/geo_reader_abc.cpp
		 ${IMAGINE_SRC_PATH}/io/image/image_reader_jpeg.cpp ${IMAGINE_SRC_PATH}/io/image/image_reader_png.cpp
		${IMAGINE_SRC_PATH}/io/image/image_writer_png.cpp ${IMAGINE_SRC_PATH}/objects/primitives/ocean.cpp
		${IMAGINE_SRC_PATH}/io/geo_helper_obj.cpp ${IMAGINE_SRC_PATH}/io/volume/volume_reader_ivv.cpp)

INCLUDE_DIRECTORIES("${IMAGINE_SRC_PATH}" ${EXTERNAL_INCLUDE_DIRS})
ADD_LIBRARY(imagineCore ${imagine_SOURCES})
ADD_DEPENDENCIES(imagineCore OPENEXR_LOCAL)

## Main Nuke Op

SET(EXTERNAL_LIBRARIES "DDImage")
LINK_DIRECTORIES(${NDK_DIR})

include_directories(src)
include_directories(${NDK_DIR}/include)
FILE(GLOB_RECURSE imagine_render_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

ADD_LIBRARY(imagine_render SHARED ${imagine_render_SOURCES})

ADD_DEPENDENCIES(imagine_render imagineCore)
SET_TARGET_PROPERTIES(imagine_render PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(imagine_render imagineCore ${CMAKE_THREAD_LIBS_INIT})
