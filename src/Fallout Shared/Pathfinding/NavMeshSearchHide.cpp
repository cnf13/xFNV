#include "Fallout Shared/Pathfinding/NavMeshSearchHide.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestHide.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>

NavMeshSearchHide::NavMeshSearchHide() : pCurrentHideRequest(nullptr) {}

NavMeshSearchHide::~NavMeshSearchHide() {}

bool NavMeshSearchHide::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pNode;
    NavMesh* pOtherMesh;
    int32_t iOtherEdge;
    uint16_t usOtherTri;
    NiPoint3 aCurrentLocation = apNode->pParentAStarNode ? apNode->tNodeData.GetCenter()
                                                         : pCurrentHideRequest->Origin.GetLocation();
    for (int32_t i = 0; i < 3; ++i) {
        if (apNode->tNodeData.pNavMesh
                ->GetMatchingEdge(apNode->tNodeData.sTriangle, i, &pOtherMesh, &usOtherTri, &iOtherEdge)) {
            EdgeExtraInfo* pExtra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if (!pExtra || pExtra->eType != EIT_LEDGE_UP) {
                float fDoorMultiplier = 1.0f;
                NavMeshTriangle* pTriangle = pOtherMesh->GetTriangle(usOtherTri);
                if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
                    if (pCurrentHideRequest->bCantOpenDoors) {
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
                double dActorRadius = pCurrentHideRequest->fActorRadius;
                double dLargeRadius = fPathingLargeActorRadius.Float();
                if (!(dLargeRadius < dActorRadius) ||
                    !pOtherMesh->GetTriangle(usOtherTri)->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR)) {
                    NiPoint3 nodeCenter = pOtherMesh->GetCenter(usOtherTri);
                    NiPoint3 nodeToNode = nodeCenter - aCurrentLocation;
                    float fCost = nodeToNode.XYLength() * fDoorMultiplier + apNode->fCost;
                    float fNodeScore = 0.0f;
                    if (pCurrentHideRequest->GetAvoidNodeArray()) {
                        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentHideRequest->GetAvoidNodeArray();
                        fNodeScore = pAvoidNodeArray->GetCostForPoint(nodeCenter);
                    }
                    float fDistToThreat = (nodeCenter - ThreatLocation).XYLength();
                    float fPositionalPenalty = 1.0 - fDistToThreat / 512.0;
                    fNodeScore = BSMAX(fPositionalPenalty, 0.0f) + fNodeScore;
                    NiPoint3 toThreat = ThreatLocation - aCurrentLocation;
                    float fDirectionalPenalty = toThreat.Dot(nodeToNode);
                    float fToThreatLength = toThreat.Length();
                    float fNodeToNodeLength = nodeToNode.Length();
                    if (fToThreatLength == 0.0 || fNodeToNodeLength == 0.0) {
                        fDirectionalPenalty = 0.0f;
                    }
                    else {
                        fDirectionalPenalty = fDirectionalPenalty / (fToThreatLength * fNodeToNodeLength);
                    }
                    fNodeScore = BSMAX(fDirectionalPenalty, 0.0f) + fNodeScore;
                    fCost = (fNodeScore + 1.0) * fCost;
                    int32_t iNodeResult = GetOrCreateAStarNode(GetKeyForNode(pOtherMesh, usOtherTri), fCost, &pNode);
                    if (iNodeResult) {
                        if (iNodeResult != 1) {
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

bool NavMeshSearchHide::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    uint16_t usVert0;
    uint16_t usVert1;
    uint16_t usVert2;
    const NavMeshSearchNode* pParentNodeData = &apNode->GetNodeData();
    const NavMeshTriangle* pTriangle = pParentNodeData->GetTriangle();
    if (!pTriangle->HasCover()) {
        return false;
    }
    NiPoint3 nodeCenter = apNode->tNodeData.GetCenter();
    // The horizontal length is stored to a float before the compare.
    float fLength = (ThreatLocation - nodeCenter).XYLength();
    double dMinDistance = pCurrentHideRequest->fMinHideDistance;
    if (fLength < dMinDistance) {
        return false;
    }
    NavMesh* pMesh = pParentNodeData->pNavMesh;
    pTriangle->GetVerts(usVert0, usVert1, usVert2);
    NiPoint3 barycentricCoords;
    bool bProperSide = false;
    NavMeshVertex* pVert2 = pMesh->GetVertex(usVert2);
    NavMeshVertex* pVert1 = pMesh->GetVertex(usVert1);
    NavMeshVertex* pVert0 = pMesh->GetVertex(usVert0);
    NavMeshMath::Get2DBaryCentricCoord(
        pVert0->Location,
        pVert1->Location,
        pVert2->Location,
        ThreatLocation,
        barycentricCoords
    );
    if (barycentricCoords.x < 0.0 && pTriangle->GetEdgeCover(1)) {
        bProperSide = true;
    }
    else if (barycentricCoords.z < 0.0 && pTriangle->GetEdgeCover(0)) {
        bProperSide = true;
    }
    if (!bProperSide) {
        return false;
    }
    float fScore = 0.0f;
    if (pCurrentHideRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* pAvoidNodeArray = pCurrentHideRequest->GetAvoidNodeArray();
        fScore = pAvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fScore <= 0.0;
}

// The start node's center is the center of its triangle, and its heuristic is how far inside the avoided area it
// lies. The threat location is read after the start node is added.
bool NavMeshSearchHide::BuildNodePath(
    const PathingRequestHide* apRequest,
    const NavMeshSearchNode& aStartNode,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
    BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray
) {
    pCurrentRequest = apRequest;
    pCurrentHideRequest = apRequest;
    StartNode = aStartNode;
    NiPoint3 kOrigin = apRequest->GetOrigin().GetLocation();
    AvoidNodeCenter = NiPoint3::ZERO;
    fDistanceUpperBound = 0.0f;
    if (pCurrentHideRequest->GetAvoidNodeArray()) {
        fDistanceUpperBound = pCurrentHideRequest->GetAvoidNodeArray()->ComputeCenter(kOrigin, AvoidNodeCenter);
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
    ThreatLocation = pCurrentHideRequest->GetThreatLocation().GetLocation();
    Search(apReturnNodeArray, apReturnLinkArray, true);
    return !apReturnNodeArray->IsEmpty();
}

void NavMeshSearchHide::SetNodeData(
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

const PathingLocation& PathingRequestHide::GetThreatLocation() const {
    return ThreatLocation;
}
