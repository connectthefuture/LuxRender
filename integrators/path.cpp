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

// path.cpp*
#include "sampling.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"
#include "dynload.h"
#include "path.h"
#include "context.h"
#include "core/partialcontribution.h"

#include "luxrays/core/geometry/raybuffer.h"

using namespace luxrays;
using namespace lux;

static const u_int passThroughLimit = 10000;

// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sampler *sampler, const Scene &scene)
{
	vector<u_int> structure;
	structure.push_back(2);	// bsdf direction sample for path
	structure.push_back(1);	// bsdf component sample for path
	structure.push_back(1); // scattering
	if (rrStrategy != RR_NONE)
		structure.push_back(1);	// continue sample

	sampleOffset = sampler->AddxD(structure, maxDepth + 1);

	if (enableDirectLightSampling) {
		// This is a bit tricky way to discover the kind of Renderer but otherwise
		// I would have to change the APIs
		if (Context::GetActive()->GetRendererType() == Renderer::HYBRIDSAMPLER_TYPE) {
			structure.clear();
			const u_int shadowRaysCount = hints.GetShadowRaysCount();
			
			// use temporary variable so we don't modify hybridRendererLightStrategy since
			// other threads use it during rendering
			samplingCount = hints.GetSamplingLimit(scene);
			for (u_int j = 0; j < samplingCount; ++j) {
				structure.push_back(1);	// light number sample
				for (u_int i = 0; i <  shadowRaysCount; ++i) {
					structure.push_back(2);	// light position sample
					structure.push_back(1);	// light portal sample
				}
			}

			hybridRendererLightSampleOffset = sampler->AddxD(structure, maxDepth + 1);
		} else {
			// Allocate and request samples for light sampling, RR, etc.
			hints.RequestSamples(sampler, scene, maxDepth + 1);
		}
	}
}

void PathIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera()->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

//------------------------------------------------------------------------------
// SamplerRenderer integrator code
//------------------------------------------------------------------------------

u_int PathIntegrator::Li(const Scene &scene, const Sample &sample) const
{
	u_int nrContribs = 0;
	// Declare common path integration variables
	const SpectrumWavelengths &sw(sample.swl);
	Ray ray;
	float xi, yi;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray, &xi, &yi);

	const float nLights = scene.lights.size();
	const u_int lightGroupCount = scene.lightGroups.size();
	// Direct lighting
	vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
	// Direct lighting samples variance
	vector<float> Vd(lightGroupCount, 0.f);
	SWCSpectrum pathThroughput(1.0f);

	PartialContribution partialContribution(lightGroupCount);

	float VContrib = .1f;
	bool specularBounce = true, specular = true, scattered = false;
	float alpha = 1.f;
	float distance = INFINITY;
	u_int vertexIndex = 0;
	const Volume *volume = NULL;

	for (u_int pathLength = 0; ; ++pathLength) {
		const SWCSpectrum prevThroughput(pathThroughput);
		const float *data = sample.sampler->GetLazyValues(sample,
			sampleOffset, pathLength);
		// Find next vertex of path
		Intersection isect;
		BSDF *bsdf;
		float spdf;
		if (!scene.Intersect(sample, volume, scattered, ray, data[3], &isect,
			&bsdf, &spdf, NULL, &pathThroughput)) {
			pathThroughput /= spdf;
			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
				&Lv, &alpha);
			if (!Lv.Black()) {
				Lv *= prevThroughput;
				partialContribution.Add(sw, Lv, g, VContrib);
				++nrContribs;
			}

			// Stop path sampling since no intersection was found
			// Possibly add horizon in render & reflections
			if (!enableDirectLightSampling || (
					(includeEnvironment || vertexIndex > 0) && specularBounce)) {
				BSDF *ibsdf;
				for (u_int i = 0; i < nLights; ++i) {
					SWCSpectrum Le(pathThroughput);
					if (scene.lights[i]->Le(scene, sample,
						ray, &ibsdf, NULL, NULL, &Le)) {
						partialContribution.Add(sw, Le, scene.lights[i]->group, VContrib);
						++nrContribs;
					}
				}
			}

			// Set alpha channel
			if (vertexIndex == 0)
				alpha = 0.f;
			break;
		}
		scattered = bsdf->dgShading.scattered;
		pathThroughput /= spdf;
		if (vertexIndex == 0) {
			distance = ray.maxt * ray.d.Length();
		}

		SWCSpectrum Lv;
		const u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
			&Lv, &alpha);
		if (!Lv.Black()) {
			Lv *= prevThroughput;
			partialContribution.Add(sw, Lv, g, VContrib);
			++nrContribs;
		}

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce) {
			SWCSpectrum Le(pathThroughput);
			BSDF *ibsdf;
			if (isect.Le(sample, ray, &ibsdf, NULL, NULL, &Le)) {
				partialContribution.Add(sw, Le, isect.arealight->group, VContrib);
				++nrContribs;
			}
		}
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point

		// Estimate direct lighting
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		if (enableDirectLightSampling && (nLights > 0)) {
			for (u_int i = 0; i < lightGroupCount; ++i) {
				Ld[i] = 0.f;
				Vd[i] = 0.f;
			}

			nrContribs += hints.SampleLights(scene, sample, p, n,
				wo, bsdf, pathLength, pathThroughput, Ld, &Vd);

			for (u_int i = 0; i < lightGroupCount; ++i) {
				partialContribution.AddUnFiltered(sw, Ld[i], i, Vd[i] * VContrib);
			}
		}

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (!bsdf->SampleF(sw, wo, &wi, data[0], data[1], data[2], &f,
			&pdf, BSDF_ALL, &flags, NULL, true))
			break;

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			// Possibly terminate the path
			if (vertexIndex > 3) {
				if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
					const float q = min<float>(1.f, f.Filter(sw));
					if (q < data[4])
						break;
					// increase path contribution
					pathThroughput /= q;
				} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
					if (continueProbability < data[4])
						break;
					// increase path contribution
					pathThroughput /= continueProbability;
				}
			}
			++vertexIndex;

			specularBounce = (flags & BSDF_SPECULAR) != 0;
			specular = specular && specularBounce;
		}
		pathThroughput *= f;
		if (!specular)
			VContrib += AbsDot(wi, n) / pdf;

		ray = Ray(p, wi);
		ray.time = sample.realTime;
		volume = bsdf->GetVolume(wi);
	}
	partialContribution.Splat(sw, sample, xi, yi, distance, alpha, bufferId, rayWeight);

	return nrContribs;
}

