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
    NiPoint3 v25;
    NiPoint3 result;
    DoorLock doorLock;
    AStarNode<ParentSpaceNode, TeleportLink>* pnode;
    DoorList.Clear(false);
    if (apNode->tNodeData.bWorldSpace) {
        apNode->tNodeData.pWorldSpace->BuildTeleportDoorList(DoorList);
    }
    else {
        apNode->tNodeData.pInteriorCell->BuildTeleportDoorList(DoorList);
    }
    if (this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS ||
        (this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_RADIO || pLockData)) {
        for (uint32_t i = 0;; ++i) {
            uint32_t FormFlags = DoorList.iSize;
            if (i >= FormFlags) {
                return true;
            }
            TESObjectREFR* pdoor = DoorList.GetAt(static_cast<int32_t>(i));
            if (!pdoor->GetDisabled()) {
                // Pointer-sized, as AStarNode::iKey: the space's whole address on x64.
                uintptr_t ikey;
                NiPoint3 doorLocInThisSpace;
                DoorTeleportData* Teleport = pdoor->GetTeleport();
                if (Teleport->GetTeleportWorldSpace()) {
                    DoorTeleportData* v5 = pdoor->GetTeleport();
                    // The key is the destination space's address, as the source hashes it.
                    ikey = reinterpret_cast<uintptr_t>(v5->GetTeleportWorldSpace());
                }
                else {
                    DoorTeleportData* v6 = pdoor->GetTeleport();
                    ikey = reinterpret_cast<uintptr_t>(v6->GetTeleportCell());
                }
                uintptr_t ikey_2 = ikey;
                Pathing::TELEPORT_DOOR_PATH_RESTRICTION Restrictions = this->Restrictions;
                if (Restrictions == Pathing::TELEPORT_DOOR_PATH_START_SPACE_ONLY) {
                    DoorTeleportData* v8;
                    DoorTeleportData* v9;
                    v8 = pdoor->GetTeleport();
                    if (!v8->GetTeleportWorldSpace() ||
                        (v9 = pdoor->GetTeleport(), v9->GetTeleportWorldSpace() == StartNode.pWorldSpace)) {
                        bool bmustPick;
                    LABEL_20:;
                        doorLock.~DoorLock();
                        ::new (&doorLock) DoorLock(pdoor);
                        bool bcanOpenLock = doorLock.CanActorUnlockDoorLock(pLockData, pdoor, bmustPick);
                        float fdoorAdditionalCost = 0.0f;
                        if (bcanOpenLock && !bmustPick) {
                            NiPoint3* v10;
                        LABEL_26:;
                            if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())
                                    ->TESObjectDOOR::IsMinimalUse() &&
                                this->DoorLockDisposition != Pathing::DOOR_LOCK_DISPOSITION_RADIO) {
                                fdoorAdditionalCost = 409600.0f;
                            }
                            doorLocInThisSpace = pdoor->GetLocationOnReference();
                            if (apNode->pParentAStarNode) {
                                v10 = ::new (&result)
                                    NiPoint3(apNode->ParentLink.DoorTeleportLocation - doorLocInThisSpace);
                            }
                            else {
                                v10 = ::new (&v25) NiPoint3(StartLocation.GetLocation() - doorLocInThisSpace);
                            }
                            float fdistToDoor = v10->Length();
                            if (fMaxDist != 0.0 && fMaxDist < apNode->fCost + fdistToDoor) {
                                goto LABEL_34;
                            }
                            float fcost = apNode->fCost + fdistToDoor + fdoorAdditionalCost;
                            DoorTeleportData* v12 = pdoor->GetTeleport();
                            TESObjectCELL* ptelCell = v12->GetTeleportCell();
                            int32_t eNodeResult = GetOrCreateAStarNode(ikey_2, fcost, &pnode);
                            if (eNodeResult) {
                                if (eNodeResult != 1) {
                                    goto LABEL_34;
                                }
                            }
                            else {
                                DoorTeleportData* v13 = pdoor->GetTeleport();
                                if (v13->GetTeleportWorldSpace()) {
                                    pnode->tNodeData.bWorldSpace = true;
                                    pnode->tNodeData.pInteriorCell = nullptr;
                                    DoorTeleportData* v14 = pdoor->GetTeleport();
                                    TESWorldSpace* ikey_3 = v14->GetTeleportWorldSpace();
                                    pnode->tNodeData.pWorldSpace = ikey_3;
                                }
                                else {
                                    pnode->tNodeData.bWorldSpace = false;
                                    pnode->tNodeData.pWorldSpace = nullptr;
                                    DoorTeleportData* v16 = pdoor->GetTeleport();
                                    TESObjectCELL* TeleportCell = v16->GetTeleportCell();
                                    pnode->tNodeData.pInteriorCell = TeleportCell;
                                }
                                pnode->fHeuristic = 1.0f;
                            }
                            pnode->ParentLink.pDoor = pdoor;
                            NiPoint3* TeleportPosition = &pdoor->GetTeleport()->position;
                            pnode->ParentLink.DoorTeleportLocation = *TeleportPosition;
                            apRetNodes->Add(pnode);
                        }
                        else {
                            Pathing::DOOR_LOCK_DISPOSITION DoorLockDisposition = this->DoorLockDisposition;
                            if (DoorLockDisposition) {
                                if (DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE) {
                                    fdoorAdditionalCost = 409600.0f;
                                }
                                goto LABEL_26;
                            }
                        }
                    LABEL_34:;
                        continue;
                    }
                }
                else {
                    if (Restrictions != Pathing::TELEPORT_DOOR_PATH_INTERIORS_ONLY) {
                        goto LABEL_20;
                    }
                    DoorTeleportData* v7 = pdoor->GetTeleport();
                    if (!v7->GetTeleportWorldSpace()) {
                        goto LABEL_20;
                    }
                }
            }
        }
    }
    return true;
}

