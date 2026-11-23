#include "Fallout Shared/Pathfinding/TeleportDoorSearch.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarQueue.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"

TeleportDoorSearch::TeleportDoorSearch() : AStarSearch<ParentSpaceNode, TeleportLink>(), StartLocation(), DoorList() {}

TeleportDoorSearch::~TeleportDoorSearch() {}

bool TeleportDoorSearch::GetNodeConnections(
    const AStarNode<ParentSpaceNode, TeleportLink>* apNode,
    BSSimpleArray<AStarNode<ParentSpaceNode, TeleportLink>*, 1024>* apRetNodes
) {
    NiPoint3 kStartDelta;
    NiPoint3 kParentDelta;
    AStarNode<ParentSpaceNode, TeleportLink>* pNode;
    DoorList.Clear(false);
    if (apNode->tNodeData.bWorldSpace) {
        apNode->tNodeData.pWorldSpace->BuildTeleportDoorList(DoorList);
    }
    else {
        apNode->tNodeData.pInteriorCell->BuildTeleportDoorList(DoorList);
    }
    if (this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS ||
        this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_RADIO ||
        pLockData) {
        for (uint32_t i = 0; i < DoorList.iSize; ++i) {
            TESObjectREFR* pDoor = DoorList.GetAt(static_cast<int32_t>(i));
            if (!pDoor->GetDisabled()) {
                // Pointer-sized, as AStarNode::iKey: the space's whole address.
                uintptr_t iKey;
                NiPoint3 doorLocInThisSpace;
                if (pDoor->GetTeleport()->GetTeleportWorldSpace()) {
                    // The key is the destination space's address, as the source hashes it.
                    iKey = reinterpret_cast<uintptr_t>(pDoor->GetTeleport()->GetTeleportWorldSpace());
                }
                else {
                    iKey = reinterpret_cast<uintptr_t>(pDoor->GetTeleport()->GetTeleportCell());
                }
                Pathing::TELEPORT_DOOR_PATH_RESTRICTION eRestrictions = this->Restrictions;
                if (eRestrictions == Pathing::TELEPORT_DOOR_PATH_START_SPACE_ONLY) {
                    if (pDoor->GetTeleport()->GetTeleportWorldSpace() &&
                        pDoor->GetTeleport()->GetTeleportWorldSpace() != StartNode.pWorldSpace) {
                        continue;
                    }
                }
                else if (
                    eRestrictions == Pathing::TELEPORT_DOOR_PATH_INTERIORS_ONLY &&
                    pDoor->GetTeleport()->GetTeleportWorldSpace()
                ) {
                    continue;
                }
                bool bMustPick;
                DoorLock doorLock(pDoor);
                bool bCanOpenLock = doorLock.CanActorUnlockDoorLock(pLockData, pDoor, bMustPick);
                float fDoorAdditionalCost = 0.0f;
                if (!bCanOpenLock || bMustPick) {
                    Pathing::DOOR_LOCK_DISPOSITION eDisposition = this->DoorLockDisposition;
                    if (!eDisposition) {
                        continue;
                    }
                    if (eDisposition == Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE) {
                        fDoorAdditionalCost = 409600.0f;
                    }
                }
                if (static_cast<TESObjectDOOR*>(pDoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse() &&
                    this->DoorLockDisposition != Pathing::DOOR_LOCK_DISPOSITION_RADIO) {
                    fDoorAdditionalCost = 409600.0f;
                }
                doorLocInThisSpace = pDoor->GetLocationOnReference();
                NiPoint3* pDelta;
                if (apNode->pParentAStarNode) {
                    pDelta = ::new (&kParentDelta)
                        NiPoint3(apNode->ParentLink.DoorTeleportLocation - doorLocInThisSpace);
                }
                else {
                    pDelta = ::new (&kStartDelta) NiPoint3(StartLocation.GetLocation() - doorLocInThisSpace);
                }
                float fDistToDoor = pDelta->Length();
                if (fMaxDist != 0.0 && fMaxDist < apNode->fCost + fDistToDoor) {
                    continue;
                }
                float fCost = apNode->fCost + fDistToDoor + fDoorAdditionalCost;
                pDoor->GetTeleport()->GetTeleportCell();
                int32_t eNodeResult = GetOrCreateAStarNode(iKey, fCost, &pNode);
                if (eNodeResult) {
                    if (eNodeResult != 1) {
                        continue;
                    }
                }
                else {
                    if (pDoor->GetTeleport()->GetTeleportWorldSpace()) {
                        pNode->tNodeData.bWorldSpace = true;
                        pNode->tNodeData.pInteriorCell = nullptr;
                        pNode->tNodeData.pWorldSpace = pDoor->GetTeleport()->GetTeleportWorldSpace();
                    }
                    else {
                        pNode->tNodeData.bWorldSpace = false;
                        pNode->tNodeData.pWorldSpace = nullptr;
                        pNode->tNodeData.pInteriorCell = pDoor->GetTeleport()->GetTeleportCell();
                    }
                    pNode->fHeuristic = 1.0f;
                }
                pNode->ParentLink.pDoor = pDoor;
                pNode->ParentLink.DoorTeleportLocation = pDoor->GetTeleport()->position;
                apRetNodes->Add(pNode);
            }
        }
    }
    return true;
}

bool TeleportDoorSearch::IsGoalNode(const AStarNode<ParentSpaceNode, TeleportLink>* apNode) {
    if (Restrictions == Pathing::TELEPORT_DOOR_PATH_FIND_FIRST_WORLDSPACE) {
        TESWorldSpace* pWorldSpace = apNode->tNodeData.pWorldSpace;
        WorldSpaceMapData* pMapData = pWorldSpace ? pWorldSpace->GetWorldMapData() : nullptr;
        return apNode->tNodeData.bWorldSpace && pMapData && pMapData->iUsableWidth && pMapData->iUsableHeight;
    }

    return apNode->tNodeData.bWorldSpace == GoalNode.bWorldSpace &&
        apNode->tNodeData.pInteriorCell == GoalNode.pInteriorCell &&
        apNode->tNodeData.pWorldSpace == GoalNode.pWorldSpace;
}

bool TeleportDoorSearch::BuildNodePath(
    const PathingLocation& aStartLocation,
    const PathingLocation& aGoalLocation,
    TeleportPath* apRetPath,
    Pathing::TELEPORT_DOOR_PATH_RESTRICTION aRestrictions,
    float afMaxDist,
    ActorLockData* apLockData,
    Pathing::DOOR_LOCK_DISPOSITION aDisposition
) {
    // Pointer-sized, as AStarNode::iKey: the space's whole address.
    uintptr_t iStartKey;
    StartLocation = aStartLocation;
    if (aStartLocation.pWorldSpace) {
        StartNode.pWorldSpace = aStartLocation.pWorldSpace;
        StartNode.bWorldSpace = true;
        StartNode.pInteriorCell = nullptr;
        // The start node's key is its space's address, as the source hashes it.
        iStartKey = reinterpret_cast<uintptr_t>(StartNode.pWorldSpace);
    }
    else {
        if (!aStartLocation.GetCell()) {
            return false;
        }
        StartNode.pInteriorCell = aStartLocation.GetCell();
        StartNode.bWorldSpace = false;
        StartNode.pWorldSpace = nullptr;
        iStartKey = reinterpret_cast<uintptr_t>(StartNode.pInteriorCell);
    }
    Restrictions = aRestrictions;
    if (StartNode.pWorldSpace && aRestrictions == Pathing::TELEPORT_DOOR_PATH_INTERIORS_ONLY) {
        return false;
    }
    fMaxDist = afMaxDist;
    DoorLockDisposition = aDisposition;
    if (Restrictions != Pathing::TELEPORT_DOOR_PATH_FIND_FIRST_WORLDSPACE) {
        if (aGoalLocation.pWorldSpace) {
            GoalNode.pWorldSpace = aGoalLocation.pWorldSpace;
            GoalNode.bWorldSpace = true;
            GoalNode.pInteriorCell = nullptr;
        }
        else if (aGoalLocation.GetCell()) {
            GoalNode.pInteriorCell = aGoalLocation.GetCell();
            GoalNode.bWorldSpace = false;
            GoalNode.pWorldSpace = nullptr;
        }
    }
    pLockData = apLockData;
    apRetPath->Start = aStartLocation.GetLocation();
    apRetPath->End = aGoalLocation.GetLocation();
    AddStartNode(iStartKey, StartNode, 1.0f);
    return Search(&apRetPath->Spaces, &apRetPath->Doors, true);
}