//------------------------------------------------------------------------------
// DataParallel integrator PathState code
//------------------------------------------------------------------------------

PathState::PathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng) {
	SetState(TO_INIT);

	// Some sampler may have to use the RandomNumber generator in InitSample()
	sample.rng = rng;
	scene.sampler->InitSample(&sample);
	sample.contribBuffer = contribBuffer;
	sample.camera = scene.camera()->Clone();
	sample.realTime = 0.f;

	const u_int lightGroupCount = scene.lightGroups.size();
	L = new SWCSpectrum[lightGroupCount];
	V = new float[lightGroupCount];

	PathIntegrator *pi = (PathIntegrator *)scene.surfaceIntegrator;
	const u_int shadowRaysCount = pi->hints.GetShadowRaysCount() *
		pi->hints.GetSamplingLimit(scene);

	Ld = new SWCSpectrum[shadowRaysCount];
	Vd = new float[shadowRaysCount];
	LdGroup = new u_int[shadowRaysCount];
	lightPdfd = new float[shadowRaysCount];
	bsdfPdfd = new float[shadowRaysCount];
	shadowRay = new Ray[shadowRaysCount];
	currentShadowRayIndex = new u_int[shadowRaysCount];
	shadowVolume = new const Volume *[shadowRaysCount];
}

bool PathState::Init(const Scene &scene) {
	flags = 0;

	// Free BSDF memory from computing image sample value
	sample.arena.FreeAll();

	const bool result = sample.sampler->GetNextSample(&sample);

	// save ray time value
	sample.realTime = sample.camera->GetTime(sample.time);
	// sample camera transformation
	sample.camera->SampleMotion(sample.realTime);

	// Sample new SWC thread wavelengths
	sample.swl.Sample(sample.wavelengths);

	pathLength = 0;
	vertexIndex = 0;
	distance = INFINITY;
	VContrib = .1f;
	volume = NULL;
	SetSpecularBounce(true);
	SetSpecular(true);
	SetScattered(false);

	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		L[i] = 0.f;
		V[i] = 0.f;
	}

	// Mandatory initialization of mint and maxt
	pathRay.mint = MachineEpsilon::E(1.f);
	pathRay.maxt = INFINITY;
	const float eyeRayWeight = sample.camera->GenerateRay(scene, sample, &pathRay, &xi, &yi);
	bouncePdf = 1.f;
	lastBounce = pathRay.o;

	pathThroughput = eyeRayWeight;

	SetState(PathState::EYE_VERTEX);

	return result;
}

