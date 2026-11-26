#include "Fallout Shared/Pathfinding/PathSmoother.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/ThreadSafeStructures.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

#include <cmath>
#include <new>

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bDrawPathBounds("bDrawPathBounds:Pathfinding", false);

bool PathSmoother::AvoidNode::IsPointInNode(const NiPoint3& aPoint) const {
    NiPoint3 kDelta = aPoint - Center;
    return fRadius * fRadius > kDelta.SqrLength();
}

PathSmoother::UniqueVert::UniqueVert(ScrapHeap* apHeap) : EdgeList(0, 0, apHeap) {}

PathSmoother::UniqueVert::~UniqueVert() {}

void PathSmoother::UniqueVert::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathSmoother::BoundsEdge::BoundsEdge() {}

PathSmoother::PathEdge::PathEdge() {}

PathSmoother::PathSmoother()
  : fSmoothingRadius(32.0f), LeftVerts(0, 0, &TheScrapHeap), RightVerts(0, 0, &TheScrapHeap),
    LeftEdges(0, 0, &TheScrapHeap), RightEdges(0, 0, &TheScrapHeap), PathEdges(0, 0, &TheScrapHeap) {}

// The vertices and edges live in TheScrapHeap: each vertex is destroyed in place and handed back, as are the edges. The
// first two right vertices are dropped before the rest are destroyed.
PathSmoother::~PathSmoother() {
    for (UniqueVert* pVert : LeftVerts) {
        pVert->~UniqueVert();
        TheScrapHeap.Deallocate(pVert);
    }
    if (!RightVerts.IsEmpty()) {
        RightVerts.RemoveFast(0, 1u);
    }
    if (!RightVerts.IsEmpty()) {
        RightVerts.RemoveFast(0, 1u);
    }
    for (UniqueVert* pVert : RightVerts) {
        pVert->~UniqueVert();
        TheScrapHeap.Deallocate(pVert);
    }
    LeftVerts.Clear(true);
    RightVerts.Clear(true);
    for (BoundsEdge* pEdge : LeftEdges) {
        TheScrapHeap.Deallocate(pEdge);
    }
    for (BoundsEdge* pEdge : RightEdges) {
        TheScrapHeap.Deallocate(pEdge);
    }
    LeftEdges.Clear(true);
    RightEdges.Clear(true);
    for (PathEdge* pEdge : PathEdges) {
        TheScrapHeap.Deallocate(pEdge);
    }
    PathEdges.Clear(true);
}

void PathSmoother::AddPathingNode(const PathingLocation& aLocation) {
    pSolution->AddPathingNode(aLocation, NiPoint3::ZERO);
}

void PathSmoother::BuildInternalArrays() {
    AddStartVert();
    for (uint32_t i = 0; i < Edges->GetSize(); ++i) {
        NiPoint3 kVert0;
        NiPoint3 kVert1;
        // Calls operator[] once per field it reads, before the call itself.
        (*Edges)[i].pNavMesh->GetEdgeVertices((*Edges)[i].sTriangle, (*Edges)[i].iEdgeIndex, &kVert0, &kVert1);
        if (i == 0) {
            NiPoint3 kToStart = kVert0 - StartLocation;
            NiPoint3 kToEdge = kVert0 - kVert1;
            NiPoint3 kCross = kToEdge.Cross(kToStart);
            if (kCross.z > 0.0) {
                AddFirstEdge(kVert1, kVert0);
            }
            else {
                AddFirstEdge(kVert0, kVert1);
            }
            continue;
        }
        // GetTail gives the last item's address; the item is the vertex pointer itself.
        if (kVert0 == (*static_cast<UniqueVert**>(LeftVerts.GetTail()))->Location) {
            AddRightVert(kVert1, i);
            continue;
        }
        if (kVert0 == (*static_cast<UniqueVert**>(RightVerts.GetTail()))->Location) {
            AddLeftVert(kVert1, i);
            continue;
        }
        if (kVert1 == (*static_cast<UniqueVert**>(LeftVerts.GetTail()))->Location) {
            AddRightVert(kVert0, i);
            continue;
        }
        if (kVert1 == (*static_cast<UniqueVert**>(RightVerts.GetTail()))->Location) {
            AddLeftVert(kVert0, i);
            continue;
        }
        NavMesh* pReturnNavMesh = nullptr;
        uint16_t usOtherTriangle = 0xFFFF;
        int32_t iOtherEdgeIndex = -1;
        (*Triangles)[i - 1].pNavMesh->GetMatchingEdge(
            (*Triangles)[i - 1].sTriangle,
            (*Edges)[i - 1].iEdgeIndex,
            &pReturnNavMesh,
            &usOtherTriangle,
            &iOtherEdgeIndex
        );
        NiPoint3 kEdgeVert0;
        NiPoint3 kEdgeVert1;
        pReturnNavMesh->GetEdgeVertices(usOtherTriangle, iOtherEdgeIndex, &kEdgeVert0, &kEdgeVert1);
        NiPoint3 kFirst;
        NiPoint3 kSecond;
        if ((*static_cast<UniqueVert**>(RightVerts.GetTail()))->Location == kEdgeVert0) {
            kFirst = kEdgeVert1;
            kSecond = kEdgeVert0;
        }
        else {
            kFirst = kEdgeVert0;
            kSecond = kEdgeVert1;
        }
        if (kVert0 == kFirst) {
            AddRightVert(kVert1, i);
        }
        else if (kVert0 == kSecond) {
            AddLeftVert(kVert1, i);
        }
        else if (kVert1 == kFirst) {
            AddRightVert(kVert0, i);
        }
        else if (kVert1 == kSecond) {
            AddLeftVert(kVert0, i);
        }
    }
    AddGoalVert();
}

void PathSmoother::AddStartVert() {
    UniqueVert* pVert = CreateVert();
    const NavMeshSearchNode& kTri = Triangles->GetFirst();
    pVert->hNavMeshTri.pNavMeshInfo = kTri.pNavMesh->GetNavMeshInfo();
    pVert->hNavMeshTri.usTriangle = kTri.sTriangle;
    pVert->Location = StartLocation;
    LeftVerts.Add(pVert);
    RightVerts.Add(pVert);
}

void PathSmoother::AddFirstEdge(const NiPoint3& aLeftLocation, const NiPoint3& aRightLocation) {
    UniqueVert* pLeftVert = CreateVert();
    const NavMeshSearchLink& kFirstEdge = Edges->GetFirst();
    pLeftVert->hNavMeshTri.pNavMeshInfo = kFirstEdge.pNavMesh->GetNavMeshInfo();
    pLeftVert->hNavMeshTri.usTriangle = kFirstEdge.sTriangle;
    pLeftVert->Location = aLeftLocation;
    UniqueVert* pRightVert = CreateVert();
    pRightVert->hNavMeshTri.pNavMeshInfo = kFirstEdge.pNavMesh->GetNavMeshInfo();
    pRightVert->hNavMeshTri.usTriangle = kFirstEdge.sTriangle;
    pRightVert->Location = aRightLocation;
    BoundsEdge* pLeftEdge = CreateBoundsEdge();
    pLeftEdge->pStartVert = *static_cast<UniqueVert**>(LeftVerts.GetTail());
    pLeftEdge->pEndVert = pLeftVert;
    pLeftEdge->hNavMeshEdge.pNavMeshInfo = kFirstEdge.pNavMesh->GetNavMeshInfo();
    pLeftEdge->hNavMeshEdge.usTriangle = kFirstEdge.sTriangle;
    pLeftEdge->hNavMeshEdge.iEdgeIndex = -1;
    pLeftEdge->iEdgeIndex = 0;
    pLeftEdge->bOpenEdge = true;
    BoundsEdge* pRightEdge = CreateBoundsEdge();
    pRightEdge->pStartVert = *static_cast<UniqueVert**>(RightVerts.GetTail());
    pRightEdge->pEndVert = pRightVert;
    pRightEdge->hNavMeshEdge.pNavMeshInfo = kFirstEdge.pNavMesh->GetNavMeshInfo();
    pRightEdge->hNavMeshEdge.usTriangle = kFirstEdge.sTriangle;
    pRightEdge->hNavMeshEdge.iEdgeIndex = -1;
    pRightEdge->iEdgeIndex = 0;
    pRightEdge->bOpenEdge = true;
    PathEdge* pPathEdge = static_cast<PathEdge*>(CreatePathEdge());
    pPathEdge->pLeftVert = pLeftVert;
    pPathEdge->pRightVert = pRightVert;
    pPathEdge->hNavMeshEdge.pNavMeshInfo = kFirstEdge.pNavMesh->GetNavMeshInfo();
    pPathEdge->hNavMeshEdge.usTriangle = kFirstEdge.sTriangle;
    pPathEdge->hNavMeshEdge.iEdgeIndex = kFirstEdge.iEdgeIndex;
    pPathEdge->iPathEdgeIndex = 0;
    pPathEdge->pPrevLeftEdge = pLeftEdge;
    pPathEdge->pPrevRightEdge = pRightEdge;
    pLeftVert->EdgeList.Add(pPathEdge);
    pRightVert->EdgeList.Add(pPathEdge);
    NiPoint3 kLeftNormal = (*Triangles)[0].pNavMesh->GetNormal((*Triangles)[0].sTriangle);
    NiPoint3 kRightNormal = (*Triangles)[1].pNavMesh->GetNormal((*Triangles)[1].sTriangle);
    pPathEdge->fTriangleNormalDelta = kLeftNormal.Dot(kRightNormal);
    LeftVerts.Add(pLeftVert);
    RightVerts.Add(pRightVert);
    LeftEdges.Add(pLeftEdge);
    RightEdges.Add(pRightEdge);
    PathEdges.Add(pPathEdge);
}

