/*
 InstancesCreate
 Created by Peter Pearson in 2016-2018.

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

#include "instancer_op.h"

#include <sstream>
#include <cmath>

#include <stdio.h>

static std::vector<Vec3>	aTranslates;

void InstancerOp::setup(Foundry::Katana::GeolibSetupInterface& interface)
{
	interface.setThreading(Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
}

void InstancerOp::cook(Foundry::Katana::GeolibCookInterface& interface)
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
		// TODO: cache this with Foundry::Katana::GeolibPrivateData...
		std::string sourceLocation;
		FnAttribute::StringAttribute sourceLocationAttr = aGroupAttr.getChildByName("sourceLocation");
		if (sourceLocationAttr.isValid())
		{
			sourceLocation = sourceLocationAttr.getValue("", false);
		}
		
		FnAttribute::IntAttribute instanceTypeAttr = aGroupAttr.getChildByName("instanceType");
		bool threeD = false;
		FnAttribute::IntAttribute threeDAttr = aGroupAttr.getChildByName("threeD");
		if (threeDAttr.isValid())
		{
			threeD = threeDAttr.getValue(0, false) == 1;
		}
		
		bool floatFormat = false;
		FnAttribute::IntAttribute floatFormatMatrixAttr = aGroupAttr.getChildByName("floatFormatMatrix");
		if (floatFormatMatrixAttr.isValid())
		{
			floatFormat = floatFormatMatrixAttr.getValue(0, false) == 1;
		}

		FnAttribute::IntAttribute numInstancesAttr = aGroupAttr.getChildByName("numInstances");
		FnAttribute::IntAttribute groupInstancesAttr = aGroupAttr.getChildByName("groupInstances");

		int numInstances = numInstancesAttr.getValue(0, false);
		if (numInstances == 0)
		{
			return;
		}
		
		bool instanceArray = true;
		FnAttribute::IntAttribute instanceArrayAttr = aGroupAttr.getChildByName("instanceArray");
		if (instanceArrayAttr.isValid())
		{
			instanceArray = instanceArrayAttr.getValue(1, false);
		}
		
		bool createInstanceIndexAttribute = false;
		FnAttribute::IntAttribute createInstanceIndexAttributeAttr = aGroupAttr.getChildByName("createInstanceIndexAttribute");
		if (createInstanceIndexAttributeAttr.isValid())
		{
			createInstanceIndexAttribute = createInstanceIndexAttributeAttr.getValue(1, false);
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
				
		// we're dangerously assuming that this will be run first...
		if (!threeD)
		{
			create2DGrid(numInstances, areaSpread, aTranslates);
		}
		else
		{
			create3DGrid(numInstances, areaSpread, aTranslates);
		}
		
		if (!instanceArray)
		{
			bool groupInstances = groupInstancesAttr.getValue(0, false);
			if (groupInstances)
			{
				FnAttribute::IntAttribute groupSizeAttr = aGroupAttr.getChildByName("groupSize");
				int groupSize = groupSizeAttr.getValue(0, false);
	
				int groupsNeeded = numInstances / groupSize;
				groupsNeeded += (int)(numInstances % groupSize > 0);
	
				int remainingInstances = numInstances;
				int indexStartCount = 0;
	
				for (int i = 0; i < groupsNeeded; i++)
				{
					std::ostringstream ss;
					ss << "group_" << i;
	
					int thisGroupSize = (remainingInstances >= groupSize) ? groupSize : remainingInstances;
	
					FnAttribute::GroupBuilder childArgsBuilder;
					childArgsBuilder.set("group.index", FnAttribute::IntAttribute(i));
					childArgsBuilder.set("group.indexStart", FnAttribute::IntAttribute(indexStartCount));
					childArgsBuilder.set("group.size", FnAttribute::IntAttribute(thisGroupSize));
					childArgsBuilder.set("group.sourceLoc", FnAttribute::StringAttribute(sourceLocation));
					interface.createChild(ss.str(), "", childArgsBuilder.build());
	
					remainingInstances -= thisGroupSize;
					indexStartCount += thisGroupSize;
				}
			}
			else
			{
				for (int i = 0; i < numInstances; i++)
				{
					std::ostringstream ss;
					ss << "instance_" << i;
	
					FnAttribute::GroupBuilder childArgsBuilder;
					childArgsBuilder.set("leaf.index", FnAttribute::IntAttribute(i));
					childArgsBuilder.set("leaf.sourceLoc", FnAttribute::StringAttribute(sourceLocation));
					interface.createChild(ss.str(), "", childArgsBuilder.build());
				}
			}
		}
		else
		{
			// just create a single instance array location
				
			FnAttribute::GroupBuilder geoGb;
			geoGb.set("instanceSource", FnAttribute::StringAttribute(sourceLocation));
			
			if (floatFormat)
			{
				// we can use FloatAttr for instance array types
				std::vector<float> aMatrixValues(numInstances * 16, 0.0f);
				
				for (int i = 0; i < numInstances; i++)
				{
					const Vec3& trans = aTranslates[i];
					
					// set matrix values
					
					unsigned int matrixStartOffset = i * 16;
					
					aMatrixValues[matrixStartOffset] = 1.0f;
					aMatrixValues[matrixStartOffset + 5] = 1.0f;
					aMatrixValues[matrixStartOffset + 10] = 1.0f;
					
					aMatrixValues[matrixStartOffset + 12] = trans.x;
					aMatrixValues[matrixStartOffset + 13] = trans.y;
					aMatrixValues[matrixStartOffset + 14] = trans.z;
					
					aMatrixValues[matrixStartOffset + 15] = 1.0f;
				}
				
				geoGb.set("instanceMatrix", FnAttribute::FloatAttribute(aMatrixValues.data(), numInstances * 16, 16));
			}
			else
			{
				// double format
				
				std::vector<double> aMatrixValues(numInstances * 16, 0.0);
				
				for (int i = 0; i < numInstances; i++)
				{
					const Vec3& trans = aTranslates[i];
					
					// set matrix values
					
					unsigned int matrixStartOffset = i * 16;
					
					aMatrixValues[matrixStartOffset] = 1.0;
					aMatrixValues[matrixStartOffset + 5] = 1.0;
					aMatrixValues[matrixStartOffset + 10] = 1.0;
					
					aMatrixValues[matrixStartOffset + 12] = trans.x;
					aMatrixValues[matrixStartOffset + 13] = trans.y;
					aMatrixValues[matrixStartOffset + 14] = trans.z;
					
					aMatrixValues[matrixStartOffset + 15] = 1.0;
				}
				
				geoGb.set("instanceMatrix", FnAttribute::DoubleAttribute(aMatrixValues.data(), numInstances * 16, 16));
			}
			
			std::vector<int> aInstanceIndices;
			if (createInstanceIndexAttribute)
			{
				// if we want the optional (for renderers we care about anyway) instanceIndex attribute, add that as well...
				// create array of 0, for each index
				aInstanceIndices.resize(numInstances, 0);
				
				geoGb.set("instanceIndex", FnAttribute::IntAttribute(aInstanceIndices.data(), numInstances, 1));
			}
			
			interface.setAttr("geometry", geoGb.build());
			
			interface.setAttr("type", FnAttribute::StringAttribute("instance array"));			
	
			interface.stopChildTraversal();
		}
		
		return;
	}

	FnAttribute::GroupAttribute group = interface.getOpArg("group");
	if (group.isValid())
	{
		FnAttribute::IntAttribute indexStartAttr = group.getChildByName("indexStart");
		int indexStart = indexStartAttr.getValue(0, false);
		FnAttribute::IntAttribute sizeAttr = group.getChildByName("size");
		int size = sizeAttr.getValue(0 , false);
		
		FnAttribute::StringAttribute sourceLocationAttr = group.getChildByName("sourceLoc");
				
		interface.setAttr("type", FnAttribute::StringAttribute("group"));

		for (int i = indexStart; i < indexStart + size; i++)
		{
			std::ostringstream ss;
			ss << "instance_" << i;

			FnAttribute::GroupBuilder childArgsBuilder;
			childArgsBuilder.set("leaf.index", FnAttribute::IntAttribute(i));
			childArgsBuilder.set("leaf.sourceLoc", sourceLocationAttr);
			interface.createChild(ss.str(), "", childArgsBuilder.build());
		}

		interface.stopChildTraversal();
	}

	FnAttribute::GroupAttribute leaf = interface.getOpArg("leaf");
	if (leaf.isValid())
	{
		FnAttribute::IntAttribute indexAttr = leaf.getChildByName("index");
		int index = indexAttr.getValue(0 , false);

		FnAttribute::GroupBuilder geoGb;

		FnAttribute::StringAttribute sourceLocationAttr = leaf.getChildByName("sourceLoc");
		if (sourceLocationAttr.isValid())
		{			
			geoGb.set("instanceSource", sourceLocationAttr);
		}
		
		interface.setAttr("type", FnAttribute::StringAttribute("instance"));
		interface.setAttr("geometry", geoGb.build());
		
		FnAttribute::GroupBuilder xformGb;
		const Vec3& trans = aTranslates[index];
		double transValues[3] = { trans.x, trans.y, trans.z };
		xformGb.set("translate", FnAttribute::DoubleAttribute(transValues, 3, 3));
		
		interface.setAttr("xform", xformGb.build());

		interface.stopChildTraversal();
	}
}

void InstancerOp::create2DGrid(int numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next square number, so we get a good even distribution for both X and Y
	int edgeCount = (int)(std::sqrt((float)numItems));
	if (edgeCount * edgeCount < numItems)
		edgeCount += 1;

	int fullItemCount = edgeCount * edgeCount;

	int extra = fullItemCount - numItems;

	// TODO: could do resize() here and then just set the members of each item
	//       directly, which might be more efficient...
	aItemPositions.reserve(numItems);	
	
	float edgeDelta = 1.0f / (float)edgeCount;
	
	std::vector<float> xPositions(edgeCount);
	std::vector<float> yPositions(edgeCount);
	
	for (unsigned int i = 0; i < edgeCount; i++)
	{
		float samplePos = (float(i) * edgeDelta) - 0.5f;
		
		xPositions[i] = samplePos * areaSpread.x;
		yPositions[i] = samplePos * areaSpread.y;
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
				if (extra > 0 && ++count >= skipStride)
				{
					count = 0;
					continue;
				}
				
				const float yPos = yPositions[yInd];
	
				aItemPositions.push_back(Vec3(xPos, 0.0f, yPos));
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
				const float yPos = yPositions[yInd];
				
				aItemPositions.push_back(Vec3(xPos, 0.0f, yPos));
			}
		}
	}
}

void InstancerOp::create3DGrid(int numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next cube number, so we get a good even distribution for both X, Y and Z
	int edgeCount = (int)(cbrtf((float)numItems));
	if (edgeCount * edgeCount * edgeCount < numItems)
		edgeCount += 1;

	int fullItemCount = edgeCount * edgeCount * edgeCount;

	int extra = fullItemCount - numItems;

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

DEFINE_GEOLIBOP_PLUGIN(InstancerOp)

void registerPlugins()
{
	REGISTER_PLUGIN(InstancerOp, "InstancesCreate", 0, 1);
}
