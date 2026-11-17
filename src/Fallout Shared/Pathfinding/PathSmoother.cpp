#include "Fallout Shared/Pathfinding/PathSmoother.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LockFreeMap.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <new>

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
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

// The vertices and edges live in TheScrapHeap: each vertex is destroyed in place (the exe's deleting destructor with
// flag 0) and handed back, as are the edges. The exe drops the first two right vertices before it destroys the rest.
PathSmoother::~PathSmoother() {
	for (uint32_t uiIndex = 0; uiIndex < LeftVerts.iSize; ++uiIndex) {
		LeftVerts.GetAt(static_cast<int32_t>(uiIndex))->~UniqueVert();
		TheScrapHeap.Deallocate(LeftVerts.GetAt(static_cast<int32_t>(uiIndex)));
	}
	if (!RightVerts.IsEmpty()) {
		RightVerts.RemoveFast(0, 1u);
	}
	if (!RightVerts.IsEmpty()) {
		RightVerts.RemoveFast(0, 1u);
	}
	for (uint32_t i = 0; i < RightVerts.iSize; ++i) {
		RightVerts.GetAt(static_cast<int32_t>(i))->~UniqueVert();
		TheScrapHeap.Deallocate(RightVerts.GetAt(static_cast<int32_t>(i)));
	}
	LeftVerts.Clear(1);
	RightVerts.Clear(1);
	for (uint32_t j = 0; j < LeftEdges.iSize; ++j) {
		TheScrapHeap.Deallocate(LeftEdges.GetAt(static_cast<int32_t>(j)));
	}
	for (uint32_t k = 0; k < RightEdges.iSize; ++k) {
		TheScrapHeap.Deallocate(RightEdges.GetAt(static_cast<int32_t>(k)));
	}
	LeftEdges.Clear(1);
	RightEdges.Clear(1);
	for (uint32_t m = 0; m < PathEdges.iSize; ++m) {
		TheScrapHeap.Deallocate(PathEdges.GetAt(static_cast<int32_t>(m)));
	}
	PathEdges.Clear(1);
}

void PathSmoother::AddPathingNode(const PathingLocation& aLocation) {
	pSolution->AddPathingNode(aLocation, NiPoint3_ZERO);
}

