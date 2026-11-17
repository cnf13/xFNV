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

#include <stdint.h>

NavMeshSearchHide::NavMeshSearchHide() : pCurrentHideRequest(nullptr) {}

NavMeshSearchHide::~NavMeshSearchHide() {}

bool NavMeshSearchHide::GetNodeConnections(
    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
    BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes
) {
    NiPoint3* Center;
    NiPoint3 result;
    NiPoint3 apLocation;
    DoorLock doorLock;
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
    NavMesh* potherMesh;
    int32_t iotherEdge;
    uint16_t usotherTri;
    if (apNode->pParentAStarNode) {
        Center = apNode->tNodeData.GetCenter(&apLocation);
    }
    else {
        Center = ::new (&result) NiPoint3(pCurrentHideRequest->Origin.GetLocation());
    }
    NiPoint3 aCurrentLocation = *Center;
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
                    if (pCurrentHideRequest->bCantOpenDoors) {
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
                double Speed = pCurrentHideRequest->fActorRadius;
                double fLargeRadius = fPathingLargeActorRadius.Float();
                if (!(fLargeRadius < Speed) ||
                    (v5 = potherMesh->GetTriangle(usotherTri), !v5->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR))) {
                    NiPoint3 nodeCenter = potherMesh->GetCenter(usotherTri);
                    NiPoint3 nodeToNode = nodeCenter - aCurrentLocation;
                    float fcost = nodeToNode.XYLength() * fdoorMultiplier + apNode->fCost;
                    float fnodescore = 0.0f;
                    if (pCurrentHideRequest->GetAvoidNodeArray()) {
                        PathingAvoidNodeArray* AvoidNodeArray = pCurrentHideRequest->GetAvoidNodeArray();
                        fnodescore = AvoidNodeArray->GetCostForPoint(nodeCenter);
                    }
                    float fdistToThreat = (nodeCenter - ThreatLocation).XYLength();
                    float fpositionalPenalty = 1.0 - fdistToThreat / 512.0;
                    fnodescore = BSMAX(fpositionalPenalty, 0.0f) + fnodescore;
                    NiPoint3 toThreat = ThreatLocation - aCurrentLocation;
                    float fdirectionalPenalty = toThreat.Dot(nodeToNode);
                    float ftoThreatLength = toThreat.Length();
                    float fnodeToNodeLength = nodeToNode.Length();
                    if (ftoThreatLength == 0.0 || fnodeToNodeLength == 0.0) {
                        fdirectionalPenalty = 0.0f;
                    }
                    else {
                        fdirectionalPenalty = fdirectionalPenalty / (ftoThreatLength * fnodeToNodeLength);
                    }
                    fnodescore = BSMAX(fdirectionalPenalty, 0.0f) + fnodescore;
                    fcost = (fnodescore + 1.0) * fcost;
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
    return 1;
}

bool NavMeshSearchHide::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    uint16_t usVert0;
    uint16_t usVert1;
    uint16_t usVert2;
    NiPoint3 nodeCenter;
    const NavMeshSearchNode* parentNodeData = &apNode->GetNodeData();
    const NavMeshTriangle* ptriangle = parentNodeData->GetTriangle();
    if (!ptriangle->HasCover()) {
        return 0;
    }
    apNode->tNodeData.GetCenter(&nodeCenter);
    // The horizontal length is stored to a float before the compare.
    float flength = (ThreatLocation - nodeCenter).XYLength();
    double fminDistance = pCurrentHideRequest->fMinHideDistance;
    if (flength < fminDistance) {
        return 0;
    }
    NavMesh* pmesh = parentNodeData->pNavMesh;
    ptriangle->GetVerts(usVert0, usVert1, usVert2);
    NiPoint3 barycentricCoords;
    bool bproperSide = 0;
    NavMeshVertex* pvert2 = pmesh->GetVertex(usVert2);
    NavMeshVertex* pvert1 = pmesh->GetVertex(usVert1);
    NavMeshVertex* pvert0 = pmesh->GetVertex(usVert0);
    NavMeshMath::Get2DBaryCentricCoord(
        pvert0->Location, pvert1->Location, pvert2->Location, ThreatLocation, barycentricCoords
    );
    if (barycentricCoords.x < 0.0 && ptriangle->GetEdgeCover(1)) {
        bproperSide = 1;
    }
    else {
        if (barycentricCoords.z < 0.0) {
            if (ptriangle->GetEdgeCover(0)) {
                bproperSide = 1;
            }
        }
    }
    if (!bproperSide) {
        return 0;
    }
    float fscore = 0.0f;
    if (pCurrentHideRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* AvoidNodeArray = pCurrentHideRequest->GetAvoidNodeArray();
        fscore = AvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fscore <= 0.0;
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
    AvoidNodeCenter = NiPoint3_ZERO;
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
    NiPoint3 kNodeCenter;
    float fHeuristic = fDistanceUpperBound - (*kStartNode.GetCenter(&kNodeCenter) - AvoidNodeCenter).Length();
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

const PathingLocation& PathingRequestHide::GetThreatLocation() const {
    return ThreatLocation;
}
