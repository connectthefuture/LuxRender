/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#include "osfunc.h"

#ifdef WIN32
#include <windows.h>
#else

#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(__sun)
#include <unistd.h>
#endif

#endif //WIN32

namespace lux
{

// Dade - this code comes from Boost 1.35 and it can be removed once we start to
// use Boost 1.35 instead of 1.34.1

// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
int osHardwareConcurrency() {
#ifdef WIN32
	SYSTEM_INFO info={0};
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#else

#if defined(PTW32_VERSION) || defined(__hpux)
	return pthread_num_processors_np();
#elif defined(__linux__)
	return get_nprocs();
#elif defined(__APPLE__) || defined(__FreeBSD__)
	int count;
	size_t size=sizeof(count);
	return sysctlbyname("hw.ncpu",&count,&size,NULL,0)?0:count;
#elif defined(__sun)
	int const count=sysconf(_SC_NPROCESSORS_ONLN);
	return (count>0)?count:0;
#else
	return 0;
#endif

#endif // WIN32
}

// Dade - used to check and swap bytes in the network rendering code and
// other places
bool osIsLittleEndian() {
    union ShortBytes {
        short shortValue;
        unsigned char bytes[2];
    };

    ShortBytes shortTest;
    shortTest.shortValue = 1;

    return (shortTest.bytes[0] == 1);
}

void osWriteLittleEndianFloat(bool isLittleEndian,
		std::basic_ostream<char> &os, float value) {
    if(isLittleEndian)
        os.write((char *)&value, sizeof(float));
    else {
        union FloatBytes {
            float floatValue;
            unsigned char bytes[4];
        };

        FloatBytes f;
        f.floatValue = value;

        for(unsigned int i = 0; i < sizeof(float); i++)
            os.write((char *)&f.bytes[sizeof(float) - i - 1], 1);
    }
}

void osReadLittleEndianFloat(bool isLittleEndian,
		std::basic_istream<char> &is, float *value) {
    if(isLittleEndian)
        is.read((char *)value, sizeof(float));
    else {
        union FloatBytes {
            float floatValue;
            unsigned char bytes[4];
        };

        FloatBytes f;
        
        for(unsigned int i = 0; i < sizeof(float); i++)
            is.read((char *)&f.bytes[sizeof(float) - i - 1], 1);

        (*value) = f.floatValue;
    }
}

void osWriteLittleEndianInt(bool isLittleEndian,
		std::basic_ostream<char> &os, int value) {
    if(isLittleEndian)
        os.write((char *)&value, sizeof(int));
    else {
        union IntBytes {
            int intValue;
            unsigned char bytes[4];
        };

        IntBytes f;
        f.intValue = value;

        for(unsigned int i = 0; i < sizeof(int); i++)
            os.write((char *)&f.bytes[sizeof(int) - i - 1], 1);
    }
}

void osReadLittleEndianInt(bool isLittleEndian,
		std::basic_istream<char> &is, int *value) {
    if(isLittleEndian)
        is.read((char *)value, sizeof(int));
    else {
        union IntBytes {
            int intValue;
            unsigned char bytes[4];
        };

        IntBytes f;
        
        for(unsigned int i = 0; i < sizeof(int); i++)
            is.read((char *)&f.bytes[sizeof(int) - i - 1], 1);

        (*value) = f.intValue;
    }
}

}//namespace lux