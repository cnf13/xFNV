#include "Fallout Shared/Pathfinding/NavMeshSearchLOS.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLOSMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <limits>

NavMeshSearchLOS::NavMeshSearchLOS()
  : pCurrentRequest(nullptr), spLOSMap(nullptr), bHasGoalNode(false), cMinLOSScore(-3), pFirstIgnoredGoal(nullptr),
    pLastValidNode(nullptr), pClosestNode(nullptr), fClosestNodeDistanceSquared(std::numeric_limits<float>::max()) {}

NavMeshSearchLOS::~NavMeshSearchLOS() {}

bool NavMeshSearchLOS::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    NiPoint3 kSearchCenter;
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pNode;
    NavMesh* pOtherMesh;
    int32_t iOtherEdge;
    uint16_t usOtherTri;
    NiPoint3 aNodeCenter = apNode->tNodeData.GetCenter();
    for (int32_t i = 0; i < 3; ++i) {
        if (apNode->tNodeData.pNavMesh
                ->GetMatchingEdge(apNode->tNodeData.sTriangle, i, &pOtherMesh, &usOtherTri, &iOtherEdge)) {
            EdgeExtraInfo* pExtra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if ((!pExtra || pExtra->eType != EIT_LEDGE_UP) &&
                (pOtherMesh == apNode->tNodeData.pNavMesh || spLOSMap->IsNavMeshInMap(pOtherMesh->iFormID))) {
                double dRadius;
                double dDistanceSquared;
                float fDoorMultiplier = 1.0f;
                NavMeshTriangle* pTriangle = pOtherMesh->GetTriangle(usOtherTri);
                if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
                    if (pCurrentRequest->bCantOpenDoors) {
                        continue;
                    }
                    uint32_t uiDoorID = pOtherMesh->GetClosedDoorInfoForTriangle(usOtherTri)->DoorFormID;
                    TESObjectREFR* pDoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(uiDoorID));
                    if (pDoor) {
                        bool bMustPick;
                        DoorLock doorLock(pDoor);
                        const ActorLockData* pLockData = &pCurrentRequest->LockData;
                        if (!doorLock.CanActorUnlockDoorLock(pLockData, pDoor, bMustPick)) {
                            continue;
                        }
                        if (static_cast<TESObjectDOOR*>(pDoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                            fDoorMultiplier = 100.0f;
                        }
                    }
                }
                if (pCurrentRequest->fSearchCenterRadius == 0.0 ||
                    (kSearchCenter = pCurrentRequest->GetSearchCenter(),
                     dDistanceSquared = (pOtherMesh->GetCenter(usOtherTri) - kSearchCenter).SqrLength(),
                     dRadius = pCurrentRequest->fSearchCenterRadius,
                     !(pCurrentRequest->fSearchCenterRadius * dRadius < dDistanceSquared))) {
                    double dActorRadius = pCurrentRequest->fActorRadius;
                    double dLargeActorRadius = fPathingLargeActorRadius.Float();
                    if (!(dLargeActorRadius < dActorRadius) ||
                        !pOtherMesh->GetTriangle(usOtherTri)->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR)) {
                        NiPoint3 nodeCenter = pOtherMesh->GetCenter(usOtherTri);
                        float fCost = (aNodeCenter - nodeCenter).XYLength();
                        float fNodeScore = 0.0f;
                        if (pCurrentRequest->GetAvoidNodeArray()) {
                            PathingAvoidNodeArray* pAvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
                            fNodeScore = pAvoidNodeArray->GetCostForPoint(nodeCenter);
                        }
                        if (bHasGoalNode) {
                            NiPoint3 deltaToGoal = aNodeCenter - GoalNode.GetCenter();
                            NiPoint3 deltaToNode = aNodeCenter - nodeCenter;
                            deltaToGoal.Unitize();
                            deltaToNode.Unitize();
                            double dDot = deltaToGoal.Dot(deltaToNode);
                            fCost = (1.0 - dDot) * fCost;
                            fCost = BSMAX(fCost, 0.0f);
                        }
                        fCost = (apNode->fCost + fCost) * (fNodeScore + 1.0);
                        fCost = fCost * fDoorMultiplier;
                        uint32_t uiKey = GetKeyForNode(pOtherMesh, usOtherTri);
                        int32_t iNodeStatus = GetOrCreateAStarNode(uiKey, fCost, &pNode);
                        if (iNodeStatus) {
                            if (iNodeStatus != 1) {
                                continue;
                            }
                        }
                        else {
                            SetNodeData(pNode, pOtherMesh, usOtherTri);
                        }
                        pNode->ParentLink.pNavMesh = apNode->tNodeData.pNavMesh;
                        pNode->ParentLink.sTriangle = apNode->tNodeData.sTriangle;
                        pNode->ParentLink.iEdgeIndex = i;
                        apRetNodes->Add(pNode);
                    }
                }
            }
        }
    }
    return true;
}

