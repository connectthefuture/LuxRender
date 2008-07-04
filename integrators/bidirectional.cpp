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

// bidirectional.cpp*
#include "bidirectional.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"

using namespace lux;

struct BidirVertex {
	BidirVertex() : bsdf(NULL), eBsdf(NULL), pdf(0.f), dAWeight(0.f),
		rr(1.f), pdfR(0.f), dARWeight(0.f), rrR(1.f),
		ePdf(0.f), ePdfDirect(0.f), flags(BxDFType(0)),
		f(0.f), flux(0.f), Le(0.f) {}
	BSDF *bsdf, *eBsdf;
	Point p;
	Normal ng, ns;
	Vector wi, wo;
	float pdf, dAWeight, rr, pdfR, dARWeight, rrR, ePdf, ePdfDirect;
	BxDFType flags;
	SWCSpectrum f, flux, Le;
};

// Bidirectional Method Definitions
void BidirIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	lightNumOffset = sample->Add1D(1);
	lightPosOffset = sample->Add2D(1);
	lightDirOffset = sample->Add2D(1);
	vector<u_int> structure;
	structure.push_back(2);	//light position
	structure.push_back(1);	//light number
	structure.push_back(2);	//bsdf sampling for light
	structure.push_back(1);	//bsdf component for light
	sampleDirectOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	structure.push_back(1);	//continue eye
	structure.push_back(2);	//bsdf sampling for eye path
	structure.push_back(1);	//bsdf component for eye path
	sampleEyeOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	structure.push_back(1); //continue light
	structure.push_back(2); //bsdf sampling for light path
	structure.push_back(1); //bsdf component for light path
	sampleLightOffset = sample->AddxD(structure, maxLightDepth);
}

