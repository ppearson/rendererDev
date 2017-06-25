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

#ifndef SPARSE_GRID_H
#define SPARSE_GRID_H

#include <vector>
#include <string.h> // for memset

#include <half.h>


class SparseGrid
{
public:
	SparseGrid();
	~SparseGrid();
	
	class SparseSubCell
	{
	public:
		SparseSubCell() : m_resX(0), m_resY(0), m_resZ(0), m_resXY(0),
			m_pFloatData(NULL), m_pHalfData(NULL)
		{

		}

		~SparseSubCell()
		{
			freeMemory();
		}

		void freeMemory()
		{
			if (m_pFloatData)
			{
				delete [] m_pFloatData;
				m_pFloatData = NULL;
			}

			if (m_pHalfData)
			{
				delete [] m_pHalfData;
				m_pHalfData = NULL;
			}
		}

		inline void initNoAllocation(unsigned int resX, unsigned int resY, unsigned int resZ)
		{
			m_resX = resX;
			m_resY = resY;
			m_resZ = resZ;
			m_resXY = resX * resY;
		}

		void allocateIfNeeded(bool isHalf)
		{
			if (m_pFloatData || m_pHalfData)
				return;

			unsigned int size = m_resXY * m_resY;

			if (!isHalf)
			{
				m_pFloatData = new float[size];
				memset(m_pFloatData, 0, size * sizeof(float));
			}
			else
			{
				m_pHalfData = new half[size];
				memset(m_pHalfData, 0, size * sizeof(half));
			}
		}

		inline bool isAllocated() const
		{
			if (m_pFloatData || m_pHalfData)
				return true;

			return false;
		}

		// these currently use local coordinates within the subCell, and assume the data
		// pointers are valid...
		inline void setVoxelValueFloat(unsigned int i, unsigned int j, unsigned int k, float value)
		{
			unsigned int overallIndex = i + (j * m_resX) + (k * m_resXY);

			m_pFloatData[overallIndex] = value;
		}

		inline float getVoxelValueFloat(unsigned int i, unsigned int j, unsigned int k) const
		{
			unsigned int overallIndex = i + (j * m_resX) + (k * m_resXY);

			return m_pFloatData[overallIndex];
		}

		inline void setVoxelValueHalf(unsigned int i, unsigned int j, unsigned int k, half value)
		{
			unsigned int overallIndex = i + (j * m_resX) + (k * m_resXY);

			m_pHalfData[overallIndex] = value;
		}

		inline half getVoxelValueHalf(unsigned int i, unsigned int j, unsigned int k) const
		{
			unsigned int overallIndex = i + (j * m_resX) + (k * m_resXY);

			return m_pHalfData[overallIndex];
		}


		unsigned int getResX() const
		{
			return m_resX;
		}

		unsigned int getResY() const
		{
			return m_resY;
		}

		unsigned int getResZ() const
		{
			return m_resZ;
		}

		unsigned int getResXY() const
		{
			return m_resXY;
		}

		size_t getMemorySize() const
		{
			size_t finalSize = 0;

			finalSize += sizeof(*this);

			if (m_pFloatData)
			{
				finalSize += m_resX * m_resY * m_resZ * sizeof(float);
			}

			if (m_pHalfData)
			{
				finalSize += m_resX * m_resY * m_resZ * sizeof(half);
			}

			return finalSize;
		}

		float* getRawFloatData()
		{
			return m_pFloatData;
		}

		const float* getRawFloatData() const
		{
			return m_pFloatData;
		}

		half* getRawHalfData()
		{
			return m_pHalfData;
		}

		const half* getRawHalfData() const
		{
			return m_pHalfData;
		}

	protected:
		uint32_t		m_resX;
		uint32_t		m_resY;
		uint32_t		m_resZ;
		uint32_t		m_resXY;

		float*			m_pFloatData;
		half*			m_pHalfData;
	};
	
	void freeCells();

	void resizeGrid(unsigned int overallResX, unsigned int overallResY, unsigned int overallResZ,
					unsigned int cellSize);

	void clear();
	
	void setVoxelValueFloat(unsigned int i, unsigned int j, unsigned int k, float value);
	void setVoxelValueHalf(unsigned int i, unsigned int j, unsigned int k, half value);
	
	std::vector<SparseSubCell*>& getSubCells() { return m_aCells; }
	const std::vector<SparseSubCell*>& getSubCells() const { return m_aCells; }
	
	uint32_t getSubCellSize() const
	{
		return m_cellSize;
	}	
	
protected:
	// currently, the implementation is such that all sub-cells of the sparse grid
	// are allocated (to make the lookup of them easy), but sub-cells themselves only
	// allocate the raw data if they have values within them
	std::vector<SparseSubCell*>		m_aCells;
	
	uint32_t			m_overallResX;
	uint32_t			m_overallResY;
	uint32_t			m_overallResZ;
	
	// currently, the cell size is the same in all 3 dimensions...
	uint32_t			m_cellSize;

	// these are worked out based on the overall volume res and the cell size...
	uint32_t			m_cellCountX;
	uint32_t			m_cellCountY;
	uint32_t			m_cellCountZ;
	uint32_t			m_cellCountXY;
};

#endif // SPARSE_GRID_H
