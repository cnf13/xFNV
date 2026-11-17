#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Reconstruction.hpp"

struct hkpAgentEntry;

// What an agent's processCollision writes: hkpProcessCollisionData and the potential contacts (points an agent only
// proposes, until the contact manager commits them).
class alignas(16) hkpProcessCollisionOutput : public hkpProcessCollisionData {
public:
	// One potential contact: the point, the agent entry that owns it and that agent's data.
	struct ContactRef {
		hkpProcessCdPoint* m_contactPoint;
		hkpAgentEntry* m_agentEntry;
		void* m_agentData;
	};

	// The potential contacts of one process step and the representatives chosen among them (for welding).
	struct PotentialInfo {
		ContactRef* m_firstFreePotentialContact;
		hkpProcessCdPoint** m_firstFreeRepresentativeContact;
		hkpProcessCdPoint* m_representativeContacts[256];
		ContactRef m_potentialContacts[256];
	};

	hkPadSpu<PotentialInfo*> m_potentialContacts;
};
