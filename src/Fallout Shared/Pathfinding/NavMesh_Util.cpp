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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

int32_t CompareUShort(const uint16_t* a1, const uint16_t* a2) {
    if (((int32_t)(*a1)) > ((int32_t)(*a2))) {
        return 1;
    }
    if (((int32_t)(*a2)) <= ((int32_t)(*a1))) {
        return 0;
    }
    return (-1);
}

POVVertexData::POVVertexData() {}

POVVertexData::~POVVertexData() {}

void POVVertexData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

AngleSinCos AngleSinCos::operator-(const AngleSinCos& aSinCos) const {
    AngleSinCos result;
    AngleSinCos ecx0a;
    ecx0a.fCos = fCos * aSinCos.fCos + fSin * aSinCos.fSin;
    ecx0a.fSin = fSin * aSinCos.fCos - fCos * aSinCos.fSin;
    result.operator=(ecx0a);
    return result;
}

AngleSinCos& AngleSinCos::operator=(const AngleSinCos& aSinCos) {
    fCos = aSinCos.fCos;
    fSin = aSinCos.fSin;
    return *this;
}

FastNavMeshEdgeHandle* FastNavMeshEdgeHandle::Copy(const FastNavMeshEdgeHandle& arOther) {
    pNavMesh = (&arOther)->pNavMesh;
    usTriangle = (&arOther)->usTriangle;
    iEdgeIndex = (&arOther)->iEdgeIndex;
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
    uint16_t Vert = ptri->GetVert(0);
    NavMeshVertex* Vertex = GetVertex(Vert);
    uint16_t v4 = ptri->GetVert(1u);
    NavMeshVertex* v5 = GetVertex(v4);
    NiPoint3 edge1 = v5->Location - Vertex->Location;
    uint16_t v6 = ptri->GetVert(0);
    NavMeshVertex* v11 = GetVertex(v6);
    uint16_t v7 = ptri->GetVert(2u);
    NavMeshVertex* v8 = GetVertex(v7);
    NiPoint3 edge2 = v8->Location - v11->Location;
    return edge1.UnitCross(edge2);
}

bool NavMesh::ReconnectEnabledPortals() {
    uint32_t i_2 = 0;
    for (uint32_t i = GetTriangleCount(); i_2 < i; i = GetTriangleCount()) {
        NavMeshTriangle* ptri = GetTriangle(i_2);
        if (ptri && ptri->HasEdgeExtraInfo()) {
            for (int32_t e = 0; e < 3; ++e) {
                if (ptri->EdgeHasExtraInfo(e)) {
                    uint16_t TriOrExtraInfo = ptri->GetTriOrExtraInfo(e);
                    EdgeExtraInfo* pinfo = GetExtraEdgeInfo(TriOrExtraInfo);
                    NavMeshPtr spotherMesh;
                    if (pinfo->Portal.pOtherMesh->GetNavMesh(spotherMesh)) {
                        NavMeshTriangle* potherTri = spotherMesh.m_pObject->GetTriangle(pinfo->Portal.sTriangle);
                        if (potherTri) {
                            int32_t portalCount = 0;
                            int32_t portalEdge = -1;
                            for (int32_t f = 0; f < 3; ++f) {
                                if (potherTri->EdgeHasExtraInfo(f)) {
                                    ++portalCount;
                                    portalEdge = f;
                                }
                            }
                            if (portalCount) {
                                // This edge's two ends; the other mesh's edge runs the other way, so its first end
                                // meets this edge's second.
                                NiPoint3 kVert2;
                                NiPoint3 kVert1;
                                GetEdgeVertices(i_2, e, &kVert1, &kVert2);
                                for (int32_t aiEdgeIndex = 0; aiEdgeIndex < 3; ++aiEdgeIndex) {
                                    NiPoint3 kOtherVert1;
                                    NiPoint3 kOtherVert2;
                                    spotherMesh.m_pObject->GetEdgeVertices(
                                        pinfo->Portal.sTriangle, aiEdgeIndex, &kOtherVert1, &kOtherVert2
                                    );
                                    if (((kVert1 - kOtherVert2).SqrLength() < 400.0) &&
                                        ((kVert2 - kOtherVert1).SqrLength() < 400.0)) {
                                        portalEdge = aiEdgeIndex;
                                        portalCount = 1;
                                        break;
                                    }
                                }
                            }
                            if (portalCount == 1) {
                                EdgeExtraInfo* potherExtraInfo =
                                    spotherMesh.m_pObject->SetEmptyEdgeExtraInfo(pinfo->Portal.sTriangle, portalEdge);
                                NavMeshInfo* NavMeshInfo_ = GetNavMeshInfo();
                                potherExtraInfo->Portal.pOtherMesh = NavMeshInfo_;
                                potherExtraInfo->Portal.sTriangle = i_2;
                                potherExtraInfo->eType = EIT_ENABLE_DISABLE_PORTAL;
                            }
                        }
                    }
                }
            }
        }
        ++i_2;
    }
    return true;
}

