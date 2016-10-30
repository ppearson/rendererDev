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

		FnAttribute::IntAttribute numInstancesAttr = aGroupAttr.getChildByName("numInstances");
		FnAttribute::IntAttribute groupInstancesAttr = aGroupAttr.getChildByName("groupInstances");

		int numInstances = numInstancesAttr.getValue(0, false);
		if (numInstances == 0)
		{
			return;
		}
		
		float areaSpread = 20.0f;
		FnAttribute::FloatAttribute areaSpreadAttr = aGroupAttr.getChildByName("areaSpread");
		if (areaSpreadAttr.isValid())
		{
			areaSpread = areaSpreadAttr.getValue(20.0f, false);
		}
		
		// we're dangerously assuming that this will be run first...
		create2DGrid(numInstances, areaSpread, aTranslates);

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
		
		std::string sourceLocation;
		FnAttribute::StringAttribute sourceLocationAttr = leaf.getChildByName("sourceLoc");
		if (sourceLocationAttr.isValid())
		{
			sourceLocation = sourceLocationAttr.getValue("", false);
			
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

void InstancerOp::create2DGrid(int numItems, float areaSpread, std::vector<Vec3>& aItemPositions)
{
	// round up to the next square number, so we get a good even distribution for both X and Y
	int edgeCount = (int)(std::sqrt((float)numItems));
	if (edgeCount * edgeCount < numItems)
		edgeCount += 1;

	int fullItemCount = edgeCount * edgeCount;

	int extra = fullItemCount - numItems;

	aItemPositions.reserve(numItems);	
	
	float edgeDelta = 1.0f / (float)edgeCount;
	
	if (extra > 0)
	{
		// we need to skip certain items
		
		int skipStride = fullItemCount / extra;
		
		int count = 0;
		for (unsigned int xInd = 0; xInd < edgeCount; xInd++)
		{
			float xPos = float(xInd);
			for (unsigned int yInd = 0; yInd < edgeCount; yInd++)
			{
				if (extra > 0 && ++count >= skipStride)
				{
					count = 0;
					continue;
				}
				
				float xSample = (xPos * edgeDelta) - 0.5f;
				float ySample = (float(yInd) * edgeDelta) - 0.5f;
	
				aItemPositions.push_back(Vec3(xSample * areaSpread, 0.0f, ySample * areaSpread));
			}
		}
	}
	else
	{
		for (unsigned int xInd = 0; xInd < edgeCount; xInd++)
		{
			float xPos = float(xInd);
			for (unsigned int yInd = 0; yInd < edgeCount; yInd++)
			{
				float xSample = (xPos * edgeDelta) - 0.5f;
				float ySample = (float(yInd) * edgeDelta) - 0.5f;
	
				aItemPositions.push_back(Vec3(xSample * areaSpread, 0.0f, ySample * areaSpread));
			}
		}
	}
}

DEFINE_GEOLIBOP_PLUGIN(InstancerOp)

void registerPlugins()
{
	REGISTER_PLUGIN(InstancerOp, "Instancer", 0, 1);
}
