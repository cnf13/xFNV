#include "Fallout Shared/BGSAutoWater.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSPolygon.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/WaterShaderProperty.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <cstdint>
#include <xmmintrin.h>

int32_t ComparePointX(const void* apA, const void* apB) {
    const NiPoint3* pA = static_cast<const NiPoint3*>(apA);
    const NiPoint3* pB = static_cast<const NiPoint3*>(apB);
    if (pB->x < static_cast<double>(pA->x)) {
        return 1;
    }
    if (!(pA->x < static_cast<double>(pB->x))) {
        return 0;
    }
    return -1;
}

int32_t ComparePointY(const void* apA, const void* apB) {
    const NiPoint3* pA = static_cast<const NiPoint3*>(apA);
    const NiPoint3* pB = static_cast<const NiPoint3*>(apB);
    if (pB->y < static_cast<double>(pA->y)) {
        return 1;
    }
    if (!(pA->y < static_cast<double>(pB->y))) {
        return 0;
    }
    return -1;
}

// The guard charges the cell's water geometry to the water context; the arrays it fills are cleared first. The guard is
// deliberately built in place in raw stack memory of its size.
void BGSAutoWater::GeneratePlaceableWaterForCell(TESObjectCELL* apCell) {
    alignas(AutoMemContext) uint8_t aContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aContext);
    ::new (pContext) AutoMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp",
        83
    );
    if (apCell && apCell->HasWater()) {
        if (pLineSegmentArray) {
            pLineSegmentArray->Clear(true);
        }
        if (pPolygonList) {
            pPolygonList->RemoveAll();
        }
        GatherLineSegments(apCell);
        GeneratePolygons(apCell);
        GeneratePlaceableWater(apCell);
    }
    pContext->~AutoMemContext();
}

