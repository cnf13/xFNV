#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMesh_Util.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

// The navmesh's square grid of cells, each listing the triangles that overlap it.
struct NavMeshGrid {
    // An empty grid: no cells, no data, GridBoundsMin.x at FLT_MAX; the rest is deliberately left unset, as the game
    // does.
    NavMeshGrid();

    // The triangle list of the cell at aiIndex.
    BSSimpleArray<uint16_t>* GetTrianglesForGridCell(uint32_t aiIndex);
    // Whether triangle aiTriangle is listed in the cell at aiIndex.
    bool IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiIndex);
    // Whether triangle aiTriangle is listed in the cell at (aiColumn, aiRow).
    bool IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiColumn, uint32_t aiRow);
    // The world position of the corner of cell (aiColumn, aiRow) at the grid's minimum height. Writes the point to
    // apResult and returns it.
    NiPoint3 GetGridPosition(uint32_t aiColumn, uint32_t aiRow);
    // The triangle list of the cell aLoc falls in, through apArray.
    void GetTriangleData(const NiPoint3& aLoc, const BSSimpleArray<uint16_t, 1024>*& apArray);
    void
    GetTriangleDataForAABB(const NiPoint3& aLocStart, const NiPoint3& aLocEnd, BSSimpleArray<uint16_t>* apReturnArray);
    // Copies GridBoundsMin and GridBoundsMax out.
    void GetMeshBounds(NiPoint3& aMin, NiPoint3& aMax);
    // Converts a location to its clamped grid row and column.
    void GetGridRowAndColumn(const NiPoint3& aLoc, uint32_t* apRetRow, uint32_t* apRetCol);
    // Gets the cell's triangle array.
    BSSimpleArray<uint16_t>* GetTriangleArray(uint32_t aiColumn, uint32_t aiRow);
    // Reads the grid from an NVGD buffer; false when the buffer is bad.
    bool Load(const char* apBuffer, uint32_t aiBufferSize, TESFile* apFile);
    // iGridSize.
    uint32_t GetGridSize() {
        return iGridSize;
    }

    void Clear();
    // Sizes the grid to the bounds aMin..aMax for aiNumTriangles triangles and allocates its
    // cells.
    void SetBoundingExtents(const NiPoint3& aMin, const NiPoint3& aMax, uint32_t aiNumTriangles);
    // Adds aiTriangle to the cell at (aiColumn, aiRow).
    void SetTriangleData(uint16_t aiTriangle, uint32_t aiColumn, uint32_t aiRow);
    void CopyTo(NavMeshGrid* apGrid) const;
    uint32_t GetGridIndex(const NiPoint3& aLoc);

    uint32_t iGridSize;
    float fColumnSectionLen;
    float fRowSectionLen;
    NiPoint3 GridBoundsMin;
    NiPoint3 GridBoundsMax;
    BSSimpleArray<uint16_t>* GridData;
};