void PathState::Free(const Scene &scene) {
	delete[] L;
	delete[] V;
	delete[] Ld;
	delete[] Vd;
	delete[] LdGroup;
	delete[] lightPdfd;
	delete[] bsdfPdfd;
	delete[] shadowRay;
	delete[] currentShadowRayIndex;
	delete[] shadowVolume;
	scene.sampler->FreeSample(&sample);
}

void PathState::Terminate(const Scene &scene, const u_int bufferId,
		const float alpha) {
	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		if (!L[i].Black())
			V[i] /= L[i].Filter(sample.swl);

		sample.AddContribution(xi, yi,
			XYZColor(sample.swl, L[i]), alpha, distance,
			V[i], bufferId, i);
	}
	sample.sampler->AddSample(sample);
	SetState(PathState::TERMINATE);
}

bool PathState::TerminatePath(const Scene &scene, const u_int bufferId,
			const float alpha) {
	// Check I if I have still to trace last direct light rays
	if (tracedShadowRayCount > 0) {
		SetTerminate();
		SetState(PathState::CONTINUE_SHADOWRAY);
		return false;
	} else {
		Terminate(scene, bufferId, alpha);
		return true;
	}
}

//------------------------------------------------------------------------------
// DataParallel integrator PathIntegrator code
//------------------------------------------------------------------------------

SurfaceIntegratorState *PathIntegrator::NewState(const Scene &scene,
		ContributionBuffer *contribBuffer, RandomGenerator *rng) {
	return new PathState(scene, contribBuffer, rng);
}

bool PathIntegrator::GenerateRays(const Scene &,
		SurfaceIntegratorState *s, luxrays::RayBuffer *rayBuffer) {
	PathState *pathState = (PathState *)s;
	const u_int leftSpace = rayBuffer->LeftSpace();

	switch (pathState->GetState()) {
		case PathState::EYE_VERTEX: {
			if (1 > leftSpace)
				return false;

			pathState->currentPathRayIndex = rayBuffer->AddRay(pathState->pathRay);
			break;
		}
		case PathState::NEXT_VERTEX: {
			if (1u + pathState->tracedShadowRayCount > leftSpace)
				return false;

			pathState->currentPathRayIndex = rayBuffer->AddRay(pathState->pathRay);

			for (u_short i = 0; i < pathState->tracedShadowRayCount; ++i) {
				pathState->currentShadowRayIndex[i] = rayBuffer->AddRay(pathState->shadowRay[i]);
			}
			break;
		}
		case PathState::CONTINUE_SHADOWRAY: {
			if (pathState->tracedShadowRayCount > leftSpace)
				return false;

			for (u_short i = 0; i < pathState->tracedShadowRayCount; ++i) {
				pathState->currentShadowRayIndex[i] = rayBuffer->AddRay(pathState->shadowRay[i]);
			}
			break;
		}
		default:
			throw std::runtime_error("Internal error in PathIntegrator::GenerateRays(): unknown path state.");
	}

	return true;
}

