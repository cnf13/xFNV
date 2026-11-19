#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSDevices/Xenon/XInputManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleEdge.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <new>

// File statics; no code reads them. The first operand of the sum, 128.0f, is not named anywhere.
static const float NAVMESH_COVER_SPLIT_EDGE_LENGTH = 128.0f + 32.0f;
static const float NAVMESH_COVER_HALF_GRANULARITY = NAVMESH_COVER_HEIGHT_GRANULARITY * 0.5;

using BSSimpleArray_BoundsTriangleInfo_1024 = BSSimpleArray<BoundsTriangleInfo, 1024>;

int32_t CompareUInt(const uint32_t* a1, const uint32_t* a2) {
    if ((*a1) > (*a2)) {
        return 1;
    }
    if ((*a2) <= (*a1)) {
        return 0;
    }
    return (-1);
}

bool EdgeHandleEqual(const NavMeshEdgeHandle* a1, const NavMeshEdgeHandle* a2) {
    return *a1 == *a2;
}

NavMeshObstacleVertex::NavMeshObstacleVertex(
    uint16_t ausVertexIndex,
    bool abOnObstacle,
    bool abOnBounds,
    bool abOnOpenEdge
)
  : usVertexIndex(ausVertexIndex), bOnObstacle(abOnObstacle), bOnBounds(abOnBounds), bOnOpenEdge(abOnOpenEdge),
    bInOverlappedArea(false), bInObstacleArea(false) {}

PortalEdgeSwap::PortalEdgeSwap() {}

PortalEdgeSwap& PortalEdgeSwap::operator=(const PortalEdgeSwap& arOther) {
    // Each edge is copied through FastNavMeshEdgeHandle::Copy; the two handles have one layout.
    reinterpret_cast<FastNavMeshEdgeHandle*>(&hOldEdge)->Copy(
        reinterpret_cast<const FastNavMeshEdgeHandle&>(arOther.hOldEdge)
    );
    reinterpret_cast<FastNavMeshEdgeHandle*>(&hNewEdge)->Copy(
        reinterpret_cast<const FastNavMeshEdgeHandle&>(arOther.hNewEdge)
    );
    bFromAddition = arOther.bFromAddition;
    return *this;
}

ObstacleUndoData::ObstacleUndoData()
  : eState(OBSTACLE_UNDO_STATE_INVALID), spObstacle(nullptr), OverlappedTriangles(32u, 0u), BoundsTriangles(32u, 0u),
    RemovedTriangleToObstacleMap(37u) {}

ObstacleUndoData::~ObstacleUndoData() {}

AStarDefaultHeap::AStarDefaultHeap() {}

AStarDefaultHeap::~AStarDefaultHeap() {}

NavMeshObstacleCutter::NavMeshObstacleCutter()
  : spObstacle(nullptr), Vertices(256u, 0u, &LocalScrapHeap), Edges(256u, 0u, &LocalScrapHeap) {}

// The edges live on the cutter's scrap heap and are never destroyed: their obstacle references are released here.
NavMeshObstacleCutter::~NavMeshObstacleCutter() {
    for (NavMeshObstacleEdge* pEdge : Edges) {
        if (pEdge->spObstacle.m_pObject) {
            pEdge->spObstacle.m_pObject->DecRefCount();
        }
    }
}

bool NavMeshObstacleCutter::SplitNavMeshFromObstacle(
    NavMesh* apNavMesh,
    const NiPointer<ObstacleData>& aBox,
    TaskPortalSwap& aPortalSwaps
) {
    NiPoint3 kFudge;
    pNavMesh = apNavMesh;
    this->spObstacle = aBox;
    this->Vertices.Clear(false);
    for (NavMeshObstacleEdge* pEdge : this->Edges) {
        if (pEdge->spObstacle.m_pObject) {
            pEdge->spObstacle.m_pObject->DecRefCount();
        }
    }
    this->Edges.Clear(false);
    ObstacleUndoData* pNewUndoData = new ObstacleUndoData;
    this->pUndoData = pNewUndoData;
    BSSimpleArray<uint16_t, 1024>* pOverlapped = &this->pUndoData->OverlappedTriangles;
    NiPoint3* pFudge = ::new (&kFudge) NiPoint3(5.0f, 5.0f, 5.0f);
    this->pNavMesh->GetOverlappedTriangleList(aBox, *pFudge, pOverlapped);
    if (this->pUndoData->OverlappedTriangles.IsEmpty()) {
        delete this->pUndoData;
        return false;
    }

    this->pUndoData->eState = ObstacleUndoData::OBSTACLE_UNDO_STATE_ACTIVE;
    this->pUndoData->spObstacle = aBox;
    this->pUndoData->iFirstAddedVertexIndex = this->pNavMesh->GetVertexCount();
    this->pUndoData->iFirstAddedTriangleIndex = this->pNavMesh->GetTriangleCount();
    this->iMaxObstacleCount = this->GetTrianglesMaxObstacleCount();
    this->GetModifiedTriangleList(this->pUndoData->BoundsTriangles);
    for (uint16_t usTriangle : this->pUndoData->OverlappedTriangles) {
        NavMeshTriangle* pTriangle = this->pNavMesh->GetTriangle(usTriangle);
        pTriangle->SetFlag(32);
    }
    this->GenerateOutsideVertsAndEdges();
    for (uint16_t ustriangle : this->pUndoData->OverlappedTriangles) {
        NiPointer<ObstacleData> sppreviousObstacle(static_cast<ObstacleData*>(nullptr));
        if (this->pNavMesh->pTriangleToObstacleMap->GetAt(ustriangle, sppreviousObstacle)) {
            // The by-value argument is copied by NiPointer's copy constructor.
            this->pUndoData->RemovedTriangleToObstacleMap.SetAt(ustriangle, sppreviousObstacle);
            this->pNavMesh->pTriangleToObstacleMap->RemoveAt(ustriangle);
        }
    }
    this->GenerateBoxVertsAndEdges();
    this->SplitEdges();
    for (NavMeshObstacleVertex* pVertex : this->Vertices) {
        if (this->IsVertInObstacleArea(pVertex)) {
            pVertex->bInObstacleArea = true;
        }
    }
    this->RemoveInvalidEdges();
    this->CreateNewEdges();
    if (this->CreateNewTriangles()) {
        this->CreateRemainingEdges();
        this->CreateNewTriangles();
        this->ConnectTriangles();
        this->CreateSwappedEdgeList(aPortalSwaps);
        uint32_t uiVertexCount = this->pNavMesh->GetVertexCount();
        this->pUndoData->iAddedVertexCount = uiVertexCount - this->pUndoData->iFirstAddedVertexIndex;
        uint32_t uiTriangleCount = this->pNavMesh->GetTriangleCount();
        this->pUndoData->iAddedTriangleCount = uiTriangleCount - this->pUndoData->iFirstAddedTriangleIndex;
        this->SetObstacleCount();
        for (uint32_t i_5 = this->pUndoData->iFirstAddedTriangleIndex;
             i_5 < this->pUndoData->iFirstAddedTriangleIndex + this->pUndoData->iAddedTriangleCount; ++i_5) {
            // Passes the index's low word and a copy of spObstacle.
            this->pNavMesh->pTriangleToObstacleMap->SetAt(static_cast<uint16_t>(i_5), this->spObstacle);
        }
        this->AddNewTrianglesToNavMeshGrid();
        this->UpdateCoverArray();
        this->AddNewPOVs();
        this->pNavMesh->AddObstacleUndoData(this->pUndoData);
        return true;
    }

    this->pNavMesh->Vertices.SetSize(this->pUndoData->iFirstAddedVertexIndex, true);
    this->pNavMesh->Triangles.SetSize(this->pUndoData->iFirstAddedTriangleIndex, true);
    delete this->pUndoData;
    return false;
}

