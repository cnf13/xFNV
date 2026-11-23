#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

class BSPolygon2D;
class TESObjectCELL;

// The x-coordinate compare (qsort form).
int32_t ComparePointX(const void* apA, const void* apB);
// The y-coordinate compare (qsort form).
int32_t ComparePointY(const void* apA, const void* apB);

// The routines operate on the supplied cell.
class BGSAutoWater {
public:
    // The bits WhichCellEdge returns: the cell borders a vertex lies on.
    enum CELL_EDGE_FLAGS : int32_t {
        EDGE_MIN_X = 0x1,
        EDGE_MAX_X = 0x2,
        EDGE_MAX_Y = 0x4,
        EDGE_MIN_Y = 0x8,
    };

    static void GeneratePlaceableWaterForCell(TESObjectCELL* apCell);

protected:
    static void GatherLineSegments(TESObjectCELL* apCell);
    static void GeneratePolygons(TESObjectCELL* apCell);
    static void GeneratePlaceableWater(TESObjectCELL* apCell);
    static int32_t WhichCellEdge(TESObjectCELL* cell, float x, float y);

public:
    static bool IsPolyonInsidePolygon(BSPolygon2D* apPoly1, BSPolygon2D* apPoly2);
    static int32_t WhichCellEdge(TESObjectCELL* apCell, NiPoint3 arVert);

    // The cell's water line segments (pairs of points) and the polygons built from them. Both are cleared per cell.
    static inline BSSimpleArray<NiPoint3>* pLineSegmentArray;
    static inline NiTPointerList<BSPolygon2D*>* pPolygonList;
};
