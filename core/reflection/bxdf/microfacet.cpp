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

// microfacet.cpp*
#include "microfacet.h"
#include "luxrays/core/color/swcspectrum.h"
#include "fresnel.h"
#include "microfacetdistribution.h"

#include "luxrays/utils/mc.h"

using namespace lux;

MicrofacetReflection::MicrofacetReflection(const SWCSpectrum &reflectance,
	const Fresnel *fr, MicrofacetDistribution *d, bool oneS)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	  R(reflectance), distribution(d), fresnel(fr), oneSided(oneS)
{
}

void MicrofacetReflection::F(const SpectrumWavelengths &sw, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	const float cosThetaO = fabsf(CosTheta(wo));
	const float cosThetaI = fabsf(CosTheta(wi));
	if (cosThetaO == 0.f || cosThetaI == 0.f)
		return;
	Vector wh = wi + wo;
	if (wh == Vector(0.f))
		return;
	wh = Normalize(wh);
	if (wh.z < 0.f) {
		if (oneSided)
			return;
		wh = -wh;
	}
	float cosThetaH = Dot(wi, wh);
	SWCSpectrum F;
	fresnel->Evaluate(sw, cosThetaH, &F);
	f_->AddWeighted(distribution->D(wh) * distribution->G(wo, wi, wh) /
		(4.f * cosThetaI), R * F);
}

bool MicrofacetReflection::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	*wi = 2.f * Dot(wo, wh) * wh - wo;
	if ((oneSided && wo.z <= 0.f) || !SameHemisphere(wo, *wi)) 
		return false;

	const float cosThetaH = Dot(wo, wh);
	const float factor = d * fabsf(cosThetaH) / *pdf *
		distribution->G(wo, *wi, wh);
	SWCSpectrum F;
	fresnel->Evaluate(sw, cosThetaH, &F);
	if (reverse)
		*f_ = (factor /	fabsf(wo.z)) * (R * F);
	else
		*f_ = (factor /	fabsf(wi->z)) * (R * F);
	*pdf /= 4.f * fabsf(cosThetaH);
	if (pdfBack)
		*pdfBack = *pdf;
	return true;
}
float MicrofacetReflection::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi) const
{
	Vector wh = wi + wo;
	if (wh == Vector(0.f))
		return 0.f;
	wh = Normalize(wh);
	if (wh.z < 0.f) {
		if (oneSided)
			return 0.f;
		wh = -wh;
	}
	return distribution->Pdf(wh) / (4.f * AbsDot(wo, wh));
}

MicrofacetTransmission::MicrofacetTransmission(const SWCSpectrum &transmitance,
	const Fresnel *fr, MicrofacetDistribution *d, bool disp)
	: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY)),
	  T(transmitance), distribution(d), fresnel(fr), dispersion(disp)
{
}

void MicrofacetTransmission::F(const SpectrumWavelengths &sw, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	const bool entering = CosTheta(wo) > 0.f;
	if (dispersion && !sw.single) {
		const float cosThetaI = fabsf(CosTheta(wi));
		SpectrumWavelengths swl(sw);
		swl.single = true;
		for (swl.single_w = 0; swl.single_w < WAVELENGTH_SAMPLES; ++swl.single_w) {
			const float eta = entering ?
				1.f / fresnel->Index(swl) : fresnel->Index(swl);
			Vector wh(eta * wo + wi);
			if (wh.z < 0.f)
				wh = -wh;
			const float lengthSquared = wh.LengthSquared();
			if (!(lengthSquared > 0.f))
				continue;
			wh /= sqrtf(lengthSquared);
			const float cosThetaIH = AbsDot(wi, wh);
			const float cosThetaOH = Dot(wo, wh);
			SWCSpectrum F;
			fresnel->Evaluate(swl, cosThetaOH, &F);
			f_->c[swl.single_w] += fabsf(cosThetaOH) * cosThetaIH *
				distribution->D(wh) * distribution->G(wo, wi, wh) /
				(cosThetaI * lengthSquared) *
				T.c[swl.single_w] * (1.f - F.c[swl.single_w]);
		}
		return;
	}
	const float eta = entering ?
		1.f / fresnel->Index(sw) : fresnel->Index(sw);
	Vector wh(eta * wo + wi);
	if (wh.z < 0.f)
		wh = -wh;
	const float lengthSquared = wh.LengthSquared();
	if (!(lengthSquared > 0.f))
		return;
	wh /= sqrtf(lengthSquared);
	const float cosThetaI = fabsf(CosTheta(wi));
	const float cosThetaIH = AbsDot(wi, wh);
	const float cosThetaOH = Dot(wo, wh);
	SWCSpectrum F;
	fresnel->Evaluate(sw, cosThetaOH, &F);
	f_->AddWeighted(fabsf(cosThetaOH) * cosThetaIH * distribution->D(wh) *
		distribution->G(wo, wi, wh) / (cosThetaI * lengthSquared),
		T * (SWCSpectrum(1.f) - F));
}

