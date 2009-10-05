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

#ifndef LUX_SCENE_H
#define LUX_SCENE_H
// scene.h*
#include "lux.h"
#include "api.h"
#include "primitive.h"
#include "sampling.h"
#include "timer.h"

#include "fastmutex.h"

#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

namespace lux {

class RenderThread : public boost::noncopyable {
public:
    RenderThread( int _n, ThreadSignals _signal, SurfaceIntegrator* _Si, VolumeIntegrator* _Vi,
            Sampler* _Splr, Camera* _Cam, Scene* _Scn)
    : n(_n), signal(_signal), surfaceIntegrator(_Si), volumeIntegrator(_Vi),
            sample(NULL), sampler(_Splr->clone()), camera(_Cam), scene(_Scn),
	    thread(NULL), samples(0.), blackSamples(0.) {
        sample = new Sample(surfaceIntegrator, volumeIntegrator, scene);
    }

    ~RenderThread()	{
//	    delete sampler; //FIXME some samplers don't clone the data pointers so deleting here will result in a double free in Scene::~Scene and use of freed memory in other render threads
        delete sample;
        delete thread;
    }

    static void render(RenderThread *r);

    int  n;
	ThreadSignals signal;
    SurfaceIntegrator *surfaceIntegrator;
    VolumeIntegrator *volumeIntegrator;
    Sample *sample;
    Sampler *sampler;
    Camera *camera;
    Scene *scene;
	TsPack *tspack;
    boost::thread *thread; // keep pointer to delete the thread object
	double samples, blackSamples;
	fast_mutex statLock;
};

// Scene Declarations
class  Scene {
public:
    // Scene Public Methods
    void Render();
    Scene(Camera *c, SurfaceIntegrator *in,
            VolumeIntegrator *vi, Sampler *s,
            boost::shared_ptr<Primitive> accel, const vector<Light *> &lts,
            const vector<string> &lg, VolumeRegion *vr);
	Scene(Camera *c);
    ~Scene();
    bool Intersect(const Ray &ray, Intersection *isect) const {
        return aggregate->Intersect(ray, isect);
    }
    bool IntersectP(const Ray &ray) const {
        return aggregate->IntersectP(ray);
    }
    const BBox &WorldBound() const;
    SWCSpectrum Li(const RayDifferential &ray, const Sample *sample,
            float *alpha = NULL) const;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
    void Transmittance(const TsPack *tspack, const Ray &ray, const Sample *sample, SWCSpectrum *const L) const;

    //Control methods
    void Start();
    void Pause();
    void Exit();
    //controlling number of threads
    int AddThread(); //returns the thread ID
    void RemoveThread();
	int getThreadsStatus(RenderingThreadInfo *info, int maxInfoCount);

	void SaveFLM( const string& filename );
	bool IsFilmOnly() const { return filmOnly; }

    double GetNumberOfSamples();
    double Statistics_SamplesPSec();
    double Statistics_SamplesPTotSec();
    double Statistics_Efficiency();
    double Statistics_SamplesPPx();
    //framebuffer access
    void UpdateFramebuffer();
    unsigned char* GetFramebuffer();

	//histogram access
	void getHistogramImage(unsigned char *outPixels, int width, int height, int options);

	// Parameter Access functions
	void SetParameterValue(luxComponent comp, luxComponentParameters param, double value, int index);
	double GetParameterValue(luxComponent comp, luxComponentParameters param, int index);
	double GetDefaultParameterValue(luxComponent comp, luxComponentParameters param, int index);
	void SetStringParameterValue(luxComponent comp, luxComponentParameters param, const string& value, int index);
	string GetStringParameterValue(luxComponent comp, luxComponentParameters param, int index);
	string GetDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, int index);

    int DisplayInterval();
    int FilmXres();
    int FilmYres();
    Timer s_Timer;
    double lastSamples, lastTime;
    //statistics
    double Statistics(const string &statName);

    int CreateRenderThread();
    void RemoveRenderThread();
    void SignalThreads(ThreadSignals signal);

    // Scene Data
    boost::shared_ptr<Primitive> aggregate;
    vector<Light *> lights;
    vector<string> lightGroups;
    Camera *camera;
    VolumeRegion *volumeRegion;
    SurfaceIntegrator *surfaceIntegrator;
    VolumeIntegrator *volumeIntegrator;
    Sampler *sampler;
    BBox bound;
    int seedBase;

	ContributionPool *contribPool;

    // Dade - number of samples received from network
    double numberOfSamplesFromNetwork;
    double stat_Samples, stat_blackSamples;

    // Dade - used to suspend render threads until the preprocessing phase is done
    bool preprocessDone;

	// Dade - tell rendering threads what to do when they have done
	bool suspendThreadsWhenDone;

private:

	// Dade - mutex used for adding/removing threads
	boost::mutex renderThreadsMutex;
    std::vector<RenderThread*> renderThreads;
    //boost::thread_group threadGroup;
    ThreadSignals CurThreadSignal;
	TsPack *tspack;
	bool filmOnly; // whether this scene has entire scene (incl. geometry, ..) or only a film
};

}//namespace lux

#endif // LUX_SCENE_H
