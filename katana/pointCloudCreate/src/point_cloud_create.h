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

#ifndef POINT_CLOUD_CREATE_H
#define POINT_CLOUD_CREATE_H

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

class PointCloudCreateOp : public Foundry::Katana::GeolibOp
{
public:
	static void setup(Foundry::Katana::GeolibSetupInterface& interface);
	static void cook(Foundry::Katana::GeolibCookInterface& interface);
	
protected:
	
	static void createGridPoints(Foundry::Katana::GeolibCookInterface& interface);
	static void createPointsFromFile(Foundry::Katana::GeolibCookInterface& interface);	
	
	static void create3DGrid(int64_t numItems, const Vec3& areaSpread, std::vector<Vec3>& aItemPositions);
	
	static bool loadPositionsFromASCIIFile(const std::string& filePath, std::vector<Vec3>& aItemPositions);
	static bool loadPositionsAndRadiiFromASCIIFile(const std::string& filePath, std::vector<Vec3>& aItemPositions, std::vector<float>& aItemWidths);
};

#endif // POINT_CLOUD_CREATE_H
