/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// single.cpp*
#include "volume.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// SingleScattering Declarations
class NoneScattering : public VolumeIntegrator {
public:
	// SingleScattering Public Methods
	NoneScattering() : VolumeIntegrator() {
		AddStringConstant(*this, "name", "Name of current volume integrator", "none");
	}
	virtual ~NoneScattering() { }

	virtual void Transmittance(const Scene &, const Ray &ray,
		const Sample &sample, float *alpha, SWCSpectrum *const L) const;
	virtual void RequestSamples(Sampler *sampler, const Scene &scene);
	virtual u_int Li(const Scene &, const Ray &ray,
		const Sample &sample, SWCSpectrum *L, float *alpha) const;
	virtual bool Intersect(const Scene &scene, const Sample &sample,
		const Volume *volume, bool scatteredStart, const Ray &ray,
		float u, Intersection *isect, BSDF **bsdf, float *pdf,
		float *pdfBack, SWCSpectrum *L) const;
	// Used to complete intersection data with LuxRays
	virtual bool Intersect(const Scene &scene, const Sample &sample,
		const Volume *volume, bool scatteredStart, const Ray &ray,
		const luxrays::RayHit &rayHit, float u, Intersection *isect,
		BSDF **bsdf, float *pdf, float *pdfBack, SWCSpectrum *L) const;

	static VolumeIntegrator *CreateVolumeIntegrator(const ParamSet &params);
};

}//namespace lux
