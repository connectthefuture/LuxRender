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

// path.cpp*
#include "lux.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// PathnIntegrator Declarations
class PathnIntegrator : public SurfaceIntegrator {
public:
	// PathnIntegrator Public Methods
	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *newAlpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene* scene);
	bool NeedAddSampleInRender() {
		return false;
	}
	PathnIntegrator(int md, float cp) { 
			maxDepth = md; continueProbability = cp;
			L.resize(maxDepth+1);
/*			lightPositionOffset = new int[maxDepth];
			lightNumOffset = new int[maxDepth];
			bsdfDirectionOffset = new int[maxDepth];
			bsdfComponentOffset = new int[maxDepth];
			continueOffset = new int[maxDepth];
			outgoingDirectionOffset = new int[maxDepth];
			outgoingComponentOffset = new int[maxDepth];*/
	}
	virtual PathnIntegrator* clone() const; // Lux (copy) constructor for multithreading
	virtual ~PathnIntegrator() {
/*		delete[] lightPositionOffset; delete[] lightNumOffset;
		delete[] bsdfDirectionOffset; delete[] bsdfComponentOffset;
		delete[] continueOffset; delete[] outgoingDirectionOffset;
		delete[] outgoingComponentOffset;*/ }
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// PathnIntegrator Private Data
	int maxDepth, sampleOffset;
	float continueProbability;
/*	int *lightPositionOffset;
	int *lightNumOffset;
	int *bsdfDirectionOffset;
	int *bsdfComponentOffset;
	int *continueOffset;
	int *outgoingDirectionOffset;
	int *outgoingComponentOffset;*/
	vector <int> bufferIds;
	mutable vector <SWCSpectrum> L;
};

}//namespace lux
