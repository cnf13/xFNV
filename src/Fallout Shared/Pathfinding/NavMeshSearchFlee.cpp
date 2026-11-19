#include "Fallout Shared/Pathfinding/NavMeshSearchFlee.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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
    DoorLock doorLock;
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
    NavMesh* potherMesh;
    int32_t iotherEdge;
    uint16_t usotherTri;
    NiPoint3 aNodeCenter = apNode->tNodeData.GetCenter();
    PathingAvoidNodeArray* pavoidNodes = pCurrentRequest->GetAvoidNodeArray();
    NiPoint3 avoidDir = NiPoint3_ZERO;
    if (pavoidNodes) {
        avoidDir = AvoidNodeCenter - aNodeCenter;
        avoidDir.Unitize();
    }
    if (!std::isfinite(apNode->fCost)) {
        // Resets a non-finite cost on the node it is given as const, through a non-const copy of the pointer, as the
        // game does.
        AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pmutableNode =
            const_cast<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*>(apNode);
        pmutableNode->fCost = 0.0f;
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (apNode->tNodeData.pNavMesh->GetMatchingEdge(
                apNode->tNodeData.sTriangle, i, &potherMesh, &usotherTri, &iotherEdge
            )) {
            EdgeExtraInfo* pextra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if (!pextra || pextra->eType != EIT_LEDGE_UP) {
                NavMeshTriangle* v5;
                float fdoorMultiplier = 1.0f;
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
                        if (!doorLock.CanActorUnlockDoorLock(lockData, pdoor, bmustPick)) {
                            continue;
                        }
                        if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                            fdoorMultiplier = 100.0f;
                        }
                    }
                }
                double Speed = pCurrentRequest->fActorRadius;
                double fLargeRadius = fPathingLargeActorRadius.Float();
                if (!(fLargeRadius < Speed) ||
                    (v5 = potherMesh->GetTriangle(usotherTri), !v5->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR))) {
                    NiPoint3 nodeCenter = potherMesh->GetCenter(usotherTri);
                    NiPoint3 toOtherNode = nodeCenter - aNodeCenter;
                    float fcost = toOtherNode.XYLength() * fdoorMultiplier + apNode->fCost;
                    if (pavoidNodes) {
                        PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
                        fcost = AvoidNodeArray->GetCostForPoint(nodeCenter) + fcost;
                        NiPoint3 unitToOtherNode = toOtherNode;
                        unitToOtherNode.Unitize();
                        float ftowardThreat = BSMAX(0.0f, unitToOtherNode.Dot(avoidDir));
                        fcost = ftowardThreat * 1e+06 + fcost;
                    }
                    if (bFavorGoingTowardsGoal) {
                        float fdistToGoal = (nodeCenter - GoalLocation).XYLength();
                        fcost = fdistToGoal * 10.0 + fcost;
                    }
                    int32_t v11 = AStarSearch_GetOrCreateAStarNode_6A7E00(
                        this, GetKeyForNode(potherMesh, usotherTri), fcost, &pnode
                    );
                    if (v11) {
                        if (v11 != 1) {
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

bool NavMeshSearchFlee::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    const PathingLocation* pdest = &pCurrentRequest->Destination;
    if (bFavorGoingTowardsGoal) {
        uint16_t sgoalTri;
        NavMeshPtr spmeshPtr;
        if (pdest->GetNavMeshAndTriangle(spmeshPtr, sgoalTri)) {
            const NavMeshSearchNode* thisNode = &apNode->GetNodeData();
            if (thisNode->pNavMesh == spmeshPtr.m_pObject && thisNode->sTriangle == sgoalTri) {
                return true;
            }
        }
    }
    if (apNode == pStartNode) {
        return false;
    }
    NiPoint3 nodeCenter = apNode->tNodeData.GetCenter();
    // The squared horizontal length is stored to a float before the compare.
    float fsqrLength = (pCurrentRequest->Origin.GetLocation() - nodeCenter).SqrXYLength();
    double fminDistance = pCurrentRequest->fMinFleeDistance;
    if (fsqrLength < pCurrentRequest->fMinFleeDistance * fminDistance) {
        return false;
    }
    if (bFavorGoingTowardsGoal) {
        NiPoint3 destinationVec = GoalLocation - nodeCenter;
        NiPoint3 originDestinationVec = pCurrentRequest->Origin.GetLocation() - GoalLocation;
        double foriginDestSqrLength = originDestinationVec.SqrLength();
        if (destinationVec.SqrLength() > foriginDestSqrLength) {
            return false;
        }
    }
    float fscore = 0.0f;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        fscore = AvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fscore <= 0.0;
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
    AvoidNodeCenter = NiPoint3_ZERO;
    fDistanceUpperBound = apRequest->fMinFleeDistance;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        AvoidNodeArray->ComputeCenter(startPoint, AvoidNodeCenter);
    }
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pstartNode =
        AddStartNode(GetKeyForNode(aStartNode.pNavMesh, aStartNode.sTriangle), aStartNode, 0.0f);
    SetNodeData(pstartNode, aStartNode.pNavMesh, aStartNode.sTriangle);
    if (!apRequest->Destination.QUnused()) {
        bFavorGoingTowardsGoal = true;
        GoalLocation = pCurrentRequest->Destination.GetLocation();
    }
    bool bresult = Search(apReturnNodeArray, apReturnLinkArray, pCurrentRequest->bAllowIncompletePath);
    if (!bresult && !apReturnNodeArray->IsEmpty()) {
        NavMeshSearchNode* endNode = &apReturnNodeArray->GetLast();
        NiPoint3 endLocation = endNode->GetCenter();
        float fminDistance = pCurrentRequest->fMinFleeDistance * 0.5;
        fminDistance = BSMIN(fminDistance, 256.0f);
        NiPoint3 vec = startPoint - endLocation;
        if (fminDistance * fminDistance < vec.SqrLength()) {
            return true;
        }
    }
    return bresult;
}

void NavMeshSearchFlee::SetNodeData(
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    NavMesh* apNavMesh,
    uint16_t aiTriangle
) {
    float fcost;
    apNode->tNodeData.pNavMesh = apNavMesh;
    apNode->tNodeData.sTriangle = aiTriangle;
    NiPoint3 center = apNavMesh->GetCenter(aiTriangle);
    // EXE-INLINED 0x6D61B0
    apNode->tNodeData.CenterX = center.x;
    apNode->tNodeData.CenterY = center.y;
    apNode->tNodeData.CenterZ = center.z;
    NiPoint3 avoidVector = center - AvoidNodeCenter;
    fcost = fDistanceUpperBound - avoidVector.Length();
    BSClamp(fcost, 0.0f, fDistanceUpperBound);
    apNode->fHeuristic = fcost;
}

uint32_t NavMeshSearchFlee::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) {
    return (uint32_t)(aiTri | (((uint16_t)apNavMesh->TESForm::GetFormID()) << 16));
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
