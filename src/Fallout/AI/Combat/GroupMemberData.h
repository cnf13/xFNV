#pragma once

#include "Reconstruction.hpp"

class Actor;

// One member of a combat group: the actor and its index in the group.
class GroupMemberData {
public:
	// The exe's default constructor is empty (0x6815C0, run by BSSimpleArray's _ConstructItems 0x6DC3A0).
	GroupMemberData() {}
	GroupMemberData(Actor* apActor, uint32_t aiIndex);

	Actor* pActor;
	uint32_t iIndex;
};