void PathSmoother::BuildInternalArrays() {
	AddStartVert();
	for (uint32_t i = 0; i < Edges->GetSize(); ++i) {
		NiPoint3 kVert0;
		NiPoint3 kVert1;
		kVert0.~NiPoint3();
		::new (&kVert0) NiPoint3;
		kVert1.~NiPoint3();
		::new (&kVert1) NiPoint3;
		// The exe calls operator[] once per field it reads, before the call itself.
		(*Edges)[i].pNavMesh->GetEdgeVertices((*Edges)[i].sTriangle, (*Edges)[i].iEdgeIndex, &kVert0, &kVert1);
		if (i == 0) {
			NiPoint3 kToStart = kVert0 - StartLocation;
			NiPoint3 kToEdge = kVert0 - kVert1;
			NiPoint3 kCross = kToEdge.Cross(kToStart);
			// The exe compares the cross product's z with a double 0.0 (fcomp qword 0x1012060).
			if (kCross.z > 0.0)
				AddFirstEdge(kVert1, kVert0);
			else
				AddFirstEdge(kVert0, kVert1);
			continue;
		}
		// The exe's GetTail (0x6F2270) gives the last item's address; the item is the vertex pointer itself.
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
		(*Triangles)[i - 1].pNavMesh->GetMatchingEdge((*Triangles)[i - 1].sTriangle, (*Edges)[i - 1].iEdgeIndex,
			&pReturnNavMesh, &usOtherTriangle, &iOtherEdgeIndex);
		NiPoint3 kEdgeVert0;
		NiPoint3 kEdgeVert1;
		kEdgeVert0.~NiPoint3();
		::new (&kEdgeVert0) NiPoint3;
		kEdgeVert1.~NiPoint3();
		::new (&kEdgeVert1) NiPoint3;
		pReturnNavMesh->GetEdgeVertices(usOtherTriangle, iOtherEdgeIndex, &kEdgeVert0, &kEdgeVert1);
		NiPoint3 kFirst;
		NiPoint3 kSecond;
		kFirst.~NiPoint3();
		::new (&kFirst) NiPoint3;
		kSecond.~NiPoint3();
		::new (&kSecond) NiPoint3;
		if ((*static_cast<UniqueVert**>(RightVerts.GetTail()))->Location == kEdgeVert0) {
			kFirst = kEdgeVert1;
			kSecond = kEdgeVert0;
		} else {
			kFirst = kEdgeVert0;
			kSecond = kEdgeVert1;
		}
		if (kVert0 == kFirst) {
			AddRightVert(kVert1, i);
		} else if (kVert0 == kSecond) {
			AddLeftVert(kVert1, i);
		} else if (kVert1 == kFirst) {
			AddRightVert(kVert0, i);
		} else if (kVert1 == kSecond) {
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
	NiPoint3 kLeftNormal;
	NiPoint3 kRightNormal;
	(*Triangles)[0].pNavMesh->GetNormal(&kLeftNormal, (*Triangles)[0].sTriangle);
	(*Triangles)[1].pNavMesh->GetNormal(&kRightNormal, (*Triangles)[1].sTriangle);
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
	pBoundsEdge->bOpenEdge =
		kEdge.pNavMesh->GetTriangle(kEdge.sTriangle)->IsEdgeConnected(pBoundsEdge->hNavMeshEdge.iEdgeIndex);
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
	NiPoint3 kLeftNormal;
	NiPoint3 kRightNormal;
	(*Triangles)[aiEdgeIndex].pNavMesh->GetNormal(&kLeftNormal, (*Triangles)[aiEdgeIndex].sTriangle);
	(*Triangles)[aiEdgeIndex + 1].pNavMesh->GetNormal(&kRightNormal, (*Triangles)[aiEdgeIndex + 1].sTriangle);
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
	pBoundsEdge->bOpenEdge =
		kEdge.pNavMesh->GetTriangle(kEdge.sTriangle)->IsEdgeConnected(pBoundsEdge->hNavMeshEdge.iEdgeIndex);
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
	NiPoint3 kLeftNormal;
	NiPoint3 kRightNormal;
	(*Triangles)[aiEdgeIndex].pNavMesh->GetNormal(&kLeftNormal, (*Triangles)[aiEdgeIndex].sTriangle);
	(*Triangles)[aiEdgeIndex + 1].pNavMesh->GetNormal(&kRightNormal, (*Triangles)[aiEdgeIndex + 1].sTriangle);
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

// The exe's cleanup-only C++ EH frame is left out.
PathSmoother::UniqueVert* PathSmoother::CreateVert() {
	PathSmoother::UniqueVert* pvert =
		static_cast<PathSmoother::UniqueVert*>(TheScrapHeap.Allocate(sizeof(PathSmoother::UniqueVert),
			ScrapHeap::ALIGNMENT));
	PathSmoother::UniqueVert* v3 = static_cast<PathSmoother::UniqueVert*>(::operator new(64u, pvert));
	if (v3) {
		::new (v3) PathSmoother::UniqueVert(&TheScrapHeap);
	}
	return pvert;
}

// The exe's cleanup-only C++ EH frame is left out.
PathSmoother::BoundsEdge* PathSmoother::CreateBoundsEdge() {
	PathSmoother::BoundsEdge* pedge =
		static_cast<PathSmoother::BoundsEdge*>(TheScrapHeap.Allocate(sizeof(PathSmoother::BoundsEdge),
			ScrapHeap::ALIGNMENT));
	PathSmoother::BoundsEdge* v2 = static_cast<PathSmoother::BoundsEdge*>(::operator new(28u, pedge));
	if (v2) {
		::new (v2) PathSmoother::BoundsEdge;
	}
	return pedge;
}

PathSmoother::PathEdge* PathSmoother::CreatePathEdge() {
	void* apWhere = (&TheScrapHeap)->Allocate(sizeof(PathEdge), ScrapHeap::ALIGNMENT);
	void* v2 = ::operator new(0x58u, apWhere);
	if (v2) {
		::new (v2) PathSmoother::PathEdge;
	}
	return static_cast<PathEdge*>(apWhere);
}

bool PathSmoother::SmoothPath(PathingRequest* apRequest, const BSSimpleArray<NavMeshSearchNode, 1024>& aTriSolution,
	const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks, const NiPoint3& aStartLocation, const NiPoint3& aGoalLocation,
	PathingSolution* apSolution, BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray, bool abAddNodesIfFail) {
	pRequest = apRequest;
	StartLocation = aStartLocation;
	GoalLocation = aGoalLocation;
	Triangles = &aTriSolution;
	Edges = &aLinks;
	pSolution = apSolution;
	pBadNodes = apBadNodeArray;
	// The exe reads the radius through the shared float getter (0x69EF80).
	fSmoothingRadius = pRequest->GetActorRadius();
	if (aTriSolution.iSize == 1) {
		// A single triangle: the path is the start and the goal, both on it.
		NavMeshInfo* pStartInfo = aTriSolution.GetAt(0).pNavMesh->GetNavMeshInfo();
		AddPathingNode_006ED730(aStartLocation, pStartInfo);
		NavMeshInfo* pGoalInfo = aTriSolution.GetAt(0).pNavMesh->GetNavMeshInfo();
		AddPathingNode_006ED730(aGoalLocation, pGoalInfo);
		return true;
	}
	LeftVerts.SetReservedSize(2 * aLinks.iSize);
	RightVerts.SetReservedSize(2 * aLinks.iSize);
	LeftEdges.SetReservedSize(2 * aLinks.iSize);
	RightEdges.SetReservedSize(2 * aLinks.iSize);
	PathEdges.SetReservedSize(2 * aLinks.iSize);
	LeftVerts.Clear(0);
	RightVerts.Clear(0);
	LeftEdges.Clear(0);
	RightEdges.Clear(0);
	PathEdges.Clear(0);
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
		BuildFinalPath(1, 1);
		ComputeTangents();
	}
	return bCanFit;
}

// The exe's cleanup-only C++ EH frame is left out.
void PathSmoother::AddPathingNode_006ED730(const NiPoint3& aLocation, NavMeshInfo* apInfo) {
	FNVD_CONSTRUCTED_LOCAL(PathingLocation, apLocation, &PathingLocation::Destruct,
		static_cast<const NiPoint3*>(&aLocation), apInfo);
	AddPathingNode(apLocation);
}

bool PathSmoother::CheckLineOfSight(uint32_t aiFirstEdgeIndex, uint32_t aiLastEdgeIndex, const NiPoint3& aLineStart,
	const NiPoint3& aLineEnd) {
	while (aiFirstEdgeIndex <= aiLastEdgeIndex) {
		if (!NavMeshMath::Find2DIntersection3(PathEdges.GetAt(aiFirstEdgeIndex)->LeftShrunkLocation,
				PathEdges.GetAt(aiFirstEdgeIndex)->RightShrunkLocation, aLineStart, aLineEnd)) {
			return 0;
		}
		++aiFirstEdgeIndex;
	}
	return 1;
}

void PathSmoother::BuildShrunkEdgeLeftRightArrays() {
	// Each end vertex's shrunk location starts at its location; the exe looks the vertex up twice.
	PathSmoother::UniqueVert* pVert = LeftVerts.GetFirst();
	LeftVerts.GetFirst()->ShrunkLocation = pVert->Location;
	pVert = RightVerts.GetFirst();
	RightVerts.GetFirst()->ShrunkLocation = pVert->Location;
	for (uint32_t i = 1; i < LeftVerts.iSize - 1; i = ComputeShrunkPoint(i, fSmoothingRadius, 1)) {
	}
	for (uint32_t i = 1; i < RightVerts.iSize - 1; i = ComputeShrunkPoint(i, fSmoothingRadius, 0)) {
	}
	pVert = *static_cast<PathSmoother::UniqueVert**>(LeftVerts.GetTail());
	(*static_cast<PathSmoother::UniqueVert**>(LeftVerts.GetTail()))->ShrunkLocation = pVert->Location;
	pVert = *static_cast<PathSmoother::UniqueVert**>(RightVerts.GetTail());
	(*static_cast<PathSmoother::UniqueVert**>(RightVerts.GetTail()))->ShrunkLocation = pVert->Location;
}

uint32_t PathSmoother::ComputeShrunkPoint(uint32_t aiVertIndex, float afRadius, bool abLeft) {
	BSScrapArray<UniqueVert*>* pVerts;
	if (abLeft) {
		pVerts = &LeftVerts;
	} else {
		pVerts = &RightVerts;
	}
	BSScrapArray<BoundsEdge*>* pEdges;
	if (abLeft) {
		pEdges = &LeftEdges;
	} else {
		pEdges = &RightEdges;
	}
	// The directions to the previous and the next vertex, past a vertex at the same place.
	NiPoint3 kPrevDir;
	NiPoint3 kNextDir;
	UniqueVert* pCurrent = (*pVerts)[aiVertIndex];
	kPrevDir = (*pVerts)[aiVertIndex - 1]->Location - pCurrent->Location;
	pCurrent = (*pVerts)[aiVertIndex];
	kNextDir = (*pVerts)[aiVertIndex + 1]->Location - pCurrent->Location;
	if (kPrevDir == NiPoint3_ZERO && aiVertIndex > 1) {
		pCurrent = (*pVerts)[aiVertIndex];
		kPrevDir = (*pVerts)[aiVertIndex - 2]->Location - pCurrent->Location;
	}
	if (kNextDir == NiPoint3_ZERO && aiVertIndex < pVerts->iSize - 2) {
		pCurrent = (*pVerts)[aiVertIndex];
		kNextDir = (*pVerts)[aiVertIndex + 2]->Location - pCurrent->Location;
	}
	kPrevDir.z = 0.0f;
	kNextDir.z = 0.0f;
	kPrevDir.Unitize();
	kNextDir.Unitize();
	NiPoint3 kNextPerp = kNextDir.Cross(NiPoint3_UNIT_Z);
	NiPoint3 kPrevPerp = kPrevDir.Cross(NiPoint3_UNIT_Z);
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
	} else {
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
			} else {
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
	} else {
		NiPoint3 kOut = kNextPerp * afRadius;
		NiPoint3 kBack = -kNextDir;
		NiPoint3 kAlong = kBack * fPushScale;
		pVert->PushDelta = kAlong - kOut;
	}
	char cShrunk = 0;
	if ((*pEdges)[aiVertIndex - 1]->bOpenEdge && (*pEdges)[aiVertIndex]->bOpenEdge) {
		UniqueVert* pNextVert = (*pVerts)[aiVertIndex + 1];
		UniqueVert* pPrevVert = (*pVerts)[aiVertIndex - 1];
		// The exe copy-constructs the by-value handle in its argument slot (0x69A690); src copies it into a local
		// first, then passes that.
		NavMeshTriHandle kTriHandle;
		kTriHandle.~NavMeshTriHandle();
		::new (&kTriHandle) NavMeshTriHandle(pVert->hNavMeshTri);
		cShrunk = PushPoint(&pVert->Location, &pVert->ShrunkLocation, kTriHandle, &pVert->PushDelta,
			&pPrevVert->ShrunkLocation, &pNextVert->Location);
	} else {
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
		if (NavMeshMath::Find2DIntersection3(*pSegment1Start, pSegment1End->Location, *pSegment2Start,
				pSegment2End->Location)) {
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

char PathSmoother::PushPoint(NiPoint3* aPoint, NiPoint3* aRetLocation, NavMeshTriHandle aTri, NiPoint3* aPushDelta,
	NiPoint3* aPrevPoint, NiPoint3* aNextPoint) {
	NiPoint3* location;
	NiPoint3 v19;
	NiPoint3 v20;
	NiPoint3 v21;
	NiPoint3 v22;
	NiPoint3 v23;
	NiPoint3 v24;
	NiPoint3 v25;
	NiPoint3 result;
	int8_t v27;
	// The exe builds these with PathingLocation's constructors and destroys them with Destruct before each return:
	// no C++ constructor storing src's vtable.
	FNVD_INLINED_LOCAL(PathingLocation, aStartLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, apReturnIntersectionLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, aGoalLocation, std::nullopt, std::nullopt);
	TESWorldSpace* worldSpace = aTri.pNavMeshInfo->GetWorldSpace();
	TESObjectCELL* parentCell = aTri.pNavMeshInfo->GetCell();
	NiPoint3* v8 = &(result = aPushDelta->operator*(0.009999999776482582f));
	NiPoint3* v9 = &(v25 = aPoint->operator+(*v8));
	NiPoint3* v10 = &(v24 = v9->operator+(NiPoint3_UNIT_X));
	::new (&aStartLocation) PathingLocation(v10, parentCell, worldSpace);
	int32_t v31 = 0;
	TESWorldSpace* v18 = aTri.pNavMeshInfo->GetWorldSpace();
	TESObjectCELL* v16 = aTri.pNavMeshInfo->GetCell();
	NiPoint3* v11 = &(v23 = aPoint->operator-(*aPushDelta));
	NiPoint3* v12 = &(v22 = v11->operator+(NiPoint3_UNIT_X));
	::new (&aGoalLocation) PathingLocation(v12, v16, v18);
	v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)1));
	::new (&apReturnIntersectionLocation) PathingLocation;
	v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)2));
	if (Pathing::CheckLineOfSight(aStartLocation, aGoalLocation, &apReturnIntersectionLocation, nullptr)) {
		*aRetLocation = *aPoint;
		v27 = 1;
		if (Pathing::CheckLineOfSight(aGoalLocation, *aPrevPoint, nullptr, nullptr)) {
			if (!Pathing::CheckLineOfSight(aGoalLocation, *aNextPoint, nullptr, nullptr)) {
				v27 = 0;
			}
		} else {
			v27 = 0;
		}
		if (v27) {
			v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)1));
			apReturnIntersectionLocation.~PathingLocation();
			v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)0));
			aGoalLocation.~PathingLocation();
			v31 = (-1);
			aStartLocation.~PathingLocation();
			return 0;
		} else {
			*aRetLocation = *&(v21 = aPoint->operator+(*aPushDelta));
			v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)1));
			apReturnIntersectionLocation.~PathingLocation();
			v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)0));
			aGoalLocation.~PathingLocation();
			v31 = (-1);
			aStartLocation.~PathingLocation();
			return 1;
		}
	} else {
		location = &(v20 = (&apReturnIntersectionLocation)->GetLocation());
		*aRetLocation = *&(v19 = location->operator+(*aPushDelta));
		v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)1));
		apReturnIntersectionLocation.~PathingLocation();
		v31 = static_cast<int32_t>((static_cast<uint32_t>(v31) & 0xffffff00u) | static_cast<uint32_t>((uint8_t)0));
		aGoalLocation.~PathingLocation();
		v31 = (-1);
		aStartLocation.~PathingLocation();
		return 1;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool PathSmoother::CheckBoundsEdges() {
	PathSmoother::BoundsEdge* pprevLeftEdge = PathEdges.GetAt(0)->pPrevLeftEdge;
	PathSmoother::BoundsEdge* pprevRightEdge = PathEdges.GetAt(0)->pPrevRightEdge;
	for (uint32_t i = 1; i < PathEdges.iSize; ++i) {
		PathSmoother::BoundsEdge* pleftEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevLeftEdge;
		PathSmoother::BoundsEdge* prightEdge = PathEdges.GetAt(static_cast<int32_t>(i))->pPrevRightEdge;
		if (pleftEdge == pprevLeftEdge && !TestBoundsEdgeAgainstOtherBound(prightEdge, pleftEdge, false)) {
			if (pBadNodes) {
				FNVD_CONSTRUCTED_LOCAL(NavMeshSearchEdge, badEdge, nullptr);
				FNVD_CONSTRUCTED_LOCAL(NavMeshPtr, spmesh, &NavMeshPtr::Destruct);
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
				FNVD_CONSTRUCTED_LOCAL(NavMeshSearchEdge, badEdge, nullptr);
				FNVD_CONSTRUCTED_LOCAL(NavMeshPtr, spmesh, &NavMeshPtr::Destruct);
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
		if (NavMeshMath::Find2DIntersection3(pleftEdge->pStartVert->ShrunkLocation, pleftEdge->pEndVert->ShrunkLocation,
				prightEdge->pStartVert->ShrunkLocation, prightEdge->pEndVert->ShrunkLocation)) {
			break;
		}
	}
	if (pBadNodes) {
		FNVD_CONSTRUCTED_LOCAL(NavMeshSearchEdge, badEdge, nullptr);
		FNVD_CONSTRUCTED_LOCAL(NavMeshPtr, spmesh, &NavMeshPtr::Destruct);
		if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
			badEdge.pNavMesh = spmesh;
			badEdge.sTriangle = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle;
			badEdge.iEdgeIndex = PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.iEdgeIndex;
			pBadNodes->Add(badEdge);
		}
	}
	return false;
}

// The exe's cleanup-only C++ EH frame is left out.
bool PathSmoother::TestBoundsEdgeAgainstOtherBound(BoundsEdge* apEdge1, BoundsEdge* apEdge2, bool abLeft) {
	NiPoint3 kStart = apEdge1->pStartVert->ShrunkLocation;
	NiPoint3 kEnd = apEdge1->pEndVert->ShrunkLocation;
	PathSmoother::UniqueVert* pVert = apEdge2->pEndVert;
	if (!NavMeshMath::Find2DIntersection3(kStart, kEnd, pVert->Location, pVert->ShrunkLocation)) {
		return !NavMeshMath::Find2DIntersection3(kStart, kEnd, apEdge2->pStartVert->ShrunkLocation,
			apEdge2->pEndVert->ShrunkLocation);
	}
	NiPoint3 kPush = pVert->ShrunkLocation - pVert->Location;
	FNVD_CONSTRUCTED_LOCAL(PathingLocation, kVertLocation, &PathingLocation::Destruct, &pVert->ShrunkLocation,
		pVert->hNavMeshTri.pNavMeshInfo);
	if (Pathing::CheckLineOfSight(kVertLocation, pVert->ShrunkLocation + kPush, 0, 0)) {
		NiPoint3 kPushed1 = pVert->ShrunkLocation + kPush;
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, kStartLocation, &PathingLocation::Destruct, &kPushed1,
			pVert->hNavMeshTri.pNavMeshInfo);
		NiPoint3 kPushed2 = pVert->ShrunkLocation + kPush;
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, kEndLocation, &PathingLocation::Destruct, &kPushed2,
			pVert->hNavMeshTri.pNavMeshInfo);
		if (Pathing::CheckLineOfSight(kStartLocation, kStart, 0, 0) &&
			Pathing::CheckLineOfSight(kEndLocation, kEnd, 0, 0)) {
			PathSmoother::UniqueVert* pNewVert = SplitBoundsEdge(apEdge1->iEdgeIndex, abLeft);
			pNewVert->Location = pVert->ShrunkLocation;
			pNewVert->ShrunkLocation = pVert->ShrunkLocation + kPush * 0.1f;
			return true;
		}
	}
	return false;
}