void PathSmoother::AddLeftVert(const NiPoint3& aLocation, uint32_t aiEdgeIndex) {
    UniqueVert* pVert = CreateVert();
    const NavMeshSearchLink& kEdge = (*Edges)[aiEdgeIndex];
    pVert->hNavMeshTri.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pVert->hNavMeshTri.usTriangle = kEdge.sTriangle;
    pVert->Location = aLocation;
    BoundsEdge* pBoundsEdge = CreateBoundsEdge();
    pBoundsEdge->pStartVert = *static_cast<UniqueVert**>(LeftVerts.GetTail());
    pBoundsEdge->pEndVert = pVert;
    pBoundsEdge->hNavMeshEdge.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pBoundsEdge->hNavMeshEdge.usTriangle = kEdge.sTriangle;
    pBoundsEdge->hNavMeshEdge.iEdgeIndex = (kEdge.iEdgeIndex + 1) % 3;
    pBoundsEdge->bOpenEdge = kEdge.pNavMesh->GetTriangle(kEdge.sTriangle)
                                 ->IsEdgeConnected(pBoundsEdge->hNavMeshEdge.iEdgeIndex);
    pBoundsEdge->iEdgeIndex = LeftEdges.GetSize();
    PathEdge* pPathEdge = static_cast<PathEdge*>(CreatePathEdge());
    pPathEdge->pLeftVert = pVert;
    pPathEdge->pRightVert = *static_cast<UniqueVert**>(RightVerts.GetTail());
    pPathEdge->hNavMeshEdge.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pPathEdge->hNavMeshEdge.usTriangle = kEdge.sTriangle;
    pPathEdge->hNavMeshEdge.iEdgeIndex = kEdge.iEdgeIndex;
    pPathEdge->iPathEdgeIndex = PathEdges.GetSize();
    pPathEdge->pPrevLeftEdge = pBoundsEdge;
    pPathEdge->pPrevRightEdge = *static_cast<BoundsEdge**>(RightEdges.GetTail());
    pVert->EdgeList.Add(pPathEdge);
    (*static_cast<UniqueVert**>(RightVerts.GetTail()))->EdgeList.Add(pPathEdge);
    NiPoint3 kLeftNormal = (*Triangles)[aiEdgeIndex].pNavMesh->GetNormal((*Triangles)[aiEdgeIndex].sTriangle);
    NiPoint3 kRightNormal = (*Triangles)[aiEdgeIndex + 1].pNavMesh->GetNormal((*Triangles)[aiEdgeIndex + 1].sTriangle);
    pPathEdge->fTriangleNormalDelta = kLeftNormal.Dot(kRightNormal);
    LeftVerts.Add(pVert);
    LeftEdges.Add(pBoundsEdge);
    PathEdges.Add(pPathEdge);
}

void PathSmoother::AddRightVert(const NiPoint3& aLocation, uint32_t aiEdgeIndex) {
    UniqueVert* pVert = CreateVert();
    const NavMeshSearchLink& kEdge = (*Edges)[aiEdgeIndex];
    pVert->hNavMeshTri.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pVert->hNavMeshTri.usTriangle = kEdge.sTriangle;
    pVert->Location = aLocation;
    BoundsEdge* pBoundsEdge = CreateBoundsEdge();
    pBoundsEdge->pStartVert = *static_cast<UniqueVert**>(RightVerts.GetTail());
    pBoundsEdge->pEndVert = pVert;
    pBoundsEdge->hNavMeshEdge.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pBoundsEdge->hNavMeshEdge.usTriangle = kEdge.sTriangle;
    pBoundsEdge->hNavMeshEdge.iEdgeIndex = (kEdge.iEdgeIndex + 2) % 3;
    pBoundsEdge->bOpenEdge = kEdge.pNavMesh->GetTriangle(kEdge.sTriangle)
                                 ->IsEdgeConnected(pBoundsEdge->hNavMeshEdge.iEdgeIndex);
    pBoundsEdge->iEdgeIndex = RightEdges.GetSize();
    PathEdge* pPathEdge = static_cast<PathEdge*>(CreatePathEdge());
    pPathEdge->pLeftVert = *static_cast<UniqueVert**>(LeftVerts.GetTail());
    pPathEdge->pRightVert = pVert;
    pPathEdge->hNavMeshEdge.pNavMeshInfo = kEdge.pNavMesh->GetNavMeshInfo();
    pPathEdge->hNavMeshEdge.usTriangle = kEdge.sTriangle;
    pPathEdge->hNavMeshEdge.iEdgeIndex = kEdge.iEdgeIndex;
    pPathEdge->iPathEdgeIndex = PathEdges.GetSize();
    pPathEdge->pPrevLeftEdge = *static_cast<BoundsEdge**>(LeftEdges.GetTail());
    pPathEdge->pPrevRightEdge = pBoundsEdge;
    pVert->EdgeList.Add(pPathEdge);
    (*static_cast<UniqueVert**>(LeftVerts.GetTail()))->EdgeList.Add(pPathEdge);
    NiPoint3 kLeftNormal = (*Triangles)[aiEdgeIndex].pNavMesh->GetNormal((*Triangles)[aiEdgeIndex].sTriangle);
    NiPoint3 kRightNormal = (*Triangles)[aiEdgeIndex + 1].pNavMesh->GetNormal((*Triangles)[aiEdgeIndex + 1].sTriangle);
    pPathEdge->fTriangleNormalDelta = kLeftNormal.Dot(kRightNormal);
    RightVerts.Add(pVert);
    RightEdges.Add(pBoundsEdge);
    PathEdges.Add(pPathEdge);
}

void PathSmoother::AddGoalVert() {
    UniqueVert* pVert = CreateVert();
    const NavMeshSearchNode& kTri = Triangles->GetLast();
    pVert->hNavMeshTri.pNavMeshInfo = kTri.pNavMesh->GetNavMeshInfo();
    pVert->hNavMeshTri.usTriangle = kTri.sTriangle;
    pVert->Location = GoalLocation;
    BoundsEdge* pLeftEdge = CreateBoundsEdge();
    pLeftEdge->pStartVert = *static_cast<UniqueVert**>(LeftVerts.GetTail());
    pLeftEdge->pEndVert = pVert;
    pLeftEdge->bOpenEdge = true;
    pLeftEdge->hNavMeshEdge.pNavMeshInfo = kTri.pNavMesh->GetNavMeshInfo();
    pLeftEdge->hNavMeshEdge.usTriangle = kTri.sTriangle;
    pLeftEdge->hNavMeshEdge.iEdgeIndex = -1;
    pLeftEdge->iEdgeIndex = LeftEdges.GetSize();
    BoundsEdge* pRightEdge = CreateBoundsEdge();
    pRightEdge->pStartVert = *static_cast<UniqueVert**>(RightVerts.GetTail());
    pRightEdge->pEndVert = pVert;
    pRightEdge->bOpenEdge = true;
    pRightEdge->hNavMeshEdge.pNavMeshInfo = kTri.pNavMesh->GetNavMeshInfo();
    pRightEdge->hNavMeshEdge.usTriangle = kTri.sTriangle;
    pRightEdge->hNavMeshEdge.iEdgeIndex = -1;
    pRightEdge->iEdgeIndex = RightEdges.GetSize();
    LeftVerts.Add(pVert);
    RightVerts.Add(pVert);
    LeftEdges.Add(pLeftEdge);
    RightEdges.Add(pRightEdge);
}

PathSmoother::UniqueVert* PathSmoother::CreateVert() {
    void* pMemory = TheScrapHeap.Allocate(sizeof(PathSmoother::UniqueVert), ScrapHeap::ALIGNMENT);
    return new (pMemory) PathSmoother::UniqueVert(&TheScrapHeap);
}

PathSmoother::BoundsEdge* PathSmoother::CreateBoundsEdge() {
    void* pMemory = TheScrapHeap.Allocate(sizeof(PathSmoother::BoundsEdge), ScrapHeap::ALIGNMENT);
    return new (pMemory) PathSmoother::BoundsEdge;
}

