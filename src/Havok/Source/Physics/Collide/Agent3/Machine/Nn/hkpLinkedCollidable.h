#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"

#include <stdint.h>

class hkMotionState;
class hkpAgentNnEntry;
class hkpShape;

// A collidable the agent machine keeps the pairs it is in touch with for.
class hkpLinkedCollidable : public hkpCollidable {
public:
	// One pair of the collidable's agent list.
	struct CollisionEntry {
		hkpAgentNnEntry* m_agentEntry;
		hkpLinkedCollidable* m_partner;
	};

	// The reflection's member table.
	using MembersArray = const hkClassMember[1];

	// The constructors run hkpCollidable's, then leave an empty collision entry array.
	hkpLinkedCollidable(const hkpShape* shape, const hkMotionState* ms, int32_t type = 0)
		: hkpCollidable(shape, ms, type) {}

	hkpLinkedCollidable(hkFinishLoadedObjectFlag flag) : hkpCollidable(flag) {}

	// Stores aiFlags into the broad phase handle's collision filter info.
	// The Havok SDK's collision filter info is an hkUint32 (hkpTypedBroadPhaseHandle::m_collisionFilterInfo).
	void setCollisionInfoFlags(uint32_t aiFlags);

	// Makes entries a copy of the agent list, sorted by partner when the owner is a fixed entity.
	void getCollisionEntriesSorted(hkArray<CollisionEntry>& entries) const;
	// The agent list itself, without the check the base's accessor makes.
	hkArray<CollisionEntry>& getCollisionEntriesDeterministicUnchecked();
	// Sorts the agent list by partner and stores each entry's new index in its agent entry.
	void sortEntries();
	// Returns the agent list itself.
	hkArray<CollisionEntry>& getCollisionEntriesNonDeterministic() { return m_collisionEntries; }

	static const hkClass& staticClass();

	hkArray<CollisionEntry> m_collisionEntries;

	static MembersArray Members;
};

// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
bool cmpLessCollisionEntries(const hkpLinkedCollidable::CollisionEntry& entry0,
	const hkpLinkedCollidable::CollisionEntry& entry1);

extern hkClass hkpLinkedCollidableClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinkedCollidableTypeInfo;

namespace hkAlgorithm {} // namespace hkAlgorithm