void PathSmoother::SetLeftRightShrunkLocations() {
	for (uint32_t i = 0; i < PathEdges.iSize; ++i) {
		NiPoint3* p_ShrunkLocation = &PathEdges.GetAt(i)->pLeftVert->ShrunkLocation;
		PathEdges.GetAt(i)->LeftShrunkLocation = *p_ShrunkLocation;
		NiPoint3* v2 = &PathEdges.GetAt(i)->pRightVert->ShrunkLocation;
		PathEdges.GetAt(i)->RightShrunkLocation = *v2;
	}
}

void PathSmoother::ComputeCenters() {
	NiPoint3 v6;
	for (uint32_t i = 0;; ++i) {
		uint32_t FormFlags = PathEdges.iSize;
		if (i >= FormFlags) {
			break;
		}
		NiPoint3* p_RightShrunkLocation = &PathEdges.GetAt(i)->RightShrunkLocation;
		NiPoint3* v3 = &(v6 = (PathEdges.GetAt(i)->LeftShrunkLocation + *p_RightShrunkLocation) * 0.5f);
		PathEdges.GetAt(i)->Center = *v3;
	}
}

void PathSmoother::BuildBestPointOnEdge() {
	double v7;
	NiPoint3 v34;
	NiPoint3 v35;
	NiPoint3 v36;
	NiPoint3 v38;
	NiPoint3 v39;
	NiPoint3 v40;
	NiPoint3 v41;
	NiPoint3 v43;
	NiPoint3 v44;
	NiPoint3 v48;
	for (int32_t i = 0;; ++i) {
		NiPoint3* a1c;
		NiPoint3* v32;
		int32_t FormFlags = PathEdges.iSize;
		if (i >= FormFlags) {
			break;
		}
		NiPoint3 averagePointBefore(0.0f, 0.0f, 0.0f);
		uint32_t ipointCountBefore = 0;
		for (int32_t offset = -4; offset < 0; ++offset) {
			int32_t iedgeIndex = offset + i;
			if (offset + i == -1) {
				++ipointCountBefore;
				averagePointBefore.operator+=(StartLocation);
			} else {
				uint32_t v2 = PathEdges.iSize;
				if (iedgeIndex == v2) {
					++ipointCountBefore;
					averagePointBefore.operator+=(GoalLocation);
				} else {
					if (iedgeIndex >= 0) {
						int32_t v3 = PathEdges.iSize;
						if (iedgeIndex < v3) {
							++ipointCountBefore;
							averagePointBefore.operator+=(PathEdges.GetAt(iedgeIndex)->Center);
						}
					}
				}
			}
		}
		if (ipointCountBefore) {
			averagePointBefore.operator/=(static_cast<float>(ipointCountBefore));
		} else {
			averagePointBefore = StartLocation;
		}
		NiPoint3 averagePointAfter(0.0f, 0.0f, 0.0f);
		uint32_t ipointCountAfter = 0;
		for (int32_t offset_2 = 1; offset_2 <= 4; ++offset_2) {
			int32_t iedgeIndex_2 = offset_2 + i;
			if (offset_2 + i == -1) {
				++ipointCountAfter;
				averagePointAfter.operator+=(StartLocation);
			} else {
				uint32_t v4 = PathEdges.iSize;
				if (iedgeIndex_2 == v4) {
					++ipointCountAfter;
					averagePointAfter.operator+=(GoalLocation);
				} else {
					if (iedgeIndex_2 >= 0) {
						int32_t v5 = PathEdges.iSize;
						if (iedgeIndex_2 < v5) {
							++ipointCountAfter;
							averagePointAfter.operator+=(PathEdges.GetAt(iedgeIndex_2)->Center);
						}
					}
				}
			}
		}
		if (ipointCountAfter) {
			averagePointAfter.operator/=(static_cast<float>(ipointCountAfter));
		} else {
			averagePointAfter = GoalLocation;
		}
		NiPoint3* x = &PathEdges.GetAt(i)->RightShrunkLocation;
		NiPoint3* v6 =
			::new (&v48) NiPoint3(NavMeshMath::ClosestPointOnSegmentToLine3(PathEdges.GetAt(i)->LeftShrunkLocation, *x,
				averagePointBefore, averagePointAfter));
		PathEdges.GetAt(i)->BestPointOnEdge = *v6;
		NiPoint3* a1b = &PathEdges.GetAt(i)->LeftShrunkLocation;
		if (PathEdges.GetAt(i)->BestPointOnEdge.operator==(*a1b) ||
			(a1c = &PathEdges.GetAt(i)->RightShrunkLocation, PathEdges.GetAt(i)->BestPointOnEdge.operator==(*a1c))) {
			float a1e;
			NiPoint3 averagePointDelta = averagePointAfter - averagePointBefore;
			averagePointDelta.Unitize();
			NiPoint3* a1d = &PathEdges.GetAt(i)->RightShrunkLocation;
			NiPoint3 segmentDelta = PathEdges.GetAt(i)->LeftShrunkLocation - *a1d;
			segmentDelta.Unitize();
			if (i <= 0 ||
				(a1e = averagePointDelta.Dot(segmentDelta), (v7 = std::fabs(a1e), !(v7 > 0.800000011920929)))) {
				NiPoint3* a1j = &PathEdges.GetAt(i)->LeftShrunkLocation;
				if (PathEdges.GetAt(i)->BestPointOnEdge.operator==(*a1j)) {
					NiPoint3* a1k = &PathEdges.GetAt(i)->LeftShrunkLocation;
					NiPoint3 delta = PathEdges.GetAt(i)->RightShrunkLocation - *a1k;
					NiPoint3* v11 = ::new (&v39) NiPoint3(delta * 0.01f);
					PathEdges.GetAt(i)->BestPointOnEdge.operator+=(*v11);
				} else {
					NiPoint3* a1l = &PathEdges.GetAt(i)->RightShrunkLocation;
					if (PathEdges.GetAt(i)->BestPointOnEdge.operator==(*a1l)) {
						NiPoint3* a1m = &PathEdges.GetAt(i)->RightShrunkLocation;
						NiPoint3 delta_2 = PathEdges.GetAt(i)->LeftShrunkLocation - *a1m;
						NiPoint3* v12 = ::new (&v38) NiPoint3(delta_2 * 0.01f);
						PathEdges.GetAt(i)->BestPointOnEdge.operator+=(*v12);
					}
				}
			} else {
				NiPoint3* v9;
				NiPoint3* a1f = &PathEdges.GetAt(i - 1)->BestPointOnEdge;
				float fsqrDistFromLeftToPrevious = (PathEdges.GetAt(i)->LeftShrunkLocation - *a1f).SqrXYLength();
				NiPoint3* a1g = &PathEdges.GetAt(i - 1)->BestPointOnEdge;
				float fsqrDistFromRightToPrevious = (PathEdges.GetAt(i)->RightShrunkLocation - *a1g).SqrXYLength();
				if (!(fsqrDistFromRightToPrevious > static_cast<double>(fsqrDistFromLeftToPrevious))) {
					NiPoint3* a1i = ::new (&v41) NiPoint3(PathEdges.GetAt(i)->RightShrunkLocation * 0.99f);
					v9 = ::new (&v40) NiPoint3(PathEdges.GetAt(i)->LeftShrunkLocation * 0.01f + *a1i);
				} else {
					NiPoint3* a1h = ::new (&v44) NiPoint3(PathEdges.GetAt(i)->RightShrunkLocation * 0.01f);
					v9 = ::new (&v43) NiPoint3(PathEdges.GetAt(i)->LeftShrunkLocation * 0.99f + *a1h);
				}
				PathEdges.GetAt(i)->BestPointOnEdge = *v9;
			}
		}
		float frandomness = pRequest->fSmoothingRandomness;
		if (!(frandomness > 0.0)) {
			NiPoint3* a1o = &PathEdges.GetAt(i)->BestPointOnEdge;
			v32 = ::new (&v35) NiPoint3(PathEdges.GetAt(i)->LeftShrunkLocation - *a1o);
		} else {
			NiPoint3* a1n = &PathEdges.GetAt(i)->BestPointOnEdge;
			v32 = ::new (&v36) NiPoint3(PathEdges.GetAt(i)->RightShrunkLocation - *a1n);
		}
		NiPoint3* v37 = v32;
		NiPoint3 deltaToVert = *v32;
		frandomness = std::fabs(frandomness);
		float v31 = frandomness * 0.5;
		NiPoint3* v13 = ::new (&v34) NiPoint3(v31 * deltaToVert);
		PathEdges.GetAt(i)->BestPointOnEdge.operator+=(*v13);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the one-byte GetAt (0x6A62F0) out of line, folded with the unsigned char instantiation's; the
// template's GetAt stands in for it.
void PathSmoother::HandleWaterTriangles() {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	using waterTris_t = BSScrapArray<bool, 1024>;
	FNVD_INLINED_LOCAL(waterTris_t, waterTris, std::nullopt, std::nullopt);
	::new (&waterTris) BSScrapArray<bool, 1024>(PathEdges.iSize);
	for (uint32_t i = 0;; ++i) {
		uint32_t v2 = PathEdges.iSize;
		if (i >= v2) {
			break;
		}
		::new (&spmesh) NavMeshPtr;
		if (PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.pNavMeshInfo->GetNavMesh(spmesh)) {
			NavMeshTriangle* Triangle =
				spmesh.m_pObject->GetTriangle(PathEdges.GetAt(static_cast<int32_t>(i))->hNavMeshEdge.usTriangle);
			waterTris.GetAt(static_cast<int32_t>(i)) = Triangle->IsWater();
		}
		PathEdges.GetAt(static_cast<int32_t>(i))->eEnterLeaveWater = PathSmoother::ELW_NONE;
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	}
	bool aValue = 1;
	uint32_t ifirstWaterTri = waterTris.Find(aValue, 0, &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
	while (ifirstWaterTri != -1) {
		NiPoint3* p_GoalLocation;
		NiPoint3* p_BestPointOnEdge;
		PathSmoother::PathEdge* plastEdge;
		PathSmoother::PathEdge* pfirstEdge;
		bool v18 = 0;
		uint32_t ilastWaterTri =
			waterTris.Find(v18, ifirstWaterTri + 1, &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
		if (ilastWaterTri == -1) {
			ilastWaterTri = waterTris.iSize - 1;
		} else {
			--ilastWaterTri;
		}
		if (ifirstWaterTri) {
			pfirstEdge = PathEdges.GetAt(static_cast<int32_t>(ifirstWaterTri - 1));
		} else {
			pfirstEdge = 0;
		}
		PathSmoother::PathEdge* pfirstEdge_2 = pfirstEdge;
		uint32_t v5 = waterTris.iSize;
		if (ilastWaterTri == v5 - 1) {
			plastEdge = 0;
		} else {
			plastEdge = PathEdges.GetAt(static_cast<int32_t>(ilastWaterTri + 1));
		}
		PathSmoother::PathEdge* plastEdge_2 = plastEdge;
		if (pfirstEdge_2) {
			pfirstEdge_2->eEnterLeaveWater = PathSmoother::ELW_ENTER;
		}
		if (plastEdge_2) {
			plastEdge_2->eEnterLeaveWater = PathSmoother::ELW_LEAVE;
		}
		if (pfirstEdge_2) {
			p_BestPointOnEdge = &pfirstEdge_2->BestPointOnEdge;
		} else {
			p_BestPointOnEdge = &StartLocation;
		}
		NiPoint3* v17 = p_BestPointOnEdge;
		NiPoint3 firstLoc = *p_BestPointOnEdge;
		if (plastEdge_2) {
			p_GoalLocation = &plastEdge_2->BestPointOnEdge;
		} else {
			p_GoalLocation = &GoalLocation;
		}
		NiPoint3* v16 = p_GoalLocation;
		NiPoint3 lastLoc = *p_GoalLocation;
		float fdeltaZ = lastLoc.z - firstLoc.z;
		float fdeltaXY = (lastLoc - firstLoc).XYLength();
		for (uint32_t i_2 = ifirstWaterTri; i_2 <= ilastWaterTri; ++i_2) {
			float fthisEdgeDeltaXY =
				(PathEdges.GetAt(static_cast<int32_t>(i_2))->BestPointOnEdge - firstLoc).XYLength();
			double v6 = fdeltaZ * fthisEdgeDeltaXY / fdeltaXY + firstLoc.z;
			PathEdges.GetAt(static_cast<int32_t>(i_2))->BestPointOnEdge.z = v6;
		}
		uint32_t v7 = waterTris.iSize;
		if (ilastWaterTri == v7 - 1) {
			ifirstWaterTri = -1;
		} else {
			bool v13 = 1;
			ifirstWaterTri =
				waterTris.Find(v13, ilastWaterTri + 1, &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
		}
	}
	waterTris.~BSScrapArray<bool, 1024>();
}

// The exe's cleanup-only C++ EH frame is left out.
void PathSmoother::BuildStraightenedPath() {
	NavMeshInfo* NavMeshInfo_ = (&Triangles->GetFirst())->pNavMesh->GetNavMeshInfo();
	{
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, startLoc, &PathingLocation::Destruct, &StartLocation, NavMeshInfo_);
		StraightenedPath.Add(startLoc);
	}
	int32_t icurrentEdge = -1;
	int32_t inextEdge = 1;
	float ftotalNormalDelta = 0.0f;
	while (1) {
		uint32_t FormFlags = PathEdges.iSize;
		if (inextEdge >= static_cast<int32_t>(FormFlags + 1)) {
			break;
		}
		uint32_t v3 = PathEdges.iSize;
		if (inextEdge != v3) {
			PathSmoother::PathEdge** v4 = &PathEdges.GetAt(inextEdge - 1);
			ftotalNormalDelta = 1.0 - (*v4)->fTriangleNormalDelta + ftotalNormalDelta;
		}
		bool bAddNode = 0;
		if (!(ftotalNormalDelta > 0.20000000298023224)) {
			PathSmoother::PathEdge* v11;
			uint32_t v5 = PathEdges.iSize;
			if (inextEdge == v5) {
				v11 = 0;
			} else {
				v11 = PathEdges.GetAt(inextEdge);
			}
			PathSmoother::PathEdge* pnextEdge = v11;
			if (v11 && pnextEdge->eEnterLeaveWater) {
				bAddNode = 1;
			} else {
				NiPoint3* p_GoalLocation;
				NiPoint3* p_StartLocation;
				if (icurrentEdge == -1) {
					p_StartLocation = &StartLocation;
				} else {
					p_StartLocation = &PathEdges.GetAt(icurrentEdge)->BestPointOnEdge;
				}
				NiPoint3 testStartPoint = *p_StartLocation;
				uint32_t v6 = PathEdges.iSize;
				if (inextEdge == v6) {
					p_GoalLocation = &GoalLocation;
				} else {
					p_GoalLocation = &PathEdges.GetAt(inextEdge)->BestPointOnEdge;
				}
				NiPoint3 testEndPoint = *p_GoalLocation;
				if (!CheckLineOfSight(icurrentEdge + 1, inextEdge - 1, testStartPoint, testEndPoint)) {
					bAddNode = 1;
				}
			}
		} else {
			bAddNode = 1;
		}
		if (bAddNode) {
			NavMeshInfo* pNavMeshInfo = PathEdges.GetAt(inextEdge - 1)->hNavMeshEdge.pNavMeshInfo;
			FNVD_CONSTRUCTED_LOCAL(PathingLocation, loc, &PathingLocation::Destruct,
				&PathEdges.GetAt(inextEdge - 1)->BestPointOnEdge, pNavMeshInfo);
			StraightenedPath.Add(loc);
			ftotalNormalDelta = 0.0f;
			icurrentEdge = inextEdge - 1;
		}
		++inextEdge;
	}
	NavMeshInfo* v7 = (&Triangles->GetLast())->pNavMesh->GetNavMeshInfo();
	FNVD_CONSTRUCTED_LOCAL(PathingLocation, goalLoc, &PathingLocation::Destruct, &GoalLocation, v7);
	StraightenedPath.Add(goalLoc);
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the 16-byte items' GetAt (0x558DD0) out of line, folded with BSSimpleArray<NiColorA, 1024>'s; the
// template's GetAt stands in for it.
void PathSmoother::HandleAvoidNodes() {
	NiPoint3 v35;
	NiPoint3 v36;
	NiPoint3 v37;
	NiPoint3 v38;
	NiPoint3 v39;
	NiPoint3 v40;
	NiPoint3 v42;
	NiPoint3 result;
	// The tangent points on the avoid circle from the segment's start (akThisTangents) and from the first location
	// past the circle (akOtherTangents); the exe builds each pair with eh_vector_constructor_iterator.
	NiPoint3 akOtherTangents[2];
	NiPoint3 akThisTangents[2];
	PathSmoother::AvoidNode newNode;
	PathingAvoidNodeArray* pAvoidNodes = pRequest->GetAvoidNodeArray();
	if (pAvoidNodes) {
		using AvoidNodes_t = BSScrapArray<PathSmoother::AvoidNode, 1024>;
		FNVD_CONSTRUCTED_LOCAL(AvoidNodes_t, avoidNodes, &AvoidNodes_t::Destruct);
		for (uint32_t i = 0;; ++i) {
			uint32_t FormFlags = pAvoidNodes->iSize;
			if (i >= FormFlags) {
				break;
			}
			// The exe copies the node to the stack (rep movsd), then stores a true flag and tests it.
			PathingAvoidNode avoidNode = pAvoidNodes->GetAt(i);
			bool bAddNode = true;
			if (bAddNode) {
				::new (&newNode) NiBound;
				newNode.Center = *avoidNode.GetPoint1();
				newNode.fRadius = avoidNode.GetRadius();
				avoidNodes.Add(newNode);
			}
		}
		for (uint32_t j = 0;; ++j) {
			uint32_t v2 = avoidNodes.iSize;
			if (j >= v2) {
				break;
			}
			for (uint32_t i_2 = 0;; ++i_2) {
				uint32_t v3 = StraightenedPath.iSize;
				if (i_2 >= v3 - 1) {
					break;
				}
				float afCircleRadius = avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius;
				NiPoint3* v21 = &avoidNodes.GetAt(static_cast<int32_t>(j)).Center;
				NiPoint3* Location_ =
					::new (&result) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(i_2 + 1)))->GetLocation());
				if (NavMeshMath::SegmentCircleIntersection((&StraightenedPath.GetAt(static_cast<int32_t>(i_2)))
															   ->GetLocation(),
						*Location_, *v21, afCircleRadius)) {
					uint32_t ii;
					rt::eh_vector_constructor_iterator(akThisTangents, sizeof(NiPoint3), 2, &emptyPointConstructor);
					NiPoint3* v19 =
						::new (&v42) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(i_2)))->GetLocation());
					float v16 = avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius;
					NiPoint3* v5 = &avoidNodes.GetAt(static_cast<int32_t>(j)).Center;
					if (!NavMeshMath::ComputeTangentPointsToCircleFromPoint(v5, v16, v19, &akThisTangents[0].x,
							&akThisTangents[1].x)) {
						akThisTangents[1] = (&StraightenedPath.GetAt(static_cast<int32_t>(i_2)))->GetLocation();
						akThisTangents[0] = akThisTangents[1];
					}
					NiPoint3* v6 =
						::new (&v40) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(i_2)))->GetLocation());
					bool v58 = IsSegmentValid(v6, &akThisTangents[0]);
					NiPoint3* v7 =
						::new (&v39) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(i_2)))->GetLocation());
					bool v59 = IsSegmentValid(v7, &akThisTangents[1]);
					for (ii = i_2 + 1;; ++ii) {
						uint32_t v8 = StraightenedPath.iSize;
						if (ii >= v8) {
							break;
						}
						NiPoint3* afCircleRadiusa =
							::new (&v38) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(ii)))->GetLocation());
						if (!avoidNodes.GetAt(static_cast<int32_t>(j)).IsPointInNode(*afCircleRadiusa)) {
							break;
						}
					}
					uint32_t v10 = StraightenedPath.iSize;
					if (ii < v10) {
						rt::eh_vector_constructor_iterator(akOtherTangents, sizeof(NiPoint3), 2,
							&emptyPointConstructor);
						NiPoint3* v20 =
							::new (&v37) NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(ii)))->GetLocation());
						float v17 = avoidNodes.GetAt(static_cast<int32_t>(j)).fRadius;
						NiPoint3* v11 = &avoidNodes.GetAt(static_cast<int32_t>(j)).Center;
						if (NavMeshMath::ComputeTangentPointsToCircleFromPoint(v11, v17, v20, &akOtherTangents[0].x,
								&akOtherTangents[1].x)) {
							NiPoint3* v12 = ::new (&v36)
								NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(ii)))->GetLocation());
							bool v51 = IsSegmentValid(v12, &akOtherTangents[0]);
							NiPoint3* v13 = ::new (&v35)
								NiPoint3((&StraightenedPath.GetAt(static_cast<int32_t>(ii)))->GetLocation());
							bool v52 = IsSegmentValid(v13, &akOtherTangents[1]);
							bool v29 = v58 && v52;
							bool v48 = v29;
							bool v28 = v59 && v51;
							bool v49 = v28;
							char v50 = 0;
							int32_t ithisTangentIndex = 0;
							int32_t iotherTangentIndex = 1;
							if (!v48 || v49) {
								if (!v49 || v48) {
									if (v48 && v49) {
										float v47 = (akThisTangents[0] - akOtherTangents[1]).SqrLength();
										float v46 = (akThisTangents[1] - akOtherTangents[0]).SqrLength();
										if (!(v46 > static_cast<double>(v47))) {
											ithisTangentIndex = 1;
											iotherTangentIndex = 0;
										} else {
											ithisTangentIndex = 0;
											iotherTangentIndex = 1;
										}
										v50 = 1;
									}
								} else {
									ithisTangentIndex = 1;
									iotherTangentIndex = 0;
									v50 = 1;
								}
							} else {
								ithisTangentIndex = 0;
								iotherTangentIndex = 1;
								v50 = 1;
							}
							if (v50) {
								if (ii > i_2 + 1) {
									int32_t howMany = ii - i_2 - 1;
									StraightenedPath.RemoveRange(i_2 + 1, static_cast<uint32_t>(howMany), 0);
								}
								{
									PathingLocation* afCircleRadiusb =
										&StraightenedPath.GetAt(static_cast<int32_t>(i_2));
									FNVD_CONSTRUCTED_LOCAL(PathingLocation, apLocation, &PathingLocation::Destruct,
										akThisTangents[ithisTangentIndex], *afCircleRadiusb);
									StraightenedPath.Insert(i_2 + 1, apLocation);
								}
								{
									PathingLocation* afCircleRadiusc =
										&StraightenedPath.GetAt(static_cast<int32_t>(i_2));
									FNVD_CONSTRUCTED_LOCAL(PathingLocation, apMemory, &PathingLocation::Destruct,
										akOtherTangents[iotherTangentIndex], *afCircleRadiusc);
									StraightenedPath.Insert(i_2 + 2, apMemory);
								}
								break;
							}
							i_2 = ii - 1;
						}
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void PathSmoother::BuildFinalPath(bool abAddStartNode, bool abAddGoalNode) {
	if (abAddStartNode) {
		NavMeshInfo* NavMeshInfo_ = (&Triangles->GetFirst())->pNavMesh->GetNavMeshInfo();
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, startLoc, &PathingLocation::Destruct, &StartLocation, NavMeshInfo_);
		AddPathingNode(startLoc);
	}
	for (uint32_t i = 1; i < StraightenedPath.iSize - 1; ++i) {
		AddPathingNode(StraightenedPath.GetAt(static_cast<int32_t>(i)));
	}
	if (abAddGoalNode) {
		NavMeshInfo* v4 = (&Triangles->GetLast())->pNavMesh->GetNavMeshInfo();
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, goalLoc, &PathingLocation::Destruct, &GoalLocation, v4);
		AddPathingNode(goalLoc);
	}
}

