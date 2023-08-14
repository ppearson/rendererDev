/*
 InstancesCreate
 Copyright 2016-2019 Peter Pearson.

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
#include <fstream>
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

		FnAttribute::IntAttribute createdShapeTypeAttr = aGroupAttr.getChildByName("createdShapeType");
		CreatedShapeType createdShapeType;
		if (createdShapeTypeAttr.isValid())
		{
			createdShapeType = (CreatedShapeType)createdShapeTypeAttr.getValue(0, false);
		}

		std::string positionsFilePath;
		FnAttribute::StringAttribute positionsFilePathAttr = aGroupAttr.getChildByName("positionsFilePath");
		if (positionsFilePathAttr.isValid())
		{
			positionsFilePath = positionsFilePathAttr.getValue("", false);
		}

		bool floatFormat = false;
		FnAttribute::IntAttribute floatFormatMatrixAttr = aGroupAttr.getChildByName("floatFormatMatrix");
		if (floatFormatMatrixAttr.isValid())
		{
			floatFormat = floatFormatMatrixAttr.getValue(0, false) == 1;
		}

		FnAttribute::IntAttribute numInstancesAttr = aGroupAttr.getChildByName("numInstances");
		FnAttribute::IntAttribute groupInstancesAttr = aGroupAttr.getChildByName("groupInstances");

		unsigned int numInstances = (unsigned int)numInstancesAttr.getValue(0, false);
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

		Vec3 areaSpread(200.0f, 200.0f, 200.0f);
		FnAttribute::FloatAttribute areaSpreadAttr = aGroupAttr.getChildByName("areaSpread");
		if (areaSpreadAttr.isValid())
		{
			FnKat::FloatConstVector data = areaSpreadAttr.getNearestSample(0);

			areaSpread.x = data[0];
			areaSpread.y = data[1];
			areaSpread.z = data[2];
		}

		// we're dangerously assuming that this will be run first...
		if (createdShapeType == eShapeTypeGrid2D)
		{
			create2DGrid(numInstances, areaSpread, aTranslates);
		}
		else if (createdShapeType == eShapeTypeGrid3D)
		{
			create3DGrid(numInstances, areaSpread, aTranslates);
		}
		else if (createdShapeType == eShapeTypePointsFileASCII)
		{
			readPositionsFromASCIIFile(positionsFilePath, aTranslates);
			numInstances = aTranslates.size();
		}
		else if (createdShapeType == eShapeTypePointsFileBinary)
		{
			readPositionsFromBinaryFile(positionsFilePath, aTranslates);
			numInstances = aTranslates.size();
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

				for (size_t i = 0; i < numInstances; i++)
				{
					const Vec3& trans = aTranslates[i];

					// set matrix values

					size_t matrixStartOffset = i * 16;

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
				size_t valuesSize = (size_t)numInstances * 16;

				std::vector<double> aMatrixValues(valuesSize, 0.0);

				for (size_t i = 0; i < numInstances; i++)
				{
					const Vec3& trans = aTranslates[i];

					// set matrix values

					size_t matrixStartOffset = i * 16;

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

void InstancerOp::create2DGrid(unsigned int numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next square number, so we get a good even distribution for both X and Y
	unsigned int edgeCount = (unsigned int)(std::sqrt((float)numItems));
	if (edgeCount * edgeCount < numItems)
		edgeCount += 1;

	unsigned int fullItemCount = edgeCount * edgeCount;

	unsigned int extra = fullItemCount - numItems;

	fprintf(stderr, "Creating 2D grid of: %u items, 'full' size: %u.\n", numItems, fullItemCount);

	aItemPositions.clear();

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

		unsigned int skipStride = fullItemCount / extra;

		unsigned int count = 0;
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

void InstancerOp::create3DGrid(unsigned int numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next cube number, so we get a good even distribution for both X, Y and Z
	unsigned int edgeCount = (unsigned int)(cbrtf((float)numItems));
	if (edgeCount * edgeCount * edgeCount < numItems)
		edgeCount += 1;

	unsigned int fullItemCount = edgeCount * edgeCount * edgeCount;

	unsigned int extra = fullItemCount - numItems;

	fprintf(stderr, "Creating 3D grid of: %u items, 'full' size: %u.\n", numItems, fullItemCount);

	aItemPositions.clear();

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

		unsigned int skipStride = fullItemCount / extra;

		unsigned int count = 0;
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

void InstancerOp::readPositionsFromASCIIFile(const std::string& positionFilePath, std::vector<Vec3>& aItemPositions)
{
	std::ios::sync_with_stdio(false);

	std::fstream fileStream;
	fileStream.open(positionFilePath.c_str(), std::ios::in);
	if (!fileStream.is_open() || fileStream.fail())
	{
		fprintf(stderr, "Error: Can't open ASCII position file: %s\n", positionFilePath.c_str());
		return;
	}

	fprintf(stdout, "Reading positions from ASCII file: %s\n", positionFilePath.c_str());

	Vec3 temp;

	// TODO: might be worth special-casing looking for comment count Imagine puts in there, so we can
	//       do a reserve?

	char buf[512];
	while (fileStream.getline(buf, 512))
	{
		if (buf[0] == '#' || buf[0] == 0)
			continue;

		sscanf(buf, "%f, %f, %f", &temp.x, &temp.y, &temp.z);

		aItemPositions.push_back(temp);
	}

	fprintf(stderr, "Loaded %u positions from file.\n", (unsigned int)aItemPositions.size());
}

void InstancerOp::readPositionsFromBinaryFile(const std::string& positionFilePath, std::vector<Vec3>& aItemPositions)
{
	FILE* pFile = fopen(positionFilePath.c_str(), "rb");
	if (!pFile)
	{
		fprintf(stderr, "Error: Can't open binary position file: %s\n", positionFilePath.c_str());
		return;
	}

	fprintf(stdout, "Reading positions from binary file: %s\n", positionFilePath.c_str());

	float extent[3];
	fread(&extent[0], sizeof(float), 3, pFile);
	fprintf(stdout, "Positions original shape extent: (%f, %f, %f)\n", extent[0], extent[1], extent[2]);

	unsigned int numPositions = 0;
	fread(&numPositions, sizeof(unsigned int), 1, pFile);

	aItemPositions.resize(numPositions);

	std::vector<Vec3>::iterator itItem = aItemPositions.begin();
	for (; itItem != aItemPositions.end(); ++itItem)
	{
		Vec3& pos = *itItem;
		fread(&pos.x, sizeof(float), 3, pFile);
	}

	fclose(pFile);

	fprintf(stderr, "Loaded %u positions from file.\n", numPositions);
}

DEFINE_GEOLIBOP_PLUGIN(InstancerOp)

void registerPlugins()
{
	REGISTER_PLUGIN(InstancerOp, "InstancesCreate", 0, 1);
}
