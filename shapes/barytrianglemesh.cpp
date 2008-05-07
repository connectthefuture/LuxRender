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


#include "barytrianglemesh.h"
#include "mc.h"
#include "paramset.h"

using namespace lux;

// BaryTriangleMesh Method Definitions
BaryTriangleMesh::BaryTriangleMesh(const Transform &o2w, bool ro,
        int nt, int nv, const int *vi, const Point *P,
        const Normal *N, const Vector *S, const float *uv)
: Shape(o2w, ro) {
    ntris = nt;
    nverts = nv;
    vertexIndex = new int[3 * ntris];
    memcpy(vertexIndex, vi, 3 * ntris * sizeof(int));
    // Copy _uv_, _N_, and _S_ vertex data, if present
    if (uv) {
        uvs = new float[2*nverts];
        memcpy(uvs, uv, 2*nverts*sizeof(float));
    }
    else uvs = NULL;
    p = new Point[nverts];
    if (N) {
        n = new Normal[nverts];
        memcpy(n, N, nverts*sizeof(Normal));
    }
    else n = NULL;
    if (S) {
        s = new Vector[nverts];
        memcpy(s, S, nverts*sizeof(Vector));
    }
    else s = NULL;
    // Transform mesh vertices to world space
    for (int i  = 0; i < nverts; ++i)
        p[i] = ObjectToWorld(P[i]);
}
BaryTriangleMesh::~BaryTriangleMesh() {
    delete[] vertexIndex;
    delete[] p;
    delete[] s;
    delete[] n;
    delete[] uvs;
}
BBox BaryTriangleMesh::ObjectBound() const {
    BBox bobj;
    for (int i = 0; i < nverts; i++)
        bobj = Union(bobj, WorldToObject(p[i]));
    return bobj;
}
BBox BaryTriangleMesh::WorldBound() const {
    BBox worldBounds;
    for (int i = 0; i < nverts; i++)
        worldBounds = Union(worldBounds, p[i]);
    return worldBounds;
}

void
BaryTriangleMesh::Refine(vector<boost::shared_ptr<Shape> > &refined)
const {
    for (int i = 0; i < ntris; ++i) {
        boost::shared_ptr<Shape> o(new BaryTriangle(ObjectToWorld,
                reverseOrientation,
                (BaryTriangleMesh *)this,
                i));
        refined.push_back(o);
    }
}