bool NavMeshSearchLOS::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    uint16_t sTriangle = apNode->tNodeData.sTriangle;
    uint32_t uiNavMeshID = apNode->tNodeData.pNavMesh->iFormID;
    char cScore = spLOSMap->GetAt(uiNavMeshID, sTriangle);
    if (cScore >= 0) {
        if (cScore == 127) {
            if (!pLastValidNode &&
                (apNode->GetNodeData().sTriangle != StartNode.sTriangle ||
                 apNode->GetNodeData().pNavMesh != StartNode.pNavMesh)) {
                NiPoint3 nodeVector = spLOSMap->GetCenter() - apNode->GetNodeData().GetCenter();
                float fNodeDistanceSquared = nodeVector.SqrLength();
                if (fClosestNodeDistanceSquared > static_cast<double>(fNodeDistanceSquared)) {
                    pClosestNode = apNode;
                    fClosestNodeDistanceSquared = fNodeDistanceSquared;
                }
            }
        }
        else {
            pLastValidNode = apNode;
        }
        return false;
    }

    const BSSimpleArray<NiPoint3, 1024>* pIgnoreLocations = &pCurrentRequest->IgnoreLocations;
    for (uint32_t i = 0; i < pIgnoreLocations->iSize; ++i) {
        NiPoint3 triangleCenter = apNode->tNodeData.GetCenter();
        NiPoint3 vec = triangleCenter - pIgnoreLocations->GetAt(static_cast<int32_t>(i));
        // The squared distance is kept as a double.
        double dDistanceSquared = vec.SqrXYLength();
        double dRadius = pCurrentRequest->fIgnoreLocationRadius;
        if (pCurrentRequest->fIgnoreLocationRadius * dRadius > dDistanceSquared) {
            if (!pFirstIgnoredGoal) {
                pFirstIgnoredGoal = apNode;
            }
            return false;
        }
    }
    return true;
}

bool NavMeshSearchLOS::BuildNodePath(
    const PathingRequestLOS* apRequest,
    const NavMeshSearchNode& aStartNode,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
    BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray
) {
    float fHeuristic;
    uint16_t usGoalNodeTri;
    pCurrentRequest = apRequest;
    StartNode = aStartNode;
    if (!apRequest->spLOSGridMap.m_pObject) {
        return false;
    }
    spLOSMap = new PathingLOSMap(apRequest->spLOSGridMap->GetLocation(), apRequest->spLOSGridMap->fRadius);
    apRequest->spLOSGridMap->SetLOSMapFromSeenTriangles(spLOSMap);
    pFirstIgnoredGoal = nullptr;
    // Reads the origin's location and drops it.
    apRequest->GetOrigin().GetLocation();
    StartNode.pNavMesh = aStartNode.pNavMesh;
    StartNode.sTriangle = aStartNode.sTriangle;
    NiPoint3 center = StartNode.pNavMesh->GetCenter(StartNode.sTriangle);
    StartNode.CenterX = center.x;
    StartNode.CenterY = center.y;
    StartNode.CenterZ = center.z;
    if (bHasGoalNode) {
        // The goal's center is built before the start's.
        NiPoint3 kGoalCenter = GoalNode.GetCenter();
        fHeuristic = (StartNode.GetCenter() - kGoalCenter).XYLength();
    }
    else {
        fHeuristic = 0.0f;
    }
    AddStartNode(GetKeyForNode(StartNode.pNavMesh, StartNode.sTriangle), StartNode, fHeuristic);
    NavMeshPtr spGoalNodeMesh;
    bHasGoalNode = false;
    PathingLocation goalLoc(pCurrentRequest->Destination);
    if (goalLoc.ResolveNavMeshInfo(nullptr) && goalLoc.GetNavMeshAndTriangle(spGoalNodeMesh, usGoalNodeTri)) {
        bHasGoalNode = true;
        GoalNode.pNavMesh = spGoalNodeMesh.m_pObject;
        GoalNode.sTriangle = usGoalNodeTri;
        NiPoint3 goalNodeCenter = GoalNode.pNavMesh->GetCenter(GoalNode.sTriangle);
        GoalNode.CenterX = goalNodeCenter.x;
        GoalNode.CenterY = goalNodeCenter.y;
        GoalNode.CenterZ = goalNodeCenter.z;
    }
    cMinLOSScore = BSMIN(spLOSMap->iMinScore + 1, -1);
    // goalLoc and then spGoalNodeMesh are destroyed after Search, on each return path.
    return Search(apReturnNodeArray, apReturnLinkArray, false);
}

void NavMeshSearchLOS::SetNodeData(
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    NavMesh* apNavMesh,
    uint16_t aiTriangle
) {
    apNode->tNodeData.pNavMesh = apNavMesh;
    apNode->tNodeData.sTriangle = aiTriangle;
    NiPoint3 center = apNavMesh->GetCenter(aiTriangle);
    apNode->tNodeData.CenterX = center.x;
    apNode->tNodeData.CenterY = center.y;
    apNode->tNodeData.CenterZ = center.z;
    if (bHasGoalNode) {
        apNode->fHeuristic = (center - GoalNode.GetCenter()).XYLength();
    }
    else {
        apNode->fHeuristic = 0.0f;
    }
}

uint32_t NavMeshSearchLOS::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const {
    return static_cast<uint32_t>(aiTri | (static_cast<uint16_t>(apNavMesh->TESForm::GetFormID()) << 16));
}

char PathingLOSMap::GetAt(uint32_t aNavMeshID, uint16_t ausTriangle) {
    uint32_t uiOffset;
    if (!NavMeshToOffsetMap.GetAt(aNavMeshID, uiOffset)) {
        return 127;
    }
    if (uiOffset + ausTriangle < iTotalTriangleCount) {
        return pTriangleScores[uiOffset + ausTriangle];
    }
    return 127;
}

NiPoint3 PathingLOSMap::GetCenter() {
    return Center;
}

NiPoint3 PathingRequestLOS::GetSearchCenter() const {
    return SearchCenter;
}