uint32_t NavMesh::GetUndoDataIndexForObstacle(const NiPointer<ObstacleData>& apObstacle) {
    for (uint32_t i = 0; i < Obstacles.iSize; ++i) {
        if ((&Obstacles.GetAt(static_cast<int32_t>(i)))->m_pObject->spObstacle.operator==(apObstacle)) {
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

    *apOutUndoData = (&Obstacles.GetAt(static_cast<int32_t>(i)))->m_pObject;
    return i;
}

bool NavMesh::RemoveObstacle(uint32_t aiUndoIndex, TaskPortalSwap& aPortalSwaps) {
    XFNV_INLINED_LOCAL(NavMeshObstacleCutter, cutter_3, std::nullopt, std::nullopt);
    ObstacleUndoData* pundoData = (&Obstacles.GetAt(static_cast<int32_t>(aiUndoIndex)))->m_pObject;
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
            using obstaclesToReAdd_t = NiPointer<ObstacleData>[100];
            XFNV_INLINED_LOCAL(obstaclesToReAdd_t, obstaclesToReAdd, std::nullopt, std::nullopt);
            rt::eh_vector_constructor_iterator(
                obstaclesToReAdd, sizeof(NiPointer<ObstacleData>), 100, &xfnv::constructElement<NiPointer<NiRefObject>>,
                &xfnv::destructElement<NiPointer<NiRefObject>>
            );
            uint32_t iobstacleToReAddCount = 0;
            ::new (&cutter_3) NavMeshObstacleCutter;
            for (iobstacleIndex = Obstacles.iSize - 1; iobstacleIndex != aiUndoIndex - 1; --iobstacleIndex) {
                if ((&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject->eState ==
                    ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE) {
                    cutter_3.UndoNavMeshFromObstacleUndoData(
                        this, (&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject, aPortalSwaps
                    );
                    if (iobstacleIndex > aiUndoIndex) {
                        NiPointer<ObstacleData>* p_spObstacle =
                            &(&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject->spObstacle;
                        obstaclesToReAdd[iobstacleToReAddCount++].operator=(*p_spObstacle);
                    }
                }
                cutter_3.ResetNavMeshFromObstacleUndoData(
                    this, (&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject
                );
                RemoveObstacleUndoData(iobstacleIndex);
            }
            while (iobstacleIndex != -1 &&
                   (&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject->eState ==
                       ObstacleUndoData::OBSTACLE_UNDO_STATE_UNDONE) {
                cutter_3.ResetNavMeshFromObstacleUndoData(
                    this, (&Obstacles.GetAt(static_cast<int32_t>(iobstacleIndex)))->m_pObject
                );
                RemoveObstacleUndoData(iobstacleIndex--);
            }
            for (uint32_t i = 0; i < iobstacleToReAddCount; ++i) {
                cutter_3.SplitNavMeshFromObstacle(this, obstaclesToReAdd[i], aPortalSwaps);
            }
            cutter_3.~NavMeshObstacleCutter();
            rt::eh_vector_destructor_iterator(
                obstaclesToReAdd, sizeof(NiPointer<ObstacleData>), 100, &xfnv::destructElement<NiPointer<NiRefObject>>
            );
        }
        else {
            NavMeshObstacleCutter cutter_2;
            if (pundoData->eState == ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE) {
                cutter_2.UndoNavMeshFromObstacleUndoData(this, pundoData, aPortalSwaps);
            }
        }
    }
    return true;
}

bool NavMesh::RemoveObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps) {
    ObstacleUndoData* apOutUndoData;
    uint32_t undoDataIndex = GetUndoDataIndexForObstacle(apObstacle, &apOutUndoData);
    return undoDataIndex != -1 && RemoveObstacle(undoDataIndex, aPortalSwaps);
}

// Builds the points of visibility: each vertex where exactly two boundary or portal edges meet and the turn between
// them qualifies gets a POV, then each triangle records its closest POV within reach.
void NavMesh::CreatePOVs() {
    POVs.RemoveAll();
    ClosestPOVs.Clear(true);
    using VertexMap_t = NiTMap<FastNavMeshVertHandle, POVVertexData*>;
    VertexMap_t VertexMap(37u);
    FastNavMeshVertHandle key;
    uint32_t itriCount = GetTriangleCount();
    for (uint32_t i = 0; i < itriCount; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t e = 0; e < 3; ++e) {
            if (ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                FastNavMeshEdgeHandle hedge;
                hedge.~FastNavMeshEdgeHandle();
                ::new (&hedge) FastNavMeshEdgeHandle(this, i, e);
                FastNavMeshVertHandle hvert1(this, ptri->GetVert(static_cast<uint32_t>(e)));
                POVVertexData* pdata1;
                key.~FastNavMeshVertHandle();
                ::new (&key) FastNavMeshVertHandle(hvert1);
                if (!VertexMap.GetAt(key, pdata1)) {
                    POVVertexData* pnew = static_cast<POVVertexData*>(ourMalloc(sizeof(POVVertexData)));
                    if (pnew) {
                        pdata1 = ::new (pnew) POVVertexData;
                    }
                    else {
                        pdata1 = nullptr;
                    }
                    pdata1->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata1->hVertex) FastNavMeshVertHandle(hvert1);
                    key.~FastNavMeshVertHandle();
                    ::new (&key) FastNavMeshVertHandle(hvert1);
                    VertexMap.SetAt(key, pdata1);
                }
                pdata1->Edges.Add(hedge);
                FastNavMeshVertHandle hvert2(this, ptri->GetVert((e + 1) % 3));
                POVVertexData* pdata2;
                key.~FastNavMeshVertHandle();
                ::new (&key) FastNavMeshVertHandle(hvert2);
                if (!VertexMap.GetAt(key, pdata2)) {
                    POVVertexData* pnew = static_cast<POVVertexData*>(ourMalloc(sizeof(POVVertexData)));
                    if (pnew) {
                        pdata2 = ::new (pnew) POVVertexData;
                    }
                    else {
                        pdata2 = nullptr;
                    }
                    pdata2->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata2->hVertex) FastNavMeshVertHandle(hvert2);
                    key.~FastNavMeshVertHandle();
                    ::new (&key) FastNavMeshVertHandle(hvert2);
                    VertexMap.SetAt(key, pdata2);
                }
                pdata2->Edges.Add(hedge);
            }
            else if (ptri->EdgeHasExtraInfo(e)) {
                FastNavMeshEdgeHandle hedge;
                hedge.~FastNavMeshEdgeHandle();
                ::new (&hedge) FastNavMeshEdgeHandle(this, i, e);
                FastNavMeshVertHandle hvert1(this, ptri->GetVert(static_cast<uint32_t>(e)));
                POVVertexData* pdata1;
                key.~FastNavMeshVertHandle();
                ::new (&key) FastNavMeshVertHandle(hvert1);
                if (!VertexMap.GetAt(key, pdata1)) {
                    POVVertexData* pnew = static_cast<POVVertexData*>(ourMalloc(sizeof(POVVertexData)));
                    if (pnew) {
                        pdata1 = ::new (pnew) POVVertexData;
                    }
                    else {
                        pdata1 = nullptr;
                    }
                    pdata1->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata1->hVertex) FastNavMeshVertHandle(hvert1);
                    key.~FastNavMeshVertHandle();
                    ::new (&key) FastNavMeshVertHandle(hvert1);
                    VertexMap.SetAt(key, pdata1);
                }
                pdata1->PortalEdges.Add(hedge);
                FastNavMeshVertHandle hvert2(this, ptri->GetVert((e + 1) % 3));
                POVVertexData* pdata2;
                key.~FastNavMeshVertHandle();
                ::new (&key) FastNavMeshVertHandle(hvert2);
                if (!VertexMap.GetAt(key, pdata2)) {
                    POVVertexData* pnew = static_cast<POVVertexData*>(ourMalloc(sizeof(POVVertexData)));
                    if (pnew) {
                        pdata2 = ::new (pnew) POVVertexData;
                    }
                    else {
                        pdata2 = nullptr;
                    }
                    pdata2->hVertex.~FastNavMeshVertHandle();
                    ::new (&pdata2->hVertex) FastNavMeshVertHandle(hvert2);
                    key.~FastNavMeshVertHandle();
                    ::new (&key) FastNavMeshVertHandle(hvert2);
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
            if ((fclosestPOVSqrDist > fsqrDist) && (std::fabs(delta.z) < 180.0)) {
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
// the second vertex's square test the cell corner's y is compared with the third vertex's y, deliberately, as the game
// does.
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
    rt::eh_vector_constructor_iterator(akVerts, sizeof(NiPoint3), 3, &emptyPointConstructor);
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
                if ((kCorner11.x >= akVerts[0].x && kCorner11.y >= akVerts[0].y && kCorner00.x <= akVerts[0].x &&
                     kCorner00.y <= akVerts[0].y) ||
                    (kCorner11.x >= akVerts[1].x && kCorner11.y >= akVerts[1].y && kCorner00.x <= akVerts[1].x &&
                     kCorner00.y <= akVerts[2].y) ||
                    (kCorner11.x >= akVerts[2].x && kCorner11.y >= akVerts[2].y && kCorner00.x <= akVerts[2].x &&
                     kCorner00.y <= akVerts[2].y)) {
                    MeshGrid.SetTriangleData(static_cast<uint16_t>(i), uiColumn, uiRow);
                    continue;
                }
                float fZDiff;
                if (IsPointInsideTriangle(static_cast<uint16_t>(i), kCorner00, fZDiff, 3.40282347e+38f, 180.0f) ||
                    IsPointInsideTriangle(static_cast<uint16_t>(i), kCorner10, fZDiff, 3.40282347e+38f, 180.0f) ||
                    IsPointInsideTriangle(static_cast<uint16_t>(i), kCorner01, fZDiff, 3.40282347e+38f, 180.0f) ||
                    IsPointInsideTriangle(static_cast<uint16_t>(i), kCorner11, fZDiff, 3.40282347e+38f, 180.0f)) {
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
    rt::eh_vector_constructor_iterator(edgeVerts, sizeof(NiPoint3), 3, &emptyPointConstructor);
    NavMeshTriangle* ptri = GetTriangle(ausTri);
    uint16_t Vert = ptri->GetVert(0);
    edgeVerts[0] = GetVertex(Vert)->Location;
    uint16_t v6 = ptri->GetVert(1u);
    edgeVerts[1] = GetVertex(v6)->Location;
    uint16_t v7 = ptri->GetVert(2u);
    edgeVerts[2] = GetVertex(v7)->Location;
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
                if (fa3 * fa4 <= 0.0 && (fa1 != 0.0 && fa4 != 0.0)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// The form ID is read before the NavMeshInfo map, as the game does.
bool NavMesh::UpdateNavMeshInfo() {
    uint32_t uiFormID = GetFormID();
    NavMeshInfo* pInfo = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(uiFormID);
    if (GetNavMeshInfo()) {
        pInfo->CreateFromNavMesh(this);
        return false;
    }
    void* pMemory = ourMalloc(sizeof(NavMeshInfo));
    NavMeshInfo* pNewInfo = pMemory ? ::new (pMemory) NavMeshInfo : nullptr;
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
        NiPoint3 triMin(3.4028235e+38f, 3.4028235e+38f, 0.0f);
        NiPoint3 triMax(-3.4028235e+38f, -3.4028235e+38f, 0.0f);
        for (int32_t j = 0; j < 3; ++j) {
            uint16_t Vert = ptri->GetVert(j);
            NiPoint3* loc = &GetVertex(Vert)->Location;
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
            for (uint32_t j_2 = istartCol; j_2 <= iendCol; ++j_2) {
                BSSimpleArray<uint16_t, 1024>* pcellarray = MeshGrid.GetTriangleArray(j_2, i);
                if (pcellarray) {
                    uint16_t aItem = t;
                    pcellarray->Add(aItem);
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
        NiPoint3 triMin(3.4028235e+38f, 3.4028235e+38f, 0.0f);
        NiPoint3 triMax(-3.4028235e+38f, -3.4028235e+38f, 0.0f);
        for (int32_t j = 0; j < 3; ++j) {
            uint16_t Vert = ptri->GetVert(j);
            NiPoint3* loc = &GetVertex(Vert)->Location;
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
            for (uint32_t j_2 = istartCol; j_2 <= iendCol; ++j_2) {
                BSSimpleArray<uint16_t, 1024>* pcellarray = MeshGrid.GetTriangleArray(j_2, i);
                uint16_t usTriangle = static_cast<uint16_t>(t);
                uint32_t iindex =
                    pcellarray->Find(usTriangle, 0, &BSSimpleArray<uint16_t, 1024>::DefaultEqualFunctionByAddress);
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
    float fBestZDiff = 3.40282347e+38f;
    float fWaterHeight = -3.40282347e+38f;
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
    float fClosestCenterDist = 3.40282347e+38f;
    uint16_t usClosestCenter = 0xFFFF;
    uint16_t usBestInside = 0xFFFF;
    const BSSimpleArray<uint16_t>* pArray = nullptr;
    MeshGrid.GetTriangleData(aLocation, pArray);
    if (pArray) {
        uint32_t uiCount = pArray->GetSize();
        float fBestZDiff = 3.40282347e+38f;
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
        float fBestZDiff = 3.40282347e+38f;
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
        kTriVert1Loc, kTriVert2Loc, kTriVert3Loc, aLocation, afZDiff, afMaxZdistAbove, afMaxZdistBelow
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
        return IsPointInsideTriangle(asTriangleIndex, aLocation, afZDiff, 3.40282347e+38f, 3.40282347e+38f);
    }
    NiPoint3 kTriVert1Loc;
    NiPoint3 kTriVert2Loc;
    NiPoint3 kTriVert3Loc;
    NavMeshTriangle* pTriangle = GetTriangle(asTriangleIndex);
    kTriVert1Loc = GetVertex(pTriangle->GetVert(0))->Location;
    kTriVert2Loc = GetVertex(pTriangle->GetVert(1))->Location;
    kTriVert3Loc = GetVertex(pTriangle->GetVert(2))->Location;
    bool bInside = NavMeshMath::IsPointInsideTriangle(
        kTriVert1Loc, kTriVert2Loc, kTriVert3Loc, aLocation, afZDiff, 3.40282347e+38f, 3.40282347e+38f
    );
    if (bInside && apFilter) {
        NiPoint3 kTriLocation = aLocation;
        kTriLocation.z = kTriLocation.z + afZDiff;
        return apFilter->IsValidTri(aLocation, this, asTriangleIndex, kTriLocation);
    }
    return bInside;
}

NavMeshClosedDoorInfo* NavMesh::GetClosedDoorInfoForTriangle(uint16_t aiTriangle) {
    for (uint32_t i = 0; i < ClosedDoors.GetSize(); ++i) {
        if (ClosedDoors.GetAt(i).iTriangleIndex == aiTriangle) {
            return &ClosedDoors.GetAt(i);
        }
    }
    return nullptr;
}

void NavMesh::GetOverlappedTriangleList(
    const NiPointer<ObstacleData>& apObstacle,
    const NiPoint3& afFudge,
    BSSimpleArray<uint16_t, 1024>* apRetTriangles
) {
    NiPoint3 result;
    NiPoint3 v41;
    NiPoint3 v42;
    NiPoint3 v43;
    NiPoint3 dst;
    uint32_t istartRow;
    uint32_t iendRow;
    uint32_t istartCol;
    NiPoint3 cornersLocalSpace[4];
    uint32_t iendCol;
    istartRow = 0;
    istartCol = 0;
    iendRow = 0;
    iendCol = 0;
    MeshGrid.GetGridRowAndColumn(apObstacle.m_pObject->aabbMin, &istartRow, &istartCol);
    MeshGrid.GetGridRowAndColumn(apObstacle.m_pObject->aabbMax, &iendRow, &iendCol);
    rt::eh_vector_constructor_iterator(cornersLocalSpace, sizeof(NiPoint3), 4, &emptyPointConstructor);
    float a2 = apObstacle.m_pObject->BoxMin.z;
    float v34 = apObstacle.m_pObject->BoxMin.y - afFudge.y;
    float v33 = apObstacle.m_pObject->BoxMin.x - afFudge.x;
    cornersLocalSpace[0] = *::new (&dst) NiPoint3(v33, v34, a2);
    float a2a = apObstacle.m_pObject->BoxMin.z;
    float v32 = apObstacle.m_pObject->BoxMax.y + afFudge.y;
    float v31 = apObstacle.m_pObject->BoxMin.x - afFudge.x;
    cornersLocalSpace[1] = *::new (&v43) NiPoint3(v31, v32, a2a);
    float a2b = apObstacle.m_pObject->BoxMin.z;
    float v30 = apObstacle.m_pObject->BoxMax.y + afFudge.y;
    float v29 = apObstacle.m_pObject->BoxMax.x + afFudge.x;
    cornersLocalSpace[2] = *::new (&v42) NiPoint3(v29, v30, a2b);
    float a2c = apObstacle.m_pObject->BoxMin.z;
    float v28 = apObstacle.m_pObject->BoxMin.y - afFudge.y;
    float v27 = apObstacle.m_pObject->BoxMax.x + afFudge.x;
    cornersLocalSpace[3] = *::new (&v41) NiPoint3(v27, v28, a2c);
    ObstacleData* v4 = apObstacle.m_pObject;
    float fzheight = v4->aabbMax.z - apObstacle.m_pObject->aabbMin.z;
    for (uint32_t r = istartRow; r <= iendRow; ++r) {
        for (uint32_t c = istartCol; c <= iendCol; ++c) {
            BSSimpleArray<uint16_t, 1024>* parray = MeshGrid.GetTriangleArray(c, r);
            if (parray) {
                for (uint32_t i = 0;; ++i) {
                    uint32_t FormFlags = parray->iSize;
                    if (i >= FormFlags) {
                        break;
                    }
                    if (apRetTriangles->Find(parray->GetAt(i), 0, return_equals) == -1) {
                        NavMeshTriangle* ptri = GetTriangle(parray->GetAt(i));
                        if (!ptri->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                            NiPoint3 wtriMin(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
                            NiPoint3 wtriMax(-3.4028235e+38f, -3.4028235e+38f, -3.4028235e+38f);
                            for (int32_t v = 0; v < 3; ++v) {
                                uint16_t Vert = ptri->GetVert(static_cast<uint32_t>(v));
                                NiPoint3* vertLocation = &GetVertex(Vert)->Location;
                                wtriMin.x = BSMIN(wtriMin.x, vertLocation->x);
                                wtriMin.y = BSMIN(wtriMin.y, vertLocation->y);
                                wtriMin.z = BSMIN(wtriMin.z, vertLocation->z);
                                wtriMax.x = BSMAX(wtriMax.x, vertLocation->x);
                                wtriMax.y = BSMAX(wtriMax.y, vertLocation->y);
                                wtriMax.z = BSMAX(wtriMax.z, vertLocation->z);
                            }
                            wtriMax.z = wtriMax.z + 128.0;
                            NiPoint3* x = ::new (&result) NiPoint3(apObstacle.m_pObject->aabbMax + afFudge);
                            if (Pathing::DoAABBsOverlap(
                                    apObstacle.m_pObject->aabbMin - afFudge, *x, wtriMin, wtriMax
                                )) {
                                bool bOverlap = false;
                                if (NavMeshObstacleManager::IsPointInBox(
                                        GetCenter(parray->GetAt(i)), apObstacle, fzheight, 128.0f
                                    )) {
                                    bOverlap = true;
                                }
                                else {
                                    float fdontCare;
                                    NiPoint3* p_Center = &apObstacle.m_pObject->Center;
                                    if (IsPointInsideTriangle(
                                            parray->GetAt(i), *p_Center, fdontCare, fzheight, 128.0f
                                        )) {
                                        bOverlap = true;
                                    }
                                    else {
                                        for (uint32_t triv = 0; triv < 3; ++triv) {
                                            uint32_t triw = triv + 1;
                                            if (triv == 2) {
                                                triw = 0;
                                            }
                                            NiMatrix3* a2d = &apObstacle.m_pObject->Orientation;
                                            NiPoint3* a1b = &apObstacle.m_pObject->Center;
                                            uint16_t v9 = ptri->GetVert(triv);
                                            NiPoint3* vertLocation_2 = &GetVertex(v9)->Location;
                                            NiPoint3 vert1 = (*vertLocation_2 - *a1b) * *a2d;
                                            NiMatrix3* a2e = &apObstacle.m_pObject->Orientation;
                                            NiPoint3* a1c = &apObstacle.m_pObject->Center;
                                            uint16_t v12 = ptri->GetVert(triw);
                                            NiPoint3* vertLocation_3 = &GetVertex(v12)->Location;
                                            NiPoint3 vert2 = (*vertLocation_3 - *a1c) * *a2e;
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
    NiPoint3 kFudge = NiPoint3_ZERO;
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
    for (uint32_t i = 0; i < pTriangles->GetSize(); ++i) {
        NavMeshClosedDoorInfo* pInfo = GetClosedDoorInfoForTriangle(pTriangles->GetAt(i));
        if (pInfo && pInfo->DoorFormID == aiDoorID) {
            continue;
        }
        GetTriangle(pTriangles->GetAt(i))->SetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
        NavMeshClosedDoorInfo kInfo;
        kInfo.~NavMeshClosedDoorInfo();
        ::new (&kInfo) NavMeshClosedDoorInfo;
        kInfo.DoorFormID = aiDoorID;
        kInfo.iTriangleIndex = pTriangles->GetAt(i);
        ClosedDoors.Add(kInfo);
    }
    return true;
}

bool NavMesh::IsPointInsideNavmesh(const NiPoint3& aPoint, FindTriangleForLocationFilter* apFilter) {
    return (bool)((uint8_t)(FindTriangleForLocation(aPoint, apFilter) != 65535));
}

void NavMesh::ComputeBounds(NiPoint3* apRetMin, NiPoint3* apRetMax) {
    uint16_t vertex;
    uint16_t v4;
    int32_t aiVertIndex;
    uint32_t i;
    NiPoint3 location;
    NavMeshVertex v9;
    NiPoint3 ecx0;
    NavMeshTriangle* triangle;
    bool vCondition1;
    vCondition1 = !(&Triangles)->IsEmpty();
    if (vCondition1) {
        ecx0;
        location;
        v9;
        triangle = GetTriangle(0);
        vertex = triangle->GetVert(0);
        location = GetVertex(vertex)->Location;
        ecx0 = location;
        for (i = 0; i < GetTriangleCount(); ++i) {
            triangle = GetTriangle(i);
            for (aiVertIndex = 0; aiVertIndex < 3; ++aiVertIndex) {
                v4 = triangle->GetVert(aiVertIndex);
                v9 = *GetVertex(v4);
                if (!(ecx0.x > ((double)v9.Location.x))) {
                    if (location.x < ((double)v9.Location.x)) {
                        location.x = v9.Location.x;
                    }
                }
                else {
                    ecx0.x = v9.Location.x;
                }
                if (!(ecx0.y > ((double)v9.Location.y))) {
                    if (location.y < ((double)v9.Location.y)) {
                        location.y = v9.Location.y;
                    }
                }
                else {
                    ecx0.y = v9.Location.y;
                }
                if (!(ecx0.z > ((double)v9.Location.z))) {
                    if (location.z < ((double)v9.Location.z)) {
                        location.z = v9.Location.z;
                    }
                }
                else {
                    ecx0.z = v9.Location.z;
                }
            }
        }
        *apRetMin = ecx0;
        *apRetMax = location;
    }
}

EdgeExtraInfo* NavMesh::SetEmptyEdgeExtraInfo(uint16_t aTriangle, int32_t aEdgeIndex) {
    NavMeshTriangle* v3;
    uint16_t v5;
    EdgeExtraInfo arInfo;
    EdgeExtraInfo* pEdgeInfo = GetEdgeExtraInfo(aTriangle, aEdgeIndex);
    if (!pEdgeInfo) {
        arInfo.~EdgeExtraInfo();
        ::new (&arInfo) EdgeExtraInfo;
        v5 = AddExtraEdgeInfo(arInfo);
        v3 = &(&Triangles)->GetAt(aTriangle);
        v3->SetEdgeExtraInfo(aEdgeIndex, v5);
        pEdgeInfo = GetEdgeExtraInfo(aTriangle, aEdgeIndex);
    }
    pEdgeInfo->Portal.pOtherMesh = nullptr;
    pEdgeInfo->Portal.sTriangle = (uint16_t)(-1);
    return pEdgeInfo;
}

bool NavMesh::IsPointInsideTriangle_0(uint16_t a1, const NiPoint3* a2, float a3, float a4, float* a5) {
    NavMeshTriangle* Triangle = GetTriangle(a1);
    uint16_t Vert = Triangle->GetVert(0);
    NavMeshVertex* Vertex = GetVertex(Vert);
    uint16_t v7 = Triangle->GetVert(1u);
    NavMeshVertex* v11 = GetVertex(v7);
    uint16_t v8 = Triangle->GetVert(2u);
    NavMeshVertex* v16 = GetVertex(v8);
    float v18 = (v11->Location.x - Vertex->Location.x) * (v16->Location.y - Vertex->Location.y) -
        (v16->Location.x - Vertex->Location.x) * (v11->Location.y - Vertex->Location.y);
    if (v18 == 0.0) {
        return false;
    }
    float v14 = v11->Location.x - a2->x;
    float v15 = v16->Location.x - a2->x;
    float v13 = v11->Location.y - a2->y;
    float v25 = v16->Location.y - a2->y;
    float v22 = 1.0 / v18;
    float v23 = (v14 * v25 - v15 * v13) * v22;
    if (v23 < 0.0) {
        return false;
    }
    float v17 = Vertex->Location.x - a2->x;
    float v26 = Vertex->Location.y - a2->y;
    float v21 = (v15 * v26 - v17 * v25) * v22;
    if (v21 < 0.0) {
        return false;
    }
    float v19 = (v17 * v13 - v14 * v26) * v22;
    if (v19 < 0.0) {
        return false;
    }
    *a5 = v23 * Vertex->Location.z + v21 * v11->Location.z + v19 * v16->Location.z;
    float v20 = a2->z - *a5;
    return !(a3 < static_cast<double>(v20)) && !(v20 < -a4);
}

uint32_t NavMesh::AddObstacleUndoData(ObstacleUndoData* apUndoData) {
    NiPointer<ObstacleUndoData> spUndoData(apUndoData);
    return Obstacles.Add(spUndoData);
}

void NavMesh::FlipTriangle(uint16_t aiTriIndex) {
    NavMeshTriangle* ptri = GetTriangle(aiTriIndex);
    uint16_t vert1index_2 = ptri->GetVert(1u);
    uint16_t vert1index = ptri->GetVert(0);
    ptri->SetVert(1u, vert1index);
    ptri->SetVert(0, vert1index_2);
    bool bEdge0IsPortal = ptri->EdgeHasExtraInfo(0);
    bool bEdge1IsPortal = ptri->EdgeHasExtraInfo(1);
    uint16_t tri1index = ptri->GetTriOrExtraInfo(1);
    uint16_t tri1index_2 = ptri->GetTriOrExtraInfo(0);
    if (bEdge0IsPortal) {
        ptri->SetEdgeExtraInfo(1, tri1index_2);
    }
    else {
        ptri->SetTri(1u, tri1index_2);
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
    uint16_t v20 = 0;
    int32_t v21 = -1;
    float v22 = 3.4028235e+38f;
    float v25 = afRadius * afRadius;
    float v23 = aLocation.z - 64.0;
    float v24 = aLocation.z + 180.0;
    if (abFlipForStartNode) {
        v23 = aLocation.z - 180.0;
        v24 = aLocation.z + 64.0;
    }
    uint32_t FormFlags = Triangles.iSize;
    for (uint32_t i = 0; i < FormFlags; ++i) {
        NavMeshTriangle* Triangle = GetTriangle(i);
        for (int32_t aiEdgeIndex = 0; aiEdgeIndex < 3; ++aiEdgeIndex) {
            if (!Triangle->IsEdgeConnected(aiEdgeIndex)) {
                NiPoint3 v10;
                NavMeshEdgeVertices result;
                NiPoint3 v13;
                result = GetEdgeVertices(i, aiEdgeIndex);
                v13 = (result.Vertices[0]->Location + result.Vertices[1]->Location) * 0.5f;
                v10 = v13 - aLocation;
                float v14 = v10.SqrXYLength();
                float v15 = v10.z * v10.z;
                float v12 = v14 + v15;
                if (v22 > static_cast<double>(v14) &&
                    ((v24 > static_cast<double>(v13.z) && v23 < static_cast<double>(v13.z)) || v12 <= v25 - v15)) {
                    v20 = i;
                    v21 = aiEdgeIndex;
                    v22 = v12;
                }
            }
        }
    }
    if (v21 == -1) {
        return false;
    }
    *apRetTriIndex = v20;
    *apRetEdgeIndex = v21;
    return true;
}

void NavMesh::GetCircumCenter2D(uint16_t apTriangle, NiPoint3& aOut) {
    NavMeshTriangle* Triangle = GetTriangle(apTriangle);
    uint16_t Vert = Triangle->GetVert(0);
    NavMeshVertex v21 = *GetVertex(Vert);
    NavMeshTriangle* v5 = GetTriangle(apTriangle);
    uint16_t v6 = v5->GetVert(1u);
    NavMeshVertex v20 = *GetVertex(v6);
    NavMeshTriangle* v7 = GetTriangle(apTriangle);
    uint16_t v8 = v7->GetVert(2u);
    NavMeshVertex* Vertex = GetVertex(v8);
    float z = Vertex->Location.z;
    float v11 = v20.Location.x - v21.Location.x;
    float v15 = v20.Location.y - v21.Location.y;
    float v16 = Vertex->Location.x - v21.Location.x;
    float v18 = Vertex->Location.y - v21.Location.y;
    float v12 = v11 * v11 + v15 * v15;
    float v17 = v16 * v16 + v18 * v18;
    float v19 = v11 * v18 - v16 * v15;
    float v13 = 0.5 / v19;
    aOut.x = v21.Location.x - (v15 * v17 - v18 * v12) * v13;
    aOut.y = (v11 * v17 - v16 * v12) * v13 + v21.Location.y;
    aOut.z = 0.0;
}

bool NavMesh::FindPortalTriangles(const NavMeshInfo* apNavMeshInfo, BSSimpleArray<int32_t, 1024>* apReturnArray) {
    apReturnArray->SetSize(0, true);
    apReturnArray->SetReservedSize(100u);
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = Triangles.iSize;
        if (i >= FormFlags) {
            break;
        }
        NavMeshTriangle* pTri = GetTriangle(i);
        if (pTri->HasEdgeExtraInfo()) {
            for (int32_t j = 0; j < 3; ++j) {
                if (pTri->EdgeHasExtraInfo(j)) {
                    uint16_t TriOrExtraInfo = pTri->GetTriOrExtraInfo(j);
                    if (GetExtraEdgeInfo(TriOrExtraInfo)->Portal.pOtherMesh == apNavMeshInfo) {
                        int32_t a2 = i;
                        apReturnArray->Add(a2);
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
    const BSSimpleArray<uint16_t, 1024>* ptriArray;
    ptriArray = nullptr;
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
    NiPoint3 result;
    bool IsPointInsideNavmesh = false;
    NiPoint3 aLocation = NiPoint3_ZERO;
    for (uint32_t i = 0; i < 8 && !IsPointInsideNavmesh; ++i) {
        float a1 = -aRadius;
        float v13 = RandomFloat(a1, aRadius);
        float v8 = aRadius * aRadius - v13 * v13;
        float v12 = std::sqrt(v8);
        float a1a = -v12;
        v12 = RandomFloat(a1a, v12);
        aLocation.x = v13;
        aLocation.y = v12;
        NiPoint3* v4 = &(result = aLocation + aStartLocation);
        IsPointInsideNavmesh = this->IsPointInsideNavmesh(*v4, nullptr);
    }
    if (!IsPointInsideNavmesh) {
        return false;
    }
    aLocation.operator+=(aStartLocation);
    *apRet1 = MoveAwayFromEdge(aLocation, 64.0f);
    return true;
}

NiPoint3 NavMesh::MoveAwayFromEdge(const NiPoint3& aLocation, float afDistance) {
    NiPoint3 retLoc = aLocation;
    uint16_t usTri = FindTriangleForLocation(aLocation, nullptr);
    if (usTri != 0xFFFF) {
        uint16_t v28[4];
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
                    aLocation, vertices.Vertices[0]->Location, vertices.Vertices[1]->Location
                );
                if (afDistance > static_cast<double>(fdistanceFromEdge)) {
                    NiPoint3 v12;
                    NiPoint3 rightAngleVector;
                    NiPoint3 triangleNormal = GetNormal(usTri);
                    rightAngleVector =
                        triangleNormal.Cross(vertices.Vertices[0]->Location - vertices.Vertices[1]->Location);
                    rightAngleVector.Unitize();
                    float fScalar = afDistance - fdistanceFromEdge;
                    retLoc.operator+=((v12 = rightAngleVector * fScalar));
                }
            }
            else {
                v28[isize++] = ival;
            }
        }
        for (uint32_t i = 0; i < isize; ++i) {
            NavMeshTriangle* ptri = GetTriangle(v28[i]);
            for (int32_t e = 0; e < 3; ++e) {
                if (!ptri->EdgeHasExtraInfo(e) && ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                    NavMeshEdgeVertices vertices_2;
                    vertices_2 = GetEdgeVertices(v28[i], e);
                    float fdistanceFromEdge_2 = NavMeshMath::DistanceFromLine(
                        aLocation, vertices_2.Vertices[0]->Location, vertices_2.Vertices[1]->Location
                    );
                    if (afDistance > static_cast<double>(fdistanceFromEdge_2)) {
                        NiPoint3 v10;
                        NiPoint3 rightAngleVector_2;
                        NiPoint3 triangleNormal_2 = GetNormal(v28[i]);
                        rightAngleVector_2 =
                            triangleNormal_2.Cross(vertices_2.Vertices[0]->Location - vertices_2.Vertices[1]->Location);
                        rightAngleVector_2.Unitize();
                        float v7 = afDistance - fdistanceFromEdge_2;
                        retLoc.operator+=((v10 = rightAngleVector_2 * v7));
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
    rt::eh_vector_constructor_iterator(edgeVerts2D, sizeof(NiPoint2), 3, &emptyPointConstructor);
    for (int32_t i = 0; i < 3; ++i) {
        NavMeshTriangle* Triangle = GetTriangle(ausTri);
        uint16_t v7 = Triangle->GetVert(static_cast<uint32_t>(i));
        NiPoint3 vert = GetVertex(v7)->Location;
        edgeVerts2D[i].x = vert.x;
        edgeVerts2D[i].y = vert.y;
    }
    for (int32_t i_2 = 0; i_2 < 3; ++i_2) {
        if (i_2 != aIgnoreEdge) {
            int32_t j = i_2 + 1;
            if (i_2 == 2) {
                j = 0;
            }
            if (NavMeshMath::Find2DIntersection_006B9E90(
                    edgeVerts2D[i_2], edgeVerts2D[j], aStartPoint, aEndPoint, apReturnIntersection
                )) {
                return i_2;
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
    NavMesh::FIEResult v12;
    NavMesh::FIEResult result_2;
    NiPoint3 edgeVerts[3];
    rt::eh_vector_constructor_iterator(edgeVerts, sizeof(NiPoint3), 3, &emptyPointConstructor);
    float x = aEndPoint.x;
    float y = aEndPoint.y;
    float z = aEndPoint.z;
    float v19 = 0.0f;
    for (int32_t i = 0; i < 3; ++i) {
        NavMeshTriangle* Triangle = GetTriangle(ausTri);
        uint16_t Vert = Triangle->GetVert(i);
        edgeVerts[i] = GetVertex(Vert)->Location;
    }
    for (int32_t i_2 = 0; i_2 < 3; ++i_2) {
        if (i_2 != aIgnoreEdge) {
            int32_t j = i_2 + 1;
            if (i_2 == 2) {
                j = 0;
            }
            if (!edgeVerts[i_2].operator==(edgeVerts[j])) {
                v12.~FIEResult();
                ::new (&v12) NavMesh::FIEResult;
                if (NavMeshMath::Find2DIntersectionStable(
                        edgeVerts[i_2], edgeVerts[j], aStartPoint, aEndPoint, &v12.Intersection
                    )) {
                    v12.eType = NavMesh::FIE_INTERSECTION;
                    v12.iEdge = i_2;
                    apResult->eType = NavMesh::FIE_INTERSECTION;
                    apResult->iEdge = v12.iEdge;
                    apResult->Intersection = v12.Intersection;
                    return apResult;
                }
            }
        }
    }
    result_2.~FIEResult();
    ::new (&result_2) NavMesh::FIEResult;
    result_2.iEdge = -1;
    apResult->eType = NavMesh::FIE_NONE;
    apResult->iEdge = result_2.iEdge;
    apResult->Intersection = result_2.Intersection;
    return apResult;
}

bool NavMesh::AddObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps) {
    NavMeshObstacleCutter cutter;
    bool bret = cutter.SplitNavMeshFromObstacle(this, apObstacle, aPortalSwaps);
    return bret;
}

void NavMesh::ClearAllObstacles() {
    TaskPortalSwap apOutPoints;
    for (uint32_t aiUndoIndex = Obstacles.iSize - 1; aiUndoIndex != -1; --aiUndoIndex) {
        RemoveObstacle(aiUndoIndex, apOutPoints);
    }
}

uint32_t NavMesh::GetUndoDataMaxObstacleCount(ObstacleUndoData* apUndoData) {
    uint32_t iA = 0;
    for (uint32_t auiIndex = 0; auiIndex < apUndoData->OverlappedTriangles.iSize; ++auiIndex) {
        uint16_t* v2 = &apUndoData->OverlappedTriangles.GetAt(auiIndex);
        NavMeshTriangle* Triangle = GetTriangle(*v2);
        uint16_t ObjectCount = Triangle->GetObjectCount();
        iA = BSMAX(iA, ObjectCount);
    }
    for (uint32_t i = 0; i < apUndoData->BoundsTriangles.iSize; ++i) {
        NavMeshTriangle* v5 = GetTriangle((&apUndoData->BoundsTriangles.GetAt(i))->usTriangleIndex);
        uint16_t v6 = v5->GetObjectCount();
        iA = BSMAX(iA, v6);
    }
    return iA;
}

bool NavMesh::RemoveClosedDoor(uint32_t aiDoorID) {
    for (uint32_t i = 0; i < ClosedDoors.iSize; ++i) {
        if ((&ClosedDoors.GetAt(i))->DoorFormID == aiDoorID) {
            uint32_t iTriangleIndex = (&ClosedDoors.GetAt(i))->iTriangleIndex;
            if (iTriangleIndex < GetTriangleCount()) {
                NavMeshTriangle* Triangle = GetTriangle((&ClosedDoors.GetAt(i))->iTriangleIndex);
                Triangle->ClearFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
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
        uint16_t* v3 = &ClosestPOVs.GetAt(static_cast<int32_t>(ausTriangle));
        aRetPOVs.Add(*v3);
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
        if (ObstaclePOVs.Find(ivertIndex, 0, return_equals) != -1) {
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
    XFNV_INLINED_LOCAL(NavMeshPtr, apResult, std::nullopt, std::nullopt);
    if (apNavmeshInfo) {
        using connectedInfos_t = BSSimpleArray<NavMeshInfo*, 1024>;
        connectedInfos_t connectedInfos;
        NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
        NavMeshInfoMap_->GetAllNavMeshInfo(&connectedInfos);
        for (uint32_t i = 0;; ++i) {
            uint32_t FormFlags = connectedInfos.iSize;
            if (i >= FormFlags) {
                break;
            }
            NavMeshInfo* pinfo = connectedInfos.GetAt(static_cast<int32_t>(i));
            int32_t isize = pinfo->AdjacentNavMeshes.iSize;
            int32_t iprefferedsize = pinfo->PreferredAdjacentNavMeshes.iSize;
            for (int32_t p = 0; p < iprefferedsize + isize; ++p) {
                NavMeshInfo** v3;
                if (p >= isize) {
                    v3 = &pinfo->PreferredAdjacentNavMeshes.GetAt(p - isize);
                }
                else {
                    v3 = &pinfo->AdjacentNavMeshes.GetAt(p);
                }
                NavMeshInfo* pportal = *v3;
                if (pportal == apNavmeshInfo) {
                    NavMeshPtr spnavmesh;
                    if (!pinfo->GetNavMeshEvenDisabled(spnavmesh)) {
                        TESForm* pnavmeshForm = TESForm::GetFormByNumericID(pinfo->NavMeshID);
                        NavMesh* pnavmesh = dynamic_cast<NavMesh*>(pnavmeshForm);
                        NavMeshPtr* ptr = NavMeshPtr::MakeNavMeshPtr(&apResult, pnavmesh);
                        spnavmesh.operator=(*ptr);
                        apResult = nullptr;
                    }
                    if (spnavmesh.operator NavMesh*()) {
                        NavMesh* Object = spnavmesh.m_pObject;
                        Object->SetAltered(true);
                        for (uint32_t e = 0; e < spnavmesh.m_pObject->ExtraEdgeInfo.GetSize(); ++e) {
                            EdgeExtraInfo* pinfo_2 = spnavmesh.m_pObject->GetExtraEdgeInfo(e);
                            if (pinfo_2->Portal.pOtherMesh == apNavmeshInfo) {
                                spnavmesh.m_pObject->DeleteExtraEdgeInfo(e--);
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