bool NavMeshObstacleCutter::CheckPotentialEdgeAgainstRunningEdgeList(
    NavMeshObstacleVertex* apVert1,
    NavMeshObstacleVertex* apVert2
) {
    if (apVert1->bOnObstacle && apVert2->bOnObstacle) {
        return false;
    }
    if (!apVert1->bOnObstacle && apVert1->bInObstacleArea) {
        return false;
    }
    if (!apVert2->bOnObstacle && apVert2->bInObstacleArea) {
        return false;
    }
    NavMeshVertex* a2_8 = pNavMesh->GetVertex(apVert1->usVertexIndex);
    NiPoint3* vert2Loc = &pNavMesh->GetVertex(apVert2->usVertexIndex)->Location;
    NiPoint2 a1(a2_8->Location.x, a2_8->Location.y);
    NiPoint2 v14(vert2Loc->x, vert2Loc->y);
    if (apVert1->bOnObstacle != apVert2->bOnObstacle) {
        NiPoint3 midPoint = (a2_8->Location + *vert2Loc) * 0.5f;
        if (!IsPointInOverlappedArea(midPoint)) {
            return false;
        }
    }
    for (uint32_t i = Edges.iSize - 1; i != -1; --i) {
        if ((Edges.GetAt(static_cast<int32_t>(i))->pVerts[0] == apVert1 &&
             Edges.GetAt(static_cast<int32_t>(i))->pVerts[1] == apVert2) ||
            (Edges.GetAt(static_cast<int32_t>(i))->pVerts[0] == apVert2 &&
             Edges.GetAt(static_cast<int32_t>(i))->pVerts[1] == apVert1)) {
            return false;
        }
        if (Edges.GetAt(static_cast<int32_t>(i))->pVerts[0] != apVert1 &&
            Edges.GetAt(static_cast<int32_t>(i))->pVerts[1] != apVert1 &&
            Edges.GetAt(static_cast<int32_t>(i))->pVerts[0] != apVert2 &&
            Edges.GetAt(static_cast<int32_t>(i))->pVerts[1] != apVert2) {
            NiPoint3* edgeVert1Loc =
                &pNavMesh->GetVertex(Edges.GetAt(static_cast<int32_t>(i))->pVerts[0]->usVertexIndex)->Location;
            NiPoint3* edgeVert2Loc =
                &pNavMesh->GetVertex(Edges.GetAt(static_cast<int32_t>(i))->pVerts[1]->usVertexIndex)->Location;
            NiPoint2 a3(edgeVert1Loc->x, edgeVert1Loc->y);
            NiPoint2 v9(edgeVert2Loc->x, edgeVert2Loc->y);
            if (NavMeshMath::Find2DIntersection(a1, v14, a3, v9)) {
                return false;
            }
        }
    }
    return true;
}

NavMeshObstacleEdge* NavMeshObstacleCutter::CreateEdge(
    NavMeshObstacleVertex* apVert1,
    NavMeshObstacleVertex* apVert2,
    EDGE_TYPE aeEdgeType
) {
    void* v6 = (&LocalScrapHeap)->Allocate(sizeof(NavMeshObstacleEdge), ScrapHeap::ALIGNMENT);
    void* v5 = ::operator new(0x30u, v6);
    if (v5) {
        ::new (v5) NavMeshObstacleEdge(apVert1, apVert2, aeEdgeType);
    }
    return static_cast<NavMeshObstacleEdge*>(v6);
}

bool NavMeshObstacleCutter::IsPointInOverlappedArea(const NiPoint3& aPoint) {
    uint16_t* v2;
    uint32_t i;
    float arZDiff;
    for (i = 0; i < (&pUndoData->OverlappedTriangles)->iSize; ++i) {
        v2 = &(&pUndoData->OverlappedTriangles)->GetAt(i);
        if (pNavMesh->IsPointInsideTriangle(*v2, aPoint, arZDiff, 180.0f, 180.0f)) {
            return true;
        }
    }
    return false;
}

bool NavMeshObstacleCutter::IsVertInOverlappedArea(NavMeshObstacleVertex* apVert) {
    if (apVert->bOnBounds || apVert->bOnOpenEdge) {
        return true;
    }
    NavMeshVertex* vertex = pNavMesh->GetVertex(apVert->usVertexIndex);
    return (bool)((uint8_t)IsPointInOverlappedArea(vertex->Location));
}

// Passes the first index truncated to 16 bits and the last index at full width.
void NavMeshObstacleCutter::AddNewTrianglesToNavMeshGrid() {
    if (pUndoData->iAddedTriangleCount) {
        pNavMesh->AddTrianglesToGrid(
            static_cast<uint16_t>(pUndoData->iFirstAddedTriangleIndex),
            pUndoData->iFirstAddedTriangleIndex + pUndoData->iAddedTriangleCount - 1
        );
    }
}

// Passes the first index truncated to 16 bits and the last index at full width.
void NavMeshObstacleCutter::RemoveTrianglesFromNavMeshGrid() {
    if (pUndoData->iAddedTriangleCount) {
        pNavMesh->RemoveTrianglesFromGrid(
            static_cast<uint16_t>(pUndoData->iFirstAddedTriangleIndex),
            pUndoData->iFirstAddedTriangleIndex + pUndoData->iAddedTriangleCount - 1
        );
    }
}

bool NavMeshObstacleCutter::IsPointInObstacleArea(const NiPoint3& aPoint) const {
    for (uint32_t i = 0; i < pUndoData->OverlappedTriangles.iSize; ++i) {
        if (NavMeshObstacleManager::IsPointInBox(aPoint, spObstacle, 200.0f, 32.0f)) {
            return true;
        }
    }
    return false;
}

bool NavMeshObstacleCutter::ResetNavMeshFromObstacleUndoData(NavMesh* apNavMesh, ObstacleUndoData* apUndoData) {
    pNavMesh = apNavMesh;
    pUndoData = apUndoData;
    RemoveTrianglesFromNavMeshGrid();
    (&pNavMesh->Vertices)->SetSize(pUndoData->iFirstAddedVertexIndex, true);
    (&pNavMesh->Triangles)->SetSize(pUndoData->iFirstAddedTriangleIndex, true);
    return true;
}

bool NavMeshObstacleCutter::CheckTriangleAgainstOverlapped(
    NavMeshObstacleVertex* apVert1,
    NavMeshObstacleVertex* apVert2,
    NavMeshObstacleVertex* apVert3
) {
    NiPoint3 v8;
    NiPoint3 v9;
    NiPoint3 v10;
    NiPoint3 location = pNavMesh->GetVertex(apVert1->usVertexIndex)->Location;
    NiPoint3 v11 = pNavMesh->GetVertex(apVert2->usVertexIndex)->Location;
    NiPoint3 v12 = pNavMesh->GetVertex(apVert3->usVertexIndex)->Location;
    NiPoint3* v4 = &(v9 = location + v11);
    NiPoint3* v5 = &(v8 = *v4 + v12);
    (v10 = *v5 * 0.3333333432674408f);
    return (bool)((int8_t)IsPointInOverlappedArea(v10));
}

