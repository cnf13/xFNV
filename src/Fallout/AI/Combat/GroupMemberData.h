#pragma once

#include <cstdint>

class Actor;

// One member of a combat group: the actor and its index in the group.
class GroupMemberData {
public:
    // The default constructor is empty and leaves the members uninitialised; BSSimpleArray runs it
    // when constructing items.
    GroupMemberData() {}
    GroupMemberData(Actor* apActor, uint32_t aiIndex);

    Actor* pActor;
    uint32_t iIndex;
};
