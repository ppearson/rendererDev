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

// positions vector for split locations
static std::vector<Vec3>	g_aPositions;

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

		FnAttribute::IntAttribute splitPointsLocationsAttr = aGroupAttr.getChildByName("splitPointcloudLocations");
		bool splitPointsLocations = false;
		if (splitPointsLocationsAttr.isValid())
		{
			splitPointsLocations = splitPointsLocationsAttr.getValue(0, false) == 1;
		}

		if (!splitPointsLocations)
		{
			// we want everything in one single location
			float pointConstantWidth = 0.1f;
			std::vector<float> aPointWidths;
			int pointWidthType = 0;
			FnAttribute::IntAttribute pointWidthTypeAttr = aGroupAttr.getChildByName("pointWidthType");
			if (pointWidthTypeAttr.isValid())
			{
				pointWidthType = pointWidthTypeAttr.getValue(0, false);
			}
			if (pointWidthType == 0 || pointWidthType == 1)
			{
				// it's constant types
				FnAttribute::FloatAttribute constantPointWidthAttr = aGroupAttr.getChildByName("constantPointWidth");
				if (constantPointWidthAttr.isValid())
				{
					pointConstantWidth = constantPointWidthAttr.getValue(0.1f, false);
				}

				if (pointWidthType == 1)
				{
					// fill vector with same value
					aPointWidths.resize(numPoints, pointConstantWidth);
				}
			}
			else if (pointWidthType == 2)
			{
				// it's random (but seeded) between min and max
				FnAttribute::FloatAttribute randomPointWidthMinAttr = aGroupAttr.getChildByName("randomPointWidthMin");
				FnAttribute::FloatAttribute randomPointWidthMaxAttr = aGroupAttr.getChildByName("randomPointWidthMax");
				if (randomPointWidthMinAttr.isValid() && randomPointWidthMaxAttr.isValid())
				{
					float randomPointWidthMinValue = randomPointWidthMinAttr.getValue(0.1f, false);
					float randomPointWidthMaxValue = randomPointWidthMaxAttr.getValue(0.2f, false);
					float delta = randomPointWidthMaxValue - randomPointWidthMinValue;
					float numValues = 10.0f;
					float jumpValue = delta / numValues;
					// TODO: proper random numbers?

					float widthValue = randomPointWidthMinValue;

					unsigned int bounceCount = 0;
					aPointWidths.reserve(numPoints);
					for (int64_t i = 0; i < numPoints; i++)
					{
						aPointWidths.push_back(widthValue);

						// TODO: proper bounce up and down...
						if (bounceCount++ < 10)
						{
							widthValue += jumpValue;
						}
						else
						{
							bounceCount = 0;
							widthValue = randomPointWidthMinValue;
						}
					}
				}
			}

			Vec3 areaSpread(20.0f, 20.0f, 20.0f);
			FnAttribute::FloatAttribute areaSpreadAttr = aGroupAttr.getChildByName("areaSpread");
			if (areaSpreadAttr.isValid())
			{
				FnKat::FloatConstVector data = areaSpreadAttr.getNearestSample(0);

				areaSpread.x = data[0];
				areaSpread.y = data[1];
				areaSpread.z = data[2];
			}

			std::vector<Vec3> aPositions;
			create3DGrid(numPoints, areaSpread, aPositions);

			FnAttribute::GroupBuilder geoGb;

			FnAttribute::GroupBuilder pointGb;

			const float* pData = &aPositions.data()->x;

			pointGb.set("P", FnAttribute::FloatAttribute(pData, numPoints * 3, 3));

			if (pointWidthType == 0)
			{
				pointGb.set("constantwidth", FnAttribute::FloatAttribute(pointConstantWidth));
			}
			else if (aPointWidths.size() == numPoints)
			{
				pointGb.set("width", FnAttribute::FloatAttribute(aPointWidths.data(), numPoints, 1));
			}

			geoGb.set("point", pointGb.build());

			bool needArbitrary = false;

			FnAttribute::GroupBuilder arbitraryGb;

			FnAttribute::IntAttribute extraFloatPrimvarTypeAttr = aGroupAttr.getChildByName("extraFloatPrimvarType");
			int extraFloatPrimvarTypeValue = extraFloatPrimvarTypeAttr.getValue(0, false);
			if (extraFloatPrimvarTypeValue > 0)
			{
			    FnAttribute::GroupBuilder floatPrimvarGb;

			    floatPrimvarGb.set("scope", FnAttribute::StringAttribute("point"));
			    floatPrimvarGb.set("elementSize", FnAttribute::IntAttribute(1));

			    std::vector<float> primvarValues;
				if (extraFloatPrimvarTypeValue == 1)
				{
					primvarValues.resize(numPoints, 42.0f);
				}
				else
				{
					primvarValues.resize(numPoints);
					float randomValue = 1.3f;
					unsigned int bounceCount = 0;
					for (int64_t i = 0; i < numPoints; i++)
					{
						float& fValue = primvarValues[i];
						if (bounceCount++ < 10)
						{
							fValue = randomValue;
							randomValue += 0.7f;
						}
						else
						{
							bounceCount = 0;
							fValue = randomValue;
							randomValue = 1.3f;
						}
					}
				}
			    floatPrimvarGb.set("value", FnAttribute::FloatAttribute(primvarValues.data(), numPoints, 1));

				arbitraryGb.set("extraFloatPrimvar", floatPrimvarGb.build());
				needArbitrary = true;
			}

			FnAttribute::IntAttribute extraVectorPrimvarTypeAttr = aGroupAttr.getChildByName("extraVectorPrimvarType");
			int extraVectorPrimvarTypeValue = extraVectorPrimvarTypeAttr.getValue(0, false);
			if (extraVectorPrimvarTypeValue > 0)
			{
			    FnAttribute::GroupBuilder vectorPrimvarGb;

			    vectorPrimvarGb.set("scope", FnAttribute::StringAttribute("point"));
				// TODO: fix this
			    vectorPrimvarGb.set("elementSize", FnAttribute::IntAttribute(3)); // should really be 1 with outputType set...
	//			vectorPrimvarGb.set("outputType", FnAttribute::StringAttribute("vector3"));

				std::vector<float> primvarValues;
				if (extraVectorPrimvarTypeValue == 1)
				{
					primvarValues.resize(numPoints * 3, 0.42f);
				}
				else
				{
					primvarValues.resize(numPoints * 3);
					float randomValue = 0.3f;
					unsigned int bounceCount = 0;
					for (int64_t i = 0; i < numPoints * 3; i++)
					{
						float& fValue = primvarValues[i];
						if (bounceCount++ < 20)
						{
							fValue = randomValue;
							randomValue += 0.05f;
						}
						else
						{
							bounceCount = 0;
							fValue = randomValue;
							randomValue = 0.3f;
						}
					}
				}

			    vectorPrimvarGb.set("value", FnAttribute::FloatAttribute(primvarValues.data(), numPoints * 3, 3));

				arbitraryGb.set("extraVectorPrimvar", vectorPrimvarGb.build());
				needArbitrary = true;
			}

			if (needArbitrary)
			{
				geoGb.set("arbitrary", arbitraryGb.build());
			}

			interface.setAttr("geometry", geoGb.build());

			interface.setAttr("type", FnAttribute::StringAttribute("pointcloud"));
		}
		else
		{
			// create further sub-locations to split the points into
			int64_t groupSize = 4000000;
			int64_t groupsNeeded = numPoints / groupSize;
			groupsNeeded += (int64_t)(numPoints % groupSize > 0);

			interface.setAttr("type", FnAttribute::StringAttribute("group"));

			Vec3 areaSpread(20.0f, 20.0f, 20.0f);
			FnAttribute::FloatAttribute areaSpreadAttr = aGroupAttr.getChildByName("areaSpread");
			if (areaSpreadAttr.isValid())
			{
				FnKat::FloatConstVector data = areaSpreadAttr.getNearestSample(0);

				areaSpread.x = data[0];
				areaSpread.y = data[1];
				areaSpread.z = data[2];
			}

			g_aPositions.clear();
			// TODO: improve this to only calculate the subset-we need per sub-location?
			create3DGrid(numPoints, areaSpread, g_aPositions);

//			fprintf(stderr, "Creating sub-locations.\n");

			int64_t startIndex = 0;
			int64_t remainingPoints = numPoints;

			for (int64_t i = 0; i < groupsNeeded; i++)
			{
				std::ostringstream ss;
				ss << "pointcloud_" << i;

				int64_t thisGroupSize = (remainingPoints >= groupSize) ? groupSize : remainingPoints;

				FnAttribute::GroupBuilder childArgsBuilder;
				childArgsBuilder.set("group.index", FnAttribute::IntAttribute(i));
				childArgsBuilder.set("group.indexStart", FnAttribute::IntAttribute(startIndex));
				childArgsBuilder.set("group.size", FnAttribute::IntAttribute(thisGroupSize));
				childArgsBuilder.set("group.parentArgs", aGroupAttr);
				interface.createChild(ss.str(), "", childArgsBuilder.build());

				startIndex += thisGroupSize;
				remainingPoints -= thisGroupSize;
			}
		}

		interface.stopChildTraversal();
	}

	// if we've got a subgroup for split locations, handle that
	FnAttribute::GroupAttribute group = interface.getOpArg("group");
	if (group.isValid())
	{
		FnAttribute::IntAttribute indexStartAttr = group.getChildByName("indexStart");
		int64_t indexStart = indexStartAttr.getValue(0, false);
		FnAttribute::IntAttribute sizeAttr = group.getChildByName("size");
		int64_t size = sizeAttr.getValue(0, false);

		interface.setAttr("type", FnAttribute::StringAttribute("pointcloud"));

		FnAttribute::GroupBuilder geoGb;
		FnAttribute::GroupBuilder pointsGb;

		// this is hacky, but should be okay...
		// TODO: improve this to only calculate the subset-we need per sub-location?
		//       would mean it could use less peak RSS and each child location could be done
		//       in parallel if the renderer supports it...
		const Vec3& startItem = g_aPositions[indexStart];
		const float* pData = &startItem.x;

		pointsGb.set("P", FnAttribute::FloatAttribute(pData, size * 3, 3));
		float pointConstantWidth = 0.1f;
		FnAttribute::GroupAttribute parentArgs = group.getChildByName("parentArgs");
		if (parentArgs.isValid())
		{
			FnAttribute::FloatAttribute constantPointWidthAttr = parentArgs.getChildByName("constantPointWidth");
			if (constantPointWidthAttr.isValid())
			{
				pointConstantWidth = constantPointWidthAttr.getValue(0.1f, false);
			}
		}

		pointsGb.set("constantwidth", FnAttribute::FloatAttribute(pointConstantWidth));

		geoGb.set("point", pointsGb.build());

		interface.setAttr("geometry", geoGb.build());
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