bool NavMeshObstacleCutter::CheckTriangleAgainstObstacle(
    NavMeshObstacleVertex* apVert1,
    NavMeshObstacleVertex* apVert2,
    NavMeshObstacleVertex* apVert3
) {
    NiPoint3 vert1Loc = pNavMesh->GetVertex(apVert1->usVertexIndex)->Location;
    NiPoint3 vert2Loc = pNavMesh->GetVertex(apVert2->usVertexIndex)->Location;
    NiPoint3 vert3Loc = pNavMesh->GetVertex(apVert3->usVertexIndex)->Location;
    NiPoint3 center = (vert1Loc + vert2Loc + vert3Loc) * 0.33333f;
    for (uint32_t i = 0;; ++i) {
        float dontCare;
        uint32_t FormFlags = Vertices.iSize;
        if (i >= FormFlags) {
            break;
        }
        NavMeshObstacleVertex* pvert = Vertices.GetAt(static_cast<int32_t>(i));
        NiPoint3 vertLoc = pNavMesh->GetVertex(pvert->usVertexIndex)->Location;
        if (pvert != apVert1 &&
            (pvert != apVert2 &&
             (pvert != apVert3 &&
              NavMeshMath::IsPointInsideTriangle(vert1Loc, vert2Loc, vert3Loc, vertLoc, dontCare, 128.0f, 180.0f)))) {
            return false;
        }
    }
    return !NavMeshObstacleManager::IsPointInBox(center, spObstacle, 128.0f, 180.0f);
}

uint32_t NavMeshObstacleCutter::GetTrianglesMaxObstacleCount() {
    uint16_t* v1;
    NavMeshTriangle* triangle;
    uint16_t objectCount;
    BoundsTriangleInfo* v4;
    NavMeshTriangle* v5;
    uint16_t v6;
    uint32_t j;
    uint32_t i;
    uint32_t v11 = 0;
    for (i = 0; i < (&pUndoData->OverlappedTriangles)->iSize; ++i) {
        v1 = &(&pUndoData->OverlappedTriangles)->GetAt(i);
        triangle = pNavMesh->GetTriangle(*v1);
        objectCount = triangle->GetObjectCount();
        v11 = BSMAX(v11, objectCount);
    }
    for (j = 0; j < (&pUndoData->BoundsTriangles)->iSize; ++j) {
        v4 = &(&pUndoData->BoundsTriangles)->GetAt(j);
        v5 = pNavMesh->GetTriangle(v4->usTriangleIndex);
        v6 = v5->GetObjectCount();
        v11 = BSMAX(v11, v6);
    }
    return v11;
}

void NavMeshObstacleCutter::GetModifiedTriangleList(BSSimpleArray<BoundsTriangleInfo, 1024>& aBoundsTriangle) {
    // The value is always a uint16_t: it is passed to Find as a const uint16_t&.
    uint16_t boundsTriangle;
    for (uint32_t i = 0;; ++i) {
        uint32_t uiCount = pUndoData->OverlappedTriangles.iSize;
        if (i >= uiCount) {
            break;
        }
        uint16_t* v3 = &pUndoData->OverlappedTriangles.GetAt(static_cast<int32_t>(i));
        NavMeshTriangle* pthisTri = pNavMesh->GetTriangle(*v3);
        pthisTri->ClearFlag(256);
        for (int32_t e = 0; e < 3; ++e) {
            if (!pthisTri->EdgeHasExtraInfo(e) && pthisTri->IsEdgeConnected(e)) {
                boundsTriangle = pthisTri->GetTriOrExtraInfo(e);
                uint32_t TriangleCount = pNavMesh->GetTriangleCount();
                if (boundsTriangle < TriangleCount) {
                    NavMeshTriangle* ptri = pNavMesh->GetTriangle(boundsTriangle);
                    if (pUndoData->OverlappedTriangles.Find(
                            boundsTriangle, 0, &BSSimpleArray<uint16_t, 1024>::DefaultEqualFunctionByAddress
                        ) == -1) {
                        bool bInList = false;
                        for (const BoundsTriangleInfo& rBoundsInfo : aBoundsTriangle) {
                            int32_t usTriangleIndex = rBoundsInfo.usTriangleIndex;
                            if (usTriangleIndex == boundsTriangle) {
                                bInList = true;
                                break;
                            }
                        }
                        if (!bInList) {
                            int32_t iboundsTriangleIndex = aBoundsTriangle.iSize;
                            aBoundsTriangle.SetSize(iboundsTriangleIndex + 1, true);
                            BoundsTriangleInfo* v6 = &aBoundsTriangle.GetAt(iboundsTriangleIndex);
                            v6->usTriangleIndex = boundsTriangle;
                            ptri->SetFlag(256);
                            for (uint32_t f = 0; f < 3; ++f) {
                                (&aBoundsTriangle.GetAt(iboundsTriangleIndex))->bPortals[f] =
                                    ptri->EdgeHasExtraInfo(static_cast<int32_t>(f));
                                uint16_t TriOrExtraInfo = ptri->GetTriOrExtraInfo(static_cast<int32_t>(f));
                                (&aBoundsTriangle.GetAt(iboundsTriangleIndex))->Triangles[f] = TriOrExtraInfo;
                            }
                        }
                    }
                }
            }
        }
    }
}

void NavMeshObstacleCutter::GenerateOutsideVertsAndEdges() {
    NavMesh* potherMesh;
    int32_t iotherEdge;
    uint16_t usotherTri;
    for (uint32_t i = 0;; ++i) {
        uint32_t uiCount = pUndoData->OverlappedTriangles.iSize;
        if (i >= uiCount) {
            break;
        }
        uint16_t* v2 = &pUndoData->OverlappedTriangles.GetAt(static_cast<int32_t>(i));
        NavMeshTriangle* ptri = pNavMesh->GetTriangle(*v2);
        for (int32_t e = 0; e < 3; ++e) {
            EDGE_TYPE eedgeType = EDGE_TYPE_OPEN;
            bool bOnBoundsEdge = false;
            bool bOnOpenEdge = false;
            if (ptri->EdgeHasExtraInfo(e)) {
                eedgeType = EDGE_TYPE_PORTAL;
                bOnOpenEdge = true;
            }
            else {
                if (ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                    eedgeType = EDGE_TYPE_OPEN;
                    bOnOpenEdge = true;
                }
                else {
                    uint16_t aValue = ptri->GetTriOrExtraInfo(e);
                    if (pUndoData->OverlappedTriangles.Find(aValue, 0, return_equals) == -1) {
                        eedgeType = EDGE_TYPE_BOUNDS;
                        bOnBoundsEdge = true;
                    }
                }
            }
            if (bOnBoundsEdge || bOnOpenEdge) {
                NavMeshObstacleVertex* edgeObsVertices[2];
                uint16_t edgeVerts[2];
                edgeVerts[0] = ptri->GetVert(static_cast<uint32_t>(e));
                edgeVerts[1] = ptri->GetVert((e + 1) % 3);
                for (uint32_t v = 0; v < 2; ++v) {
                    uint32_t ivertIndex = FindVert(edgeVerts[v]);
                    if (ivertIndex == -1) {
                        NavMeshObstacleVertex* pvert = CreateVertex(edgeVerts[v], false, bOnBoundsEdge, bOnOpenEdge);
                        Vertices.Add(pvert);
                        edgeObsVertices[v] = pvert;
                    }
                    else {
                        bool v11 = Vertices.GetAt(static_cast<int32_t>(ivertIndex))->bOnBounds || bOnBoundsEdge;
                        Vertices.GetAt(static_cast<int32_t>(ivertIndex))->bOnBounds = v11;
                        bool v10 = Vertices.GetAt(static_cast<int32_t>(ivertIndex))->bOnOpenEdge || bOnOpenEdge;
                        Vertices.GetAt(static_cast<int32_t>(ivertIndex))->bOnOpenEdge = v10;
                        NavMeshObstacleVertex** v3 = &Vertices.GetAt(static_cast<int32_t>(ivertIndex));
                        edgeObsVertices[v] = *v3;
                    }
                }
                NavMeshObstacleEdge* poutsideEdge = CreateEdge(edgeObsVertices[0], edgeObsVertices[1], eedgeType);
                if (eedgeType == EDGE_TYPE_BOUNDS) {
                    int32_t v7 = e;
                    uint16_t* v4 = &pUndoData->OverlappedTriangles.GetAt(static_cast<int32_t>(i));
                    if (pNavMesh->GetMatchingEdge(*v4, v7, &potherMesh, &usotherTri, &iotherEdge)) {
                        poutsideEdge->AddTriangle(usotherTri, iotherEdge);
                    }
                }
                if (eedgeType == EDGE_TYPE_OPEN) {
                    NiPointer<ObstacleData>* p_spObstacle = &poutsideEdge->spObstacle;
                    uint16_t v8 = pUndoData->OverlappedTriangles.GetAt(static_cast<int32_t>(i));
                    pNavMesh->GetObstacleForTriangle(v8, *p_spObstacle);
                }
                if (eedgeType == EDGE_TYPE_PORTAL) {
                    poutsideEdge->hOriginalEdge.pNavMeshInfo = nullptr;
                    uint16_t* v5 = &pUndoData->OverlappedTriangles.GetAt(static_cast<int32_t>(i));
                    poutsideEdge->hOriginalEdge.usTriangle = *v5;
                    poutsideEdge->hOriginalEdge.iEdgeIndex = e;
                    uint16_t TriOrExtraInfo = ptri->GetTriOrExtraInfo(e);
                    poutsideEdge->hOriginalEdgeExtraInfo = TriOrExtraInfo;
                }
                Edges.Add(poutsideEdge);
                // The signed edge index is compared unsigned here, as the game does.
                if (static_cast<uint32_t>(e) < 2) {
                    ptri->GetEdgeCoverData(
                        e, poutsideEdge->coverData, poutsideEdge->bLeftSide, poutsideEdge->bRightSide
                    );
                }
            }
        }
    }
}

