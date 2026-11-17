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
    DoorLock doorLock;
    AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
    NavMesh* potherMesh;
    int32_t iotherEdge;
    uint16_t usotherTri;
    NiPoint3 aNodeCenter;
    apNode->tNodeData.GetCenter(&aNodeCenter);
    int32_t irandom = Random() % 6;
    int32_t istart = irandom / 2;
    int32_t istep = (irandom & 1) + 1;
    int32_t count = 0;
    int32_t i = irandom / 2;
    while (count < 3) {
        if (apNode->tNodeData.pNavMesh->GetMatchingEdge(
                apNode->tNodeData.sTriangle, i, &potherMesh, &usotherTri, &iotherEdge
            )) {
            EdgeExtraInfo* pextra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
            if (!pextra || pextra->eType != EIT_LEDGE_UP) {
                NavMeshTriangle* v4;
                float fdoorMultiplier = 1.0f;
                NavMeshTriangle* Triangle = potherMesh->GetTriangle(usotherTri);
                if (Triangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
                    if (pCurrentRequest->bCantOpenDoors) {
                        goto LABEL_2;
                    }
                    uint32_t doorID = potherMesh->GetClosedDoorInfoForTriangle(usotherTri)->DoorFormID;
                    TESObjectREFR* pdoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(doorID));
                    if (pdoor) {
                        bool bmustPick;
                        doorLock.~DoorLock();
                        ::new (&doorLock) DoorLock(pdoor);
                        const ActorLockData* lockData = &pCurrentRequest->LockData;
                        if (!doorLock.CanActorUnlockDoorLock(lockData, pdoor, bmustPick)) {
                            goto LABEL_2;
                        }
                        if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                            fdoorMultiplier = 100.0f;
                        }
                    }
                }
                double Speed = pCurrentRequest->fActorRadius;
                if (!(fPathingLargeActorRadius.Float() < Speed) ||
                    (v4 = potherMesh->GetTriangle(usotherTri), !v4->GetFlag(16))) {
                    float fcost = 0.0f;
                    if (fdoorMultiplier != 1.0) {
                        fcost = fdoorMultiplier;
                    }
                    int32_t v6 = AStarSearch_GetOrCreateAStarNode_6A7E00(
                        this, GetKeyForNode(potherMesh, usotherTri), fcost, &pnode
                    );
                    if (v6) {
                        if (v6 != 1) {
                            goto LABEL_2;
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
    LABEL_2:;
        ++count;
        i = (istep + i) % 3;
    }
    return true;
}

bool NavMeshSearchClosePoint::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    NiPoint3 nodeCenter;
    apNode->tNodeData.GetCenter(&nodeCenter);
    // The horizontal length is stored to a float local before the compare.
    float flength = (pCurrentRequest->Origin.GetLocation() - nodeCenter).XYLength();
    if (fMinDistance > static_cast<double>(flength)) {
        return false;
    }
    NavMeshInfo* NavMeshInfo_ = apNode->tNodeData.pNavMesh->GetNavMeshInfo();
    PathingLocation TempLoc(&nodeCenter, NavMeshInfo_);
    float afRadius = pCurrentRequest->fActorRadius;
    if (!Pathing::CheckSphere(TempLoc, afRadius)) {
        return false;
    }
    float fscore = 0.0f;
    if (pCurrentRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
        fscore = AvoidNodeArray->GetCostForPoint(nodeCenter);
    }
    return fscore <= 0.0;
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
    AvoidNodeCenter = NiPoint3_ZERO;
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
    NiPoint3 kNodeCenter;
    float fHeuristic = fDistanceUpperBound - (*kStartNode.GetCenter(&kNodeCenter) - AvoidNodeCenter).Length();
    BSClamp(fHeuristic, 0.0f, fDistanceUpperBound);
    AddStartNode(GetKeyForNode(kStartNode.pNavMesh, kStartNode.sTriangle), kStartNode, fHeuristic);
    Search(apReturnNodeArray, apReturnLinkArray, true);
    return !apReturnNodeArray->IsEmpty();
}

uint32_t NavMeshSearchClosePoint::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const {
    return (uint32_t)(aiTri | (((uint16_t)apNavMesh->TESForm::GetFormID()) << 16));
}

void NavMeshSearchClosePoint::SetNodeData(
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