void PathSmoother::ComputeTangents() {
	NiPoint3 v15;
	NiPoint3 v18;
	NiPoint3 result;
	for (uint32_t i = 1;; ++i) {
		NiPoint3 deltaToPrevious;
		NiPoint3 deltaToNext;
		uint32_t v1 = pSolution->GetNodeCount() - 1;
		if (i >= v1) {
			break;
		}
		PathingNode* Node = pSolution->GetNode(i);
		NiPoint3* afB = &(result = (&Node->Location)->GetLocation());
		PathingNode* v3 = pSolution->GetNode(i + 1);
		deltaToNext = (&v3->Location)->GetLocation() - *afB;
		float flengthToNext = deltaToNext.UnitizeGetLength();
		PathingNode* v5 = pSolution->GetNode(i - 1);
		NiPoint3* afBa = &(v18 = (&v5->Location)->GetLocation());
		PathingNode* v6 = pSolution->GetNode(i);
		deltaToPrevious = (&v6->Location)->GetLocation() - *afBa;
		float flengthToPrevious = deltaToPrevious.UnitizeGetLength();
		float fminLength = BSMIN(flengthToNext, flengthToPrevious);
		float fScalar = fminLength;
		NiPoint3* afBb = &(v15 = (deltaToNext + deltaToPrevious) * fScalar * 0.5f);
		pSolution->GetNode(i)->SetTangent(*afBb);
	}
}