PathSmoother::PathEdge* PathSmoother::CreatePathEdge() {
    void* pMemory = TheScrapHeap.Allocate(sizeof(PathEdge), ScrapHeap::ALIGNMENT);
    return new (pMemory) PathSmoother::PathEdge;
}

bool PathSmoother::SmoothPath(
    PathingRequest* apRequest,
    const BSSimpleArray<NavMeshSearchNode, 1024>& aTriSolution,
    const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks,
    const NiPoint3& aStartLocation,
    const NiPoint3& aGoalLocation,
    PathingSolution* apSolution,
    BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray,
    bool abAddNodesIfFail
) {
    pRequest = apRequest;
    StartLocation = aStartLocation;
    GoalLocation = aGoalLocation;
    Triangles = &aTriSolution;
    Edges = &aLinks;
    pSolution = apSolution;
    pBadNodes = apBadNodeArray;
    fSmoothingRadius = pRequest->GetActorRadius();
    if (aTriSolution.iSize == 1) {
        // A single triangle: the path is the start and the goal, both on it.
        NavMeshInfo* pStartInfo = aTriSolution.GetAt(0).pNavMesh->GetNavMeshInfo();
        AddPathingNode(aStartLocation, pStartInfo);
        NavMeshInfo* pGoalInfo = aTriSolution.GetAt(0).pNavMesh->GetNavMeshInfo();
        AddPathingNode(aGoalLocation, pGoalInfo);
        return true;
    }
    LeftVerts.SetReservedSize(2 * aLinks.iSize);
    RightVerts.SetReservedSize(2 * aLinks.iSize);
    LeftEdges.SetReservedSize(2 * aLinks.iSize);
    RightEdges.SetReservedSize(2 * aLinks.iSize);
    PathEdges.SetReservedSize(2 * aLinks.iSize);
    LeftVerts.Clear(false);
    RightVerts.Clear(false);
    LeftEdges.Clear(false);
    RightEdges.Clear(false);
    PathEdges.Clear(false);
    BuildInternalArrays();
    BuildShrunkEdgeLeftRightArrays();
    bool bCanFit = CheckBoundsEdges();
    SetLeftRightShrunkLocations();
    ComputeCenters();
    BuildBestPointOnEdge();
    HandleWaterTriangles();
    BuildStraightenedPath();
    HandleAvoidNodes();
    if (abAddNodesIfFail || apBadNodeArray->IsEmpty()) {
        BuildFinalPath(true, true);
        ComputeTangents();
    }
    return bCanFit;
}

void PathSmoother::AddPathingNode(const NiPoint3& aLocation, NavMeshInfo* apInfo) {
    PathingLocation kLocation(&aLocation, apInfo);
    AddPathingNode(kLocation);
}

bool PathSmoother::CheckLineOfSight(
    uint32_t aiFirstEdgeIndex,
    uint32_t aiLastEdgeIndex,
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd
) {
    while (aiFirstEdgeIndex <= aiLastEdgeIndex) {
        if (!NavMeshMath::Find2DIntersection3(
                PathEdges.GetAt(aiFirstEdgeIndex)->LeftShrunkLocation,
                PathEdges.GetAt(aiFirstEdgeIndex)->RightShrunkLocation,
                aLineStart,
                aLineEnd
            )) {
            return false;
        }
        ++aiFirstEdgeIndex;
    }
    return true;
}

void PathSmoother::BuildShrunkEdgeLeftRightArrays() {
    // Each end vertex's shrunk location starts at its location; the vertex is looked up twice.
    PathSmoother::UniqueVert* pVert = LeftVerts.GetFirst();
    LeftVerts.GetFirst()->ShrunkLocation = pVert->Location;
    pVert = RightVerts.GetFirst();
    RightVerts.GetFirst()->ShrunkLocation = pVert->Location;
    for (uint32_t i = 1; i < LeftVerts.iSize - 1; i = ComputeShrunkPoint(i, fSmoothingRadius, true)) {}
    for (uint32_t i = 1; i < RightVerts.iSize - 1; i = ComputeShrunkPoint(i, fSmoothingRadius, false)) {}
    pVert = *static_cast<PathSmoother::UniqueVert**>(LeftVerts.GetTail());
    (*static_cast<PathSmoother::UniqueVert**>(LeftVerts.GetTail()))->ShrunkLocation = pVert->Location;
    pVert = *static_cast<PathSmoother::UniqueVert**>(RightVerts.GetTail());
    (*static_cast<PathSmoother::UniqueVert**>(RightVerts.GetTail()))->ShrunkLocation = pVert->Location;
}

uint32_t PathSmoother::ComputeShrunkPoint(uint32_t aiVertIndex, float afRadius, bool abLeft) {
    BSScrapArray<UniqueVert*>* pVerts;
    if (abLeft) {
        pVerts = &LeftVerts;
    }
    else {
        pVerts = &RightVerts;
    }
    BSScrapArray<BoundsEdge*>* pEdges;
    if (abLeft) {
        pEdges = &LeftEdges;
    }
    else {
        pEdges = &RightEdges;
    }
    // The directions to the previous and the next vertex, past a vertex at the same place.
    NiPoint3 kPrevDir;
    NiPoint3 kNextDir;
    UniqueVert* pCurrent = (*pVerts)[aiVertIndex];
    kPrevDir = (*pVerts)[aiVertIndex - 1]->Location - pCurrent->Location;
    pCurrent = (*pVerts)[aiVertIndex];
    kNextDir = (*pVerts)[aiVertIndex + 1]->Location - pCurrent->Location;
    if (kPrevDir == NiPoint3::ZERO && aiVertIndex > 1) {
        pCurrent = (*pVerts)[aiVertIndex];
        kPrevDir = (*pVerts)[aiVertIndex - 2]->Location - pCurrent->Location;
    }
    if (kNextDir == NiPoint3::ZERO && aiVertIndex < pVerts->iSize - 2) {
        pCurrent = (*pVerts)[aiVertIndex];
        kNextDir = (*pVerts)[aiVertIndex + 2]->Location - pCurrent->Location;
    }
    kPrevDir.z = 0.0f;
    kNextDir.z = 0.0f;
    kPrevDir.Unitize();
    kNextDir.Unitize();
    NiPoint3 kNextPerp = kNextDir.Cross(NiPoint3::UNIT_Z);
    NiPoint3 kPrevPerp = kPrevDir.Cross(NiPoint3::UNIT_Z);
    float fSin = kPrevDir.Dot(kNextPerp);
    float fCos = kPrevDir.Dot(kNextDir);
    float fHalfAngle = (3.1415927410125732 - std::atan2(fSin, fCos)) * 0.5;
    float fPushScale = std::tan(fHalfAngle) * afRadius;
    UniqueVert* pVert = (*pVerts)[aiVertIndex];
    // The vertex's first path edge, from this side's vertex to the other side's.
    NiPoint3 kEdgeDir;
    if (abLeft) {
        UniqueVert* pLeftVert = pVert->EdgeList.GetAt(0)->pLeftVert;
        kEdgeDir = pVert->EdgeList.GetAt(0)->pRightVert->Location - pLeftVert->Location;
    }
    else {
        UniqueVert* pRightVert = pVert->EdgeList.GetAt(0)->pRightVert;
        kEdgeDir = pVert->EdgeList.GetAt(0)->pLeftVert->Location - pRightVert->Location;
    }
    // A sharp corner the path edge runs between: the vertex is split in two, each pushed out from its own edge.
    if (fCos >= 0.800000011920929) {
        double dPrevSide = kPrevPerp.Dot(kEdgeDir);
        if (kNextPerp.Dot(kEdgeDir) * dPrevSide > 0.0) {
            NiPoint3 kPrevVertNewLocation;
            NiPoint3 kNewVertLocation;
            if (abLeft) {
                float fPrevBack = afRadius * 1.5;
                NiPoint3 kPrevBack = fPrevBack * kPrevDir;
                NiPoint3 kPrevOut = afRadius * kPrevPerp;
                kPrevVertNewLocation = pVert->Location - kPrevOut - kPrevBack;
                float fNextBack = afRadius * 1.5;
                NiPoint3 kNextBack = fNextBack * kNextDir;
                NiPoint3 kNextOut = afRadius * kNextPerp;
                kNewVertLocation = pVert->Location + kNextOut - kNextBack;
            }
            else {
                float fPrevBack = afRadius * 1.5;
                NiPoint3 kPrevBack = fPrevBack * kPrevDir;
                NiPoint3 kPrevOut = afRadius * kPrevPerp;
                kPrevVertNewLocation = pVert->Location + kPrevOut - kPrevBack;
                float fNextBack = afRadius * 1.5;
                NiPoint3 kNextBack = fNextBack * kNextDir;
                NiPoint3 kNextOut = afRadius * kNextPerp;
                kNewVertLocation = pVert->Location - kNextOut - kNextBack;
            }
            DuplicateVert(aiVertIndex, abLeft, kPrevVertNewLocation, kNewVertLocation);
            return aiVertIndex + 2;
        }
    }
    // Otherwise the vertex is pushed out along the corner's bisector.
    if (abLeft) {
        NiPoint3 kOut = kNextPerp * afRadius;
        NiPoint3 kAlong = kNextDir * fPushScale;
        pVert->PushDelta = kAlong + kOut;
    }
    else {
        NiPoint3 kOut = kNextPerp * afRadius;
        NiPoint3 kBack = -kNextDir;
        NiPoint3 kAlong = kBack * fPushScale;
        pVert->PushDelta = kAlong - kOut;
    }
    char cShrunk = 0;
    if ((*pEdges)[aiVertIndex - 1]->bOpenEdge && (*pEdges)[aiVertIndex]->bOpenEdge) {
        UniqueVert* pNextVert = (*pVerts)[aiVertIndex + 1];
        UniqueVert* pPrevVert = (*pVerts)[aiVertIndex - 1];
        NavMeshTriHandle kTriHandle(pVert->hNavMeshTri);
        cShrunk = PushPoint(
            &pVert->Location,
            &pVert->ShrunkLocation,
            kTriHandle,
            &pVert->PushDelta,
            &pPrevVert->ShrunkLocation,
            &pNextVert->Location
        );
    }
    else {
        pVert->ShrunkLocation = pVert->Location + pVert->PushDelta;
        cShrunk = 1;
    }
    if (cShrunk) {
        // Where the shrunk bounds edge to the previous vertex crosses the original one, the two shrunk locations
        // move to just either side of their midpoint.
        UniqueVert* pSegment2End = (*pVerts)[aiVertIndex];
        NiPoint3* pSegment2Start = &(*pVerts)[aiVertIndex]->ShrunkLocation;
        UniqueVert* pSegment1End = (*pVerts)[aiVertIndex - 1];
        NiPoint3* pSegment1Start = &(*pVerts)[aiVertIndex - 1]->ShrunkLocation;
        if (NavMeshMath::Find2DIntersection3(
                *pSegment1Start,
                pSegment1End->Location,
                *pSegment2Start,
                pSegment2End->Location
            )) {
            UniqueVert* pPrevVert = (*pVerts)[aiVertIndex - 1];
            NiPoint3 kDelta = (*pVerts)[aiVertIndex]->ShrunkLocation - pPrevVert->ShrunkLocation;
            UniqueVert* pThisVert = (*pVerts)[aiVertIndex];
            NiPoint3 kMid = ((*pVerts)[aiVertIndex - 1]->ShrunkLocation + pThisVert->ShrunkLocation) * 0.5f;
            NiPoint3 kPrevShrunk = kMid + kDelta * 0.01f;
            (*pVerts)[aiVertIndex - 1]->ShrunkLocation = kPrevShrunk;
            NiPoint3 kThisShrunk = kMid - kDelta * 0.01f;
            (*pVerts)[aiVertIndex]->ShrunkLocation = kThisShrunk;
        }
    }
    return aiVertIndex + 1;
}