static int generateEyePath(const Scene *scene, const Ray &r,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices)
{
	RayDifferential ray(r.o, r.d);
	int nVerts = 0;
	while (nVerts < static_cast<int>(vertices.size())) {
		// Find next vertex in path and initialize _vertices_
		BidirVertex &v = vertices[nVerts];
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		Intersection isect;
		++nVerts;
		v.wo = -ray.d;
		if (!scene->Intersect(ray, &isect)) {
			v.p = ray.o;
			v.bsdf = NULL;
			break;
		}
		v.bsdf = isect.GetBSDF(ray, fabsf(2.f * data[3] - 1.f)); // do before Ns is set!
		if (nVerts == 1)
			v.Le = isect.Le(v.wo);
		else
			v.Le = isect.Le(ray, vertices[nVerts - 2].ns, &v.eBsdf, &v.ePdf, &v.ePdfDirect);
		v.p = isect.dg.p;
		v.ng = isect.dg.nn;
		v.ns = v.bsdf->dgShading.nn;
		// Possibly terminate bidirectional path sampling
		if (nVerts == static_cast<int>(vertices.size()))
			break;
		v.f = v.bsdf->Sample_f(v.wo, &v.wi, data[1], data[2], data[3],
			 &v.pdfR, BSDF_ALL, &v.flags, &v.pdf, true);
		if (v.pdfR == 0.f || v.f.Black())
			break;
		v.flux = v.f * (AbsDot(v.wi, v.ns) / v.pdfR);
		v.rrR = min<float>(1.f, v.flux.filter());
		v.rr = min<float>(1.f,
			v.f.filter() * AbsDot(v.wo, v.ns) / v.pdf);
		if (nVerts > 1)
			v.flux *= vertices[nVerts - 2].flux;
		if (nVerts > 3) {
			if (v.rrR < data[0])
				break;
			v.flux /= v.rrR;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wi);
	}
	// Initialize additional values in _vertices_
	for (int i = 0; i < nVerts - 1; ++i) {
		if (vertices[i + 1].bsdf == NULL)
			break;
		vertices[i + 1].dARWeight = vertices[i].pdfR *
			AbsDot(vertices[i + 1].wo, vertices[i + 1].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i].dAWeight = vertices[i + 1].pdf *
			AbsDot(vertices[i].wi, vertices[i].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
	}
	return nVerts;
}

static int generateLightPath(const Scene *scene, BSDF *bsdf,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices)
{
	RayDifferential ray;
	Intersection isect;
	int nVerts = 0;
	while (nVerts < static_cast<int>(vertices.size())) {
		BidirVertex &v = vertices[nVerts];
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		if (nVerts == 0) {
			v.wi = Vector(bsdf->dgShading.nn);
			v.bsdf = bsdf;
			v.p = bsdf->dgShading.p;
			v.ng = bsdf->dgShading.nn;
		} else {
			v.wi = -ray.d;
			v.bsdf = isect.GetBSDF(ray, fabsf(2.f * data[3] - 1.f));
			v.p = isect.dg.p;
			v.ng = isect.dg.nn;
		}
		v.ns = v.bsdf->dgShading.nn;
		++nVerts;
		// Possibly terminate bidirectional path sampling
		if (nVerts == static_cast<int>(vertices.size()))
			break;
		v.f = v.bsdf->Sample_f(v.wi, &v.wo, data[1], data[2], data[3],
			 &v.pdf, BSDF_ALL, &v.flags, &v.pdfR);
		if (v.pdf == 0.f || v.f.Black())
			break;
		v.flux = v.f * AbsDot(v.wo, v.ns) / v.pdf;
		v.rr = min<float>(1.f, v.flux.filter());
		v.rrR = min<float>(1.f,
			v.f.filter() * AbsDot(v.wi, v.ns) / v.pdfR);
		if (nVerts > 1)
			v.flux *= vertices[nVerts - 2].flux;
		if (nVerts > 3) {
			if (v.rrR < data[0])
				break;
			v.flux /= v.rrR;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
		if (!scene->Intersect(ray, &isect))
			break;
	}
	// Initialize additional values in _vertices_
	for (int i = 0; i < nVerts - 1; ++i) {
		vertices[i + 1].dAWeight = vertices[i].pdf *
			AbsDot(vertices[i + 1].wi, vertices[i + 1].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i].dARWeight = vertices[i + 1].pdfR *
			AbsDot(vertices[i].wo, vertices[i].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
	}
	return nVerts;
}

// Connecting factor
static float G(const BidirVertex &eye, const BidirVertex &light)
{
	return AbsDot(eye.ns, eye.wi) * AbsDot(light.ns, light.wo) /
		DistanceSquared(eye.p, light.p);
}

// Visibility test
static bool visible(const Scene *scene, const Point &P0,
	const Point &P1)
{
	return !scene->IntersectP(Ray(P0, P1 - P0, RAY_EPSILON, 1.f - RAY_EPSILON));
}

// Weighting of path with regard to alternate methods of obtaining it
static float weightPath(const vector<BidirVertex> &eye, int nEye, int eyeDepth,
	const vector<BidirVertex> &light, int nLight, int lightDepth,
	float pdfLightDirect, bool isLightDirect)
{
	// Current path weight is 1;
	float weight = 1.f, p = 1.f, pDirect = 0.f;
	if (nLight == 1) {
		pDirect = p * pdfLightDirect / light[0].dAWeight;
		// Unconditional add since the last eye vertex can't be specular
		// otherwise the connectio wouldn't have been possible
		// and weightPath wouldn't have been called
		weight += pDirect;
		// If the light is unidirectional the path can only be obtained
		// by sampling the light directly
		if (isLightDirect && (light[0].flags & BSDF_SPECULAR) != 0)
			weight -= 1.f;
	}
	// Find other paths by extending light path toward eye path
	for (int i = nEye - 1; i >= max(0, nEye - (lightDepth - nLight)); --i) {
		// Exit if the path is impossible
		if (!(eye[i].dARWeight > 0.f))
			break;
		// Check for direct path
		// only possible if the eye path hit a light
		// So the eye path has at least 2 vertices
		if (nLight + nEye - i == 1) {
			pDirect = p * pdfLightDirect / eye[i].dARWeight;
			// The light vertex cannot be specular otherwise
			// the eye path wouldn't have received any light
			if (i > 0 && (eye[i - 1].flags & BSDF_SPECULAR) == 0)
				weight += pDirect;
		}
		// Exit if the path is impossible
		if (!(eye[i].dAWeight > 0.f))
			break;
		p *= eye[i].dAWeight / eye[i].dARWeight;
		if (i > 3)
			p /= eye[i - 1].rrR;
		if (nLight + nEye - i > 3) {
			if (i < nEye - 1)
				p *= eye[i + 1].rr;
			else if (nLight > 0)
				p *= light[nLight - 1].rr;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((eye[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (eye[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p;
	}
	// Reinitialize p to search paths in the other direction
	p = 1.f;
	// Find other paths by extending eye path toward light path
	for (int i = nLight - 1; i >= max(0, nLight - (eyeDepth - nEye)); --i) {
		// Exit if the path is impossible
		if (!(light[i].dARWeight > 0.f))
			break;
		// Check for direct path
		// light path has at least 2 remaining vertices here
		if (i == 1) {
			pDirect = p * light[i].dARWeight / pdfLightDirect;
			// The path can only be obtained if none of the vertices
			// is specular
			if ((light[i].flags & BSDF_SPECULAR) == 0 &&
				(light[i - 1].flags & BSDF_SPECULAR) == 0)
				weight += pDirect;
		}
		// Exit if the path is impossible
		if (!(light[i].dAWeight > 0.f))
			break;
		p *= light[i].dARWeight / light[i].dAWeight;
		if (i > 3)
			p /= light[i - 1].rr;
		if (nEye + nLight - i > 3) {
			if (i < nLight - 1)
				p *= light[i + 1].rrR;
			else if (nEye > 0)
				p *= eye[nEye - 1].rrR;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((light[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (light[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p;
	}
	// The weight is computed for normal lighting,
	// if we have direct lighting, compensate for it
	if (isLightDirect && pDirect > 0.f)
		weight /= pDirect;
	return weight;
}

static SWCSpectrum evalPath(const Scene *scene,
	vector<BidirVertex> &eye, int nEye, int eyeDepth,
	vector<BidirVertex> &light, int nLight, int lightDepth,
	float pdfLightDirect, bool isLightDirect)
{
	SWCSpectrum L;
	float eWeight, lWeight;
	// Be carefull, eye and light last vertex can be modified here
	// If each path has at least 1 vertex, connect them
	if (nLight > 0 && nEye > 0) {
		BidirVertex &eyeV(eye[nEye - 1]);
		BidirVertex &lightV(light[nLight - 1]);
		// Prepare eye vertex for connection
		eyeV.wi = Normalize(lightV.p - eyeV.p);
		eyeV.pdf = eyeV.bsdf->Pdf(eyeV.wi, eyeV.wo);
		eyeV.pdfR = eyeV.bsdf->Pdf(eyeV.wo, eyeV.wi);
		if (!(eyeV.pdfR > 0.f))
			return 0.f;
		eyeV.f = eyeV.bsdf->f(eyeV.wi, eyeV.wo);
		if (eyeV.f.Black())
			return 0.f;
		eyeV.rr = min<float>(1.f, eyeV.f.filter() *
			AbsDot(eyeV.ns, eyeV.wo) / eyeV.pdf);
		eyeV.rrR = min<float>(1.f, eyeV.f.filter() *
			AbsDot(eyeV.ns, eyeV.wi) / eyeV.pdfR);
		eyeV.flux = eyeV.f; // No pdf as it is a direct connection
		if (nEye > 1)
			eyeV.flux *= eye[nEye - 2].flux;
		// Prepare light vertex for connection
		lightV.wo = -eyeV.wi;
		lightV.pdf = lightV.bsdf->Pdf(lightV.wi, lightV.wo);
		lightV.pdfR = lightV.bsdf->Pdf(lightV.wo, lightV.wi);
		if (!(lightV.pdf > 0.f))
			return 0.f;
		lightV.f = lightV.bsdf->f(lightV.wi, lightV.wo);
		if (lightV.f.Black())
			return 0.f;
		lightV.rr = min<float>(1.f, lightV.f.filter() *
			AbsDot(lightV.ns, lightV.wo) / lightV.pdf);
		lightV.rrR = min<float>(1.f, lightV.f.filter() *
			AbsDot(lightV.ns, lightV.wi) / lightV.pdfR);
		lightV.flux = lightV.f; // No pdf as it is a direct connection
		if (nLight > 1)
			lightV.flux *= light[nLight - 2].flux;
		// Connect eye and light vertices
		// The occlusion must have been checked before
		L = eyeV.flux * lightV.flux * G(eyeV, lightV);
		if (L.Black())
			return 0.f;
		// Evaluate factors for path weighting
		lightV.dARWeight = eyeV.pdfR * AbsDot(lightV.ns, lightV.wo) /
			DistanceSquared(eyeV.p, lightV.p);
/*		if (nEye > 3)
			lightV.dARWeight *= eyeV.rrR;*/
		if (nLight > 1) {
			lWeight = light[nLight - 2].dARWeight;
			light[nLight - 2].dARWeight = lightV.pdfR *
				AbsDot(light[nLight - 2].ns, light[nLight - 2].wo) /
				DistanceSquared(lightV.p, light[nLight - 2].p);
/*			if (nEye + 1 > 3)
				light[nLight - 2].dARWeight *= lightV.rrR;*/
			lightV.dAWeight = light[nLight - 2].pdf *
				AbsDot(lightV.ns, lightV.wi) /
				DistanceSquared(light[nLight - 2].p, lightV.p);
/*			if (nLight - 1 > 3)
				lightV.dAWeight *= light[nLight - 2].rr;*/
		}
		lightV.flags = BxDFType(~BSDF_SPECULAR);
		eyeV.dAWeight = lightV.pdf * AbsDot(eyeV.ns, eyeV.wi) /
			DistanceSquared(lightV.p, eyeV.p);
/*		if (nLight > 3)
			eyeV.dAWeight *= lightV.rr;*/
		if (nEye > 1) {
			eWeight = eye[nEye - 2].dAWeight;
			eye[nEye - 2].dAWeight = eyeV.pdf *
				AbsDot(eye[nEye - 2].ns, eye[nEye - 2].wi) /
				DistanceSquared(eyeV.p, eye[nEye - 2].p);
/*			if (nLight + 1 > 3)
				eye[nEye - 2].dAWeight *= eyeV.rr;*/
			eyeV.dARWeight = eye[nEye - 2].pdfR *
				AbsDot(eyeV.ns, eyeV.wo) /
				DistanceSquared(eye[nEye - 2].p, eyeV.p);
/*			if (nEye - 1 > 3)
				eyeV.dARWeight *= eye[nEye - 2].rrR;*/
		}
		eyeV.flags = BxDFType(~BSDF_SPECULAR);
	} else if (nEye > 1) { // Evaluate eye path without light path
		BidirVertex &v(eye[nEye - 1]);
		L = eye[nEye - 2].flux;
		float distance2 = DistanceSquared(v.p, eye[nEye - 2].p);
		// Evaluate factors for path weighting
		v.dAWeight = v.ePdf;
		v.pdf = v.eBsdf->Pdf(Vector(v.ns), v.wo);
		eWeight = eye[nEye - 2].dAWeight;
		eye[nEye - 2].dAWeight = v.pdf *
			AbsDot(eye[nEye - 2].ns, eye[nEye - 2].wi) /
			distance2;
		v.dARWeight = eye[nEye - 2].pdfR * AbsDot(v.ns, v.wo) /
			distance2;
/*		if (nEye - 1 > 3)
			v.dARWeight *= eye[nEye - 2].rrR;*/
		v.flags = BxDFType(~BSDF_SPECULAR);
	} else if (nLight > 1) { // Evaluate light path without eye path
		BidirVertex &v(light[nLight - 1]);
		L = light[nLight - 2].flux;//FIXME: not implemented yet
		float distance2 = DistanceSquared(v.p, light[nLight - 2].p);
		// Evaluate factors for path weighting
		v.dARWeight = v.ePdf;
		v.pdfR = v.eBsdf->Pdf(Vector(v.ns), v.wi);
		lWeight = light[nLight - 2].dARWeight;
		light[nLight - 2].dARWeight = v.pdfR *
			AbsDot(light[nLight - 2].ns, light[nLight - 2].wo) /
			distance2;
		v.dAWeight = light[nLight - 2].pdf * AbsDot(v.ns, v.wi) /
			distance2;
/*		if (nLight - 1 > 3)
			v.dAWeight *= light[nLight - 2].rr;*/
		v.flags = BxDFType(~BSDF_SPECULAR);
	}
	else
		return 0.f;
	L /= weightPath(eye, nEye, eyeDepth, light, nLight, lightDepth,
		pdfLightDirect, isLightDirect);
	if (nEye > 1)
		eye[nEye - 2].dAWeight = eWeight;
	if (nLight > 1)
		light[nLight - 2].dARWeight = lWeight;
	return L;
}

static bool getLightHit(const Scene *scene, vector<BidirVertex> &eyePath,
	int length, int eyeDepth, int lightDepth, SWCSpectrum *Le)
{
	BidirVertex &v(eyePath[length - 1]);
	if (v.Le.Black())
		return false;
	*Le = v.Le;
	if (length == 1) //FIXME
		return true;
	if (!(v.ePdf > 0.f))
		return false;
	vector<BidirVertex> path(0);
	BidirVertex e(v);
	*Le *= evalPath(scene, eyePath, length, eyeDepth, path, 0, lightDepth,
		v.ePdfDirect, false);
	v = e;
	return !Le->Black();
}

static bool getEnvironmentLight(const Scene *scene,
	vector<BidirVertex> &eyePath, int length, int eyeDepth, int lightDepth,
	SWCSpectrum *Le)
{
	BidirVertex &v(eyePath[length - 1]);
	if (v.bsdf)
		return false;
	if (length == 1) { //FIXME
		for (unsigned int lightNumber = 0; lightNumber < scene->lights.size(); ++lightNumber) {
			const Light *light = scene->lights[lightNumber];
			*Le += light->Le(RayDifferential(v.p, -v.wo));
		}
		return true;
	}
	// The eye path has at least 2 vertices
	vector<BidirVertex> path(0);
	for (unsigned int lightNumber = 0; lightNumber < scene->lights.size(); ++lightNumber) {
		const Light *light = scene->lights[lightNumber];
		v.Le = light->Le(scene, RayDifferential(eyePath[length - 2].p,
				eyePath[length - 2].wi), eyePath[length - 2].ns,
			&v.eBsdf, &v.ePdf, &v.ePdfDirect);
		if (v.eBsdf == NULL || !(v.ePdf > 0.f) || v.Le.Black())
			continue;
		v.p = v.eBsdf->dgShading.p;
		v.ns = v.eBsdf->dgShading.nn;
		v.Le *= evalPath(scene, eyePath, length, eyeDepth, path, 0,
			lightDepth, v.ePdfDirect, false);
		if (!v.Le.Black())
			*Le += v.Le;
	}
	return true;
}

static bool getDirectLight(const Scene *scene, vector<BidirVertex> &eyePath,
	int length, int eyeDepth, int lightDepth, const Light *light,
	float u0, float u1, float portal, SWCSpectrum *Ld)
{
	vector<BidirVertex> lightPath(1);
	BidirVertex &vE(eyePath[length - 1]);
	BidirVertex &vL(lightPath[0]);
	VisibilityTester visibility;
	// Sample the chosen light
	*Ld = light->Sample_L(scene, vE.p, vE.ns, u0, u1, portal, &vL.bsdf,
		&vL.dAWeight, &vL.ePdfDirect, &visibility);
	// Test the visibility of the light
	if (!(vL.ePdfDirect > 0.f) || Ld->Black() || !visibility.Unoccluded(scene))
		return false;
	BidirVertex e(vE);
	vL.p = vL.bsdf->dgShading.p;
	vL.ns = vL.bsdf->dgShading.nn;
	vL.wi = Vector(vL.ns);
	*Ld *= evalPath(scene, eyePath, length, eyeDepth,
		lightPath, 1, lightDepth, vL.ePdfDirect, true);
	*Ld /= vL.ePdfDirect;
	vE = e;
	return !Ld->Black();
}

static bool getLight(const Scene *scene,
	vector<BidirVertex> &eyePath, int nEye, int eyeDepth,
	vector<BidirVertex> &lightPath, int nLight, int lightDepth,
	float lightDirectPdf, SWCSpectrum *Ll)
{
	if (!visible(scene, eyePath[nEye - 1].p, lightPath[nLight - 1].p))
		return false;
	BidirVertex vE(eyePath[nEye - 1]), vL(lightPath[nLight - 1]);
	*Ll *= evalPath(scene, eyePath, nEye, eyeDepth,
		lightPath, nLight, lightDepth, lightDirectPdf, false);
	eyePath[nEye - 1] = vE;
	lightPath[nLight - 1] = vL;
	return !Ll->Black();
}

SWCSpectrum BidirIntegrator::Li(const Scene *scene, const RayDifferential &ray,
	const Sample *sample, float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth);
	SWCSpectrum L(0.f);
	int numberOfLights = static_cast<int>(scene->lights.size());
	// Trace eye path
	int nEye = generateEyePath(scene, ray, sample, sampleEyeOffset, eyePath);
	// Light path cannot intersect camera (FIXME)
	eyePath[0].dARWeight = 0.f;
	if (nEye == 0) {//FIXME cannot happen
		for (int i = 0; i < numberOfLights; ++i)
			L += scene->lights[i]->Le(ray);
		XYZColor color(L.ToXYZ());
		if (color.Black())
			*alpha = 0.f;
		if (color.y() > 0.f)
			sample->AddContribution(sample->imageX, sample->imageY,
				color, *alpha);
		return L;
	}
	// Choose light
	int lightNum = Floor2Int(sample->oneD[lightNumOffset][0] *
		numberOfLights);
	lightNum = min<int>(lightNum, numberOfLights - 1);
	Light *light = scene->lights[lightNum];
	const float *data = sample->twoD[lightPosOffset];
	BSDF *lightBsdf;
	float lightPdf;
	SWCSpectrum Le = light->Sample_L(scene, data[0], data[1], &lightBsdf,
		&lightPdf);
	int nLight = 0;
	if (lightPdf == 0.f)
		Le = 0.f;
	else {
		Le *= numberOfLights / lightPdf;
		nLight = generateLightPath(scene, lightBsdf, sample,
			sampleLightOffset, lightPath);
	}
	// Give the light point probability for the weighting if the light
	// is not delta
	if (lightPdf > 0.f && !light->IsDeltaLight())
		lightPath[0].dAWeight = lightPdf;
	else
		lightPath[0].dAWeight = 0.f;
	float lightDirectPdf = 0.f;
	if (nLight > 1)
		lightDirectPdf = light->Pdf(lightPath[1].p, lightPath[1].ns,
			lightPath[1].wi) / //FIXME
			DistanceSquared(lightPath[1].p, lightPath[0].p) *
			AbsDot(lightPath[0].ns, lightPath[0].wo);
	// Connect paths
	for (int i = 1; i <= nEye; ++i) {
		// Check eye path light intersection
		SWCSpectrum Le(0.f);
		if (getEnvironmentLight(scene, eyePath, i, maxEyeDepth,
			maxLightDepth, &Le)) {
			L += Le;
			break; //from now on the eye path does not intersect anything
		} else if (getLightHit(scene, eyePath, i, maxEyeDepth,
			maxLightDepth, &Le))
			L += Le;
		// Do direct lighting
		SWCSpectrum Ld(0.f);
		data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, i);
		switch(lightStrategy) {
			case SAMPLE_ONE_UNIFORM: {
				float portal = data[2] * numberOfLights;
				const int lightDirectNumber = min(Floor2Int(portal), numberOfLights - 1);
				portal -= lightDirectNumber;
				if (getDirectLight(scene, eyePath, i,
					maxEyeDepth, maxLightDepth,
					scene->lights[lightDirectNumber],
					data[0], data[1], portal, &Ld)) {
					Ld *= numberOfLights;
					L += Ld;
				}
				break;
			}
			case SAMPLE_ALL_UNIFORM:
				for (u_int l = 0; l < scene->lights.size(); ++l) {
					if (getDirectLight(scene, eyePath, i,
						maxEyeDepth, maxLightDepth,
						scene->lights[l],
						data[0], data[1], data[2], &Ld))
						L += Ld;
				}
				break;
			default:
				break;
		}
		// Compute direct lighting pdf for first light vertex link
		float directPdf = light->Pdf(eyePath[i - 1].p, eyePath[i - 1].ns,
			Normalize(lightPath[0].p - eyePath[i - 1].p)) / //FIXME
			DistanceSquared(eyePath[i - 1].p, lightPath[0].p) *
			AbsDot(lightPath[0].ns, lightPath[0].wo);
		// Connect to light subpath
		for (int j = 1; j <= nLight; ++j) {
			SWCSpectrum Ll(Le);
			if (getLight(scene, eyePath, i, maxEyeDepth,
				lightPath, j, maxLightDepth, directPdf, &Ll)) {
				Ll *= Le;
				L += Ll;
			}
			// Use general direct lighting pdf for next events
			if (j == 1)
				directPdf = lightDirectPdf;
		}
	}
	XYZColor color(L.ToXYZ());
	if (color.y() > 0.f)
		sample->AddContribution(sample->imageX, sample->imageY,
			color, *alpha);
	return L;
}

SurfaceIntegrator* BidirIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int eyeDepth = params.FindOneInt("eyedepth", 8);
	int lightDepth = params.FindOneInt("lightdepth", 8);
	LightStrategy estrategy;
	string st = params.FindOneString("strategy", "auto");
	if (st == "one") estrategy = SAMPLE_ONE_UNIFORM;
	else if (st == "all") estrategy = SAMPLE_ALL_UNIFORM;
	else if (st == "auto") estrategy = SAMPLE_AUTOMATIC;
	else {
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		estrategy = SAMPLE_AUTOMATIC;
	}
	return new BidirIntegrator(eyeDepth, lightDepth, estrategy);
}
