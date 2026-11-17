#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

// One space a path crosses: a worldspace or an interior cell.
struct ParentSpaceNode {
	bool bWorldSpace;
	uint8_t padding01[3];
	TESWorldSpace* pWorldSpace;
	TESObjectCELL* pInteriorCell;
};

// A door on a teleport path and where it leads.
struct TeleportLink {
	// The constructor deliberately leaves pDoor unset, as the game does.
	TeleportLink();

	TESObjectREFR* pDoor;
	NiPoint3 DoorTeleportLocation;
};

// The teleport-door search's node, under a name the layout macros can take.
using AStarNode_ParentSpaceNode_TeleportLink = AStarNode<ParentSpaceNode, TeleportLink>;

// The spaces and doors a path crosses between two points.
class TeleportPath {
public:
	TeleportPath();
	~TeleportPath();

	// Empties both arrays.
	void Clear();
	// The path's length: start to the first door, door to door, the last door's destination to the end; -1 without
	// spaces.
	float ComputeLength();

	BSSimpleArray<ParentSpaceNode> Spaces;
	BSSimpleArray<TeleportLink> Doors;
	NiPoint3 Start;
	NiPoint3 End;
};

using BSSimpleArray_TeleportLink_1024 = BSSimpleArray<TeleportLink, 1024>;
using BSSimpleArray_TeleportLink_1024 = BSSimpleArray<TeleportLink, 1024>;