BBox BaryTriangle::ObjectBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(WorldToObject(p1), WorldToObject(p2)),
            WorldToObject(p3));
}
BBox BaryTriangle::WorldBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(p1, p2), p3);
}
bool BaryTriangle::Intersect(const Ray &ray, float *tHit,
        DifferentialGeometry *dg) const {
    
    Vector e1, e2, s1;
    // Initialize triangle intersection statistics
    // radiance - disabled for threading // static StatsPercentage triangleHits("Geometry", "Triangle Ray Intersections");
    // Update triangle tests count
    // radiance - disabled for threading // triangleHits.Add(0, 1);
    // Compute $\VEC{s}_1$
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    e1 = p2 - p1;
    e2 = p3 - p1;
    s1 = Cross(ray.d, e2);
    const float divisor = Dot(s1, e1);
    if (divisor == 0.f)
        return false;
    const float invDivisor = 1.f / divisor;
    // Compute first barycentric coordinate
    const Vector d = ray.o - p1;
    const float b1 = Dot(d, s1) * invDivisor;
    if (b1 < 0.f || b1 > 1.f)
        return false;
    // Compute second barycentric coordinate
    const Vector s2 = Cross(d, e1);
    const float b2 = Dot(ray.d, s2) * invDivisor;
    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;
    // Compute _t_ to intersection point
    const float t = Dot(e2, s2) * invDivisor;
    if (t < ray.mint || t > ray.maxt)
        return false;
    // radiance - disabled for threading // triangleHits.Add(1, 0); //NOBOOK
    // Fill in _DifferentialGeometry_ from triangle hit
    // Compute triangle partial derivatives
    Vector dpdu, dpdv;
    float uvs[3][2];
    GetUVs(uvs);
    // Compute deltas for triangle partial derivatives
    const float du1 = uvs[0][0] - uvs[2][0];
    const float du2 = uvs[1][0] - uvs[2][0];
    const float dv1 = uvs[0][1] - uvs[2][1];
    const float dv2 = uvs[1][1] - uvs[2][1];
    const Vector dp1 = p1 - p3, dp2 = p2 - p3;
    const float determinant = du1 * dv2 - dv1 * du2;
    if (determinant == 0.f) {
        // Handle zero determinant for triangle partial derivative matrix
        CoordinateSystem(Normalize(Cross(e1, e2)), &dpdu, &dpdv);
    }
    else {
        const float invdet = 1.f / determinant;
        dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
        dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
    }
    
    // Interpolate $(u,v)$ triangle parametric coordinates
    const float b0 = 1 - b1 - b2;
    const float tu = b0*uvs[0][0] + b1*uvs[1][0] + b2*uvs[2][0];
    const float tv = b0*uvs[0][1] + b1*uvs[1][1] + b2*uvs[2][1];
    *dg = DifferentialGeometry(ray(t), dpdu, dpdv,
            Vector(0, 0, 0), Vector(0, 0, 0),
            tu, tv, this);
    
    // NOTE - ratow - Invert generated normal in case it falls on the wrong side.
    if(!mesh->uvs) {
        if(mesh->n) {
            if(Dot(ObjectToWorld(mesh->n[v[0]]+mesh->n[v[1]]+mesh->n[v[2]]), dg->nn) < 0)
                dg->nn *= -1;
        } else {
            if(Dot(Cross(e1, e2), dg->nn) < 0)
                dg->nn *= -1;
        }
    }
    
    *tHit = t;
    return true;
}
bool BaryTriangle::IntersectP(const Ray &ray) const {
    // Initialize triangle intersection statistics
    // radiance - disabled for threading // static StatsPercentage triangleHits("Geometry","Triangle Ray Intersections");
    // Update triangle tests count
    // radiance - disabled for threading // triangleHits.Add(0, 1);
    // Compute $\VEC{s}_1$
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    Vector e1 = p2 - p1;
    Vector e2 = p3 - p1;
    Vector s1 = Cross(ray.d, e2);
    float divisor = Dot(s1, e1);
    if (divisor == 0.f)
        return false;
    float invDivisor = 1.f / divisor;
    // Compute first barycentric coordinate
    Vector d = ray.o - p1;
    float b1 = Dot(d, s1) * invDivisor;
    if (b1 < 0.f || b1 > 1.f)
        return false;
    // Compute second barycentric coordinate
    Vector s2 = Cross(d, e1);
    float b2 = Dot(ray.d, s2) * invDivisor;
    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;
    // Compute _t_ to intersection point
    float t = Dot(e2, s2) * invDivisor;
    if (t < ray.mint || t > ray.maxt)
        return false;
    // radiance - disabled for threading // triangleHits.Add(1, 0); //NOBOOK
    return true;
}
void BaryTriangle::GetUVs(float uv[3][2]) const {
    if (mesh->uvs) {
        uv[0][0] = mesh->uvs[2*v[0]];
        uv[0][1] = mesh->uvs[2*v[0]+1];
        uv[1][0] = mesh->uvs[2*v[1]];
        uv[1][1] = mesh->uvs[2*v[1]+1];
        uv[2][0] = mesh->uvs[2*v[2]];
        uv[2][1] = mesh->uvs[2*v[2]+1];
    } else {
        uv[0][0] = mesh->p[v[0]].x;
        uv[0][1] = mesh->p[v[0]].y;
        uv[1][0] = mesh->p[v[1]].x;
        uv[1][1] = mesh->p[v[1]].y;
        uv[2][0] = mesh->p[v[2]].x;
        uv[2][1] = mesh->p[v[2]].y;
    }
}
float BaryTriangle::Area() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return 0.5f * Cross(p2-p1, p3-p1).Length();
}
Point BaryTriangle::Sample(float u1, float u2,
        Normal *Ns) const {
    float b1, b2;
    UniformSampleTriangle(u1, u2, &b1, &b2);
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    Point p = b1 * p1 + b2 * p2 + (1.f - b1 - b2) * p3;
    Normal n = Normal(Cross(p2-p1, p3-p1));
    *Ns = Normalize(n);
    if (reverseOrientation) *Ns *= -1.f;
    return p;
}
Shape* BaryTriangleMesh::CreateShape(const Transform &o2w,
        bool reverseOrientation, const ParamSet &params) {
    int nvi, npi, nuvi, nsi, nni;
    const int *vi = params.FindInt("indices", &nvi);
    const Point *P = params.FindPoint("P", &npi);
    const float *uvs = params.FindFloat("uv", &nuvi);
    if (!uvs) uvs = params.FindFloat("st", &nuvi);
    // NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
    if (uvs && nuvi != npi * 2) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"uv\"s for triangle mesh must match \"P\"s");
        uvs = NULL;
    }
    if (!vi || !P) return NULL;
    const Vector *S = params.FindVector("S", &nsi);
    if (S && nsi != npi) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"S\"s for triangle mesh must match \"P\"s");
        S = NULL;
    }
    const Normal *N = params.FindNormal("N", &nni);
    if (N && nni != npi) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"N\"s for triangle mesh must match \"P\"s");
        N = NULL;
    }
    if (uvs && N) {
        // if there are normals, check for bad uv's that
        // give degenerate mappings; discard them if so
        const int *vp = vi;
        for (int i = 0; i < nvi; i += 3, vp += 3) {
            float area = .5f * Cross(P[vp[0]]-P[vp[1]], P[vp[2]]-P[vp[1]]).Length();
            if (area < 1e-7) continue; // ignore degenerate tris.
            // NOTE - radiance - disabled check for degenerate uv coordinates.
            // discards all uvs in case of unimportant small errors from exporters.
            /*if ((uvs[2*vp[0]] == uvs[2*vp[1]] &&
             * uvs[2*vp[0]+1] == uvs[2*vp[1]+1]) ||
             * (uvs[2*vp[1]] == uvs[2*vp[2]] &&
             * uvs[2*vp[1]+1] == uvs[2*vp[2]+1]) ||
             * (uvs[2*vp[2]] == uvs[2*vp[0]] &&
             * uvs[2*vp[2]+1] == uvs[2*vp[0]+1])) {
             * Warning("Degenerate uv coordinates in triangle mesh.  Discarding all uvs.");
             * printf("vp = %i\n", *vp);
             * uvs = NULL;
             * break;
             * }*/
        }
    }
    for (int i = 0; i < nvi; ++i)
        if (vi[i] >= npi) {
            //Error("trianglemesh has out of-bounds vertex index %d (%d \"P\" values were given",
            //	vi[i], npi);
            std::stringstream ss;
            ss<<"trianglemesh has out of-bounds vertex index "<<vi[i]<<" ("<<npi<<"  \"P\" values were given";
            luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
            return NULL;
        }
    return new BaryTriangleMesh(o2w, reverseOrientation, nvi/3, npi, vi, P,
            N, S, uvs);
}