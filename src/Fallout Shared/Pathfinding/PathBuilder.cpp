#include "Fallout Shared/Pathfinding/PathBuilder.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "BSMain/ThreadSafeStructures.h"
#include "BSMain/XINISetting.h"
#include "BSSystem/BSTaskletManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathSmoother.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"
#include "Fallout Shared/Pathfinding/TeleportDoorSearch.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

#include <cmath>
#include <limits>
#include <new>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseStraightLineCheckFirst("bUseStraightLineCheckFirst:Pathfinding", true);
SettingT<INISettingCollection> bUseObstacleAvoidance("bUseObstacleAvoidance:Pathfinding", true);
SettingT<INISettingCollection> bUsePathSmoothing("bUsePathSmoothing:Pathfinding", true);
SettingT<INISettingCollection> bCreateDebugInfo("bCreateDebugInfo:Pathfinding", false);
SettingT<INISettingCollection> bRebuildPathIfSmootherFailed("bRebuildPathIfSmootherFailed:Pathfinding", true);
SettingT<INISettingCollection> bUseOldPathSmoothing("bUseOldPathSmoothing:Pathfinding", false);
SettingT<INISettingCollection> bUseRayCasts("bUseRayCasts:Pathfinding", true);
SettingT<INISettingCollection> bUseAlternateSmoothingForPrime("bUseAlternateSmoothingForPrime:Pathfinding", true);

using VirtualNodeSegmentArray = BSSimpleArray<PathBuilder::VirtualNodeSegment, 1024>;
using VirtualNodeSegmentScrapArray = BSScrapArray<PathBuilder::VirtualNodeSegment, 1024>;

bool IsEqual(uint8_t* apA, uint8_t* apB) {
    return *apA == *apB;
}

RayCastPathRequest::RayCastPathRequest() {}

RayCastPathRequest::~RayCastPathRequest() {}

void RayCastPathRequest::operator delete(void* apMemory) {
    ourFree(apMemory);
}

CheckLineOfSightFilterWater::CheckLineOfSightFilterWater() {}

bool CheckLineOfSightFilterWater::CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) {
    NavMeshTriangle* pTriangle = hMatchingEdge.pNavMesh->GetTriangle(hMatchingEdge.usTriangle);
    if (pTriangle->IsWater()) {
        return false;
    }
    if (pTriangle->IsNoLargeCreature() && bIsLargeCreature) {
        return false;
    }
    if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_AVOID_NODE)) {
        return false;
    }
    if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
        return false;
    }
    return true;
}

PathBuilder::FindTriangleForLocationWaterFilter::FindTriangleForLocationWaterFilter() {}

bool PathBuilder::FindTriangleForLocationWaterFilter::IsValidTri(
    const NiPoint3& aLocation,
    const NavMesh* apNavMesh,
    uint16_t aiTri,
    const NiPoint3& aPointOnTri
) {
    return true;
}

// The request and the solution are stored without taking a reference and the destructor never releases them, so the raw
// pointers are written directly to keep that balance.
PathBuilder::PathBuilder(PathingRequest* apRequest, PathingSolution* apSolution)
  : pScrapHeap(MemoryManager::Instance().GetThreadScrapHeap()), bPathUpdate(false) {
    pRequest.m_pObject = apRequest;
    pSolution.m_pObject = apSolution;
}

PathBuilder::~PathBuilder() {
    pSolution.m_pObject = nullptr;
    pRequest.m_pObject = nullptr;
}

Pathing::UPDATE_DETAILED_PATH_RESULT
PathBuilder::UpdateDetailedPath(uint32_t aiCurrentVirtualNodeIndex, const NiPoint3& aCurrentActorLocation) {
    int32_t iFirstLoadedNodeIndex = -1;
    int32_t iLastLoadedNodeIndex = -1;
    if (FindLoadedNodeIndices(aiCurrentVirtualNodeIndex, &iFirstLoadedNodeIndex, &iLastLoadedNodeIndex)) {
        if (pSolution->GetNodeCount()) {
            if (pSolution->iLastLoadedVirtualNodeIndex >= iLastLoadedNodeIndex) {
                return Pathing::NO_CHANGE;
            }
            pSolution->iFirstLoadedVirtualNodeIndex = aiCurrentVirtualNodeIndex;
            VirtualPathingNode* pVirtualNode = pSolution->GetVirtualNode(aiCurrentVirtualNodeIndex);
            PathingLocation* pNodeLocation = static_cast<PathingLocation*>(&pVirtualNode->GetLocation());
            PathingLocation actorLocation(aCurrentActorLocation, *pNodeLocation);
            pSolution->GetVirtualNode(aiCurrentVirtualNodeIndex)->SetLocation(actorLocation);
            uint32_t uiLastNodeIndex = pSolution->GetNodeCount() - 1;
            pSolution->RemoveLastNode();
            pSolution->iLastLoadedVirtualNodeIndex = iLastLoadedNodeIndex;
            bPathUpdate = true;
            if (!BuildDetailedPathForLoadedVirtualNodes(nullptr) || pSolution->IsIncomplete()) {
                return Pathing::FAILED;
            }
            pSolution->RemoveNode(uiLastNodeIndex);
        }
        else if (!BuildDetailedPath(nullptr, aiCurrentVirtualNodeIndex)) {
            return Pathing::FAILED;
        }
        return Pathing::CREATED_DETAILED_PATH;
    }

    if (pSolution->GetNodeCount()) {
        pSolution->ClearCurrentDetailedPath();
        return Pathing::CLEARED_DETAILED_PATH;
    }

    return Pathing::NO_CHANGE;
}

Pathing::UPDATE_DETAILED_PATH_RESULT PathBuilder::UpdatePathMoveToLow() {
    pSolution->ClearCurrentDetailedPath();
    return Pathing::CLEARED_DETAILED_PATH;
}

Pathing::UPDATE_DETAILED_PATH_RESULT PathBuilder::UpdatePathMoveToHigh(uint32_t aiCurrentVirtualNodeIndex) {
    int32_t iNewFirstLoadedNodeIndex = -1;
    int32_t iNewLastLoadedNodeIndex = -1;
    if (!FindLoadedNodeIndices(aiCurrentVirtualNodeIndex, &iNewFirstLoadedNodeIndex, &iNewLastLoadedNodeIndex)) {
        return Pathing::NO_CHANGE;
    }
    for (uint32_t i = 0; i < pSolution->GetVirtualNodeCount(); ++i) {
        pSolution->GetVirtualNode(i)->Location.ClearTriangleData();
    }
    for (int32_t iNode = iNewFirstLoadedNodeIndex; iNode <= iNewLastLoadedNodeIndex; ++iNode) {
        pSolution->GetVirtualNode(iNode)->UpdateNode();
    }
    if (BuildDetailedPath(nullptr, aiCurrentVirtualNodeIndex)) {
        return Pathing::CREATED_DETAILED_PATH;
    }

    return Pathing::FAILED;
}

// BuildDetailedPath is passed a null actor local and 0 as the first high-level node.
bool PathBuilder::BuildPath() {
    Actor* pActor = nullptr;
    pRequest->GetOrigin().ClearTriangleData();
    pRequest->GetDestination().ClearTriangleData();
    if (!pSolution->GetVirtualNodeCount()) {
        const NavMeshInfo* pStartMesh = nullptr;
        const NavMeshInfo* pGoalMesh = nullptr;
        if (pRequest->GetOrigin().ResolveNavMeshInfo(&WaterFilter)) {
            pStartMesh = pRequest->GetOrigin().GetBestNavMeshInfo();
        }
        if (pRequest->GetDestination().ResolveNavMeshInfo(&WaterFilter)) {
            pGoalMesh = pRequest->GetDestination().GetBestNavMeshInfo();
        }
        if (!BuildHighLevelPath(pStartMesh, pGoalMesh) && !pSolution->GetVirtualNodeCount()) {
            return false;
        }
    }
    else {
        pRequest->GetOrigin().ResolveNavMeshInfo(&WaterFilter);
        pRequest->GetDestination().ResolveNavMeshInfo(&WaterFilter);
    }
    return BuildDetailedPath(pActor, 0);
}

