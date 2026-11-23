#include "Fallout Shared/Pathfinding/NavMesh_Util.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XInputManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshGrid.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>
#include <limits>

int32_t CompareUShort(const uint16_t* apOne, const uint16_t* apTwo) {
    if (*apOne > *apTwo) {
        return 1;
    }
    if (*apTwo <= *apOne) {
        return 0;
    }
    return -1;
}

POVVertexData::POVVertexData() {}

POVVertexData::~POVVertexData() {}

void POVVertexData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

AngleSinCos AngleSinCos::operator-(const AngleSinCos& aSinCos) const {
    AngleSinCos kResult;
    AngleSinCos kDifference;
    kDifference.fCos = fCos * aSinCos.fCos + fSin * aSinCos.fSin;
    kDifference.fSin = fSin * aSinCos.fCos - fCos * aSinCos.fSin;
    kResult = kDifference;
    return kResult;
}

AngleSinCos& AngleSinCos::operator=(const AngleSinCos& aSinCos) {
    fCos = aSinCos.fCos;
    fSin = aSinCos.fSin;
    return *this;
}

FastNavMeshEdgeHandle* FastNavMeshEdgeHandle::Copy(const FastNavMeshEdgeHandle& arOther) {
    pNavMesh = arOther.pNavMesh;
    usTriangle = arOther.usTriangle;
    iEdgeIndex = arOther.iEdgeIndex;
    return this;
}

// The arrays are emptied first and the info record forgets the navmesh; the compiler destroys the members and bases.
NavMesh::~NavMesh() {
    Vertices.Clear(true);
    Triangles.Clear(true);
    ExtraEdgeInfo.Clear(true);
    DoorPortals.Clear(true);
    CoverArray.Clear(true);
    MeshGrid.Clear();
    if (pNavMeshInfo) {
        pNavMeshInfo->SetNavMesh(nullptr);
    }
    if (pTriangleToObstacleMap) {
        delete pTriangleToObstacleMap;
    }
}

NiPoint3 NavMesh::GetNormal(uint16_t ausTriangle) {
    const NavMeshTriangle* ptri = GetTriangle(ausTriangle);
    NavMeshVertex* pVertex0 = GetVertex(ptri->GetVert(0));
    NavMeshVertex* pVertex1 = GetVertex(ptri->GetVert(1u));
    NiPoint3 edge1 = pVertex1->Location - pVertex0->Location;
    pVertex0 = GetVertex(ptri->GetVert(0));
    NavMeshVertex* pVertex2 = GetVertex(ptri->GetVert(2u));
    NiPoint3 edge2 = pVertex2->Location - pVertex0->Location;
    return edge1.UnitCross(edge2);
}

bool NavMesh::ReconnectEnabledPortals() {
    for (uint32_t i = 0; i < GetTriangleCount(); ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        if (ptri && ptri->HasEdgeExtraInfo()) {
            for (int32_t e = 0; e < 3; ++e) {
                if (ptri->EdgeHasExtraInfo(e)) {
                    EdgeExtraInfo* pinfo = GetExtraEdgeInfo(ptri->GetTriOrExtraInfo(e));
                    NavMeshPtr spotherMesh;
                    if (pinfo->Portal.pOtherMesh->GetNavMesh(spotherMesh)) {
                        NavMeshTriangle* potherTri = spotherMesh->GetTriangle(pinfo->Portal.sTriangle);
                        if (potherTri) {
                            int32_t iPortalCount = 0;
                            int32_t iPortalEdge = -1;
                            for (int32_t f = 0; f < 3; ++f) {
                                if (potherTri->EdgeHasExtraInfo(f)) {
                                    ++iPortalCount;
                                    iPortalEdge = f;
                                }
                            }
                            if (iPortalCount) {
                                // This edge's two ends; the other mesh's edge runs the other way, so its first end
                                // meets this edge's second.
                                NiPoint3 kVert2;
                                NiPoint3 kVert1;
                                GetEdgeVertices(i, e, &kVert1, &kVert2);
                                for (int32_t iOtherEdge = 0; iOtherEdge < 3; ++iOtherEdge) {
                                    NiPoint3 kOtherVert1;
                                    NiPoint3 kOtherVert2;
                                    spotherMesh->GetEdgeVertices(
                                        pinfo->Portal.sTriangle,
                                        iOtherEdge,
                                        &kOtherVert1,
                                        &kOtherVert2
                                    );
                                    if ((kVert1 - kOtherVert2).SqrLength() < 400.0 &&
                                        (kVert2 - kOtherVert1).SqrLength() < 400.0) {
                                        iPortalEdge = iOtherEdge;
                                        iPortalCount = 1;
                                        break;
                                    }
                                }
                            }
                            if (iPortalCount == 1) {
                                EdgeExtraInfo* potherExtraInfo = spotherMesh->SetEmptyEdgeExtraInfo(
                                    pinfo->Portal.sTriangle,
                                    iPortalEdge
                                );
                                potherExtraInfo->Portal.pOtherMesh = GetNavMeshInfo();
                                potherExtraInfo->Portal.sTriangle = i;
                                potherExtraInfo->eType = EIT_ENABLE_DISABLE_PORTAL;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

uint32_t NavMesh::GetUndoDataIndexForObstacle(const NiPointer<ObstacleData>& apObstacle) {
    for (uint32_t i = 0; i < Obstacles.iSize; ++i) {
        if (Obstacles.GetAt(static_cast<int32_t>(i))->spObstacle == apObstacle) {
            return i;
        }
    }
    return -1;
}

uint32_t
NavMesh::GetUndoDataIndexForObstacle(const NiPointer<ObstacleData>& apObstacle, ObstacleUndoData** apOutUndoData) {
    uint32_t i = GetUndoDataIndexForObstacle(apObstacle);
    if (i == -1) {
        *apOutUndoData = nullptr;
        return -1;
    }

    *apOutUndoData = Obstacles.GetAt(static_cast<int32_t>(i)).m_pObject;
    return i;
}

bool NavMesh::RemoveObstacle(uint32_t aiUndoIndex, TaskPortalSwap& aPortalSwaps) {
    ObstacleUndoData* pundoData = Obstacles.GetAt(static_cast<int32_t>(aiUndoIndex)).m_pObject;
    if (aiUndoIndex == Obstacles.iSize - 1) {
        NavMeshObstacleCutter cutter;
        if (pundoData->eState == ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE) {
            cutter.UndoNavMeshFromObstacleUndoData(this, pundoData, aPortalSwaps);
        }
        cutter.ResetNavMeshFromObstacleUndoData(this, pundoData);
        RemoveObstacleUndoData(aiUndoIndex);
    }
    else {
        if (GetUndoDataMaxObstacleCount(pundoData)) {
            uint32_t iobstacleIndex;
            NiPointer<ObstacleData> obstaclesToReAdd[100];
            uint32_t iobstacleToReAddCount = 0;
            NavMeshObstacleCutter cutter;
            for (iobstacleIndex = Obstacles.iSize - 1; iobstacleIndex != aiUndoIndex - 1; --iobstacleIndex) {
                if (Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex))->eState ==
                    ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE) {
                    cutter.UndoNavMeshFromObstacleUndoData(
                        this,
                        Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)).m_pObject,
                        aPortalSwaps
                    );
                    if (iobstacleIndex > aiUndoIndex) {
                        obstaclesToReAdd[iobstacleToReAddCount++] = Obstacles
                                                                        .GetAt(static_cast<int32_t>(iobstacleIndex))
                                                                        ->spObstacle;
                    }
                }
                cutter.ResetNavMeshFromObstacleUndoData(
                    this,
                    Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)).m_pObject
                );
                RemoveObstacleUndoData(iobstacleIndex);
            }
            while (iobstacleIndex != -1 &&
                   Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex))->eState ==
                       ObstacleUndoData::OBSTACLE_UNDO_STATE_UNDONE) {
                cutter.ResetNavMeshFromObstacleUndoData(
                    this,
                    Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)).m_pObject
                );
                RemoveObstacleUndoData(iobstacleIndex--);
            }
            for (uint32_t i = 0; i < iobstacleToReAddCount; ++i) {
                cutter.SplitNavMeshFromObstacle(this, obstaclesToReAdd[i], aPortalSwaps);
            }
        }
        else {
            NavMeshObstacleCutter cutter;
            if (pundoData->eState == ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE) {
                cutter.UndoNavMeshFromObstacleUndoData(this, pundoData, aPortalSwaps);
            }
        }
    }
    return true;
}

bool NavMesh::RemoveObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps) {
    ObstacleUndoData* pUndoData;
    uint32_t uiUndoDataIndex = GetUndoDataIndexForObstacle(apObstacle, &pUndoData);
    return uiUndoDataIndex != -1 && RemoveObstacle(uiUndoDataIndex, aPortalSwaps);
}

