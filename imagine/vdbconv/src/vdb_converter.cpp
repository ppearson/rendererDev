/*
 vdbconv
 Copyright 2014-2017 Peter Pearson.

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

#include "vdb_converter.h"

#include "sparse_grid.h"

VDBConverter::VDBConverter()
{
	m_sizeMultiplier = 2.0f;
	m_valueMultiplier = 1.0f;
	m_subCellSize = 32;

	m_storeAsHalf = false;
	m_useSparseGrids = false;
}

bool VDBConverter::convertSingle(const std::string& srcPath, const std::string& dstPath)
{
	GridBounds bounds;

	openvdb::io::File file(srcPath);

	if (!file.open())
	{
		fprintf(stderr, "Can't open VDB file: %s\n", srcPath.c_str());
		return false;
	}

	openvdb::GridBase::Ptr baseGrid;

	openvdb::io::File::NameIterator nameIter = file.beginName();

	unsigned int count = 0;

	for (; nameIter != file.endName(); ++nameIter)
	{
		count++;

		baseGrid = file.readGrid(nameIter.gridName());

		openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
		
		if (!grid)
			continue;

		bounds.mergeGrid(grid);
	}

	nameIter = file.beginName();

	// if we've only got one grid, save only that one out
	if (count == 1)
	{
		baseGrid = file.readGrid(nameIter.gridName());

		fprintf(stderr, "Converting single grid: %s...\n", nameIter.gridName().c_str());

		openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

		saveGrid(grid, bounds, dstPath);
	}
	else
	{
		// otherwise, work out how we're going to name them...

		unsigned int dotPos = dstPath.find_last_of(".");

		if (dotPos == -1)
		{
			file.close();
			return false;
		}

		std::string fileName1 = dstPath.substr(0, dotPos);
		fileName1 += "_";

		std::string fileName2 = dstPath.substr(dotPos);

		for (; nameIter != file.endName(); ++nameIter)
		{
			if (nameIter.gridName() == "density")
			{
				baseGrid = file.readGrid(nameIter.gridName());

				fprintf(stderr, "Converting grid: %s...\n", nameIter.gridName().c_str());

				openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

				std::string gridSaveFilename = fileName1 + "den" + fileName2;

				saveGrid(grid, bounds, gridSaveFilename);
			}
			else if (nameIter.gridName() == "temperature")
			{
				baseGrid = file.readGrid(nameIter.gridName());

				fprintf(stderr, "Converting grid: %s...\n", nameIter.gridName().c_str());

				openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

				std::string gridSaveFilename = fileName1 + "temp" + fileName2;

				saveGrid(grid, bounds, gridSaveFilename);
			}
		}
	}

	file.close();

	return true;
}

bool VDBConverter::convertSequence(const std::string& srcPath, const std::string& dstPath)
{
	GridBounds bounds;

	unsigned int startFrame = 1;
	unsigned int endFrame = 137;
	endFrame = 45;

	// work out the overall bounds for all frames up-front...

	for (unsigned int fr = startFrame; fr <= endFrame; fr++)
	{
		std::string realSourceFile = getFrameFileName(srcPath, fr);

		openvdb::io::File file(realSourceFile);

		file.open();

		openvdb::GridBase::Ptr baseGrid;
		openvdb::io::File::NameIterator nameIter = file.beginName();

		for (; nameIter != file.endName(); ++nameIter)
		{
			// readGridMetadata doesn't seem to be any faster than just readGrid() in 3.0...
			baseGrid = file.readGrid(nameIter.gridName());

			openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

			bounds.mergeGrid(grid);
		}

		file.close();
	}

	// now loop through again

	openvdb::Coord lastDenMin;
	openvdb::Coord lastDenMax;
	openvdb::Coord lastTempMin;
	openvdb::Coord lastTempMax;

	for (unsigned int fr = startFrame; fr <= endFrame; fr++)
	{
		std::string realSourceFile = getFrameFileName(srcPath, fr);

		std::string realDestFile = getFrameFileName(dstPath, fr);

		openvdb::io::File file(realSourceFile);

		if (!file.open())
		{
			fprintf(stderr, "Can't open VDB file: %s\n", realSourceFile.c_str());
			continue;
		}

		openvdb::GridBase::Ptr baseGrid;

		openvdb::io::File::NameIterator nameIter = file.beginName();

		unsigned int count = 0;

		for (; nameIter != file.endName(); ++nameIter)
		{
			count++;
		}

		nameIter = file.beginName();

		// if we've only got one grid, save only that one out
		if (count == 1)
		{
			baseGrid = file.readGrid(nameIter.gridName());

			fprintf(stderr, "Converting single grid: %s, frame %d...\n", nameIter.gridName().c_str(), fr);

			openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

			saveGrid(grid, bounds, realDestFile);
		}
		else
		{
			// otherwise, work out how we're going to name them...

			unsigned int dotPos = realDestFile.find_last_of(".");

			if (dotPos == -1)
			{
				file.close();
				return -1;
			}

			fprintf(stderr, "Converting grid frame: %d: ", fr);

			std::string fileName1 = realDestFile.substr(0, dotPos);
			fileName1 += "_";

			std::string fileName2 = realDestFile.substr(dotPos);

			for (; nameIter != file.endName(); ++nameIter)
			{
				if (nameIter.gridName() == "density")
				{
					baseGrid = file.readGrid(nameIter.gridName());

					fprintf(stderr, "%s,", nameIter.gridName().c_str());

					openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

					openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();

					openvdb::Coord thisDenMin = bbox.min();
					openvdb::Coord thisDenMax = bbox.max();

					std::string gridSaveName = fileName1 + "den" + fileName2;

					int offsetX = (thisDenMax - lastDenMin).x() / 2;
					int offsetZ = (thisDenMax - lastDenMin).z() / 2;

					if (fr == startFrame)
					{
						saveGrid(grid, bounds, gridSaveName);
					}
					else
					{
						saveGrid(grid, bounds, gridSaveName);
					}

					lastDenMin = thisDenMin;
					lastDenMax = thisDenMax;
				}
				else if (nameIter.gridName() == "temperature")
				{
					baseGrid = file.readGrid(nameIter.gridName());

					fprintf(stderr, "%s,", nameIter.gridName().c_str());

					openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

					openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();

					openvdb::Coord thisTempMin = bbox.min();
					openvdb::Coord thisTempMax = bbox.max();

					std::string gridSaveName = fileName1 + "temp" + fileName2;

					int offsetX = (thisTempMax - lastTempMax).x() / 2;
					int offsetZ = (thisTempMax - lastTempMax).z() / 2;

					if (fr == startFrame)
					{
						saveGrid(grid, bounds, gridSaveName);
					}
					else
					{
						saveGrid(grid, bounds, gridSaveName);
					}

					lastTempMax = thisTempMax;
					lastTempMin = thisTempMin;
				}
			}

			fprintf(stderr, "\n");
		}

		file.close();
	}

	return true;
}

bool VDBConverter::saveGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const
{
	if (!m_useSparseGrids)
	{
		return saveDenseGrid(grid, bounds, path);
	}
	else
	{
		return saveSparseGrid(grid, bounds, path);
	}
}

bool VDBConverter::saveDenseGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const
{
	openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

	openvdb::Coord ijk;
	int &i = ijk[0];
	int &j = ijk[1];
	int &k = ijk[2];

	float value = 0.0f;

	unsigned char version = 2;

	unsigned int gridResX = bounds.max.x() - bounds.min.x() + 1;
	unsigned int gridResY = bounds.max.y() - bounds.min.y() + 1;
	unsigned int gridResZ = bounds.max.z() - bounds.min.z() + 1;

	openvdb::Vec3f extent((float)gridResX, (float)gridResY, (float)gridResZ);
	extent.normalize();

	extent *= m_sizeMultiplier;

	float bbMinX = -extent.x();
	float bbMinY = -extent.y();
	float bbMinZ = -extent.z();

	float bbMaxX = extent.x();
	float bbMaxY = extent.y();
	float bbMaxZ = extent.z();

	FILE* pFinalFile = fopen(path.c_str(), "wb");

	if (!pFinalFile)
	{
		fprintf(stderr, "Couldn't open file: %s for writing.\n", path.c_str());
		return false;
	}

	fwrite(&version, 1, 1, pFinalFile);

	unsigned char dataType = 0;
	if (m_storeAsHalf)
		dataType = 1;

	fwrite(&dataType, 1, 1, pFinalFile);

	unsigned char gridType = 0; // dense
	fwrite(&gridType, sizeof(unsigned char), 1, pFinalFile);

	fwrite(&gridResX, sizeof(unsigned int), 1, pFinalFile);
	fwrite(&gridResY, sizeof(unsigned int), 1, pFinalFile);
	fwrite(&gridResZ, sizeof(unsigned int), 1, pFinalFile);

	fwrite(&bbMinX, sizeof(float), 1, pFinalFile);
	fwrite(&bbMinY, sizeof(float), 1, pFinalFile);
	fwrite(&bbMinZ, sizeof(float), 1, pFinalFile);

	fwrite(&bbMaxX, sizeof(float), 1, pFinalFile);
	fwrite(&bbMaxY, sizeof(float), 1, pFinalFile);
	fwrite(&bbMaxZ, sizeof(float), 1, pFinalFile);

	unsigned int totalNumVoxels = gridResX * gridResY * gridResZ;

	// TODO: do we even need to allocate this memory? we can just write the values
	//       directly... - assuming the axis order is correct, but we can modify that
	//       on the read side of things if needed (but that will make reading slower)...

	if (!m_storeAsHalf)
	{
		float* pFinalValues = new float[totalNumVoxels];
		memset(pFinalValues, 0, sizeof(float) * totalNumVoxels);

		float* pFin = pFinalValues;

		for (k = bounds.min.z(); k <= bounds.max.z(); k++)
		{
			for (j = bounds.min.y(); j <= bounds.max.y(); j++)
			{
				for (i = bounds.min.x(); i <= bounds.max.x(); i++)
				{
					value = accessor.getValue(ijk) * m_valueMultiplier;
					*(pFin++) = value;
				}
			}
		}

		fwrite(pFinalValues, sizeof(float) * totalNumVoxels, 1, pFinalFile);

		delete [] pFinalValues;
	}
	else
	{
		half* pFinalValues = new half[totalNumVoxels];
		memset(pFinalValues, 0, sizeof(half) * totalNumVoxels);

		half* pFin = pFinalValues;

		for (k = bounds.min.z(); k <= bounds.max.z(); k++)
		{
			for (j = bounds.min.y(); j <= bounds.max.y(); j++)
			{
				for (i = bounds.min.x(); i <= bounds.max.x(); i++)
				{
					value = accessor.getValue(ijk) * m_storeAsHalf;
					*(pFin++) = (half)value;
				}
			}
		}

		fwrite(pFinalValues, sizeof(half) * totalNumVoxels, 1, pFinalFile);

		delete [] pFinalValues;
	}

	fclose(pFinalFile);

	return true;
}

bool VDBConverter::saveSparseGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path) const
{
	openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

	openvdb::Coord ijk;
	int &i = ijk[0];
	int &j = ijk[1];
	int &k = ijk[2];

	unsigned char version = 2;

	unsigned int gridResX = bounds.max.x() - bounds.min.x() + 1;
	unsigned int gridResY = bounds.max.y() - bounds.min.y() + 1;
	unsigned int gridResZ = bounds.max.z() - bounds.min.z() + 1;

	openvdb::Vec3f extent((float)gridResX, (float)gridResY, (float)gridResZ);
	extent.normalize();

	extent *= m_sizeMultiplier;

	float bbMinX = -extent.x();
	float bbMinY = -extent.y();
	float bbMinZ = -extent.z();

	float bbMaxX = extent.x();
	float bbMaxY = extent.y();
	float bbMaxZ = extent.z();

	FILE* pFinalFile = fopen(path.c_str(), "wb");

	if (!pFinalFile)
	{
		fprintf(stderr, "Couldn't open file: %s for writing.\n", path.c_str());
		return false;
	}

	fwrite(&version, 1, 1, pFinalFile);

	unsigned char dataType = 0;
	if (m_storeAsHalf)
		dataType = 1;

	fwrite(&dataType, 1, 1, pFinalFile);

	unsigned char gridType = 1; // sparse
	fwrite(&gridType, sizeof(unsigned char), 1, pFinalFile);

	unsigned short subCellSize = m_subCellSize;
	fwrite(&subCellSize, sizeof(unsigned short), 1, pFinalFile);

	fwrite(&gridResX, sizeof(unsigned int), 1, pFinalFile);
	fwrite(&gridResY, sizeof(unsigned int), 1, pFinalFile);
	fwrite(&gridResZ, sizeof(unsigned int), 1, pFinalFile);

	fwrite(&bbMinX, sizeof(float), 1, pFinalFile);
	fwrite(&bbMinY, sizeof(float), 1, pFinalFile);
	fwrite(&bbMinZ, sizeof(float), 1, pFinalFile);

	fwrite(&bbMaxX, sizeof(float), 1, pFinalFile);
	fwrite(&bbMaxY, sizeof(float), 1, pFinalFile);
	fwrite(&bbMaxZ, sizeof(float), 1, pFinalFile);

	// create a sparse grid structure to temporarily store the data

	SparseGrid sparseGrid;
	sparseGrid.resizeGrid(gridResX, gridResY, gridResZ, (unsigned int)subCellSize);

	// now write to the grid subcells

	// indices for local access to subcells...
	unsigned int iIndex = 0;
	unsigned int jIndex = 0;
	unsigned int kIndex = 0;

	float value = 0.0f;

	if (!m_storeAsHalf)
	{
		// full float

		for (k = bounds.min.z(), kIndex = 0; k <= bounds.max.z(); k++, kIndex++)
		{
			for (j = bounds.min.y(), jIndex = 0; j <= bounds.max.y(); j++, jIndex++)
			{
				for (i = bounds.min.x(), iIndex = 0; i <= bounds.max.x(); i++, iIndex++)
				{
					value = accessor.getValue(ijk) * m_valueMultiplier;

					sparseGrid.setVoxelValueFloat(iIndex, jIndex, kIndex, value);
				}
			}
		}
	}
	else
	{
		// half

		for (k = bounds.min.z(), kIndex = 0; k <= bounds.max.z(); k++, kIndex++)
		{
			for (j = bounds.min.y(), jIndex = 0; j <= bounds.max.y(); j++, jIndex++)
			{
				for (i = bounds.min.x(), iIndex = 0; i <= bounds.max.x(); i++, iIndex++)
				{
					value = accessor.getValue(ijk) * m_valueMultiplier;

					sparseGrid.setVoxelValueHalf(iIndex, jIndex, kIndex, (half)value);
				}
			}
		}
	}

	// now we need to store for each subcell whether they have data or not.
	// because we know the subcell size and the full grid size, we can work out
	// each subcell size on-the-fly, without having to store it

	// TODO: is it worth writing the number of sub-cells just to be sure...?

	std::vector<SparseGrid::SparseSubCell*>::const_iterator itSubCell = sparseGrid.getSubCells().begin();
	for (; itSubCell != sparseGrid.getSubCells().end(); ++itSubCell)
	{
		const SparseGrid::SparseSubCell* pSubCell = *itSubCell;

		static const unsigned char emptyVal = 0;
		static const unsigned char fullVal = 1;

		// TODO: this is wastefull...
		if (!pSubCell->isAllocated())
		{
			fwrite(&emptyVal, sizeof(unsigned char), 1, pFinalFile);
		}
		else
		{
			fwrite(&fullVal, sizeof(unsigned char), 1, pFinalFile);

			// also write the data - we don't need to write the length, as we can
			// reverse-engineer it when reading based on info we already have...
			unsigned int cellDataLength = pSubCell->getResXY() * pSubCell->getResZ();

			if (!m_storeAsHalf)
			{
				const float* pCellFloatData = pSubCell->getRawFloatData();
				fwrite(pCellFloatData, sizeof(float), cellDataLength, pFinalFile);
			}
			else
			{
				const half* pCellHalfData = pSubCell->getRawHalfData();
				fwrite(pCellHalfData, sizeof(half), cellDataLength, pFinalFile);
			}
		}
	}

	fclose(pFinalFile);

	return true;
}

std::string VDBConverter::getFrameFileName(const std::string& fileName, unsigned int frame)
{
	size_t sequenceCharStart = fileName.find_first_of("#");
	size_t sequenceCharEnd = fileName.find_first_not_of("#", sequenceCharStart);
	unsigned int sequenceCharLength = sequenceCharEnd - sequenceCharStart;

	char szFrameFormatter[16];
	memset(szFrameFormatter, 0, 16);
	sprintf(szFrameFormatter, "%%0%dd", sequenceCharLength);

	// generate frame sequence filename
	char szFrame[16];
	memset(szFrame, 0, 16);
	sprintf(szFrame, szFrameFormatter, frame);

	std::string sFrame(szFrame);

	std::string sequenceFramePath = fileName;
	sequenceFramePath.replace(sequenceCharStart, sequenceCharLength, sFrame);

	return sequenceFramePath;
}