RAYCASTPATH_RESULT PathBuilder::BuildPathFromNavMeshSearchGoal(
    uint32_t aiLastVirtualNodeIndex,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
) {
    VirtualPathingNode* pLastNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
    if (aiLastVirtualNodeIndex != pSolution->GetVirtualNodeCount() - 1) {
        VirtualPathingNode* pNextNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex + 1);
        // The index compare is signed.
        if (static_cast<int32_t>(aiLastVirtualNodeIndex) >= pSolution->iLastLoadedVirtualNodeIndex) {
            if (!pLastNode->IsTransitionNode()) {
                PathingLocation exitLoc;
                if (FindExitPoint(pLastNode, pNextNode, exitLoc)) {
                    pLastNode->Location = exitLoc;
                    aRetPath.Add(exitLoc);
                    return RCP_SUCCESS;
                }
            }
        }
        else {
            if (pNextNode->HasActualLocation()) {
                PathingLocation exitLoc;
                if (FindExitPoint(pLastNode, pNextNode, exitLoc)) {
                    pLastNode->Location = exitLoc;
                    aRetPath.Add(exitLoc);
                    return RCP_SUCCESS;
                }
            }
        }
    }
    if (!pLastNode->HasActualLocation()) {
        return RCP_FAILED;
    }
    PathingLocation* pLastNodeLoc = &pLastNode->Location;
    if (!pLastNodeLoc->ResolveNavMeshInfo(nullptr)) {
        return RCP_FAILED;
    }
    if (pLastNodeLoc->HasNavMeshData()) {
        aRetPath.Add(*pLastNodeLoc);
        return RCP_SUCCESS;
    }
    Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(*pLastNodeLoc, 10u, aRetEdges);
    if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
        aRetPath.Add(*pLastNodeLoc);
        return RCP_SUCCESS;
    }
    if (eClosestEdges != Pathing::FIND_CLOSEST_EDGES_OK) {
        return RCP_FAILED;
    }
    for (const NavMeshEdgeLocation& rEdge : aRetEdges) {
        NiPoint3 deltaToEdge = pLastNodeLoc->GetLocation() - rEdge.Location;
        // The squared distance and the radius are kept as doubles, and the radius is squared in double precision.
        double dSqrDistance = deltaToEdge.SqrXYLength();
        double dRadius = pRequest->fTargetRadius;
        if (pRequest->fTargetRadius * dRadius > dSqrDistance && deltaToEdge.z > -64.0 && deltaToEdge.z < 180.0) {
            uint16_t usTriangle = rEdge.usTriangle;
            NavMeshInfo* pNavMeshInfo = rEdge.pNavMeshInfo;
            PathingLocation retPathLoc(rEdge.Location, pNavMeshInfo, usTriangle);
            aRetPath.Add(retPathLoc);
            return RCP_SUCCESS;
        }
    }
    return BuildPathToValidNavMeshTriangle(*pLastNodeLoc, aRetPath, false, aRetEdges);
}

RAYCASTPATH_RESULT PathBuilder::BuildStraightPathUsingRayCasts(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath
) {
    if (!bUseRayCasts.Bool()) {
        return RCP_FAILED;
    }
    if (BSTaskletManager::QInstance().IsTaskletThread(BSGetCurrentThreadID())) {
        PathingRequest* pPathRequest = pRequest.m_pObject;
        return PathManager::QInstance()->GetRayCastPath(pPathRequest, aStartLocation, aGoalLocation, aRetPath);
    }

    RayCastPathRequest request;
    request.StartLocation = aStartLocation;
    request.GoalLocation = aGoalLocation;
    request.eType = RCRT_NOMESH;
    request.pInitialPathRequest = pRequest.m_pObject;
    RayCastPath path;
    PathingSearchRayCast search;
    if (search.BuildPath(request, path) == RCP_SUCCESS) {
        path.Nodes.CopyTo(&aRetPath);
        return RCP_SUCCESS;
    }

    return RCP_FAILED;
}

bool PathBuilder::BuildNavMeshPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
    using LocationArray = BSSimpleArray<PathingLocation, 1024>;
    using EdgeArray = BSSimpleArray<NavMeshEdgeLocation, 1024>;
    EdgeArray startEdges;
    LocationArray startNodes;
    RAYCASTPATH_RESULT efoundPathToStart = BuildPathToNavMeshSearchStart(
        aiFirstVirtualNodeIndex,
        startNodes,
        startEdges
    );
    EdgeArray endEdges;
    LocationArray endNodes;
    RAYCASTPATH_RESULT efoundPathToGoal = BuildPathFromNavMeshSearchGoal(aiLastVirtualNodeIndex, endNodes, endEdges);
    if (efoundPathToStart == RCP_NONE || efoundPathToGoal == RCP_NONE) {
        return false;
    }
    float fdistFromStartToClosestEdge = 0.0f;
    if (!startEdges.IsEmpty()) {
        NiPoint3 closestStartEdgeCenter = startEdges.GetAt(0).Location;
        VirtualPathingNode* pFirstNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
        fdistFromStartToClosestEdge = (closestStartEdgeCenter - pFirstNode->Location.GetLocation()).XYLength();
    }
    float fdistFromGoalToClosestEdge = 0.0f;
    if (!endEdges.IsEmpty()) {
        NiPoint3 closestGoalEdgeCenter = endEdges.GetAt(0).Location;
        VirtualPathingNode* pLastNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
        fdistFromGoalToClosestEdge = (closestGoalEdgeCenter - pLastNode->Location.GetLocation()).XYLength();
    }
    // Fetches the last node's location before the first node.
    const NiPoint3& lastLocation = pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->Location.GetLocation();
    VirtualPathingNode* pFirstNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
    float fdistFromStartToGoal = (pFirstNode->Location.GetLocation() - lastLocation).XYLength();
    bool btryRayCastPath = fdistFromStartToGoal < (fdistFromStartToClosestEdge + fdistFromGoalToClosestEdge) * 2.0;
    if ((efoundPathToStart == RCP_FAILED || efoundPathToGoal == RCP_FAILED) && fdistFromStartToGoal < 512.0) {
        btryRayCastPath = true;
    }
    if (btryRayCastPath) {
        LocationArray rayCastPath;
        PathingLocation* pGoalLoc = &pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
        VirtualPathingNode* pStartNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
        RAYCASTPATH_RESULT res = BuildStraightPathUsingRayCasts(pStartNode->Location, *pGoalLoc, rayCastPath);
        if (res == RCP_NONE) {
            return false;
        }
        if (res == RCP_SUCCESS) {
            for (const PathingLocation& rLocation : rayCastPath) {
                pSolution->AddPathingNode(rLocation, NiPoint3::ZERO);
            }
            VirtualPathingNode* plastVirtualNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
            if (plastVirtualNode->IsTransitionNode()) {
                TESObjectREFR* pDoorRef = plastVirtualNode->pDoorRef;
                PathingNode* pLastPathNode = pSolution->GetLastNode();
                pLastPathNode->SetTeleportDoor(pDoorRef);
            }
            else if (
                plastVirtualNode->Location.pWorldSpace && pSolution->GetVirtualNodeCount() > aiLastVirtualNodeIndex + 1
            ) {
                TESWorldSpace* pNextWorldSpace = pSolution->GetVirtualNode(aiLastVirtualNodeIndex + 1)
                                                     ->Location.pWorldSpace;
                if (pNextWorldSpace == plastVirtualNode->Location.pWorldSpace) {
                    PathingLocation* pNextLoc = &pSolution->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location;
                    pSolution->AddPathingNode(*pNextLoc, NiPoint3::ZERO);
                    PathingNode* pLastPathNode = pSolution->GetLastNode();
                    pLastPathNode->SetTransitionFlag(true);
                }
            }
            return true;
        }
    }
    if (efoundPathToStart == RCP_FAILED) {
        return false;
    }
    if (efoundPathToGoal == RCP_FAILED) {
        if (endEdges.IsEmpty()) {
            return false;
        }
        if (!CheckClosestEdgeToGoal(aiLastVirtualNodeIndex, endEdges, endNodes)) {
            pSolution->SetIncomplete(true);
        }
        PathingLocation endLoc(
            endEdges.GetFirst().Location,
            endEdges.GetFirst().pNavMeshInfo,
            endEdges.GetFirst().usTriangle
        );
        endNodes.Add(endLoc);
    }
    for (uint32_t i = 0; i < startNodes.iSize - 1; ++i) {
        pSolution->AddPathingNode(startNodes.GetAt(static_cast<int32_t>(i)), NiPoint3::ZERO);
    }
    PathingLocation* pGoalTail = static_cast<PathingLocation*>(endNodes.GetTail());
    PathingLocation* pStartTail = static_cast<PathingLocation*>(startNodes.GetTail());
    PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT
        eAddResult = AddNodesFromNavMesh(*pStartTail, *pGoalTail, aiFirstVirtualNodeIndex, aiLastVirtualNodeIndex);
    if (eAddResult) {
        if (eAddResult == PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE) {
            pSolution->SetIncomplete(true);
            return false;
        }
        if (eAddResult != PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED) {
            return false;
        }
        if (!(fdistFromStartToGoal < 512.0)) {
            return false;
        }
        double dActorRadius = pRequest->fActorRadius;
        if (!(fPathingLargeActorRadius.Float() > dActorRadius)) {
            return false;
        }
        PathingLocation* pGoalLoc = &pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
        VirtualPathingNode* pStartNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
        if (Pathing::CheckLineOfSight(pStartNode->Location, *pGoalLoc, nullptr, nullptr)) {
            return false;
        }
        LocationArray rayCastPath;
        pGoalLoc = &pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
        pStartNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
        RAYCASTPATH_RESULT res = BuildStraightPathUsingRayCasts(pStartNode->Location, *pGoalLoc, rayCastPath);
        if (res == RCP_SUCCESS) {
            pSolution->SetIncomplete(false);
            for (const PathingLocation& rLocation : rayCastPath) {
                pSolution->AddPathingNode(rLocation, NiPoint3::ZERO);
            }
            bool bComplete = !pSolution->IsIncomplete();
            return bComplete;
        }
        return false;
    }
    if (!endNodes.IsEmpty()) {
        for (int32_t i = endNodes.iSize - 1; i >= 0; --i) {
            pSolution->AddPathingNode(endNodes.GetAt(i), NiPoint3::ZERO);
        }
    }
    VirtualPathingNode* plastVirtualNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
    if (plastVirtualNode->IsTransitionNode()) {
        TESObjectREFR* pDoorRef = plastVirtualNode->pDoorRef;
        PathingNode* pLastPathNode = pSolution->GetLastNode();
        pLastPathNode->SetTeleportDoor(pDoorRef);
    }
    else if (plastVirtualNode->Location.pWorldSpace && pSolution->GetVirtualNodeCount() > aiLastVirtualNodeIndex + 1) {
        TESWorldSpace* pNextWorldSpace = pSolution->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location.pWorldSpace;
        if (pNextWorldSpace == plastVirtualNode->Location.pWorldSpace) {
            PathingLocation* pNextLoc = &pSolution->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location;
            pSolution->AddPathingNode(*pNextLoc, NiPoint3::ZERO);
            PathingNode* pLastPathNode = pSolution->GetLastNode();
            pLastPathNode->SetTransitionFlag(true);
        }
    }
    bool bComplete = !pSolution->IsIncomplete();
    return bComplete;
}

PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT PathBuilder::AddNodesFromNavMesh(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    uint32_t aiFirstVirtualNodeIndex,
    uint32_t aiLastVirtualNodeIndex
) {
    NiPoint3 lastVirtualLocation;
    PathingNode* plastNode;
    NiPoint3 delta;
    PathingLocation lastLoc;
    uint32_t icurrentNodeCount;
    using nodes_t = BSSimpleArray<NavMeshSearchNode, 1024>;
    using BadNodes_t = BSSimpleArray<NavMeshSearchLink, 1024>;
    int32_t pass;
    if (bUseStraightLineCheckFirst.Bool() &&
        !pRequest->GetAvoidNodeArray() &&
        !pRequest->bFaceTargetAtGoal &&
        !pRequest->GetTryToUseCurvedPath() &&
        TryBuildStraightPath(aStartLocation, aGoalLocation)) {
        return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_OK;
    }
    BadNodes_t BadNodes(32u, 0);
    bool bAddNodesFromNavMeshIncomplete = false;
    int32_t imaxPassCount = pRequest->iRetryCount;
    BSClamp<int32_t>(imaxPassCount, 1, 10);
    DetailedPathInfos.Clear(false);
    DetailedPathMeshes.Clear(false);
    for (uint32_t i = 0; i < pSolution->GetVirtualNodeCount(); ++i) {
        NavMeshPtr spmesh;
        if (pSolution->GetVirtualNode(i)->Location.GetNavMesh(spmesh)) {
            DetailedPathMeshes.Add(spmesh);
            NavMeshInfo* pInfo = spmesh->GetNavMeshInfo();
            DetailedPathInfos.Add(pInfo);
        }
    }
    for (pass = 0; pass < imaxPassCount; ++pass) {
        VirtualPathingNode* plastVirtualNode;
        float ftargetRadius;
        if (pass == 1) {
            using loadedNavmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
            loadedNavmeshes_t loadedNavmeshes;
            Pathing::GetAllLoadedNavmeshes(&loadedNavmeshes);
            for (uint32_t i = 0; i < loadedNavmeshes.iSize; ++i) {
                if (DetailedPathMeshes.Find(
                        loadedNavmeshes.GetAt(static_cast<int32_t>(i)),
                        0,
                        &BSSimpleArray<NavMeshPtr, 1024>::DefaultEqualFunctionByAddress
                    ) == -1) {
                    DetailedPathMeshes.Add(loadedNavmeshes.GetAt(static_cast<int32_t>(i)));
                    NavMeshInfo* pInfo = loadedNavmeshes.GetAt(static_cast<int32_t>(i))->GetNavMeshInfo();
                    DetailedPathInfos.Add(pInfo);
                }
            }
        }
        nodes_t nodes(32u, 0);
        // The nodes are read through the const GetAt.
        const nodes_t& rconstNodes = nodes;
        BadNodes_t links(32u, 0);
        PathingLocation smoothingStartLocation(aStartLocation);
        PathingLocation smoothingGoalLocation(aGoalLocation);
        // A failed search may still have its partial path, up to its last node, smoothed; its other outcomes return or
        // end the pass.
        if (!RunNavMeshSearch(
                aStartLocation,
                aGoalLocation,
                &BadNodes,
                nodes,
                links,
                aiFirstVirtualNodeIndex,
                aiLastVirtualNodeIndex
            )) {
            if (nodes.IsEmpty() && pass >= 1) {
                return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
            }
            if (nodes.IsEmpty()) {
                continue;
            }
            NavMeshSearchNode* lastNode = &nodes.GetLast();
            uint16_t usTriangle = lastNode->sTriangle;
            NavMeshInfo* pNavMeshInfo = lastNode->pNavMesh->GetNavMeshInfo();
            lastLoc = PathingLocation(lastNode->GetCenter(), pNavMeshInfo, usTriangle);
            if (pSolution->iLastLoadedVirtualNodeIndex == aiLastVirtualNodeIndex) {
                plastVirtualNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
                lastVirtualLocation = plastVirtualNode->Location.GetLocation();
                delta = lastLoc.GetLocation() - lastVirtualLocation;
                if (plastVirtualNode->IsTransitionNode()) {
                    ftargetRadius = 80.0f;
                }
                else {
                    ftargetRadius = pRequest->fTargetRadius;
                }
                if (ftargetRadius * ftargetRadius > delta.SqrXYLength() && std::fabs(delta.z) < 180.0) {
                    if (!pRequest->bAllowIncompletePath) {
                        return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
                    }
                    if (!plastVirtualNode->IsTransitionNode()) {
                        bAddNodesFromNavMeshIncomplete = true;
                        pSolution->SetIncomplete(true);
                    }
                }
                else {
                    if (!pass) {
                        continue;
                    }
                    bAddNodesFromNavMeshIncomplete = true;
                    pSolution->SetIncomplete(true);
                }
            }
            else {
                bAddNodesFromNavMeshIncomplete = true;
                pSolution->SetIncomplete(true);
            }
            if (!pRequest->bAllowIncompletePath) {
                return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
            }
            smoothingGoalLocation = lastLoc;
        }
        icurrentNodeCount = pSolution->GetNodeCount();
        if (RunPathSmoother(smoothingStartLocation, smoothingGoalLocation, BadNodes, nodes, links)) {
            break;
        }
        if (pass == imaxPassCount - 1 && pRequest->GetIgnoreSmoothingIfFails()) {
            bAddNodesFromNavMeshIncomplete = true;
            pSolution->SetIncomplete(true);
            for (uint32_t i = 0; i < nodes.iSize; ++i) {
                NavMeshInfo* pInfo = rconstNodes.GetAt(static_cast<int32_t>(i)).pNavMesh->GetNavMeshInfo();
                NiPoint3 center = rconstNodes.GetAt(static_cast<int32_t>(i)).GetCenter();
                PathingLocation loc(&center, pInfo);
                pSolution->AddPathingNode(loc, NiPoint3::ZERO);
            }
            break;
        }
        plastNode = pSolution->GetLastNode();
        if (plastNode && pSolution->iLastLoadedVirtualNodeIndex == aiLastVirtualNodeIndex) {
            plastVirtualNode = pSolution->GetVirtualNode(aiLastVirtualNodeIndex);
            lastVirtualLocation = plastVirtualNode->Location.GetLocation();
            NiPoint3 lastDelta = plastNode->Location.GetLocation() - lastVirtualLocation;
            if (plastVirtualNode->IsTransitionNode()) {
                ftargetRadius = 80.0f;
            }
            else {
                ftargetRadius = pRequest->fTargetRadius;
            }
            if (ftargetRadius * ftargetRadius > lastDelta.SqrXYLength() && std::fabs(lastDelta.z) < 180.0) {
                break;
            }
        }

        if (!BadNodes.IsEmpty() && pass != imaxPassCount - 1) {
            if (pass < imaxPassCount - 1) {
                while (pSolution->GetNodeCount() != icurrentNodeCount) {
                    pSolution->RemoveLastNode();
                }
            }
            continue;
        }
        if (plastNode) {
            bAddNodesFromNavMeshIncomplete = true;
            pSolution->SetIncomplete(true);
            if (pRequest->bAllowIncompletePath) {
                return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE;
            }
            while (pSolution->GetNodeCount() != icurrentNodeCount) {
                pSolution->RemoveLastNode();
            }
        }
        return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
    }
    if (bAddNodesFromNavMeshIncomplete) {
        return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE;
    }

    return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_OK;
}

// The setting's value is unused: Setting::Bool is called for a debug report that is compiled out.
bool PathBuilder::BuildHighLevelPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh) {
    bool bFound = false;
    if (!apStartMesh || !apGoalMesh) {
        bCreateDebugInfo.Bool();
    }
    else if (apStartMesh == apGoalMesh) {
        pSolution->AddVirtualPathingNode(pRequest->GetOrigin());
        pSolution->AddVirtualPathingNode(pRequest->GetDestination());
        bFound = true;
    }
    else if (BuildNavMeshInfoPath(apStartMesh, apGoalMesh)) {
        bFound = true;
    }
    if (bFound) {
        return true;
    }
    if (pRequest->GetOrigin().GetWorldspaceSimple()) {
        if (pRequest->GetOrigin().GetWorldspaceSimple() == pRequest->GetDestination().GetWorldspaceSimple() &&
            pRequest->GetOrigin().GetBestNavMeshInfo() &&
            pRequest->GetDestination().GetBestNavMeshInfo()) {
            return false;
        }
    }
    else if (
        pRequest->GetOrigin().GetCell() == pRequest->GetDestination().GetCell() &&
        pRequest->GetOrigin().GetBestNavMeshInfo() &&
        pRequest->GetDestination().GetBestNavMeshInfo()
    ) {
        return false;
    }
    pSolution->VirtualPathingNodes.Clear(false);
    return BuildTeleportDoorPath();
}