// Builds the points of visibility: each vertex where exactly two boundary or portal edges meet and the turn between
// them qualifies gets a POV, then each triangle records its closest POV within reach.
void NavMesh::CreatePOVs() {
    POVs.RemoveAll();
    ClosestPOVs.Clear(true);
    using VertexMap_t = NiTMap<FastNavMeshVertHandle, POVVertexData*>;
    VertexMap_t VertexMap(37u);
    uint32_t itriCount = GetTriangleCount();
    for (uint32_t i = 0; i < itriCount; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t e = 0; e < 3; ++e) {
            if (ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                FastNavMeshEdgeHandle hedge(this, i, e);
                FastNavMeshVertHandle hvert1(this, ptri->GetVert(static_cast<uint32_t>(e)));
                POVVertexData* pdata1;
                FastNavMeshVertHandle key(hvert1);
                if (!VertexMap.GetAt(key, pdata1)) {
                    pdata1 = new POVVertexData;
                    pdata1->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata1->hVertex) FastNavMeshVertHandle(hvert1);
                    key = hvert1;
                    VertexMap.SetAt(key, pdata1);
                }
                pdata1->Edges.Add(hedge);
                FastNavMeshVertHandle hvert2(this, ptri->GetVert((e + 1) % 3));
                POVVertexData* pdata2;
                key = hvert2;
                if (!VertexMap.GetAt(key, pdata2)) {
                    pdata2 = new POVVertexData;
                    pdata2->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata2->hVertex) FastNavMeshVertHandle(hvert2);
                    key = hvert2;
                    VertexMap.SetAt(key, pdata2);
                }
                pdata2->Edges.Add(hedge);
            }
            else if (ptri->EdgeHasExtraInfo(e)) {
                FastNavMeshEdgeHandle hedge(this, i, e);
                FastNavMeshVertHandle hvert1(this, ptri->GetVert(static_cast<uint32_t>(e)));
                POVVertexData* pdata1;
                FastNavMeshVertHandle key(hvert1);
                if (!VertexMap.GetAt(key, pdata1)) {
                    pdata1 = new POVVertexData;
                    pdata1->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata1->hVertex) FastNavMeshVertHandle(hvert1);
                    key = hvert1;
                    VertexMap.SetAt(key, pdata1);
                }
                pdata1->PortalEdges.Add(hedge);
                FastNavMeshVertHandle hvert2(this, ptri->GetVert((e + 1) % 3));
                POVVertexData* pdata2;
                key = hvert2;
                if (!VertexMap.GetAt(key, pdata2)) {
                    pdata2 = new POVVertexData;
                    pdata2->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata2->hVertex) FastNavMeshVertHandle(hvert2);
                    key = hvert2;
                    VertexMap.SetAt(key, pdata2);
                }
                pdata2->PortalEdges.Add(hedge);
            }
        }
    }
    NiTMapItem<FastNavMeshVertHandle, POVVertexData*>* it = VertexMap.GetFirstPos();
    while (it) {
        FastNavMeshVertHandle hvert;
        POVVertexData* pdata;
        VertexMap.GetNext(it, hvert, pdata);
        uint32_t iedgeCount = pdata->Edges.iSize;
        uint32_t iportalEdgeCount = pdata->PortalEdges.iSize;
        if (iportalEdgeCount + iedgeCount != 2) {
            continue;
        }
        FastNavMeshEdgeHandle hdataEdge0;
        FastNavMeshEdgeHandle hdataEdge1;
        if (iedgeCount == 2) {
            hdataEdge0.Copy(pdata->Edges.GetAt(0));
            hdataEdge1.Copy(pdata->Edges.GetAt(1));
        }
        else if (iedgeCount == 1) {
            hdataEdge0.Copy(pdata->Edges.GetAt(0));
            hdataEdge1.Copy(pdata->PortalEdges.GetAt(0));
        }
        else {
            continue;
        }
        FastNavMeshEdgeHandle aEdge1;
        FastNavMeshEdgeHandle aEdge2;
        NavMeshEdgeVertices verts1 = GetEdgeVertices(hdataEdge0.usTriangle, hdataEdge0.iEdgeIndex);
        NavMeshEdgeVertices verts2 = GetEdgeVertices(hdataEdge1.usTriangle, hdataEdge1.iEdgeIndex);
        // The shared vertex, and the far ends of the edge into it and of the edge out of it.
        NiPoint3* pEnd1;
        NiPoint3* pShared;
        NiPoint3* pEnd2;
        if (verts1.Vertices[0] == verts2.Vertices[1]) {
            aEdge1.Copy(hdataEdge0);
            aEdge2.Copy(hdataEdge1);
            pEnd1 = &verts1.Vertices[1]->Location;
            pShared = &verts1.Vertices[0]->Location;
            pEnd2 = &verts2.Vertices[0]->Location;
        }
        else if (verts1.Vertices[1] == verts2.Vertices[0]) {
            aEdge1.Copy(hdataEdge1);
            aEdge2.Copy(hdataEdge0);
            pEnd1 = &verts2.Vertices[1]->Location;
            pShared = &verts2.Vertices[0]->Location;
            pEnd2 = &verts1.Vertices[0]->Location;
        }
        else {
            continue;
        }
        NiPoint3 edge1 = *pEnd1 - *pShared;
        NiPoint3 edge2 = *pEnd2 - *pShared;
        edge1.z = 0.0f;
        edge2.z = 0.0f;
        edge1.Unitize();
        edge2.Unitize();
        NiPoint3 cross = edge1.Cross(edge2);
        float fdot = edge1.Dot(edge2);
        bool badd = false;
        if (fdot > 0.0) {
            if (cross.z < 0.10000000149011612) {
                badd = true;
                if (cross.z > 0.0) {
                    NiPoint3 temp = edge1;
                    edge1 = edge2;
                    edge2 = temp;
                }
            }
        }
        else if (cross.z < -0.10000000149011612) {
            badd = true;
        }
        if (badd || iedgeCount == 1) {
            NavMeshPOVData pov;
            pov.Edge1.Set(aEdge1);
            pov.Edge2.Set(aEdge2);
            POVs.SetAt(hvert.usVertex, pov);
        }
    }
    it = VertexMap.GetFirstPos();
    while (it) {
        FastNavMeshVertHandle hvert;
        POVVertexData* pdata;
        VertexMap.GetNext(it, hvert, pdata);
        if (pdata) {
            delete pdata;
        }
    }
    const float fMaxDist = 128.0f;
    for (uint32_t t = 0; t < itriCount; ++t) {
        NiPoint3 circumCenter = GetCircumCenter(static_cast<uint16_t>(t));
        NiPoint3 vertLoc = GetVertex(GetTriangle(static_cast<uint16_t>(t))->GetVert(0))->Location;
        float fradius = (vertLoc - circumCenter).XYLength();
        float fmaxRadius = fradius + fMaxDist;
        float fmaxRadiusSqr = fmaxRadius * fmaxRadius;
        uint16_t iClosestPOV = 0xFFFF;
        float fclosestPOVSqrDist = fmaxRadiusSqr * fmaxRadiusSqr;
        NiTMapItem<uint16_t, NavMeshPOVData>* pos = POVs.GetFirstPos();
        while (pos) {
            uint16_t iVertIndex;
            NavMeshPOVData data;
            POVs.GetNext(pos, iVertIndex, data);
            NiPoint3 delta = GetVertex(iVertIndex)->Location - circumCenter;
            float fsqrDist = delta.SqrXYLength();
            if (fclosestPOVSqrDist > fsqrDist && std::fabs(delta.z) < 180.0) {
                fclosestPOVSqrDist = fsqrDist;
                iClosestPOV = iVertIndex;
            }
        }
        ClosestPOVs.Add(iClosestPOV);
    }
}

