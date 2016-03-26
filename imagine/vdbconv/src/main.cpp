/*
 vdbconv
 Copyright 2014 Peter Pearson.

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

#include <openvdb/openvdb.h>

#include <OpenEXR/half.h>

#include <string>
#include <stdio.h>

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

//		openvdb::Vec3d gridMin = grid->indexToWorld(voxelMin);
//		openvdb::Vec3d gridMax = grid->indexToWorld(voxelMax);

//		fprintf(stderr, "rr: %f, %f, %f - %f, %f, %f\n", gridMin.x(), gridMin.y(), gridMin.z(), gridMax.x(), gridMax.y(), gridMax.z());

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

#define INDIVIDUAL_REGION_ONLY 1

bool saveGrid(openvdb::FloatGrid::Ptr grid, const GridBounds& bounds, const std::string& path, float bboxMultiplier, float valueMultiplier, bool asHalf)
{
	openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

	openvdb::Coord ijk;
	int &i = ijk[0];
	int &j = ijk[1];
	int &k = ijk[2];

	float value = 0.0f;

	unsigned char version = 1;

	unsigned int gridResX = bounds.max.x() - bounds.min.x() + 1;
	unsigned int gridResY = bounds.max.y() - bounds.min.y() + 1;
	unsigned int gridResZ = bounds.max.z() - bounds.min.z() + 1;

	openvdb::Vec3f extent((float)gridResX, (float)gridResY, (float)gridResZ);
	extent.normalize();

	extent *= 2.5f;

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

	unsigned char fileType = 0;
	if (asHalf)
		fileType = 1;

	fwrite(&fileType, 1, 1, pFinalFile);

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

	if (!asHalf)
	{
		float* pFinalValues = new float[totalNumVoxels];
		memset(pFinalValues, 0, sizeof(float) * totalNumVoxels);

		float* pFin = pFinalValues;
		
#if INDIVIDUAL_REGION_ONLY
		openvdb::CoordBBox localBounds = grid->evalActiveVoxelBoundingBox();
		
		for (k = localBounds.min().z(); k <= localBounds.max().z(); k++)
		{
			for (j = localBounds.min().y(); j <= localBounds.max().y(); j++)
			{
				unsigned int destVoxelOffsetIndex = (j * gridResX) + (k * gridResX * gridResY) + localBounds.min().x();
				
				pFin = &pFinalValues[destVoxelOffsetIndex];
				
				for (i = localBounds.min().x(); i <= localBounds.max().x(); i++)
				{
					value = accessor.getValue(ijk) * valueMultiplier;
					*(pFin++) = value;
				}
			}
		}
		
#else
		for (k = bounds.min.z(); k <= bounds.max.z(); k++)
		{
			for (j = bounds.min.y(); j <= bounds.max.y(); j++)
			{
				for (i = bounds.min.x(); i <= bounds.max.x(); i++)
				{
					value = accessor.getValue(ijk) * valueMultiplier;
					*(pFin++) = value;
				}
			}
		}
#endif
		fwrite(pFinalValues, sizeof(float) * totalNumVoxels, 1, pFinalFile);

		delete [] pFinalValues;
	}
	else
	{
		half* pFinalValues = new half[totalNumVoxels];
		memset(pFinalValues, 0, sizeof(half) * totalNumVoxels);

		half* pFin = pFinalValues;

#if INDIVIDUAL_REGION_ONLY
		openvdb::CoordBBox localBounds = grid->evalActiveVoxelBoundingBox();
		
		for (k = localBounds.min().z(); k <= localBounds.max().z(); k++)
		{
			for (j = localBounds.min().y(); j <= localBounds.max().y(); j++)
			{
				unsigned int destVoxelOffsetIndex = (j * gridResX) + (k * gridResX * gridResY) + localBounds.min().x();
				
				pFin = &pFinalValues[destVoxelOffsetIndex];
				
				for (i = localBounds.min().x(); i <= localBounds.max().x(); i++)
				{
					value = accessor.getValue(ijk) * valueMultiplier;
					*(pFin++) = (half)value;
				}
			}
		}
		
#else
		for (k = bounds.min.z(); k <= bounds.max.z(); k++)
		{
			for (j = bounds.min.y(); j <= bounds.max.y(); j++)
			{
				for (i = bounds.min.x(); i <= bounds.max.x(); i++)
				{
					value = accessor.getValue(ijk) * valueMultiplier;
					*(pFin++) = (half)value;
				}
			}
		}
#endif

		fwrite(pFinalValues, sizeof(half) * totalNumVoxels, 1, pFinalFile);

		delete [] pFinalValues;
	}

	fclose(pFinalFile);

	return true;
}

std::string getFrameFileName(const std::string& fileName, unsigned int frame)
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

int main(int argc, char** argv)
{
	openvdb::initialize();

	bool printHelp = false;

	if (argc < 3)
	{
		fprintf(stderr, "incorrects args:\n");
		printHelp = true;
	}

	unsigned int argOffset = 0;

	float bboxMultiplier = 2.5f;
	float valueMultiplier = 1.0f;

	bool asHalf = false;

	bool sequence = false;

	unsigned int numOptionArgs = (argc - 1) - 2;

	for (unsigned int i = 0; i < numOptionArgs; i++)
	{
		std::string argString = argv[i + 1];

		if (argString.substr(0, 1) != "-")
			continue;

		size_t startPos = argString.find_first_not_of("-");

		std::string argName = argString.substr(startPos);

		if (argName == "help")
		{
			printHelp = true;
			break;
		}
		else if (argName == "half")
		{
			asHalf = true;
		}
		else if (argName == "valMul" && numOptionArgs > i + 1)
		{
			std::string strValMultValue = argv[i + 1 + 1];
			if (!strValMultValue.empty())
			{
				valueMultiplier = atof(strValMultValue.c_str());
				argOffset += 1;
			}
		}
		else if (argName == "seq")
		{
			sequence = true;
		}

		argOffset += 1;
	}

	if (printHelp)
	{
		fprintf(stderr, "OpenVDB to Imagine volume converter. 0.1.\nUsage: vdbconv [options] <source_vdb> <dest_ivv>\n");
		fprintf(stderr, "    Options: -half\tsave as half format\n             -valMul <float>\tapply value modifier\n\n");
		return 0;
	}

	// now handle the last two args, which should be the input and output filenames...

	std::string sourceFile(argv[1 + argOffset]);

	std::string destFile(argv[2 + argOffset]);

	if (sequence && (sourceFile.find("#") == std::string::npos || destFile.find("#") == std::string::npos))
	{
		sequence = false;
	}

	GridBounds bounds;

	if (!sequence)
	{
		openvdb::io::File file(sourceFile);

		file.open();

		openvdb::GridBase::Ptr baseGrid;

		openvdb::io::File::NameIterator nameIter = file.beginName();

		unsigned int count = 0;

		for (; nameIter != file.endName(); ++nameIter)
		{
			count++;

			baseGrid = file.readGrid(nameIter.gridName());

			openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

			bounds.mergeGrid(grid);
		}

		nameIter = file.beginName();

		// if we've only got one grid, save only that one out
		if (count == 1)
		{
			baseGrid = file.readGrid(nameIter.gridName());

			fprintf(stderr, "Converting single grid: %s...\n", nameIter.gridName().c_str());

			openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

			saveGrid(grid, bounds, destFile, bboxMultiplier, valueMultiplier, asHalf);
		}
		else
		{
			// otherwise, work out how we're going to name them...

			unsigned int dotPos = destFile.find_last_of(".");

			if (dotPos == -1)
			{
				file.close();
				return -1;
			}

			std::string fileName1 = destFile.substr(0, dotPos);
			fileName1 += "_";

			std::string fileName2 = destFile.substr(dotPos);

			for (; nameIter != file.endName(); ++nameIter)
			{
				if (nameIter.gridName() == "density")
				{
					baseGrid = file.readGrid(nameIter.gridName());

					fprintf(stderr, "Converting grid: %s...\n", nameIter.gridName().c_str());

					openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

					std::string gridSaveName = fileName1 + "den" + fileName2;

					saveGrid(grid, bounds, gridSaveName, bboxMultiplier, valueMultiplier, asHalf);
				}
				else if (nameIter.gridName() == "temperature")
				{
					baseGrid = file.readGrid(nameIter.gridName());

					fprintf(stderr, "Converting grid: %s...\n", nameIter.gridName().c_str());

					openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

					std::string gridSaveName = fileName1 + "temp" + fileName2;

					saveGrid(grid, bounds, gridSaveName, bboxMultiplier, valueMultiplier, asHalf);
				}
			}
		}

		file.close();
	}
	else
	{
		unsigned int startFrame = 1;
		unsigned int endFrame = 95;

		// work out the overall bounds for all frames

		for (unsigned int fr = startFrame; fr <= endFrame; fr++)
		{
			std::string realSourceFile = getFrameFileName(sourceFile, fr);

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

		for (unsigned int fr = startFrame; fr <= endFrame; fr++)
		{
			std::string realSourceFile = getFrameFileName(sourceFile, fr);

			std::string realDestFile = getFrameFileName(destFile, fr);

			openvdb::io::File file(realSourceFile);

			file.open();

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

				saveGrid(grid, bounds, realDestFile, bboxMultiplier, valueMultiplier, asHalf);
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

						std::string gridSaveName = fileName1 + "den" + fileName2;

						saveGrid(grid, bounds, gridSaveName, bboxMultiplier, valueMultiplier, asHalf);
					}
					else if (nameIter.gridName() == "temperature")
					{
						baseGrid = file.readGrid(nameIter.gridName());

						fprintf(stderr, "%s,", nameIter.gridName().c_str());

						openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

						std::string gridSaveName = fileName1 + "temp" + fileName2;

						saveGrid(grid, bounds, gridSaveName, bboxMultiplier, valueMultiplier, asHalf);
					}
				}

				fprintf(stderr, "\n");
			}

			file.close();
		}
	}

	return 0;
}

