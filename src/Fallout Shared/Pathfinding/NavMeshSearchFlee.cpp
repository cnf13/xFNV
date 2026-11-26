#include "Fallout Shared/Pathfinding/NavMeshSearchFlee.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cmath>
#include <cstdint>

NavMeshSearchFlee::NavMeshSearchFlee() : pCurrentRequest(nullptr), bFavorGoingTowardsGoal(false) {}

NavMeshSearchFlee::~NavMeshSearchFlee() {}

bool NavMeshSearchFlee::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pNode;
    NavMesh* pOtherMesh;
    int32_t iOtherEdge;
    uint16_t usOtherTri;
    NiPoint3 aNodeCenter = apNode->tNodeData.GetCenter();
    PathingAvoidNodeArray* pAvoidNodes = pCurrentRequest->GetAvoidNodeArray();
    NiPoint3 avoidDir = NiPoint3::ZERO;
    if (pAvoidNodes) {
        avoidDir = AvoidNodeCenter - aNodeCenter;
        avoidDir.Unitize();
    }
    if (!std::isfinite(apNode->fCost)) {
        // Resets a non-finite cost on the node it is given as const, through a non-const copy of the pointer.
        AStarNode<NavMeshSearchNode, NavMeshSearchLink>*
            pMutableNode = const_cast<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*>(apNode);
        pMutableNode->fCost = 0.0f;
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (apNode->tNodeData.pNavMesh
                ->GetMatchingEdge(apNode->tNodeData.sTriangle, i, &pOtherMesh, &usOtherTri, &iOtherEdge)) {
            EdgeExtraInfo* pExtra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if (!pExtra || pExtra->eType != EIT_LEDGE_UP) {
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
                double dActorRadius = pCurrentRequest->fActorRadius;
                double dLargeRadius = fPathingLargeActorRadius.Float();
                if (!(dLargeRadius < dActorRadius) ||
                    !pOtherMesh->GetTriangle(usOtherTri)->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR)) {
                    NiPoint3 nodeCenter = pOtherMesh->GetCenter(usOtherTri);
                    NiPoint3 toOtherNode = nodeCenter - aNodeCenter;
                    float fCost = toOtherNode.XYLength() * fDoorMultiplier + apNode->fCost;
                    if (pAvoidNodes) {
                        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
                        fCost = pAvoidNodeArray->GetCostForPoint(nodeCenter) + fCost;
                        NiPoint3 unitToOtherNode = toOtherNode;
                        unitToOtherNode.Unitize();
                        float fTowardThreat = BSMAX(0.0f, unitToOtherNode.Dot(avoidDir));
                        fCost = fTowardThreat * 1e+06 + fCost;
                    }
                    if (bFavorGoingTowardsGoal) {
                        float fDistToGoal = (nodeCenter - GoalLocation).XYLength();
                        fCost = fDistToGoal * 10.0 + fCost;
                    }
                    int32_t iNodeStatus = GetOrCreateAStarNode(GetKeyForNode(pOtherMesh, usOtherTri), fCost, &pNode);
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
    return true;
}

bool NavMeshSearchFlee::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    const PathingLocation* pDest = &pCurrentRequest->Destination;
    if (bFavorGoingTowardsGoal) {
        uint16_t sGoalTri;
        NavMeshPtr spMeshPtr;
        if (pDest->GetNavMeshAndTriangle(spMeshPtr, sGoalTri)) {
            const NavMeshSearchNode* pThisNode = &apNode->GetNodeData();
            if (pThisNode->pNavMesh == spMeshPtr.m_pObject && pThisNode->sTriangle == sGoalTri) {
                return true;
            }
        }
    }
    if (apNode == pStartNode) {
        return false;
    }
    NiPoint3 nodeCenter = apNode->tNodeData.GetCenter();
    // The squared horizontal length is stored to a float before the compare.
    float fSqrLength = (pCurrentRequest->Origin.GetLocation() - nodeCenter).SqrXYLength();
    double dMinDistance = pCurrentRequest->fMinFleeDistance;
    if (fSqrLength < pCurrentRequest->fMinFleeDistance * dMinDistance) {
        return false;
    }
    if (bFavorGoingTowardsGoal) {
        NiPoint3 destinationVec = GoalLocation - nodeCenter;
        NiPoint3 originDestinationVec = pCurrentRequest->Origin.GetLocation() - GoalLocation;
        double dOriginDestSqrLength = originDestinationVec.SqrLength();
        if (destinationVec.SqrLength() > dOriginDestSqrLength) {
            return false;
        }
    }
    float fScore = 0.0f;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        fScore = pAvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fScore <= 0.0;
}

bool NavMeshSearchFlee::BuildNodePath(
    const PathingRequestFlee* apRequest,
    const NavMeshSearchNode& aStartNode,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
    BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray
) {
    pCurrentRequest = apRequest;
    StartNode = aStartNode;
    NiPoint3 startPoint = apRequest->Origin.GetLocation();
    AvoidNodeCenter = NiPoint3::ZERO;
    fDistanceUpperBound = apRequest->fMinFleeDistance;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        pAvoidNodeArray->ComputeCenter(startPoint, AvoidNodeCenter);
    }
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pstartNode = AddStartNode(
        GetKeyForNode(aStartNode.pNavMesh, aStartNode.sTriangle),
        aStartNode,
        0.0f
    );
    SetNodeData(pstartNode, aStartNode.pNavMesh, aStartNode.sTriangle);
    if (!apRequest->Destination.QUnused()) {
        bFavorGoingTowardsGoal = true;
        GoalLocation = pCurrentRequest->Destination.GetLocation();
    }
    bool bResult = Search(apReturnNodeArray, apReturnLinkArray, pCurrentRequest->bAllowIncompletePath);
    if (!bResult && !apReturnNodeArray->IsEmpty()) {
        NavMeshSearchNode* pEndNode = &apReturnNodeArray->GetLast();
        NiPoint3 endLocation = pEndNode->GetCenter();
        float fMinDistance = pCurrentRequest->fMinFleeDistance * 0.5;
        fMinDistance = BSMIN(fMinDistance, 256.0f);
        NiPoint3 vec = startPoint - endLocation;
        if (fMinDistance * fMinDistance < vec.SqrLength()) {
            return true;
        }
    }
    return bResult;
}

void NavMeshSearchFlee::SetNodeData(
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    NavMesh* apNavMesh,
    uint16_t aiTriangle
) {
    apNode->tNodeData.pNavMesh = apNavMesh;
    apNode->tNodeData.sTriangle = aiTriangle;
    NiPoint3 center = apNavMesh->GetCenter(aiTriangle);
    apNode->tNodeData.SetCenter(center);
    NiPoint3 avoidVector = center - AvoidNodeCenter;
    float fCost = fDistanceUpperBound - avoidVector.Length();
    BSClamp(fCost, 0.0f, fDistanceUpperBound);
    apNode->fHeuristic = fCost;
}

uint32_t NavMeshSearchFlee::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) {
    return static_cast<uint32_t>(aiTri | (static_cast<uint16_t>(apNavMesh->TESForm::GetFormID()) << 16));
}

void NavMeshSearchFlee::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathingLocation& PathingRequest::GetDestination() {
    return Destination;
}

const PathingLocation& PathingRequest::GetDestination() const {
    return Destination;
}