bool PathBuilder::BuildNavMeshInfoPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh) {
    NiPoint3 nextMarkerLoc;
    NiPoint3 markerLoc;
    using infos_t = BSScrapArray<const NavMeshInfo*, 1024>;
    using portals_t = BSScrapArray<TESObjectREFR*, 1024>;
    NavMeshInfoSearch highSearch;
    infos_t infos(20u, 0, pScrapHeap);
    portals_t portals(20u, 0, pScrapHeap);
    ActorLockData* plockData = nullptr;
    if (!pRequest->bCantOpenDoors) {
        plockData = &pRequest->LockData;
    }
    Pathing::DOOR_LOCK_DISPOSITION doorLockDisp = static_cast<Pathing::DOOR_LOCK_DISPOSITION>(
        pRequest->GetIgnoreLocks()
    );
    NiPoint3 destination = pRequest->Destination.GetLocation();
    bool binfoSearchSucceeded = highSearch.Pathfind(
        plockData,
        pRequest->Origin.GetLocation(),
        apStartMesh,
        destination,
        apGoalMesh,
        &infos,
        &portals,
        doorLockDisp
    );
    if (infos.iSize > 1) {
        pSolution->AddVirtualPathingNode(*&pRequest->Origin);
        if (portals.GetAt(0)) {
            TESObjectREFR* pTelDoor = portals.GetAt(0);
            markerLoc = pTelDoor->GetLinkedDoorTeleportPosition();
            pSolution->AddNode_Door(infos.GetAt(0), &markerLoc, pTelDoor);
            NiPoint3 teleportLoc = *&pTelDoor->GetTeleport()->position;
            pSolution->AddVirtualPathingNode_0(teleportLoc, infos.GetAt(1));
        }
        for (uint32_t i = 1; i < infos.iSize - 1; ++i) {
            if (portals.GetAt(static_cast<int32_t>(i))) {
                TESObjectREFR* pTelDoor = portals.GetAt(static_cast<int32_t>(i));
                if (pTelDoor) {
                    nextMarkerLoc = pTelDoor->GetLinkedDoorTeleportPosition();
                    pSolution->AddNode_Door(infos.GetAt(static_cast<int32_t>(i)), &nextMarkerLoc, pTelDoor);
                    NiPoint3 teleportLoc = *&pTelDoor->GetTeleport()->position;
                    pSolution->AddVirtualPathingNode_0(teleportLoc, infos.GetAt(static_cast<int32_t>(i + 1)));
                }
            }
            else {
                pSolution->AddVirtualPathingNode_3(infos.GetAt(static_cast<int32_t>(i)));
            }
        }
        pSolution->AddVirtualPathingNode(*&pRequest->Destination);
    }
    else {
        pSolution->AddVirtualPathingNode(*&pRequest->Origin);
        pSolution->AddVirtualPathingNode(*&pRequest->Destination);
    }
    return binfoSearchSucceeded;
}

bool PathBuilder::BuildTeleportDoorPath() {
    TeleportDoorSearch doorSearch;
    TeleportPath doorPath;
    doorPath.Spaces.SetReservedSize(10u);
    doorPath.Doors.SetReservedSize(10u);
    ActorLockData* plockData = nullptr;
    if (!pRequest->bCantOpenDoors) {
        plockData = &pRequest->LockData;
    }
    bool bteleportSearchSucceeded = doorSearch.BuildNodePath(
        *&pRequest->Origin,
        *&pRequest->Destination,
        &doorPath,
        Pathing::TELEPORT_DOOR_PATH_NONE,
        0.0f,
        plockData,
        Pathing::DOOR_LOCK_DISPOSITION_NORMAL
    );
    if (doorPath.Spaces.iSize > 1) {
        NiPoint3 startLocation = pRequest->Origin.GetLocation();
        NiPoint3 goalLocation = doorPath.Doors.GetFirst().pDoor->GetLinkedDoorTeleportPosition();
        TESObjectREFR* pDoor = doorPath.Doors.GetFirst().pDoor;
        if (!BuildNavMeshInfoPathInSpace(startLocation, goalLocation, &doorPath.Spaces.GetFirst(), pDoor)) {
            return false;
        }
        for (uint32_t i = 0; i < doorPath.Doors.iSize - 1; ++i) {
            NiPoint3 doorLocation = doorPath.Doors.GetAt(static_cast<int32_t>(i)).DoorTeleportLocation;
            NiPoint3 nextDoorLocation = doorPath.Doors.GetAt(static_cast<int32_t>(i + 1))
                                            .pDoor->GetLinkedDoorTeleportPosition();
            TESObjectREFR* pNextDoor = doorPath.Doors.GetAt(static_cast<int32_t>(i + 1)).pDoor;
            if (!BuildNavMeshInfoPathInSpace(
                    doorLocation,
                    nextDoorLocation,
                    &doorPath.Spaces.GetAt(static_cast<int32_t>(i + 1)),
                    pNextDoor
                )) {
                pSolution->SetIncomplete(true);
                return false;
            }
        }
        startLocation = doorPath.Doors.GetLast().DoorTeleportLocation;
        goalLocation = pRequest->Destination.GetLocation();
        if (!BuildNavMeshInfoPathInSpace(startLocation, goalLocation, &doorPath.Spaces.GetLast(), nullptr)) {
            pSolution->SetIncomplete(true);
            return false;
        }
    }
    else if (bteleportSearchSucceeded) {
        ParentSpaceNode* pSpace = &doorPath.Spaces.GetAt(0);
        NiPoint3 destination = pRequest->Destination.GetLocation();
        if (!BuildNavMeshInfoPathInSpace(pRequest->Origin.GetLocation(), destination, pSpace, nullptr)) {
            return false;
        }
    }
    return bteleportSearchSucceeded;
}

bool PathBuilder::BuildDetailedPath(Actor* pActor, uint32_t aiFirstHighLevelNode) {
    pSolution->ClearCurrentDetailedPath();
    int32_t iFirstLoadedNode = -1;
    int32_t iLastLoadedNode = -1;
    if (!FindLoadedNodeIndices(aiFirstHighLevelNode, &iFirstLoadedNode, &iLastLoadedNode)) {
        return true;
    }
    pSolution->iFirstLoadedVirtualNodeIndex = iFirstLoadedNode;
    pSolution->iLastLoadedVirtualNodeIndex = iLastLoadedNode;
    bool bbuildDetailedPathRes = BuildDetailedPathForLoadedVirtualNodes(pActor);
    if (!bbuildDetailedPathRes && !pSolution->GetNodeCount()) {
        return false;
    }
    if (pRequest->GetFirstTangentUsesHeading() &&
        !pSolution->iFirstLoadedVirtualNodeIndex &&
        pSolution->GetNodeCount() >= 2) {
        PathingNode* pnode0 = pSolution->GetNode(0);
        PathingNode* pnode1 = pSolution->GetNode(1u);
        if (pnode0 && pnode1) {
            PathingLocation startLoc(*&pnode0->Location);
            PathingLocation nextLoc(*&pnode1->Location);
            NiPoint3 startPos = startLoc.GetLocation();
            if ((nextLoc.GetLocation() - startPos).SqrLength() > 4096.0) {
                float fActorRadius = pRequest->fActorRadius;
                float fHeading = pRequest->fInitialPathHeading;
                NiPoint3 tangent = Pathing::ComputeInitialTangentForActor(fHeading, startLoc, nextLoc, fActorRadius);
                pnode0->SetTangent(tangent);
            }
        }
    }
    if (bbuildDetailedPathRes && pRequest->bFaceTargetAtGoal) {
        int32_t iLastLoadedIndex = pSolution->iLastLoadedVirtualNodeIndex;
        if (iLastLoadedIndex == pSolution->GetVirtualNodeCount() - 1) {
            PathingNode* pnodeN = pSolution->GetNode(pSolution->GetNodeCount() - 1);
            PathingNode* pnodeN1 = pSolution->GetNode(pSolution->GetNodeCount() - 2);
            if (pnodeN && pnodeN1) {
                PathingLocation endLoc(*&pnodeN->Location);
                PathingLocation prevLoc(*&pnodeN1->Location);
                NiPoint3 prevPos = prevLoc.GetLocation();
                if ((endLoc.GetLocation() - prevPos).SqrLength() > 4096.0) {
                    PathingLocation goalLoc(*&pnodeN->Location);
                    float fActorRadius = pRequest->fActorRadius;
                    float fGoalAngle = pRequest->fGoalAngle;
                    NiPoint3 tangent = Pathing::ComputeFinalTangentForActor(fGoalAngle, goalLoc, fActorRadius);
                    pnodeN->SetTangent(tangent);
                }
            }
        }
    }

    return bbuildDetailedPathRes;
}

