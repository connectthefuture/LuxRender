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

#ifndef LUX_SPPMRenderer_H
#define LUX_SPPMRenderer_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "sppm/hitpoints.h"
#include "sppm/photonsampler.h"

namespace lux
{

class SPPMRenderer;
class SPPMRHostDescription;
class SPPMIntegrator;

//------------------------------------------------------------------------------
// SPPMRDeviceDescription
//------------------------------------------------------------------------------

class SPPMRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	u_int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	u_int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const u_int units);

	friend class SPPMRenderer;
	friend class SPPMRHostDescription;

private:
	SPPMRDeviceDescription(SPPMRHostDescription *h, const string &n) :
		host(h), name(n) { }
	~SPPMRDeviceDescription() { }

	SPPMRHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SPPMRHostDescription
//------------------------------------------------------------------------------

class SPPMRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SPPMRenderer;
	friend class SPPMRDeviceDescription;

private:
	SPPMRHostDescription(SPPMRenderer *r, const string &n);
	~SPPMRHostDescription();

	SPPMRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SPPMRenderer
//------------------------------------------------------------------------------

class SPPMRenderer : public Renderer {
public:
	SPPMRenderer();
	~SPPMRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	double Statistics(const string &statName);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SPPMRDeviceDescription;
	friend class SPPMRHostDescription;

	static Renderer *CreateRenderer(const ParamSet &params);

	friend class HitPoints;

private:
	//--------------------------------------------------------------------------
	// Render threads
	//--------------------------------------------------------------------------

	class EyePassRenderThread : public boost::noncopyable {
	public:
		EyePassRenderThread(u_int index, SPPMRenderer *renderer);
		~EyePassRenderThread();

		static void RenderImpl(EyePassRenderThread *r);

		u_int  n;
		SPPMRenderer *renderer;
		boost::thread *thread; // keep pointer to delete the thread object

		RandomGenerator *threadRng;
		Sample *threadSample;
	};

	class PhotonPassRenderThread : public boost::noncopyable {
	public:
		PhotonPassRenderThread(u_int index, SPPMRenderer *renderer);
		~PhotonPassRenderThread();

		void TracePhotons(HaltonPhotonSampler *sampler);

		bool IsVisible(Scene &scene, const Sample *sample, const float *u);
		bool Splat(Scene &scene, const Sample *sample, const float *u);
		void Splat(SplatList *splatList, Scene &scene, const Sample *sample, const float *u);
		void TracePhotons(AMCMCPhotonSampler *sampler);

		static void RenderImpl(PhotonPassRenderThread *r);

		u_int  n;
		SPPMRenderer *renderer;
		boost::thread *thread; // keep pointer to delete the thread object

		// Used by AMC Photon Sampler
		u_int amcUniformCount;

		RandomGenerator *threadRng;
		Distribution1D *lightCDF;
	};

	double Statistics_GetNumberOfSamples();
	double Statistics_SamplesPSec();
	double Statistics_SamplesPTotSec();
	double Statistics_Efficiency();
	double Statistics_SamplesPPx();

	//--------------------------------------------------------------------------

	mutable boost::mutex classWideMutex;
	mutable boost::mutex renderThreadsMutex;
	boost::barrier *allThreadBarrier;
	boost::barrier *eyePassThreadBarrier;
	boost::barrier *photonPassThreadBarrier;
	boost::barrier *exitBarrier;

	RendererState state;
	vector<RendererHostDescription *> hosts;
	vector<EyePassRenderThread *> eyePassRenderThreads;
	vector<PhotonPassRenderThread *> photonPassRenderThreads;

	Scene *scene;
	SPPMIntegrator *sppmi;
	HitPoints *hitPoints;

	double photonHitEfficiency;

	// Store number of photon traced by lightgroup
	unsigned long long photonTracedTotal;
	u_int photonTracedPass;
	// Used by AMC Photon Sampler
	float accumulatedFluxScale;

	fast_mutex sampPosMutex;
	u_int sampPos;

	Timer s_Timer;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_SPPMRenderer_H
