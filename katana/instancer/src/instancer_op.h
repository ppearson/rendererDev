#ifndef INSTANCEROP_H
#define INSTANCEROP_H

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>

#include <FnPluginSystem/FnPlugin.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/util/Path.h>

#include <FnGeolibServices/FnGeolibCookInterfaceUtilsService.h>

class InstancerOp : public Foundry::Katana::GeolibOp
{
public:
	static void setup(Foundry::Katana::GeolibSetupInterface& interface);
	static void cook(Foundry::Katana::GeolibCookInterface& interface);
};

#endif // INSTANCEROP_H
