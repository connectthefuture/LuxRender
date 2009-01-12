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

// motionsystem.cpp*
#include "motionsystem.h"
#include "error.h"

using namespace lux;

MotionSystem::MotionSystem(float st, float et,
		const Transform &s, const Transform &e) {
	
	startTime = st;
	endTime = et;
	start = s;
	end = e;
	startMat = start.GetMatrix();
	endMat = end.GetMatrix();

	// initialize to false
	hasTranslationX = hasTranslationY = hasTranslationZ =
		hasScaleX = hasScaleY = hasScaleZ = hasRotation = isActive = false;

	if (!DecomposeMatrix(startMat, startT)) {
		luxError(LUX_MATH, LUX_WARNING, "Singular start matrix in MotionSystem, interpolation disabled");
		return;
	}
	if (!DecomposeMatrix(endMat, endT)) {
		luxError(LUX_MATH, LUX_WARNING, "Singular end matrix in MotionSystem, interpolation disabled");
		return;
	}

	Transform rot;
	
	rot = RotateX(Degrees(startT.Rx)) * 
		RotateY(Degrees(startT.Ry)) * 
		RotateZ(Degrees(startT.Rz));

	startRot = rot.GetMatrix();
	startQ = Quaternion(startRot);
	startQ.Normalize();

	rot = RotateX(Degrees(endT.Rx)) * 
		RotateY(Degrees(endT.Ry)) * 
		RotateZ(Degrees(endT.Rz));

	endQ = Quaternion(rot.GetMatrix());
	endQ.Normalize();	

	hasTranslationX = startT.Tx != endT.Tx;
	hasTranslationY = startT.Ty != endT.Ty;
	hasTranslationZ = startT.Tz != endT.Tz;
	hasTranslation = hasTranslationX || 
		hasTranslationY || hasTranslationZ;

	hasScaleX = startT.Sx != endT.Sx;
	hasScaleY = startT.Sy != endT.Sy;
	hasScaleZ = startT.Sz != endT.Sz;
	hasScale = hasScaleX || hasScaleY || hasScaleZ;

	hasRotation = (startT.Rx != endT.Rx) ||
		(startT.Ry != endT.Ry) ||
		(startT.Rz != endT.Rz);

	isActive = hasTranslation ||
		hasScale || hasRotation;
}

Transform MotionSystem::Sample(float time) const {
	if (!isActive)
		return start;

	// Determine interpolation value
	if(time <= startTime)
		return start;
	if(time >= endTime)
		return end;

	float w = endTime - startTime;
	float d = time - startTime;
	float le = d / w;

	float interMatrix[4][4];

	// if translation only, just modify start matrix
	if (hasTranslation && !(hasScale || hasRotation)) {
		memcpy(interMatrix, startMat->m, sizeof(float) * 16);
		if (hasTranslationX)
			interMatrix[0][3] = Lerp(le, startT.Tx, endT.Tx);
		if (hasTranslationY)
			interMatrix[1][3] = Lerp(le, startT.Ty, endT.Ty);
		if (hasTranslationZ)
			interMatrix[2][3] = Lerp(le, startT.Tz, endT.Tz);
		return Transform(interMatrix);
	}

	if (hasRotation) {
		// Quaternion interpolation of rotation
		Quaternion between_quat = slerp(le, startQ, endQ);
		toMatrix(between_quat, interMatrix);
	} else
		//toMatrix(startQ, interMatrix);
		memcpy(interMatrix, startRot->m, sizeof(float) * 16);

/*
	Transform R(interMatrix);
	Transform S = Scale(
		Lerp(le, startT.Sx, endT.Sx), 
		Lerp(le, startT.Sy, endT.Sy), 
		Lerp(le, startT.Sz, endT.Sz));
	Transform T = Translate(Vector(
		Lerp(le, startT.Tx, endT.Tx),
		Lerp(le, startT.Ty, endT.Ty), 
		Lerp(le, startT.Tz, endT.Tz)));

	return T * S * R;
*/

	if (hasScale) {
		float Sx = Lerp(le, startT.Sx, endT.Sx);
		float Sy = Lerp(le, startT.Sy, endT.Sy); 
		float Sz = Lerp(le, startT.Sz, endT.Sz);

		// T * S * R
		for (int j = 0; j < 3; j++) {
			interMatrix[0][j] = Sx * interMatrix[0][j];
			interMatrix[1][j] = Sy * interMatrix[1][j];
			interMatrix[2][j] = Sz * interMatrix[2][j];
		}
	} else {
		for (int j = 0; j < 3; j++) {
			interMatrix[0][j] = startT.Sx * interMatrix[0][j];
			interMatrix[1][j] = startT.Sy * interMatrix[1][j];
			interMatrix[2][j] = startT.Sz * interMatrix[2][j];
		}
	}

	if (hasTranslationX) {
		interMatrix[0][3] = Lerp(le, startT.Tx, endT.Tx);
	} else {
		interMatrix[0][3] = startT.Tx;
	}

	if (hasTranslationY) {
		interMatrix[1][3] = Lerp(le, startT.Ty, endT.Ty);
	} else {
		interMatrix[1][3] = startT.Ty;
	}

	if (hasTranslationZ) {
		interMatrix[2][3] = Lerp(le, startT.Tz, endT.Tz);
	} else {
		interMatrix[2][3] = startT.Tz;
	}

	return Transform(interMatrix);
}

