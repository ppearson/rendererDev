// Fisheye projection plugin (Equidistant)

#include <cmath>

#include <RixProjection.h>
#include <RixIntegrator.h>
#include <RixShadingUtils.h>

class FisheyeProjection : public RixProjection
{
public:
	FisheyeProjection();

	virtual ~FisheyeProjection();

	virtual int Init(RixContext& ctx, char const* pluginPath);

	virtual const RixSCParamInfo* GetParamTable();

	virtual void Finalize(RixContext& ctx);

	virtual void RenderBegin(RixContext& ctx, RixProjectionEnvironment& env, const RixParameterList* parms);

	virtual void RenderEnd(RixContext& ctx);

	virtual void Synchronize(RixContext& ctx, RixSCSyncMsg syncMsg, const RixParameterList* parms);

	virtual void Project(RixProjectionContext& pCtx);

protected:

	float	m_fov;
	float	m_fovRadians;

	float	m_screenWidth;
	float	m_screenHeight;

	float	m_invScreenWidth;
	float	m_invScreenHeight;

	float	m_stepXY;
};


/////

FisheyeProjection::FisheyeProjection()
{

}

FisheyeProjection::~FisheyeProjection()
{

}

int FisheyeProjection::Init(RixContext& ctx, char const* pluginPath)
{
	return 0;
}

const RixSCParamInfo* FisheyeProjection::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("fov", k_RixSCFloat),
		RixSCParamInfo() // end of table
	};
	return &s_ptable[0];
}

void FisheyeProjection::Finalize(RixContext& ctx)
{

}

void FisheyeProjection::RenderBegin(RixContext& ctx, RixProjectionEnvironment& env, const RixParameterList* parms)
{
	RixRenderState& state = *reinterpret_cast<RixRenderState*>(ctx.GetRixInterface(k_RixRenderState));
	RixRenderState::Type fmtType;
	RtInt resCnt;
	struct { RtFloat fstop, flength, fdistance; } dof;
	struct { RtFloat left, right, top, bottom; } window;
	struct { RtFloat width, height, pixelAspect; } format;
	state.GetOption("Ri:DepthOfField", &dof, 3 * sizeof(RtFloat), &fmtType, &resCnt);
	state.GetOption("Ri:ScreenWindow", &window, 4 * sizeof(RtFloat), &fmtType, &resCnt);
	state.GetOption("DeviceResolution", &format, 3 * sizeof(RtFloat), &fmtType, &resCnt);
	m_screenWidth = window.right - window.left;
	m_screenHeight = window.top - window.bottom;
	if (m_screenWidth == 0.0f || m_screenHeight == 0.0f)
	{
		float aspect = format.width * format.pixelAspect / format.height;
		m_screenWidth = std::max(2.0f, 2.0f * aspect);
		m_screenHeight = std::max(2.0f, 2.0f / aspect);
	}
	float xStep = 0.25f * m_screenWidth / format.width;
	float yStep = 0.25f * m_screenHeight / format.height;
	m_stepXY = std::max(xStep, yStep);

	m_fov = 180.0f;
	m_fovRadians = m_fov * F_DEGTORAD;

	m_invScreenWidth = 1.0f / m_screenWidth;
	m_invScreenHeight = 1.0f / m_screenHeight;

	Synchronize(ctx, k_RixSCInstanceEdit, parms);
}

void FisheyeProjection::RenderEnd(RixContext& ctx)
{

}

void FisheyeProjection::Synchronize(RixContext& ctx, RixSCSyncMsg syncMsg, const RixParameterList* parms)
{
	if (syncMsg != k_RixSCInstanceEdit)
			return;

	RtInt paramId;

	if (parms->GetParamId("fov", &paramId) == 0)
	{
		parms->EvalParam(paramId, 0, &m_fov);

		m_fovRadians = m_fov * F_DEGTORAD;
	}
}

void FisheyeProjection::Project(RixProjectionContext& pCtx)
{
	for (int i = 0; i < pCtx.numRays; i++)
	{
		RtPoint2& screen(pCtx.screen[i]);
		RtRayGeometry& ray(pCtx.rays[i]);

		ray.origin = RtPoint3(0.0f, 0.0f, 0.0f);

		// thankfully, we already have the sample position within -1.0f - 1.0f range fisheye needs...

		// work out unit circle
		float rad = (screen.x * screen.x) + (screen.y * screen.y);
		rad = std::sqrt(rad);

		if (rad > 1.0f)
		{
			// we can't sample this direction, so set it to upwards, which causes PRMan to discard it
			ray.direction = RtVector3(0.0f, 0.0f, 0.0f);
			continue;
		}
		else
		{
			float phi = rad * m_fovRadians * 0.5f;
			float theta = std::atan2(screen.y, screen.x);

			float sinPhi = std::sin(phi);
			ray.direction = RtVector3(sinPhi * std::cos(theta), sinPhi * std::sin(theta), std::cos(phi));
		}

		ray.direction.Normalize();
		ray.originRadius = 0.0f;
		// TODO...
		ray.raySpread = 0.035f * ray.direction.z;
	}
}

RIX_PROJECTIONCREATE
{
	return new FisheyeProjection();
}

RIX_PROJECTIONDESTROY
{
	delete reinterpret_cast<FisheyeProjection*>(projection);
}
