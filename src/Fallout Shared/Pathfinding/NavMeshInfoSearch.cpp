#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TeleportData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>
#include <limits>

NavMeshInfoSearch::NavMeshInfoSearch() : pStartMesh(nullptr), pGoalMesh(nullptr), pActorLockData(nullptr) {}

NavMeshInfoSearch::~NavMeshInfoSearch() {}

// The search keys a NavMeshInfo's node by the info's address, using the pointer as the untyped integer key.
bool NavMeshInfoSearch::GetNodeConnections(
    const AStarNode<const NavMeshInfo*, TESObjectREFR*>* apNode,
    BSSimpleArray<AStarNode<const NavMeshInfo*, TESObjectREFR*>*, 1024>* apRetNodes
) {
    AStarNode<const NavMeshInfo*, TESObjectREFR*>* pnode;
    float fparentCost = apNode->fCost;
    const NavMeshInfo* pparentInfo = apNode->tNodeData;
    float fcostFactor = 1.0 - pparentInfo->fPreferredPercent * 100.0;
    BSClamp(fcostFactor, 0.1f, 1.0f);
    uint32_t isize = pparentInfo->AdjacentNavMeshes.iSize;
    for (uint32_t i = 0; i < isize; ++i) {
        const NavMeshInfo* pportal = pparentInfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
        if (!pportal) {
            continue;
        }
        const NiPoint3* pbeginLocation = pparentInfo == pStartMesh ? &StartLocation : &pparentInfo->ApproxLocation;
        NiPoint3 begin = *pbeginLocation;
        const NiPoint3* pendLocation = pportal == pGoalMesh ? &GoalLocation : &pportal->ApproxLocation;
        NiPoint3 end = *pendLocation;
        double dlength = (end - begin).Length();
        float fcost = dlength * fcostFactor * 3.0 + fparentCost;
        int32_t eNodeResult = GetOrCreateAStarNode(reinterpret_cast<uintptr_t>(pportal), fcost, &pnode);
        if (!eNodeResult) {
            pnode->tNodeData = pportal;
            if (bSameSpace) {
                double dgoalDistance = (GoalLocation - pportal->ApproxLocation).Length();
                pnode->fHeuristic = dgoalDistance * 0.10000000149011612;
            }
            else {
                pnode->fHeuristic = 1.0f;
            }
        }
        else if (eNodeResult != 1) {
            continue;
        }
        pnode->ParentLink = 0;
        apRetNodes->Add(pnode);
    }
    uint32_t ipreferredSize = pparentInfo->PreferredAdjacentNavMeshes.iSize;
    for (uint32_t i = 0; i < ipreferredSize; ++i) {
        const NavMeshInfo* pportal = pparentInfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
        if (!pportal) {
            continue;
        }
        const NiPoint3* pbeginLocation = pparentInfo == pStartMesh ? &StartLocation : &pparentInfo->ApproxLocation;
        NiPoint3 begin = *pbeginLocation;
        const NiPoint3* pendLocation = pportal == pGoalMesh ? &GoalLocation : &pportal->ApproxLocation;
        NiPoint3 end = *pendLocation;
        double dlength = (end - begin).Length();
        float fcost = dlength * fcostFactor + fparentCost;
        int32_t eNodeResult = GetOrCreateAStarNode(reinterpret_cast<uintptr_t>(pportal), fcost, &pnode);
        if (!eNodeResult) {
            pnode->tNodeData = pportal;
            if (bSameSpace) {
                double dgoalDistance = (GoalLocation - pportal->ApproxLocation).Length();
                pnode->fHeuristic = dgoalDistance * 0.10000000149011612;
            }
            else {
                pnode->fHeuristic = 1.0f;
            }
        }
        else if (eNodeResult != 1) {
            continue;
        }
        pnode->ParentLink = 0;
        apRetNodes->Add(pnode);
    }
    if (pActorLockData || this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS) {
        isize = pparentInfo->ConnectedDoors.iSize;
        for (uint32_t i = 0; i < isize; ++i) {
            TESObjectREFR* pdoor = pparentInfo->ConnectedDoors.GetAt(static_cast<int32_t>(i));
            if (pdoor && !pdoor->GetDisabled()) {
                bool bmustPick;
                TempDoorLock.SetFromDoorRef(pdoor);
                bool bcanOpenLock = TempDoorLock.CanActorUnlockDoorLock(pActorLockData, pdoor, bmustPick);
                float fdoorAdditionalCost = 0.0f;
                if (!bcanOpenLock || bmustPick) {
                    Pathing::DOOR_LOCK_DISPOSITION edoorLockDisposition = this->DoorLockDisposition;
                    if (edoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_NORMAL) {
                        continue;
                    }
                    if (edoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE) {
                        fdoorAdditionalCost = 409600.0f;
                    }
                }
                if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                    fdoorAdditionalCost = 409600.0f;
                }
                if ((this->DoorLockDisposition != Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE ||
                     !pdoor->GetIsHiddenDoor()) &&
                    pdoor->GetTeleport()) {
                    TESObjectREFR* potherDoor = pdoor->GetTeleport()->pLinkedDoor;
                    if (potherDoor) {
                        ExtraNavMeshPortal* pextraPortal = potherDoor->m_Extra.GetNavMeshPortal();
                        if (pextraPortal) {
                            NavMeshInfo* pinfo = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(
                                pextraPortal->Portal.NavMeshID
                            );
                            if (pinfo && !pinfo->GetDisabled()) {
                                const NiPoint3* pbeginLocation = pparentInfo == pStartMesh
                                    ? &StartLocation
                                    : &pparentInfo->ApproxLocation;
                                NiPoint3 begin = *pbeginLocation;
                                NiPoint3* pendLocation = pinfo == pGoalMesh ? &GoalLocation : &pinfo->ApproxLocation;
                                NiPoint3 end = *pendLocation;
                                DoorTeleportData* potherTeleport = potherDoor->GetTeleport();
                                double dstartLength = (potherTeleport->position - begin).Length();
                                float fcost = (end - pdoor->GetTeleport()->position).Length() + dstartLength;
                                fcost = fcost * fcostFactor;
                                fcost = fcost * 3.0;
                                fcost = fcost + fparentCost;
                                float ftotalCost = fcost + fdoorAdditionalCost;
                                int32_t eNodeResult = GetOrCreateAStarNode(
                                    reinterpret_cast<uintptr_t>(pinfo),
                                    ftotalCost,
                                    &pnode
                                );
                                if (eNodeResult) {
                                    if (eNodeResult != 1) {
                                        continue;
                                    }
                                }
                                else {
                                    pnode->tNodeData = pinfo;
                                    if (bSameSpace) {
                                        double dheuristic = apNode->fHeuristic;
                                        pnode->fHeuristic = dheuristic + dheuristic;
                                    }
                                    else {
                                        pnode->fHeuristic = 1.0f;
                                    }
                                }
                                pnode->ParentLink = pdoor;
                                apRetNodes->Add(pnode);
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool NavMeshInfoSearch::IsGoalNode(const AStarNode<const NavMeshInfo*, TESObjectREFR*>* apNode) {
    return apNode->tNodeData == pGoalMesh;
}

bool NavMeshInfoSearch::Pathfind(
    ActorLockData* apActorLockData,
    const NiPoint3& aStartLocation,
    const NavMeshInfo* apStartMesh,
    const NiPoint3& aGoalLocation,
    const NavMeshInfo* apGoalMesh,
    BSSimpleArray<const NavMeshInfo*, 1024>* apReturnNodes,
    BSSimpleArray<TESObjectREFR*, 1024>* apReturnLinks,
    Pathing::DOOR_LOCK_DISPOSITION aeDoorLockDisposition
) {
    StartLocation = aStartLocation;
    pStartMesh = apStartMesh;
    GoalLocation = aGoalLocation;
    pGoalMesh = apGoalMesh;
    pActorLockData = apActorLockData;
    DoorLockDisposition = aeDoorLockDisposition;
    bSameSpace = false;
    if (apStartMesh->GetWorldSpace()) {
        TESWorldSpace* pworldSpace = apStartMesh->GetWorldSpace();
        if (pworldSpace == apGoalMesh->GetWorldSpace()) {
            bSameSpace = true;
        }
    }
    float fstartheuristic = std::numeric_limits<float>::max();
    if (bSameSpace) {
        fstartheuristic = (GoalLocation - pStartMesh->ApproxLocation).Length() * 0.10000000149011612;
    }
    else {
        fstartheuristic = 1.0f;
    }
    // The search keys a NavMeshInfo's node by the info's address. The key is pointer-sized (AStarNode::iKey), as
    // GetNodeConnections passes it.
    AddStartNode(reinterpret_cast<uintptr_t>(apStartMesh), apStartMesh, fstartheuristic);
    return Search(apReturnNodes, apReturnLinks, true);
}

void NavMeshInfoSearch::operator delete(void* apMemory) {
    ourFree(apMemory);
}