char PathSmoother::PushPoint(
    NiPoint3* aPoint,
    NiPoint3* aRetLocation,
    NavMeshTriHandle aTri,
    NiPoint3* aPushDelta,
    NiPoint3* aPrevPoint,
    NiPoint3* aNextPoint
) {
    TESWorldSpace* pStartWorldSpace = aTri.pNavMeshInfo->GetWorldSpace();
    TESObjectCELL* pStartCell = aTri.pNavMeshInfo->GetCell();
    NiPoint3 kStart = *aPoint + *aPushDelta * 0.009999999776482582f + NiPoint3::UNIT_X;
    PathingLocation kStartLocation(&kStart, pStartCell, pStartWorldSpace);
    TESWorldSpace* pGoalWorldSpace = aTri.pNavMeshInfo->GetWorldSpace();
    TESObjectCELL* pGoalCell = aTri.pNavMeshInfo->GetCell();
    NiPoint3 kGoal = *aPoint - *aPushDelta + NiPoint3::UNIT_X;
    PathingLocation kGoalLocation(&kGoal, pGoalCell, pGoalWorldSpace);
    PathingLocation kIntersectionLocation;
    if (Pathing::CheckLineOfSight(kStartLocation, kGoalLocation, &kIntersectionLocation, nullptr)) {
        *aRetLocation = *aPoint;
        if (Pathing::CheckLineOfSight(kGoalLocation, *aPrevPoint, nullptr, nullptr) &&
            Pathing::CheckLineOfSight(kGoalLocation, *aNextPoint, nullptr, nullptr)) {
            return 0;
        }

        *aRetLocation = *aPoint + *aPushDelta;
        return 1;
    }

    *aRetLocation = kIntersectionLocation.GetLocation() + *aPushDelta;
    return 1;
}

bool PathSmoother::CheckBoundsEdges() {
    PathSmoother::BoundsEdge* pprevLeftEdge = PathEdges.GetAt(0)->pPrevLeftEdge;
    PathSmoother::BoundsEdge* pprevRightEdge = PathEdges.GetAt(0)->pPrevRightEdge;
    for (uint32_t i = 1; i < PathEdges.iSize; ++i) {
        PathSmoother::BoundsEdge* pleftEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevLeftEdge;
        PathSmoother::BoundsEdge* prightEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevRightEdge;
        if (pleftEdge == pprevLeftEdge && !TestBoundsEdgeAgainstOtherBound(prightEdge, pleftEdge, false)) {
            if (pBadNodes) {
                NavMeshSearchEdge badEdge;
                NavMeshPtr spmesh;
                if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
                    badEdge.pNavMesh = spmesh;
                    badEdge.sTriangle = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle;
                    badEdge.iEdgeIndex = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.iEdgeIndex;
                    pBadNodes->Add(badEdge);
                }
            }
            return false;
        }
        if (prightEdge == pprevRightEdge && !TestBoundsEdgeAgainstOtherBound(pleftEdge, prightEdge, true)) {
            if (pBadNodes) {
                NavMeshSearchEdge badEdge;
                NavMeshPtr spmesh;
                if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
                    badEdge.pNavMesh = spmesh;
                    badEdge.sTriangle = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle;
                    badEdge.iEdgeIndex = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.iEdgeIndex;
                    pBadNodes->Add(badEdge);
                }
            }
            return false;
        }
        pprevLeftEdge = pleftEdge;
        pprevRightEdge = prightEdge;
    }
    uint32_t i;
    for (i = 0;; ++i) {
        if (i >= PathEdges.iSize) {
            return true;
        }
        PathSmoother::BoundsEdge* pleftEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevLeftEdge;
        PathSmoother::BoundsEdge* prightEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevRightEdge;
        if (NavMeshMath::Find2DIntersection3(
                pleftEdge->pStartVert->ShrunkLocation,
                pleftEdge->pEndVert->ShrunkLocation,
                prightEdge->pStartVert->ShrunkLocation,
                prightEdge->pEndVert->ShrunkLocation
            )) {
            break;
        }
    }
    if (pBadNodes) {
        NavMeshSearchEdge badEdge;
        NavMeshPtr spmesh;
        if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
            badEdge.pNavMesh = spmesh;
            badEdge.sTriangle = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle;
            badEdge.iEdgeIndex = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.iEdgeIndex;
            pBadNodes->Add(badEdge);
        }
    }
    return false;
}

bool PathSmoother::TestBoundsEdgeAgainstOtherBound(BoundsEdge* apEdge1, BoundsEdge* apEdge2, bool abLeft) {
    NiPoint3 kStart = apEdge1->pStartVert->ShrunkLocation;
    NiPoint3 kEnd = apEdge1->pEndVert->ShrunkLocation;
    PathSmoother::UniqueVert* pVert = apEdge2->pEndVert;
    if (!NavMeshMath::Find2DIntersection3(kStart, kEnd, pVert->Location, pVert->ShrunkLocation)) {
        return !NavMeshMath::Find2DIntersection3(
            kStart,
            kEnd,
            apEdge2->pStartVert->ShrunkLocation,
            apEdge2->pEndVert->ShrunkLocation
        );
    }
    NiPoint3 kPush = pVert->ShrunkLocation - pVert->Location;
    PathingLocation kVertLocation(&pVert->ShrunkLocation, pVert->hNavMeshTri.pNavMeshInfo);
    if (Pathing::CheckLineOfSight(kVertLocation, pVert->ShrunkLocation + kPush, nullptr, nullptr)) {
        NiPoint3 kPushed1 = pVert->ShrunkLocation + kPush;
        PathingLocation kStartLocation(&kPushed1, pVert->hNavMeshTri.pNavMeshInfo);
        NiPoint3 kPushed2 = pVert->ShrunkLocation + kPush;
        PathingLocation kEndLocation(&kPushed2, pVert->hNavMeshTri.pNavMeshInfo);
        if (Pathing::CheckLineOfSight(kStartLocation, kStart, nullptr, nullptr) &&
            Pathing::CheckLineOfSight(kEndLocation, kEnd, nullptr, nullptr)) {
            PathSmoother::UniqueVert* pNewVert = SplitBoundsEdge(apEdge1->iEdgeIndex, abLeft);
            pNewVert->Location = pVert->ShrunkLocation;
            pNewVert->ShrunkLocation = pVert->ShrunkLocation + kPush * 0.1f;
            return true;
        }
    }
    return false;
}