bool PathSmoother::IsSegmentValid(NiPoint3* a1, NiPoint3* a2) {
	return PointInShrunkPath(a2) && SegmentClearOfEdges(a1, a2);
}

char PathSmoother::SegmentClearOfEdges(NiPoint3* a1, NiPoint3* a2) {
	for (uint32_t uiIndex = 0; uiIndex < LeftEdges.iSize; ++uiIndex) {
		if (NavMeshMath::Find2DIntersection3(*a1, *a2, LeftEdges.GetAt(uiIndex)->pStartVert->ShrunkLocation,
				LeftEdges.GetAt(uiIndex)->pEndVert->ShrunkLocation)) {
			return 0;
		}
	}
	for (uint32_t i = 0; i < RightEdges.iSize; ++i) {
		if (NavMeshMath::Find2DIntersection3(*a1, *a2, RightEdges.GetAt(i)->pStartVert->ShrunkLocation,
				RightEdges.GetAt(i)->pEndVert->ShrunkLocation)) {
			return 0;
		}
	}
	return 1;
}

bool PathSmoother::PointInShrunkPath(NiPoint3* aLocation) {
	float afZDiff;
	if (PathEdges.IsEmpty()) {
		return 1;
	}
	if (NavMeshMath::IsPointInsideTriangle(StartLocation, PathEdges.GetAt(0)->pRightVert->ShrunkLocation,
			PathEdges.GetAt(0)->pLeftVert->ShrunkLocation, *aLocation, afZDiff, 180.0f, 180.0f)) {
		return 1;
	}
	for (uint32_t uiIndex = 0; uiIndex < PathEdges.iSize - 1; ++uiIndex) {
		PathSmoother::UniqueVert** v3 = &PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pLeftVert;
		if (*v3 == PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pLeftVert) {
			if (NavMeshMath::IsPointInsideTriangle(PathEdges.GetAt(static_cast<int32_t>(uiIndex))
													   ->pLeftVert->ShrunkLocation,
					PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pRightVert->ShrunkLocation,
					PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pRightVert->ShrunkLocation, *aLocation, afZDiff,
					180.0f, 180.0f)) {
				return 1;
			}
		} else {
			PathSmoother::PathEdge* v4 = PathEdges.GetAt(static_cast<int32_t>(uiIndex));
			if (v4->pRightVert == PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pRightVert &&
				NavMeshMath::IsPointInsideTriangle(PathEdges.GetAt(static_cast<int32_t>(uiIndex))
													   ->pLeftVert->ShrunkLocation,
					PathEdges.GetAt(static_cast<int32_t>(uiIndex))->pRightVert->ShrunkLocation,
					PathEdges.GetAt(static_cast<int32_t>(uiIndex + 1))->pLeftVert->ShrunkLocation, *aLocation, afZDiff,
					180.0f, 180.0f)) {
				return 1;
			}
		}
	}
	// The exe takes the tail's right vertex first (the last argument pushed first), then its left one.
	NiPoint3& rRightLocation =
		(*static_cast<PathSmoother::PathEdge**>(PathEdges.GetTail()))->pRightVert->ShrunkLocation;
	NiPoint3& rLeftLocation = (*static_cast<PathSmoother::PathEdge**>(PathEdges.GetTail()))->pLeftVert->ShrunkLocation;
	return NavMeshMath::IsPointInsideTriangle(GoalLocation, rLeftLocation, rRightLocation, *aLocation, afZDiff, 180.0f,
		180.0f);
}