// The water heights are doubles; a start or goal point below its water height plus 25 skips the side checks. The side
// lines run from the start and goal moved by the actor's radius along the horizontal normal of the path, first to one
// side, then the other.
bool PathBuilder::TryBuildStraightPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation) {
    alignas(CheckLineOfSightFilterWater) uint8_t aFilterStorage[sizeof(CheckLineOfSightFilterWater)];
    CheckLineOfSightFilterWater& kFilter = *::new (aFilterStorage) CheckLineOfSightFilterWater;
    // Reads the radius before the setting.
    float fRadius = pRequest->GetActorRadius();
    kFilter.bIsLargeCreature = fPathingLargeActorRadius.Float() < fRadius;
    if (!Pathing::CheckLineOfSight(aStartLocation, aGoalLocation, nullptr, &kFilter)) {
        return false;
    }

    TESObjectCELL* pCell = aStartLocation.GetCell();
    NiPoint3 kLocation = aStartLocation.GetLocation();
    double dWaterHeight = TES::pInstance->GetWaterHeight(kLocation, pCell) + 25.0;
    if (aStartLocation.GetLocation().z < dWaterHeight) {
        pSolution->AddPathingNode(aStartLocation, NiPoint3::ZERO);
        return true;
    }
    pCell = aGoalLocation.GetCell();
    kLocation = aGoalLocation.GetLocation();
    dWaterHeight = TES::pInstance->GetWaterHeight(kLocation, pCell) + 25.0;
    if (aGoalLocation.GetLocation().z < dWaterHeight) {
        pSolution->AddPathingNode(aStartLocation, NiPoint3::ZERO);
        return true;
    }

    NiPoint3 kStart = aStartLocation.GetLocation();
    NiPoint3 kDirection = aGoalLocation.GetLocation() - kStart;
    float fOffsetRadius = pRequest->GetActorRadius();
    NiPoint3 kOffset = kDirection.UnitCross(NiPoint3::UNIT_Z) * fOffsetRadius;

    alignas(PathingLocation) uint8_t aStartLeftStorage[sizeof(PathingLocation)];
    PathingLocation& kStartLeft = *::new (aStartLeftStorage)
                                      PathingLocation(aStartLocation.GetLocation() - kOffset, aStartLocation);
    alignas(PathingLocation) uint8_t aGoalLeftStorage[sizeof(PathingLocation)];
    PathingLocation& kGoalLeft = *::new (aGoalLeftStorage)
                                     PathingLocation(aGoalLocation.GetLocation() - kOffset, aGoalLocation);
    if (!Pathing::CheckLineOfSight(kStartLeft, kGoalLeft, nullptr, nullptr)) {
        kGoalLeft.~PathingLocation();
        kStartLeft.~PathingLocation();
        return false;
    }

    alignas(PathingLocation) uint8_t aStartRightStorage[sizeof(PathingLocation)];
    PathingLocation& kStartRight = *::new (aStartRightStorage)
                                       PathingLocation(aStartLocation.GetLocation() + kOffset, aStartLocation);
    alignas(PathingLocation) uint8_t aGoalRightStorage[sizeof(PathingLocation)];
    PathingLocation& kGoalRight = *::new (aGoalRightStorage)
                                      PathingLocation(aGoalLocation.GetLocation() + kOffset, aGoalLocation);
    bool bResult = false;
    if (Pathing::CheckLineOfSight(kStartRight, kGoalRight, nullptr, nullptr)) {
        pSolution->AddPathingNode(aStartLocation, NiPoint3::ZERO);
        bResult = true;
    }
    kGoalRight.~PathingLocation();
    kStartRight.~PathingLocation();
    kGoalLeft.~PathingLocation();
    kStartLeft.~PathingLocation();
    return bResult;
}

// The location is passed by value as a bitwise copy: the destructor of the by-value location only stores the vtable, so
// the copy leaves nothing to release twice.
bool PathBuilder::FindClosestOpenEdge(PathingLocation aStartLocation, PathingLocation& aRetLocation, bool abStart) {
    NiPoint3 pt;
    int32_t iEdgeIndex;
    NiPoint3 bestLocation;
    using infos_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    float fBestDistSqr = std::numeric_limits<float>::max();
    uint32_t uiBestIndex = -1;
    uint16_t usBestTriangle = -1;
    bestLocation = NiPoint3::ZERO;
    infos_t infos;
    aStartLocation.GetNavMeshInfos(infos);
    uint32_t uiInfoCount = infos.iSize;
    for (uint32_t uiIndex = 0; uiIndex < uiInfoCount; ++uiIndex) {
        NavMeshPtr spMesh;
        if (infos.GetAt(static_cast<int32_t>(uiIndex))->GetNavMesh(spMesh)) {
            uint16_t usTriIndex[2];
            float fTargetRadius = pRequest->fTargetRadius;
            if (spMesh->NavMesh::FindClosestOpenEdgeForLocation(
                    aStartLocation.GetLocation(),
                    fTargetRadius,
                    usTriIndex,
                    &iEdgeIndex,
                    abStart
                )) {
                NavMeshEdgeVertices edgeVertices = spMesh->NavMesh::GetEdgeVertices(usTriIndex[0], iEdgeIndex);
                NiPoint3 center = spMesh->NavMesh::GetCenter(usTriIndex[0]);
                pt = center;
                NiPoint3 triCenter = spMesh->NavMesh::GetCenter(usTriIndex[0]);
                if (NavMeshMath::Find2DIntersectionPoint(
                        aStartLocation.GetLocation(),
                        triCenter,
                        edgeVertices.Vertices[0]->Location,
                        edgeVertices.Vertices[1]->Location,
                        &pt
                    )) {
                    pt += (center - pt) * 0.1f;
                }
                float fDistSqr = (pt - aStartLocation.GetLocation()).SqrLength();
                if (fBestDistSqr > static_cast<double>(fDistSqr)) {
                    fBestDistSqr = fDistSqr;
                    uiBestIndex = uiIndex;
                    usBestTriangle = usTriIndex[0];
                    bestLocation = pt;
                }
            }
        }
    }
    if (uiBestIndex == -1) {
        return false;
    }

    aRetLocation = PathingLocation(bestLocation, infos.GetAt(static_cast<int32_t>(uiBestIndex)), usBestTriangle);
    return true;
}

RAYCASTPATH_RESULT PathBuilder::BuildPathToNavMeshSearchStart(
    uint32_t aiFirstVirtualNodeIndex,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
) {
    VirtualPathingNode* pNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex);
    if (aiFirstVirtualNodeIndex != 0 && !bPathUpdate) {
        VirtualPathingNode* pPrevNode = pSolution->GetVirtualNode(aiFirstVirtualNodeIndex - 1);
        uint32_t uiVirtualNodeCount = pSolution->GetCount();
        // The index and the count are compared signed.
        if (static_cast<int32_t>(aiFirstVirtualNodeIndex) <= static_cast<int32_t>(uiVirtualNodeCount)) {
            // Tests the current node in this branch and the previous one in the other.
            if (!pNode->HasActualLocation()) {
                alignas(PathingLocation) uint8_t aEntryMemory[sizeof(PathingLocation)];
                PathingLocation* pEntry = ::new (aEntryMemory) PathingLocation;
                if (FindEntryPoint(pPrevNode, pNode, *pEntry)) {
                    aRetPath.Add(*pEntry);
                    pEntry->~PathingLocation();
                    return RCP_SUCCESS;
                }
                pEntry->~PathingLocation();
            }
        }
        else if (pPrevNode->HasActualLocation()) {
            alignas(PathingLocation) uint8_t aEntryMemory[sizeof(PathingLocation)];
            PathingLocation* pEntry = ::new (aEntryMemory) PathingLocation;
            if (FindEntryPoint(pPrevNode, pNode, *pEntry)) {
                aRetPath.Add(*pEntry);
                pEntry->~PathingLocation();
                return RCP_SUCCESS;
            }
            pEntry->~PathingLocation();
        }
    }
    if (!pNode->HasActualLocation()) {
        return RCP_FAILED;
    }
    PathingLocation& kLocation = pNode->GetLocation();
    if (!kLocation.ResolveNavMeshInfo(nullptr)) {
        return RCP_FAILED;
    }
    if (kLocation.HasNavMeshData()) {
        aRetPath.Add(kLocation);
        return RCP_SUCCESS;
    }
    Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(kLocation, 0xAu, aRetEdges);
    if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
        aRetPath.Add(kLocation);
        return RCP_SUCCESS;
    }
    if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_OK) {
        return BuildPathToValidNavMeshTriangle(kLocation, aRetPath, true, aRetEdges);
    }
    return RCP_FAILED;
}