void PathSmoother::SetLeftRightShrunkLocations() {
    for (PathEdge* pEdge : PathEdges) {
        pEdge->LeftShrunkLocation = pEdge->pLeftVert->ShrunkLocation;
        pEdge->RightShrunkLocation = pEdge->pRightVert->ShrunkLocation;
    }
}

void PathSmoother::ComputeCenters() {
    for (uint32_t i = 0; i < PathEdges.iSize; ++i) {
        PathEdges.GetAt(i)->Center = (PathEdges.GetAt(i)->LeftShrunkLocation +
                                      PathEdges.GetAt(i)->RightShrunkLocation) *
            0.5f;
    }
}

void PathSmoother::BuildBestPointOnEdge() {
    for (int32_t i = 0; i < static_cast<int32_t>(PathEdges.iSize); ++i) {
        NiPoint3 averagePointBefore(0.0f, 0.0f, 0.0f);
        uint32_t uiPointCountBefore = 0;
        for (int32_t offset = -4; offset < 0; ++offset) {
            int32_t iEdgeIndex = offset + i;
            if (offset + i == -1) {
                ++uiPointCountBefore;
                averagePointBefore += StartLocation;
            }
            else {
                uint32_t uiNumEdges = PathEdges.iSize;
                if (iEdgeIndex == uiNumEdges) {
                    ++uiPointCountBefore;
                    averagePointBefore += GoalLocation;
                }
                else if (iEdgeIndex >= 0) {
                    int32_t iNumEdges = PathEdges.iSize;
                    if (iEdgeIndex < iNumEdges) {
                        ++uiPointCountBefore;
                        averagePointBefore += PathEdges.GetAt(iEdgeIndex)->Center;
                    }
                }
            }
        }
        if (uiPointCountBefore) {
            averagePointBefore /= static_cast<float>(uiPointCountBefore);
        }
        else {
            averagePointBefore = StartLocation;
        }
        NiPoint3 averagePointAfter(0.0f, 0.0f, 0.0f);
        uint32_t uiPointCountAfter = 0;
        for (int32_t offset = 1; offset <= 4; ++offset) {
            int32_t iEdgeIndex = offset + i;
            if (offset + i == -1) {
                ++uiPointCountAfter;
                averagePointAfter += StartLocation;
            }
            else {
                uint32_t uiNumEdges = PathEdges.iSize;
                if (iEdgeIndex == uiNumEdges) {
                    ++uiPointCountAfter;
                    averagePointAfter += GoalLocation;
                }
                else if (iEdgeIndex >= 0) {
                    int32_t iNumEdges = PathEdges.iSize;
                    if (iEdgeIndex < iNumEdges) {
                        ++uiPointCountAfter;
                        averagePointAfter += PathEdges.GetAt(iEdgeIndex)->Center;
                    }
                }
            }
        }
        if (uiPointCountAfter) {
            averagePointAfter /= static_cast<float>(uiPointCountAfter);
        }
        else {
            averagePointAfter = GoalLocation;
        }
        PathEdges.GetAt(i)->BestPointOnEdge = NavMeshMath::ClosestPointOnSegmentToLine3(
            PathEdges.GetAt(i)->LeftShrunkLocation,
            PathEdges.GetAt(i)->RightShrunkLocation,
            averagePointBefore,
            averagePointAfter
        );
        if (PathEdges.GetAt(i)->BestPointOnEdge == PathEdges.GetAt(i)->LeftShrunkLocation ||
            PathEdges.GetAt(i)->BestPointOnEdge == PathEdges.GetAt(i)->RightShrunkLocation) {
            NiPoint3 averagePointDelta = averagePointAfter - averagePointBefore;
            averagePointDelta.Unitize();
            NiPoint3 segmentDelta = PathEdges.GetAt(i)->LeftShrunkLocation - PathEdges.GetAt(i)->RightShrunkLocation;
            segmentDelta.Unitize();
            if (i <= 0 || !(std::fabs(averagePointDelta.Dot(segmentDelta)) > 0.800000011920929)) {
                if (PathEdges.GetAt(i)->BestPointOnEdge == PathEdges.GetAt(i)->LeftShrunkLocation) {
                    NiPoint3 delta = PathEdges.GetAt(i)->RightShrunkLocation - PathEdges.GetAt(i)->LeftShrunkLocation;
                    PathEdges.GetAt(i)->BestPointOnEdge += delta * 0.01f;
                }
                else if (PathEdges.GetAt(i)->BestPointOnEdge == PathEdges.GetAt(i)->RightShrunkLocation) {
                    NiPoint3 delta = PathEdges.GetAt(i)->LeftShrunkLocation - PathEdges.GetAt(i)->RightShrunkLocation;
                    PathEdges.GetAt(i)->BestPointOnEdge += delta * 0.01f;
                }
            }
            else {
                float fSqrDistFromLeftToPrevious = (PathEdges.GetAt(i)->LeftShrunkLocation -
                                                    PathEdges.GetAt(i - 1)->BestPointOnEdge)
                                                       .SqrXYLength();
                float fSqrDistFromRightToPrevious = (PathEdges.GetAt(i)->RightShrunkLocation -
                                                     PathEdges.GetAt(i - 1)->BestPointOnEdge)
                                                        .SqrXYLength();
                if (fSqrDistFromRightToPrevious > static_cast<double>(fSqrDistFromLeftToPrevious)) {
                    NiPoint3 kRightPart = PathEdges.GetAt(i)->RightShrunkLocation * 0.01f;
                    PathEdges.GetAt(i)->BestPointOnEdge = PathEdges.GetAt(i)->LeftShrunkLocation * 0.99f + kRightPart;
                }
                else {
                    NiPoint3 kRightPart = PathEdges.GetAt(i)->RightShrunkLocation * 0.99f;
                    PathEdges.GetAt(i)->BestPointOnEdge = PathEdges.GetAt(i)->LeftShrunkLocation * 0.01f + kRightPart;
                }
            }
        }
        float fRandomness = pRequest->fSmoothingRandomness;
        NiPoint3 deltaToVert;
        if (fRandomness > 0.0) {
            deltaToVert = PathEdges.GetAt(i)->RightShrunkLocation - PathEdges.GetAt(i)->BestPointOnEdge;
        }
        else {
            deltaToVert = PathEdges.GetAt(i)->LeftShrunkLocation - PathEdges.GetAt(i)->BestPointOnEdge;
        }
        fRandomness = std::fabs(fRandomness);
        float fHalfRandomness = fRandomness * 0.5;
        PathEdges.GetAt(i)->BestPointOnEdge += fHalfRandomness * deltaToVert;
    }
}

void PathSmoother::HandleWaterTriangles() {
    using waterTris_t = BSScrapArray<bool, 1024>;
    waterTris_t waterTris(PathEdges.iSize);
    for (uint32_t i = 0; i < PathEdges.iSize; ++i) {
        NavMeshPtr spmesh;
        if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
            NavMeshTriangle* pTriangle = spmesh->GetTriangle(
                PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle
            );
            waterTris.GetAt(static_cast<int32_t>(i)) = pTriangle->IsWater();
        }
        PathEdges.GetAt(static_cast<int32_t>(i))->eEnterLeaveWater = PathSmoother::ELW_NONE;
    }
    bool bWater = true;
    uint32_t ifirstWaterTri = waterTris.Find(bWater, 0, &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
    while (ifirstWaterTri != -1) {
        PathSmoother::PathEdge* plastEdge;
        PathSmoother::PathEdge* pfirstEdge;
        bool bDry = false;
        uint32_t ilastWaterTri = waterTris.Find(
            bDry,
            ifirstWaterTri + 1,
            &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress
        );
        if (ilastWaterTri == -1) {
            ilastWaterTri = waterTris.iSize - 1;
        }
        else {
            --ilastWaterTri;
        }
        if (ifirstWaterTri) {
            pfirstEdge = PathEdges.GetAt(static_cast<int32_t>(ifirstWaterTri - 1));
        }
        else {
            pfirstEdge = nullptr;
        }
        if (ilastWaterTri == waterTris.iSize - 1) {
            plastEdge = nullptr;
        }
        else {
            plastEdge = PathEdges.GetAt(static_cast<int32_t>(ilastWaterTri + 1));
        }
        if (pfirstEdge) {
            pfirstEdge->eEnterLeaveWater = PathSmoother::ELW_ENTER;
        }
        if (plastEdge) {
            plastEdge->eEnterLeaveWater = PathSmoother::ELW_LEAVE;
        }
        NiPoint3 firstLoc = pfirstEdge ? pfirstEdge->BestPointOnEdge : StartLocation;
        NiPoint3 lastLoc = plastEdge ? plastEdge->BestPointOnEdge : GoalLocation;
        float fDeltaZ = lastLoc.z - firstLoc.z;
        float fDeltaXY = (lastLoc - firstLoc).XYLength();
        for (uint32_t i = ifirstWaterTri; i <= ilastWaterTri; ++i) {
            float fThisEdgeDeltaXY = (PathEdges.GetAt(static_cast<int32_t>(i))->BestPointOnEdge - firstLoc).XYLength();
            double dZ = fDeltaZ * fThisEdgeDeltaXY / fDeltaXY + firstLoc.z;
            PathEdges.GetAt(static_cast<int32_t>(i))->BestPointOnEdge.z = dZ;
        }
        if (ilastWaterTri == waterTris.iSize - 1) {
            ifirstWaterTri = -1;
        }
        else {
            bool bWaterNext = true;
            ifirstWaterTri = waterTris.Find(
                bWaterNext,
                ilastWaterTri + 1,
                &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress
            );
        }
    }
}