void NavMeshObstacleCutter::GenerateBoxVertsAndEdges() {
    NiPoint3 v13;
    NiPoint3 v14;
    NiPoint3 v15;
    NiPoint3 dst;
    NavMeshObstacleEdge* pboxedges[4];
    NiPoint3 corners[4];
    NavMeshObstacleVertex* pboxverts[4];
    NiPoint3 ptr[4];
    rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
    ptr[0] =
        *::new (&dst)
            NiPoint3(spObstacle.m_pObject->BoxMin.x, spObstacle.m_pObject->BoxMin.y, spObstacle.m_pObject->BoxMin.z);
    ptr[1] =
        *::new (&v15)
            NiPoint3(spObstacle.m_pObject->BoxMin.x, spObstacle.m_pObject->BoxMax.y, spObstacle.m_pObject->BoxMin.z);
    ptr[2] =
        *::new (&v14)
            NiPoint3(spObstacle.m_pObject->BoxMax.x, spObstacle.m_pObject->BoxMax.y, spObstacle.m_pObject->BoxMin.z);
    ptr[3] =
        *::new (&v13)
            NiPoint3(spObstacle.m_pObject->BoxMax.x, spObstacle.m_pObject->BoxMin.y, spObstacle.m_pObject->BoxMin.z);
    ObstacleData* v1 = spObstacle.m_pObject;
    float fzheight = v1->aabbMax.z - spObstacle.m_pObject->aabbMin.z;
    uint16_t coverData = NavMeshTriangle::GetCoverLevel(fzheight, true, false);
    rt::eh_vector_constructor_iterator(corners, sizeof(NiPoint3), 4, &emptyPointConstructor);
    for (uint32_t i = 0; i < 4; ++i) {
        NiPoint3* z = &spObstacle.m_pObject->Center;
        corners[i] = spObstacle.m_pObject->Orientation * ptr[i] + *z;
        float fnavmeshHeight = corners[i].z;
        if (pNavMesh->ComputeNavMeshZForLocation(corners[i], 128.0f, fzheight, fnavmeshHeight)) {
            corners[i].z = fnavmeshHeight;
        }
    }
    for (uint32_t i_1 = 0; i_1 < 4; ++i_1) {
        NavMeshVertex kVertex;
        kVertex.Location = corners[i_1];
        uint16_t ivertIndex = pNavMesh->AddVertex(kVertex);
        pboxverts[i_1] = CreateVertex(ivertIndex, true, false, false);
        Vertices.Add(pboxverts[i_1]);
    }
    for (uint32_t i_2 = 0; i_2 < 4; ++i_2) {
        NavMeshObstacleEdge* pobsedge = CreateEdge(pboxverts[i_2], pboxverts[(i_2 + 1) % 4], EDGE_TYPE_OBSTACLE);
        pboxedges[i_2] = pobsedge;
        Edges.Add(pobsedge);
    }
    for (uint32_t i_3 = 0; i_3 < 4; ++i_3) {
        NavMeshVertex* za = pNavMesh->GetVertex(pboxedges[i_3]->pVerts[0]->usVertexIndex);
        NavMeshVertex* v6 = pNavMesh->GetVertex(pboxedges[i_3]->pVerts[1]->usVertexIndex);
        if ((v6->Location - za->Location).SqrXYLength() > 64.0) {
            pboxedges[i_3]->coverData = coverData;
            pboxedges[i_3]->bLeftSide = true;
            pboxedges[i_3]->bRightSide = true;
            pboxedges[i_3]->spObstacle = spObstacle;
        }
    }
}

