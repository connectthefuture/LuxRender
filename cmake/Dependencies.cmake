###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of Lux.                                             #
#                                                                         #
#   Lux is free software; you can redistribute it and/or modify           #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   Lux is distributed in the hope that it will be useful,                #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
#   Lux website: http://www.luxrender.net                                 #
###########################################################################

#############################################################################
#############################################################################
##########################      Find LuxRays       ##########################
#############################################################################
#############################################################################
IF(APPLE)
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ${OSX_DEPENDENCY_ROOT}/include/LuxRays)
	FIND_LIBRARY(LUXRAYS_LIBRARY libluxrays.a ${OSX_DEPENDENCY_ROOT}/lib/LuxRays)
ELSE(APPLE)
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ../luxrays/include)
	FIND_LIBRARY(LUXRAYS_LIBRARY luxrays ../luxrays/lib)
ENDIF(APPLE)

IF (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)
	MESSAGE(STATUS "LuxRays include directory: " ${LUXRAYS_INCLUDE_DIRS})
	MESSAGE(STATUS "LuxRays library directory: " ${LUXRAYS_LIBRARY})
	INCLUDE_DIRECTORIES(SYSTEM ${LUXRAYS_INCLUDE_DIRS})
ELSE (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)
	MESSAGE(FATAL_ERROR "LuxRays not found.")
ENDIF (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)


#############################################################################
#############################################################################
###########################      Find OpenCL       ##########################
#############################################################################
#############################################################################

if(LUXRAYS_DISABLE_OPENCL)
	SET(OCL_LIBRARY "")
else(LUXRAYS_DISABLE_OPENCL)
	FIND_PATH(OPENCL_INCLUDE_DIRS NAMES CL/cl.hpp OpenCL/cl.hpp PATHS /usr/src/opencl-sdk/include /usr/local/cuda/include)
	FIND_LIBRARY(OPENCL_LIBRARY OpenCL /usr/src/opencl-sdk/lib/x86_64)

	IF (OPENCL_INCLUDE_DIRS AND OPENCL_LIBRARY)
		MESSAGE(STATUS "OpenCL include directory: " ${OPENCL_INCLUDE_DIRS})
		MESSAGE(STATUS "OpenCL library directory: " ${OPENCL_LIBRARY})
		INCLUDE_DIRECTORIES(SYSTEM ${OPENCL_INCLUDE_DIRS})
	ELSE (OPENCL_INCLUDE_DIRS AND OPENCL_LIBRARY)
		MESSAGE(FATAL_ERROR "OpenCL not found, try to compile with LUXRAYS_DISABLE_OPENCL=ON")
	ENDIF (OPENCL_INCLUDE_DIRS AND OPENCL_LIBRARY)
endif(LUXRAYS_DISABLE_OPENCL)


#############################################################################
#############################################################################
###########################      Find OpenGL       ##########################
#############################################################################
#############################################################################

FIND_PACKAGE(OpenGL)
IF (OPENGL_FOUND)
	message(STATUS "OpenGL include directory: " "${OPENGL_INCLUDE_DIRS}")
	message(STATUS "OpenGL library: " "${OPENGL_LIBRARY}")
	INCLUDE_DIRECTORIES(SYSTEM ${OPENGL_INCLUDE_DIRS})
ELSE (OPENGL_FOUND)
	MESSAGE(FATAL_ERROR "OpenGL not found.")
ENDIF (OPENGL_FOUND)


#############################################################################
#############################################################################
###########################      Find BISON       ###########################
#############################################################################
#############################################################################

FIND_PACKAGE(BISON REQUIRED)
IF (NOT BISON_FOUND)
	MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_FOUND)


#############################################################################
#############################################################################
###########################      Find FLEX        ###########################
#############################################################################
#############################################################################

FIND_PACKAGE(FLEX REQUIRED)
IF (NOT FLEX_FOUND)
	MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_FOUND)


#############################################################################
#############################################################################
########################### BOOST LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

IF(APPLE)
	SET(BOOST_ROOT ${OSX_DEPENDENCY_ROOT})
ENDIF(APPLE)
SET(Boost_MINIMUM_VERSION "1.43")
SET(Boost_ADDITIONAL_VERSIONS "1.46.2" "1.46.1" "1.46.0" "1.46" "1.45.0" "1.45" "1.44.0" "1.44" "1.43.0" "1.43")
SET(Boost_COMPONENTS thread program_options filesystem serialization iostreams regex system)
IF(WIN32)
	SET(Boost_COMPONENTS ${Boost_COMPONENTS} zlib)
	SET(Boost_USE_STATIC_LIBS ON)
	SET(Boost_USE_MULTITHREADED ON)
	SET(Boost_USE_STATIC_RUNTIME OFF)
ENDIF(WIN32)

FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS python REQUIRED)

SET(Boost_python_FOUND ${Boost_FOUND})
SET(Boost_python_LIBRARIES ${Boost_LIBRARIES})
SET(Boost_FOUND)
SET(Boost_LIBRARIES)

FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${Boost_COMPONENTS} REQUIRED)

IF(Boost_FOUND)
	MESSAGE(STATUS "Boost library directory: " ${Boost_LIBRARY_DIRS})
	MESSAGE(STATUS "Boost include directory: " ${Boost_INCLUDE_DIRS})

	# Don't use latest boost versions interfaces
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION=2)
	INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find Boost")
ENDIF(Boost_FOUND)


#############################################################################
#############################################################################
###########################   FREEIMAGE LIBRARIES    ########################
#############################################################################
#############################################################################

IF(APPLE)
	SET(FREEIMAGE_ROOT ${OSX_DEPENDENCY_ROOT})
ENDIF(APPLE)
FIND_PACKAGE(FreeImage REQUIRED)

IF(FREEIMAGE_FOUND)
	MESSAGE(STATUS "FreeImage include directory: " ${FREEIMAGE_INCLUDE_DIR})
	MESSAGE(STATUS "FreeImage library: " ${FREEIMAGE_LIBRARIES})
	INCLUDE_DIRECTORIES(SYSTEM ${FREEIMAGE_INCLUDE_DIR})
ELSE(FREEIMAGE_FOUND)
	MESSAGE(FATAL_ERROR "Could not find FreeImage")
ENDIF(FREEIMAGE_FOUND)


#############################################################################
#############################################################################
######################### OPENEXR LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

# !!!!freeimage needs headers from or matched with freeimage !!!!
IF(APPLE)
	SET(OPENEXR_ROOT ${OSX_DEPENDENCY_ROOT})
ENDIF(APPLE)
FIND_PACKAGE(OpenEXR REQUIRED)
IF (OPENEXR_FOUND)
	MESSAGE(STATUS "OpenEXR include directory: " ${OPENEXR_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES(SYSTEM ${OPENEXR_INCLUDE_DIRS})
ELSE(OPENEXR_FOUND)
	MESSAGE(FATAL_ERROR "OpenEXR not found.")
ENDIF(OPENEXR_FOUND)


#############################################################################
#############################################################################
########################### PNG   LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

# !!!!freeimage needs headers from or matched with freeimage !!!!
IF(APPLE)
	SET(PNG_ROOT ${OSX_DEPENDENCY_ROOT})
ENDIF(APPLE)
FIND_PACKAGE(PNG)
IF(PNG_FOUND)
	MESSAGE(STATUS "PNG include directory: " ${PNG_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES(SYSTEM ${PNG_INCLUDE_DIRS})
ELSE(PNG_FOUND)
	MESSAGE(STATUS "Warning : could not find PNG - building without png support")
ENDIF(PNG_FOUND)


#############################################################################
#############################################################################
###########################   ADDITIONAL LIBRARIES   ########################
#############################################################################
#############################################################################

# The OpenEXR library might be accessible from the FreeImage library
# Otherwise add it to the FreeImage library (required by exrio)
TRY_COMPILE(FREEIMAGE_PROVIDES_OPENEXR ${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/cmake/FindFreeImage.cxx
	CMAKE_FLAGS
	"-DINCLUDE_DIRECTORIES:STRING=${OPENEXR_INCLUDE_DIRS}"
	"-DLINK_LIBRARIES:STRING=${FREEIMAGE_LIBRARIES}"
	COMPILE_DEFINITIONS -D__TEST_OPENEXR__)
# The PNG library might be accessible from the FreeImage library
# Otherwise add it to the FreeImage library (required by pngio)
TRY_COMPILE(FREEIMAGE_PROVIDES_PNG ${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/cmake/FindFreeImage.cxx
	CMAKE_FLAGS
	"-DINCLUDE_DIRECTORIES:STRING=${PNG_INCLUDE_DIRS}"
	"-DLINK_LIBRARIES:STRING=${FREEIMAGE_LIBRARIES}"
	COMPILE_DEFINITIONS -D__TEST_PNG__)
IF (NOT FREEIMAGE_PROVIDES_OPENEXR)
	MESSAGE(STATUS "OpenEXR library: " ${OPENEXR_LIBRARIES})
	SET(FREEIMAGE_LIBRARIES ${FREEIMAGE_LIBRARIES} ${OPENEXR_LIBRARIES})
ENDIF(NOT FREEIMAGE_PROVIDES_OPENEXR)
IF (NOT FREEIMAGE_PROVIDES_PNG)
	MESSAGE(STATUS "PNG library: " ${PNG_LIBRARIES})
	SET(FREEIMAGE_LIBRARIES ${FREEIMAGE_LIBRARIES} ${PNG_LIBRARIES})
ENDIF(NOT FREEIMAGE_PROVIDES_PNG)


#############################################################################
#############################################################################
############################ THREADING LIBRARIES ############################
#############################################################################
#############################################################################

FIND_PACKAGE(Threads REQUIRED)
