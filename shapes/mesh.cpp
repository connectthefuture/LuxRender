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

#include "mesh.h"

using namespace lux;

Mesh::Mesh(const Transform &o2w, bool ro,
			int nv, const Point *P, const Normal *N, const float *UV,
			MeshTriangleType tritype, int trisCount, const int *tris,
			MeshQuadType quadtype, int nquadsCount, const int *quads) : Shape(o2w, ro) {
	// TODO: use AllocAligned

	// Dade - copy vertex data
    nverts = nv;
	p = new Point[nverts];
    // Dade - transform mesh vertices to world space
    for (int i  = 0; i < nverts; ++i)
        p[i] = ObjectToWorld(P[i]);

	// Dade - copy UV and N vertex data, if present
    if (UV) {
        uvs = new float[2 * nverts];
        memcpy(uvs, UV, 2 * nverts*sizeof(float));
    } else
		uvs = NULL;

    if (N) {
        n = new Normal[nverts];
        memcpy(n, N, nverts * sizeof(Normal));
    } else
		n = NULL;

	// Dade - copy triangle data    
	triType = tritype;
	ntris = trisCount;
	if (ntris == 0)
		triVertexIndex = NULL;
	else {
		triVertexIndex = new int[3 * ntris];
		memcpy(triVertexIndex, tris, 3 * ntris * sizeof(int));
	}

	// Dade - copy quad data
	quadType = quadtype;
	nquads = nquadsCount;
	if (nquads == 0)
		quadVertexIndex = NULL;
	else {
		quadVertexIndex = new int[4 * nquads];
		memcpy(quadVertexIndex, quads, 4 * nquads * sizeof(int));
	}
}

Mesh::~Mesh() {
    delete[] triVertexIndex;
	delete[] quadVertexIndex;
    delete[] p;
    delete[] n;
    delete[] uvs;
}

BBox Mesh::ObjectBound() const {
    BBox bobj;
    for (int i = 0; i < nverts; i++)
        bobj = Union(bobj, WorldToObject(p[i]));
    return bobj;
}

BBox Mesh::WorldBound() const {
    BBox worldBounds;
    for (int i = 0; i < nverts; i++)
        worldBounds = Union(worldBounds, p[i]);
    return worldBounds;
}

void Mesh::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	// TODO: skip degenerate shape

	std::stringstream ss;

	// Dade - refine triangles
	switch (triType) {
		case WALD_TRIANGLE:
			for (int i = 0; i < ntris; ++i) {
				boost::shared_ptr<Shape> o(new MeshWaldTriangle(ObjectToWorld,
						reverseOrientation,
						(Mesh *)this,
						i));
				refined.push_back(o);
			}
			break;
		case BARY_TRIANGLE:
			for (int i = 0; i < ntris; ++i) {
				boost::shared_ptr<Shape> o(new MeshBaryTriangle(ObjectToWorld,
						reverseOrientation,
						(Mesh *)this,
						i));
				refined.push_back(o);
			}
			break;
		default:
			ss.str("");
            ss << "Unknow triangle type in a mesh: " << triType;
            luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
			break;
	}

	// Dade - refine quads
	switch (quadType) {
		case QUADRILATERAL:
			for (int i = 0; i < nquads; ++i) {
				boost::shared_ptr<Shape> o(new MeshQuadrilateral(ObjectToWorld,
						reverseOrientation,
						(Mesh *)this,
						i));
				refined.push_back(o);
			}
			break;
		default:
			ss.str("");
            ss << "Unknow quad type in a mesh: " << quadType;
            luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
			break;
	}
}

Shape *Mesh::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	// Dade - read vertex data
    int npi;
    const Point *P = params.FindPoint("P", &npi);
	int nuvi;
    const float *UV = params.FindFloat("UV", &nuvi);

    // NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
    if (UV && (nuvi != npi * 2)) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"UV\"s for mesh must match \"P\"s");
        UV = NULL;
    }
    if (!P) return NULL;

	int nni;
	const Normal *N = params.FindNormal("N", &nni);
    if (N && (nni != npi)) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"N\"s for mesh must match \"P\"s");
        N = NULL;
    }

	// Dade - read triangle data
	MeshTriangleType triType;
	string stype = params.FindOneString("tritype", "wald");
	if (stype == "wald") triType = WALD_TRIANGLE;
	else if (stype == "bary") triType = BARY_TRIANGLE;
	else {
		std::stringstream ss;
		ss << "Triangle type  '" << stype << "' unknown. Using \"wald\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		triType = WALD_TRIANGLE;
	}

	int triIndicesCount;
	const int *triIndices = params.FindInt("triindices", &triIndicesCount);
	if (triIndices) {
		for (int i = 0; i < triIndicesCount; ++i) {
			if (triIndices[i] >= npi) {
				std::stringstream ss;
				ss << "Mesh has out of-bounds triangle vertex index " << triIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
				return NULL;
			}
		}
		
		triIndicesCount /= 3;
	} else
		triIndicesCount = 0;

	// Dade - read quad data
	MeshQuadType quadType;
	stype = params.FindOneString("quadtype", "quadrilateral");
	if (stype == "quadrilateral") quadType = QUADRILATERAL;
	else {
		std::stringstream ss;
		ss << "Quad type  '" << stype << "' unknown. Using \"quadrilateral\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		quadType = QUADRILATERAL;
	}

	int quadIndicesCount;
	const int *quadIndices = params.FindInt("quadindices", &quadIndicesCount);
	if (quadIndices) {
		for (int i = 0; i < quadIndicesCount; ++i) {
			if (quadIndices[i] >= npi) {
				std::stringstream ss;
				ss << "Mesh has out of-bounds quad vertex index " << quadIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
				return NULL;
			}
		}
		
		quadIndicesCount /= 4;
	} else
		quadIndicesCount = 0;

	if ((!triIndices) && (!quadIndices)) return NULL;

    return new Mesh(o2w, reverseOrientation,
			npi, P, N, UV,
			triType, triIndicesCount, triIndices,
			quadType, quadIndicesCount, quadIndices);
}