void NavMeshObstacleCutter::SplitEdges() {
    NiPoint3 intersection;
    for (uint32_t i = 0;; ++i) {
        uint32_t uiCount = Edges.iSize;
        if (i >= uiCount) {
            break;
        }
        NavMeshObstacleEdge* pObstacleEdge = Edges.GetAt(static_cast<int32_t>(i));
        if (pObstacleEdge->EdgeType == EDGE_TYPE_OBSTACLE) {
            NavMeshObstacleVertex* pObsVert1 = pObstacleEdge->pVerts[0];
            NavMeshObstacleVertex* pObsVert2 = pObstacleEdge->pVerts[1];
            NiPoint3* obsVert1Loc = &pNavMesh->GetVertex(pObsVert1->usVertexIndex)->Location;
            NiPoint3* obsVert2Loc = &pNavMesh->GetVertex(pObsVert2->usVertexIndex)->Location;
            for (uint32_t j = 0;; ++j) {
                uint32_t uiCount2 = Edges.iSize;
                if (j >= uiCount2) {
                    break;
                }
                NavMeshObstacleEdge* pOpenEdge = Edges.GetAt(static_cast<int32_t>(j));
                if (pOpenEdge->EdgeType == EDGE_TYPE_OPEN || pOpenEdge->EdgeType == EDGE_TYPE_PORTAL) {
                    NavMeshObstacleVertex* pOpenVert1 = pOpenEdge->pVerts[0];
                    NavMeshObstacleVertex* pOpenVert2 = pOpenEdge->pVerts[1];
                    NiPoint3* openVert1Loc = &pNavMesh->GetVertex(pOpenVert1->usVertexIndex)->Location;
                    NiPoint3* openVert2Loc = &pNavMesh->GetVertex(pOpenVert2->usVertexIndex)->Location;
                    if ((pOpenVert1 != pObsVert1 && pOpenVert2 != pObsVert1 && pOpenVert1 != pObsVert2 &&
                         pOpenVert2 != pObsVert2) &&
                        (NavMeshMath::Find2DIntersectionPoint(
                            *openVert1Loc, *openVert2Loc, *obsVert1Loc, *obsVert2Loc, &intersection
                        ))) {
                        NavMeshObstacleVertex* pintersectionVert;
                        float fminIntersectionLengthSqr = 0.0001f;
                        bool bcutObstacleEdge = false;
                        bool bcutOpenEdge = false;
                        if (fminIntersectionLengthSqr > (*openVert1Loc - intersection).SqrXYLength()) {
                            pintersectionVert = pOpenVert1;
                            bcutObstacleEdge = true;
                            bcutOpenEdge = false;
                        }
                        else if (fminIntersectionLengthSqr > (*openVert2Loc - intersection).SqrXYLength()) {
                            pintersectionVert = pOpenVert2;
                            bcutObstacleEdge = true;
                            bcutOpenEdge = false;
                        }
                        else if (fminIntersectionLengthSqr > (*obsVert1Loc - intersection).SqrXYLength()) {
                            pintersectionVert = pObsVert1;
                            bcutObstacleEdge = false;
                            bcutOpenEdge = true;
                        }
                        else if (fminIntersectionLengthSqr > (*obsVert2Loc - intersection).SqrXYLength()) {
                            pintersectionVert = pObsVert2;
                            bcutObstacleEdge = false;
                            bcutOpenEdge = true;
                        }
                        else {
                            NavMeshVertex kVertex;
                            kVertex.Location = intersection;
                            uint16_t ivertIndex = pNavMesh->AddVertex(kVertex);
                            pintersectionVert = CreateVertex(ivertIndex, true, false, true);
                            Vertices.Add(pintersectionVert);
                            bcutObstacleEdge = true;
                            bcutOpenEdge = true;
                        }
                        NavMeshObstacleEdge* pobsEdge1 = nullptr;
                        NavMeshObstacleEdge* pobsEdge2 = nullptr;
                        NavMeshObstacleEdge* popenEdge1 = nullptr;
                        NavMeshObstacleEdge* popenEdge2 = nullptr;
                        if (bcutObstacleEdge) {
                            pobsEdge1 = CreateEdge(pObsVert1, pintersectionVert, EDGE_TYPE_OBSTACLE);
                            pobsEdge2 = CreateEdge(pintersectionVert, pObsVert2, EDGE_TYPE_OBSTACLE);
                            pobsEdge1->bFromSplit = true;
                            pobsEdge2->bFromSplit = true;
                            pobsEdge1->coverData = pObstacleEdge->coverData;
                            pobsEdge2->coverData = pObstacleEdge->coverData;
                            if (pObstacleEdge->bLeftSide) {
                                pobsEdge1->bLeftSide = true;
                            }
                            if (pObstacleEdge->bRightSide) {
                                pobsEdge2->bRightSide = true;
                            }
                            pobsEdge1->spObstacle = pObstacleEdge->spObstacle;
                            pobsEdge2->spObstacle = pObstacleEdge->spObstacle;
                        }
                        if (bcutOpenEdge) {
                            popenEdge1 = CreateEdge(pOpenVert1, pintersectionVert, pOpenEdge->EdgeType);
                            popenEdge2 = CreateEdge(pintersectionVert, pOpenVert2, pOpenEdge->EdgeType);
                            popenEdge1->bFromSplit = true;
                            popenEdge1->hOriginalEdge.Copy(pOpenEdge->hOriginalEdge);
                            popenEdge1->hOriginalEdgeExtraInfo = pOpenEdge->hOriginalEdgeExtraInfo;
                            popenEdge2->bFromSplit = true;
                            popenEdge2->hOriginalEdge.Copy(pOpenEdge->hOriginalEdge);
                            popenEdge2->hOriginalEdgeExtraInfo = pOpenEdge->hOriginalEdgeExtraInfo;
                            popenEdge1->coverData = pOpenEdge->coverData;
                            popenEdge2->coverData = pOpenEdge->coverData;
                            if (pOpenEdge->bLeftSide) {
                                popenEdge1->bLeftSide = true;
                            }
                            if (pOpenEdge->bRightSide) {
                                popenEdge2->bRightSide = true;
                            }
                            popenEdge1->spObstacle = pOpenEdge->spObstacle;
                            popenEdge2->spObstacle = pOpenEdge->spObstacle;
                        }
                        if (bcutObstacleEdge && bcutOpenEdge) {
                            if (i <= j) {
                                if (Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject->DecRefCount();
                                }
                                if (Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject->DecRefCount();
                                }
                                Edges.Remove(j, true);
                                Edges.Remove(i--, true);
                            }
                            else {
                                if (Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject->DecRefCount();
                                }
                                if (Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject->DecRefCount();
                                }
                                Edges.Remove(i, true);
                                Edges.Remove(j, true);
                                i -= 2;
                            }
                        }
                        else {
                            if (bcutObstacleEdge) {
                                if (Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject->DecRefCount();
                                }
                                Edges.Remove(i--, true);
                            }
                            if (bcutOpenEdge) {
                                if (Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject) {
                                    Edges.GetAt(static_cast<int32_t>(j))->spObstacle.m_pObject->DecRefCount();
                                }
                                Edges.Remove(j, true);
                                if (i > j) {
                                    --i;
                                }
                            }
                        }
                        if (bcutObstacleEdge) {
                            Edges.Add(pobsEdge1);
                            Edges.Add(pobsEdge2);
                        }
                        if (bcutOpenEdge) {
                            Edges.Add(popenEdge1);
                            Edges.Add(popenEdge2);
                        }
                        break;
                    }
                }
            }
        }
    }
}

void NavMeshObstacleCutter::RemoveInvalidEdges() {
    for (uint32_t i = 0; i < Edges.iSize; ++i) {
        if (IsEdgeInObstacle(Edges.GetAt(static_cast<int32_t>(i)))) {
            if (Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject) {
                Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject->DecRefCount();
            }
            Edges.RemoveFast(i--, 1u);
        }
        else {
            if (Edges.GetAt(static_cast<int32_t>(i))->EdgeType == EDGE_TYPE_OBSTACLE &&
                ((!Edges.GetAt(static_cast<int32_t>(i))->pVerts[0]->bInOverlappedArea &&
                  !Edges.GetAt(static_cast<int32_t>(i))->pVerts[0]->bOnOpenEdge) ||
                 (!Edges.GetAt(static_cast<int32_t>(i))->pVerts[1]->bInOverlappedArea &&
                  !Edges.GetAt(static_cast<int32_t>(i))->pVerts[1]->bOnOpenEdge))) {
                if (Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject) {
                    Edges.GetAt(static_cast<int32_t>(i))->spObstacle.m_pObject->DecRefCount();
                }
                Edges.RemoveFast(i--, 1u);
            }
        }
    }
}

void NavMeshObstacleCutter::CreateNewEdges() {
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = Vertices.iSize;
        if (i >= FormFlags) {
            break;
        }
        NavMeshObstacleVertex* pobstacleVert = Vertices.GetAt(static_cast<int32_t>(i));
        if (pobstacleVert->bOnObstacle) {
            for (uint32_t j = 0;; ++j) {
                uint32_t v2 = Vertices.iSize;
                if (j >= v2) {
                    break;
                }
                NavMeshObstacleVertex* pboundsVert = Vertices.GetAt(static_cast<int32_t>(j));
                if (!pboundsVert->bInObstacleArea && (pboundsVert->bOnBounds || pboundsVert->bOnOpenEdge) &&
                    CheckPotentialEdgeAgainstRunningEdgeList(pobstacleVert, pboundsVert)) {
                    NavMeshObstacleEdge* pnewedge = CreateEdge(pobstacleVert, pboundsVert, EDGE_TYPE_TWOSIDED);
                    Edges.Add(pnewedge);
                }
            }
        }
    }
}

