#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"

// Havok SDK signature (hkpCollide.lib, tied by reloc).
const void* getVtablehkpCollisionFilterList();
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpCollisionFilterList(void* p, int32_t finishing);

// A collision filter that asks a list of other filters: a pair collides when every filter in the list says so. It
// overrides the destructor and the five isCollisionEnabled interfaces.
class hkpCollisionFilterList : public hkpCollisionFilter {
public:
	// The reflection's member table for hkpCollisionFilterList.
	using MembersArray = const hkClassMember[1];

	hkpCollisionFilterList();
	hkpCollisionFilterList(hkFinishLoadedObjectFlag aFlag);
	hkpCollisionFilterList(const hkArray<const hkpCollisionFilter*, hkContainerHeapAllocator>& collisionFilters);
	~hkpCollisionFilterList() override;

	// The five filter interfaces of hkpCollisionFilter, answered by the filters of the list (the last one first).
	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;

	// Takes a reference on the filter and appends it to the list.
	void addCollisionFilter(const hkpCollisionFilter* filter);
	// Removes the filter from the list (the last entry takes its place) and releases the reference on it.
	void removeCollisionFilter(const hkpCollisionFilter* filter);
	// The SDK's getCollisionFilters: the list's filters.
	const hkArray<const hkpCollisionFilter*, hkContainerHeapAllocator>& getCollisionFilters();

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<const hkpCollisionFilter*> m_collisionFilters;

	static MembersArray Members;
};