// The portal triangles are 32-bit values that are read as 16-bit where a triangle index is passed.
bool PathBuilder::FindEntryPoint(
    VirtualPathingNode* apPrevNode,
    VirtualPathingNode* apCurrentNode,
    PathingLocation& aRetLocation
) {
    // bestLoc and entryLoc are built and destroyed with the Construct and Destruct calls only.
    using portalTriangles_t = BSScrapArray<int32_t, 1024>;
    NiPoint3 point1;
    NiPoint3 point2;
    int32_t iedge;
    apCurrentNode->Location.ResolveNavMeshInfoOnly();
    NavMeshPtr pcurrentMesh;
    if (!apCurrentNode->Location.GetBestNavMeshInfo()->GetNavMesh(pcurrentMesh)) {
        return false;
    }
    if (apCurrentNode->HasActualLocation()) {
        uint16_t usTriIndex[2];
        const NiPoint3& kApproxLocation = apCurrentNode->Location.GetBestNavMeshInfo()->ApproxLocation;
        float fdist = (apPrevNode->Location.GetLocation() - kApproxLocation).Length();
        usTriIndex[0] = -1;
        iedge = -1;
        float fRadius = fdist + fdist;
        if (pcurrentMesh->FindClosestOpenEdgeForLocation(
                apPrevNode->Location.GetLocation(),
                fRadius,
                usTriIndex,
                &iedge,
                true
            )) {
            pcurrentMesh->GetEdgeVertices(usTriIndex[0], iedge, &point1, &point2);
            NiPoint3 center = pcurrentMesh->GetCenter(usTriIndex[0]);
            uint16_t usTriangle = usTriIndex[0];
            NavMeshInfo* pNavMeshInfo = pcurrentMesh->GetNavMeshInfo();
            PathingLocation entryLoc((point1 + point2) * 0.45f + center * 0.1f, pNavMeshInfo, usTriangle);
            aRetLocation = entryLoc;
            return true;
        }
    }
    else {
        portalTriangles_t portalTriangles(0, 0, pScrapHeap);
        pcurrentMesh->FindPortalTriangles(apPrevNode->Location.GetBestNavMeshInfo(), &portalTriangles);
        uint32_t ibestTri = 0xFFFF;
        float fbestDist = std::numeric_limits<float>::max();
        for (uint32_t i = 0; i < portalTriangles.iSize; ++i) {
            NavMeshTriangle* ptri = pcurrentMesh->GetTriangle(
                static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i)))
            );
            if (!(pRequest->fActorRadius > 150.0) || !ptri->IsNoLargeCreature()) {
                NiPoint3 triCenter = pcurrentMesh->GetCenter(
                    static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i)))
                );
                float fTriDist = (triCenter - pRequest->Origin.GetLocation()).Length();
                if (fbestDist > static_cast<double>(fTriDist)) {
                    fTriDist = fbestDist;
                    ibestTri = portalTriangles.GetAt(static_cast<int32_t>(i));
                }
            }
        }
        if (ibestTri != 0xFFFF) {
            uint16_t usBestTriangle = static_cast<uint16_t>(ibestTri);
            const NavMeshInfo* pInfo = apCurrentNode->Location.GetBestNavMeshInfo();
            PathingLocation bestLoc(pcurrentMesh->GetCenter(static_cast<uint16_t>(ibestTri)), pInfo, usBestTriangle);
            aRetLocation = bestLoc;
            return true;
        }
    }
    return false;
}

// KEEP: FindClosestOpenEdge's by-value PathingLocation argument is meant to be built in place by an out-of-line
// PathingLocation copy constructor and destroyed by the callee. src's PathingLocation has only the implicit copy
// constructor, which writes src's own vtable pointer into the argument; giving it an out-of-line one (the form the
// NiPointer arguments use) would change every implicit copy of PathingLocation and of the classes holding one. So src
// builds the copy in a local and passes that.
// The exe's cleanup-only C++ EH frame is left out.
RAYCASTPATH_RESULT PathBuilder::BuildPathToValidNavMeshTriangle(
    PathingLocation& aStartLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    bool abStart,
    BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
) {
    if (bUseRayCasts.Bool()) {
        Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(aStartLocation, 10u, aRetEdges);
        if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_NONE) {
            return RCP_FAILED;
        }
        if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
            aRetPath.Add(aStartLocation);
            return RCP_SUCCESS;
        }
        if (eClosestEdges != Pathing::FIND_CLOSEST_EDGES_OK) {
            return RCP_FAILED;
        }
        if (BSTaskletManager::QInstance().IsTaskletThread(BSGetCurrentThreadID())) {
            return PathManager::QInstance()
                ->GetRayCastPathToNavmesh(pRequest.m_pObject, aRetEdges, &aStartLocation, aRetPath, abStart);
        }
        RayCastPathRequest rayCastRequest;
        rayCastRequest.GoalLocation = aStartLocation;
        rayCastRequest.eType = static_cast<RAYCAST_REQUEST_TYPE>(!abStart);
        rayCastRequest.pInitialPathRequest = pRequest.m_pObject;
        aRetEdges.CopyTo(&rayCastRequest.Edges);
        RayCastPath retPath;
        PathingSearchRayCast rayCastSearch;
        RAYCASTPATH_RESULT eResult = rayCastSearch.BuildPath(rayCastRequest, retPath);
        while (eResult == RCP_NONE) {
            eResult = rayCastSearch.BuildPath(rayCastRequest, retPath);
        }
        RAYCASTPATH_RESULT eReturn = RCP_FAILED;
        if (eResult == RCP_SUCCESS) {
            retPath.Nodes.CopyTo(&aRetPath);
            eReturn = RCP_SUCCESS;
        }
        return eReturn;
    }
    PathingLocation goalLoc;
    RAYCASTPATH_RESULT eReturn = RCP_FAILED;
    if (FindClosestOpenEdge(aStartLocation, goalLoc, abStart)) {
        aRetPath.Add(aStartLocation);
        aRetPath.Add(goalLoc);
        eReturn = RCP_SUCCESS;
    }
    return eReturn;
}

bool PathBuilder::CheckClosestEdgeToGoal(
    uint32_t aiLastVirtualNodeIndex,
    const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEndEdges,
    BSSimpleArray<PathingLocation, 1024>& aEndNodes
) {
    alignas(NavMeshEdgeLocation) uint8_t aEdgeStorage[sizeof(NavMeshEdgeLocation)];
    NavMeshEdgeLocation* pEdge = ::new (aEdgeStorage) NavMeshEdgeLocation(aEndEdges.GetFirst());
    alignas(NiPoint3) uint8_t aVert1Storage[sizeof(NiPoint3)];
    NiPoint3* pVert1 = ::new (aVert1Storage) NiPoint3;
    alignas(NiPoint3) uint8_t aVert2Storage[sizeof(NiPoint3)];
    NiPoint3* pVert2 = ::new (aVert2Storage) NiPoint3;
    alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
    NavMeshPtr* pspNavMesh = ::new (aNavMeshStorage) NavMeshPtr;
    bool bResult = false;
    if (pEdge->pNavMeshInfo->GetNavMesh(*pspNavMesh)) {
        (*pspNavMesh)->GetEdgeVertices(pEdge->usTriangle, pEdge->iEdgeIndex, pVert1, pVert2);
        NiPoint3 kNodeLocation = pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->GetLocation().GetLocation();
        NiPoint3 kPoint = NavMeshMath::ProjectPointOntoSegment(kNodeLocation, *pVert1, *pVert2);
        // The squared distance and the first radius are kept as doubles; the product is a double.
        double dSqrDistance = (kPoint - kNodeLocation).SqrXYLength();
        double dRadius = pRequest->GetTargetRadius();
        if (pRequest->GetTargetRadius() * dRadius > dSqrDistance &&
            (kPoint - kNodeLocation).z > -64.0 &&
            (kPoint - kNodeLocation).z < 180.0) {
            NiPoint3 kCenter = (*pspNavMesh)->GetCenter(pEdge->usTriangle);
            kPoint += (kCenter - kPoint) * 0.1f;
            aEndNodes.Clear(false);
            alignas(PathingLocation) uint8_t aLocationStorage[sizeof(PathingLocation)];
            PathingLocation* pLocation = ::new (aLocationStorage)
                PathingLocation(kPoint, pEdge->pNavMeshInfo, pEdge->usTriangle);
            aEndNodes.Add(*pLocation);
            bResult = true;
            pLocation->~PathingLocation();
        }
    }
    *pspNavMesh = nullptr;
    return bResult;
}

bool PathBuilder::FindLoadedNodeIndices(
    uint32_t aiFirstHighLevelNode,
    int32_t* apFirstLoadedNodeIndex,
    int32_t* apLastLoadedNodeIndex
) {
    TESObjectCELL* pcell = pSolution->GetVirtualNode(aiFirstHighLevelNode)->Location.GetCell();
    if (!pcell || !pcell->GetAttached()) {
        return false;
    }
    *apFirstLoadedNodeIndex = aiFirstHighLevelNode;
    // The index is compared with the node count signed.
    for (*apLastLoadedNodeIndex = *apFirstLoadedNodeIndex + 1;
         *apLastLoadedNodeIndex < static_cast<int32_t>(pSolution->GetVirtualNodeCount());
         ++*apLastLoadedNodeIndex) {
        pcell = pSolution->GetVirtualNode(*apLastLoadedNodeIndex)->Location.GetCell();
        if (!pcell || !pcell->GetAttached()) {
            break;
        }
        if (pSolution->GetVirtualNode(*apLastLoadedNodeIndex)->IsTransitionNode()) {
            ++*apLastLoadedNodeIndex;
            break;
        }
    }
    --*apLastLoadedNodeIndex;
    return true;
}

