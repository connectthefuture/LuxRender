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

#include "shape.h"
#include "mc.h"

namespace lux
{

// minimalistic mesh class for now
class QuadMesh : public Shape  {
public:
	QuadMesh(const Transform &o2w, bool ro, int nq, int nv, 
		const int *vi, const Point *P) 
		: Shape(o2w, ro), nquads(nq), nverts(nv) {				
		
	    idx = new int[3 * nquads];
		memcpy(idx, vi, 4 * nquads * sizeof(int));

		p = new Point[nverts];
		// Transform mesh vertices to world space
		for (int i  = 0; i < nverts; ++i)
			p[i] = ObjectToWorld(P[i]);	
	}

	~QuadMesh() {
		delete[] idx;
		delete[] p;
	}

	BBox ObjectBound() const;

	int nquads, nverts;
	int *idx;
	Point *p;
private:

};

// Quadrilateral Declarations
// assumes points form a strictly convex, planar quad
class Quadrilateral : public Shape {
public:
	// Quadrilateral Public Methods
	Quadrilateral(const Transform &o2w, bool ro, QuadMesh *m, int *indices);
	~Quadrilateral();
	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	bool IntersectP(const Ray &ray) const;
	float Area() const;
	Point Sample(float u1, float u2, Normal *Ns) const {
		Point p;

		const Point &p0 = mesh->p[idx[0]];
		const Point &p1 = mesh->p[idx[1]];
		const Point &p2 = mesh->p[idx[2]];
	    const Point &p3 = mesh->p[idx[3]];

		p = (1.f-u1)*(1.f-u2)*p0 + u1*(1.f-u2)*p1
			+u1*u2*p2 + (1.f-u1)*u2*p3;

		Vector e0 = p1 - p0;
		Vector e1 = p2 - p0;

		*Ns = Normalize(Normal(Cross(e0, e1)));
		if (reverseOrientation) *Ns *= -1.f;
		return p;
	}
	
private:
	// Quadrilateral Private Data
	int *idx;
	QuadMesh *mesh;
};

}//namespace lux