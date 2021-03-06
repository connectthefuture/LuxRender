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

#include "lux.h"
#include "transport.h"
#include "renderers/sppm/hitpoints.h"
#include "renderers/sppm/photonsampler.h"
#include "renderinghints.h"

namespace lux
{

// A dummy integrator required mainly by internal interfaces and to store some
// rendering parameter

class SPPMIntegrator : public SurfaceIntegrator {
public:
	// SPPMIntegrator Public Methods
	SPPMIntegrator();
	virtual ~SPPMIntegrator();

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

	// Variables used by the hybrid sppm renderer
	string PixelSampler;
	PhotonSamplerType photonSamplerType;
	LookUpAccelType lookupAccelType;

	// double instead of float because photon counters declared as int 64bit
	double photonAlpha;
	float photonStartRadiusScale;
	float parallelHashGridSpare;
	u_int maxEyePathDepth;
	u_int maxPhotonPathDepth;
	u_int hitpointPerPass;
	u_int photonPerPass;
	u_int photonStartK;

	u_int sampleOffset, bufferPhotonId, bufferEyeId;
	bool includeEnvironment;
	bool useproba;

	u_int wavelengthStratification;

	// Few debugging options
	//bool dbg_enableradiusdraw, dbg_enablemsedraw;
	SurfaceIntegratorRenderingHints hints;
	bool directLightSampling;
	bool debug;
	bool storeGlossy;

};

}//namespace lux