bool PathBuilder::FindVirtualNodeSegments(BSSimpleArray<PathBuilder::VirtualNodeSegment, 1024>& aRetSegmentArray) {
    NavMeshInfo* pnextInfo;
    using NodesHaveNavMeshInfo_t = BSScrapArray<bool, 1024>;
    aRetSegmentArray.Clear(false);
    int32_t ifirstLoadedVirtualNode = pSolution->iFirstLoadedVirtualNodeIndex;
    int32_t ilastLoadedVirtualNode = pSolution->iLastLoadedVirtualNodeIndex;
    NodesHaveNavMeshInfo_t NodesHaveNavMeshInfo(ilastLoadedVirtualNode - ifirstLoadedVirtualNode + 1, 0, pScrapHeap);
    for (int32_t i = ifirstLoadedVirtualNode; i <= ilastLoadedVirtualNode; ++i) {
        VirtualPathingNode* pvnode = pSolution->GetVirtualNode(static_cast<uint32_t>(i));
        bool bHasNavMesh = pvnode->HasNavMesh();
        NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Add(bHasNavMesh);
    }
    int32_t nodeIndex = ifirstLoadedVirtualNode;
    while (nodeIndex <= ilastLoadedVirtualNode) {
        bool bValue = true;
        int32_t firstNodeWithInfoIndex = NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Find(
            bValue,
            nodeIndex - ifirstLoadedVirtualNode,
            &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress
        );
        if (firstNodeWithInfoIndex == -1) {
            PathBuilder::VirtualNodeSegment segment;
            segment.iFirstVirtualNode = nodeIndex;
            segment.iLastVirtualNode = ilastLoadedVirtualNode;
            segment.bHasNavMeshes = false;
            aRetSegmentArray.Add(segment);
            nodeIndex = ilastLoadedVirtualNode + 1;
        }
        else {
            bValue = false;
            int32_t iFirstNodeWithoutInfo = NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Find(
                bValue,
                firstNodeWithInfoIndex,
                &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress
            );
            int32_t lastNodeWithInfoIndex = iFirstNodeWithoutInfo - 1;
            if (iFirstNodeWithoutInfo == -1) {
                lastNodeWithInfoIndex = NodesHaveNavMeshInfo.iSize - 1;
            }
            firstNodeWithInfoIndex += ifirstLoadedVirtualNode;
            lastNodeWithInfoIndex += ifirstLoadedVirtualNode;
            if (firstNodeWithInfoIndex > nodeIndex) {
                PathBuilder::VirtualNodeSegment segment;
                segment.iFirstVirtualNode = nodeIndex;
                segment.iLastVirtualNode = firstNodeWithInfoIndex - 1;
                segment.bHasNavMeshes = false;
                aRetSegmentArray.Add(segment);
            }
            int32_t icurrentsubSegmentFirstNode = firstNodeWithInfoIndex;
            for (int32_t icurrentNode = firstNodeWithInfoIndex; icurrentNode + 1 <= lastNodeWithInfoIndex;
                 ++icurrentNode) {
                VirtualPathingNode* pthisNode = pSolution->GetVirtualNode(static_cast<uint32_t>(icurrentNode));
                VirtualPathingNode* pnextNode = pSolution->GetVirtualNode(icurrentNode + 1);
                pthisNode->Location.ResolveNavMeshInfoOnly();
                pnextNode->Location.ResolveNavMeshInfoOnly();
                const NavMeshInfo* pthisInfo = pthisNode->Location.GetBestNavMeshInfo();
                // The adjacency arrays hold non-const infos, so this local's address is passed to their Find.
                pnextInfo = const_cast<NavMeshInfo*>(pnextNode->Location.GetBestNavMeshInfo());
                if (!pthisInfo || !pnextInfo) {
                    return false;
                }
                if (pthisInfo != pnextInfo &&
                    pthisInfo->AdjacentNavMeshes
                            .Find(pnextInfo, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1 &&
                    pthisInfo->PreferredAdjacentNavMeshes
                            .Find(pnextInfo, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1) {
                    PathBuilder::VirtualNodeSegment segment;
                    segment.iFirstVirtualNode = icurrentsubSegmentFirstNode;
                    segment.iLastVirtualNode = icurrentNode;
                    segment.bHasNavMeshes = true;
                    aRetSegmentArray.Add(segment);
                    icurrentsubSegmentFirstNode = icurrentNode + 1;
                }
            }
            PathBuilder::VirtualNodeSegment segment;
            segment.iFirstVirtualNode = icurrentsubSegmentFirstNode;
            segment.iLastVirtualNode = lastNodeWithInfoIndex;
            segment.bHasNavMeshes = true;
            aRetSegmentArray.Add(segment);
            nodeIndex = lastNodeWithInfoIndex + 1;
        }
    }
    return true;
}

bool PathBuilder::BuildDetailedPathForLoadedVirtualNodes(Actor* pActor) {
    using segments_t = BSScrapArray<PathBuilder::VirtualNodeSegment, 1024>;
    segments_t segments(0, 0, pScrapHeap);
    if (!FindVirtualNodeSegments(segments)) {
        return false;
    }
    bool bGoDirectIfPathFails = pRequest->GetGoDirectIfPathingFails();
    for (uint32_t i = 0; i < segments.iSize; ++i) {
        bool bSegmentBuilt = false;
        PathBuilder::VirtualNodeSegment* pSegment = &segments.GetAt(static_cast<int32_t>(i));
        if (pSegment->bHasNavMeshes) {
            if (i && segments.GetAt(static_cast<int32_t>(i - 1)).bHasNavMeshes && !bGoDirectIfPathFails) {
                pSolution->SetIncomplete(true);
                return false;
            }
            bSegmentBuilt = BuildNavMeshPathForVirtualNodes(
                static_cast<uint32_t>(pSegment->iFirstVirtualNode),
                static_cast<uint32_t>(pSegment->iLastVirtualNode)
            );
            if (!bSegmentBuilt && !bGoDirectIfPathFails) {
                return false;
            }
        }
        if (!bSegmentBuilt &&
            !BuildStraightPathForVirtualNodes(
                static_cast<uint32_t>(pSegment->iFirstVirtualNode),
                static_cast<uint32_t>(pSegment->iLastVirtualNode)
            )) {
            return false;
        }
    }
    return true;
}

bool PathBuilder::RunNavMeshSearch(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    const BSSimpleArray<NavMeshSearchLink, 1024>* aBadNodes,
    BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
    BSSimpleArray<NavMeshSearchLink, 1024>& aLinks,
    uint32_t aiFirstVirtualNodeIndex,
    uint32_t aiLastVirtualNodeIndex
) {
    uint16_t usGoalTriangle;
    uint16_t usStartTriangle;
    NavMeshPtr spStartMesh;
    if (aStartLocation.GetNavMeshAndTriangle(spStartMesh, usStartTriangle)) {
        NavMeshSearchNode start;
        start.pNavMesh = spStartMesh.m_pObject;
        start.sTriangle = usStartTriangle;
        NavMeshPtr spGoalMesh;
        if (aGoalLocation.GetNavMeshAndTriangle(spGoalMesh, usGoalTriangle)) {
            NavMeshSearchNode goal;
            goal.pNavMesh = spGoalMesh.m_pObject;
            goal.sTriangle = usGoalTriangle;
            NavMeshSearch search;
            search.SetBadNodeArray(aBadNodes);
            if (search.BuildNodePath(pRequest.m_pObject, start, goal, &aNodes, &aLinks, &DetailedPathInfos)) {
                return true;
            }

            return false;
        }

        return false;
    }

    return false;
}

bool PathBuilder::RunPathSmoother(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    BSSimpleArray<NavMeshSearchLink, 1024>& aBadNodes,
    const BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
    const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks
) {
    using triangles_t = BSScrapArray<FastNavMeshTriHandle, 1024>;
    if (bUseAlternateSmoothingForPrime.Bool() && pRequest->fActorRadius > 150.0) {
        ScrapHeap* pScrapHeap = this->pScrapHeap;
        triangles_t kTriangles(aNodes.iSize, 0, pScrapHeap);
        for (uint32_t i = 0; i < aNodes.iSize; ++i) {
            uint16_t sTriangle = aNodes.GetAt(static_cast<int32_t>(i)).sTriangle;
            kTriangles.Add(FastNavMeshTriHandle(aNodes.GetAt(static_cast<int32_t>(i)).pNavMesh, sTriangle));
        }
        NavMeshPOVSearch povSearch;
        return povSearch.BuildPath(aStartLocation, aGoalLocation, &kTriangles, pRequest.m_pObject, pSolution.m_pObject);
    }

    if (bUseOldPathSmoothing.Bool()) {
        PathSmoother pathSmoother;
        PathingSolution* pPathSolution = pSolution.m_pObject;
        NiPoint3 goal = aGoalLocation.GetLocation();
        return pathSmoother.SmoothPath(
            pRequest.m_pObject,
            aNodes,
            aLinks,
            aStartLocation.GetLocation(),
            goal,
            pPathSolution,
            &aBadNodes,
            true
        );
    }

    PathSmootherPOVSearch pathSmoother;
    PathingSolution* pPathSolution = pSolution.m_pObject;
    NiPoint3 goal = aGoalLocation.GetLocation();
    return pathSmoother.SmoothPath(
        pRequest.m_pObject,
        aNodes,
        aLinks,
        aStartLocation.GetLocation(),
        goal,
        pPathSolution,
        &aBadNodes,
        &DetailedPathInfos
    );
}

// The portal triangles are 32-bit values that are read as 16-bit where a triangle index is passed.
bool PathBuilder::FindExitPoint(
    VirtualPathingNode* apCurrentNode,
    VirtualPathingNode* apNextNode,
    PathingLocation& aRetLocation
) {
    using portalTriangles_t = BSScrapArray<int32_t, 1024>;
    NiPoint3 point1;
    NiPoint3 point2;
    int32_t iedge;
    apCurrentNode->Location.ResolveNavMeshInfoOnly();
    NavMeshPtr pcurrentMesh;
    if (!apCurrentNode->Location.GetBestNavMeshInfo()->GetNavMesh(pcurrentMesh)) {
        return false;
    }
    if (apCurrentNode->HasActualLocation()) {
        uint16_t usTriIndex[2];
        const NiPoint3& kApproxLocation = apCurrentNode->Location.GetBestNavMeshInfo()->ApproxLocation;
        float fdist = (apNextNode->Location.GetLocation() - kApproxLocation).Length();
        usTriIndex[0] = -1;
        iedge = -1;
        float fRadius = fdist + fdist;
        if (pcurrentMesh->FindClosestOpenEdgeForLocation(
                apNextNode->Location.GetLocation(),
                fRadius,
                usTriIndex,
                &iedge,
                false
            )) {
            pcurrentMesh->GetEdgeVertices(usTriIndex[0], iedge, &point1, &point2);
            NiPoint3 center = pcurrentMesh->GetCenter(usTriIndex[0]);
            NiPoint3 loc = (point1 + point2) * 0.45f + center * 0.1f;
            TESObjectCELL* pCell = pcurrentMesh->GetParentCell();
            float fwaterHeight = TES::pInstance->GetWaterHeight(loc, pCell);
            if (loc.z < fwaterHeight + 25.0) {
                loc.z = fwaterHeight;
            }
            uint16_t usTriangle = usTriIndex[0];
            NavMeshInfo* pNavMeshInfo = pcurrentMesh->GetNavMeshInfo();
            PathingLocation exitLoc(loc, pNavMeshInfo, usTriangle);
            aRetLocation = exitLoc;
            return true;
        }
        return false;
    }
    portalTriangles_t portalTriangles(0, 0, pScrapHeap);
    pcurrentMesh->FindPortalTriangles(apNextNode->Location.GetBestNavMeshInfo(), &portalTriangles);
    int32_t ibestTri = 0xFFFF;
    float fbestDist = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < portalTriangles.iSize; ++i) {
        NavMeshTriangle* ptri = pcurrentMesh->GetTriangle(
            static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i)))
        );
        if (!(pRequest->fActorRadius > 150.0) || !ptri->IsNoLargeCreature()) {
            NiPoint3 triCenter = pcurrentMesh->GetCenter(
                static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i)))
            );
            float fTriDist = (triCenter - pRequest->Destination.GetLocation()).Length();
            if (fbestDist > static_cast<double>(fTriDist)) {
                fTriDist = fbestDist;
                ibestTri = portalTriangles.GetAt(static_cast<int32_t>(i));
            }
        }
    }
    if (ibestTri == 0xFFFF) {
        return false;
    }
    NiPoint3 loc = pcurrentMesh->GetCenter(static_cast<uint16_t>(ibestTri));
    TESObjectCELL* pCell = pcurrentMesh->GetParentCell();
    float fwaterHeight = TES::pInstance->GetWaterHeight(loc, pCell);
    if (loc.z < fwaterHeight + 25.0) {
        loc.z = fwaterHeight;
    }
    uint16_t usBestTriangle = static_cast<uint16_t>(ibestTri);
    const NavMeshInfo* pInfo = apCurrentNode->Location.GetBestNavMeshInfo();
    PathingLocation bestLoc(loc, pInfo, usBestTriangle);
    aRetLocation = bestLoc;
    return true;
}