// The four edge arrays (EDGE_MIN_X, EDGE_MAX_X, EDGE_MAX_Y, EDGE_MIN_Y, in that order) collect the points where the
// water plane crosses the land on the cell's border.
void BGSAutoWater::GatherLineSegments(TESObjectCELL* apCell) {
    NiPoint3 intpt;
    TESObjectLAND* pLand = apCell->GetLand();
    float fWaterHeight = apCell->GetWaterHeight();
    if (BGSAutoWater::pLineSegmentArray) {
        BGSAutoWater::pLineSegmentArray->Clear(true);
    }
    else {
        BGSAutoWater::pLineSegmentArray = new BSSimpleArray<NiPoint3>;
    }
    if (pLand) {
        bool bLoaded = pLand->HasVertices();
        if (!bLoaded) {
            pLand->LoadVertices(false);
        }
        using edgeIntersect_t = BSSimpleArray<BSSimpleArray<NiPoint3, 1024>, 1024>;
        edgeIntersect_t edgeIntersect;
        for (int32_t i = 0; i < 4; ++i) {
            using tmp_t = BSSimpleArray<NiPoint3, 1024>;
            tmp_t kEdgePoints;
            edgeIntersect.Insert(static_cast<uint32_t>(i), kEdgePoints);
        }
        bool bAdjustLandHeight = false;
        float fBaseHeight = 0.0f;
        if (apCell->GetWorldSpace()) {
            TESWorldSpace* pWorldSpace = apCell->GetWorldSpace();
            if (pWorldSpace->GetNeedsWaterAdjustment()) {
                fBaseHeight = pLand->GetBaseHeight();
                bAdjustLandHeight = true;
            }
        }
        NiPoint3** pVerts = pLand->GetVertexArray();
        for (int32_t iBlock = 0; iBlock < 4; ++iBlock) {
            for (int32_t iIndex = 0; iIndex < 1019; ++iIndex) {
                uint16_t iv1idx = TESObjectLAND::pTriStripListBLOCK[iIndex];
                uint16_t iv2idx = TESObjectLAND::pTriStripListBLOCK[iIndex + 1];
                uint16_t iv3idx = TESObjectLAND::pTriStripListBLOCK[iIndex + 2];
                NiPoint3 v1 = pVerts[iBlock][iv1idx];
                NiPoint3 v2 = pVerts[iBlock][iv2idx];
                NiPoint3 v3 = pVerts[iBlock][iv3idx];
                if (bAdjustLandHeight) {
                    v1.z = v1.z + fBaseHeight;
                    v2.z = v2.z + fBaseHeight;
                    v3.z = v3.z + fBaseHeight;
                }
                if (v1 != v2 && v1 != v3 && v2 != v3) {
                    NiPoint3 p(0.0f, 0.0f, fWaterHeight);
                    NiPoint3 n(0.0f, 0.0f, 1.0f);
                    NiPlane waterplane(n, p);
                    uint16_t ucount = 0;
                    bool bVertIntersect = false;
                    bool bCoPlanar = false;
                    int32_t v1flags = BGSAutoWater::WhichCellEdge(apCell, v1);
                    int32_t v2flags = BGSAutoWater::WhichCellEdge(apCell, v2);
                    int32_t v3flags = BGSAutoWater::WhichCellEdge(apCell, v3);
                    int32_t iSect = NavMeshMath::SegmentPlaneIntersection(v1, v2, p, n, &intpt);
                    if (iSect == 1) {
                        if (v1 == intpt || v2 == intpt) {
                            bVertIntersect = true;
                        }
                        BGSAutoWater::pLineSegmentArray->Add(intpt);
                        ++ucount;
                        if ((v1flags & EDGE_MIN_X) != 0 && (v2flags & EDGE_MIN_X) != 0) {
                            edgeIntersect.GetAt(0).Add(intpt);
                        }
                        else if ((v1flags & EDGE_MAX_X) != 0 && (v2flags & EDGE_MAX_X) != 0) {
                            edgeIntersect.GetAt(1).Add(intpt);
                        }
                        else if ((v1flags & EDGE_MAX_Y) != 0 && (v2flags & EDGE_MAX_Y) != 0) {
                            edgeIntersect.GetAt(2).Add(intpt);
                        }
                        else if ((v1flags & EDGE_MIN_Y) != 0 && (v2flags & EDGE_MIN_Y) != 0) {
                            edgeIntersect.GetAt(3).Add(intpt);
                        }
                    }
                    else if (iSect == -1) {
                        bCoPlanar = true;
                        BGSAutoWater::pLineSegmentArray->Add(v1);
                        BGSAutoWater::pLineSegmentArray->Add(v2);
                        ucount += 2;
                    }
                    iSect = NavMeshMath::SegmentPlaneIntersection(v1, v3, p, n, &intpt);
                    if (iSect == 1) {
                        bool bAddPoint = true;
                        if (v1 == intpt || v3 == intpt) {
                            if (bVertIntersect || bCoPlanar) {
                                bAddPoint = false;
                            }
                            else {
                                bVertIntersect = true;
                            }
                        }
                        if (bAddPoint) {
                            BGSAutoWater::pLineSegmentArray->Add(intpt);
                            ++ucount;
                            if ((v1flags & EDGE_MIN_X) != 0 && (v3flags & EDGE_MIN_X) != 0) {
                                edgeIntersect.GetAt(0).Add(intpt);
                            }
                            else if ((v1flags & EDGE_MAX_X) != 0 && (v3flags & EDGE_MAX_X) != 0) {
                                edgeIntersect.GetAt(1).Add(intpt);
                            }
                            else if ((v1flags & EDGE_MAX_Y) != 0 && (v3flags & EDGE_MAX_Y) != 0) {
                                edgeIntersect.GetAt(2).Add(intpt);
                            }
                            else if ((v1flags & EDGE_MIN_Y) != 0 && (v3flags & EDGE_MIN_Y) != 0) {
                                edgeIntersect.GetAt(3).Add(intpt);
                            }
                        }
                    }
                    else if (iSect == -1) {
                        bCoPlanar = true;
                        for (int32_t i = 0; i < ucount; ++i) {
                            BGSAutoWater::pLineSegmentArray->RemoveLast();
                        }
                        BGSAutoWater::pLineSegmentArray->Add(v1);
                        BGSAutoWater::pLineSegmentArray->Add(v3);
                        ucount = 2;
                    }
                    iSect = NavMeshMath::SegmentPlaneIntersection(v2, v3, p, n, &intpt);
                    if (iSect == 1) {
                        bool bAddPoint = true;
                        if (v2 == intpt || (v3 == intpt && (bVertIntersect || bCoPlanar))) {
                            bAddPoint = false;
                        }
                        if (bAddPoint) {
                            BGSAutoWater::pLineSegmentArray->Add(intpt);
                            ++ucount;
                            if ((v2flags & EDGE_MIN_X) != 0 && (v3flags & EDGE_MIN_X) != 0) {
                                edgeIntersect.GetAt(0).Add(intpt);
                            }
                            else if ((v2flags & EDGE_MAX_X) != 0 && (v3flags & EDGE_MAX_X) != 0) {
                                edgeIntersect.GetAt(1).Add(intpt);
                            }
                            else if ((v2flags & EDGE_MAX_Y) != 0 && (v3flags & EDGE_MAX_Y) != 0) {
                                edgeIntersect.GetAt(2).Add(intpt);
                            }
                            else if ((v2flags & EDGE_MIN_Y) != 0 && (v3flags & EDGE_MIN_Y) != 0) {
                                edgeIntersect.GetAt(3).Add(intpt);
                            }
                        }
                    }
                    else if (iSect == -1) {
                        for (int32_t i = 0; i < ucount; ++i) {
                            BGSAutoWater::pLineSegmentArray->RemoveLast();
                        }
                        BGSAutoWater::pLineSegmentArray->Add(v2);
                        BGSAutoWater::pLineSegmentArray->Add(v3);
                        ucount = 2;
                    }
                    if (ucount != 2) {
                        for (int32_t i = 0; i < ucount; ++i) {
                            BGSAutoWater::pLineSegmentArray->RemoveLast();
                        }
                    }
                    switch (v1flags) {
                        case EDGE_MIN_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v1)) {
                                    edgeIntersect.GetAt(0).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v1)) {
                                    edgeIntersect.GetAt(3).Add(v1);
                                }
                            }
                            break;
                        }
                        case EDGE_MAX_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v1)) {
                                    edgeIntersect.GetAt(1).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v1)) {
                                    edgeIntersect.GetAt(3).Add(v1);
                                }
                            }
                            break;
                        }
                        case EDGE_MIN_X | EDGE_MAX_Y: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v1)) {
                                    edgeIntersect.GetAt(0).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v1)) {
                                    edgeIntersect.GetAt(2).Add(v1);
                                }
                            }
                            break;
                        }
                        default: {
                            if (v1flags == (EDGE_MAX_X | EDGE_MAX_Y) && waterplane.WhichSide(v1) == 2) {
                                v1.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v1)) {
                                    edgeIntersect.GetAt(1).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v1)) {
                                    edgeIntersect.GetAt(2).Add(v1);
                                }
                            }
                            break;
                        }
                    }
                    switch (v2flags) {
                        case EDGE_MIN_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v2) == 2) {
                                v2.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v2)) {
                                    edgeIntersect.GetAt(0).Add(v2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v2)) {
                                    edgeIntersect.GetAt(3).Add(v2);
                                }
                            }
                            break;
                        }
                        case EDGE_MAX_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v2) == 2) {
                                v2.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v2)) {
                                    edgeIntersect.GetAt(1).Add(v2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v2)) {
                                    edgeIntersect.GetAt(3).Add(v2);
                                }
                            }
                            break;
                        }
                        case EDGE_MIN_X | EDGE_MAX_Y: {
                            if (waterplane.WhichSide(v2) == 2) {
                                v2.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v2)) {
                                    edgeIntersect.GetAt(0).Add(v2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v2)) {
                                    edgeIntersect.GetAt(2).Add(v2);
                                }
                            }
                            break;
                        }
                        default: {
                            if (v2flags == (EDGE_MAX_X | EDGE_MAX_Y) && waterplane.WhichSide(v2) == 2) {
                                v2.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v2)) {
                                    edgeIntersect.GetAt(1).Add(v2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v2)) {
                                    edgeIntersect.GetAt(2).Add(v2);
                                }
                            }
                            break;
                        }
                    }
                    switch (v3flags) {
                        case EDGE_MIN_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v3) == 2) {
                                v3.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v3)) {
                                    edgeIntersect.GetAt(0).Add(v3);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v3)) {
                                    edgeIntersect.GetAt(3).Add(v3);
                                }
                            }
                            break;
                        }
                        case EDGE_MAX_X | EDGE_MIN_Y: {
                            if (waterplane.WhichSide(v3) == 2) {
                                v3.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v3)) {
                                    edgeIntersect.GetAt(1).Add(v3);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v3)) {
                                    edgeIntersect.GetAt(3).Add(v3);
                                }
                            }
                            break;
                        }
                        case EDGE_MIN_X | EDGE_MAX_Y: {
                            if (waterplane.WhichSide(v3) == 2) {
                                v3.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v3)) {
                                    edgeIntersect.GetAt(0).Add(v3);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v3)) {
                                    edgeIntersect.GetAt(2).Add(v3);
                                }
                            }
                            break;
                        }
                        default: {
                            if (v3flags == (EDGE_MAX_X | EDGE_MAX_Y) && waterplane.WhichSide(v3) == 2) {
                                v3.z = fWaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v3)) {
                                    edgeIntersect.GetAt(1).Add(v3);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v3)) {
                                    edgeIntersect.GetAt(2).Add(v3);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (!bLoaded) {
            pLand->UnLoadVertices();
        }
        // The left and right edges sort with one comparator, the bottom and top ones with the other; then each edge's
        // points are added in pairs. The size compares are signed.
        for (int32_t i = 0; i < 4; ++i) {
            if (i == 0 || i == 1) {
                edgeIntersect.GetAt(i).Sort(&ComparePointY);
            }
            else {
                edgeIntersect.GetAt(i).Sort(&ComparePointX);
            }
            for (int32_t j = 0; j < static_cast<int32_t>(edgeIntersect.GetAt(i).GetSize()); j += 2) {
                if (static_cast<int32_t>(edgeIntersect.GetAt(i).GetSize()) > j + 1) {
                    BGSAutoWater::pLineSegmentArray->Add(edgeIntersect.GetAt(i).GetAt(j));
                    BGSAutoWater::pLineSegmentArray->Add(edgeIntersect.GetAt(i).GetAt(j + 1));
                }
            }
        }
    }
}

