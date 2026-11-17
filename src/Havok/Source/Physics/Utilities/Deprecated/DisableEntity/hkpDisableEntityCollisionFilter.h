#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

class hkpEntity;

// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpDisableEntityCollisionFilter();
void finishLoadedObjecthkpDisableEntityCollisionFilter(void* p, int32_t finishing);

// A filter that disables every collision of the entities it holds, and lets everything else collide. It listens to
// the entities so that it drops them when they leave the world.
class hkpDisableEntityCollisionFilter : public hkpCollisionFilter, public hkpEntityListener {
public:
	hkpDisableEntityCollisionFilter();
	hkpDisableEntityCollisionFilter(hkFinishLoadedObjectFlag aFlag);
	~hkpDisableEntityCollisionFilter() override;

	void entityRemovedCallback(hkpEntity* entity) override;
	void entityDeletedCallback(hkpEntity* entity) override;

	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;

	// Drops entity from the filter; the result says whether it was held.
	hkBool* removeEntityFromFilter(hkBool* arResult, hkpEntity* entity);
	hkBool* addEntityToFilter(hkBool* apResult, hkpEntity* new_entity);

	// DeletingDestruct's free (the thread's Havok heap): names one of the two bases' operator deletes.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Havok SDK signature (hkpUtilities.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkpEntity*> m_disabledEntities;
};

extern hkClass hkpDisableEntityCollisionFilterClass;

extern hkTypeInfo hkpDisableEntityCollisionFilterTypeInfo;
