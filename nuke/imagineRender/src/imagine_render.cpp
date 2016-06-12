/*
 ImagineRender: an integration of Imagine into Nuke as a Render node
 Copyright 2012-2016 Peter Pearson.
*/

#include "imagine_render.h"

#include <cmath>

#include "DDImage/DDMath.h"
#include "DDImage/CameraOp.h"
#include "DDImage/LightOp.h"
#include "DDImage/GeoOp.h"
#include "DDImage/Thread.h"

/// imagine

#include "utils/system.h"
#include "global_context.h"
#include "output_context.h"

const char* const PROJECTION_TYPE[] =
{
	"perspective",
	"spherical",
	"spherical omni-dir",
	"fisheye",
	"fisheye omni-dir",
	0
};

const char* const ANTI_ALIASING_TYPE[] =
{
	"1",
	"4",
	"9",
	"16",
	"25",
	"36",
	"49",
	"64",
	"81",
	0
};


//
using namespace DD::Image;

ImagineRenderIop::ImagineRenderIop(Node* node) : Iop(node), m_pOutputImage(NULL), m_pRaytracer(NULL), m_firstEngine(true),
	m_ambientOcclusion(false), m_antiAliasing(1), m_globalIlluminationSamples(64)
{
	m_cameraHash = 0;
	m_lightsHash = 0;
	m_geometryHash = 0;

	// number of threads to use for geo tess/building and accel building
	GlobalContext::instance().setWorkerThreads(8);
}

void ImagineRenderIop::knobs(Knob_Callback callback)
{
	Divider(callback, "render settings");

	Enumeration_knob(callback, (int*)&m_projectionType, PROJECTION_TYPE, "projection_type", "projection type");
	Newline(callback);

	Enumeration_knob(callback, (int*)&m_antiAliasing, ANTI_ALIASING_TYPE, "anti_aliasing", "anti aliasing");

	Bool_knob(callback, &m_ambientOcclusion, "ambient_occlusion", "ambient occlusion");

	Int_knob(callback, &m_globalIlluminationSamples, IRange(1, 512), "gi_samples", "GI Samples");
	SetFlags(callback, DD::Image::Knob::SLIDER);
}

void ImagineRenderIop::_validate(bool for_real)
{
	if (m_pOutputImage)
	{
		delete m_pOutputImage;
		m_pOutputImage = NULL;
	}

	Op* pInput2 = Op::input(2);
	if (dynamic_cast<Iop*>(pInput2))
	{
		Iop* pBackground = dynamic_cast<Iop*>(pInput2);

		pBackground->validate(real_valid());

		m_imageWidth = pBackground->info().format().width();
		m_imageHeight = pBackground->info().format().height();
	}
	else
	{
		m_imageWidth = 1280;
		m_imageHeight = 720;
	}

	m_format = Format(m_imageWidth, m_imageHeight);

	info_.full_size_format(m_format);
	info_.format(m_format);
	info_.channels(Mask_RGBA);
	info_.set(m_format);

	// clear the settings from last time
	m_renderSettings = Params();

	m_renderSettings.add("width", m_imageWidth);
	m_renderSettings.add("height", m_imageHeight);

	unsigned int flags = COMPONENT_RGBA;
	m_pOutputImage = new OutputImage(m_imageWidth, m_imageHeight, flags);

	if (m_pRaytracer)
	{
		delete m_pRaytracer;
		m_pRaytracer = NULL;
	}

	m_firstEngine = true;

	buildScene();

	// we want to do things like bbox calculation, vertex normal calculation (for Subdiv approximations) and triangle tessellation
	// in parallel across multiple threads
	m_scene.setParallelGeoBuild(true);

	// set lazy mode
	m_scene.setLazy(true);

	unsigned int threads = System::getNumberOfThreads();

	m_pRaytracer = new Raytracer(m_scene, m_pOutputImage, m_renderSettings, false, threads);
	m_pRaytracer->setHost(this);
	m_pRaytracer->setAmbientColour(Colour3f(1.0f));
	m_pRaytracer->setExtraChannels(0);

	// for the moment we can map this directly with a shift (i.e. enum 0 == 1 sample per pixel squared)
	m_renderSettings.add("antiAliasing", m_antiAliasing + 1);

	if (m_ambientOcclusion)
	{
		m_renderSettings.add("ambientOcclusion", true);
		m_renderSettings.add("ambientOcclusionSamples", 5);
	}

	m_pRaytracer->initialise(m_pOutputImage, m_renderSettings);
}

void ImagineRenderIop::_request(int x, int y, int r, int t, DD::Image::ChannelMask m, int n)
{

}

void ImagineRenderIop::append(DD::Image::Hash &hash)
{
	for (unsigned int i = 0; i < 3; i++)
	{
		Op* pOp = Op::input(i);

		if (pOp)
			pOp->append(hash);
	}
}

void ImagineRenderIop::engine(int y, int x, int r, ChannelMask channels, DD::Image::Row& out)
{
	if (m_firstEngine)
	{
		DD::Image::Guard guard(m_lock);
		if (m_firstEngine)
		{
			m_pRaytracer->renderScene(1.0f, NULL);

			m_pOutputImage->normaliseProgressive();
			m_pOutputImage->applyExposure(1.3f);

			m_firstEngine = false;
		}
	}

	float* p[4];
	p[0] = out.writable(Chan_Red);
	p[1] = out.writable(Chan_Green);
	p[2] = out.writable(Chan_Blue);
	p[3] = out.writable(Chan_Alpha);

	for (unsigned int i = 0; i < 4; i++)
	{
		Colour4f* pImage = m_pOutputImage->colourRowPtr(m_imageHeight - y - 1);
		float* TO = p[i] + x;
		float* END = TO + (r - x);
		while (TO < END)
		{
			float C = (*pImage)[i];
			*TO++ = C;
			pImage++;
		}
	}
}

const char* ImagineRenderIop::input_label(int n, char*) const
{
	switch (n)
	{
		case 0: return "Camera";
		case 1: return "Obj/Scn";
		case 2: return "Bkg";
	}

	return 0;
}

bool ImagineRenderIop::test_input(int index, Op* pOp) const
{
	switch (index)
	{
		case 0:
			return dynamic_cast<CameraOp*>(pOp) != NULL;
			break;
		case 1:
			return dynamic_cast<GeoOp*>(pOp) != NULL;
			break;
		case 2:
			return dynamic_cast<Iop*>(pOp) != NULL;
			break;
	}

	return false;
}

void ImagineRenderIop::tileDone(const TileInfo& tileInfo, unsigned int threadID)
{
	// TODO: try and do something useful with this, i.e. working out that an entire row of tiles is done,
	//       so we can return the corresponding scanlines to Nuke
}

static Iop* build(Node* node)
{
	return new ImagineRenderIop(node);
}

const Iop::Description ImagineRenderIop::d("ImagineRender", "ImagineRender", build);