// Joins the gathered line segments into chains of points; each closed chain becomes a triangulated polygon on the
// polygon list, then polygons lying inside another polygon are removed.
void BGSAutoWater::GeneratePolygons(TESObjectCELL* apCell) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp",
        522
    );
    while (BGSAutoWater::pLineSegmentArray->iSize) {
        BSSimpleArray<NiPoint3>* pVerts = new BSSimpleArray<NiPoint3>;
        pVerts->Add((*BGSAutoWater::pLineSegmentArray)[0]);
        pVerts->Add((*BGSAutoWater::pLineSegmentArray)[1]);
        BGSAutoWater::pLineSegmentArray->RemoveRange(0, 2, true);
        bool bFound = true;
        bool bClosed = false;
        // A segment that extends the chain at either end is moved into it and the search starts again.
        while (bFound) {
            bFound = false;
            for (int32_t i = 0; i < static_cast<int32_t>(BGSAutoWater::pLineSegmentArray->iSize);) {
                NiPoint3 v0 = (*BGSAutoWater::pLineSegmentArray)[i];
                NiPoint3 v1 = (*BGSAutoWater::pLineSegmentArray)[i + 1];
                if (BSUtilities::ComparePoints(&v0, &pVerts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &pVerts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &(*pVerts)[pVerts->iSize - 2], 0.01f)) {
                    pVerts->Add(v1);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bFound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v1, &pVerts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &pVerts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &(*pVerts)[pVerts->iSize - 2], 0.01f)) {
                    pVerts->Add(v0);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bFound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v0, &pVerts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &pVerts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &(*pVerts)[1], 0.01f)) {
                    pVerts->Insert(0, v1);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bFound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v1, &pVerts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &pVerts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &(*pVerts)[1], 0.01f)) {
                    pVerts->Insert(0, v0);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bFound = true;
                    break;
                }
                // A segment joining both ends closes the chain.
                if ((BSUtilities::ComparePoints(&v0, &pVerts->GetLast(), 0.01f) &&
                     BSUtilities::ComparePoints(&v1, &pVerts->GetFirst(), 0.01f)) ||
                    (BSUtilities::ComparePoints(&v0, &pVerts->GetFirst(), 0.01f) &&
                     BSUtilities::ComparePoints(&v1, &pVerts->GetLast(), 0.01f))) {
                    bClosed = true;
                }
                i += 2;
            }
        }
        float fWaterHeight = apCell->GetWaterHeight();
        float fOffsetY = apCell->GetLand()->GetWorldOffsetForBlock(0).y;
        NiPoint3 worldtranslate(apCell->GetLand()->GetWorldOffsetForBlock(0).x, fOffsetY, fWaterHeight);
        if (bClosed) {
            BSPolygon2D* pNewPoly;
            pNewPoly = new BSPolygon2D(pVerts);
            if (pNewPoly->iVertexCount <= 2) {
                if (pNewPoly) {
                    delete pNewPoly;
                }
            }
            else {
                pNewPoly->RemoveZ();
                pNewPoly->Triangulate();
                if (!BGSAutoWater::pPolygonList) {
                    BGSAutoWater::pPolygonList = ::new NiTPointerList<BSPolygon2D*>;
                }
                BGSAutoWater::pPolygonList->AddHead(pNewPoly);
            }
        }
        else if (pVerts) {
            delete pVerts;
        }
    }
    if (BGSAutoWater::pPolygonList) {
        NiTListItem<BSPolygon2D*>* pIter = BGSAutoWater::pPolygonList->m_pkHead;
        while (pIter) {
            NiTListItem<BSPolygon2D*>* pNext = BGSAutoWater::pPolygonList->GetNextPos(pIter);
            if (pIter) {
                BSPolygon2D* pPoly1 = BGSAutoWater::pPolygonList->Get(pIter);
                NiTListItem<BSPolygon2D*>* pPolyIter = BGSAutoWater::pPolygonList->m_pkHead;
                NiTListItem<BSPolygon2D*>* pPolyNext = nullptr;
                while (pPolyIter) {
                    pPolyNext = BGSAutoWater::pPolygonList->GetNextPos(pPolyIter);
                    if (pPolyIter) {
                        BSPolygon2D* pPoly2 = BGSAutoWater::pPolygonList->Get(pPolyIter);
                        if (pPoly1 != pPoly2 && BGSAutoWater::IsPolyonInsidePolygon(pPoly1, pPoly2)) {
                            BGSAutoWater::pPolygonList->RemovePos(pIter);
                            break;
                        }
                    }
                    pPolyIter = pPolyNext;
                }
            }
            pIter = pNext;
        }
    }
}

