/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/  

#ifndef LUX_REGULARSPD_H
#define LUX_REGULARSPD_H
// regular.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "spd.h"

namespace lux
{

// regularly sampled SPD, reconstructed using linear interpolation
class RegularSPD : public SPD {
  public:
    RegularSPD() : SPD() {}

    //  creates a regularly sampled SPD
    //  samples    array of sample values
    //  lambdaMin  wavelength (nm) of first sample
    //  lambdaMax  wavelength (nm) of last sample
    //  n          number of samples
    RegularSPD(const float* const samples, float lambdaMin, float lambdaMax, int n) : SPD() {
      init(lambdaMin, lambdaMax, samples, n);
    }

    ~RegularSPD() {}

  protected:
    void init(float lMin, float lMax, const float* const s, int n);

  private:   
  };

  }//namespace lux

#endif // LUX_REGULARSPD_H
