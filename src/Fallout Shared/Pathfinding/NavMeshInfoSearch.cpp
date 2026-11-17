#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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

NavMeshInfoSearch::NavMeshInfoSearch() : pStartMesh(nullptr), pGoalMesh(nullptr), pActorLockData(nullptr) {}

NavMeshInfoSearch::~NavMeshInfoSearch() {}

// The search keys a NavMeshInfo's node by the info's address, using the pointer as the untyped integer key.
bool NavMeshInfoSearch::GetNodeConnections(
    const AStarNode<const NavMeshInfo*, TESObjectREFR*>* apNode,
    BSSimpleArray<AStarNode<const NavMeshInfo*, TESObjectREFR*>*, 1024>* apRetNodes
) {
    AStarNode<const NavMeshInfo*, TESObjectREFR*>* pnode_3;
    AStarNode<const NavMeshInfo*, TESObjectREFR*>* pnode_2;
    AStarNode<const NavMeshInfo*, TESObjectREFR*>* pnode;
    float fcostFactor;
    float fparentCost = apNode->fCost;
    const NavMeshInfo* pparentInfo = apNode->tNodeData;
    fcostFactor = 1.0 - pparentInfo->fPreferredPercent * 100.0;
    BSClamp(fcostFactor, 0.1f, 1.0f);
    uint32_t isize = pparentInfo->AdjacentNavMeshes.iSize;
    for (uint32_t i = 0; i < isize; ++i) {
        const NiPoint3* p_GoalLocation;
        const NiPoint3* p_StartLocation;
        const NavMeshInfo* pportal = pparentInfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
        if (!pportal) {
            continue;
        }
        if (pparentInfo == pStartMesh) {
            p_StartLocation = &StartLocation;
        }
        else {
            p_StartLocation = &pparentInfo->ApproxLocation;
        }
        NiPoint3 begin = *p_StartLocation;
        if (pportal == pGoalMesh) {
            p_GoalLocation = &GoalLocation;
        }
        else {
            p_GoalLocation = &pportal->ApproxLocation;
        }
        NiPoint3 end = *p_GoalLocation;
        double v4 = (end - begin).Length();
        float fcost = v4 * fcostFactor * 3.0 + fparentCost;
        int32_t eNodeResult = GetOrCreateAStarNode(reinterpret_cast<uintptr_t>(pportal), fcost, &pnode);
        if (!eNodeResult) {
            pnode->tNodeData = pportal;
            if (bSameSpace) {
                double v6 = (GoalLocation - pportal->ApproxLocation).Length();
                pnode->fHeuristic = v6 * 0.10000000149011612;
            }
            else {
                pnode->fHeuristic = 1.0f;
            }
        LABEL_17:;
            pnode->ParentLink = 0;
            apRetNodes->Add(pnode);
            continue;
        }
        if (eNodeResult == 1) {
            goto LABEL_17;
        }
    }
    uint32_t ipreferredSize = pparentInfo->PreferredAdjacentNavMeshes.iSize;
    for (uint32_t i_2 = 0; i_2 < ipreferredSize; ++i_2) {
        const NiPoint3* v25;
        const NiPoint3* p_ApproxLocation;
        const NavMeshInfo* pportal_2 = pparentInfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2));
        if (!pportal_2) {
            continue;
        }
        if (pparentInfo == pStartMesh) {
            p_ApproxLocation = &StartLocation;
        }
        else {
            p_ApproxLocation = &pparentInfo->ApproxLocation;
        }
        NiPoint3 begin_2 = *p_ApproxLocation;
        if (pportal_2 == pGoalMesh) {
            v25 = &GoalLocation;
        }
        else {
            v25 = &pportal_2->ApproxLocation;
        }
        NiPoint3 end_2 = *v25;
        double v8 = (end_2 - begin_2).Length();
        float fcost_2 = v8 * fcostFactor + fparentCost;
        int32_t eNodeResult_2 = GetOrCreateAStarNode(reinterpret_cast<uintptr_t>(pportal_2), fcost_2, &pnode_2);
        if (!eNodeResult_2) {
            pnode_2->tNodeData = pportal_2;
            if (bSameSpace) {
                double v10 = (GoalLocation - pportal_2->ApproxLocation).Length();
                pnode_2->fHeuristic = v10 * 0.10000000149011612;
            }
            else {
                pnode_2->fHeuristic = 1.0f;
            }
        LABEL_34:;
            pnode_2->ParentLink = 0;
            apRetNodes->Add(pnode_2);
            continue;
        }
        if (eNodeResult_2 == 1) {
            goto LABEL_34;
        }
    }
    if (pActorLockData || this->DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS) {
        isize = pparentInfo->ConnectedDoors.iSize;
        for (uint32_t i_3 = 0;; ++i_3) {
            if (i_3 >= isize) {
                return true;
            }
            TESObjectREFR* pdoor = pparentInfo->ConnectedDoors.GetAt(static_cast<int32_t>(i_3));
            if (pdoor && !pdoor->GetDisabled()) {
                bool bmustPick;
                TempDoorLock.SetFromDoorRef(pdoor);
                bool bcanOpenLock = TempDoorLock.CanActorUnlockDoorLock(pActorLockData, pdoor, bmustPick);
                float fdoorAdditionalCost = 0.0f;
                if (!bcanOpenLock || bmustPick) {
                    Pathing::DOOR_LOCK_DISPOSITION DoorLockDisposition = this->DoorLockDisposition;
                    if (DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_NORMAL) {
                        continue;
                    }
                    if (DoorLockDisposition == Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE) {
                        fdoorAdditionalCost = 409600.0f;
                    }
                }
                if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
                    fdoorAdditionalCost = 409600.0f;
                }
                if (this->DoorLockDisposition != Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE ||
                    !pdoor->GetIsHiddenDoor()) {
                    if (pdoor->GetTeleport()) {
                        TESObjectREFR* potherDoor = pdoor->GetTeleport()->pLinkedDoor;
                        if (potherDoor) {
                            ExtraNavMeshPortal* pextraPortal = (&potherDoor->m_Extra)->GetNavMeshPortal();
                            if (pextraPortal) {
                                NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
                                NavMeshInfo* pinfo = NavMeshInfoMap_->GetNavMeshInfo(pextraPortal->Portal.NavMeshID);
                                if (pinfo) {
                                    if (!pinfo->GetDisabled()) {
                                        NiPoint3* v21;
                                        const NiPoint3* v22;
                                        if (pparentInfo == pStartMesh) {
                                            v22 = &StartLocation;
                                        }
                                        else {
                                            v22 = &pparentInfo->ApproxLocation;
                                        }
                                        NiPoint3 begin_3 = *v22;
                                        if (pinfo == pGoalMesh) {
                                            v21 = &GoalLocation;
                                        }
                                        else {
                                            v21 = &pinfo->ApproxLocation;
                                        }
                                        NiPoint3 end_3 = *v21;
                                        DoorTeleportData* Teleport = potherDoor->GetTeleport();
                                        double v20 = (Teleport->position - begin_3).Length();
                                        DoorTeleportData* v14 = pdoor->GetTeleport();
                                        float fcost_3 = (end_3 - v14->position).Length() + v20;
                                        fcost_3 = fcost_3 * fcostFactor;
                                        fcost_3 = fcost_3 * 3.0;
                                        fcost_3 = fcost_3 + fparentCost;
                                        float fcost_4 = fcost_3 + fdoorAdditionalCost;
                                        int32_t v19 =
                                            GetOrCreateAStarNode(reinterpret_cast<uintptr_t>(pinfo), fcost_4, &pnode_3);
                                        if (v19) {
                                            if (v19 != 1) {
                                                continue;
                                            }
                                        }
                                        else {
                                            pnode_3->tNodeData = pinfo;
                                            if (bSameSpace) {
                                                double Value = apNode->fHeuristic;
                                                pnode_3->fHeuristic = Value + Value;
                                            }
                                            else {
                                                pnode_3->fHeuristic = 1.0f;
                                            }
                                        }
                                        pnode_3->ParentLink = pdoor;
                                        apRetNodes->Add(pnode_3);
                                    }
                                }
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
        TESWorldSpace* WorldSpace = apStartMesh->GetWorldSpace();
        if (WorldSpace == apGoalMesh->GetWorldSpace()) {
            bSameSpace = true;
        }
    }
    float fstartheuristic = 3.4028235e+38f;
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
