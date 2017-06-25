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

#include "sparse_grid.h"

SparseGrid::SparseGrid()
{
	
}

SparseGrid::~SparseGrid()
{
	
}

void SparseGrid::freeCells()
{
	std::vector<SparseSubCell*>::iterator itCell = m_aCells.begin();
	for (; itCell != m_aCells.end(); ++itCell)
	{
		SparseSubCell* pCell = *itCell;
		delete pCell;
	}

	m_aCells.clear();
}

void SparseGrid::resizeGrid(unsigned int overallResX, unsigned int overallResY, unsigned int overallResZ,
				unsigned int cellSize)
{
	freeCells();

	m_overallResX = overallResX;
	m_overallResY = overallResY;
	m_overallResZ = overallResZ;

	m_cellSize = cellSize;

	m_cellCountX = m_overallResX / m_cellSize;
	m_cellCountX += (m_overallResX % m_cellSize > 0);
	m_cellCountY = m_overallResY / m_cellSize;
	m_cellCountY += (m_overallResY % m_cellSize > 0);
	m_cellCountZ = m_overallResZ / m_cellSize;
	m_cellCountZ += (m_overallResZ % m_cellSize > 0);
	
	m_cellCountXY = m_cellCountX * m_cellCountY;

	// create the subcells
	// for the moment, just lay them out in sequential axis order, but something like Morten codes will possibly
	// make more sense (for boundary interpolation) in terms of locality...

	unsigned int cellSizeX = m_cellSize;
	unsigned int cellSizeY = m_cellSize;
	unsigned int cellSizeZ = m_cellSize;

	for (unsigned int k = 0; k < m_cellCountZ; k++)
	{
		unsigned int resRemainingZ = m_overallResZ - (k * m_cellSize);
		cellSizeZ = std::min(m_cellSize, resRemainingZ);
		for (unsigned int j = 0; j < m_cellCountY; j++)
		{
			unsigned int resRemainingY = m_overallResY - (j * m_cellSize);
			cellSizeY = std::min(m_cellSize, resRemainingY);
			for (unsigned int i = 0; i < m_cellCountX; i++)
			{
				unsigned int resRemainingX = m_overallResX - (i * m_cellSize);
				cellSizeX = std::min(m_cellSize, resRemainingX);

				SparseSubCell* pNewSubCell = new SparseSubCell();
				pNewSubCell->initNoAllocation(cellSizeX, cellSizeY, cellSizeZ);

				m_aCells.push_back(pNewSubCell);
			}
		}
	}
}

void SparseGrid::clear()
{
	std::vector<SparseSubCell*>::iterator itCell = m_aCells.begin();
	for (; itCell != m_aCells.end(); ++itCell)
	{
		SparseSubCell* pCell = *itCell;

		pCell->freeMemory();
	}
}

void SparseGrid::setVoxelValueFloat(unsigned int i, unsigned int j, unsigned int k, float value)
{
	if (value == 0.0f)
		return;

	// work out the cell indices, and the indices within the cell
	unsigned int subcellIndexI = i / m_cellSize;
	unsigned int subCellVoxelI = i - (subcellIndexI * m_cellSize);

	unsigned int subcellIndexJ = j / m_cellSize;
	unsigned int subCellVoxelJ = j - (subcellIndexJ * m_cellSize);

	unsigned int subcellIndexK = k / m_cellSize;
	unsigned int subCellVoxelK = k - (subcellIndexK * m_cellSize);

	unsigned int subCellIndex = subcellIndexI + (subcellIndexJ * m_cellCountX) + (subcellIndexK * m_cellCountXY);

	SparseSubCell* pSubCell = m_aCells[subCellIndex];

	pSubCell->allocateIfNeeded(false);

	pSubCell->setVoxelValueFloat(subCellVoxelI, subCellVoxelJ, subCellVoxelK, value);
}

void SparseGrid::setVoxelValueHalf(unsigned int i, unsigned int j, unsigned int k, half value)
{
	if (value == 0.0f)
		return;

	// work out the cell indices, and the indices within the cell
	unsigned int subcellIndexI = i / m_cellSize;
	unsigned int subCellVoxelI = i - (subcellIndexI * m_cellSize);

	unsigned int subcellIndexJ = j / m_cellSize;
	unsigned int subCellVoxelJ = j - (subcellIndexJ * m_cellSize);

	unsigned int subcellIndexK = k / m_cellSize;
	unsigned int subCellVoxelK = k - (subcellIndexK * m_cellSize);

	unsigned int subCellIndex = subcellIndexI + (subcellIndexJ * m_cellCountX) + (subcellIndexK * m_cellCountXY);

	SparseSubCell* pSubCell = m_aCells[subCellIndex];

	pSubCell->allocateIfNeeded(true);

	pSubCell->setVoxelValueHalf(subCellVoxelI, subCellVoxelJ, subCellVoxelK, value);
}
