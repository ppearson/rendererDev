#include <cmath>

#include "DDImage/DDMath.h"
#include "DDImage/Iop.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"

#include "DDImage/CameraOp.h"
#include "DDImage/LightOp.h"
#include "DDImage/GeoOp.h"
#include "DDImage/Format.h"
#include "DDImage/Thread.h"
#include "DDImage/Scene.h"
#include "DDImage/ImagePlane.h"

/// imagine
#include "scene.h"
#include "objects/camera.h"
#include "objects/mesh.h"
#include "objects/primitives/sphere.h"
#include "lights/point_light.h"
#include "lights/spot_light.h"
#include "geometry/standard_geometry_instance.h"
#include "geometry/standard_geometry_operations.h"
#include "materials/standard_material.h"
#include "raytracer/raytracer.h"
#include "utils/system.h"
#include "utils/params.h"
#include "image/output_image.h"
#include "image/image_colour3f.h"
#include "textures/image/image_texture_3f.h"

#include "global_context.h"
#include "output_context.h"

using namespace DD::Image;

static const char* const CLASS = "imagine_render";
static const char* const HELP = "Imagine_render";

const char* const RENDER_TYPE[] =
{
	"Global illumination",
	"Direct illumination",
	0
};

class ImagineRenderIop : public Iop
{
public:
	enum eRenderType
	{
		eGlobalIllumination,
		eDirectIllumination
	};

	ImagineRenderIop(Node* node);

	virtual void knobs(Knob_Callback callback);
	virtual void _validate(bool for_real);
	virtual void _request(int x, int y, int r, int t, DD::Image::ChannelMask m, int n);

	virtual void append(DD::Image::Hash &hash);

	virtual void engine(int y, int x, int r, ChannelMask channels, DD::Image::Row& out);

	const char* input_label(int n, char*) const;
	virtual bool test_input(int index, Op* pOp) const;

	int minimum_inputs() const
	{
		return 3;
	}
	int maximum_inputs() const
	{
		return 3;
	}

	void buildScene();

	const char* Class() const { return CLASS; }
	const char* node_help() const { return HELP; }
	const char* displayName() const { return "ImagineRender"; }
	static const Iop::Description d;

protected:

	::Scene			m_scene;

	Params			m_renderSettings;

	OutputImage*	m_pOutputImage;
	Raytracer*		m_pRaytracer;

	Channel			m_channels[4];
	FormatPair		m_formats;

	DD::Image::Lock	m_lock;
	bool			m_firstEngine;

	////

	eRenderType		m_renderType;



	bool			m_ambientOcclusion;
	bool			m_antiAliasing;

	int				m_globalIlluminationSamples;

	unsigned int	m_imageWidth;
	unsigned int	m_imageHeight;

	/// hashes
	DD::Image::Hash		m_cameraHash;
	DD::Image::Hash		m_lightsHash;
	DD::Image::Hash		m_geometryHash;
};


//// class impl

ImagineRenderIop::ImagineRenderIop(Node* node) : Iop(node), m_pOutputImage(NULL), m_pRaytracer(NULL), m_firstEngine(true),
	m_renderType(eDirectIllumination), m_ambientOcclusion(false), m_antiAliasing(false), m_globalIlluminationSamples(64)
{
	m_cameraHash = 0;
	m_lightsHash = 0;
	m_geometryHash = 0;
}