void PathSmoother::BuildStraightenedPath() {
    NavMeshInfo* pStartNavMeshInfo = Triangles->GetFirst().pNavMesh->GetNavMeshInfo();
    {
        PathingLocation startLoc(&StartLocation, pStartNavMeshInfo);
        StraightenedPath.Add(startLoc);
    }
    int32_t icurrentEdge = -1;
    int32_t inextEdge = 1;
    float ftotalNormalDelta = 0.0f;
    while (inextEdge < static_cast<int32_t>(PathEdges.iSize + 1)) {
        if (inextEdge != PathEdges.iSize) {
            ftotalNormalDelta = 1.0 - PathEdges.GetAt(inextEdge - 1)->fTriangleNormalDelta + ftotalNormalDelta;
        }
        bool bAddNode = false;
        if (ftotalNormalDelta > 0.20000000298023224) {
            bAddNode = true;
        }
        else {
            PathSmoother::PathEdge* pnextEdge = inextEdge == PathEdges.iSize ? nullptr : PathEdges.GetAt(inextEdge);
            if (pnextEdge && pnextEdge->eEnterLeaveWater) {
                bAddNode = true;
            }
            else {
                NiPoint3 testStartPoint = icurrentEdge == -1 ? StartLocation
                                                             : PathEdges.GetAt(icurrentEdge)->BestPointOnEdge;
                NiPoint3 testEndPoint = inextEdge == PathEdges.iSize ? GoalLocation
                                                                     : PathEdges.GetAt(inextEdge)->BestPointOnEdge;
                if (!CheckLineOfSight(icurrentEdge + 1, inextEdge - 1, testStartPoint, testEndPoint)) {
                    bAddNode = true;
                }
            }
        }
        if (bAddNode) {
            NavMeshInfo* pNavMeshInfo = PathEdges.GetAt(inextEdge - 1)->hNavMeshEdge.pNavMeshInfo;
            PathingLocation loc(&PathEdges.GetAt(inextEdge - 1)->BestPointOnEdge, pNavMeshInfo);
            StraightenedPath.Add(loc);
            ftotalNormalDelta = 0.0f;
            icurrentEdge = inextEdge - 1;
        }
        ++inextEdge;
    }
    NavMeshInfo* pGoalNavMeshInfo = Triangles->GetLast().pNavMesh->GetNavMeshInfo();
    PathingLocation goalLoc(&GoalLocation, pGoalNavMeshInfo);
    StraightenedPath.Add(goalLoc);
}

void PathSmoother::HandleAvoidNodes() {
    PathSmoother::AvoidNode newNode;
    PathingAvoidNodeArray* pAvoidNodes = pRequest->GetAvoidNodeArray();
    if (pAvoidNodes) {
        using AvoidNodes_t = BSScrapArray<PathSmoother::AvoidNode, 1024>;
        AvoidNodes_t avoidNodes;
        for (uint32_t i = 0; i < pAvoidNodes->iSize; ++i) {
            // Copies the node by value, then stores a true flag and tests it.
            PathingAvoidNode avoidNode = pAvoidNodes->GetAt(i);
            bool bAddNode = true;
            if (bAddNode) {
                ::new (&newNode) NiBound;
                newNode.Center = *avoidNode.GetPoint1();
                newNode.fRadius = avoidNode.GetRadius();
                avoidNodes.Add(newNode);
            }
        }
        for (uint32_t j = 0; j < avoidNodes.iSize; ++j) {
            for (uint32_t i = 0; i < StraightenedPath.iSize - 1; ++i) {
                float fCircleRadius = avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius;
                NiPoint3 kNextLocation = StraightenedPath.GetAt(static_cast<int32_t>(i + 1)).GetLocation();
                if (NavMeshMath::SegmentCircleIntersection(
                        StraightenedPath.GetAt(static_cast<int32_t>(i)).GetLocation(),
                        kNextLocation,
                        avoidNodes.GetAt(static_cast<int32_t>(j)).Center,
                        fCircleRadius
                    )) {
                    uint32_t ii;
                    // The tangent points on the avoid circle from the segment's start.
                    NiPoint3 akThisTangents[2];
                    NiPoint3 kThisLocation = StraightenedPath.GetAt(static_cast<int32_t>(i)).GetLocation();
                    if (!NavMeshMath::ComputeTangentPointsToCircleFromPoint(
                            &avoidNodes.GetAt(static_cast<int32_t>(j)).Center,
                            avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius,
                            &kThisLocation,
                            &akThisTangents[0].x,
                            &akThisTangents[1].x
                        )) {
                        akThisTangents[1] = StraightenedPath.GetAt(static_cast<int32_t>(i)).GetLocation();
                        akThisTangents[0] = akThisTangents[1];
                    }
                    kThisLocation = StraightenedPath.GetAt(static_cast<int32_t>(i)).GetLocation();
                    bool bThisValid0 = IsSegmentValid(&kThisLocation, &akThisTangents[0]);
                    kThisLocation = StraightenedPath.GetAt(static_cast<int32_t>(i)).GetLocation();
                    bool bThisValid1 = IsSegmentValid(&kThisLocation, &akThisTangents[1]);
                    for (ii = i + 1; ii < StraightenedPath.iSize; ++ii) {
                        NiPoint3 kLocation = StraightenedPath.GetAt(static_cast<int32_t>(ii)).GetLocation();
                        if (!avoidNodes.GetAt(static_cast<int32_t>(j)).IsPointInNode(kLocation)) {
                            break;
                        }
                    }
                    if (ii < StraightenedPath.iSize) {
                        // The tangent points on the avoid circle from the first location past the circle.
                        NiPoint3 akOtherTangents[2];
                        NiPoint3 kOtherLocation = StraightenedPath.GetAt(static_cast<int32_t>(ii)).GetLocation();
                        if (NavMeshMath::ComputeTangentPointsToCircleFromPoint(
                                &avoidNodes.GetAt(static_cast<int32_t>(j)).Center,
                                avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius,
                                &kOtherLocation,
                                &akOtherTangents[0].x,
                                &akOtherTangents[1].x
                            )) {
                            kOtherLocation = StraightenedPath.GetAt(static_cast<int32_t>(ii)).GetLocation();
                            bool bOtherValid0 = IsSegmentValid(&kOtherLocation, &akOtherTangents[0]);
                            kOtherLocation = StraightenedPath.GetAt(static_cast<int32_t>(ii)).GetLocation();
                            bool bOtherValid1 = IsSegmentValid(&kOtherLocation, &akOtherTangents[1]);
                            bool bFirstPairValid = bThisValid0 && bOtherValid1;
                            bool bSecondPairValid = bThisValid1 && bOtherValid0;
                            char cFound = 0;
                            int32_t ithisTangentIndex = 0;
                            int32_t iotherTangentIndex = 1;
                            if (bFirstPairValid && !bSecondPairValid) {
                                ithisTangentIndex = 0;
                                iotherTangentIndex = 1;
                                cFound = 1;
                            }
                            else if (bSecondPairValid && !bFirstPairValid) {
                                ithisTangentIndex = 1;
                                iotherTangentIndex = 0;
                                cFound = 1;
                            }
                            else if (bFirstPairValid && bSecondPairValid) {
                                float fFirstPairSqrDist = (akThisTangents[0] - akOtherTangents[1]).SqrLength();
                                float fSecondPairSqrDist = (akThisTangents[1] - akOtherTangents[0]).SqrLength();
                                if (fSecondPairSqrDist > static_cast<double>(fFirstPairSqrDist)) {
                                    ithisTangentIndex = 0;
                                    iotherTangentIndex = 1;
                                }
                                else {
                                    ithisTangentIndex = 1;
                                    iotherTangentIndex = 0;
                                }
                                cFound = 1;
                            }
                            if (cFound) {
                                if (ii > i + 1) {
                                    int32_t iHowMany = ii - i - 1;
                                    StraightenedPath.RemoveRange(i + 1, static_cast<uint32_t>(iHowMany), false);
                                }
                                {
                                    PathingLocation kThisTangentLocation(
                                        akThisTangents[ithisTangentIndex],
                                        StraightenedPath.GetAt(static_cast<int32_t>(i))
                                    );
                                    StraightenedPath.Insert(i + 1, kThisTangentLocation);
                                }
                                {
                                    PathingLocation kOtherTangentLocation(
                                        akOtherTangents[iotherTangentIndex],
                                        StraightenedPath.GetAt(static_cast<int32_t>(i))
                                    );
                                    StraightenedPath.Insert(i + 2, kOtherTangentLocation);
                                }
                                break;
                            }
                            i = ii - 1;
                        }
                    }
                }
            }
        }
    }
}

