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
	VirtualPathingNode();
	VirtualPathingNode(const PathingLocation& aLocation);
	VirtualPathingNode(const NavMeshInfo* apInfo);
	VirtualPathingNode(const NiPoint3* apPoint, const NavMeshInfo* apNavmeshInfo);
	VirtualPathingNode(const NavMeshInfo* apNavMeshInfo, const NiPoint3* apLocation, TESObjectREFR* apDoor);
	// From Construct_006F4DA0.
	VirtualPathingNode(const PathingLocation* a1, TESObjectREFR* a2);
	~VirtualPathingNode();

	// Copies apSource into this node.
	VirtualPathingNode* Copy(VirtualPathingNode* apSource);
	// Sets bit 1 of iFlags once the location resolves to a navmesh.
	void UpdateNode();
	// Takes aLocation; iFlags becomes 2, plus 1 when it resolves to a navmesh.
	void SetLocation(PathingLocation& arLocation);
	bool HasNavMesh() const;
	bool HasActualLocation() const;
	// Whether bit 4 of iFlags is set, the same test as PathingNode::IsTransitionNode.
	bool IsTransitionNode();
	PathingLocation& GetLocation();
	void Save(BGSSaveGameBuffer* a2);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	// Deleting frees through ourFree.
	static void operator delete(void* apMemory);

	uint32_t iFlags;
	PathingLocation Location;
	TESObjectREFR* pDoorRef;
};
