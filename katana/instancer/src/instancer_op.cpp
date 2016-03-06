#include "instancer_op.h"

#include <sstream>

#include <stdio.h>

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
			fprintf(stderr, "Location: %s\n", sourceLocation.c_str());
		}

		FnAttribute::IntAttribute numInstancesAttr = aGroupAttr.getChildByName("numInstances");
		FnAttribute::IntAttribute groupInstancesAttr = aGroupAttr.getChildByName("groupInstances");

		int numInstances = numInstancesAttr.getValue(0, false);
		if (numInstances == 0)
		{
			return;
		}

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
		interface.setAttr("type", FnAttribute::StringAttribute("group"));

		for (int i = indexStart; i < indexStart + size; i++)
		{
			std::ostringstream ss;
			ss << "instance_" << i;

			FnAttribute::GroupBuilder childArgsBuilder;
			childArgsBuilder.set("leaf.index", FnAttribute::IntAttribute(i));
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
//		geoGb.set("instanceSource", sourceLocationAttr);

		interface.setAttr("type", FnAttribute::StringAttribute("instance"));
//		interface.setAttr("geometry", geoGb.build());

		interface.stopChildTraversal();
	}
}

DEFINE_GEOLIBOP_PLUGIN(InstancerOp)

void registerPlugins()
{
	REGISTER_PLUGIN(InstancerOp, "Instancer", 0, 1);
}
