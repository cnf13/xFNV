#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Reconstruction.hpp"

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
	// Not named in the PDB or IDA; name chosen here. Whether bit 4 of iFlags is set: the exe calls
	// PathingNode::IsTransitionNode's body (thiscall, 0x6CA0C0) on virtual nodes, whose iFlags is at the same offset.
	bool IsTransitionNode();
	// Location; the exe's body is the shared "this + 4" getter (thiscall, 0x717E50).
	PathingLocation& GetLocation();
	// Inferred type: a2: BGSSaveGameBuffer* from 0x6e8bd5 (parameter 0 of PathingSolution::Save)
	void Save(BGSSaveGameBuffer* a2);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	uint32_t iFlags;
	PathingLocation Location;
	TESObjectREFR* pDoorRef;
};
