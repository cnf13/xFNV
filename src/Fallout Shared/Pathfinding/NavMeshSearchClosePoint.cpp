#include "Fallout Shared/Pathfinding/NavMeshSearchClosePoint.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>

NavMeshSearchClosePoint::NavMeshSearchClosePoint() : pCurrentRequest(nullptr) {}

NavMeshSearchClosePoint::~NavMeshSearchClosePoint() {}

bool NavMeshSearchClosePoint::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pNode;
    NavMesh* pOtherMesh;
    int32_t iOtherEdge;
    uint16_t usOtherTri;
    int32_t iRandom = Random() % 6;
    int32_t iStart = iRandom / 2;
    int32_t iStep = (iRandom & 1) + 1;
    int32_t i = iStart;
    for (int32_t count = 0; count < 3; ++count, i = (iStep + i) % 3) {
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
                        if (!doorLock.CanActorUnlockDoorLock(&pCurrentRequest->LockData, pDoor, bMustPick)) {
                            continue;
                        }
                        if (static_cast<TESObjectDOOR*>(pDoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                            fDoorMultiplier = 100.0f;
                        }
                    }
                }
                double dActorRadius = pCurrentRequest->fActorRadius;
                if (!(fPathingLargeActorRadius.Float() < dActorRadius) ||
                    !pOtherMesh->GetTriangle(usOtherTri)->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR)) {
                    float fCost = 0.0f;
                    if (fDoorMultiplier != 1.0) {
                        fCost = fDoorMultiplier;
                    }
                    int32_t iResult = GetOrCreateAStarNode(GetKeyForNode(pOtherMesh, usOtherTri), fCost, &pNode);
                    if (iResult) {
                        if (iResult != 1) {
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

bool NavMeshSearchClosePoint::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    NiPoint3 nodeCenter = apNode->tNodeData.GetCenter();
    // The horizontal length is stored to a float local before the compare.
    float fLength = (pCurrentRequest->Origin.GetLocation() - nodeCenter).XYLength();
    if (fMinDistance > static_cast<double>(fLength)) {
        return false;
    }
    NavMeshInfo* pNavMeshInfo = apNode->tNodeData.pNavMesh->GetNavMeshInfo();
    PathingLocation TempLoc(&nodeCenter, pNavMeshInfo);
    float fRadius = pCurrentRequest->fActorRadius;
    if (!Pathing::CheckSphere(TempLoc, fRadius)) {
        return false;
    }
    float fScore = 0.0f;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        fScore = pAvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fScore <= 0.0;
}

// The start node's center is the center of its triangle, and its heuristic is how far inside the avoided area it lies.
bool NavMeshSearchClosePoint::BuildNodePath(
    const PathingRequestClosePoint* apRequest,
    const NavMeshSearchNode& aStartNode,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
    BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray
) {
    pCurrentRequest = apRequest;
    StartNode = aStartNode;
    float fMaxDistance = apRequest->GetMaxDistance();
    float fMinDist = apRequest->GetMinDistance();
    fMinDistance = RandomFloat(fMinDist, fMaxDistance);
    NiPoint3 kOrigin = apRequest->GetOrigin().GetLocation();
    AvoidNodeCenter = NiPoint3::ZERO;
    fDistanceUpperBound = 0.0f;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        fDistanceUpperBound = pCurrentRequest->GetAvoidNodeArray()->ComputeCenter(kOrigin, AvoidNodeCenter);
    }

    NavMeshSearchNode kStartNode;
    kStartNode.pNavMesh = aStartNode.pNavMesh;
    kStartNode.sTriangle = aStartNode.sTriangle;
    NiPoint3 kCenter = kStartNode.pNavMesh->GetCenter(kStartNode.sTriangle);
    kStartNode.CenterX = kCenter.x;
    kStartNode.CenterY = kCenter.y;
    kStartNode.CenterZ = kCenter.z;
    float fHeuristic = fDistanceUpperBound - (kStartNode.GetCenter() - AvoidNodeCenter).Length();
    BSClamp(fHeuristic, 0.0f, fDistanceUpperBound);
    AddStartNode(GetKeyForNode(kStartNode.pNavMesh, kStartNode.sTriangle), kStartNode, fHeuristic);
    Search(apReturnNodeArray, apReturnLinkArray, true);
    return !apReturnNodeArray->IsEmpty();
}

uint32_t NavMeshSearchClosePoint::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const {
    return static_cast<uint32_t>(aiTri | (static_cast<uint16_t>(apNavMesh->TESForm::GetFormID()) << 16));
}

void NavMeshSearchClosePoint::SetNodeData(
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

void NavMeshSearchClosePoint::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathingLocation& PathingRequest::GetOrigin() {
    return Origin;
}

const PathingLocation& PathingRequest::GetOrigin() const {
    return Origin;
}

float PathingRequestClosePoint::GetMaxDistance() const {
    return fMaxDistance;
}
