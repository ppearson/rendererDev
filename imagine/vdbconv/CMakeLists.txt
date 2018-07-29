cmake_minimum_required (VERSION 2.6)
project (vdbconv)

enable_language(CXX)
include(ExternalProject)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++11 -mfpmath=sse -fPIC -ffast-math -fvisibility-inlines-hidden -fvisibility=hidden")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -std=c++11 -mfpmath=sse -fPIC -ffast-math")

SET(OPENVDB_DIR "/opt/OpenVDB")
#SET(BOOST_DIR "")
#SET(TBB_DIR "")
#SET(ILMBASE_DIR "")

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

SET(USE_OWN_OPENEXR 0)

IF (USE_OWN_OPENEXR)
	set(ILMBASE_DIST ${PROJECT_BINARY_DIR}/external/dist/ilmbase)
	set(OPENEXR_DIST ${PROJECT_BINARY_DIR}/external/dist/openexr)

	SET(OPENEXR_SHARED_LIBS_FLAGS "")
	IF (APPLE)
		# Because we can't get DYLD_LIBRARY_PATH via LLDB any more, we have to always link statically
		SET(OPENEXR_SHARED_LIBS_FLAGS "-DBUILD_SHARED_LIBS=OFF")
	ENDIF(APPLE)

	# IlmBase / OpenEXR
	set(ILMBASE_VERSION 2.0.1)
	ExternalProject_Add(ILMBASE_LOCAL
		URL ${CMAKE_SOURCE_DIR}/external/ilmbase-${ILMBASE_VERSION}.tar.bz2
		BINARY_DIR external/build/ilmbase
		INSTALL_DIR external/dist/ilmbase
		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external/dist/ilmbase ${OPENEXR_SHARED_LIBS_FLAGS}
	    CMAKE_CACHE_ARGS "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
	)

	set(OPENEXR_VERSION 2.0.1)
	ExternalProject_Add(OPENEXR_LOCAL
		URL ${CMAKE_SOURCE_DIR}/external/openexr-${OPENEXR_VERSION}.tar.bz2
		BINARY_DIR external/build/openexr
		INSTALL_DIR ${OPENEXR_DIST}
		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${OPENEXR_DIST}
		CMAKE_ARGS -DILMBASE_PACKAGE_PREFIX=${ILMBASE_DIST}
		CMAKE_ARGS  ${OPENEXR_SHARED_LIBS_FLAGS}
	    CMAKE_CACHE_ARGS "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
	)

	ADD_DEPENDENCIES(OPENEXR_LOCAL ILMBASE_LOCAL)

	#

	set(ILMBASE_LIB ${ILMBASE_DIST}/lib)
	set(OPENEXR_LIB ${OPENEXR_DIST}/lib)
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		# Link statically
		set(EXTERNAL_OPENEXR_LIBRARIES ${ILMBASE_LIB}/libHalf.a ${ILMBASE_LIB}/libIex-2_0.a ${ILMBASE_LIB}/libIlmThread-2_0.a ${ILMBASE_LIB}/libImath-2_0.a ${OPENEXR_LIB}/libImf-2_0.a)
	ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(EXTERNAL_OPENEXR_LIBRARIES ${ILMBASE_LIB}/libHalf.so ${ILMBASE_LIB}/libIex-2_0.so ${ILMBASE_LIB}/libIlmThread-2_0.so ${ILMBASE_LIB}/libImath-2_0.so ${OPENEXR_LIB}/libImf-2_0.so)
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	set(EXTERNAL_INCLUDE_DIRS ${ILMBASE_DIST}/include/OpenEXR ${OPENEXR_DIST}/include/OpenEXR)
	set(EXTERNAL_COMPILE_FLAGS "")
	set(EXTERNAL_LINK_FLAGS "")
	set(EXTERNAL_LIBRARY_DIRS ${ILMBASE_DIST}/lib ${OPENEXR_DIST}/lib)
	set(EXTERNAL_LIBRARIES ${EXTERNAL_OPENEXR_LIBRARIES})
ELSE()
	set(EXTERNAL_INCLUDE_DIRS "/usr/include/OpenEXR")
	set(EXTERNAL_LIBRARIES "Half")
ENDIF(USE_OWN_OPENEXR)

#include_directories(src)
FILE(GLOB_RECURSE vdbc_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

include_directories("${OPENVDB_DIR}/include")
#include_directories("${BOOST_DIR}/include")
#include_directories("${TBB_DIR}/include")
#include_directories("${ILMBASE_DIR}/include")
include_directories(${EXTERNAL_INCLUDE_DIRS})

LINK_DIRECTORIES("${OPENVDB_DIR}/lib")
#LINK_DIRECTORIES("${BOOST_DIR}/lib")
#LINK_DIRECTORIES("${ILMBASE_DIR}/lib")
#LINK_DIRECTORIES("${TBB_DIR}/lib")
LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu")

ADD_EXECUTABLE(vdbconv MACOSX_BUNDLE ${vdbc_SOURCES})

TARGET_LINK_LIBRARIES(vdbconv "openvdb" "tbb" ${EXTERNAL_LIBRARIES})

