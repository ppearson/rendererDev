/*
 ImagineRender: an integration of Imagine into Nuke as a Render node
 Copyright 2012-2016 Peter Pearson.
*/

#ifndef IMAGINE_RENDER_H
#define IMAGINE_RENDER_H

#include "DDImage/Iop.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"

/// imagine
#include "scene.h"
#include "raytracer/raytracer.h"
#include "utils/params.h"
#include "image/output_image.h"

class ImagineRenderIop : public DD::Image::Iop, public RaytracerHost
{
public:
	enum ProjectionType
	{
		ePerspective,
		eSpherical,
		eSphericalOmniDir,
		eFisheye,
		eFisheyeOmniDir
	};

	// Nuke stuff
	ImagineRenderIop(Node* node);

	virtual void knobs(DD::Image::Knob_Callback callback);
	virtual void _validate(bool for_real);
	virtual void _request(int x, int y, int r, int t, DD::Image::ChannelMask m, int n);

	virtual void append(DD::Image::Hash &hash);

	virtual void engine(int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out);

	const char* input_label(int n, char*) const;
	virtual bool test_input(int index, DD::Image::Op* pOp) const;

	int minimum_inputs() const
	{
		return 3;
	}
	int maximum_inputs() const
	{
		return 3;
	}

	void buildScene();

	const char* Class() const { return "ImagineRender"; }
	const char* node_help() const { return "Imagine Renderer node."; }
	const char* displayName() const { return "ImagineRender"; }
	static const DD::Image::Iop::Description d;

	// Imagine stuff
	virtual void tileDone(const TileInfo& tileInfo, unsigned int threadID);

protected:
// Imagine Stuff
	::Scene				m_scene;

	Params				m_renderSettings;

	OutputImage*		m_pOutputImage;
	Raytracer*			m_pRaytracer;


// Nuke stuff
	DD::Image::Format	m_format;

	DD::Image::Lock		m_lock;
	bool				m_firstEngine;

	////

	// knob settings
	ProjectionType		m_projectionType;

	int					m_antiAliasing;
	bool				m_ambientOcclusion;

	int					m_globalIlluminationSamples;

	// cached values
	unsigned int		m_imageWidth;
	unsigned int		m_imageHeight;

	/// hashes
	DD::Image::Hash		m_cameraHash;
	DD::Image::Hash		m_lightsHash;
	DD::Image::Hash		m_geometryHash;
};

#endif // IMAGINE_RENDER_H