void PathIntegrator::BuildShadowRays(const Scene &scene, PathState *pathState, BSDF *bsdf) {
	pathState->tracedShadowRayCount = 0;

	const u_int nLights = scene.lights.size();
	if (!enableDirectLightSampling || nLights == 0 /*||
		(bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) == 0)*/)
		return;

	const float *sampleData = pathState->sample.sampler->GetLazyValues(pathState->sample,
		hybridRendererLightSampleOffset, pathState->pathLength);

	const u_int shadowRaysCount = hints.GetShadowRaysCount();

	for (u_int j = 0; j < samplingCount; ++j) {
		const u_int offset = j * (1 + shadowRaysCount * 3);
		float lc = sampleData[offset];
		float lightSelectionPdf;
		const Light *light = hints.SampleLight(scene, j, &lc,
			&lightSelectionPdf);
		if (!light)
			break;
		lightSelectionPdf *= shadowRaysCount;
		for (u_int i = 0; i < shadowRaysCount; ++i) {
			const u_int offset2 = offset + i * 3 + 1;

			const float *lightSample = &sampleData[offset2];
			const float lightPortal = sampleData[offset2 + 2];

			const Point &p = bsdf->dgShading.p;

			// Trace a shadow ray by sampling the light source
			float lightPdf;
			SWCSpectrum Li;
			BSDF *lightBsdf;
			if (!light->SampleL(scene, pathState->sample, p,
				lightSample[0], lightSample[1],
				lightPortal, &lightBsdf, NULL,
				&lightPdf, &Li))
				continue;
			lightPdf *= lightSelectionPdf;

			const Point &pL(lightBsdf->dgShading.p);
			const Vector wi0(pL - p);
			const float d2 = wi0.LengthSquared();
			const float length = sqrtf(d2);
			const float shadowRayEpsilon = max(MachineEpsilon::E(pL),
				MachineEpsilon::E(length));

			if (length <= 2.f * shadowRayEpsilon)
				continue;
			const Vector wi(wi0 / length);

			const SpectrumWavelengths &sw(pathState->sample.swl);
			Vector wo(-pathState->pathRay.d);

			Li *= lightBsdf->F(sw, Vector(lightBsdf->dgShading.nn),
				-wi, false) / (d2 * lightSelectionPdf);
			Li *= bsdf->F(sw, wi, wo, true) *
				pathState->pathThroughput;

			if (Li.Black())
				continue;
			if (!light->IsDeltaLight()) {
				pathState->lightPdfd[pathState->tracedShadowRayCount] = lightPdf * d2 / AbsDot(wi, lightBsdf->ng);
				pathState->bsdfPdfd[pathState->tracedShadowRayCount] = bsdf->Pdf(sw, wo, wi);
				if (pathState->vertexIndex > 3) {
					if (rrStrategy == RR_EFFICIENCY) // use efficiency optimized RR
						pathState->bsdfPdfd[pathState->tracedShadowRayCount] *= min(1.f, bsdf->F(sw, wi, wo, true).Filter(sw) / pathState->bsdfPdfd[pathState->tracedShadowRayCount]);
					else if (rrStrategy == RR_PROBABILITY) // use normal/probability RR
						pathState->bsdfPdfd[pathState->tracedShadowRayCount] *= continueProbability;
				}
			} else {
				pathState->lightPdfd[pathState->tracedShadowRayCount] = 0.f;
				pathState->bsdfPdfd[pathState->tracedShadowRayCount] = 0.f;
			}

			// Store light's contribution
			pathState->Ld[pathState->tracedShadowRayCount] = Li;
			pathState->Vd[pathState->tracedShadowRayCount] = Li.Filter(sw) * pathState->VContrib;
			pathState->LdGroup[pathState->tracedShadowRayCount] = light->group;

			const float maxt = length - shadowRayEpsilon;
			pathState->shadowRay[pathState->tracedShadowRayCount] = Ray(p, wi, shadowRayEpsilon, maxt, pathState->sample.realTime);
			pathState->shadowVolume[pathState->tracedShadowRayCount] = bsdf->GetVolume(wi);
			++(pathState->tracedShadowRayCount);
		}
	}
}