bool MicrofacetTransmission::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	const bool entering = CosTheta(wo) > 0.f;
	const bool single = sw.single; // save the initial value
	if (dispersion)
		sw.single = true;
	const float eta = entering ?
		1.f / fresnel->Index(sw) : fresnel->Index(sw);
	sw.single = single;
	const float cosThetaOH = Dot(wo, wh);
	const float sinThetaIH2 = eta * eta * max(0.f,
		1.f - cosThetaOH * cosThetaOH);
	if (sinThetaIH2 >= 1.f)
		return false;
	float cosThetaIH = sqrtf(1.f - sinThetaIH2);
	if (entering)
		cosThetaIH = -cosThetaIH;
	const float length = eta * cosThetaOH + cosThetaIH;
	*wi = length * wh - eta * wo;
	if (dispersion && !sw.single) {
		*f_ = SWCSpectrum(0.f);
		if (reverse)
			F(sw, *wi, wo, f_);
		else
			F(sw, wo, *wi, f_);
		*pdf = Pdf(sw, wo, *wi);
		*f_ /= *pdf;
		if (pdfBack)
			*pdfBack = Pdf(sw, *wi, wo);
		return true;
	}
	const float lengthSquared = length * length;
	if (pdfBack)
		*pdfBack = *pdf * fabsf(cosThetaOH) * eta * eta / lengthSquared;
	const float factor = distribution->G(wo, *wi, wh) * d *
		fabsf(cosThetaOH) / *pdf;

	SWCSpectrum F;
	if (reverse) {
		fresnel->Evaluate(sw, cosThetaIH, &F);
		*f_ = (factor / fabsf(CosTheta(wo))) *
			(T * (SWCSpectrum(1.f) - F));
	} else {
		fresnel->Evaluate(sw, cosThetaOH, &F);
		*f_ = (factor / fabsf(CosTheta(*wi))) *
			(T * (SWCSpectrum(1.f) - F));
	}
	*pdf *= fabsf(cosThetaIH) / lengthSquared;
	return true;
}
float MicrofacetTransmission::Pdf(const SpectrumWavelengths &sw,
	const Vector &wo, const Vector &wi) const
{
	if (SameHemisphere(wo, wi))
		return 0.f;
	const bool entering = CosTheta(wo) > 0.f;
	if (dispersion && !sw.single) {
		SpectrumWavelengths swl(sw);
		swl.single = true;
		float result = 0.f;
		for (swl.single_w = 0; swl.single_w < WAVELENGTH_SAMPLES; ++swl.single_w) {
			const float eta = entering ?
				1.f / fresnel->Index(swl) : fresnel->Index(swl);
			Vector wh(eta * wo + wi);
			if (wh.z < 0.f)
				wh = -wh;
			const float lengthSquared = wh.LengthSquared();
			if (!(lengthSquared > 0.f))
				continue;
			wh /= sqrtf(lengthSquared);
			const float cosThetaIH = AbsDot(wi, wh);
			result += distribution->Pdf(wh) * cosThetaIH / lengthSquared;
		}
		return result * .25f;
	}
	const float eta = entering ?
		1.f / fresnel->Index(sw) : fresnel->Index(sw);
	Vector wh(eta * wo + wi);
	if (wh.z < 0.f)
		wh = -wh;
	const float lengthSquared = wh.LengthSquared();
	if (!(lengthSquared > 0.f))
		return 0.f;
	wh /= sqrtf(lengthSquared);
	const float cosThetaIH = AbsDot(wi, wh);
	return distribution->Pdf(wh) * cosThetaIH / lengthSquared;
}
