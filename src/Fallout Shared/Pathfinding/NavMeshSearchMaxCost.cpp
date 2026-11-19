#include "Fallout Shared/Pathfinding/NavMeshSearchMaxCost.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

NavMeshSearchMaxCost::NavMeshSearchMaxCost() : fMaxDist(0.0f), bHasGoalNode(false) {}

NavMeshSearchMaxCost::~NavMeshSearchMaxCost() {}

bool NavMeshSearchMaxCost::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    DoorLock doorLock;
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
    NavMesh* potherMesh;
    int32_t iotherEdge;
    uint16_t usotherTri;
    for (int32_t i = 0; i < 3; ++i) {
        if (apNode->tNodeData.pNavMesh->GetMatchingEdge(
                apNode->tNodeData.sTriangle, i, &potherMesh, &usotherTri, &iotherEdge
            )) {
            EdgeExtraInfo* pextra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if (!pextra || pextra->eType != EIT_LEDGE_UP) {
                NiPoint3 nodeCenter = potherMesh->GetCenter(usotherTri);
                float fcost = (nodeCenter - apNode->tNodeData.GetCenter()).XYLength() + apNode->fCost;
                if (!(fMaxDist < static_cast<double>(fcost))) {
                    NavMeshTriangle* Triangle = potherMesh->GetTriangle(usotherTri);
                    if (Triangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
                        if (pCurrentRequest->bCantOpenDoors) {
                            continue;
                        }
                        uint32_t doorID = potherMesh->GetClosedDoorInfoForTriangle(usotherTri)->DoorFormID;
                        TESObjectREFR* pdoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(doorID));
                        if (pdoor) {
                            bool bmustPick;
                            doorLock.~DoorLock();
                            ::new (&doorLock) DoorLock(pdoor);
                            const ActorLockData* lockData = &pCurrentRequest->LockData;
                            if (!doorLock.CanActorUnlockDoorLock(lockData, pdoor, bmustPick) ||
                                static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())
                                    ->TESObjectDOOR::IsMinimalUse()) {
                                continue;
                            }
                        }
                    }
                    int32_t v6 = AStarSearch_GetOrCreateAStarNode_6A7E00(
                        this, GetKeyForNode(potherMesh, usotherTri), fcost, &pnode
                    );
                    if (v6) {
                        if (v6 != 1) {
                            continue;
                        }
                    }
                    else {
                        SetNodeData(pnode, potherMesh, usotherTri);
                    }
                    pnode->ParentLink.pNavMesh = apNode->tNodeData.pNavMesh;
                    pnode->ParentLink.sTriangle = apNode->tNodeData.sTriangle;
                    pnode->ParentLink.iEdgeIndex = i;
                    apRetNodes->Add(pnode);
                }
            }
        }
    }
    return true;
}

bool NavMeshSearchMaxCost::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    if (bHasGoalNode) {
        return (bool)((int8_t)NavMeshSearch::IsGoalNode(apNode));
    }

    return false;
}

NiPoint3 NavMeshSearchMaxCost::FindClosestPoint(
    const NiPoint3& aStartLocation,
    const FastNavMeshTriHandle& aStartNode,
    const NiPoint3& aGoalLocation,
    const FastNavMeshTriHandle& aGoalNode,
    float afMaxDist
) {
    NiPoint3 result;
    StartNode.pNavMesh = aStartNode.pNavMesh;
    StartNode.sTriangle = aStartNode.usTriangle;
    NiPoint3 center = StartNode.pNavMesh->GetCenter(StartNode.sTriangle);
    StartNode.CenterX = center.x;
    StartNode.CenterY = center.y;
    StartNode.CenterZ = center.z;
    // The goal's center is built before the start's.
    NiPoint3 apLocation = GoalNode.GetCenter();
    float afHeuristic = (StartNode.GetCenter() - apLocation).Length();
    AddStartNode(GetKeyForNode(StartNode.pNavMesh, StartNode.sTriangle), StartNode, afHeuristic);
    if (!aGoalNode.pNavMesh && aGoalNode.usTriangle == 0xFFFF) {
        bHasGoalNode = false;
    }
    else {
        bHasGoalNode = true;
        GoalNode.pNavMesh = aGoalNode.pNavMesh;
        GoalNode.sTriangle = aGoalNode.usTriangle;
        center = GoalNode.pNavMesh->GetCenter(GoalNode.sTriangle);
        GoalNode.CenterX = center.x;
        GoalNode.CenterY = center.y;
        GoalNode.CenterZ = center.z;
    }
    fMaxDist = afMaxDist;
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    nodes_t nodes;
    BSScrapArray<NavMeshSearchLink, 1024> links;
    if (Search(&nodes, &links, false)) {
        result = aGoalLocation;
    }
    else {
        result = GetBestAStarNode()->tNodeData.GetCenter();
    }
    return result;
}

bool NavMeshSearchMaxCost::BuildNodePath(
    const PathingRequestOptimalLocation* apRequest,
    const NavMeshSearchNode& aStartNode,
    PathingLocation& aOptimalLocation,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
    BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray
) {
    StartNode = aStartNode;
    pCurrentRequest = apRequest;
    apReturnNodeArray->Clear(false);
    apReturnLinkArray->Clear(false);
    ClearSearchData();
    if (!aOptimalLocation.QUnused() && aOptimalLocation.ResolveNavMeshInfo(nullptr)) {
        uint16_t usgoalTri;
        NavMeshPtr spgoalMesh;
        if (aOptimalLocation.GetNavMeshAndTriangle(spgoalMesh, usgoalTri)) {
            GoalNode.pNavMesh = spgoalMesh.m_pObject;
            GoalNode.sTriangle = usgoalTri;
            NiPoint3 center = GoalNode.pNavMesh->GetCenter(GoalNode.sTriangle);
            GoalNode.CenterX = center.x;
            GoalNode.CenterY = center.y;
            GoalNode.CenterZ = center.z;
            bHasGoalNode = true;
        }
    }
    // The goal's center is built before the start's.
    NiPoint3 apLocation = GoalNode.GetCenter();
    float afHeuristic = (StartNode.GetCenter() - apLocation).Length();
    AddStartNode(GetKeyForNode(aStartNode.pNavMesh, aStartNode.sTriangle), aStartNode, afHeuristic);
    return Search(apReturnNodeArray, apReturnLinkArray, false);
}
