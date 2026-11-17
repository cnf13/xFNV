#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class ActorLockData;
class NavMeshInfo;

// The search base of NavMeshInfoSearch; its destructor body is defined in NavMeshInfoSearch.cpp.
using AStarSearch_NavMeshInfo = AStarSearch<const NavMeshInfo*, TESObjectREFR*>;

// The search's node array and the scratch array of the same items; their constructor, destructor, initial setup and
// clear are defined in NavMeshInfoSearch.cpp.
using NavMeshInfoNodeArray = BSSimpleArray<AStarNode<const NavMeshInfo*, TESObjectREFR*>*, 1024>;
using NavMeshInfoNodeScrapArray = BSScrapArray<AStarNode<const NavMeshInfo*, TESObjectREFR*>*, 1024>;

// The A* search over navmesh infos (whole navmeshes) joined by the doors between them, used for the high-level path
// from a start to a goal location.
class NavMeshInfoSearch : public AStarSearch<const NavMeshInfo*, TESObjectREFR*> {
public:
    NavMeshInfoSearch();
    ~NavMeshInfoSearch() override;

    bool GetNodeConnections(
        const AStarNode<const NavMeshInfo*, TESObjectREFR*>* apNode,
        BSSimpleArray<AStarNode<const NavMeshInfo*, TESObjectREFR*>*, 1024>* apRetNodes
    ) override;
    bool IsGoalNode(const AStarNode<const NavMeshInfo*, TESObjectREFR*>* apNode) override;

    // Searches the navmesh infos from apStartMesh at aStartLocation to apGoalMesh at
    // aGoalLocation; the infos on the way go to apReturnNodes and the doors between them to apReturnLinks.
    bool Pathfind(
        ActorLockData* apActorLockData,
        const NiPoint3& aStartLocation,
        const NavMeshInfo* apStartMesh,
        const NiPoint3& aGoalLocation,
        const NavMeshInfo* apGoalMesh,
        BSSimpleArray<const NavMeshInfo*, 1024>* apReturnNodes,
        BSSimpleArray<TESObjectREFR*, 1024>* apReturnLinks,
        Pathing::DOOR_LOCK_DISPOSITION aeDoorLockDisposition
    );

    static void operator delete(void* apMemory);

    NiPoint3 StartLocation;
    const NavMeshInfo* pStartMesh;
    NiPoint3 GoalLocation;
    const NavMeshInfo* pGoalMesh;
    ActorLockData* pActorLockData;
    Pathing::DOOR_LOCK_DISPOSITION DoorLockDisposition;
    bool bSameSpace;
    uint8_t padding2081[3];
    DoorLock TempDoorLock;
};