void NavMeshObstacleCutter::CreateRemainingEdges() {
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = Edges.iSize;
        if (i >= FormFlags) {
            break;
        }
        NavMeshObstacleEdge* pedge1 = Edges.GetAt(static_cast<int32_t>(i));
        if (pedge1->CanBeUsed()) {
            for (uint32_t j = i + 1;; ++j) {
                uint32_t v2 = Edges.iSize;
                if (j >= v2) {
                    break;
                }
                NavMeshObstacleEdge* pedge2 = Edges.GetAt(static_cast<int32_t>(j));
                if (pedge2->CanBeUsed()) {
                    NavMeshObstacleVertex* pnewEdgeVert1;
                    NavMeshObstacleVertex* pnewEdgeVert2;
                    if (pedge1->pVerts[0] == pedge2->pVerts[0]) {
                        pnewEdgeVert1 = pedge1->pVerts[1];
                        pnewEdgeVert2 = pedge2->pVerts[1];
                    }
                    else {
                        if (pedge1->pVerts[0] == pedge2->pVerts[1]) {
                            pnewEdgeVert1 = pedge1->pVerts[1];
                            pnewEdgeVert2 = pedge2->pVerts[0];
                        }
                        else {
                            if (pedge1->pVerts[1] == pedge2->pVerts[0]) {
                                pnewEdgeVert1 = pedge1->pVerts[0];
                                pnewEdgeVert2 = pedge2->pVerts[1];
                            }
                            else {
                                if (pedge1->pVerts[1] != pedge2->pVerts[1]) {
                                    continue;
                                }
                                pnewEdgeVert1 = pedge1->pVerts[0];
                                pnewEdgeVert2 = pedge2->pVerts[0];
                            }
                        }
                    }
                    if (CheckPotentialEdgeAgainstRunningEdgeList(pnewEdgeVert1, pnewEdgeVert2)) {
                        NavMeshObstacleEdge* pnewedge = CreateEdge(pnewEdgeVert1, pnewEdgeVert2, EDGE_TYPE_TWOSIDED);
                        Edges.Add(pnewedge);
                    }
                }
            }
        }
    }
}

void NavMeshObstacleCutter::AddNewPOVs() {
    for (NavMeshObstacleVertex* pVertex : Vertices) {
        if (pVertex->bOnObstacle) {
            pNavMesh->ObstaclePOVs.Add(pVertex->usVertexIndex);
            pUndoData->AddedPOVs.Add(pVertex->usVertexIndex);
        }
    }
}

void NavMeshObstacleCutter::SetObstacleCount() {
    for (uint32_t i = pUndoData->iFirstAddedTriangleIndex;
         i < pUndoData->iAddedTriangleCount + pUndoData->iFirstAddedTriangleIndex; ++i) {
        uint32_t v3 = iMaxObstacleCount + 1;
        NavMeshTriangle* Triangle = pNavMesh->GetTriangle(i);
        Triangle->SetObjectCount(v3);
    }
    for (const BoundsTriangleInfo& rBoundsInfo : pUndoData->BoundsTriangles) {
        NavMeshTriangle* v5 = pNavMesh->GetTriangle(rBoundsInfo.usTriangleIndex);
        uint16_t ObjectCount = v5->GetObjectCount();
        v5->SetObjectCount(ObjectCount + 1);
    }
}

void NavMeshObstacleCutter::ConnectTriangles() {
    for (NavMeshObstacleEdge* pEdge : Edges) {
        if (pEdge->Triangles[0] != 0xFFFF && pEdge->Triangles[1] != 0xFFFF) {
            uint16_t v5 = pEdge->Triangles[1];
            uint32_t v3 = pEdge->TriangleEdges[0];
            NavMeshTriangle* Triangle = pNavMesh->GetTriangle(pEdge->Triangles[0]);
            Triangle->SetTri(v3, v5);
            uint16_t v6 = pEdge->Triangles[0];
            uint32_t v4 = pEdge->TriangleEdges[1];
            NavMeshTriangle* v2 = pNavMesh->GetTriangle(pEdge->Triangles[1]);
            v2->SetTri(v4, v6);
        }
    }
}

void NavMeshObstacleCutter::CreateSwappedEdgeList(TaskPortalSwap& aPortalSwaps) {
    PortalEdgeSwap swapEdge;
    // The indices into aPortalSwaps.PotentialChildren of the children to remove.
    using childSwapsToRemove_t = BSSimpleArray<uint32_t, 1024>;
    childSwapsToRemove_t childSwapsToRemove;
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        uint32_t uiEdgeCount = Edges.iSize;
        if (uiIndex >= uiEdgeCount) {
            break;
        }
        if (Edges.GetAt(static_cast<int32_t>(uiIndex))->EdgeType == EDGE_TYPE_PORTAL &&
            Edges.GetAt(static_cast<int32_t>(uiIndex))->Triangles[0] != 0xFFFF) {
            NavMeshEdgeHandle hNewEdge;
            hNewEdge.pNavMeshInfo = nullptr;
            hNewEdge.usTriangle = Edges.GetAt(static_cast<int32_t>(uiIndex))->Triangles[0];
            hNewEdge.iEdgeIndex = Edges.GetAt(static_cast<int32_t>(uiIndex))->TriangleEdges[0];
            swapEdge.~PortalEdgeSwap();
            ::new (&swapEdge) PortalEdgeSwap;
            swapEdge.hNewEdge.Copy(hNewEdge);
            swapEdge.hOldEdge.Copy(Edges.GetAt(static_cast<int32_t>(uiIndex))->hOriginalEdge);
            swapEdge.bFromAddition = true;
            pUndoData->EdgeSwap.Add(swapEdge);
            uint32_t uiChildIndex = aPortalSwaps.PotentialChildren.Find(swapEdge.hOldEdge, 0, &EdgeHandleEqual);
            if (static_cast<int32_t>(uiChildIndex) == -1) {
                if (aPortalSwaps.PotentialParents.Find(swapEdge.hOldEdge, 0, &EdgeHandleEqual) == -1) {
                    aPortalSwaps.PotentialParents.Add(swapEdge.hOldEdge);
                }
            }
            else {
                if (childSwapsToRemove.Find(uiChildIndex, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) ==
                    -1) {
                    childSwapsToRemove.Add(uiChildIndex);
                }
            }
            if (aPortalSwaps.PotentialChildren.Find(swapEdge.hNewEdge, 0, &EdgeHandleEqual) == -1) {
                aPortalSwaps.PotentialChildren.Add(swapEdge.hNewEdge);
            }
        }
    }
    if (!childSwapsToRemove.IsEmpty()) {
        childSwapsToRemove.Sort([](const void* apOne, const void* apTwo) {
            return CompareUInt(static_cast<const uint32_t*>(apOne), static_cast<const uint32_t*>(apTwo));
        });
        for (int32_t i = childSwapsToRemove.iSize - 1; i >= 0; --i) {
            aPortalSwaps.PotentialChildren.Remove(childSwapsToRemove.GetAt(i), true);
        }
    }
}

bool NavMeshObstacleCutter::IsVertInObstacleArea(NavMeshObstacleVertex* apVert) const {
    NavMeshVertex* Vertex = pNavMesh->GetVertex(apVert->usVertexIndex);
    return IsPointInObstacleArea(Vertex->Location);
}

void NavMeshObstacleCutter::UpdateCoverArray() {
    pNavMesh->CreateCoverArray();
}

