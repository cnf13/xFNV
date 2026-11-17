#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

class BSPolygon2D;
class TESObjectCELL;

// BSSimpleArray<NiPoint3,1024>::operator=: copies pBuffer, iSize and iReservedSize from arThat, leaves the vtable and
// returns apThis. The outer point arrays' Insert and Add call it. It is a free function because a user-declared
// operator= on the template would change every instance's assignment.
BSSimpleArray<NiPoint3, 1024>* BSSimpleArray_NiPoint3_1024_operator_assign(
    BSSimpleArray<NiPoint3, 1024>* apThis,
    const BSSimpleArray<NiPoint3, 1024>& arThat
);
// The x-coordinate compare (qsort form).
int32_t ComparePointX(const void* apA, const void* apB);
// The y-coordinate compare (qsort form).
int32_t ComparePointY(const void* apA, const void* apB);

// The routines operate on the supplied cell.
class BGSAutoWater {
public:
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
