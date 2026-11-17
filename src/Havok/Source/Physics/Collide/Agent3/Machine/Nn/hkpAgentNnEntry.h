#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Reconstruction.hpp"

class hkpContactMgr;
class hkpLinkedCollidable;

// The agent entry of a pair of linked collidables: the contact manager and the two collidables.
// The PDB places its functions in hkpWorld.cpp.
class hkpAgentNnEntry : public hkpAgentEntry {
public:
	uint16_t m_agentIndexOnCollidable[2];
	hkpContactMgr* m_contactMgr;
	uint8_t m_collisionQualityIndex;
	uint8_t m_forceCollideOntoPpu;
	uint8_t m_padding[2];
	hkpLinkedCollidable* m_collidable[2];
};

// Not named in the PDB or IDA; name chosen here. Points both collidables' agent list entries for the entry back at
// it and returns it (usercall, entry in eax, ret 0).
hkpAgentNnEntry* updateCollidableEntryPointers(hkpAgentNnEntry* r_eax);