// Closes triangles out of three usable edges that pairwise share a vertex: the first two edges meet at pShared, the
// third joins their other ends. The triangle must lie outside the obstacle and in the overlapped area, and its doubled
// area (the cross product's length squared) must reach 0.1. Its winding follows the normal's sign (a NaN z takes the
// upward branch); each edge gives its cover to the triangle edge it becomes and, for edge type 4, its original extra
// edge info. Once the first edge is used up the search moves on to the next first edge.
bool NavMeshObstacleCutter::CreateNewTriangles() {
    bool bCreated = false;
    for (uint32_t i = 0; i < Edges.GetSize(); ++i) {
        NavMeshObstacleEdge* pEdge1 = Edges.GetAt(i);
        if (!pEdge1->CanBeUsed()) {
            continue;
        }
        for (uint32_t j = i + 1; j < Edges.GetSize(); ++j) {
            NavMeshObstacleEdge* pEdge2 = Edges.GetAt(j);
            if (!pEdge2->CanBeUsed()) {
                continue;
            }
            if (pEdge1->Triangles[0] != 0xFFFF && pEdge1->Triangles[0] == pEdge2->Triangles[0]) {
                continue;
            }
            NavMeshObstacleVertex* pShared;
            NavMeshObstacleVertex* pEnd1;
            NavMeshObstacleVertex* pEnd2;
            if (pEdge1->pVerts[0] == pEdge2->pVerts[0]) {
                pShared = pEdge1->pVerts[0];
                pEnd1 = pEdge1->pVerts[1];
                pEnd2 = pEdge2->pVerts[1];
            }
            else if (pEdge1->pVerts[0] == pEdge2->pVerts[1]) {
                pShared = pEdge1->pVerts[0];
                pEnd1 = pEdge1->pVerts[1];
                pEnd2 = pEdge2->pVerts[0];
            }
            else if (pEdge1->pVerts[1] == pEdge2->pVerts[0]) {
                pShared = pEdge1->pVerts[1];
                pEnd1 = pEdge1->pVerts[0];
                pEnd2 = pEdge2->pVerts[1];
            }
            else if (pEdge1->pVerts[1] == pEdge2->pVerts[1]) {
                pShared = pEdge1->pVerts[1];
                pEnd1 = pEdge1->pVerts[0];
                pEnd2 = pEdge2->pVerts[0];
            }
            else {
                continue;
            }
            bool bEdge1UsedUp = false;
            for (uint32_t k = j + 1; k < Edges.GetSize(); ++k) {
                NavMeshObstacleEdge* pEdge3 = Edges.GetAt(k);
                if (!pEdge3->CanBeUsed()) {
                    continue;
                }
                if (!((pEdge3->pVerts[0] == pEnd1 && pEdge3->pVerts[1] == pEnd2) ||
                      (pEdge3->pVerts[0] == pEnd2 && pEdge3->pVerts[1] == pEnd1))) {
                    continue;
                }
                if (!CheckTriangleAgainstObstacle(pShared, pEnd1, pEnd2)) {
                    continue;
                }
                if (!CheckTriangleAgainstOverlapped(pShared, pEnd1, pEnd2)) {
                    continue;
                }
                // The shared vertex is fetched again before each end vertex.
                NavMeshVertex* pSharedVertex = pNavMesh->GetVertex(pShared->usVertexIndex);
                NiPoint3 kSide1 = pNavMesh->GetVertex(pEnd1->usVertexIndex)->Location - pSharedVertex->Location;
                NavMeshVertex* pSharedVertex2 = pNavMesh->GetVertex(pShared->usVertexIndex);
                NiPoint3 kSide2 = pNavMesh->GetVertex(pEnd2->usVertexIndex)->Location - pSharedVertex2->Location;
                NiPoint3 kNormal = kSide1.Cross(kSide2);
                if (kNormal.SqrLength() < 0.10000000149011612) {
                    break;
                }
                NavMeshTriangle kTriangle;
                kTriangle.~NavMeshTriangle();
                ::new (&kTriangle) NavMeshTriangle;
                if (kNormal.z < 0.0) {
                    kTriangle.SetVerts(pShared->usVertexIndex, pEnd2->usVertexIndex, pEnd1->usVertexIndex);
                    kTriangle.SetEdgeCover(0, pEdge2->coverData, pEdge2->bLeftSide, pEdge2->bRightSide);
                    kTriangle.SetEdgeCover(1, pEdge3->coverData, pEdge3->bLeftSide, pEdge3->bRightSide);
                    if (pEdge1->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(2, pEdge1->hOriginalEdgeExtraInfo);
                    }
                    if (pEdge2->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(0, pEdge2->hOriginalEdgeExtraInfo);
                    }
                    if (pEdge3->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(1, pEdge3->hOriginalEdgeExtraInfo);
                    }
                }
                else {
                    kTriangle.SetVerts(pShared->usVertexIndex, pEnd1->usVertexIndex, pEnd2->usVertexIndex);
                    kTriangle.SetEdgeCover(0, pEdge1->coverData, pEdge1->bLeftSide, pEdge1->bRightSide);
                    kTriangle.SetEdgeCover(1, pEdge3->coverData, pEdge3->bLeftSide, pEdge3->bRightSide);
                    if (pEdge1->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(0, pEdge1->hOriginalEdgeExtraInfo);
                    }
                    if (pEdge2->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(2, pEdge2->hOriginalEdgeExtraInfo);
                    }
                    if (pEdge3->EdgeType == 4) {
                        kTriangle.SetEdgeExtraInfo(1, pEdge3->hOriginalEdgeExtraInfo);
                    }
                }
                uint16_t usTriangle = pNavMesh->AddTriangle(kTriangle);
                bCreated = true;
                if (kNormal.z < 0.0) {
                    pEdge1->AddTriangle(usTriangle, 2);
                    pEdge2->AddTriangle(usTriangle, 0);
                    pEdge3->AddTriangle(usTriangle, 1);
                }
                else {
                    pEdge1->AddTriangle(usTriangle, 0);
                    pEdge2->AddTriangle(usTriangle, 2);
                    pEdge3->AddTriangle(usTriangle, 1);
                }
                if (!pEdge1->CanBeUsed()) {
                    bEdge1UsedUp = true;
                }
                break;
            }
            if (bEdge1UsedUp) {
                break;
            }
        }
    }
    return bCreated;
}