PathSmoother::UniqueVert* PathSmoother::DuplicateVert(uint32_t aiVertIndex, bool abLeft,
	const NiPoint3& aPreviousVertNewLocation, const NiPoint3& aNewVertLocation) {
	PathSmoother::UniqueVert** ppOriginalVert;
	if (abLeft) {
		ppOriginalVert = &LeftVerts.GetAt(static_cast<int32_t>(aiVertIndex));
	} else {
		ppOriginalVert = &RightVerts.GetAt(static_cast<int32_t>(aiVertIndex));
	}
	PathSmoother::UniqueVert* pOriginalVert = *ppOriginalVert;
	PathSmoother::UniqueVert* pNewVert = CreateVert();
	pNewVert->Location = pOriginalVert->Location;
	FNVD_RECONSTRUCT(NavMeshTriHandle)(&pNewVert->hNavMeshTri, pOriginalVert->hNavMeshTri);
	uint32_t iEdgeCount = pOriginalVert->EdgeList.iSize;
	pOriginalVert->ShrunkLocation = aPreviousVertNewLocation;
	pOriginalVert->PushDelta = pOriginalVert->ShrunkLocation - pOriginalVert->Location;
	pNewVert->ShrunkLocation = aNewVertLocation;
	pNewVert->PushDelta = pNewVert->ShrunkLocation - pNewVert->Location;
	if (iEdgeCount == 1) {
		pNewVert->EdgeList.Add(pOriginalVert->EdgeList.GetAt(0));
	} else {
		uint32_t iHalfEdgeCount = iEdgeCount >> 1;
		for (uint32_t i = iEdgeCount >> 1; i < iEdgeCount; ++i) {
			if (abLeft) {
				pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i))->pLeftVert = pNewVert;
			} else {
				pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i))->pRightVert = pNewVert;
			}
			pNewVert->EdgeList.Add(pOriginalVert->EdgeList.GetAt(static_cast<int32_t>(i)));
		}
		pOriginalVert->EdgeList.SetSize(iHalfEdgeCount, 1);
	}
	if (abLeft) {
		LeftVerts.Insert(aiVertIndex + 1, pNewVert);
	} else {
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
	} else {
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
		if (NavMeshMath::Find2DIntersection3(pOriginalVert->ShrunkLocation, pFirstEdge->pRightVert->ShrunkLocation,
				pNewVertCopy->Location, *pNewShrunkLocation)) {
			pNewPathEdge->pLeftVert = pNewVert;
			pNewPathEdge->pRightVert =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pRightVert;
			pNewPathEdge->pPrevLeftEdge = pBoundsEdge;
			pNewPathEdge->pPrevRightEdge =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pPrevRightEdge;
		} else {
			pNewPathEdge->pLeftVert = pOriginalVert;
			pNewPathEdge->pRightVert = pNewVert->EdgeList.GetFirst()->pRightVert;
			pNewPathEdge->pPrevLeftEdge =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pPrevLeftEdge;
			pNewPathEdge->pPrevRightEdge = pNewVert->EdgeList.GetFirst()->pPrevRightEdge;
		}
	} else {
		NiPoint3* pNewShrunkLocation = &pNewVert->ShrunkLocation;
		PathSmoother::UniqueVert* pNewVertCopy = pNewVert;
		PathSmoother::PathEdge* pFirstEdge = pNewVert->EdgeList.GetFirst();
		if (NavMeshMath::Find2DIntersection3(pOriginalVert->ShrunkLocation, pFirstEdge->pLeftVert->ShrunkLocation,
				pNewVertCopy->Location, *pNewShrunkLocation)) {
			pNewPathEdge->pLeftVert =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pLeftVert;
			pNewPathEdge->pRightVert = pNewVert;
			pNewPathEdge->pPrevLeftEdge =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pPrevLeftEdge;
			pNewPathEdge->pPrevRightEdge = pBoundsEdge;
		} else {
			pNewPathEdge->pLeftVert = pNewVert->EdgeList.GetFirst()->pLeftVert;
			pNewPathEdge->pRightVert = pOriginalVert;
			pNewPathEdge->pPrevLeftEdge = pNewVert->EdgeList.GetFirst()->pPrevLeftEdge;
			pNewPathEdge->pPrevRightEdge =
				(*static_cast<PathSmoother::PathEdge**>(pOriginalVert->EdgeList.GetTail()))->pPrevRightEdge;
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
	} else {
		pEdges = &RightEdges;
	}
	PathSmoother::BoundsEdge* pEdge = pEdges->operator[](aiEdgeIndex);
	PathSmoother::UniqueVert* pStartVert = pEdge->pStartVert;
	PathSmoother::UniqueVert* pEndVert = pEdge->pEndVert;
	PathSmoother::PathEdge* pPathEdge;
	if (pStartVert->EdgeList.IsEmpty()) {
		pPathEdge = pEndVert->EdgeList.GetFirst();
	} else {
		pPathEdge = *static_cast<PathSmoother::PathEdge**>(pStartVert->EdgeList.GetTail());
	}
	PathSmoother::UniqueVert* pNewVert = CreateVert();
	pNewVert->Location = pStartVert->Location;
	FNVD_RECONSTRUCT(NavMeshTriHandle)(&pNewVert->hNavMeshTri, pStartVert->hNavMeshTri);
	if (abLeft) {
		LeftVerts.Insert(aiEdgeIndex + 1, pNewVert);
	} else {
		RightVerts.Insert(aiEdgeIndex + 1, pNewVert);
	}
	PathSmoother::BoundsEdge* pBoundsEdge = CreateBoundsEdge();
	pBoundsEdge->pStartVert = pStartVert;
	pBoundsEdge->pEndVert = pNewVert;
	pBoundsEdge->hNavMeshEdge.Copy(pEdges->operator[](aiEdgeIndex)->hNavMeshEdge);
	pBoundsEdge->bOpenEdge = pEdges->operator[](aiEdgeIndex)->bOpenEdge;
	pEdges->operator[](aiEdgeIndex)->pStartVert = pNewVert;
	// The exe zero-extends the byte again before passing it on.
	InsertBoundsEdge(aiEdgeIndex, pBoundsEdge, abLeft);
	PathSmoother::PathEdge* pNewPathEdge = CreatePathEdge();
	pNewPathEdge->fTriangleNormalDelta = 1.0f;
	pNewPathEdge->hNavMeshEdge.Copy(pPathEdge->hNavMeshEdge);
	if (abLeft) {
		pNewPathEdge->pLeftVert = pNewVert;
		pNewPathEdge->pRightVert = pPathEdge->pRightVert;
		pNewPathEdge->pPrevRightEdge = pPathEdge->pPrevRightEdge;
		pNewPathEdge->pPrevLeftEdge = pBoundsEdge;
	} else {
		pNewPathEdge->pLeftVert = pPathEdge->pLeftVert;
		pNewPathEdge->pPrevLeftEdge = pPathEdge->pPrevLeftEdge;
		pNewPathEdge->pRightVert = pNewVert;
		pNewPathEdge->pPrevRightEdge = pBoundsEdge;
	}
	if (pStartVert->EdgeList.IsEmpty()) {
		PathSmoother::PathEdge* pFirstEdge = pEndVert->EdgeList.GetFirst();
		InsertPathEdge(static_cast<uint32_t>(pFirstEdge->iPathEdgeIndex), pNewPathEdge);
	} else {
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
	} else {
		RightEdges.Insert(aiInsertIndex, apEdge);
		for (uint32_t i = aiInsertIndex; i < RightEdges.iSize; ++i) {
			RightEdges.GetAt(static_cast<int32_t>(i))->iEdgeIndex = i;
		}
	}
}

NavMeshSearchEdge::NavMeshSearchEdge() {}
