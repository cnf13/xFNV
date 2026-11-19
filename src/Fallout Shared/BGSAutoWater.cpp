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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <cstdint>
#include <xmmintrin.h>

NiPoint3* NiGeometry::GetVertices() {
#if XFNV_64BIT
    return this->m_spModelData.operator->()->m_pkVertex;
#else
    return static_cast<NiPoint3*>(
        static_cast<void*>(static_cast<TESObjectREFR*>(static_cast<void*>(this->m_spModelData.operator->()))
                               ->TESObjectREFR::GetObjectReference())
    );
#endif
}

BSSimpleArray<NiPoint3, 1024>* BSSimpleArray_NiPoint3_1024_operator_assign(
    BSSimpleArray<NiPoint3, 1024>* apThis,
    const BSSimpleArray<NiPoint3, 1024>& arThat
) {
    apThis->pBuffer = arThat.pBuffer;
    apThis->iSize = arThat.iSize;
    apThis->iReservedSize = arThat.iReservedSize;
    return apThis;
}

int32_t ComparePointX(const void* apA, const void* apB) {
    const NiPoint3* a1 = static_cast<const NiPoint3*>(apA);
    const NiPoint3* a2 = static_cast<const NiPoint3*>(apB);
    if (a2->x < static_cast<double>(a1->x)) {
        return 1;
    }
    if (!(a1->x < static_cast<double>(a2->x))) {
        return 0;
    }
    return -1;
}

int32_t ComparePointY(const void* apA, const void* apB) {
    const NiPoint3* a1 = static_cast<const NiPoint3*>(apA);
    const NiPoint3* a2 = static_cast<const NiPoint3*>(apB);
    if (a2->y < static_cast<double>(a1->y)) {
        return 1;
    }
    if (!(a1->y < static_cast<double>(a2->y))) {
        return 0;
    }
    return -1;
}

