#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSMain/ThreadSafeStructures.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
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
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
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

#include <cmath>
#include <limits>
#include <new>

NiPointer<NiNode> PathingSearchRayCast::spRootDrawNode;

PathingSearchRayCast::PathingSearchRayCast()
  : StartLocation(), GoalLocation(), pHavokWorld(nullptr), pPickData(nullptr), pCollector(nullptr), iRayCastCount(0),
    fMaxDrop(fJumpFallHeightMin.Float()), fMaxStep(32.0f) {}

PathingSearchRayCast::~PathingSearchRayCast() {}

RAYCASTPATH_RESULT PathingSearchRayCast::BuildPath(RayCastPathRequest& aRequest, RayCastPath& aRetPath) {
    this->eType = aRequest.eType;
    pRequest = aRequest.pInitialPathRequest;
    aRetPath.eType = this->eType;
    fActorRadius = BSMAX(pRequest->fActorRadius, 32.0f);
    iRayCastCount = 0;
    RAYCAST_REQUEST_TYPE eType = aRequest.eType;
    if (aRequest.eType == RCRT_START || eType == RCRT_GOAL) {
        GoalLocation = aRequest.GoalLocation;
        ComputeBounds(aRequest.Edges);
    }
    else if (eType == RCRT_NOMESH) {
        GoalLocation = aRequest.GoalLocation;
        StartLocation = aRequest.StartLocation;
        NiPoint3 kStart = StartLocation.GetLocation();
        NiPoint3 delta = GoalLocation.GetLocation() - kStart;
        float fmax = BSMAX(std::fabs(delta.x), std::fabs(delta.y));
        float fGoalZ = GoalLocation.GetLocation().z;
        float fzmin = BSMIN(StartLocation.GetLocation().z, fGoalZ);
        fGoalZ = GoalLocation.GetLocation().z;
        float fzmax = BSMAX(StartLocation.GetLocation().z, fGoalZ);
        kStart = StartLocation.GetLocation();
        NiPoint3 center = (GoalLocation.GetLocation() + kStart) * 0.5f;
        float fHalfExtent = fmax * 0.5;
        BoundsMin = center - NiPoint3(fHalfExtent, fHalfExtent, 0.0f);
        fHalfExtent = fmax * 0.5;
        BoundsMax = center + NiPoint3(fHalfExtent, fHalfExtent, 0.0f);
        BoundsMin.z = fzmin;
        BoundsMax.z = fzmax;
        if (InitRayCasts()) {
            NiPoint3 kGoal;
            TESObjectCELL* pCell;
            if (StartLocation.GetCell() &&
                GoalLocation.GetCell() &&
                (kStart = StartLocation.GetLocation(),
                 pCell = StartLocation.GetCell(),
                 static_cast<double>(kStart.z) < pCell->GetWaterHeight()) &&
                (kGoal = GoalLocation.GetLocation(),
                 pCell = GoalLocation.GetCell(),
                 static_cast<double>(kGoal.z) < pCell->GetWaterHeight())) {
                kGoal = GoalLocation.GetLocation();
                if (UnderWaterRayCast(StartLocation.GetLocation(), kGoal)) {
                    aRetPath.Nodes.Add(aRequest.StartLocation);
                    aRetPath.Nodes.Add(aRequest.GoalLocation);
                    aRetPath.eResult = RCP_SUCCESS;
                }
                else {
                    aRetPath.eResult = RCP_FAILED;
                }
            }
            else {
                kGoal = aRequest.GoalLocation.GetLocation();
                if (RayCastRecurse(aRequest.StartLocation.GetLocation(), kGoal)) {
                    aRetPath.Nodes.Add(aRequest.StartLocation);
                    aRetPath.Nodes.Add(aRequest.GoalLocation);
                    aRetPath.eResult = RCP_SUCCESS;
                }
                else {
                    aRetPath.eResult = RCP_FAILED;
                }
            }
            CleanUpRayCasts();
            return aRetPath.eResult;
        }

        aRetPath.eResult = RCP_FAILED;
        return RCP_FAILED;
    }
    if (!InitRayCasts()) {
        return RCP_FAILED;
    }
    iRayCastCount = 0;
    while (iRayCastCount < 10 && !aRequest.Edges.IsEmpty()) {
        bool braycastPassed = false;
        if (this->eType) {
            NiPoint3 kGoal = GoalLocation.GetLocation();
            braycastPassed = RayCastRecurse(aRequest.Edges.GetFirst().Location, kGoal);
        }
        else {
            const NiPoint3& kEdgeLocation = aRequest.Edges.GetFirst().Location;
            braycastPassed = RayCastRecurse(GoalLocation.GetLocation(), kEdgeLocation);
        }
        if (braycastPassed) {
            NavMeshEdgeLocation edge(aRequest.Edges.GetFirst());
            NavMeshPtr spmesh;
            if (edge.pNavMeshInfo->GetNavMesh(spmesh)) {
                NiPoint3 pointOnEdge = edge.Location;
                NiPoint3 center = spmesh->GetCenter(edge.usTriangle);
                pointOnEdge += (center - pointOnEdge) * 0.1f;
                PathingLocation startLoc(pointOnEdge, edge.pNavMeshInfo, edge.usTriangle);
                aRetPath.Nodes.Add(GoalLocation);
                aRetPath.Nodes.Add(startLoc);
                PathingLocation* ptail = static_cast<PathingLocation*>(aRetPath.Nodes.GetTail());
                ptail->ResolveNavMeshInfo(nullptr);
                aRetPath.eResult = RCP_SUCCESS;
            }
            else {
                aRequest.Edges.Clear(true);
                aRetPath.eResult = RCP_FAILED;
            }
            break;
        }
        aRequest.Edges.Remove(0, true);
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
    TESObjectCELL* pCell = this->pRequest->Origin.GetCell();
    bhkWorld* pw = pCell->GetbhkWorld();
    pw->MarkForWrite();
    hkpBroadPhase* pBroadPhase = (**pw)->m_broadPhase;
    int32_t icacheSize = pBroadPhase->getAabbCacheSize();
    this->pCache = hkAllocateStack<char>(icacheSize, nullptr);
    pBroadPhase = (**pw)->m_broadPhase;
    pBroadPhase->calcAabbCache(aabb, this->pCache);
    pw->MarkForWrite();
}

bool PathingSearchRayCast::InitRayCasts() {
    TESObjectCELL* pcell = pRequest->Origin.GetCell();
    if (!pcell) {
        return false;
    }
    pHavokWorld = pcell->GetbhkWorld();
    if (!pHavokWorld) {
        return false;
    }
    CFilter kFilter(0);
    kFilter.SetLayer(L_PATHINGPICK);
    kFilter.SetGroup(3u);
    CreateCache();
    pPickData = ::new bhkPickData;
    pPickData->SetFilter(kFilter);
    pPickData->pCache = pCache;
    pCollector = ::new hkpClosestRayHitCollector;
    pPickData->SetClosestCollector(pCollector);
    return true;
}

void PathingSearchRayCast::CleanUpRayCasts() {
    FreeCache();
    if (pCollector) {
        ::delete pCollector;
        pCollector = nullptr;
    }
    if (pPickData) {
        ::delete pPickData;
        pPickData = nullptr;
    }
}

bool PathingSearchRayCast::RayCastRecurse(const NiPoint3& aStart, const NiPoint3& aGoal) {
    NiPoint3 kDirection = aGoal - aStart;
    float fLength = kDirection.UnitizeGetLength();
    int32_t iSteps = static_cast<int32_t>(fLength / fActorRadius);
    NiPoint3 kCurrent = aStart;
    for (int32_t i = 0; i < iSteps; ++i) {
        NiPoint3 kNext;
        NiPoint3 kHit;
        kNext = kCurrent + kDirection * fActorRadius;
        kNext.z = kCurrent.z;
        if (!RayCast(kCurrent, kNext, kHit)) {
            return false;
        }
        kCurrent = kHit;
    }
    NiPoint3 kGoal;
    kGoal.x = aGoal.x;
    kGoal.y = aGoal.y;
    kGoal.z = kCurrent.z;
    NiPoint3 kHit;
    if (!RayCast(kCurrent, kGoal, kHit)) {
        return false;
    }
    float fZDiff = kHit.z - aGoal.z;
    return std::fabs(fZDiff) < 64.0;
}

bool PathingSearchRayCast::RayCast(const NiPoint3& aStart, const NiPoint3& aGoal, NiPoint3& aRetLoc) {
    float finitialOffset = fMaxStep;
    NiPoint3 horizontalPickStart = aStart + NiPoint3::UNIT_Z * finitialOffset;
    NiPoint3 horizontalPickEnd = aGoal + NiPoint3::UNIT_Z * finitialOffset;
    pPickData->reset();
    pCollector->reset();
    ++iRayCastCount;
    pPickData->NiSetFrom(horizontalPickStart);
    pPickData->NiSetTo(horizontalPickEnd);
    if (pHavokWorld->PickObject(*pPickData)) {
        return false;
    }
    ++iRayCastCount;
    float fDropDistance = fMaxDrop + fMaxStep;
    NiPoint3 verticalPickEnd = horizontalPickEnd - NiPoint3::UNIT_Z * fDropDistance;
    pPickData->reset();
    pCollector->reset();
    pPickData->NiSetFrom(horizontalPickEnd);
    pPickData->NiSetTo(verticalPickEnd);
    if (!pHavokWorld->PickObject(*pPickData)) {
        return false;
    }
    float fScalar = pCollector->m_rayHit.m_hitFraction;
    NiPoint3 pickEnd = horizontalPickEnd + (verticalPickEnd - horizontalPickEnd) * fScalar;
    aRetLoc = pickEnd;
    return true;
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
    BoundsMin = NiPoint3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    BoundsMax = NiPoint3(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    );
    float fBound;
    for (uint32_t uiIndex = 0; uiIndex < aEdges.iSize; ++uiIndex) {
        fBound = BoundsMin.x;
        const NavMeshEdgeLocation* pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMin.x = BSMIN(pEdge->Location.x, fBound);
        fBound = BoundsMin.y;
        pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMin.y = BSMIN(pEdge->Location.y, fBound);
        fBound = BoundsMin.z;
        pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMin.z = BSMIN(pEdge->Location.z, fBound);
        fBound = BoundsMax.x;
        pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMax.x = BSMAX(pEdge->Location.x, fBound);
        fBound = BoundsMax.y;
        pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMax.y = BSMAX(pEdge->Location.y, fBound);
        fBound = BoundsMax.z;
        pEdge = &aEdges.GetAt(static_cast<int32_t>(uiIndex));
        BoundsMax.z = BSMAX(pEdge->Location.z, fBound);
    }
    fBound = BoundsMin.x;
    BoundsMin.x = BSMIN(GoalLocation.GetLocation().x, fBound);
    fBound = BoundsMin.y;
    BoundsMin.y = BSMIN(GoalLocation.GetLocation().y, fBound);
    fBound = BoundsMin.z;
    BoundsMin.z = BSMIN(GoalLocation.GetLocation().z, fBound);
    fBound = BoundsMax.x;
    BoundsMax.x = BSMAX(GoalLocation.GetLocation().x, fBound);
    fBound = BoundsMax.y;
    BoundsMax.y = BSMAX(GoalLocation.GetLocation().y, fBound);
    fBound = BoundsMax.z;
    BoundsMax.z = BSMAX(GoalLocation.GetLocation().z, fBound);
}

void PathingSearchRayCast::operator delete(void* apMemory) {
    ourFree(apMemory);
}
