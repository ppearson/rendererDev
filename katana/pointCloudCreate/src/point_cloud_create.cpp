/*
 PointCloudCreate
 Copyright 2018 Peter Pearson.

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

#include "point_cloud_create.h"

#include <sstream>
#include <cmath>

#include <stdio.h>

static std::vector<Vec3>	aPositions;

void PointCloudCreateOp::setup(Foundry::Katana::GeolibSetupInterface& interface)
{
	interface.setThreading(Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
}

void PointCloudCreateOp::cook(Foundry::Katana::GeolibCookInterface& interface)
{
	if (interface.atRoot())
	{
		interface.stopChildTraversal();
	}

	// find c argument
	FnAttribute::GroupAttribute cGroupAttr = interface.getOpArg("c");
	if (cGroupAttr.isValid())
	{
		if (cGroupAttr.getNumberOfChildren() != 1)
		{
			Foundry::Katana::ReportError(interface, "Invalid attributes.");
			interface.stopChildTraversal();
			return;
		}

		std::string childName = FnAttribute::DelimiterDecode(cGroupAttr.getChildName(0));
		FnAttribute::GroupAttribute childArgs = cGroupAttr.getChildByIndex(0);
		interface.createChild(childName, "", childArgs);
		return;
	}

	FnAttribute::GroupAttribute aGroupAttr = interface.getOpArg("a");
	if (aGroupAttr.isValid())
	{
		FnAttribute::IntAttribute numInstancesAttr = aGroupAttr.getChildByName("numPoints");

		int64_t numPoints = numInstancesAttr.getValue(0, false);
		if (numPoints == 0)
		{
			return;
		}
		
		FnAttribute::FloatAttribute pointWidthAttr = aGroupAttr.getChildByName("pointWidth");
		float pointConstantWidth = pointWidthAttr.getValue(0.1f, false);
		
		Vec3 areaSpread(20.0f, 20.0f, 20.0f);
		FnAttribute::FloatAttribute areaSpreadAttr = aGroupAttr.getChildByName("areaSpread");
		if (areaSpreadAttr.isValid())
		{
			FnKat::FloatConstVector data = areaSpreadAttr.getNearestSample(0);
		
			areaSpread.x = data[0];
			areaSpread.y = data[1];
			areaSpread.z = data[2];
		}
				
		// we're dangerously assuming that this will be run first...
		create3DGrid(numPoints, areaSpread, aPositions);
						
		FnAttribute::GroupBuilder geoGb;
		
		FnAttribute::GroupBuilder pointGb;
		
		const float* pData = &aPositions.data()->x;
		
		pointGb.set("P", FnAttribute::FloatAttribute(pData, numPoints * 3, 3));
		
		pointGb.set("constantwidth", FnAttribute::FloatAttribute(pointConstantWidth));
		
		geoGb.set("point", pointGb.build());		
		
		interface.setAttr("geometry", geoGb.build());
		
		interface.setAttr("type", FnAttribute::StringAttribute("pointcloud"));			

		interface.stopChildTraversal();
		
		return;
	}
}

void PointCloudCreateOp::create3DGrid(int64_t numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next cube number, so we get a good even distribution for both X, Y and Z
	int64_t edgeCount = (int64_t)(cbrtf((float)numItems));
	if (edgeCount * edgeCount * edgeCount < numItems)
		edgeCount += 1;

	int64_t fullItemCount = edgeCount * edgeCount * edgeCount;

	int64_t extra = fullItemCount - numItems;

	// TODO: could do resize() here and then just set the members of each item
	//       directly, which might be more efficient...
	aItemPositions.reserve(numItems);	
	
	float edgeDelta = 1.0f / (float)edgeCount;
	
	std::vector<float> xPositions(edgeCount);
	std::vector<float> yPositions(edgeCount);
	std::vector<float> zPositions(edgeCount);
	
	for (unsigned int i = 0; i < edgeCount; i++)
	{
		float samplePos = (float(i) * edgeDelta) - 0.5f;
		
		xPositions[i] = samplePos * areaSpread.x;
		yPositions[i] = samplePos * areaSpread.y;
		zPositions[i] = samplePos * areaSpread.z;
	}
	
	if (extra > 0)
	{
		// we need to skip certain items
		
		int skipStride = fullItemCount / extra;
		
		int count = 0;
		for (unsigned int xInd = 0; xInd < edgeCount; xInd++)
		{
			const float xPos = xPositions[xInd];
			for (unsigned int yInd = 0; yInd < edgeCount; yInd++)
			{
				float yPos = yPositions[yInd];
				for (unsigned int zInd = 0; zInd < edgeCount; zInd++)
				{
					if (extra > 0 && ++count >= skipStride)
					{
						count = 0;
						continue;
					}
					
					float zPos = zPositions[zInd];
		
					aItemPositions.push_back(Vec3(xPos, yPos, zPos));
				}
			}
		}
	}
	else
	{
		for (unsigned int xInd = 0; xInd < edgeCount; xInd++)
		{
			const float xPos = xPositions[xInd];
			for (unsigned int yInd = 0; yInd < edgeCount; yInd++)
			{
				float yPos = yPositions[yInd];
				
				for (unsigned int zInd = 0; zInd < edgeCount; zInd++)
				{
					float zPos = zPositions[zInd];
					aItemPositions.push_back(Vec3(xPos, yPos, zPos));
				}
			}
		}
	}
}

DEFINE_GEOLIBOP_PLUGIN(PointCloudCreateOp)

void registerPlugins()
{
	REGISTER_PLUGIN(PointCloudCreateOp, "PointCloudCreate", 0, 1);
}