bool PathIntegrator::NextState(const Scene &scene, SurfaceIntegratorState *s, luxrays::RayBuffer *rayBuffer, u_int *nrContribs) {
	PathState *pathState = (PathState *)s;

	*nrContribs = 0;

	//--------------------------------------------------------------------------
	// Finish direct light sampling
	//--------------------------------------------------------------------------

	const PathState::PathStateType state = pathState->GetState();
	if (((state == PathState::NEXT_VERTEX) ||
		(state == PathState::CONTINUE_SHADOWRAY))) {
		u_short leftShadowRaysToTrace = 0;

		for (u_short i = 0; i < pathState->tracedShadowRayCount; ++i) {
			float spdf = 1.f; // Scene.Connect() requires to initialize the variable
			int result = scene.Connect(pathState->sample, pathState->shadowVolume + i,
				pathState->GetScattered(), false, pathState->shadowRay[i],
				*(rayBuffer->GetRayHit(pathState->currentShadowRayIndex[i])),
				&pathState->Ld[i], &spdf, NULL);
			if (result == 1) {
				if (pathState->lightPdfd[i] > 0.f) {
					const float weight = PowerHeuristic(1, pathState->lightPdfd[i], 1, pathState->bsdfPdfd[i] * spdf);
					pathState->Ld[i] *= weight;
					pathState->Vd[i] *= weight;
				}
				const u_int group = pathState->LdGroup[i];
				pathState->L[group] += pathState->Ld[i];
				pathState->V[group] += pathState->Vd[i];
				++(*nrContribs);
			} else if (result == 0) {
				// I have to continue to trace the ray
				pathState->shadowRay[leftShadowRaysToTrace] = pathState->shadowRay[i];
				pathState->shadowVolume[leftShadowRaysToTrace] = pathState->shadowVolume[i];
				pathState->Ld[leftShadowRaysToTrace] = pathState->Ld[i];
				pathState->LdGroup[leftShadowRaysToTrace] = pathState->LdGroup[i];
				pathState->Vd[leftShadowRaysToTrace] = pathState->Vd[i];
				pathState->lightPdfd[leftShadowRaysToTrace] = pathState->lightPdfd[i];
				pathState->bsdfPdfd[leftShadowRaysToTrace] = pathState->bsdfPdfd[i] * spdf;
				++leftShadowRaysToTrace;
			}
		}

		if (leftShadowRaysToTrace > 0) {
			// I have to continue to trace shadow rays
			if (state == PathState::NEXT_VERTEX)
				pathState->pathRayHit = *(rayBuffer->GetRayHit(pathState->currentPathRayIndex));
			pathState->SetState(PathState::CONTINUE_SHADOWRAY);
			pathState->tracedShadowRayCount = leftShadowRaysToTrace;

			return false;
		}

		if (pathState->GetTerminate()) {
			pathState->Terminate(scene, bufferId);
			return true;
		}
	}

	//--------------------------------------------------------------------------
	// Calculate next step
	//--------------------------------------------------------------------------

	const luxrays::RayHit *rayHit;
	if (state == PathState::CONTINUE_SHADOWRAY)
		rayHit = &(pathState->pathRayHit);
	else
		rayHit = rayBuffer->GetRayHit(pathState->currentPathRayIndex);
	const u_int nLights = scene.lights.size();
	const SpectrumWavelengths &sw(pathState->sample.swl);
	const u_int shadowRaysCount = hints.GetShadowRaysCount();

	const float *data = pathState->sample.sampler->GetLazyValues(pathState->sample,
			sampleOffset, pathState->pathLength);
	BSDF *bsdf;
	Intersection isect;
	float spdf;
	if (!scene.Intersect(pathState->sample, pathState->volume, pathState->GetScattered(),
		pathState->pathRay, *rayHit, data[3], &isect, &bsdf, &spdf, NULL,
		&pathState->pathThroughput)) {
		// Stop path sampling since no intersection was found
		// Possibly add horizon in render & reflections
		if ((includeEnvironment || pathState->vertexIndex > 0)) {
			pathState->pathThroughput /= spdf;
			pathState->bouncePdf *= spdf;
			// Reset ray origin
			pathState->pathRay.o = pathState->lastBounce;
			BSDF *ibsdf;
			for (u_int i = 0; i < nLights; ++i) {
				const Light *light = scene.lights[i].get();
				if (!light->IsEnvironmental())
					continue;
				float pdf;
				SWCSpectrum Le(pathState->pathThroughput);
				if (!light->Le(scene, pathState->sample,
					pathState->pathRay, &ibsdf, NULL, &pdf, &Le))
					continue;
				if (enableDirectLightSampling &&
					!pathState->GetSpecularBounce())
					Le *= PowerHeuristic(1, pathState->bouncePdf, 1, pdf * hints.Pdf(scene, i) * shadowRaysCount * DistanceSquared(pathState->pathRay.o, ibsdf->dgShading.p) / (AbsDot(pathState->pathRay.d, ibsdf->ng)));
				pathState->L[light->group] += Le;
				pathState->V[light->group] += Le.Filter(sw) * pathState->VContrib;
				++(*nrContribs);
			}
		}

		// Set alpha channel
		const float alpha = (pathState->vertexIndex == 0) ? 0.f : 1.f;

		// The path is finished
		pathState->Terminate(scene, bufferId, alpha);

		return true;
	}
	pathState->SetScattered(bsdf->dgShading.scattered);
	pathState->pathThroughput /= spdf;
	pathState->bouncePdf *= spdf;
	if (pathState->vertexIndex == 0)
		pathState->distance = pathState->pathRay.maxt * pathState->pathRay.d.Length();

	// Possibly add emitted light at path vertex
	Vector wo(-pathState->pathRay.d);

	// Reset ray origin
	pathState->pathRay.o = pathState->lastBounce;
	BSDF *ibsdf;
	float pdf;
	SWCSpectrum Le(pathState->pathThroughput);
	if (isect.Le(pathState->sample, pathState->pathRay, &ibsdf, NULL, &pdf,
		&Le)) {
		if (enableDirectLightSampling &&
			!pathState->GetSpecularBounce())
			Le *= PowerHeuristic(1, pathState->bouncePdf, 1, pdf * hints.Pdf(scene, isect.arealight) * shadowRaysCount * DistanceSquared(pathState->pathRay.o, ibsdf->dgShading.p) / (AbsDot(pathState->pathRay.d, ibsdf->ng)));
		pathState->L[isect.arealight->group] += Le;
		pathState->V[isect.arealight->group] += Le.Filter(sw) * pathState->VContrib;
		++(*nrContribs);
	}

	// Check if we have reached the max. path depth
	if (pathState->pathLength == maxDepth) {
		pathState->Terminate(scene, bufferId);
		return true;
	}

	const Point &p = bsdf->dgShading.p;
	const Normal &n = bsdf->dgShading.nn;


	// Direct light sampling, only if there's a non specular component and
	// direct light sampling is enabled
	BuildShadowRays(scene, pathState, bsdf);

	// Sample BSDF to get new path direction
	Vector wi;
	BxDFType flags;
	SWCSpectrum f;
	if (!bsdf->SampleF(sw, wo, &wi, data[0], data[1], data[2], &f,
		&pdf, BSDF_ALL, &flags, NULL, true))
		return pathState->TerminatePath(scene, bufferId);

	if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
		!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
		// Possibly terminate the path
		if (pathState->vertexIndex > 3) {
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min<float>(1.f, f.Filter(sw));
				if (q < data[4])
					return pathState->TerminatePath(scene, bufferId);

				// increase path contribution
				f /= q;
				pdf *= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (continueProbability < data[4])
					return pathState->TerminatePath(scene, bufferId);

				// increase path contribution
				f /= continueProbability;
				pdf *= continueProbability;
			}
		}
		pathState->lastBounce = p;
		pathState->bouncePdf = pdf;
		pathState->SetSpecularBounce((flags & BSDF_SPECULAR) != 0);
		pathState->SetSpecular(pathState->GetSpecular() && pathState->GetSpecularBounce());
		++(pathState->vertexIndex);
	} else
		pathState->bouncePdf *= pdf;

	pathState->pathRay = Ray(p, wi);
	pathState->pathRay.time = pathState->sample.realTime;
	++(pathState->pathLength);
	pathState->pathThroughput *= f;
	if (!pathState->GetSpecular())
		pathState->VContrib += AbsDot(wi, n) / pdf;

	pathState->volume = bsdf->GetVolume(wi);
	pathState->SetState(PathState::NEXT_VERTEX);

	return false;
}

//------------------------------------------------------------------------------
// Integrator parsing code
//------------------------------------------------------------------------------

SurfaceIntegrator* PathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);

	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for plain RR (0.0-1.0)
	RRStrategy rstrategy;
	string rst = params.FindOneString("rrstrategy", "efficiency");
	if (rst == "efficiency") rstrategy = RR_EFFICIENCY;
	else if (rst == "probability") rstrategy = RR_PROBABILITY;
	else if (rst == "none") rstrategy = RR_NONE;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '" << rst <<"' for russian roulette path termination unknown. Using \"efficiency\".";
		rstrategy = RR_EFFICIENCY;
	}
	bool include_environment = params.FindOneBool("includeenvironment", true);
	bool directLightSampling = params.FindOneBool("directlightsampling", true);

	PathIntegrator *pi = new PathIntegrator(rstrategy, max(maxDepth, 0), RRcontinueProb,
			include_environment, directLightSampling);
	// Initialize the rendering hints
	pi->hints.InitParam(params);

	return pi;
}

static DynamicLoader::RegisterSurfaceIntegrator<PathIntegrator> r("path");
