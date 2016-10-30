#ifndef INSTANCEROP_H
#define INSTANCEROP_H

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>

#include <FnPluginSystem/FnPlugin.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/util/Path.h>

#include <FnGeolibServices/FnGeolibCookInterfaceUtilsService.h>

#include <vector>

struct Vec3
{
	Vec3()
	{
		
	}
	
	Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z)
	{
		
	}

	float x;
	float y;
	float z;
};

class InstancerOp : public Foundry::Katana::GeolibOp
{
public:
	static void setup(Foundry::Katana::GeolibSetupInterface& interface);
	static void cook(Foundry::Katana::GeolibCookInterface& interface);
	
protected:
	static void create2DGrid(int numItems, float areaSpread, std::vector<Vec3>& aItemPositions);
};

#endif // INSTANCEROP_H
