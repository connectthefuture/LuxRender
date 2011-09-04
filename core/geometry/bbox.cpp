/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

// bbox.cpp*
#include "bbox.h"
#include "ray.h"

namespace lux
{

// BBox Method Definitions
 BBox Union(const BBox &b, const Point &p) {
	BBox ret = b;
	ret.pMin.x = min(b.pMin.x, p.x);
	ret.pMin.y = min(b.pMin.y, p.y);
	ret.pMin.z = min(b.pMin.z, p.z);
	ret.pMax.x = max(b.pMax.x, p.x);
	ret.pMax.y = max(b.pMax.y, p.y);
	ret.pMax.z = max(b.pMax.z, p.z);
	return ret;
}
 BBox Union(const BBox &b, const BBox &b2) {
	BBox ret;
	ret.pMin.x = min(b.pMin.x, b2.pMin.x);
	ret.pMin.y = min(b.pMin.y, b2.pMin.y);
	ret.pMin.z = min(b.pMin.z, b2.pMin.z);
	ret.pMax.x = max(b.pMax.x, b2.pMax.x);
	ret.pMax.y = max(b.pMax.y, b2.pMax.y);
	ret.pMax.z = max(b.pMax.z, b2.pMax.z);
	return ret;
}
 
void BBox::BoundingSphere(Point *c, float *rad) const {
	*c = .5f * pMin + .5f * pMax;
	*rad = Inside(*c) ? Distance(*c, pMax) : 0.f;
}
// NOTE - lordcrc - BBox::IntersectP relies on IEEE 754 behaviour of infinity and /fp:fast breaks this
#if defined(WIN32) && !defined(__CYGWIN__)
#pragma float_control(push)
#pragma float_control(precise, on)
#endif
bool BBox::IntersectP(const Ray &ray, float *hitt0,
		float *hitt1) const {
	float t0 = ray.mint, t1 = ray.maxt;
	for (int i = 0; i < 3; ++i) {
		// Update interval for _i_th bounding box slab
		float invRayDir = 1.f / ray.d[i];
		float tNear = (pMin[i] - ray.o[i]) * invRayDir;
		float tFar  = (pMax[i] - ray.o[i]) * invRayDir;
		// Update parametric interval from slab intersection $t$s
		if (tNear > tFar) swap(tNear, tFar);
		t0 = tNear > t0 ? tNear : t0;
		t1 = tFar  < t1 ? tFar  : t1;
		if (t0 > t1) return false;
	}
	if (hitt0) *hitt0 = t0;
	if (hitt1) *hitt1 = t1;
	return true;
}
#if defined(WIN32) && !defined(__CYGWIN__)
#pragma float_control(pop)
#endif

}