// Lists every triangle in each grid cell it overlaps: the cells of its three vertices, then every cell of their
// bounding range whose square holds a vertex, has a corner over the triangle or has an edge crossing one of the
// triangle's. Cell indices are floorf results converted to 64 bits with the low 32 bits kept, and compared unsigned. In
// the second vertex's square test the cell corner's y is compared with the third vertex's y, deliberately.
void NavMesh::CreateGrid() {
    int32_t iNumTriangles = static_cast<int32_t>(GetTriangleCount());
    MeshGrid.Clear();
    if (!iNumTriangles) {
        return;
    }
    NiPoint3 kMin;
    NiPoint3 kMax;
    ComputeBounds(&kMin, &kMax);
    MeshGrid.SetBoundingExtents(kMin, kMax, static_cast<uint32_t>(iNumTriangles));
    uint32_t uiGridSize = MeshGrid.GetGridSize();
    float fColumnSectionLen = MeshGrid.fColumnSectionLen;
    float fRowSectionLen = MeshGrid.fRowSectionLen;
    NiPoint3 kCorner00;
    NiPoint3 kCorner10;
    NiPoint3 kCorner01;
    NiPoint3 kCorner11;
    NavMeshVertex kVertex;
    NiPoint3 akVerts[3];
    for (int32_t i = 0; i < iNumTriangles; ++i) {
        NavMeshTriangle* pTriangle = GetTriangle(static_cast<uint16_t>(i));
        // Each vertex's column [0] and row [1].
        uint32_t auiCells[3][2];
        for (int32_t j = 0; j < 3; ++j) {
            kVertex = *GetVertex(pTriangle->GetVert(j));
            akVerts[j] = kVertex.Location;
            float fColumn = (kVertex.Location.x - kMin.x) / fColumnSectionLen;
            auiCells[j][0] = static_cast<uint32_t>(static_cast<int64_t>(std::floorf(fColumn)));
            float fRow = (kVertex.Location.y - kMin.y) / fRowSectionLen;
            auiCells[j][1] = static_cast<uint32_t>(static_cast<int64_t>(std::floorf(fRow)));
            if (auiCells[j][0] >= uiGridSize) {
                auiCells[j][0] = uiGridSize - 1;
            }
            if (auiCells[j][1] >= uiGridSize) {
                auiCells[j][1] = uiGridSize - 1;
            }
        }
        uint32_t uiMaxColumn = auiCells[0][0];
        uint32_t uiMinColumn = uiMaxColumn;
        uint32_t uiMaxRow = auiCells[0][1];
        uint32_t uiMinRow = uiMaxRow;
        for (int32_t k = 1; k < 3; ++k) {
            if (auiCells[k][0] < uiMinColumn) {
                uiMinColumn = auiCells[k][0];
            }
            if (auiCells[k][0] > uiMaxColumn) {
                uiMaxColumn = auiCells[k][0];
            }
            if (auiCells[k][1] < uiMinRow) {
                uiMinRow = auiCells[k][1];
            }
            if (auiCells[k][1] > uiMaxRow) {
                uiMaxRow = auiCells[k][1];
            }
        }
        if (uiMinColumn == uiMaxColumn && uiMinRow == uiMaxRow) {
            MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiMinColumn, uiMinRow);
            continue;
        }
        for (uint32_t uiColumn = uiMinColumn; uiColumn <= uiMaxColumn; ++uiColumn) {
            for (uint32_t uiRow = uiMinRow; uiRow <= uiMaxRow; ++uiRow) {
                if ((auiCells[0][0] == uiColumn && auiCells[0][1] == uiRow) ||
                    (auiCells[1][0] == uiColumn && auiCells[1][1] == uiRow) ||
                    (auiCells[2][0] == uiColumn && auiCells[2][1] == uiRow)) {
                    MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiColumn, uiRow);
                    continue;
                }
                kCorner00 = MeshGrid.GetGridPosition(uiColumn, uiRow);
                kCorner10 = MeshGrid.GetGridPosition(uiColumn + 1, uiRow);
                kCorner01 = MeshGrid.GetGridPosition(uiColumn, uiRow + 1);
                kCorner11 = MeshGrid.GetGridPosition(uiColumn + 1, uiRow + 1);
                if ((kCorner11.x >= akVerts[0].x &&
                     kCorner11.y >= akVerts[0].y &&
                     kCorner00.x <= akVerts[0].x &&
                     kCorner00.y <= akVerts[0].y) ||
                    (kCorner11.x >= akVerts[1].x &&
                     kCorner11.y >= akVerts[1].y &&
                     kCorner00.x <= akVerts[1].x &&
                     kCorner00.y <= akVerts[2].y) ||
                    (kCorner11.x >= akVerts[2].x &&
                     kCorner11.y >= akVerts[2].y &&
                     kCorner00.x <= akVerts[2].x &&
                     kCorner00.y <= akVerts[2].y)) {
                    MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiColumn, uiRow);
                    continue;
                }
                float fZDiff;
                if (IsPointInsideTriangle(
                        static_cast<uint16_t>(i),
                        kCorner00,
                        fZDiff,
                        std::numeric_limits<float>::max(),
                        180.0f
                    ) ||
                    IsPointInsideTriangle(
                        static_cast<uint16_t>(i),
                        kCorner10,
                        fZDiff,
                        std::numeric_limits<float>::max(),
                        180.0f
                    ) ||
                    IsPointInsideTriangle(
                        static_cast<uint16_t>(i),
                        kCorner01,
                        fZDiff,
                        std::numeric_limits<float>::max(),
                        180.0f
                    ) ||
                    IsPointInsideTriangle(
                        static_cast<uint16_t>(i),
                        kCorner11,
                        fZDiff,
                        std::numeric_limits<float>::max(),
                        180.0f
                    )) {
                    MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiColumn, uiRow);
                    continue;
                }
                NiPoint3 kSquareStart;
                NiPoint3 kSquareEnd;
                NiPoint3 kEdgeStart;
                NiPoint3 kEdgeEnd;
                bool bIntersects = false;
                for (int32_t iSide = 0; iSide < 4 && !bIntersects; ++iSide) {
                    switch (iSide) {
                        case 0:
                            kSquareStart = kCorner00;
                            kSquareEnd = kCorner10;
                            break;
                        case 1:
                            kSquareStart = kCorner10;
                            kSquareEnd = kCorner11;
                            break;
                        case 2:
                            kSquareStart = kCorner11;
                            kSquareEnd = kCorner01;
                            break;
                        case 3:
                            kSquareStart = kCorner01;
                            kSquareEnd = kCorner00;
                            break;
                        default: break;
                    }
                    for (int32_t iEdge = 0; iEdge < 3; ++iEdge) {
                        if (iEdge == 0) {
                            kEdgeStart = akVerts[0];
                            kEdgeEnd = akVerts[1];
                        }
                        else if (iEdge == 1) {
                            kEdgeStart = akVerts[1];
                            kEdgeEnd = akVerts[2];
                        }
                        else if (iEdge == 2) {
                            kEdgeStart = akVerts[2];
                            kEdgeEnd = akVerts[0];
                        }
                        if (NavMeshMath::Find2DIntersection3(kSquareStart, kSquareEnd, kEdgeStart, kEdgeEnd)) {
                            bIntersects = true;
                            break;
                        }
                    }
                }
                if (bIntersects) {
                    MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiColumn, uiRow);
                }
            }
        }
    }
}

