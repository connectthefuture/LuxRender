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

// area.cpp*
#include "light.h"
#include "shape.h"
#include "context.h"
#include "paramset.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "luxrays/core/color/swcspectrum.h"
#include "texture.h"
#include "sphericalfunction.h"
#include "sampling.h"
#include "dynload.h"
#include "queryable.h"

using namespace luxrays;
using namespace lux;

class  UniformAreaBSDF : public BSDF  {
public:
	// UniformAreaBSDF Public Methods
	UniformAreaBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior) :
		BSDF(dgs, ngeom, exterior, interior) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_DIFFUSE)) ==
			(BSDF_REFLECTION | BSDF_DIFFUSE) ? 1U : 0U;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*wiW = CosineSampleHemisphere(u1, u2);
		const float cosi = wiW->z;
		*wiW = LocalToWorld(*wiW);
		const float cosig = Dot(*wiW, ng);
		if (!(cosig > 0.f))
			return false;
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE);
		*pdf = cosi * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(fabsf(cosig * Dot(woW, dgShading.nn) / cosi));
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 &&
			Dot(wiW, ng) > 0.f) {
			const float cosi = Dot(wiW, dgShading.nn);
			if (cosi > 0.f)
				return cosi * INV_PI;
		}
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		const float cosig = Dot(wiW, ng);
		if (NumComponents(flags) == 1 && cosig > 0.f)
			return SWCSpectrum(INV_PI * fabsf(reverse ? Dot(woW, dgShading.nn) : cosig * Dot(woW, dgShading.nn)));
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

protected:
	// UniformAreaBSDF Private Methods
	virtual ~UniformAreaBSDF() { }
};

class  GonioAreaBSDF : public BSDF  {
public:
	// GonioBSDF Public Methods
	GonioAreaBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const SampleableSphericalFunction *func) :
		BSDF(dgs, ngeom, exterior, interior), sf(func) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & BSDF_DIFFUSE) == BSDF_DIFFUSE ? 1U : 0U;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*f_ = sf->SampleF(sw, u1, u2, wiW, pdf);
		*wiW = Normalize(LocalToWorld(*wiW));
		*f_ *= fabsf(Dot(*wiW, ng) * Dot(woW, dgShading.nn) /
			(Dot(*wiW, dgShading.nn) * sf->Average_f()));
		if (sampledType)
			*sampledType = BSDF_DIFFUSE;
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->Pdf(WorldToLocal(wiW));
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->f(sw, WorldToLocal(wiW)) *
				fabsf(reverse ? Dot(woW, dgShading.nn)  / sf->Average_f() :
				Dot(wiW, ng) * Dot(woW, dgShading.nn) /
				(Dot(wiW, dgShading.nn) * sf->Average_f()));
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

protected:
	// GonioAreaBSDF Private Methods
	virtual ~GonioAreaBSDF() { }
	//GonioAreaBSDF Private Data
	const SampleableSphericalFunction *sf;
};

// AreaLight Method Definitions
AreaLightImpl::AreaLightImpl(const Transform &light2world,
	boost::shared_ptr<Texture<SWCSpectrum> > &le, float g, float pow,
	float e, SampleableSphericalFunction *ssf, u_int ns,
	const boost::shared_ptr<Primitive> &p) :
	AreaLight("AreaLight-" + boost::lexical_cast<string>(this),
	light2world, ns), Le(le), paramGain(g), gain(g), power(pow),
	efficacy(e), func(ssf)
{
	if (p->CanIntersect() && p->CanSample()) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Primitive> pr(p);
		prim = pr;
	} else {
		// Create _PrimitiveSet_ for _Primitive_
		vector<boost::shared_ptr<Primitive> > refinedPrims;
		PrimitiveRefinementHints refineHints(true);
		p->Refine(refinedPrims, refineHints, p);
		if (refinedPrims.size() == 1)
			prim = refinedPrims[0];
		else
			prim = boost::shared_ptr<Primitive>(new PrimitiveSet(refinedPrims));
	}
	area = prim->Area();
	Le->SetIlluminant(); // Illuminant must be set before calling Le->Y()
	const float gainFactor = power * efficacy /
		(area * M_PI * Le->Y());
	if (gainFactor > 0.f && !isinf(gainFactor))
		gain *= gainFactor;

	AddFloatAttribute(*this, "gain", "AreaLight gain", &AreaLightImpl::paramGain);
	AddFloatAttribute(*this, "power", "AreaLight power", &AreaLightImpl::power);
	AddFloatAttribute(*this, "efficacy", "AreaLight efficacy", &AreaLightImpl::efficacy);
	AddFloatAttribute(*this, "area", "AreaLight area", &AreaLightImpl::area);
}