bool TeleportDoorSearch::IsGoalNode(const AStarNode<ParentSpaceNode, TeleportLink>* apNode) {
    WorldSpaceMapData* mapData;
    TESWorldSpace* pWorldSpace;
    if (((int32_t)Restrictions) == 3) {
        pWorldSpace = apNode->tNodeData.pWorldSpace;
        if (pWorldSpace) {
            mapData = pWorldSpace->GetWorldMapData();
        }
        else {
            mapData = nullptr;
        }
        return (bool)((uint8_t)(apNode->tNodeData.bWorldSpace &&
                                (mapData && (mapData->iUsableWidth && mapData->iUsableHeight))));
    }

    return (bool)((uint8_t)((((uint8_t)apNode->tNodeData.bWorldSpace) == ((uint8_t)GoalNode.bWorldSpace)) &&
                            ((apNode->tNodeData.pInteriorCell == GoalNode.pInteriorCell) &&
                             (apNode->tNodeData.pWorldSpace == GoalNode.pWorldSpace))));
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
    // Pointer-sized, as AStarNode::iKey: the space's whole address on x64.
    uintptr_t istartKey;
    StartLocation = aStartLocation;
    if (aStartLocation.pWorldSpace) {
        StartNode.pWorldSpace = aStartLocation.pWorldSpace;
        StartNode.bWorldSpace = true;
        StartNode.pInteriorCell = nullptr;
        // The start node's key is its space's address, as the source hashes it.
        istartKey = reinterpret_cast<uintptr_t>(StartNode.pWorldSpace);
    }
    else {
        if (!aStartLocation.GetCell()) {
            return false;
        }
        StartNode.pInteriorCell = aStartLocation.GetCell();
        StartNode.bWorldSpace = false;
        StartNode.pWorldSpace = nullptr;
        istartKey = reinterpret_cast<uintptr_t>(StartNode.pInteriorCell);
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
        else {
            if (aGoalLocation.GetCell()) {
                GoalNode.pInteriorCell = aGoalLocation.GetCell();
                GoalNode.bWorldSpace = false;
                GoalNode.pWorldSpace = nullptr;
            }
        }
    }
    pLockData = apLockData;
    apRetPath->Start = aStartLocation.GetLocation();
    apRetPath->End = aGoalLocation.GetLocation();
    AddStartNode(istartKey, StartNode, 1.0f);
    return Search(&apRetPath->Spaces, &apRetPath->Doors, true);
}
