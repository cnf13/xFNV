#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Reconstruction.hpp"

class hkpConstraintInstance;

const void* getVtablehkpConstrainedSystemFilter();
void finishLoadedObjecthkpConstrainedSystemFilter(void* p, int32_t finishing);

// A collision filter that keeps the entities a constraint joins from colliding with each other, and passes every
// other pair on to another filter. It listens to the constraints added to and removed from the world. The SDK's
// constructor takes the other filter.
class hkpConstrainedSystemFilter : public hkpCollisionFilter, public hkpConstraintListener {
public:
	hkpConstrainedSystemFilter(hkFinishLoadedObjectFlag aFlag);
	hkpConstrainedSystemFilter(const hkpCollisionFilter* otherFilter);
	~hkpConstrainedSystemFilter() override;

	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;
	void constraintAddedCallback(hkpConstraintInstance* constraint) override;
	void constraintRemovedCallback(hkpConstraintInstance* constraint) override;
	const hkClass* getClassType() const override;

	// DeletingDestruct's free (the thread's Havok heap): names one of the two bases' operator deletes.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// The reflection class hkpConstrainedSystemFilterClass.
	static const hkClass& staticClass();

	// The filter the pairs no constraint decides go to (private in the SDK).
	const hkpCollisionFilter* m_otherFilter;
};

inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable{};
inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable_hkpCollidableCollidableFilter{};
inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable_hkpShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable_hkpRayShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable_hkpRayCollidableFilter{};
inline constexpr fnvd::Vtable hkpConstrainedSystemFilter_vftable_hkpConstraintListener{};
extern hkClass hkpConstrainedSystemFilterClass;
extern hkTypeInfo hkpConstrainedSystemFilterTypeInfo;
