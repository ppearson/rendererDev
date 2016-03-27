/*
 vdbconv
 Copyright 2014-2016 Peter Pearson.

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

#ifndef VDB_CONVERTER_H
#define VDB_CONVERTER_H

#include <openvdb/openvdb.h>

#include <OpenEXR/half.h>

struct GridBounds
{
	GridBounds() : min(5000.0f), max(-5000.0f)
	{

	}

	void mergeGrid(openvdb::FloatGrid::Ptr grid)
	{
		// work out bbox
		openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();

		openvdb::Coord voxelMin = bbox.min();
		openvdb::Coord voxelMax = bbox.max();

		openvdb::Vec3d gridMin = voxelMin.asVec3d();
		openvdb::Vec3d gridMax = voxelMax.asVec3d();
/*
		fprintf(stderr, "rr: %f, %f, %f - %f, %f, %f\n", gridMin.x(), gridMin.y(), gridMin.z(), gridMax.x(), gridMax.y(), gridMax.z());

		grid->transform().print(std::cout, " ");
*/
		min.x() = std::min(min.x(), (float)gridMin.x());
		min.y() = std::min(min.y(), (float)gridMin.y());
		min.z() = std::min(min.z(), (float)gridMin.z());

		max.x() = std::max(max.x(), (float)gridMax.x());
		max.y() = std::max(max.y(), (float)gridMax.y());
		max.z() = std::max(max.z(), (float)gridMax.z());
	}

	openvdb::Vec3f min;
	openvdb::Vec3f max;
};



class VDBConverter
{
public:
	VDBConverter();

	void setSizeMultiplier(float sizeMultipler) { m_sizeMultiplier = sizeMultipler; }
	void setValueMultiplier(float valueMultiplier) { m_valueMultiplier = valueMultiplier; }

	void setStoreAsHalf(bool storeHalf) { m_storeAsHalf = storeHalf; }
	void setUseSparseGrid(bool useSparse) { m_useSparseGrids = useSparse; }

	bool convertSingle(const std::string& srcPath, const std::string& dstPath);
	bool convertSequence(const std::string& srcPath, const std::string& dstPath);

protected:

	bool saveGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const;

	bool saveDenseGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const;
	bool saveSparseGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const;

	static std::string getFrameFileName(const std::string& fileName, unsigned int frame);

protected:
	float		m_sizeMultiplier;
	float		m_valueMultiplier;

	bool		m_storeAsHalf;
	bool		m_useSparseGrids;
};

#endif // VDB_CONVERTER_H