bool NavMeshObstacleCutter::UndoNavMeshFromObstacleUndoData(
    NavMesh* apNavMesh,
    ObstacleUndoData* apUndoData,
    TaskPortalSwap& aPortalSwaps
) {
    PortalEdgeSwap reverseSwap;
    pNavMesh = apNavMesh;
    pUndoData = apUndoData;
    // The indices into aPortalSwaps.PotentialChildren of the children the undone swaps leave stale.
    using childSwapsToRemove_t = BSSimpleArray<uint32_t, 1024>;
    childSwapsToRemove_t childSwapsToRemove;
    for (const PortalEdgeSwap& rSwap : pUndoData->EdgeSwap) {
        // The swap the other way round: the old edge becomes the new one and the new one the old.
        reverseSwap.~PortalEdgeSwap();
        ::new (&reverseSwap) PortalEdgeSwap;
        reverseSwap.hNewEdge.Copy(rSwap.hOldEdge);
        reverseSwap.hOldEdge.Copy(rSwap.hNewEdge);
        reverseSwap.bFromAddition = false;
        uint32_t index =
            static_cast<uint32_t>(aPortalSwaps.PotentialChildren.Find(reverseSwap.hOldEdge, 0, &EdgeHandleEqual));
        if (index == 0xFFFFFFFF) {
            if (aPortalSwaps.PotentialParents.Find(reverseSwap.hOldEdge, 0, &EdgeHandleEqual) == -1) {
                aPortalSwaps.PotentialParents.Add(reverseSwap.hOldEdge);
            }
        }
        else if (childSwapsToRemove.Find(index, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) == -1) {
            childSwapsToRemove.Add(index);
        }
        if (aPortalSwaps.PotentialChildren.Find(reverseSwap.hNewEdge, 0, &EdgeHandleEqual) == -1) {
            aPortalSwaps.PotentialChildren.Add(reverseSwap.hNewEdge);
        }
    }
    if (!childSwapsToRemove.IsEmpty()) {
        // Removed from the highest index down, so that the lower indices stay valid.
        childSwapsToRemove.Sort([](const void* apOne, const void* apTwo) {
            return CompareUInt(static_cast<const uint32_t*>(apOne), static_cast<const uint32_t*>(apTwo));
        });
        for (int32_t i_2 = static_cast<int32_t>(childSwapsToRemove.iSize) - 1; i_2 >= 0; --i_2) {
            aPortalSwaps.PotentialChildren.Remove(childSwapsToRemove.GetAt(i_2), true);
        }
    }
    for (uint16_t usTriangle : pUndoData->OverlappedTriangles) {
        NavMeshTriangle* ptri = pNavMesh->GetTriangle(usTriangle);
        ptri->ClearFlag(32);
    }
    for (const BoundsTriangleInfo& rBoundsInfo : pUndoData->BoundsTriangles) {
        NavMeshTriangle* ptri_2 = pNavMesh->GetTriangle(rBoundsInfo.usTriangleIndex);
        ptri_2->ClearFlag(256);
        for (int32_t e = 0; e < 3; ++e) {
            if (rBoundsInfo.bPortals[e]) {
                ptri_2->SetEdgeExtraInfo(e, rBoundsInfo.Triangles[e]);
            }
            else {
                ptri_2->SetTri(static_cast<uint32_t>(e), rBoundsInfo.Triangles[e]);
            }
        }
    }
    for (uint32_t i_5 = pUndoData->iFirstAddedTriangleIndex;
         i_5 < pUndoData->iFirstAddedTriangleIndex + pUndoData->iAddedTriangleCount; ++i_5) {
        // Passes the index's low 16 bits.
        NavMeshTriangle* pTriangle = pNavMesh->GetTriangle(static_cast<uint16_t>(i_5));
        pTriangle->SetFlag(32);
    }
    for (const BoundsTriangleInfo& rBoundsInfo : pUndoData->BoundsTriangles) {
        NavMeshTriangle* ptri_3 = pNavMesh->GetTriangle(rBoundsInfo.usTriangleIndex);
        uint16_t usObjectCount = ptri_3->GetObjectCount();
        // Passes count - 1 unmasked.
        ptri_3->SetObjectCount(usObjectCount - 1);
    }
    for (uint32_t i_7 = pUndoData->iFirstAddedTriangleIndex;
         i_7 < pUndoData->iFirstAddedTriangleIndex + pUndoData->iAddedTriangleCount; ++i_7) {
        pNavMesh->pTriangleToObstacleMap->RemoveAt(static_cast<uint16_t>(i_7));
    }
    NiTMapItem<uint16_t, NiPointer<ObstacleData>>* it = pUndoData->RemovedTriangleToObstacleMap.GetFirstPos();
    while (it) {
        uint16_t ustriangle;
        NiPointer<ObstacleData> sppreviousObstacle(static_cast<ObstacleData*>(nullptr));
        pUndoData->RemovedTriangleToObstacleMap.GetNext(it, ustriangle, sppreviousObstacle);
        // The by-value argument is copied by NiPointer's copy constructor.
        pNavMesh->pTriangleToObstacleMap->SetAt(ustriangle, sppreviousObstacle);
    }
    UpdateCoverArray();
    RemovePOVs();
    pUndoData->eState = ObstacleUndoData::OBSTACLE_UNDO_STATE_UNDONE;
    return true;
}

void NavMeshObstacleCutter::RemovePOVs() {
    for (uint16_t usPOV : pUndoData->AddedPOVs) {
        uint32_t iindex = pNavMesh->ObstaclePOVs.Find(usPOV, 0, return_equals);
        if (iindex != -1) {
            pNavMesh->ObstaclePOVs.RemoveFast(iindex, 1u);
        }
    }
}

uint32_t NavMeshObstacleCutter::FindVert(uint16_t a2) {
    for (uint32_t i = 0; i < Vertices.iSize; ++i) {
        if (Vertices.GetAt(i)->usVertexIndex == a2) {
            return i;
        }
    }
    return -1;
}

bool NavMeshObstacleCutter::IsEdgeInObstacle(NavMeshObstacleEdge* apEdge) {
    if (!apEdge->pVerts[0]->bOnObstacle && apEdge->pVerts[0]->bInObstacleArea) {
        return true;
    }
    if (!apEdge->pVerts[1]->bOnObstacle && apEdge->pVerts[1]->bInObstacleArea) {
        return true;
    }
    return apEdge->pVerts[0]->bOnObstacle && (apEdge->pVerts[1]->bOnObstacle && apEdge->EdgeType != 1);
}

NavMeshObstacleVertex*
NavMeshObstacleCutter::CreateVertex(uint16_t aiIndex, bool abOnObstacle, bool abOnBounds, bool abOnOpenEdge) {
    NavMeshObstacleVertex* apWhere = static_cast<NavMeshObstacleVertex*>(
        LocalScrapHeap.Allocate(sizeof(NavMeshObstacleVertex), ScrapHeap::ALIGNMENT)
    );
    NavMeshObstacleVertex* apMemory = static_cast<NavMeshObstacleVertex*>(::operator new(8u, apWhere));
    if (apMemory) {
        ::new (apMemory) NavMeshObstacleVertex(aiIndex, abOnObstacle, abOnBounds, abOnOpenEdge);
    }
    apWhere->bInOverlappedArea = IsVertInOverlappedArea(apWhere);
    return apWhere;
}

bool NavMeshEdgeHandle::operator==(const NavMeshEdgeHandle& aNavMeshEdgeHandle) const {
    return pNavMeshInfo == aNavMeshEdgeHandle.pNavMeshInfo &&
        (usTriangle == aNavMeshEdgeHandle.usTriangle && iEdgeIndex == aNavMeshEdgeHandle.iEdgeIndex);
}

NavMeshObstacleEdge::NavMeshObstacleEdge(
    NavMeshObstacleVertex* apVert0,
    NavMeshObstacleVertex* apVert1,
    uint32_t aeEdgeType
)
  : pVerts{apVert0, apVert1}, Triangles{0xFFFF, 0xFFFF}, TriangleEdges{0xFFFF, 0xFFFF},
    EdgeType(static_cast<EDGE_TYPE>(aeEdgeType)), EdgeExtraInfoIndex(0xFFFF), coverData(0), bLeftSide(false),
    bRightSide(false), spObstacle(nullptr), hOriginalEdgeExtraInfo(0xFFFF), bFromSplit(false) {
    hOriginalEdge.pNavMeshInfo = nullptr;
    hOriginalEdge.usTriangle = 0xFFFF;
    hOriginalEdge.iEdgeIndex = -1;
}

bool NavMeshObstacleEdge::CanBeUsed() const {
    if (Triangles[0] == 65535) {
        return true;
    }
    if (Triangles[1] != 65535) {
        return false;
    }
    return (bool)((uint8_t)((EdgeType != 1) && ((EdgeType != 3) && (EdgeType != 4))));
}

void NavMeshObstacleEdge::AddTriangle(uint16_t ausTri, uint16_t ausEdge) {
    if (Triangles[0u] == 0xFFFF) {
        Triangles[0u] = ausTri;
        TriangleEdges[0u] = ausEdge;
    }
    else if (Triangles[1] == 0xFFFF) {
        Triangles[1] = ausTri;
        TriangleEdges[1] = ausEdge;
    }
}

void NavMeshTriangle::SetVerts(uint16_t i0, uint16_t i1, uint16_t i2) {
    Vertices[0] = i0;
    Vertices[1] = i1;
    Vertices[2] = i2;
}