AreaLightImpl::~AreaLightImpl()
{
	delete func;
}

float AreaLightImpl::Power(const Scene &scene) const
{
	return gain * area * M_PI * Le->Y();
}

float AreaLightImpl::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	return prim->Pdf(p, dg);
}

bool AreaLightImpl::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	DifferentialGeometry dg;
	dg.time = sample.realTime;
	*pdf = prim->Sample(u1, u2, u3, &dg);
	if (!(*pdf > 0.f)) {
		*Le = 0.f;
		return false;
	}
	if(func)
		*bsdf = ARENA_ALLOC(sample.arena, GonioAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior(), func);
	else
		*bsdf = ARENA_ALLOC(sample.arena, UniformAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior());
	*Le = this->Le->Evaluate(sample.swl, dg) * (gain * M_PI / *pdf);
	return true;
}
bool AreaLightImpl::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *Le) const
{
	DifferentialGeometry dg;
	dg.time = sample.realTime;
	const float pdfd = prim->Sample(p, u1, u2, u3, &dg);
	if (!(pdfd > 0.f)) {
		*Le = 0.f;
		return false;
	}
	if (pdf)
		*pdf = prim->Pdf(dg);
	*pdfDirect = pdfd;
	if(func)
		*bsdf = ARENA_ALLOC(sample.arena, GonioAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior(), func);
	else
		*bsdf = ARENA_ALLOC(sample.arena, UniformAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior());
	*Le = this->Le->Evaluate(sample.swl, dg) * (gain * M_PI / pdfd);
	return true;
}
bool AreaLightImpl::L(const Sample &sample, const Ray &ray,
	const DifferentialGeometry &dg, BSDF **bsdf, float *pdf,
	float *pdfDirect, SWCSpectrum *Le) const
{
	if(func) {
		*bsdf = ARENA_ALLOC(sample.arena, GonioAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior(), func);
	} else {
		if (!(Dot(dg.nn, ray.d) < 0.f))
			return false;
		*bsdf = ARENA_ALLOC(sample.arena, UniformAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior());
	}
	if (pdf)
		*pdf = prim->Pdf(dg);
	if (pdfDirect)
		*pdfDirect = prim->Pdf(ray.o, dg);
	*Le *= this->Le->Evaluate(sample.swl, dg) * (gain * M_PI) * (*bsdf)->F(sample.swl, Vector(dg.nn), -ray.d, true);
	return !Le->Black();
}

AreaLight* AreaLightImpl::CreateAreaLight(const Transform &light2world,
	const ParamSet &paramSet, const boost::shared_ptr<Primitive> &prim)
{
	boost::shared_ptr<Texture<SWCSpectrum> > L(
		paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));

	float g = paramSet.FindOneFloat("gain", 1.f);
	float p = paramSet.FindOneFloat("power", 100.f);		// Power/Lm in Watts
	float e = paramSet.FindOneFloat("efficacy", 17.f);		// Efficacy Lm per Watt

	boost::shared_ptr<const SphericalFunction> sf(CreateSphericalFunction(paramSet));
	SampleableSphericalFunction *ssf = NULL;
	if (sf)
		ssf = new SampleableSphericalFunction(sf);

	int nSamples = paramSet.FindOneInt("nsamples", 1);

	AreaLightImpl *l = new AreaLightImpl(light2world, L, g, p, e, ssf, nSamples, prim);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterAreaLight<AreaLightImpl> r("area");

