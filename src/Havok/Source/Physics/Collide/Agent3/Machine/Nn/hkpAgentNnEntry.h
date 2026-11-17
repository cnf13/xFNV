#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"

class hkpContactMgr;
class hkpLinkedCollidable;

// The agent entry of a pair of linked collidables: the contact manager and the two collidables.
class hkpAgentNnEntry : public hkpAgentEntry {
public:
	uint16_t m_agentIndexOnCollidable[2];
	hkpContactMgr* m_contactMgr;
	uint8_t m_collisionQualityIndex;
	uint8_t m_forceCollideOntoPpu;
	uint8_t m_padding[2];
	hkpLinkedCollidable* m_collidable[2];
};

// Points both collidables' agent list entries for the entry back at it and returns it.
hkpAgentNnEntry* updateCollidableEntryPointers(hkpAgentNnEntry* r_eax);