int32_t NavMesh::FindIntersectingEdgeFast(
    uint16_t ausTri,
    int32_t aIgnoreEdge,
    const NiPoint3& aStartPoint,
    const NiPoint3& aEndPoint
) {
    NiPoint3 edgeVerts[3];
    NavMeshTriangle* ptri = GetTriangle(ausTri);
    edgeVerts[0] = GetVertex(ptri->GetVert(0))->Location;
    edgeVerts[1] = GetVertex(ptri->GetVert(1u))->Location;
    edgeVerts[2] = GetVertex(ptri->GetVert(2u))->Location;
    for (int32_t i = 0; i < 3; ++i) {
        if (i != aIgnoreEdge) {
            int32_t j = i + 1;
            if (i == 2) {
                j = 0;
            }
            float fa1 = (edgeVerts[i].x - aEndPoint.x) * (edgeVerts[j].y - aEndPoint.y) -
                (edgeVerts[i].y - aEndPoint.y) * (edgeVerts[j].x - aEndPoint.x);
            float fa2 = (edgeVerts[i].x - aStartPoint.x) * (edgeVerts[j].y - aStartPoint.y) -
                (edgeVerts[i].y - aStartPoint.y) * (edgeVerts[j].x - aStartPoint.x);
            float fa4 = (aStartPoint.x - edgeVerts[j].x) * (aEndPoint.y - edgeVerts[j].y) -
                (aStartPoint.y - edgeVerts[j].y) * (aEndPoint.x - edgeVerts[j].x);
            if (fa1 * fa2 <= 0.0) {
                float fa3 = (aStartPoint.x - edgeVerts[i].x) * (aEndPoint.y - edgeVerts[i].y) -
                    (aStartPoint.y - edgeVerts[i].y) * (aEndPoint.x - edgeVerts[i].x);
                if (fa3 * fa4 <= 0.0 && fa1 != 0.0 && fa4 != 0.0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// The form ID is read before the NavMeshInfo map.
bool NavMesh::UpdateNavMeshInfo() {
    uint32_t uiFormID = GetFormID();
    NavMeshInfo* pInfo = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(uiFormID);
    if (GetNavMeshInfo()) {
        pInfo->CreateFromNavMesh(this);
        return false;
    }
    NavMeshInfo* pNewInfo = new NavMeshInfo;
    pNewInfo->CreateFromNavMesh(this);
    SetNavMeshInfo(pNewInfo);
    pNewInfo->SetNavMesh(this);
    TES::pInstance->GetNavMeshInfoMap()->AddNavMeshInfo(pNewInfo);
    return true;
}

bool NavMesh::AddTrianglesToGrid(uint16_t ausFirstTriangleIndex, uint16_t ausLastTriangleIndex) {
    uint32_t istartRow;
    uint32_t iendRow;
    uint32_t istartCol;
    uint32_t iendCol;
    for (uint32_t t = ausFirstTriangleIndex; t <= ausLastTriangleIndex; ++t) {
        const NavMeshTriangle* ptri = GetTriangle(t);
        NiPoint3 triMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0.0f);
        NiPoint3 triMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0.0f);
        for (int32_t j = 0; j < 3; ++j) {
            NiPoint3* loc = &GetVertex(ptri->GetVert(j))->Location;
            if (triMin.x > static_cast<double>(loc->x)) {
                triMin.x = loc->x;
            }
            if (triMax.x < static_cast<double>(loc->x)) {
                triMax.x = loc->x;
            }
            if (triMin.y > static_cast<double>(loc->y)) {
                triMin.y = loc->y;
            }
            if (triMax.y < static_cast<double>(loc->y)) {
                triMax.y = loc->y;
            }
        }
        istartRow = 0;
        istartCol = 0;
        iendRow = 0;
        iendCol = 0;
        MeshGrid.GetGridRowAndColumn(triMin, &istartRow, &istartCol);
        MeshGrid.GetGridRowAndColumn(triMax, &iendRow, &iendCol);
        for (uint32_t i = istartRow; i <= iendRow; ++i) {
            for (uint32_t j = istartCol; j <= iendCol; ++j) {
                BSSimpleArray<uint16_t, 1024>* pcellarray = MeshGrid.GetTriangleArray(j, i);
                if (pcellarray) {
                    uint16_t usTriangle = t;
                    pcellarray->Add(usTriangle);
                }
            }
        }
    }
    return true;
}

bool NavMesh::RemoveTrianglesFromGrid(uint16_t ausFirstTriangleIndex, uint16_t ausLastTriangleIndex) {
    uint32_t istartRow;
    uint32_t iendRow;
    uint32_t istartCol;
    uint32_t iendCol;
    uint32_t t;
    for (t = ausFirstTriangleIndex; t <= ausLastTriangleIndex; ++t) {
        const NavMeshTriangle* ptri = GetTriangle(t);
        NiPoint3 triMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0.0f);
        NiPoint3 triMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0.0f);
        for (int32_t j = 0; j < 3; ++j) {
            NiPoint3* loc = &GetVertex(ptri->GetVert(j))->Location;
            if (triMin.x > static_cast<double>(loc->x)) {
                triMin.x = loc->x;
            }
            if (triMax.x < static_cast<double>(loc->x)) {
                triMax.x = loc->x;
            }
            if (triMin.y > static_cast<double>(loc->y)) {
                triMin.y = loc->y;
            }
            if (triMax.y < static_cast<double>(loc->y)) {
                triMax.y = loc->y;
            }
        }
        istartRow = 0;
        istartCol = 0;
        iendRow = 0;
        iendCol = 0;
        MeshGrid.GetGridRowAndColumn(triMin, &istartRow, &istartCol);
        MeshGrid.GetGridRowAndColumn(triMax, &iendRow, &iendCol);
        for (uint32_t i = istartRow; i <= iendRow; ++i) {
            for (uint32_t j = istartCol; j <= iendCol; ++j) {
                BSSimpleArray<uint16_t, 1024>* pcellarray = MeshGrid.GetTriangleArray(j, i);
                uint16_t usTriangle = static_cast<uint16_t>(t);
                uint32_t iindex = pcellarray->Find(
                    usTriangle,
                    0,
                    &BSSimpleArray<uint16_t, 1024>::DefaultEqualFunctionByAddress
                );
                pcellarray->RemoveFast(iindex, 1u);
            }
        }
    }
    return true;
}

void NavMesh::RemoveObstacleUndoData(uint32_t aiUndoDataIndex) {
    Obstacles.RemoveFast(aiUndoDataIndex, 1);
}

bool NavMesh::GetObstacleForTriangle(uint16_t ausTriangle, NiPointer<ObstacleData>& spRetObstacle) {
    return pTriangleToObstacleMap->GetAt(ausTriangle, spRetObstacle);
}

uint32_t NavMesh::GetExtraEdgeInfoCount() {
    return ExtraEdgeInfo.iSize;
}

bool NavMesh::GetBounds(NiPoint3& aRetMin, NiPoint3& aRetMax) {
    MeshGrid.GetMeshBounds(aRetMin, aRetMax);
    return true;
}

NiPoint3 NavMesh::GetCircumCenter(uint16_t apTriangle) {
    NiPoint3 kVert1 = GetVertex(GetTriangle(apTriangle)->GetVert(0))->Location;
    NiPoint3 kVert2 = GetVertex(GetTriangle(apTriangle)->GetVert(1))->Location;
    NiPoint3 kVert3 = GetVertex(GetTriangle(apTriangle)->GetVert(2))->Location;
    float fX2 = kVert2.x - kVert1.x;
    float fY2 = kVert2.y - kVert1.y;
    float fX3 = kVert3.x - kVert1.x;
    float fY3 = kVert3.y - kVert1.y;
    float fLength2 = fX2 * fX2 + fY2 * fY2;
    float fLength3 = fX3 * fX3 + fY3 * fY3;
    float fDenominator = fX2 * fY3 - fX3 * fY2;
    float fScale = 0.5 / fDenominator;
    float fZ = kVert3.z;
    if (kVert2.z <= kVert1.z && kVert3.z <= kVert1.z) {
        fZ = kVert1.z;
    }
    else if (kVert3.z <= kVert2.z) {
        fZ = kVert2.z;
    }
    float fY = (fX2 * fLength3 - fX3 * fLength2) * fScale + kVert1.y;
    float fX = kVert1.x - (fY2 * fLength3 - fY3 * fLength2) * fScale;
    return NiPoint3(fX, fY, fZ);
}

// Triangles under water level + 50 are accepted however far below the location they lie; others only down to 180 units
// below. The first one within 40 units in height wins outright.
uint16_t NavMesh::FindTriangleForLocation(const NiPoint3& aLocation, FindTriangleForLocationFilter* apFilter) {
    const BSSimpleArray<uint16_t>* pArray = nullptr;
    MeshGrid.GetTriangleData(aLocation, pArray);
    uint16_t usBestTriangle = 0xFFFF;
    if (!pArray) {
        return 0xFFFF;
    }
    uint32_t uiCount = pArray->GetSize();
    float fBestZDiff = std::numeric_limits<float>::max();
    float fWaterHeight = -std::numeric_limits<float>::max();
    if (pParentCell) {
        pParentCell->GetWaterHeight(aLocation, fWaterHeight);
    }
    float fWaterLimit = fWaterHeight + 50.0;
    bool bUnderWater = aLocation.z < fWaterLimit;
    for (uint32_t i = 0; i < uiCount; ++i) {
        // BSSimpleArray has no const operator[] declared, so the array is const_cast to index it.
        uint16_t usTriangle = (*const_cast<BSSimpleArray<uint16_t>*>(pArray))[i];
        if (Triangles.GetAt(usTriangle).GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
            continue;
        }
        float fZDiff;
        if (!IsPointInsideTriangleFilter(usTriangle, aLocation, fZDiff, apFilter)) {
            continue;
        }
        if (!bUnderWater && fZDiff < -180.0) {
            continue;
        }
        if (fZDiff > 64.0) {
            continue;
        }
        fZDiff = std::fabs(fZDiff);
        if (fBestZDiff > fZDiff) {
            if (fZDiff < 40.0) {
                return usTriangle;
            }
            usBestTriangle = usTriangle;
            fBestZDiff = fZDiff;
        }
    }
    return usBestTriangle;
}

// Without a grid cell for the location every triangle is tried, disabled ones too. A triangle the location lies in
// within 180 units above or 64 below wins over the closest centre; one within 40 units in height wins outright. Each
// loop keeps one height difference variable for all its iterations, holding the last fabs result when
// IsPointInsideTriangle is next handed it.
bool NavMesh::FindClosestTriangleForLocation(const NiPoint3& aLocation, uint16_t* apRetTriIndex, bool* apbOnTriangle) {
    float fClosestCenterDist = std::numeric_limits<float>::max();
    uint16_t usClosestCenter = 0xFFFF;
    uint16_t usBestInside = 0xFFFF;
    const BSSimpleArray<uint16_t>* pArray = nullptr;
    MeshGrid.GetTriangleData(aLocation, pArray);
    if (pArray) {
        uint32_t uiCount = pArray->GetSize();
        float fBestZDiff = std::numeric_limits<float>::max();
        float fZDiff;
        for (uint32_t i = 0; i < uiCount; ++i) {
            uint16_t usTriangle = (*const_cast<BSSimpleArray<uint16_t>*>(pArray))[i];
            if (Triangles.GetAt(usTriangle).GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                continue;
            }
            if (IsPointInsideTriangle(usTriangle, aLocation, fZDiff, 180.0f, 64.0f)) {
                fZDiff = std::fabs(fZDiff);
                if (fBestZDiff > fZDiff) {
                    if (fZDiff < 40.0) {
                        *apRetTriIndex = usTriangle;
                        *apbOnTriangle = true;
                        return true;
                    }
                    usBestInside = usTriangle;
                    fBestZDiff = fZDiff;
                }
            }
            else {
                float fDist = (GetCenter(usTriangle) - aLocation).SqrLength();
                if (fClosestCenterDist > fDist) {
                    usClosestCenter = usTriangle;
                    fClosestCenterDist = fDist;
                }
            }
        }
        if (usBestInside != 0xFFFF) {
            *apRetTriIndex = usBestInside;
            *apbOnTriangle = true;
            return true;
        }
    }
    else {
        uint32_t uiCount = Triangles.GetSize();
        float fBestZDiff = std::numeric_limits<float>::max();
        float fZDiff;
        for (uint32_t i = 0; i < uiCount; ++i) {
            if (IsPointInsideTriangle(static_cast<uint16_t>(i), aLocation, fZDiff, 180.0f, 64.0f)) {
                fZDiff = std::fabs(fZDiff);
                if (fBestZDiff > fZDiff) {
                    if (fZDiff < 40.0) {
                        *apRetTriIndex = static_cast<uint16_t>(i);
                        *apbOnTriangle = true;
                        return true;
                    }
                    usBestInside = static_cast<uint16_t>(i);
                    fBestZDiff = fZDiff;
                }
            }
            else {
                float fDist = (GetCenter(static_cast<uint16_t>(i)) - aLocation).SqrLength();
                if (fClosestCenterDist > fDist) {
                    usClosestCenter = static_cast<uint16_t>(i);
                    fClosestCenterDist = fDist;
                }
            }
        }
    }
    if (usBestInside != 0xFFFF) {
        *apRetTriIndex = usBestInside;
        *apbOnTriangle = true;
        return true;
    }
    if (usClosestCenter == 0xFFFF) {
        *apRetTriIndex = 0xFFFF;
        *apbOnTriangle = false;
        return false;
    }
    *apRetTriIndex = usClosestCenter;
    *apbOnTriangle = false;
    return true;
}

bool NavMesh::IsPointInsideTriangle(
    uint16_t asTriangleIndex,
    const NiPoint3& aLocation,
    float& afZDiff,
    float afMaxZdistAbove,
    float afMaxZdistBelow
) {
    NiPoint3 kTriVert1Loc;
    NiPoint3 kTriVert2Loc;
    NiPoint3 kTriVert3Loc;
    NavMeshTriangle* pTriangle = GetTriangle(asTriangleIndex);
    kTriVert1Loc = GetVertex(pTriangle->GetVert(0))->Location;
    kTriVert2Loc = GetVertex(pTriangle->GetVert(1))->Location;
    kTriVert3Loc = GetVertex(pTriangle->GetVert(2))->Location;
    return NavMeshMath::IsPointInsideTriangle(
        kTriVert1Loc,
        kTriVert2Loc,
        kTriVert3Loc,
        aLocation,
        afZDiff,
        afMaxZdistAbove,
        afMaxZdistBelow
    );
}

// Heights are not limited.
bool NavMesh::IsPointInsideTriangleFilter(
    uint16_t asTriangleIndex,
    const NiPoint3& aLocation,
    float& afZDiff,
    FindTriangleForLocationFilter* apFilter
) {
    if (!apFilter) {
        return IsPointInsideTriangle(
            asTriangleIndex,
            aLocation,
            afZDiff,
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
    }
    NiPoint3 kTriVert1Loc;
    NiPoint3 kTriVert2Loc;
    NiPoint3 kTriVert3Loc;
    NavMeshTriangle* pTriangle = GetTriangle(asTriangleIndex);
    kTriVert1Loc = GetVertex(pTriangle->GetVert(0))->Location;
    kTriVert2Loc = GetVertex(pTriangle->GetVert(1))->Location;
    kTriVert3Loc = GetVertex(pTriangle->GetVert(2))->Location;
    bool bInside = NavMeshMath::IsPointInsideTriangle(
        kTriVert1Loc,
        kTriVert2Loc,
        kTriVert3Loc,
        aLocation,
        afZDiff,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    if (bInside && apFilter) {
        NiPoint3 kTriLocation = aLocation;
        kTriLocation.z = kTriLocation.z + afZDiff;
        return apFilter->IsValidTri(aLocation, this, asTriangleIndex, kTriLocation);
    }
    return bInside;
}

NavMeshClosedDoorInfo* NavMesh::GetClosedDoorInfoForTriangle(uint16_t aiTriangle) {
    for (NavMeshClosedDoorInfo& rDoorInfo : ClosedDoors) {
        if (rDoorInfo.iTriangleIndex == aiTriangle) {
            return &rDoorInfo;
        }
    }
    return nullptr;
}

void NavMesh::GetOverlappedTriangleList(
    const NiPointer<ObstacleData>& apObstacle,
    const NiPoint3& afFudge,
    BSSimpleArray<uint16_t, 1024>* apRetTriangles
) {
    uint32_t istartRow;
    uint32_t iendRow;
    uint32_t istartCol;
    uint32_t iendCol;
    istartRow = 0;
    istartCol = 0;
    iendRow = 0;
    iendCol = 0;
    MeshGrid.GetGridRowAndColumn(apObstacle->aabbMin, &istartRow, &istartCol);
    MeshGrid.GetGridRowAndColumn(apObstacle->aabbMax, &iendRow, &iendCol);
    NiPoint3 cornersLocalSpace[4];
    cornersLocalSpace[0] = NiPoint3(
        apObstacle->BoxMin.x - afFudge.x,
        apObstacle->BoxMin.y - afFudge.y,
        apObstacle->BoxMin.z
    );
    cornersLocalSpace[1] = NiPoint3(
        apObstacle->BoxMin.x - afFudge.x,
        apObstacle->BoxMax.y + afFudge.y,
        apObstacle->BoxMin.z
    );
    cornersLocalSpace[2] = NiPoint3(
        apObstacle->BoxMax.x + afFudge.x,
        apObstacle->BoxMax.y + afFudge.y,
        apObstacle->BoxMin.z
    );
    cornersLocalSpace[3] = NiPoint3(
        apObstacle->BoxMax.x + afFudge.x,
        apObstacle->BoxMin.y - afFudge.y,
        apObstacle->BoxMin.z
    );
    float fzheight = apObstacle->aabbMax.z - apObstacle->aabbMin.z;
    for (uint32_t r = istartRow; r <= iendRow; ++r) {
        for (uint32_t c = istartCol; c <= iendCol; ++c) {
            BSSimpleArray<uint16_t, 1024>* parray = MeshGrid.GetTriangleArray(c, r);
            if (parray) {
                for (uint32_t i = 0; i < parray->iSize; ++i) {
                    if (apRetTriangles
                            ->Find(parray->GetAt(i), 0, &BSSimpleArray<uint16_t>::DefaultEqualFunctionByAddress) ==
                        -1) {
                        NavMeshTriangle* ptri = GetTriangle(parray->GetAt(i));
                        if (!ptri->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                            NiPoint3 wtriMin(
                                std::numeric_limits<float>::max(),
                                std::numeric_limits<float>::max(),
                                std::numeric_limits<float>::max()
                            );
                            NiPoint3 wtriMax(
                                -std::numeric_limits<float>::max(),
                                -std::numeric_limits<float>::max(),
                                -std::numeric_limits<float>::max()
                            );
                            for (int32_t v = 0; v < 3; ++v) {
                                NiPoint3* vertLocation = &GetVertex(ptri->GetVert(static_cast<uint32_t>(v)))->Location;
                                wtriMin.x = BSMIN(wtriMin.x, vertLocation->x);
                                wtriMin.y = BSMIN(wtriMin.y, vertLocation->y);
                                wtriMin.z = BSMIN(wtriMin.z, vertLocation->z);
                                wtriMax.x = BSMAX(wtriMax.x, vertLocation->x);
                                wtriMax.y = BSMAX(wtriMax.y, vertLocation->y);
                                wtriMax.z = BSMAX(wtriMax.z, vertLocation->z);
                            }
                            wtriMax.z = wtriMax.z + 128.0;
                            NiPoint3 kFudgedMax = apObstacle->aabbMax + afFudge;
                            if (Pathing::DoAABBsOverlap(apObstacle->aabbMin - afFudge, kFudgedMax, wtriMin, wtriMax)) {
                                bool bOverlap = false;
                                if (NavMeshObstacleManager::IsPointInBox(
                                        GetCenter(parray->GetAt(i)),
                                        apObstacle,
                                        fzheight,
                                        128.0f
                                    )) {
                                    bOverlap = true;
                                }
                                else {
                                    float fdontCare;
                                    if (IsPointInsideTriangle(
                                            parray->GetAt(i),
                                            apObstacle->Center,
                                            fdontCare,
                                            fzheight,
                                            128.0f
                                        )) {
                                        bOverlap = true;
                                    }
                                    else {
                                        for (uint32_t triv = 0; triv < 3; ++triv) {
                                            uint32_t triw = triv + 1;
                                            if (triv == 2) {
                                                triw = 0;
                                            }
                                            NiMatrix3* pOrientation = &apObstacle->Orientation;
                                            NiPoint3* pCenter = &apObstacle->Center;
                                            NiPoint3 vert1 = (GetVertex(ptri->GetVert(triv))->Location - *pCenter) *
                                                *pOrientation;
                                            pOrientation = &apObstacle->Orientation;
                                            pCenter = &apObstacle->Center;
                                            NiPoint3 vert2 = (GetVertex(ptri->GetVert(triw))->Location - *pCenter) *
                                                *pOrientation;
                                            for (uint32_t boxv = 0; boxv < 4; ++boxv) {
                                                uint32_t boxw = boxv + 1;
                                                if (boxv == 3) {
                                                    boxw = 0;
                                                }
                                                NiPoint3 box1 = cornersLocalSpace[boxv];
                                                NiPoint3 box2 = cornersLocalSpace[boxw];
                                                if (NavMeshMath::Find2DIntersection3(vert1, vert2, box1, box2)) {
                                                    bOverlap = true;
                                                    break;
                                                }
                                            }
                                            if (bOverlap) {
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (bOverlap) {
                                    apRetTriangles->Add(parray->GetAt(i));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// The box is shrunk by just under half its extent along one axis, chosen by comparing the extents' sizes.
bool NavMesh::AddClosedDoor(uint32_t aiDoorID, ObstacleData* apData) {
    NiPoint3 kExtent = apData->BoxMax - apData->BoxMin;
    NiPoint3 kFudge = NiPoint3::ZERO;
    float fAbsX = std::fabs(kExtent.x);
    if (std::fabs(kExtent.y) > fAbsX) {
        float fAbsX2 = std::fabs(kExtent.x);
        if (std::fabs(kExtent.z) > fAbsX2) {
            kFudge.x = -std::fabs(kExtent.x) * 0.49000000953674316;
        }
        else {
            kFudge.z = -std::fabs(kExtent.z) * 0.49000000953674316;
        }
    }
    else {
        float fAbsY = std::fabs(kExtent.y);
        if (std::fabs(kExtent.z) > fAbsY) {
            kFudge.y = -std::fabs(kExtent.y) * 0.49000000953674316;
        }
        else {
            kFudge.z = -std::fabs(kExtent.z) * 0.49000000953674316;
        }
    }
    BSSimpleArray<uint16_t> kTriangles;
    BSSimpleArray<uint16_t>* pTriangles = &kTriangles;
    {
        NiPointer<ObstacleData> spData(apData);
        GetOverlappedTriangleList(spData, kFudge, pTriangles);
    }
    for (uint16_t usTriangle : *pTriangles) {
        NavMeshClosedDoorInfo* pInfo = GetClosedDoorInfoForTriangle(usTriangle);
        if (pInfo && pInfo->DoorFormID == aiDoorID) {
            continue;
        }
        GetTriangle(usTriangle)->SetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
        NavMeshClosedDoorInfo kInfo;
        kInfo.DoorFormID = aiDoorID;
        kInfo.iTriangleIndex = usTriangle;
        ClosedDoors.Add(kInfo);
    }
    return true;
}

bool NavMesh::IsPointInsideNavmesh(const NiPoint3& aPoint, FindTriangleForLocationFilter* apFilter) {
    return FindTriangleForLocation(aPoint, apFilter) != 65535;
}

void NavMesh::ComputeBounds(NiPoint3* apRetMin, NiPoint3* apRetMax) {
    NavMeshVertex kVertex;
    if (!Triangles.IsEmpty()) {
        NiPoint3 kMax = GetVertex(GetTriangle(0)->GetVert(0))->Location;
        NiPoint3 kMin = kMax;
        for (uint32_t i = 0; i < GetTriangleCount(); ++i) {
            NavMeshTriangle* pTriangle = GetTriangle(i);
            for (int32_t j = 0; j < 3; ++j) {
                kVertex = *GetVertex(pTriangle->GetVert(j));
                if (kMin.x > static_cast<double>(kVertex.Location.x)) {
                    kMin.x = kVertex.Location.x;
                }
                else if (kMax.x < static_cast<double>(kVertex.Location.x)) {
                    kMax.x = kVertex.Location.x;
                }
                if (kMin.y > static_cast<double>(kVertex.Location.y)) {
                    kMin.y = kVertex.Location.y;
                }
                else if (kMax.y < static_cast<double>(kVertex.Location.y)) {
                    kMax.y = kVertex.Location.y;
                }
                if (kMin.z > static_cast<double>(kVertex.Location.z)) {
                    kMin.z = kVertex.Location.z;
                }
                else if (kMax.z < static_cast<double>(kVertex.Location.z)) {
                    kMax.z = kVertex.Location.z;
                }
            }
        }
        *apRetMin = kMin;
        *apRetMax = kMax;
    }
}

EdgeExtraInfo* NavMesh::SetEmptyEdgeExtraInfo(uint16_t aTriangle, int32_t aEdgeIndex) {
    EdgeExtraInfo* pEdgeInfo = GetEdgeExtraInfo(aTriangle, aEdgeIndex);
    if (!pEdgeInfo) {
        EdgeExtraInfo kInfo;
        uint16_t usInfoIndex = AddExtraEdgeInfo(kInfo);
        Triangles.GetAt(aTriangle).SetEdgeExtraInfo(aEdgeIndex, usInfoIndex);
        pEdgeInfo = GetEdgeExtraInfo(aTriangle, aEdgeIndex);
    }
    pEdgeInfo->Portal.pOtherMesh = nullptr;
    pEdgeInfo->Portal.sTriangle = static_cast<uint16_t>(-1);
    return pEdgeInfo;
}

bool NavMesh::IsPointInsideTriangle_0(
    uint16_t asTriangleIndex,
    const NiPoint3* apLocation,
    float afMaxZdistAbove,
    float afMaxZdistBelow,
    float* apZCoord
) {
    NavMeshTriangle* pTriangle = GetTriangle(asTriangleIndex);
    NavMeshVertex* pVert1 = GetVertex(pTriangle->GetVert(0));
    NavMeshVertex* pVert2 = GetVertex(pTriangle->GetVert(1u));
    NavMeshVertex* pVert3 = GetVertex(pTriangle->GetVert(2u));
    float fDenominator = (pVert2->Location.x - pVert1->Location.x) * (pVert3->Location.y - pVert1->Location.y) -
        (pVert3->Location.x - pVert1->Location.x) * (pVert2->Location.y - pVert1->Location.y);
    if (fDenominator == 0.0) {
        return false;
    }
    float fX2 = pVert2->Location.x - apLocation->x;
    float fX3 = pVert3->Location.x - apLocation->x;
    float fY2 = pVert2->Location.y - apLocation->y;
    float fY3 = pVert3->Location.y - apLocation->y;
    float fScale = 1.0 / fDenominator;
    float fWeight1 = (fX2 * fY3 - fX3 * fY2) * fScale;
    if (fWeight1 < 0.0) {
        return false;
    }
    float fX1 = pVert1->Location.x - apLocation->x;
    float fY1 = pVert1->Location.y - apLocation->y;
    float fWeight2 = (fX3 * fY1 - fX1 * fY3) * fScale;
    if (fWeight2 < 0.0) {
        return false;
    }
    float fWeight3 = (fX1 * fY2 - fX2 * fY1) * fScale;
    if (fWeight3 < 0.0) {
        return false;
    }
    *apZCoord = fWeight1 * pVert1->Location.z + fWeight2 * pVert2->Location.z + fWeight3 * pVert3->Location.z;
    float fZDiff = apLocation->z - *apZCoord;
    return !(afMaxZdistAbove < static_cast<double>(fZDiff)) && !(fZDiff < -afMaxZdistBelow);
}

uint32_t NavMesh::AddObstacleUndoData(ObstacleUndoData* apUndoData) {
    NiPointer<ObstacleUndoData> spUndoData(apUndoData);
    return Obstacles.Add(spUndoData);
}

void NavMesh::FlipTriangle(uint16_t aiTriIndex) {
    NavMeshTriangle* ptri = GetTriangle(aiTriIndex);
    uint16_t vert1index = ptri->GetVert(1u);
    uint16_t vert0index = ptri->GetVert(0);
    ptri->SetVert(1u, vert0index);
    ptri->SetVert(0, vert1index);
    bool bEdge0IsPortal = ptri->EdgeHasExtraInfo(0);
    bool bEdge1IsPortal = ptri->EdgeHasExtraInfo(1);
    uint16_t tri1index = ptri->GetTriOrExtraInfo(1);
    uint16_t tri0index = ptri->GetTriOrExtraInfo(0);
    if (bEdge0IsPortal) {
        ptri->SetEdgeExtraInfo(1, tri0index);
    }
    else {
        ptri->SetTri(1u, tri0index);
    }
    if (bEdge1IsPortal) {
        ptri->SetEdgeExtraInfo(0, tri1index);
    }
    else {
        ptri->SetTri(0, tri1index);
    }
    if (ptri->HasCoverOrWallFlags()) {
        bool b0right;
        bool b1right;
        int16_t ausCoverValue[3];
        bool b0left;
        bool b1left;
        uint16_t icover0;
        ptri->GetEdgeCoverData(0, icover0, b0left, b0right);
        ptri->GetEdgeCoverData(1, *reinterpret_cast<uint16_t*>(ausCoverValue), b1left, b1right);
        ptri->SetEdgeCover(0, ausCoverValue[0], b1right, b1left);
        ptri->SetEdgeCover(1, icover0, b0right, b0left);
    }
}

void NavMesh::GetTrianglesForAABB(
    const NiPoint3& aAABBMin,
    const NiPoint3& aAABBMax,
    BSSimpleArray<uint16_t, 1024>* apRetTriangles
) {
    MeshGrid.GetTriangleDataForAABB(aAABBMin, aAABBMax, apRetTriangles);
}

bool NavMesh::FindClosestOpenEdgeForLocation(
    const NiPoint3& aLocation,
    float afRadius,
    uint16_t* apRetTriIndex,
    int32_t* apRetEdgeIndex,
    bool abFlipForStartNode
) {
    uint16_t usClosestTri = 0;
    int32_t iClosestEdge = -1;
    float fClosestDistSqr = std::numeric_limits<float>::max();
    float fRadiusSqr = afRadius * afRadius;
    float fMinZ = aLocation.z - 64.0;
    float fMaxZ = aLocation.z + 180.0;
    if (abFlipForStartNode) {
        fMinZ = aLocation.z - 180.0;
        fMaxZ = aLocation.z + 64.0;
    }
    uint32_t uiTriCount = Triangles.iSize;
    for (uint32_t i = 0; i < uiTriCount; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t e = 0; e < 3; ++e) {
            if (!ptri->IsEdgeConnected(e)) {
                NiPoint3 kDelta;
                NavMeshEdgeVertices verts;
                NiPoint3 kMidpoint;
                verts = GetEdgeVertices(i, e);
                kMidpoint = (verts.Vertices[0]->Location + verts.Vertices[1]->Location) * 0.5f;
                kDelta = kMidpoint - aLocation;
                float fXYDistSqr = kDelta.SqrXYLength();
                float fZDistSqr = kDelta.z * kDelta.z;
                float fDistSqr = fXYDistSqr + fZDistSqr;
                if (fClosestDistSqr > static_cast<double>(fXYDistSqr) &&
                    ((fMaxZ > static_cast<double>(kMidpoint.z) && fMinZ < static_cast<double>(kMidpoint.z)) ||
                     fDistSqr <= fRadiusSqr - fZDistSqr)) {
                    usClosestTri = i;
                    iClosestEdge = e;
                    fClosestDistSqr = fDistSqr;
                }
            }
        }
    }
    if (iClosestEdge == -1) {
        return false;
    }
    *apRetTriIndex = usClosestTri;
    *apRetEdgeIndex = iClosestEdge;
    return true;
}

void NavMesh::GetCircumCenter2D(uint16_t apTriangle, NiPoint3& aOut) {
    NavMeshVertex kVert1 = *GetVertex(GetTriangle(apTriangle)->GetVert(0));
    NavMeshVertex kVert2 = *GetVertex(GetTriangle(apTriangle)->GetVert(1u));
    NavMeshVertex* pVert3 = GetVertex(GetTriangle(apTriangle)->GetVert(2u));
    float fX2 = kVert2.Location.x - kVert1.Location.x;
    float fY2 = kVert2.Location.y - kVert1.Location.y;
    float fX3 = pVert3->Location.x - kVert1.Location.x;
    float fY3 = pVert3->Location.y - kVert1.Location.y;
    float fLength2 = fX2 * fX2 + fY2 * fY2;
    float fLength3 = fX3 * fX3 + fY3 * fY3;
    float fDenominator = fX2 * fY3 - fX3 * fY2;
    float fScale = 0.5 / fDenominator;
    aOut.x = kVert1.Location.x - (fY2 * fLength3 - fY3 * fLength2) * fScale;
    aOut.y = (fX2 * fLength3 - fX3 * fLength2) * fScale + kVert1.Location.y;
    aOut.z = 0.0;
}

bool NavMesh::FindPortalTriangles(const NavMeshInfo* apNavMeshInfo, BSSimpleArray<int32_t, 1024>* apReturnArray) {
    apReturnArray->SetSize(0, true);
    apReturnArray->SetReservedSize(100u);
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* pTri = GetTriangle(i);
        if (pTri->HasEdgeExtraInfo()) {
            for (int32_t j = 0; j < 3; ++j) {
                if (pTri->EdgeHasExtraInfo(j)) {
                    if (GetExtraEdgeInfo(pTri->GetTriOrExtraInfo(j))->Portal.pOtherMesh == apNavMeshInfo) {
                        int32_t iTriangle = i;
                        apReturnArray->Add(iTriangle);
                        break;
                    }
                }
            }
        }
    }
    return apReturnArray->iSize != 0;
}

bool NavMesh::ComputeNavMeshZForLocation(
    const NiPoint3& aLocation,
    float afMaxZdistAbove,
    float afMaxZdistBelow,
    float& afZCoord
) {
    const BSSimpleArray<uint16_t, 1024>* ptriArray = nullptr;
    MeshGrid.GetTriangleData(aLocation, ptriArray);
    if (!ptriArray) {
        return false;
    }
    uint32_t inum = ptriArray->iSize;
    for (uint32_t i = 0; i < inum; ++i) {
        if (IsPointInsideTriangle_0(ptriArray->GetAt(i), &aLocation, afMaxZdistAbove, afMaxZdistBelow, &afZCoord)) {
            return true;
        }
    }
    return false;
}

bool NavMesh::FindRandomPointWithinRadius(const NiPoint3& aStartLocation, float aRadius, NiPoint3* apRet1) {
    bool bInsideNavmesh = false;
    NiPoint3 kLocation = NiPoint3::ZERO;
    for (uint32_t i = 0; i < 8 && !bInsideNavmesh; ++i) {
        float fX = RandomFloat(-aRadius, aRadius);
        float fY = std::sqrt(aRadius * aRadius - fX * fX);
        fY = RandomFloat(-fY, fY);
        kLocation.x = fX;
        kLocation.y = fY;
        bInsideNavmesh = IsPointInsideNavmesh(kLocation + aStartLocation, nullptr);
    }
    if (!bInsideNavmesh) {
        return false;
    }
    kLocation += aStartLocation;
    *apRet1 = MoveAwayFromEdge(kLocation, 64.0f);
    return true;
}

NiPoint3 NavMesh::MoveAwayFromEdge(const NiPoint3& aLocation, float afDistance) {
    NiPoint3 retLoc = aLocation;
    uint16_t usTri = FindTriangleForLocation(aLocation, nullptr);
    if (usTri != 0xFFFF) {
        uint16_t ausAdjacentTris[4];
        uint16_t isize = 0;
        NavMeshTriangle* ptempTri = GetTriangle(usTri);
        for (int32_t t = 0; t < 3; ++t) {
            uint16_t ival = -1;
            if (!ptempTri->EdgeHasExtraInfo(t)) {
                ival = ptempTri->GetTriOrExtraInfo(t);
            }
            if (ival == 0xFFFF) {
                NavMeshEdgeVertices vertices;
                vertices = GetEdgeVertices(usTri, t);
                float fdistanceFromEdge = NavMeshMath::DistanceFromLine(
                    aLocation,
                    vertices.Vertices[0]->Location,
                    vertices.Vertices[1]->Location
                );
                if (afDistance > static_cast<double>(fdistanceFromEdge)) {
                    NiPoint3 rightAngleVector;
                    NiPoint3 triangleNormal = GetNormal(usTri);
                    rightAngleVector = triangleNormal.Cross(
                        vertices.Vertices[0]->Location - vertices.Vertices[1]->Location
                    );
                    rightAngleVector.Unitize();
                    float fScalar = afDistance - fdistanceFromEdge;
                    retLoc += rightAngleVector * fScalar;
                }
            }
            else {
                ausAdjacentTris[isize++] = ival;
            }
        }
        for (uint32_t i = 0; i < isize; ++i) {
            NavMeshTriangle* ptri = GetTriangle(ausAdjacentTris[i]);
            for (int32_t e = 0; e < 3; ++e) {
                if (!ptri->EdgeHasExtraInfo(e) && ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                    NavMeshEdgeVertices vertices;
                    vertices = GetEdgeVertices(ausAdjacentTris[i], e);
                    float fdistanceFromEdge = NavMeshMath::DistanceFromLine(
                        aLocation,
                        vertices.Vertices[0]->Location,
                        vertices.Vertices[1]->Location
                    );
                    if (afDistance > static_cast<double>(fdistanceFromEdge)) {
                        NiPoint3 rightAngleVector;
                        NiPoint3 triangleNormal = GetNormal(ausAdjacentTris[i]);
                        rightAngleVector = triangleNormal.Cross(
                            vertices.Vertices[0]->Location - vertices.Vertices[1]->Location
                        );
                        rightAngleVector.Unitize();
                        float fScalar = afDistance - fdistanceFromEdge;
                        retLoc += rightAngleVector * fScalar;
                    }
                }
            }
        }
    }
    return retLoc;
}

int32_t NavMesh::FindInterectingEdge(
    uint16_t ausTri,
    int32_t aIgnoreEdge,
    const NiPoint2& aStartPoint,
    const NiPoint2& aEndPoint,
    NiPoint2* apReturnIntersection
) {
    NiPoint2 edgeVerts2D[3];
    for (int32_t i = 0; i < 3; ++i) {
        NiPoint3 vert = GetVertex(GetTriangle(ausTri)->GetVert(static_cast<uint32_t>(i)))->Location;
        edgeVerts2D[i].x = vert.x;
        edgeVerts2D[i].y = vert.y;
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (i != aIgnoreEdge) {
            int32_t j = i + 1;
            if (i == 2) {
                j = 0;
            }
            if (NavMeshMath::Find2DIntersection(
                    edgeVerts2D[i],
                    edgeVerts2D[j],
                    aStartPoint,
                    aEndPoint,
                    apReturnIntersection
                )) {
                return i;
            }
        }
    }
    return -1;
}

NavMesh::FIEResult* NavMesh::FindInterectingEdge2(
    NavMesh::FIEResult* apResult,
    uint16_t ausTri,
    int32_t aIgnoreEdge,
    const NiPoint3& aStartPoint,
    NiPoint3& aEndPoint
) {
    NiPoint3 edgeVerts[3];
    for (int32_t i = 0; i < 3; ++i) {
        edgeVerts[i] = GetVertex(GetTriangle(ausTri)->GetVert(i))->Location;
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (i != aIgnoreEdge) {
            int32_t j = i + 1;
            if (i == 2) {
                j = 0;
            }
            if (!(edgeVerts[i] == edgeVerts[j])) {
                NavMesh::FIEResult kResult;
                if (NavMeshMath::Find2DIntersectionStable(
                        edgeVerts[i],
                        edgeVerts[j],
                        aStartPoint,
                        aEndPoint,
                        &kResult.Intersection
                    )) {
                    kResult.eType = NavMesh::FIE_INTERSECTION;
                    kResult.iEdge = i;
                    apResult->eType = NavMesh::FIE_INTERSECTION;
                    apResult->iEdge = kResult.iEdge;
                    apResult->Intersection = kResult.Intersection;
                    return apResult;
                }
            }
        }
    }
    NavMesh::FIEResult kNoResult;
    kNoResult.iEdge = -1;
    apResult->eType = NavMesh::FIE_NONE;
    apResult->iEdge = kNoResult.iEdge;
    apResult->Intersection = kNoResult.Intersection;
    return apResult;
}

bool NavMesh::AddObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps) {
    NavMeshObstacleCutter cutter;
    bool bret = cutter.SplitNavMeshFromObstacle(this, apObstacle, aPortalSwaps);
    return bret;
}

void NavMesh::ClearAllObstacles() {
    TaskPortalSwap kPortalSwaps;
    for (uint32_t uiUndoIndex = Obstacles.iSize - 1; uiUndoIndex != -1; --uiUndoIndex) {
        RemoveObstacle(uiUndoIndex, kPortalSwaps);
    }
}

uint32_t NavMesh::GetUndoDataMaxObstacleCount(ObstacleUndoData* apUndoData) {
    uint32_t iA = 0;
    for (uint16_t usTriangle : apUndoData->OverlappedTriangles) {
        iA = BSMAX(iA, GetTriangle(usTriangle)->GetObjectCount());
    }
    for (const BoundsTriangleInfo& rBoundsInfo : apUndoData->BoundsTriangles) {
        iA = BSMAX(iA, GetTriangle(rBoundsInfo.usTriangleIndex)->GetObjectCount());
    }
    return iA;
}

bool NavMesh::RemoveClosedDoor(uint32_t aiDoorID) {
    for (uint32_t i = 0; i < ClosedDoors.iSize; ++i) {
        if (ClosedDoors.GetAt(i).DoorFormID == aiDoorID) {
            uint32_t iTriangleIndex = ClosedDoors.GetAt(i).iTriangleIndex;
            if (iTriangleIndex < GetTriangleCount()) {
                GetTriangle(ClosedDoors.GetAt(i).iTriangleIndex)->ClearFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
            }
            ClosedDoors.RemoveFast(i--, 1u);
        }
    }
    return true;
}

uint32_t NavMesh::GetPOVsForTriangle(uint16_t ausTriangle, BSSimpleArray<uint16_t, 1024>& aRetPOVs) {
    if (POVs.IsEmpty()) {
        CreatePOVs();
    }
    aRetPOVs.Clear(false);
    uint32_t itotal = 0;
    NavMeshTriangle* ptri = GetTriangle(ausTriangle);
    for (int32_t i = 0; i < 3; ++i) {
        NavMeshPOVData data;
        uint16_t ivertIndex = ptri->GetVert(static_cast<uint32_t>(i));
        if (POVs.GetAt(ivertIndex, data)) {
            aRetPOVs.Add(ivertIndex);
            ++itotal;
        }
    }
    if (ausTriangle < ClosestPOVs.iSize && ClosestPOVs.GetAt(static_cast<int32_t>(ausTriangle)) != 0xFFFF) {
        aRetPOVs.Add(ClosestPOVs.GetAt(static_cast<int32_t>(ausTriangle)));
        ++itotal;
    }
    return itotal;
}

uint32_t NavMesh::GetObstaclePOVsForTriangle(uint16_t ausTriangle, BSSimpleArray<uint16_t, 1024>& aRetObstaclePOVs) {
    aRetObstaclePOVs.Clear(false);
    uint32_t itotal = 0;
    NavMeshTriangle* ptri = GetTriangle(ausTriangle);
    for (int32_t i = 0; i < 3; ++i) {
        uint16_t ivertIndex = ptri->GetVert(static_cast<uint32_t>(i));
        if (ObstaclePOVs.Find(ivertIndex, 0, &BSSimpleArray<uint16_t>::DefaultEqualFunctionByAddress) != -1) {
            aRetObstaclePOVs.Add(ivertIndex);
            ++itotal;
        }
    }
    return itotal;
}

bool NavMesh::GetPOVForVertex(uint16_t ausVertex, NavMeshPOVData& aRetData) {
    if (POVs.IsEmpty()) {
        CreatePOVs();
    }
    return POVs.GetAt(ausVertex, aRetData);
}

void NavMesh::FixPortalsFromNavMeshDeletion(NavMeshInfo* apNavmeshInfo) {
    if (apNavmeshInfo) {
        using connectedInfos_t = BSSimpleArray<NavMeshInfo*, 1024>;
        connectedInfos_t connectedInfos;
        TES::pInstance->GetNavMeshInfoMap()->GetAllNavMeshInfo(&connectedInfos);
        for (uint32_t i = 0; i < connectedInfos.iSize; ++i) {
            NavMeshInfo* pinfo = connectedInfos.GetAt(static_cast<int32_t>(i));
            int32_t isize = pinfo->AdjacentNavMeshes.iSize;
            int32_t iprefferedsize = pinfo->PreferredAdjacentNavMeshes.iSize;
            for (int32_t p = 0; p < iprefferedsize + isize; ++p) {
                NavMeshInfo* pportal = p >= isize ? pinfo->PreferredAdjacentNavMeshes.GetAt(p - isize)
                                                  : pinfo->AdjacentNavMeshes.GetAt(p);
                if (pportal == apNavmeshInfo) {
                    NavMeshPtr spnavmesh;
                    if (!pinfo->GetNavMeshEvenDisabled(spnavmesh)) {
                        TESForm* pnavmeshForm = TESForm::GetFormByNumericID(pinfo->NavMeshID);
                        NavMesh* pnavmesh = dynamic_cast<NavMesh*>(pnavmeshForm);
                        spnavmesh = NavMeshPtr::MakeNavMeshPtr(pnavmesh);
                    }
                    if (spnavmesh) {
                        spnavmesh->SetAltered(true);
                        for (uint32_t e = 0; e < spnavmesh->ExtraEdgeInfo.GetSize(); ++e) {
                            EdgeExtraInfo* pEdgeInfo = spnavmesh->GetExtraEdgeInfo(e);
                            if (pEdgeInfo->Portal.pOtherMesh == apNavmeshInfo) {
                                spnavmesh->DeleteExtraEdgeInfo(e--);
                            }
                        }
                    }
                    if (p >= isize) {
                        pinfo->PreferredAdjacentNavMeshes.Remove(p - isize, true);
                        --iprefferedsize;
                    }
                    else {
                        pinfo->AdjacentNavMeshes.Remove(p, true);
                        --isize;
                    }
                    --p;
                }
            }
        }
    }
}

void NavMeshGrid::GetMeshBounds(NiPoint3& aMin, NiPoint3& aMax) {
    aMin = GridBoundsMin;
    aMax = GridBoundsMax;
}

void NavMeshGrid::SetTriangleData(uint16_t aiTriangle, uint32_t aiColumn, uint32_t aiRow) {
    GridData[aiColumn + iGridSize * aiRow].Add(aiTriangle);
}

TaskPortalSwap::~TaskPortalSwap() {}