void PathSmoother::BuildFinalPath(bool abAddStartNode, bool abAddGoalNode) {
    if (abAddStartNode) {
        NavMeshInfo* pStartNavMeshInfo = Triangles->GetFirst().pNavMesh->GetNavMeshInfo();
        PathingLocation startLoc(&StartLocation, pStartNavMeshInfo);
        AddPathingNode(startLoc);
    }
    for (uint32_t i = 1; i < StraightenedPath.iSize - 1; ++i) {
        AddPathingNode(StraightenedPath.GetAt(static_cast<int32_t>(i)));
    }
    if (abAddGoalNode) {
        NavMeshInfo* pGoalNavMeshInfo = Triangles->GetLast().pNavMesh->GetNavMeshInfo();
        PathingLocation goalLoc(&GoalLocation, pGoalNavMeshInfo);
        AddPathingNode(goalLoc);
    }
}

void PathSmoother::ComputeTangents() {
    for (uint32_t i = 1; i < pSolution->GetNodeCount() - 1; ++i) {
        NiPoint3 deltaToPrevious;
        NiPoint3 deltaToNext;
        PathingNode* pNode = pSolution->GetNode(i);
        NiPoint3 kLocation = pNode->Location.GetLocation();
        PathingNode* pNextNode = pSolution->GetNode(i + 1);
        deltaToNext = pNextNode->Location.GetLocation() - kLocation;
        float fLengthToNext = deltaToNext.UnitizeGetLength();
        PathingNode* pPreviousNode = pSolution->GetNode(i - 1);
        NiPoint3 kPreviousLocation = pPreviousNode->Location.GetLocation();
        pNode = pSolution->GetNode(i);
        deltaToPrevious = pNode->Location.GetLocation() - kPreviousLocation;
        float fLengthToPrevious = deltaToPrevious.UnitizeGetLength();
        float fMinLength = BSMIN(fLengthToNext, fLengthToPrevious);
        NiPoint3 kTangent = (deltaToNext + deltaToPrevious) * fMinLength * 0.5f;
        pSolution->GetNode(i)->SetTangent(kTangent);
    }
}

bool PathSmoother::IsSegmentValid(NiPoint3* apStart, NiPoint3* apEnd) {
    return PointInShrunkPath(apEnd) && SegmentClearOfEdges(apStart, apEnd);
}

char PathSmoother::SegmentClearOfEdges(NiPoint3* apStart, NiPoint3* apEnd) {
    for (BoundsEdge* pEdge : LeftEdges) {
        if (NavMeshMath::Find2DIntersection3(
                *apStart,
                *apEnd,
                pEdge->pStartVert->ShrunkLocation,
                pEdge->pEndVert->ShrunkLocation
            )) {
            return 0;
        }
    }
    for (BoundsEdge* pEdge : RightEdges) {
        if (NavMeshMath::Find2DIntersection3(
                *apStart,
                *apEnd,
                pEdge->pStartVert->ShrunkLocation,
                pEdge->pEndVert->ShrunkLocation
            )) {
            return 0;
        }
    }
    return 1;
}

bool PathSmoother::PointInShrunkPath(NiPoint3* aLocation) {
    float afZDiff;
    if (PathEdges.IsEmpty()) {
        return true;
    }
    if (NavMeshMath::IsPointInsideTriangle(
            StartLocation,
            PathEdges.GetAt(0)->pRightVert->ShrunkLocation,
            PathEdges.GetAt(0)->pLeftVert->ShrunkLocation,
            *aLocation,
            afZDiff,
            180.0f,
            180.0f
        )) {
        return true;
    }
    for (uint32_t uiIndex = 0; uiIndex < PathEdges.iSize - 1; ++uiIndex) {
        if (PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pLeftVert ==
            PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pLeftVert) {
            if (NavMeshMath::IsPointInsideTriangle(
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pLeftVert->ShrunkLocation,
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pRightVert->ShrunkLocation,
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pRightVert->ShrunkLocation,
                    *aLocation,
                    afZDiff,
                    180.0f,
                    180.0f
                )) {
                return true;
            }
        }
        else {
            if (PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pRightVert ==
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pRightVert &&
                NavMeshMath::IsPointInsideTriangle(
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pLeftVert->ShrunkLocation,
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pRightVert->ShrunkLocation,
                    PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pLeftVert->ShrunkLocation,
                    *aLocation,
                    afZDiff,
                    180.0f,
                    180.0f
                )) {
                return true;
            }
        }
    }
    // Takes the tail's right vertex first, then its left one.
    NiPoint3& rRightLocation = (*static_cast<PathSmoother::PathEdge**>(PathEdges.GetTail()))
                                   ->pRightVert->ShrunkLocation;
    NiPoint3& rLeftLocation = (*static_cast<PathSmoother::PathEdge**>(PathEdges.GetTail()))->pLeftVert->ShrunkLocation;
    return NavMeshMath::IsPointInsideTriangle(
        GoalLocation,
        rLeftLocation,
        rRightLocation,
        *aLocation,
        afZDiff,
        180.0f,
        180.0f
    );
}

