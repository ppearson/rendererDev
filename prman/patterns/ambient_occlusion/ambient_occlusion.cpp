/*
 PRMan 19 RIS AmbientOcclusion pattern
 Copyright 2015 Peter Pearson.
 Contains Random class, taken from Intel's Embree library

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#include <limits>
 
#include <RixPattern.h>
 
#include <RixIntegrator.h>
 
class AmbientOcclusion : public RixPattern
{
public:
	AmbientOcclusion();
 
	// Taken from Intel's Embree
	class Random
	{
	public:
		Random(const int seed = 27)
		{
			setSeed(seed);
		}
 
		void setSeed(const int s)
		{
			const int a = 16807;
			const int m = 2147483647;
			const int q = 127773;
			const int r = 2836;
			int j, k;
 
			if (s == 0)
			 	seed = 1;
			else if (s < 0)
			 	seed = -s;
			else
			 	seed = s;
 
			for (j = 32 + 7; j >= 0; j--)
			{
				k = seed / q;
				seed = a * (seed - k*q) - r*k;
				if (seed < 0)
				 	seed += m;
				if (j < 32)
				 	table[j] = seed;
			}
			state = table[0];
		}
 
		int getInt()
		{
			const int a = 16807;
			const int m = 2147483647;
			const int q = 127773;
			const int r = 2836;
 
			int k = seed / q;
			seed = a*(seed - k*q) - r*k;
			if (seed < 0)
			 	seed += m;
			int j = state / (1 + (2147483647-1) / 32);
			state = table[j];
			table[j] = seed;
 
			return state;
		}
 
		int getInt(int limit)
		{
			return getInt() % limit;
		}
 
		float getFloat()
		{
			return std::min(getInt() / 2147483647.0f, 1.0f - std::numeric_limits<float>::epsilon());
		}
 
	private:
		int seed;
		int state;
		int table[32];
	};
 
	// both of these are annoyingly marked as pure...
	virtual int Init(RixContext& ctx, const char* pluginPath)
	{
		return 0;
	}

	virtual void Finalize(RixContext &ctx)
	{

	}

	virtual const RixSCParamInfo* GetParamTable();

	virtual int CreateInstanceData(RixContext& ctx, char const* handle, RixParameterList const* plist, InstanceData* idata);
 
	virtual int ComputeOutputParams(const RixShadingContext* sCtx, RtInt* numOutputs, OutputSpec** outputs,
									RtConstPointer instanceData, const RixSCParamInfo*);

	enum paramId
	{
		// outputs
		k_color = 0,
		k_mask,

		// params
		k_numSamples,
		k_maxTraceDistance,

		k_numParams
	};
 
protected:
	int         m_numSamples;
	float       m_maxTraceDistance;
};
 
AmbientOcclusion::AmbientOcclusion() : RixPattern(), m_numSamples(4), m_maxTraceDistance(100.0f)
{

}

const RixSCParamInfo* AmbientOcclusion::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs first
		RixSCParamInfo("color", k_RixSCColor, k_RixSCOutput),
		RixSCParamInfo("mask", k_RixSCFloat, k_RixSCOutput),
 
		// params
		RixSCParamInfo("numSamples", k_RixSCInteger),
		RixSCParamInfo("maxTraceDistance", k_RixSCFloat),

		RixSCParamInfo()
	};
 
	return& s_ptable[0];
}

int AmbientOcclusion::CreateInstanceData(RixContext& ctx, char const* handle, RixParameterList const* plist, InstanceData* idata)
{
	// TODO: evaluate the constant non-varying stuff here

	return 0;
}
 
int AmbientOcclusion::ComputeOutputParams(const RixShadingContext* sCtx, RtInt* numOutputs, OutputSpec** outputs,
										  RtConstPointer instanceData, const RixSCParamInfo* ignored)
{
	// Pixar should provide a helper/wrapper for this boilerplate stuff...

	// Find the number of outputs
	RixSCParamInfo const* paramTable = GetParamTable();
	int actualOutputs = -1;
	while (paramTable[++actualOutputs].access == k_RixSCOutput)
	{
	}

	// Allocate and bind our outputs
	RixShadingContext::Allocator pool(sCtx);
	OutputSpec* outputSpec = pool.AllocForPattern<OutputSpec>(actualOutputs);
	*outputs = outputSpec;
	*numOutputs = actualOutputs;

	// loop through the different output connection IDs, allocating memory for the connection
	// if it's got something attached to it
	for (int i = 0; i < actualOutputs; i++)
	{
		outputSpec[i].paramId = i;
		outputSpec[i].detail = k_RixSCInvalidDetail;
		outputSpec[i].value = NULL;

		RixSCType type = paramTable[i].type;
		RixSCConnectionInfo cinfo;
		sCtx->GetParamInfo(i, &type, &cinfo);
		if (cinfo == k_RixSCNetworkValue)
		{
			if (type == k_RixSCColor)
			{
				outputSpec[i].detail = k_RixSCVarying;
				outputSpec[i].value = pool.AllocForPattern<RtColorRGB>(sCtx->numPts);
			}
			else if (type == k_RixSCFloat)
			{
				outputSpec[i].detail = k_RixSCVarying;
				outputSpec[i].value = pool.AllocForPattern<RtFloat>(sCtx->numPts);
			}
		}
	}
 
	RtColorRGB* outputColor = (RtColorRGB*)outputSpec[k_color].value;
	RtFloat* outputMask = (RtFloat*)outputSpec[k_mask].value;

	// if nothing's connected, no point continuing
	if (!outputColor && !outputMask)
	{
		return 0;
	}
 
	const RtFloat3* P = NULL;
	const RtFloat* PRadius = NULL;
	const RtFloat3* Nn = NULL;
 
	sCtx->GetBuiltinVar(RixShadingContext::k_P, &P);
	sCtx->GetBuiltinVar(RixShadingContext::k_PRadius, &PRadius);
	sCtx->GetBuiltinVar(RixShadingContext::k_Nn, &Nn);
 
	const RtFloat* biasR = NULL;
	const RtFloat* biasT = NULL;
	sCtx->GetBuiltinVar(RixShadingContext::k_biasR, &biasR);
	sCtx->GetBuiltinVar(RixShadingContext::k_biasT, &biasT);
   
	const int* numSamples = NULL;
	sCtx->EvalParam(k_numSamples, -1, &numSamples, &m_numSamples, false);
	const float* maxTraceDistance = NULL;
	sCtx->EvalParam(k_maxTraceDistance, -1, &maxTraceDistance, &m_maxTraceDistance, false);

	// we're stratifying, so square the number to get our real count...
	const int numSamplesEdge = *numSamples;
	int numTotalSamples = numSamplesEdge * numSamplesEdge;
 
	float strataDelta = 1.0f / (float)numSamplesEdge;
 
	float rcpTotalSamples = 1.0f / (float)numTotalSamples;
 
	// annoyingly, we can't get access to the Random number generator context here,
	// so we have to provide our own random number, meaning we can't get samples as uncorrelated
	// as we would like...
	Random rng(rand()); // Note: on older Linux versions (CentOS 4), this has a lock around it, but on
						//       CentOS 5+ we seem to be okay.
 
	RtRayGeometry* occrays = pool.AllocForPattern<RtRayGeometry>(numTotalSamples);
	RtHitPoint* occhits = pool.AllocForPattern<RtHitPoint>(numTotalSamples);

	if (sCtx->scTraits.shadingMode == k_RixSCPresenceQuery || sCtx->scTraits.shadingMode == k_RixSCOpacityQuery)
	{
		// if we're a presence or opacity query, don't bother doing anything, and early-out

		for (int pt = 0; pt < sCtx->numPts; pt++)
		{
			if (outputColor)
			{
				outputColor[pt] = RtColorRGB(0.0f);
			}

			if (outputMask)
			{
				outputMask[pt] = 0.0f;
			}
		}

		return 0;
	}

	float dummy = 0.0f;
 
	for (int pt = 0; pt < sCtx->numPts; pt++)
	{
		// for each shading point, send out the required number of samples in a hemisphere over the
		// shading point, orientated to the normal
		int rayCount = 0;
 
		for (int x = 0; x < numSamplesEdge; x++)
		{
			const float floatX = (float)x * strataDelta + (rng.getFloat() * strataDelta);
			for (int y = 0; y < numSamplesEdge; y++)
			{
				const float floatY = (float)y * strataDelta + (rng.getFloat() * strataDelta);
 
				RixUniformDirectionalDistribution(RtFloat2(floatX, floatY), Nn[pt], occrays[rayCount].direction, dummy);
				occrays[rayCount].direction.Normalize();
				occrays[rayCount].origin = P[pt];
				occrays[rayCount].origin = RixApplyTraceBias(P[pt], Nn[pt], occrays[rayCount].direction, biasR[pt], biasT[pt]);
 
				// for the moment, hard-code a max distance of the far clipping plane - TODO: get the actual value
				occrays[rayCount].maxDist = RixMin(*maxTraceDistance, 1000.0f);
				occrays[rayCount].raySpread = 0.00001f;
				occrays[rayCount].originRadius = PRadius[pt];
 
				occrays[rayCount].InitTransmitOrigination(sCtx, pt);
 
				rayCount++;
			}
		}

		sCtx->GetNearestHits(rayCount, occrays, occhits, NULL);
 
		// if dist == 0 then nothing was hit for that point
 
		// now work out the ratio of how many sample rays were occluded
		float occlusionRatio = 0.0f;
 
		for (int i = 0; i < rayCount; i++)
		{
			if (occhits[i].dist > 0.0f)
			{
				occlusionRatio += 1.0f;
			}
		}

		const float occlusionValue = 1.0f - (occlusionRatio * rcpTotalSamples);
 
		if (outputColor)
		{
			outputColor[pt] = RtColorRGB(occlusionValue);
		}

		if (outputMask)
		{
			outputMask[pt] = occlusionValue;
		}
	}
 
	return 0;
}
 
RIX_PATTERNCREATE
{
	return new AmbientOcclusion();
}
 
 
RIX_PATTERNDESTROY
{
	delete ((AmbientOcclusion*)pattern);
}
