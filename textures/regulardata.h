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

// regulardata.cpp*
#include "lux.h"
#include "texture.h"
#include "regular.h"
#include "paramset.h"

namespace lux
{

// RegularDataTexture Declarations
template <class T>
class RegularDataFloatTexture : public Texture<T> {
public:
	// RegularDataFloatTexture Public Methods
	RegularDataFloatTexture(const T &v) { value = v; }
	T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return value;
	}
private:
	T value;
};

template <class T>
class RegularDataSpectrumTexture : public Texture<T> {
public:
	// RegularDataSpectrumTexture Public Methods
	RegularDataSpectrumTexture(const float &s, const float &e, const int &n, const float *data) {
		SPD = new RegularSPD(data, s, e, n);
	}
	T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, SPD);
	}
	void SetPower(float power, float area) {
		SPD->Scale(power / (area * M_PI * SPD->y()));
	}
private:
	RegularSPD* SPD;
};

class RegularDataTexture
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux
