#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <new>

NiPointer<NiNode> PathingSearchRayCast::spRootDrawNode;

PathingSearchRayCast::PathingSearchRayCast()
	: StartLocation(), GoalLocation(), pHavokWorld(nullptr), pPickData(nullptr), pCollector(nullptr), iRayCastCount(0),
	  fMaxDrop(fJumpFallHeightMin.Float()), fMaxStep(32.0f) {}

PathingSearchRayCast::~PathingSearchRayCast() {}

RAYCASTPATH_RESULT PathingSearchRayCast::BuildPath(RayCastPathRequest& aRequest, RayCastPath& aRetPath) {
	NiPoint3 v37;
	NiPoint3 v40;
	NiPoint3 v42;
	NiPoint3 v43;
	NiPoint3 v44;
	NiPoint3 v46;
	NiPoint3 dst;
	NiPoint3 v50;
	NiPoint3 result;
	this->eType = aRequest.eType;
	pRequest = aRequest.pInitialPathRequest;
	aRetPath.eType = this->eType;
	float a1 = pRequest->fActorRadius;
	fActorRadius = BSMAX(a1, 32.0f);
	iRayCastCount = 0;
	RAYCAST_REQUEST_TYPE eType = aRequest.eType;
	if (aRequest.eType == RCRT_START || eType == RCRT_GOAL) {
		GoalLocation.operator=(aRequest.GoalLocation);
		ComputeBounds(aRequest.Edges);
	} else {
		if (eType == RCRT_NOMESH) {
			GoalLocation.operator=(aRequest.GoalLocation);
			StartLocation.operator=(aRequest.StartLocation);
			NiPoint3* a2 = &(result = StartLocation.GetLocation());
			NiPoint3 delta = GoalLocation.GetLocation() - *a2;
			float a2a = std::fabs(delta.y);
			float a1a = std::fabs(delta.x);
			float fmax = BSMAX(a1a, a2a);
			float a2b = GoalLocation.GetLocation().z;
			float fzmin = BSMIN(StartLocation.GetLocation().z, a2b);
			float a2c = GoalLocation.GetLocation().z;
			float fzmax = BSMAX(StartLocation.GetLocation().z, a2c);
			NiPoint3* x = &(v50 = StartLocation.GetLocation());
			NiPoint3 center = (GoalLocation.GetLocation() + *x) * 0.5f;
			float v32 = fmax * 0.5;
			NiPoint3* v8 = ::new (&dst) NiPoint3(v32, v32, 0.0f);
			BoundsMin = center - *v8;
			float v31 = fmax * 0.5;
			NiPoint3* v9 = ::new (&v46) NiPoint3(v31, v31, 0.0f);
			BoundsMax = center + *v9;
			BoundsMin.z = fzmin;
			BoundsMax.z = fzmax;
			if (InitRayCasts()) {
				NiPoint3* v11;
				TESObjectCELL* Cell;
				NiPoint3* v13;
				TESObjectCELL* v14;
				if (StartLocation.GetCell() &&
					(GoalLocation.GetCell() &&
						((v11 = &(v44 = StartLocation.GetLocation()),
							 (Cell = StartLocation.GetCell(), static_cast<double>(v11->z) < Cell->GetWaterHeight())) &&
							(v13 = &(v43 = GoalLocation.GetLocation()),
								(v14 = GoalLocation.GetCell(),
									static_cast<double>(v13->z) < v14->GetWaterHeight()))))) {
					NiPoint3* a2d = &(v42 = GoalLocation.GetLocation());
					if (UnderWaterRayCast(StartLocation.GetLocation(), *a2d)) {
						aRetPath.Nodes.Add(aRequest.StartLocation);
						aRetPath.Nodes.Add(aRequest.GoalLocation);
						aRetPath.eResult = RCP_SUCCESS;
					} else {
						aRetPath.eResult = RCP_FAILED;
					}
				} else {
					NiPoint3* a2e = &(v40 = aRequest.GoalLocation.GetLocation());
					if (RayCastRecurse(aRequest.StartLocation.GetLocation(), *a2e)) {
						aRetPath.Nodes.Add(aRequest.StartLocation);
						aRetPath.Nodes.Add(aRequest.GoalLocation);
						aRetPath.eResult = RCP_SUCCESS;
					} else {
						aRetPath.eResult = RCP_FAILED;
					}
				}
				CleanUpRayCasts();
				return aRetPath.eResult;
			} else {
				aRetPath.eResult = RCP_FAILED;
				return RCP_FAILED;
			}
		}
	}
	if (!InitRayCasts()) {
		return RCP_FAILED;
	}
	iRayCastCount = 0;
	while (iRayCastCount < 10 && !aRequest.Edges.IsEmpty()) {
		bool braycastPassed = 0;
		if (this->eType) {
			NiPoint3* v18 = &(v37 = GoalLocation.GetLocation());
			braycastPassed = RayCastRecurse((&aRequest.Edges.GetFirst())->Location, *v18);
		} else {
			NiPoint3* a2f = &(&aRequest.Edges.GetFirst())->Location;
			braycastPassed = RayCastRecurse(GoalLocation.GetLocation(), *a2f);
		}
		if (braycastPassed) {
			NavMeshEdgeLocation edge(aRequest.Edges.GetFirst());
			NavMeshPtr spmesh;
			if (edge.pNavMeshInfo->GetNavMesh(spmesh)) {
				NiPoint3 v35;
				NiPoint3 pointOnEdge = edge.Location;
				NiPoint3 center_2 = spmesh.m_pObject->GetCenter(edge.usTriangle);
				pointOnEdge.operator+=((v35 = (center_2 - pointOnEdge) * 0.1f));
				PathingLocation startLoc(pointOnEdge, edge.pNavMeshInfo, edge.usTriangle);
				aRetPath.Nodes.Add(GoalLocation);
				aRetPath.Nodes.Add(startLoc);
				PathingLocation* Tail = static_cast<PathingLocation*>(aRetPath.Nodes.GetTail());
				Tail->ResolveNavMeshInfo(0);
				aRetPath.eResult = RCP_SUCCESS;
			} else {
				aRequest.Edges.Clear(1);
				aRetPath.eResult = RCP_FAILED;
			}
			break;
		}
		aRequest.Edges.Remove(0, 1);
		if (aRequest.Edges.IsEmpty()) {
			aRetPath.eResult = RCP_FAILED;
			break;
		}
		aRetPath.eResult = RCP_NONE;
	}
	CleanUpRayCasts();
	return aRetPath.eResult;
}

void PathingSearchRayCast::FreeCache() {
	hkDeallocateStack<char>(pCache);
	pCache = nullptr;
}

void PathingSearchRayCast::CreateCache() {
	hkAabb aabb;
	NI2HK(aabb.m_min, this->BoundsMin);
	NI2HK(aabb.m_max, this->BoundsMax);
	TESObjectCELL* Cell = (&this->pRequest->Origin)->GetCell();
	bhkWorld* pw = Cell->GetbhkWorld();
	pw->MarkForWrite();
	hkpBroadPhase* NumberOfItemsActivated = pw->operator*()->m_broadPhase;
	int32_t icacheSize = NumberOfItemsActivated->getAabbCacheSize();
	char* v2 = hkAllocateStack<char>(icacheSize, nullptr);
	this->pCache = v2;
	hkpBroadPhase* v3 = pw->operator*()->m_broadPhase;
	v3->calcAabbCache(aabb, this->pCache);
	pw->MarkForWrite();
}

bool PathingSearchRayCast::InitRayCasts() {
	hkpClosestRayHitCollector* pNewCollector;
	bhkPickData* pNewPickData;
	CFilter kFilter;
	TESObjectCELL* pcell = (&pRequest->Origin)->GetCell();
	if (!pcell) {
		return 0;
	}
	pHavokWorld = pcell->GetbhkWorld();
	if (!pHavokWorld) {
		return 0;
	}
	kFilter.~CFilter();
	::new (&kFilter) CFilter(0);
	kFilter.SetLayer(L_PATHINGPICK);
	kFilter.SetGroup(3u);
	CreateCache();
	void* apWhere = ourMemalign(16, sizeof(bhkPickData));
	bhkPickData* apMemory = static_cast<bhkPickData*>(::operator new(sizeof(bhkPickData), apWhere));
	if (apMemory) {
		pNewPickData = ::new (apMemory) bhkPickData;
	} else {
		pNewPickData = 0;
	}
	pPickData = pNewPickData;
	pPickData->SetFilter(kFilter);
	pPickData->pCache = pCache;
	void* v5 = ourMemalign(16, sizeof(hkpClosestRayHitCollector));
	hkpClosestRayHitCollector* v6 =
		static_cast<hkpClosestRayHitCollector*>(::operator new(sizeof(hkpClosestRayHitCollector), v5));
	if (v6) {
		pNewCollector = ::new (v6) hkpClosestRayHitCollector;
	} else {
		pNewCollector = 0;
	}
	pCollector = pNewCollector;
	pPickData->SetClosestCollector(pCollector);
	return 1;
}

void PathingSearchRayCast::CleanUpRayCasts() {
	FreeCache();
	if (pCollector) {
		pCollector->~hkpClosestRayHitCollector();
		ourMemalignFree(pCollector);
		pCollector = 0;
	}
	if (pPickData) {
		ourMemalignFree(pPickData);
		pPickData = 0;
	}
}

bool PathingSearchRayCast::RayCastRecurse(const NiPoint3& aStart, const NiPoint3& aGoal) {
	NiPoint3 v8;
	NiPoint3 result;
	NiPoint3 v14;
	NiPoint3 v15;
	result = aGoal - aStart;
	float Length = result.UnitizeGetLength();
	int32_t v17 = static_cast<int32_t>(Length / fActorRadius);
	NiPoint3 v16 = aStart;
	for (int32_t i = 0; i < v17; ++i) {
		NiPoint3 v9;
		NiPoint3 ecx0;
		NiPoint3* v3 = &(v8 = result * fActorRadius);
		v9 = v16 + *v3;
		v9.z = v16.z;
		if (!RayCast(v16, v9, ecx0)) {
			return 0;
		}
		v16 = ecx0;
	}
	v14.x = aGoal.x;
	v14.y = aGoal.y;
	float z = aGoal.z;
	v14.z = v16.z;
	if (!RayCast(v16, v14, v15)) {
		return 0;
	}
	float _Xx = v15.z - aGoal.z;
	return std::fabs(_Xx) < 64.0;
}

bool PathingSearchRayCast::RayCast(const NiPoint3& aStart, const NiPoint3& aGoal, NiPoint3& aRetLoc) {
	float finitialOffset = fMaxStep;
	NiPoint3 horizontalPickStart = aStart + NiPoint3_UNIT_Z * finitialOffset;
	NiPoint3 horizontalPickEnd = aGoal + NiPoint3_UNIT_Z * finitialOffset;
	pPickData->reset();
	pCollector->reset();
	++iRayCastCount;
	pPickData->NiSetFrom(horizontalPickStart);
	pPickData->NiSetTo(horizontalPickEnd);
	if (pHavokWorld->PickObject(*pPickData)) {
		return 0;
	}
	++iRayCastCount;
	float v11 = fMaxDrop + fMaxStep;
	NiPoint3 verticalPickEnd = horizontalPickEnd - NiPoint3_UNIT_Z * v11;
	pPickData->reset();
	pCollector->reset();
	pPickData->NiSetFrom(horizontalPickEnd);
	pPickData->NiSetTo(verticalPickEnd);
	if (!pHavokWorld->PickObject(*pPickData)) {
		return 0;
	}
	float fScalar = (&pCollector->m_rayHit)->m_hitFraction;
	NiPoint3 pickEnd = horizontalPickEnd + (verticalPickEnd - horizontalPickEnd) * fScalar;
	aRetLoc = pickEnd;
	return 1;
}

bool PathingSearchRayCast::UnderWaterRayCast(const NiPoint3& aStart, const NiPoint3& aGoal) {
	pPickData->reset();
	pCollector->reset();
	++iRayCastCount;
	pPickData->NiSetFrom(aStart);
	pPickData->NiSetTo(aGoal);
	return !pHavokWorld->PickObject(*pPickData);
}

void PathingSearchRayCast::ComputeBounds(const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges) {
	NiPoint3 v34;
	NiPoint3 dst;
	BoundsMin = *::new (&dst) NiPoint3(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
	BoundsMax = *::new (&v34) NiPoint3(-3.4028235e+38f, -3.4028235e+38f, -3.4028235e+38f);
	for (uint32_t auiIndex = 0;; ++auiIndex) {
		uint32_t uiSize = aEdges.iSize;
		if (auiIndex >= uiSize) {
			break;
		}
		float a2 = BoundsMin.x;
		const NavMeshEdgeLocation* v3 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMin.x = BSMIN(v3->Location.x, a2);
		float a2a = BoundsMin.y;
		const NavMeshEdgeLocation* v4 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMin.y = BSMIN(v4->Location.y, a2a);
		float a2b = BoundsMin.z;
		const NavMeshEdgeLocation* v5 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMin.z = BSMIN(v5->Location.z, a2b);
		float a2c = BoundsMax.x;
		const NavMeshEdgeLocation* v6 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMax.x = BSMAX(v6->Location.x, a2c);
		float a2d = BoundsMax.y;
		const NavMeshEdgeLocation* v7 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMax.y = BSMAX(v7->Location.y, a2d);
		float a2e = BoundsMax.z;
		const NavMeshEdgeLocation* v8 = &aEdges.GetAt(static_cast<int32_t>(auiIndex));
		BoundsMax.z = BSMAX(v8->Location.z, a2e);
	}
	float a2f = BoundsMin.x;
	BoundsMin.x = BSMIN(GoalLocation.GetLocation().x, a2f);
	float a2g = BoundsMin.y;
	BoundsMin.y = BSMIN(GoalLocation.GetLocation().y, a2g);
	float a2h = BoundsMin.z;
	BoundsMin.z = BSMIN(GoalLocation.GetLocation().z, a2h);
	float a2i = BoundsMax.x;
	BoundsMax.x = BSMAX(GoalLocation.GetLocation().x, a2i);
	float a2j = BoundsMax.y;
	BoundsMax.y = BSMAX(GoalLocation.GetLocation().y, a2j);
	float a2k = BoundsMax.z;
	BoundsMax.z = BSMAX(GoalLocation.GetLocation().z, a2k);
}

void PathingSearchRayCast::operator delete(void* apMemory) {
	ourFree(apMemory);
}