PathSmoother::UniqueVert* PathSmoother::DuplicateVert(
    uint32_t aiVertIndex,
    bool abLeft,
    const NiPoint3& aPreviousVertNewLocation,
    const NiPoint3& aNewVertLocation
) {
    PathSmoother::UniqueVert** ppOriginalVert;
    if (abLeft) {
        ppOriginalVert = &LeftVerts.GetAt(static_cast<int32_t>(aiVertIndex));
    }
    else {
        ppOriginalVert = &RightVerts.GetAt(static_cast<int32_t>(aiVertIndex));
    }
    PathSmoother::UniqueVert* pOriginalVert = *ppOriginalVert;
    PathSmoother::UniqueVert* pNewVert = CreateVert();
    pNewVert->Location = pOriginalVert->Location;
    pNewVert->hNavMeshTri = pOriginalVert->hNavMeshTri;
    uint32_t iEdgeCount = pOriginalVert->EdgeList.iSize;
    pOriginalVert->ShrunkLocation = aPreviousVertNewLocation;
    pOriginalVert->PushDelta = pOriginalVert->ShrunkLocation - pOriginalVert->Location;
    pNewVert->ShrunkLocation = aNewVertLocation;
    pNewVert->PushDelta = pNewVert->ShrunkLocation - pNewVert->Location;
    if (iEdgeCount == 1) {
        pNewVert->EdgeList.Add(pOriginalVert->EdgeList.GetAt(0));
    }
    else {
        uint32_t iHalfEdgeCount = iEdgeCount >> 1;
        for (uint32_t i = iEdgeCount >> 1; i < iEdgeCount; ++i) {
            if (abLeft) {
                pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i))->pLeftVert = pNewVert;
            }
            else {
                pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i))->pRightVert = pNewVert;
            }
            pNewVert->EdgeList.Add(pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i)));
        }
        pOriginalVert->EdgeList.SetSize(iHalfEdgeCount, true);
    }
    if (abLeft) {
        LeftVerts.Insert(aiVertIndex + 1, pNewVert);
    }
    else {
        RightVerts.Insert(aiVertIndex + 1, pNewVert);
    }
    PathSmoother::BoundsEdge* pBoundsEdge = CreateBoundsEdge();
    pBoundsEdge->pStartVert = pOriginalVert;
    pBoundsEdge->pEndVert = pNewVert;
    InsertBoundsEdge(aiVertIndex, pBoundsEdge, abLeft);
    PathSmoother::BoundsEdge** ppNextEdge;
    if (abLeft) {
        pBoundsEdge->hNavMeshEdge.Copy(LeftEdges.GetAt(static_cast<int32_t>(aiVertIndex))->hNavMeshEdge);
        pBoundsEdge->bOpenEdge = LeftEdges.GetAt(static_cast<int32_t>(aiVertIndex))->bOpenEdge;
        ppNextEdge = &LeftEdges.GetAt(static_cast<int32_t>(aiVertIndex + 1));
    }
    else {
        pBoundsEdge->hNavMeshEdge.Copy(RightEdges.GetAt(static_cast<int32_t>(aiVertIndex))->hNavMeshEdge);
        pBoundsEdge->bOpenEdge = RightEdges.GetAt(static_cast<int32_t>(aiVertIndex))->bOpenEdge;
        ppNextEdge = &RightEdges.GetAt(static_cast<int32_t>(aiVertIndex + 1));
    }
    (*ppNextEdge)->pStartVert = pNewVert;
    PathSmoother::PathEdge* pNewPathEdge = CreatePathEdge();
    // GetFirst gives the first item of the new vertex's edge list, GetTail the address of the last item of the
    // original vertex's.
    if (abLeft) {
        NiPoint3* pNewShrunkLocation = &pNewVert->ShrunkLocation;
        PathSmoother::UniqueVert* pNewVertCopy = pNewVert;
        PathSmoother::PathEdge* pFirstEdge = pNewVert->EdgeList.GetFirst();
        if (NavMeshMath::Find2DIntersection3(
                pOriginalVert->ShrunkLocation,
                pFirstEdge->pRightVert->ShrunkLocation,
                pNewVertCopy->Location,
                *pNewShrunkLocation
            )) {
            pNewPathEdge->pLeftVert = pNewVert;
            pNewPathEdge->pRightVert = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                           ->pRightVert;
            pNewPathEdge->pPrevLeftEdge = pBoundsEdge;
            pNewPathEdge->pPrevRightEdge = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                               ->pPrevRightEdge;
        }
        else {
            pNewPathEdge->pLeftVert = pOriginalVert;
            pNewPathEdge->pRightVert = pNewVert->EdgeList.GetFirst()->pRightVert;
            pNewPathEdge->pPrevLeftEdge = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                              ->pPrevLeftEdge;
            pNewPathEdge->pPrevRightEdge = pNewVert->EdgeList.GetFirst()->pPrevRightEdge;
        }
    }
    else {
        NiPoint3* pNewShrunkLocation = &pNewVert->ShrunkLocation;
        PathSmoother::UniqueVert* pNewVertCopy = pNewVert;
        PathSmoother::PathEdge* pFirstEdge = pNewVert->EdgeList.GetFirst();
        if (NavMeshMath::Find2DIntersection3(
                pOriginalVert->ShrunkLocation,
                pFirstEdge->pLeftVert->ShrunkLocation,
                pNewVertCopy->Location,
                *pNewShrunkLocation
            )) {
            pNewPathEdge->pLeftVert = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                          ->pLeftVert;
            pNewPathEdge->pRightVert = pNewVert;
            pNewPathEdge->pPrevLeftEdge = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                              ->pPrevLeftEdge;
            pNewPathEdge->pPrevRightEdge = pBoundsEdge;
        }
        else {
            pNewPathEdge->pLeftVert = pNewVert->EdgeList.GetFirst()->pLeftVert;
            pNewPathEdge->pRightVert = pOriginalVert;
            pNewPathEdge->pPrevLeftEdge = pNewVert->EdgeList.GetFirst()->pPrevLeftEdge;
            pNewPathEdge->pPrevRightEdge = (*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))
                                               ->pPrevRightEdge;
        }
    }
    int32_t iPrevPathEdgeIndex = pNewVert->EdgeList.GetFirst()->iPathEdgeIndex;
    pNewPathEdge->hNavMeshEdge.Copy(PathEdges.GetAt(iPrevPathEdgeIndex)->hNavMeshEdge);
    pNewPathEdge->fTriangleNormalDelta = 1.0f;
    InsertPathEdge(iPrevPathEdgeIndex, pNewPathEdge);
    return pNewVert;
}

PathSmoother::UniqueVert* PathSmoother::SplitBoundsEdge(uint32_t aiEdgeIndex, bool abLeft) {
    BSScrapArray<PathSmoother::BoundsEdge*, 1024>* pEdges;
    if (abLeft) {
        pEdges = &LeftEdges;
    }
    else {
        pEdges = &RightEdges;
    }
    PathSmoother::BoundsEdge* pEdge = (*pEdges)[aiEdgeIndex];
    PathSmoother::UniqueVert* pStartVert = pEdge->pStartVert;
    PathSmoother::UniqueVert* pEndVert = pEdge->pEndVert;
    PathSmoother::PathEdge* pPathEdge;
    if (pStartVert->EdgeList.IsEmpty()) {
        pPathEdge = pEndVert->EdgeList.GetFirst();
    }
    else {
        pPathEdge = *static_cast<PathSmoother::PathEdge**>(pStartVert->EdgeList.GetTail());
    }
    PathSmoother::UniqueVert* pNewVert = CreateVert();
    pNewVert->Location = pStartVert->Location;
    pNewVert->hNavMeshTri = pStartVert->hNavMeshTri;
    if (abLeft) {
        LeftVerts.Insert(aiEdgeIndex + 1, pNewVert);
    }
    else {
        RightVerts.Insert(aiEdgeIndex + 1, pNewVert);
    }
    PathSmoother::BoundsEdge* pBoundsEdge = CreateBoundsEdge();
    pBoundsEdge->pStartVert = pStartVert;
    pBoundsEdge->pEndVert = pNewVert;
    pBoundsEdge->hNavMeshEdge.Copy((*pEdges)[aiEdgeIndex]->hNavMeshEdge);
    pBoundsEdge->bOpenEdge = (*pEdges)[aiEdgeIndex]->bOpenEdge;
    (*pEdges)[aiEdgeIndex]->pStartVert = pNewVert;
    InsertBoundsEdge(aiEdgeIndex, pBoundsEdge, abLeft);
    PathSmoother::PathEdge* pNewPathEdge = CreatePathEdge();
    pNewPathEdge->fTriangleNormalDelta = 1.0f;
    pNewPathEdge->hNavMeshEdge.Copy(pPathEdge->hNavMeshEdge);
    if (abLeft) {
        pNewPathEdge->pLeftVert = pNewVert;
        pNewPathEdge->pRightVert = pPathEdge->pRightVert;
        pNewPathEdge->pPrevRightEdge = pPathEdge->pPrevRightEdge;
        pNewPathEdge->pPrevLeftEdge = pBoundsEdge;
    }
    else {
        pNewPathEdge->pLeftVert = pPathEdge->pLeftVert;
        pNewPathEdge->pPrevLeftEdge = pPathEdge->pPrevLeftEdge;
        pNewPathEdge->pRightVert = pNewVert;
        pNewPathEdge->pPrevRightEdge = pBoundsEdge;
    }
    if (pStartVert->EdgeList.IsEmpty()) {
        PathSmoother::PathEdge* pFirstEdge = pEndVert->EdgeList.GetFirst();
        InsertPathEdge(static_cast<uint32_t>(pFirstEdge->iPathEdgeIndex), pNewPathEdge);
    }
    else {
        PathSmoother::PathEdge* pLastEdge = *static_cast<PathSmoother::PathEdge**>(pStartVert->EdgeList.GetTail());
        InsertPathEdge(static_cast<uint32_t>(pLastEdge->iPathEdgeIndex + 1), pNewPathEdge);
    }
    return pNewVert;
}

void PathSmoother::InsertPathEdge(uint32_t aiInsertIndex, PathSmoother::PathEdge* apEdge) {
    PathEdges.Insert(aiInsertIndex, apEdge);
    for (uint32_t i = aiInsertIndex; i < PathEdges.iSize; ++i) {
        PathEdges.GetAt(static_cast<int32_t>(i))->iPathEdgeIndex = i;
    }
}

void PathSmoother::InsertBoundsEdge(uint32_t aiInsertIndex, PathSmoother::BoundsEdge* apEdge, bool abLeft) {
    if (abLeft) {
        LeftEdges.Insert(aiInsertIndex, apEdge);
        for (uint32_t uiIndex = aiInsertIndex; uiIndex < LeftEdges.iSize; ++uiIndex) {
            LeftEdges.GetAt(static_cast<int32_t>(uiIndex))->iEdgeIndex = uiIndex;
        }
    }
    else {
        RightEdges.Insert(aiInsertIndex, apEdge);
        for (uint32_t i = aiInsertIndex; i < RightEdges.iSize; ++i) {
            RightEdges.GetAt(static_cast<int32_t>(i))->iEdgeIndex = i;
        }
    }
}

NavMeshSearchEdge::NavMeshSearchEdge() {}
