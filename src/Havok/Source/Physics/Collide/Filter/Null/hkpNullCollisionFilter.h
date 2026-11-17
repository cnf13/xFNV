#pragma once

#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Reconstruction.hpp"

// The filter that lets every pair collide (m_type HK_FILTER_NULL). It adds no data to hkpCollisionFilter. The exe
// inlines its constructor (hkpCollisionFilter's constructor, then the five vtables below).
class hkpNullCollisionFilter : public hkpCollisionFilter {
public:
	// The SDK's inline default constructor (the world's constructor inlines it): hkpCollisionFilter's, then the
	// null filter's type.
	hkpNullCollisionFilter() { m_type = HK_FILTER_NULL; }
	hkpNullCollisionFilter(hkFinishLoadedObjectFlag aFlag);

	// Every overload lets the pair collide: the exe writes true through the hidden result pointer.
	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override { return true; }

	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override {
		return true;
	}

	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override {
		return true;
	}

	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override {
		return true;
	}

	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override {
		return true;
	}

	~hkpNullCollisionFilter() override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();
};

inline constexpr fnvd::Vtable hkpNullCollisionFilter_vftable{};
inline constexpr fnvd::Vtable hkpNullCollisionFilter_vftable_hkpCollidableCollidableFilter{};
inline constexpr fnvd::Vtable hkpNullCollisionFilter_vftable_hkpShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpNullCollisionFilter_vftable_hkpRayShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpNullCollisionFilter_vftable_hkpRayCollidableFilter{};