bool MotionSystem::DecomposeMatrix(const boost::shared_ptr<Matrix4x4> m, Transforms &trans) const {

	boost::shared_ptr<Matrix4x4> locmat(new Matrix4x4(m->m));

	/* Normalize the matrix. */
	if (locmat->m[3][3] == 0)
		return false;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			locmat->m[i][j] /= locmat->m[3][3];
	/* pmat is used to solve for perspective, but it also provides
	 * an easy way to test for singularity of the upper 3x3 component.
	 */
	boost::shared_ptr<Matrix4x4> pmat(new Matrix4x4(locmat->m));
	for (int i = 0; i < 3; i++)
		pmat->m[i][3] = 0;
	pmat->m[3][3] = 1;

	// Note - radiance - disables as memory bug on win32
//	if ( pmat->Determinant() == 0.0 )
//		return false;

	/* First, isolate perspective.  This is the messiest. */
	if ( locmat->m[3][0] != 0 || locmat->m[3][1] != 0 ||
		locmat->m[3][2] != 0 ) {
		/* prhs is the right hand side of the equation. */
		//float prhs[4];
		//prhs[0] = locmat->m[3][0];
		//prhs[1] = locmat->m[3][1];
		//prhs[2] = locmat->m[3][2];
		//prhs[3] = locmat->m[3][3];

		/* Solve the equation by inverting pmat and multiplying
		 * prhs by the inverse.  (This is the easiest way, not
		 * necessarily the best.)
		 * inverse function (and det4x4, above) from the Matrix
		 * Inversion gem in the first volume.
		 */
		//boost::shared_ptr<Matrix4x4> tinvpmat = pmat->Inverse()->Transpose();
		//V4MulPointByMatrix(&prhs, &tinvpmat, &psol);
 
		/* Stuff the answer away. */
		//tran[U_PERSPX] = psol.x;
		//tran[U_PERSPY] = psol.y;
		//tran[U_PERSPZ] = psol.z;
		//tran[U_PERSPW] = psol.w;
		trans.Px = trans.Py = trans.Pz = trans.Pw = 0;
		/* Clear the perspective partition. */
		locmat->m[0][3] = locmat->m[1][3] =
			locmat->m[2][3] = 0;
		locmat->m[3][3] = 1;
	};
//	else		/* No perspective. */
//		tran[U_PERSPX] = tran[U_PERSPY] = tran[U_PERSPZ] =
//			tran[U_PERSPW] = 0;

	/* Next take care of translation (easy). */
	trans.Tx = locmat->m[0][3];
	trans.Ty = locmat->m[1][3];
	trans.Tz = locmat->m[2][3];
	for (int i = 0; i < 3; i++ )
		locmat->m[i][3] = 0;
	
	Vector row[3];
	/* Now get scale and shear. */
	for (int i = 0; i < 3; i++ ) {
		row[i].x = locmat->m[i][0];
		row[i].y = locmat->m[i][1];
		row[i].z = locmat->m[i][2];
	}

	/* Compute X scale factor and normalize first row. */
	trans.Sx = row[0].Length();
	row[0] *= 1 / trans.Sx;

	/* Compute XY shear factor and make 2nd row orthogonal to 1st. */
	trans.Sxy = Dot(row[0], row[1]);
	row[1] -= trans.Sxy * row[0];

	/* Now, compute Y scale and normalize 2nd row. */
	trans.Sy = row[1].Length();
	row[1] *= 1.0 / trans.Sy;
	trans.Sxy /= trans.Sy;

	/* Compute XZ and YZ shears, orthogonalize 3rd row. */
	trans.Sxz = Dot(row[0], row[2]);
	row[2] -= trans.Sxz * row[0];
	trans.Syz = Dot(row[1], row[2]);
	row[2] -= trans.Syz * row[1];

	/* Next, get Z scale and normalize 3rd row. */
	trans.Sz = row[2].Length();
	row[2] *= 1.0 / trans.Sz;
	trans.Sxz /= trans.Sz;
	trans.Syz /= trans.Sz;

	/* At this point, the matrix (in rows[]) is orthonormal.
	 * Check for a coordinate system flip.  If the determinant
	 * is -1, then negate the matrix and the scaling factors.
	 */
	if (Dot(row[0], Cross(row[1], row[2])) < 0) {
		trans.Sx *= -1;
		trans.Sy *= -1;
		trans.Sz *= -1;
		for (int i = 0; i < 3; i++ ) {
			row[i].x *= -1;
			row[i].y *= -1;
			row[i].z *= -1;
		}
	}

	/* Now, get the rotations out, as described in the gem. */
	trans.Ry = -asinf(-row[0].z);
	if ( cosf(trans.Ry) != 0 ) {
		trans.Rx = -atan2f(row[1].z, row[2].z);
		trans.Rz = -atan2f(row[0].y, row[0].x);
	} else {
		trans.Rx = -atan2f(-row[2].x, row[1].y);
		trans.Rz = 0;
	}
	/* All done! */
	return true;
}