// Creates a placeable water reference for each polygon of the list that has its geometry, then removes and deletes the
// polygon.
void BGSAutoWater::GeneratePlaceableWater(TESObjectCELL* apCell) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp",
        717
    );
    if (BGSAutoWater::pPolygonList) {
        float fWaterHeight = apCell->GetWaterHeight();
        TESObjectLAND* pLand = apCell->GetLand();
        float fOffsetY = pLand->GetWorldOffsetForBlock(0).y;
        pLand = apCell->GetLand();
        NiPoint3 worldtranslate(pLand->GetWorldOffsetForBlock(0).x, fOffsetY, fWaterHeight);
        bool bCreatedHavok = false;
        NiTListItem<BSPolygon2D*>* pPos = BGSAutoWater::pPolygonList->GetHeadPos();
        while (pPos) {
            NiTListItem<BSPolygon2D*>* pNext = BGSAutoWater::pPolygonList->GetNextPos(pPos);
            if (pPos) {
                BSPolygon2D* pPoly = BGSAutoWater::pPolygonList->Get(pPos);
                if (pPoly->GetRootNode() && pPoly->Get3D()) {
                    pPoly->GetRootNode()->SetName("AutoWaterNode");
                    pPoly->Get3D()->SetName("AutoWater");
                    TESObjectREFR* pWaterRef;
                    pWaterRef = new TESObjectREFR;
                    WaterShaderProperty* pWaterShadProp = new WaterShaderProperty;
                    pPoly->Get3D()->AttachProperty(pWaterShadProp);
                    BSShaderManager::PrepareObject(pPoly->GetRootNode(), 0, 0);
                    NiStencilProperty* pStencilProp = new NiStencilProperty;
                    pStencilProp->SetDrawMode(NiStencilProperty::DRAW_BOTH);
                    pPoly->Get3D()->AttachProperty(pStencilProp);
                    TESWaterForm* pWaterType = apCell->GetWaterType();
                    BGSPlaceableWater* pPlaceableAutoWater = pWaterType->GetPlaceableAutoWater();
                    pWaterRef->SetObjectReference(pPlaceableAutoWater);
                    BSFadeNode* pFadeNode = new BSFadeNode;
                    pFadeNode->SetIgnoreFade(true);
                    pWaterRef->Set3D(pFadeNode, true);
                    pFadeNode->SetTranslate(worldtranslate);
                    pWaterRef->SetLocationOnReferenceSimple(worldtranslate);
                    BSMultiBoundAABB* pMBShape = new BSMultiBoundAABB;
                    float fMinX = 0.0f;
                    float fMinY = 0.0f;
                    float fMinZ = 0.0f;
                    float fMaxX = 0.0f;
                    float fMaxY = 0.0f;
                    float fMaxZ = 0.0f;
                    NiTriShape* pTriStrips = pPoly->Get3D();
                    const NiPoint3* pVertices = pTriStrips->GetVertices();
                    for (int32_t i = 0; i < pTriStrips->GetVertexCount(); ++i) {
                        const NiPoint3* pVertex = &pVertices[i];
                        float fX = pVertex->x;
                        float fY = pVertex->y;
                        float fZ = pVertex->z;
                        if (i <= 0) {
                            fMaxX = fX;
                            fMinX = fX;
                            fMaxY = fY;
                            fMinY = fY;
                            fMaxZ = fZ;
                            fMinZ = fZ;
                        }
                        else {
                            fMinX = BSMIN(fMinX, fX);
                            fMinY = BSMIN(fMinY, fY);
                            fMinZ = BSMIN(fMinZ, fZ);
                            fMaxX = BSMAX(fMaxX, fX);
                            fMaxY = BSMAX(fMaxY, fY);
                            fMaxZ = BSMAX(fMaxZ, fZ);
                        }
                    }
                    float fHalfZ = (fMaxZ - fMinZ) / 2.0;
                    float fExtentZ = std::fabs(fHalfZ);
                    float fHalfY = (fMaxY - fMinY) / 2.0;
                    float fExtentY = std::fabs(fHalfY);
                    float fHalfX = (fMaxX - fMinX) / 2.0;
                    float fExtentX = std::fabs(fHalfX);
                    NiPoint3 halfextents(fExtentX, fExtentY, fExtentZ);
                    float fCenterZ = fMaxZ - halfextents.z;
                    float fCenterY = fMaxY - halfextents.y;
                    float fCenterX = fMaxX - halfextents.x;
                    NiPoint3 center(fCenterX, fCenterY, fCenterZ);
                    center += worldtranslate;
                    pMBShape->SetCenter(center);
                    pMBShape->SetHalfExtents(halfextents);
                    BSMultiBound* pMB = new BSMultiBound;
                    pMB->SetShape(pMBShape);
                    BSMultiBoundNode* pMBNode = new BSMultiBoundNode;
                    pMBNode->SetMultiBound(pMB);
                    pFadeNode->AttachChild(pMBNode, 1);
                    pMBNode->AttachChild(pPoly->GetRootNode(), 1);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    pFadeNode->Update(kUpdateData);
                    apCell->AddReference(pWaterRef, false);
                    pWaterRef->SetTemporary();
                    if (bAutoWaterUsesHavokShapes.Bool() && !bCreatedHavok) {
                        pWaterRef->InitHavokForPlaceableWater();
                        bCreatedHavok = true;
                    }
                    if (pPoly) {
                        delete pPoly;
                    }
                    pPoly = nullptr;
                    BGSAutoWater::pPolygonList->RemovePos(pPos);
                }
            }
            pPos = pNext;
        }
    }
}