void ImagineRenderIop::knobs(Knob_Callback callback)
{
	Channel_knob(callback, m_channels, 4, "channels");
	Format_knob(callback, &m_formats, "format");

	Divider(callback, "render settings");

	Enumeration_knob(callback, (int*)&m_renderType, RENDER_TYPE, "render_type", "render type");
	Newline(callback);

	Bool_knob(callback, &m_ambientOcclusion, "ambient_occlusion", "ambient occlusion");
	Bool_knob(callback, &m_antiAliasing, "anti_aliasing", "anti aliasing");

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

	info_.full_size_format(*m_formats.fullSizeFormat());
	info_.format(*m_formats.format());
	info_.channels(Mask_RGBA);
	info_.set(format());

	m_imageWidth = m_formats.format()->width();
	m_imageHeight = m_formats.format()->height();

	m_renderSettings.add("width", m_imageWidth);
	m_renderSettings.add("height", m_imageHeight);

	unsigned int flags = COMPONENT_RGBA;
/*
	if (m_renderType == eDirectIllumination)
	{
		if (m_ambientOcclusion)
		{
			settings.ambientOcclusion = true;
			settings.ambientOcclusionSamples = 7;
			settings.ambientRandomJitter = true;
		}

		settings.antiAliasing = m_antiAliasing ? 3 : 1;
		settings.globalIllumination = false;
	}
	else
	{
		flags |= COMPONENT_DEPTH | COMPONENT_SAMPLES;
		settings.globalIlluminationIterations = 1;
		settings.globalIlluminationSamplesPerIteration = m_globalIlluminationSamples;
	}
*/
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


	unsigned int threads = System::getNumberOfCores();

	m_pRaytracer = new Raytracer(m_scene, m_pOutputImage, m_renderSettings, false, threads);
	m_pRaytracer->setAmbientColour(Colour3f(0.35f, 0.35f, 0.35f));
	m_pRaytracer->setExtraChannels(0);
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

void ImagineRenderIop::buildScene()
{
	Op* pInput0 = input(0);
	Op* pInput1 = input(1);
	Op* pInput2 = input(2);

	if (dynamic_cast<CameraOp*>(pInput0))
	{
		CameraOp* pCamera = (CameraOp*)pInput0;

		m_scene.freeCamera();

		DD::Image::Knob* pTrans = pCamera->knob("translate");
		DD::Image::Knob* pRotate = pCamera->knob("rotate");

		Point position(pTrans->get_value(0), pTrans->get_value(1), pTrans->get_value(2));
		Vector rotation(pRotate->get_value(0), pRotate->get_value(1), pRotate->get_value(2));

		Camera* pImagineCamera = new Camera(position);
		pImagineCamera->setRotation(rotation);

		DD::Image::Knob* pFocalLength = pCamera->knob("focal");
		if (pFocalLength)
		{
			float focalLength = pFocalLength->get_value(0);

			pImagineCamera->setFOV(90.0f - focalLength);
		}

		m_scene.setViewCamera(pImagineCamera);
	}

	DD::Image::Hash geometryHash = -1;

	if (dynamic_cast<GeoOp*>(pInput2))
	{
		GeoOp* pGeo = (GeoOp*)pInput2;

		/// construct Scene

		DD::Image::Scene nukeScene;
		pGeo->build_scene(nukeScene);

		DD::Image::Scene scene2;
		GeometryList geoList;
		pGeo->get_geometry(scene2, geoList);

		if (geoList.size() == 0)
			return;

		geometryHash.append(geoList.size());
		geometryHash.append(pGeo->hash(DD::Image::Group_Points));
	}


	// now add the lights
	m_scene.freeLights();

	if (dynamic_cast<LightOp*>(pInput1))
	{
		LightOp* pLight = (LightOp*)pInput1;

		DD::Image::Knob* pTrans = pLight->knob("translate");
		DD::Image::Knob* pRotate = pLight->knob("rotate");

		Point position(pTrans->get_value(0), pTrans->get_value(1), pTrans->get_value(2));
		Vector rotation(pRotate->get_value(0), pRotate->get_value(1), pRotate->get_value(2));

		PointLight* pNewLight = new PointLight();
		pNewLight->setPosition(position);
		pNewLight->setRotation(rotation);

		m_scene.addObject(pNewLight, false, false);
	}

	if (dynamic_cast<GeoOp*>(pInput2))
	{
		GeoOp* pGeo = (GeoOp*)pInput2;

		DD::Image::Scene nukeScene;
		pGeo->build_scene(nukeScene);

		std::vector<LightContext*>::iterator itLight = nukeScene.lights.begin();
		for (; itLight != nukeScene.lights.end(); ++itLight)
		{
			LightContext* pLightC = *itLight;

			LightOp* pLight = pLightC->light();

			DD::Image::Knob* pTrans = pLight->knob("translate");
			DD::Image::Knob* pRotate = pLight->knob("rotate");

			Point position(pTrans->get_value(0), pTrans->get_value(1), pTrans->get_value(2));
			Vector rotation(pRotate->get_value(0), pRotate->get_value(1), pRotate->get_value(2));

			Light* pNewLight = NULL;
			bool spot = false;
			DD::Image::Knob* pType = pLight->knob("light_type");
			if (pType)
			{
				float lightType = pType->get_value(0);
				if (lightType == 2.0f)
					spot = true;
			}

			if (spot)
			{
				float coneAngle = 40.0f;
				float penAngle = 7.0f;
				DD::Image::Knob* pConeAngle = pLight->knob("cone_angle");
				if (pConeAngle)
					coneAngle = pConeAngle->get_value();
				DD::Image::Knob* pPenAngle = pLight->knob("cone_penumbra_angle");
				if (pPenAngle)
					penAngle = pPenAngle->get_value();

				if (coneAngle > 57.0f)
					coneAngle = 56.0f;

				SpotLight* pSP = new SpotLight();
				pSP->setConeAngle(coneAngle);
				pSP->setPenumbraAngle(coneAngle + penAngle);

				pNewLight = static_cast<Light*>(pSP);

				::Matrix4 spotRotate;
				spotRotate.setRotationX(90.0f);

				rotation = spotRotate.transform(rotation);
			}
			else
			{
				pNewLight = new PointLight();
			}

			pNewLight->setPosition(position);
			pNewLight->setRotation(rotation);

			DD::Image::Knob* pIntensity = pLight->knob("intensity");
			float intensity = pIntensity->get_value();

			pNewLight->setIntensity(intensity / 4);

			m_scene.addObject(pNewLight, false, false);
		}
	}

	bool do1 = true;
	// check if scene has updated...
	if (do1)
	{
		if (dynamic_cast<GeoOp*>(pInput2))
		{
			GeoOp* pGeo = (GeoOp*)pInput2;

			m_scene.freeGeometry();
			/// construct Scene

			DD::Image::Scene nukeScene;
			pGeo->build_scene(nukeScene);

			DD::Image::Scene scene2;
			GeometryList geoList;
			pGeo->get_geometry(scene2, geoList);

			if (geoList.size() == 0)
				return;

			StandardMaterial* pNewMat = NULL;

			for (unsigned int objIndex = 0; objIndex < geoList.size(); objIndex++)
			{
				DD::Image::GeoInfo& object = geoList.object(objIndex);

				bool haveUVs = false;

				const AttribContext* pUVs = object.get_attribcontext("uv");
				// check we've got valid UVs
				if (pUVs && pUVs->attribute && pUVs->attribute->size())
					haveUVs = true;



				if (object.material)
				{
					// create a new material type which reads from an Iop...

					Iop* pNukeMaterial = object.material;

					pNukeMaterial->request(Mask_RGBA, 1);

					bool haveTexture = true;

					// see if we've got a default material
					float red = pNukeMaterial->at(0, 0, Chan_Red);
					float green = pNukeMaterial->at(0, 0, Chan_Green);
					float blue = pNukeMaterial->at(0, 0, Chan_Blue);

					if (red == 0.0f && green == 0.0f && blue == 0.0f)
					{
						haveTexture = false;
					}

					if (haveUVs && haveTexture)
					{
						unsigned int width = pNukeMaterial->info().w();
						unsigned int height = pNukeMaterial->info().h();
						DD::Image::Box bbox(0, 0, width, height);

						DD::Image::ImagePlane newImagePlane(bbox, false, Mask_RGB, 3);

						// make a texture
						pNukeMaterial->fetchPlane(newImagePlane);

						const float* pRawValues = newImagePlane.readable();

						ImageColour3f* pNewImage = new ImageColour3f(width, height);

						for (unsigned int y = 0; y < height; y++)
						{
							for (unsigned int x = 0; x < width; x++)
							{
								float red = *pRawValues++;
								float green = *pRawValues++;
								float blue = *pRawValues++;

								Colour3f& imagePixel = pNewImage->colourAt(x, y);

								imagePixel.r = red;
								imagePixel.g = green;
								imagePixel.b = blue;
							}
						}

						pNewMat = new StandardMaterial();

						if (pNewImage)
						{
							ImageTexture3f* pImageTexture = new ImageTexture3f(pNewImage, width, height);
							pNewMat->setDiffuseColourManualTexture(pImageTexture);
						}
					}
					else
					{
						float alpha = pNukeMaterial->at(0, 0, Chan_Alpha);

						// hack to check for null input - Nuke's creating a default black Iop when there isn't one...

						if (red == 0.0f && green == 0.0f && blue == 0.0f)
						{
							pNewMat = new StandardMaterial();
							pNewMat->setDiffuseColour(Colour3f(0.7f, 0.7f, 0.7f));

							pNewMat->setReflection(0.5f);
						}
						else
						{
							pNewMat = new StandardMaterial();
							pNewMat->setDiffuseColour(Colour3f(red, green, blue));

				/*			if (alpha < 1.0f)
							{
								pNewMat->setTransparancy(1.0f - alpha);
							}
				*/		}
					}
				}
				else
				{
					pNewMat = new StandardMaterial();
					pNewMat->setDiffuseColour(Colour3f(0.7f, 0.7f, 0.7f));

					pNewMat->setReflection(0.5f);
				}

				Material* pMat = static_cast<Material*>(pNewMat);

				StandardGeometryInstance* pNewGeomInstance = new StandardGeometryInstance();

				std::vector<Point>& points = pNewGeomInstance->getPoints();

				const PointList* geomPoints = object.point_list();
				unsigned int numPoints = geomPoints->size();

				for (unsigned int pointIndex = 0; pointIndex < numPoints; pointIndex++)
				{
					const Vector3& localPoint = (*geomPoints)[pointIndex];

					Vector4 worldPoint = object.matrix * localPoint;

					points.push_back(Point(worldPoint.x, worldPoint.y, worldPoint.z));
				}

				// check we've got valid UVs
				if (haveUVs)
				{
					int uvGroupType = pUVs->group;
					unsigned int numUVs = pUVs->attribute->size();

					std::vector<UV>& uvs = pNewGeomInstance->getUVs();

					for (unsigned int uvIndex = 0; uvIndex < numUVs; uvIndex++)
					{
						DD::Image::Vector4& sourceUV = pUVs->attribute->vector4(uvIndex);
						UV newUV(sourceUV.x, sourceUV.y);

						uvs.push_back(newUV);
					}
				}

				unsigned int numFaces = object.primitives();

				std::vector<uint32_t>& aPolyOffsets = pNewGeoInstance->getPolygonOffsets();
				aPolyOffsets.reserve(numFaces);

				std::vector<uint32_t>& aPolyIndices = pNewGeoInstance->getPolygonIndices();
				aPolyIndices.resize(numIndices);

				for (unsigned int faceIndex = 0; faceIndex < numFaces; faceIndex++)
				{
					const DD::Image::Primitive* prim = object.primitive(faceIndex);
					unsigned int numVertices = prim->vertices();

					unsigned int numSubFaces = prim->faces();
					if (numSubFaces == 1)
					{
						Face newFace(numVertices);

						for (unsigned int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
						{
							unsigned int thisVertexIndex = prim->vertex(vertexIndex);

							newFace.addVertex(thisVertexIndex);
							newFace.addUV(thisVertexIndex);
						}

						newFace.calculateNormal(pNewGeomInstance);

						faces.push_back(newFace);
					}
					else
					{
						for (unsigned int subFace = 0; subFace < numSubFaces; subFace++)
						{
							unsigned int numSubFaceVertices = prim->face_vertices(subFace);
							unsigned int* pSubFaceVertices = new unsigned int[numSubFaceVertices];
							prim->get_face_vertices(subFace, pSubFaceVertices);

							Face newFace(numSubFaceVertices);

							for (unsigned int vertexIndex = 0; vertexIndex < numSubFaceVertices; vertexIndex++)
							{
								unsigned int thisVertexIndex = pSubFaceVertices[numSubFaceVertices - 1 - vertexIndex];

								newFace.addVertex(thisVertexIndex);
								newFace.addUV(thisVertexIndex);
							}

							if (pSubFaceVertices)
								delete pSubFaceVertices;

							newFace.calculateNormal(pNewGeomInstance);

							faces.push_back(newFace);
						}
					}
				}

				pNewGeomInstance->setHasPerVertexUVs(haveUVs);

				Mesh* pMesh = new Mesh();

				pMesh->setGeometryInstance(pNewGeomInstance);
				pMesh->setMaterial(pMat);

				m_scene.addObject(pMesh, false, false);
			}
		}

		m_geometryHash = geometryHash;
	}
}

void ImagineRenderIop::engine(int y, int x, int r, ChannelMask channels, DD::Image::Row& out)
{
	if (m_firstEngine)
	{
		DD::Image::Guard guard(m_lock);
		if (m_firstEngine)
		{
			GlobalContext::instance().setWorkerThreads(8);

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
		case 1: return "Light";
		default: return "Scene";
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
			return dynamic_cast<LightOp*>(pOp) != NULL;
			break;
		case 2:
			return dynamic_cast<GeoOp*>(pOp) != NULL;
			break;
	}

	return false;
}

static Iop* build(Node* node)
{
	return new ImagineRenderIop(node);
}

const Iop::Description ImagineRenderIop::d(CLASS, "imagine_render", build);