bool PathBuilder::BuildNavMeshInfoPathInSpace(
    const NiPoint3& aStartLocation,
    const NiPoint3& aGoalLocation,
    ParentSpaceNode* pSpace,
    TESObjectREFR* apTeleportDoor
) {
    PathingLocation startLoc(&aStartLocation, pSpace->pInteriorCell, pSpace->pWorldSpace);
    PathingLocation goalLoc(&aGoalLocation, pSpace->pInteriorCell, pSpace->pWorldSpace);
    if (!startLoc.ResolveNavMeshInfoOnly() || !goalLoc.ResolveNavMeshInfoOnly()) {
        return false;
    }
    using NavMeshInfoArray = BSScrapArray<const NavMeshInfo*, 1024>;
    using PortalArray = BSScrapArray<TESObjectREFR*, 1024>;
    NavMeshInfoSearch infoSearch;
    NavMeshInfoArray returnNodes(20u, 0u, pScrapHeap);
    PortalArray portals(20u, 0u, pScrapHeap);
    ActorLockData* plockData = nullptr;
    // Pathfind's arguments are evaluated right to left: the goal's before the start's.
    const NavMeshInfo* pgoalMesh = goalLoc.GetBestNavMeshInfo();
    NiPoint3 goalPoint = goalLoc.GetLocation();
    const NavMeshInfo* pstartMesh = startLoc.GetBestNavMeshInfo();
    NiPoint3 startPoint = startLoc.GetLocation();
    bool binfosearchRes = infoSearch.Pathfind(
        plockData,
        startPoint,
        pstartMesh,
        goalPoint,
        pgoalMesh,
        &returnNodes,
        &portals,
        Pathing::DOOR_LOCK_DISPOSITION_NORMAL
    );
    if (!binfosearchRes) {
        if (returnNodes.IsEmpty() || !pRequest->bAllowIncompletePath) {
            return false;
        }
        pSolution->SetIncomplete(true);
    }
    pSolution->AddVirtualPathingNode(startLoc);
    for (uint32_t i = 1; i < returnNodes.iSize - 1; ++i) {
        pSolution->AddVirtualPathingNode_3(returnNodes.GetAt(static_cast<int32_t>(i)));
    }
    if (binfosearchRes) {
        if (apTeleportDoor) {
            pSolution->AddVirtualPathingNode(goalLoc, apTeleportDoor);
        }
        else {
            pSolution->AddVirtualPathingNode(goalLoc);
        }
    }
    else {
        // GetTail gives the last item's address; it is called once for the info and once for its location.
        const NavMeshInfo* plastInfo = *static_cast<const NavMeshInfo**>(returnNodes.GetTail());
        const NavMeshInfo* ptailInfo = *static_cast<const NavMeshInfo**>(returnNodes.GetTail());
        pSolution->AddVirtualPathingNode_0(ptailInfo->ApproxLocation, plastInfo);
    }
    return binfosearchRes;
}

bool PathBuilder::BuildStraightPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
    for (uint32_t i = aiFirstVirtualNodeIndex; i <= aiLastVirtualNodeIndex; ++i) {
        VirtualPathingNode* pVirtualNode = pSolution->GetVirtualNode(i);
        PathingLocation loc(*&pVirtualNode->Location);
        if (!pSolution->GetVirtualNode(i)->HasActualLocation()) {
            loc.SetLocation(loc.GetBestNavMeshInfo()->ApproxLocation);
        }
        pSolution->AddPathingNode(loc, NiPoint3::ZERO);
    }
    if (pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->IsTransitionNode()) {
        TESObjectREFR* pDoorRef = pSolution->GetVirtualNode(aiLastVirtualNodeIndex)->pDoorRef;
        pSolution->GetLastNode()->SetTeleportDoor(pDoorRef);
    }
    return true;
}

RayCastPath::RayCastPath() {}

RayCastPath::~RayCastPath() {}

void RayCastPath::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NavMeshSearch* NavMeshSearch::SetBadNodeArray(const BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray) {
    pBadNodes = apBadNodeArray;
    return this;
}

bool PathingRequest::GetFirstTangentUsesHeading() const {
    return bFirstTangentUsesHeading;
}

float PathingRequest::GetInitialPathHeading() const {
    return fInitialPathHeading;
}

PathingLocation& VirtualPathingNode::GetLocation() {
    return Location;
}

// Empties the detailed path, releasing its buffer, and forgets the loaded virtual node range.
void PathingSolution::ClearCurrentDetailedPath() {
    CurrentPathingNodes.Clear(true);
    iFirstLoadedVirtualNodeIndex = -1;
    iLastLoadedVirtualNodeIndex = -1;
}

uint32_t PathingSolution::GetVirtualNodeCount() {
    return VirtualPathingNodes.iSize;
}

uint32_t PathingSolution::AddVirtualPathingNode(const PathingLocation& apLocation) {
    uint32_t uiIndex = VirtualPathingNodes.AddUninitialized();
    new (&VirtualPathingNodes.GetAt(uiIndex)) VirtualPathingNode(apLocation);
    return uiIndex;
}

uint32_t PathingSolution::AddVirtualPathingNode(const PathingLocation& aLocation, TESObjectREFR* apDoor) {
    uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
    new (&VirtualPathingNodes.GetAt(static_cast<int32_t>(inewNodeIndex))) VirtualPathingNode(&aLocation, apDoor);
    if (apDoor) {
        uint32_t uiDoorID = apDoor->iFormID;
        DoorIDs.Add(uiDoorID);
    }
    return inewNodeIndex;
}

void PathingSolution::RemoveNode(uint32_t aiIndex) {
    CurrentPathingNodes.Remove(aiIndex, true);
}

void PathingSolution::RemoveLastNode() {
    CurrentPathingNodes.Remove(CurrentPathingNodes.GetSize() - 1, true);
}

uint32_t PathingSolution::AddVirtualPathingNode_3(const NavMeshInfo* apNavMeshInfo) {
    uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
    new (&VirtualPathingNodes.GetAt(inewNodeIndex)) VirtualPathingNode(apNavMeshInfo);
    return inewNodeIndex;
}

uint32_t PathingSolution::AddVirtualPathingNode_0(const NiPoint3& aLocation, const NavMeshInfo* apNavMeshInfo) {
    uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
    new (&VirtualPathingNodes.GetAt(inewNodeIndex)) VirtualPathingNode(&aLocation, apNavMeshInfo);
    return inewNodeIndex;
}

uint32_t
PathingSolution::AddVirtualPathingNode(const NiPoint3& aLocation, TESObjectCELL* apCell, TESWorldSpace* apWorldSpace) {
    PathingLocation loc(&aLocation, apCell, apWorldSpace);
    loc.ResolveNavMeshInfo(nullptr);
    uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
    new (&VirtualPathingNodes.GetAt(inewNodeIndex)) VirtualPathingNode(loc);
    return inewNodeIndex;
}
