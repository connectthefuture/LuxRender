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

// path2.cpp*
#include "lux.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// PathIntegrator Declarations
class Path2Integrator : public SurfaceIntegrator {
public:
	// PathIntegrator Public Methods
	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *newAlpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	Path2Integrator(int md, float cp) { 
		maxDepth = md; continueProbability = cp; 
	}
	virtual ~Path2Integrator() { }
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// PathIntegrator Private Data
	int maxDepth, sampleOffset;
	float continueProbability;
};

}//namespace lux
