#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveGameBuffer;
class NavMeshInfo;
class TESObjectREFR;

// A node of a path that has no navmesh triangle of its own: a location, and the teleport door that leads there.
class VirtualPathingNode {
public:
    enum : uint32_t {
        kFlagHasNavMesh = 0x01,
        kFlagHasActualLocation = 0x02,
        kFlagTransition = 0x04,
    };

    VirtualPathingNode();
    VirtualPathingNode(const PathingLocation& aLocation);
    VirtualPathingNode(const NavMeshInfo* apInfo);
    VirtualPathingNode(const NiPoint3* apPoint, const NavMeshInfo* apNavmeshInfo);
    VirtualPathingNode(const NavMeshInfo* apNavMeshInfo, const NiPoint3* apLocation, TESObjectREFR* apDoor);
    VirtualPathingNode(const PathingLocation* apLocation, TESObjectREFR* apDoor);
    ~VirtualPathingNode();

    // Copies apSource into this node.
    VirtualPathingNode* Copy(VirtualPathingNode* apSource);
    // Sets kFlagHasNavMesh once the location resolves to a navmesh.
    void UpdateNode();
    // Takes aLocation; iFlags becomes kFlagHasActualLocation, plus kFlagHasNavMesh when it resolves to a navmesh.
    void SetLocation(PathingLocation& arLocation);
    bool HasNavMesh() const;
    bool HasActualLocation() const;
    // Whether kFlagTransition is set, the same test as PathingNode::IsTransitionNode.
    bool IsTransitionNode();
    PathingLocation& GetLocation();
    void Save(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    // Deleting frees through ourFree.
    static void operator delete(void* apMemory);

    uint32_t iFlags;
    PathingLocation Location;
    TESObjectREFR* pDoorRef;
};
