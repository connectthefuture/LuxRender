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

include(FindPkgMacros)
getenv_path(lux_DEPENDENCIES_DIR)

#######################################################################
# Core dependencies
#######################################################################

# Find FreeImage
FIND_PACKAGE(FreeImage)

IF (FreeImage_FOUND)
	INCLUDE_DIRECTORIES(SYSTEM ${FreeImage_INCLUDE_DIRS})
ENDIF ()

# Find Boost
SET(Boost_USE_STATIC_LIBS       ON)
SET(Boost_USE_MULTITHREADED     ON)
SET(Boost_USE_STATIC_RUNTIME    OFF)
SET(BOOST_ROOT                  "${BOOST_SEARCH_PATH}")
#SET(Boost_DEBUG                 ON)
SET(Boost_MINIMUM_VERSION       "1.43.0")

SET(Boost_ADDITIONAL_VERSIONS "1.46.2" "1.46.1" "1.46" "1.46.0" "1.45" "1.45.0" "1.44" "1.44.0" "1.43" "1.43.0" )

SET(LUX_BOOST_COMPONENTS python thread program_options filesystem serialization iostreams regex system zlib)
FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${LUX_BOOST_COMPONENTS})
IF (NOT Boost_FOUND)
	# Try again with the other type of libs
	IF(Boost_USE_STATIC_LIBS)
		set(Boost_USE_STATIC_LIBS)
	ELSE(Boost_USE_STATIC_LIBS)
		set(Boost_USE_STATIC_LIBS OFF)
	ENDIF(Boost_USE_STATIC_LIBS)
	FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${LUX_BOOST_COMPONENTS})
ENDIF(NOT Boost_FOUND)
FIND_PACKAGE(Boost)

IF (Boost_FOUND)
	INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF (Boost_FOUND)

FIND_PACKAGE(OpenGL)

IF (OPENGL_FOUND)
	INCLUDE_DIRECTORIES(SYSTEM ${OPENGL_INCLUDE_PATH})
ENDIF(OPENGL_FOUND)

SET(OPENCL_ROOT "${OPENCL_SEARCH_PATH}")
FIND_PACKAGE(OpenCL)
# OpenCL
IF (OPENCL_FOUND)
	INCLUDE_DIRECTORIES(SYSTEM ${OPENCL_INCLUDE_PATH})
ENDIF (OPENCL_FOUND)

FIND_PACKAGE(Qt4 4.6 COMPONENTS QtCore QtGui)


SET(Python_ADDITIONAL_VERSIONS 3.2 3.1 3.0 2.9 2.8)
FIND_PACKAGE(PythonLibs)

set (OpenEXR_HOME "d:/ExtProjects/OpenEXR/")
FIND_PACKAGE(OpenEXR)




SET(LUXRAYS_LIBRARY "../luxrays/lib/Release/luxrays")
SET(LUXRAYS_INCLUDE_DIR "../luxrays/include")

#"../FreeImage/Dist/FreeImage"


SET(PNG_INCLUDE_DIR "../png/include")
SET(PNG_LIBRARY "../png/lib/libpng15")

IF (WIN32)
	SET(SYS_LIBRARIES ${SYS_LIBRARIES} "shell32.lib")
ENDIF (WIN32)