bool BGSAutoWater::IsPolyonInsidePolygon(BSPolygon2D* apPoly1, BSPolygon2D* apPoly2) {
    for (int32_t i = 0; i < apPoly1->iVertexCount; ++i) {
        NiPoint3* pVertex = &(*apPoly1->pVertexSimpleArray)[i];
        NiPoint2 vec2(pVertex->x, pVertex->y);
        if (apPoly2->IsPointInsidePoly(vec2)) {
            return true;
        }
    }
    return false;
}

int32_t BGSAutoWater::WhichCellEdge(TESObjectCELL* apCell, NiPoint3 arVert) {
    int32_t edgeFlags = 0;
    TESObjectLAND* pLand = apCell->GetLand();
    NiPoint3 offset = pLand->GetWorldOffsetForBlock(0);
    arVert += offset;
    float fCorner1Y = static_cast<float>(apCell->GetDataY() << 12);
    float fCorner1X = static_cast<float>(apCell->GetDataX() << 12);
    NiPoint2 corner1(fCorner1X, fCorner1Y);
    float fCorner2Y = static_cast<float>(apCell->GetDataY() << 12);
    float fCorner2X = static_cast<float>((apCell->GetDataX() << 12) + 0x1000);
    NiPoint2 corner2(fCorner2X, fCorner2Y);
    float fCorner3Y = static_cast<float>((apCell->GetDataY() << 12) + 0x1000);
    float fCorner3X = static_cast<float>(apCell->GetDataX() << 12);
    NiPoint2 corner3(fCorner3X, fCorner3Y);
    float fCorner4Y = static_cast<float>((apCell->GetDataY() << 12) + 0x1000);
    float fCorner4X = static_cast<float>((apCell->GetDataX() << 12) + 0x1000);
    NiPoint2 corner4(fCorner4X, fCorner4Y);

    if (corner1.x == arVert.x && corner1.y == arVert.y) {
        return EDGE_MIN_X | EDGE_MIN_Y;
    }
    if (corner2.x == arVert.x && corner2.y == arVert.y) {
        return EDGE_MAX_X | EDGE_MIN_Y;
    }
    if (corner3.x == arVert.x && corner3.y == arVert.y) {
        return EDGE_MIN_X | EDGE_MAX_Y;
    }
    if (corner4.x == arVert.x && corner4.y == arVert.y) {
        return EDGE_MAX_X | EDGE_MAX_Y;
    }
    if (corner1.x == arVert.x) {
        return EDGE_MIN_X;
    }
    if (corner2.x == arVert.x) {
        return EDGE_MAX_X;
    }
    if (corner1.y == arVert.y) {
        return EDGE_MIN_Y;
    }
    if (corner4.y == arVert.y) {
        return EDGE_MAX_Y;
    }
    return edgeFlags;
}

NiPoint3** TESObjectLAND::GetVertexArray() {
    if (pLoadedData) {
        return pLoadedData->ppVertices;
    }

    return nullptr;
}