// The guard charges the cell's water geometry to the water context; the arrays it fills are cleared first. The guard is
// deliberately built in place in raw stack memory of its size, as the game does.
void BGSAutoWater::GeneratePlaceableWaterForCell(TESObjectCELL* apCell) {
    alignas(AutoMemContext) uint8_t aContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aContext);
    ::new (pContext) AutoMemContext(
        MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp", 83
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

// The four edge arrays (left, right, bottom, top, by WhichCellEdge's bits 1, 2, 4, 8) collect the points where the
// water plane crosses the land on the cell's border.
void BGSAutoWater::GatherLineSegments(TESObjectCELL* apCell) {
    NiPoint3 intpt;
    TESObjectLAND* pland = apCell->GetLand();
    float fwaterHeight = apCell->GetWaterHeight();
    if (BGSAutoWater::pLineSegmentArray) {
        BGSAutoWater::pLineSegmentArray->Clear(true);
    }
    else {
        BSSimpleArray<NiPoint3>* pPointArray;
        BSSimpleArray<NiPoint3>* apMemory =
            static_cast<BSSimpleArray<NiPoint3>*>(ourMalloc(sizeof(BSSimpleArray<NiPoint3>)));
        if (apMemory) {
            pPointArray = ::new (apMemory) BSSimpleArray<NiPoint3>;
        }
        else {
            pPointArray = nullptr;
        }
        BGSAutoWater::pLineSegmentArray = pPointArray;
    }
    if (pland) {
        bool bloaded = pland->HasVertices();
        if (!bloaded) {
            pland->LoadVertices(false);
        }
        using edgeIntersect_t = BSSimpleArray<BSSimpleArray<NiPoint3, 1024>, 1024>;
        edgeIntersect_t edgeIntersect;
        for (int32_t i = 0; i < 4; ++i) {
            using tmp_t = BSSimpleArray<NiPoint3, 1024>;
            tmp_t tmp;
            edgeIntersect.Insert(static_cast<uint32_t>(i), tmp);
        }
        bool badjustlandheight = false;
        float fbaseheight = 0.0f;
        if (apCell->GetWorldSpace()) {
            TESWorldSpace* WorldSpace = apCell->GetWorldSpace();
            if (WorldSpace->GetNeedsWaterAdjustment()) {
                fbaseheight = pland->GetBaseHeight();
                badjustlandheight = true;
            }
        }
        NiPoint3** pverts = pland->GetVertexArray();
        for (int32_t iblock = 0; iblock < 4; ++iblock) {
            for (int32_t iindex = 0; iindex < 1019; ++iindex) {
                uint16_t iv1idx = TESObjectLAND::pTriStripListBLOCK[iindex];
                uint16_t iv2idx = TESObjectLAND::pTriStripListBLOCK[iindex + 1];
                uint16_t iv3idx = TESObjectLAND::pTriStripListBLOCK[iindex + 2];
                NiPoint3 v1 = pverts[iblock][iv1idx];
                NiPoint3 v2_2 = pverts[iblock][iv2idx];
                NiPoint3 v3_2 = pverts[iblock][iv3idx];
                if (badjustlandheight) {
                    v1.z = v1.z + fbaseheight;
                    v2_2.z = v2_2.z + fbaseheight;
                    v3_2.z = v3_2.z + fbaseheight;
                }
                if (v1 != v2_2 && v1 != v3_2 && v2_2 != v3_2) {
                    NiPoint3 p(0.0f, 0.0f, fwaterHeight);
                    NiPoint3 n(0.0f, 0.0f, 1.0f);
                    NiPlane waterplane(n, p);
                    uint16_t ucount = 0;
                    bool bvertIntersect = false;
                    bool bcoPlanar = false;
                    int32_t v1flags = BGSAutoWater::WhichCellEdge(apCell, v1);
                    int32_t v2flags = BGSAutoWater::WhichCellEdge(apCell, v2_2);
                    int32_t v3flags = BGSAutoWater::WhichCellEdge(apCell, v3_2);
                    int32_t isect = NavMeshMath::SegmentPlaneIntersection(v1, v2_2, p, n, &intpt);
                    if (isect == 1) {
                        if (v1 == intpt || v2_2 == intpt) {
                            bvertIntersect = true;
                        }
                        BGSAutoWater::pLineSegmentArray->Add(intpt);
                        ++ucount;
                        if ((v1flags & 1) != 0 && (v2flags & 1) != 0) {
                            edgeIntersect.GetAt(0).Add(intpt);
                        }
                        else {
                            if ((v1flags & 2) != 0 && (v2flags & 2) != 0) {
                                edgeIntersect.GetAt(1).Add(intpt);
                            }
                            else {
                                if ((v1flags & 4) != 0 && (v2flags & 4) != 0) {
                                    edgeIntersect.GetAt(2).Add(intpt);
                                }
                                else {
                                    if ((v1flags & 8) != 0 && (v2flags & 8) != 0) {
                                        edgeIntersect.GetAt(3).Add(intpt);
                                    }
                                }
                            }
                        }
                    }
                    else {
                        if (isect == -1) {
                            bcoPlanar = true;
                            BGSAutoWater::pLineSegmentArray->Add(v1);
                            BGSAutoWater::pLineSegmentArray->Add(v2_2);
                            ucount += 2;
                        }
                    }
                    isect = NavMeshMath::SegmentPlaneIntersection(v1, v3_2, p, n, &intpt);
                    if (isect == 1) {
                        bool baddpoint = true;
                        if (v1 == intpt || v3_2 == intpt) {
                            if (bvertIntersect || bcoPlanar) {
                                baddpoint = false;
                            }
                            else {
                                bvertIntersect = true;
                            }
                        }
                        if (baddpoint) {
                            BGSAutoWater::pLineSegmentArray->Add(intpt);
                            ++ucount;
                            if ((v1flags & 1) != 0 && (v3flags & 1) != 0) {
                                edgeIntersect.GetAt(0).Add(intpt);
                            }
                            else {
                                if ((v1flags & 2) != 0 && (v3flags & 2) != 0) {
                                    edgeIntersect.GetAt(1).Add(intpt);
                                }
                                else {
                                    if ((v1flags & 4) != 0 && (v3flags & 4) != 0) {
                                        edgeIntersect.GetAt(2).Add(intpt);
                                    }
                                    else {
                                        if ((v1flags & 8) != 0 && (v3flags & 8) != 0) {
                                            edgeIntersect.GetAt(3).Add(intpt);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else {
                        if (isect == -1) {
                            bcoPlanar = true;
                            for (int32_t i_2 = 0; i_2 < ucount; ++i_2) {
                                BGSAutoWater::pLineSegmentArray->RemoveLast();
                            }
                            BGSAutoWater::pLineSegmentArray->Add(v1);
                            BGSAutoWater::pLineSegmentArray->Add(v3_2);
                            ucount = 2;
                        }
                    }
                    isect = NavMeshMath::SegmentPlaneIntersection(v2_2, v3_2, p, n, &intpt);
                    if (isect == 1) {
                        bool baddpoint_2 = true;
                        if (v2_2 == intpt || (v3_2 == intpt && (bvertIntersect || bcoPlanar))) {
                            baddpoint_2 = false;
                        }
                        if (baddpoint_2) {
                            BGSAutoWater::pLineSegmentArray->Add(intpt);
                            ++ucount;
                            if ((v2flags & 1) != 0 && (v3flags & 1) != 0) {
                                edgeIntersect.GetAt(0).Add(intpt);
                            }
                            else {
                                if ((v2flags & 2) != 0 && (v3flags & 2) != 0) {
                                    edgeIntersect.GetAt(1).Add(intpt);
                                }
                                else {
                                    if ((v2flags & 4) != 0 && (v3flags & 4) != 0) {
                                        edgeIntersect.GetAt(2).Add(intpt);
                                    }
                                    else {
                                        if ((v2flags & 8) != 0 && (v3flags & 8) != 0) {
                                            edgeIntersect.GetAt(3).Add(intpt);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else {
                        if (isect == -1) {
                            for (int32_t i_3 = 0; i_3 < ucount; ++i_3) {
                                BGSAutoWater::pLineSegmentArray->RemoveLast();
                            }
                            BGSAutoWater::pLineSegmentArray->Add(v2_2);
                            BGSAutoWater::pLineSegmentArray->Add(v3_2);
                            ucount = 2;
                        }
                    }
                    if (ucount != 2) {
                        for (int32_t i_4 = 0; i_4 < ucount; ++i_4) {
                            BGSAutoWater::pLineSegmentArray->RemoveLast();
                        }
                    }
                    switch (v1flags) {
                        case 9: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v1)) {
                                    edgeIntersect.GetAt(0).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v1)) {
                                    edgeIntersect.GetAt(3).Add(v1);
                                }
                            }
                            break;
                        }
                        case 10: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v1)) {
                                    edgeIntersect.GetAt(1).Add(v1);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v1)) {
                                    edgeIntersect.GetAt(3).Add(v1);
                                }
                            }
                            break;
                        }
                        case 5: {
                            if (waterplane.WhichSide(v1) == 2) {
                                v1.z = fwaterHeight;
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
                            if (v1flags == 6 && waterplane.WhichSide(v1) == 2) {
                                v1.z = fwaterHeight;
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
                        case 9: {
                            if (waterplane.WhichSide(v2_2) == 2) {
                                v2_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(0).Add(v2_2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(3).Add(v2_2);
                                }
                            }
                            break;
                        }
                        case 10: {
                            if (waterplane.WhichSide(v2_2) == 2) {
                                v2_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(1).Add(v2_2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(3).Add(v2_2);
                                }
                            }
                            break;
                        }
                        case 5: {
                            if (waterplane.WhichSide(v2_2) == 2) {
                                v2_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(0).Add(v2_2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(2).Add(v2_2);
                                }
                            }
                            break;
                        }
                        default: {
                            if (v2flags == 6 && waterplane.WhichSide(v2_2) == 2) {
                                v2_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(1).Add(v2_2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v2_2)) {
                                    edgeIntersect.GetAt(2).Add(v2_2);
                                }
                            }
                            break;
                        }
                    }
                    switch (v3flags) {
                        case 9: {
                            if (waterplane.WhichSide(v3_2) == 2) {
                                v3_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(0).Add(v3_2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(3).Add(v3_2);
                                }
                            }
                            break;
                        }
                        case 10: {
                            if (waterplane.WhichSide(v3_2) == 2) {
                                v3_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(1).Add(v3_2);
                                }
                                if (!edgeIntersect.GetAt(3).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(3).Add(v3_2);
                                }
                            }
                            break;
                        }
                        case 5: {
                            if (waterplane.WhichSide(v3_2) == 2) {
                                v3_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(0).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(0).Add(v3_2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(2).Add(v3_2);
                                }
                            }
                            break;
                        }
                        default: {
                            if (v3flags == 6 && waterplane.WhichSide(v3_2) == 2) {
                                v3_2.z = fwaterHeight;
                                if (!edgeIntersect.GetAt(1).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(1).Add(v3_2);
                                }
                                if (!edgeIntersect.GetAt(2).IsInArray(v3_2)) {
                                    edgeIntersect.GetAt(2).Add(v3_2);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (!bloaded) {
            pland->UnLoadVertices();
        }
        // The left and right edges sort with one comparator, the bottom and top ones with the other; then each edge's
        // points are added in pairs. The size compares are signed.
        for (int32_t i_5 = 0; i_5 < 4; ++i_5) {
            if (i_5 == 0 || i_5 == 1) {
                edgeIntersect.GetAt(i_5).Sort(&ComparePointY);
            }
            else {
                edgeIntersect.GetAt(i_5).Sort(&ComparePointX);
            }
            for (int32_t j = 0; j < static_cast<int32_t>(edgeIntersect.GetAt(i_5).GetSize()); j += 2) {
                if (static_cast<int32_t>(edgeIntersect.GetAt(i_5).GetSize()) > j + 1) {
                    BGSAutoWater::pLineSegmentArray->Add(edgeIntersect.GetAt(i_5).GetAt(j));
                    BGSAutoWater::pLineSegmentArray->Add(edgeIntersect.GetAt(i_5).GetAt(j + 1));
                }
            }
        }
    }
}

// Joins the gathered line segments into chains of points; each closed chain becomes a triangulated polygon on the
// polygon list, then polygons lying inside another polygon are removed.
void BGSAutoWater::GeneratePolygons(TESObjectCELL* apCell) {
    NiPoint3 v0;
    NiPoint3 v1;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp", 522
    );
    while (BGSAutoWater::pLineSegmentArray->iSize) {
        BSSimpleArray<NiPoint3>* pverts;
        BSSimpleArray<NiPoint3>* pMemory =
            static_cast<BSSimpleArray<NiPoint3>*>(ourMalloc(sizeof(BSSimpleArray<NiPoint3>)));
        if (pMemory) {
            pverts = ::new (pMemory) BSSimpleArray<NiPoint3>;
        }
        else {
            pverts = nullptr;
        }
        pverts->Add((*BGSAutoWater::pLineSegmentArray)[0]);
        pverts->Add((*BGSAutoWater::pLineSegmentArray)[1]);
        BGSAutoWater::pLineSegmentArray->RemoveRange(0, 2, true);
        bool bfound = true;
        bool bclosed = false;
        // A segment that extends the chain at either end is moved into it and the search starts again.
        while (bfound) {
            bfound = false;
            for (int32_t i = 0; i < static_cast<int32_t>(BGSAutoWater::pLineSegmentArray->iSize);) {
                v0 = (*BGSAutoWater::pLineSegmentArray)[i];
                v1 = (*BGSAutoWater::pLineSegmentArray)[i + 1];
                if (BSUtilities::ComparePoints(&v0, &pverts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &pverts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &(*pverts)[pverts->iSize - 2], 0.01f)) {
                    pverts->Add(v1);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bfound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v1, &pverts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &pverts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &(*pverts)[pverts->iSize - 2], 0.01f)) {
                    pverts->Add(v0);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bfound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v0, &pverts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &pverts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v1, &(*pverts)[1], 0.01f)) {
                    pverts->Insert(0, v1);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bfound = true;
                    break;
                }
                if (BSUtilities::ComparePoints(&v1, &pverts->GetFirst(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &pverts->GetLast(), 0.01f) &&
                    !BSUtilities::ComparePoints(&v0, &(*pverts)[1], 0.01f)) {
                    pverts->Insert(0, v0);
                    BGSAutoWater::pLineSegmentArray->RemoveRange(i, 2, true);
                    bfound = true;
                    break;
                }
                // A segment joining both ends closes the chain.
                if ((BSUtilities::ComparePoints(&v0, &pverts->GetLast(), 0.01f) &&
                     BSUtilities::ComparePoints(&v1, &pverts->GetFirst(), 0.01f)) ||
                    (BSUtilities::ComparePoints(&v0, &pverts->GetFirst(), 0.01f) &&
                     BSUtilities::ComparePoints(&v1, &pverts->GetLast(), 0.01f))) {
                    bclosed = true;
                    i += 2;
                }
                else {
                    i += 2;
                }
            }
        }
        float z = apCell->GetWaterHeight();
        float y = apCell->GetLand()->GetWorldOffsetForBlock(0).y;
        NiPoint3 worldtranslate(apCell->GetLand()->GetWorldOffsetForBlock(0).x, y, z);
        if (bclosed) {
            BSPolygon2D* pnewPoly;
            BSPolygon2D* pPolyMemory = static_cast<BSPolygon2D*>(ourMalloc(sizeof(BSPolygon2D)));
            if (pPolyMemory) {
                pnewPoly = ::new (pPolyMemory) BSPolygon2D(pverts);
            }
            else {
                pnewPoly = nullptr;
            }
            if (pnewPoly->iVertexCount <= 2) {
                if (pnewPoly) {
                    delete pnewPoly;
                }
            }
            else {
                pnewPoly->RemoveZ();
                pnewPoly->Triangulate();
                if (!BGSAutoWater::pPolygonList) {
                    NiTPointerList<BSPolygon2D*>* pList;
                    NiTPointerList<BSPolygon2D*>* pListMemory =
                        static_cast<NiTPointerList<BSPolygon2D*>*>(ourMalloc(sizeof(NiTPointerList<BSPolygon2D*>)));
                    if (pListMemory) {
                        pList = ::new (pListMemory) NiTPointerList<BSPolygon2D*>;
                    }
                    else {
                        pList = nullptr;
                    }
                    BGSAutoWater::pPolygonList = pList;
                }
                BGSAutoWater::pPolygonList->AddHead(pnewPoly);
            }
        }
        else {
            if (pverts) {
                delete pverts;
            }
        }
    }
    if (BGSAutoWater::pPolygonList) {
        NiTListItem<BSPolygon2D*>* iter = BGSAutoWater::pPolygonList->m_pkHead;
        while (iter) {
            NiTListItem<BSPolygon2D*>* next = BGSAutoWater::pPolygonList->GetNextPos(iter);
            if (iter) {
                BSPolygon2D* ppoly1 = BGSAutoWater::pPolygonList->Get(iter);
                NiTListItem<BSPolygon2D*>* polyiter = BGSAutoWater::pPolygonList->m_pkHead;
                NiTListItem<BSPolygon2D*>* polynext = nullptr;
                while (polyiter) {
                    polynext = BGSAutoWater::pPolygonList->GetNextPos(polyiter);
                    if (polyiter) {
                        BSPolygon2D* ppoly2 = BGSAutoWater::pPolygonList->Get(polyiter);
                        if (ppoly1 != ppoly2 && BGSAutoWater::IsPolyonInsidePolygon(ppoly1, ppoly2)) {
                            BGSAutoWater::pPolygonList->RemovePos(iter);
                            break;
                        }
                    }
                    polyiter = polynext;
                }
            }
            iter = next;
        }
    }
}

// Creates a placeable water reference for each polygon of the list that has its geometry, then removes and deletes the
// polygon.
void BGSAutoWater::GeneratePlaceableWater(TESObjectCELL* apCell) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSAutoWater.cpp", 717
    );
    if (BGSAutoWater::pPolygonList) {
        float z = apCell->GetWaterHeight();
        TESObjectLAND* Land = apCell->GetLand();
        float a1 = Land->GetWorldOffsetForBlock(0).y;
        TESObjectLAND* v2 = apCell->GetLand();
        NiPoint3 worldtranslate(v2->GetWorldOffsetForBlock(0).x, a1, z);
        bool bcreatedHavok = false;
        NiTListItem<BSPolygon2D*>* pos = BGSAutoWater::pPolygonList->GetHeadPos();
        while (pos) {
            NiTListItem<BSPolygon2D*>* next = BGSAutoWater::pPolygonList->GetNextPos(pos);
            if (pos) {
                BSPolygon2D* ppoly = BGSAutoWater::pPolygonList->Get(pos);
                if ((ppoly->GetRootNode()) && (ppoly->Get3D())) {
                    BSMultiBoundNode* pmbnode;
                    BSMultiBound* pmb;
                    BSMultiBoundAABB* pmbshape;
                    BSFadeNode* pfadenode;
                    NiStencilProperty* pstencilprop;
                    WaterShaderProperty* pwatershadprop;
                    TESObjectREFR* pwaterRef;
                    ppoly->GetRootNode()->SetName("AutoWaterNode");
                    ppoly->Get3D()->SetName("AutoWater");
                    TESObjectREFR* v41 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                    if (v41) {
                        pwaterRef = ::new (v41) TESObjectREFR;
                    }
                    else {
                        pwaterRef = nullptr;
                    }
                    WaterShaderProperty* v40 =
                        static_cast<WaterShaderProperty*>(NiMemObject::operator new(sizeof(WaterShaderProperty)));
                    if (v40) {
                        pwatershadprop = ::new (v40) WaterShaderProperty;
                    }
                    else {
                        pwatershadprop = nullptr;
                    }
                    ppoly->Get3D()->AttachProperty(pwatershadprop);
                    BSShaderManager::PrepareObject(ppoly->GetRootNode(), 0, 0);
                    NiStencilProperty* v39 =
                        static_cast<NiStencilProperty*>(NiMemObject::operator new(sizeof(NiStencilProperty)));
                    if (v39) {
                        pstencilprop = ::new (v39) NiStencilProperty;
                    }
                    else {
                        pstencilprop = nullptr;
                    }
                    pstencilprop->SetDrawMode(NiStencilProperty::DRAW_BOTH);
                    ppoly->Get3D()->AttachProperty(pstencilprop);
                    TESWaterForm* WaterType = apCell->GetWaterType();
                    BGSPlaceableWater* PlaceableAutoWater = WaterType->GetPlaceableAutoWater();
                    pwaterRef->SetObjectReference(PlaceableAutoWater);
                    BSFadeNode* v38 = static_cast<BSFadeNode*>(NiMemObject::operator new(sizeof(BSFadeNode)));
                    if (v38) {
                        pfadenode = ::new (v38) BSFadeNode;
                    }
                    else {
                        pfadenode = nullptr;
                    }
                    pfadenode->SetIgnoreFade(true);
                    pwaterRef->Set3D(pfadenode, true);
                    pfadenode->SetTranslate(worldtranslate);
                    pwaterRef->SetLocationOnReferenceSimple(worldtranslate);
                    BSMultiBoundAABB* v37 =
                        static_cast<BSMultiBoundAABB*>(NiMemObject::operator new(sizeof(BSMultiBoundAABB)));
                    if (v37) {
                        pmbshape = ::new (v37) BSMultiBoundAABB;
                    }
                    else {
                        pmbshape = nullptr;
                    }
                    float minx = 0.0f;
                    float miny = 0.0f;
                    float minz = 0.0f;
                    float maxx = 0.0f;
                    float maxy = 0.0f;
                    float maxz = 0.0f;
                    NiTriShape* ptristrips = ppoly->Get3D();
                    const NiPoint3* pvertices = ptristrips->GetVertices();
                    for (int32_t i = 0;; ++i) {
                        uint16_t VertexCount = ptristrips->GetVertexCount();
                        if (i >= VertexCount) {
                            break;
                        }
                        const NiPoint3* v12 = &pvertices[i];
                        float v46 = v12->x;
                        float y = v12->y;
                        float v48 = v12->z;
                        if (i <= 0) {
                            maxx = v46;
                            minx = v46;
                            maxy = y;
                            miny = y;
                            maxz = v48;
                            minz = v48;
                        }
                        else {
                            minx = BSMIN(minx, v46);
                            miny = BSMIN(miny, y);
                            minz = BSMIN(minz, v48);
                            maxx = BSMAX(maxx, v46);
                            maxy = BSMAX(maxy, y);
                            maxz = BSMAX(maxz, v48);
                        }
                    }
                    float v27 = (maxz - minz) / 2.0;
                    float za = std::fabs(v27);
                    float v26 = (maxy - miny) / 2.0;
                    float a1a = std::fabs(v26);
                    float v25 = (maxx - minx) / 2.0;
                    float x = std::fabs(v25);
                    NiPoint3 halfextents(x, a1a, za);
                    float v24 = maxz - halfextents.z;
                    float v23 = maxy - halfextents.y;
                    float v22 = maxx - halfextents.x;
                    NiPoint3 center(v22, v23, v24);
                    center.operator+=(worldtranslate);
                    pmbshape->SetCenter(center);
                    pmbshape->SetHalfExtents(halfextents);
                    BSMultiBound* v36 = static_cast<BSMultiBound*>(NiMemObject::operator new(sizeof(BSMultiBound)));
                    if (v36) {
                        pmb = ::new (v36) BSMultiBound;
                    }
                    else {
                        pmb = nullptr;
                    }
                    pmb->SetShape(pmbshape);
                    BSMultiBoundNode* v35 =
                        static_cast<BSMultiBoundNode*>(NiMemObject::operator new(sizeof(BSMultiBoundNode)));
                    if (v35) {
                        pmbnode = ::new (v35) BSMultiBoundNode;
                    }
                    else {
                        pmbnode = nullptr;
                    }
                    pmbnode->SetMultiBound(pmb);
                    pfadenode->AttachChild(pmbnode, 1);
                    pmbnode->AttachChild(ppoly->GetRootNode(), 1);
                    NiUpdateData tmp(0.0f, false, false);
                    pfadenode->Update(tmp);
                    apCell->AddReference(pwaterRef, false);
                    pwaterRef->SetTemporary();
                    if (bAutoWaterUsesHavokShapes.Bool() && !bcreatedHavok) {
                        pwaterRef->InitHavokForPlaceableWater();
                        bcreatedHavok = true;
                    }
                    if (ppoly) {
                        delete ppoly;
                    }
                    ppoly = nullptr;
                    BGSAutoWater::pPolygonList->RemovePos(pos);
                }
            }
            pos = next;
        }
    }
}

bool BGSAutoWater::IsPolyonInsidePolygon(BSPolygon2D* apPoly1, BSPolygon2D* apPoly2) {
    for (int32_t i = 0;; ++i) {
        int32_t Target = apPoly1->iVertexCount;
        if (i >= Target) {
            break;
        }
        NiPoint3* v3 = &apPoly1->pVertexSimpleArray->operator[](i);
        float z = v3->z;
        NiPoint2 vec2(v3->x, v3->y);
        if (apPoly2->IsPointInsidePoly(vec2)) {
            return true;
        }
    }
    return false;
}

int32_t BGSAutoWater::WhichCellEdge(TESObjectCELL* apCell, NiPoint3 arVert) {
    NiPoint3 offset;
    int32_t edgeFlags = 0;
    TESObjectLAND* Land = apCell->GetLand();
    offset = Land->GetWorldOffsetForBlock(0);
    arVert.operator+=(offset);
    float a2 = static_cast<float>(apCell->GetDataY() << 12);
    float a1 = static_cast<float>(apCell->GetDataX() << 12);
    NiPoint2 corner1(a1, a2);
    float a2a = static_cast<float>(apCell->GetDataY() << 12);
    float a1a = static_cast<float>((apCell->GetDataX() << 12) + 0x1000);
    NiPoint2 corner2(a1a, a2a);
    float a2b = static_cast<float>((apCell->GetDataY() << 12) + 0x1000);
    float a1b = static_cast<float>(apCell->GetDataX() << 12);
    NiPoint2 corner3(a1b, a2b);
    float a2c = static_cast<float>((apCell->GetDataY() << 12) + 0x1000);
    float a1c = static_cast<float>((apCell->GetDataX() << 12) + 0x1000);
    NiPoint2 corner4(a1c, a2c);
    if (corner1.x == arVert.x && corner1.y == arVert.y) {
        return 9;
    }
    if (corner2.x == arVert.x && corner2.y == arVert.y) {
        return 10;
    }
    if (corner3.x == arVert.x && corner3.y == arVert.y) {
        return 5;
    }
    if (corner4.x == arVert.x && corner4.y == arVert.y) {
        return 6;
    }
    if (corner1.x == arVert.x) {
        return 1;
    }
    if (corner2.x == arVert.x) {
        return 2;
    }
    if (corner1.y == arVert.y) {
        return 8;
    }
    if (corner4.y == arVert.y) {
        return 4;
    }
    return edgeFlags;
}

NiNode* BSPolygon2D::GetRootNode() {
    return spRootNode;
}

NiTriShape* BSPolygon2D::Get3D() {
    return sp3D;
}

void BSFadeNode::SetIgnoreFade(bool abIgnoreFade) {
    NiAVObject::SetIgnoreFade(abIgnoreFade);
}

NiPoint3** TESObjectLAND::GetVertexArray() {
    if (pLoadedData) {
        return pLoadedData->ppVertices;
    }

    return nullptr;
}

NiStencilProperty::~NiStencilProperty() {}
