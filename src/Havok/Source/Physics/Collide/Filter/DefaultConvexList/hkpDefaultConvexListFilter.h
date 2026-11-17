#pragma once

#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Reconstruction.hpp"

// The convex list filter worlds use unless told otherwise: the answer is a flag the collision input's dispatcher
// keeps per shape type, read for the other body's shape (0xCF6DC0). The exe's vtable has 4 slots; slot 0 is the
// shared deleting destructor (0xDE10B0) and slot 3 its own getConvexListCollisionType. No data besides
// hkReferencedObject's.
class hkpDefaultConvexListFilter : public hkpConvexListFilter {
public:
	// The hybrid's implicit default constructor (the SDK's, which hkpWorld's constructor uses).
	hkpDefaultConvexListFilter() = default;
	// The SDK's finish constructor of a loaded object; assumes hkpConvexListFilter's (the SDK's).
	hkpDefaultConvexListFilter(hkFinishLoadedObjectFlag aFlag) : hkpConvexListFilter(aFlag) {}
	~hkpDefaultConvexListFilter() override;

	ConvexListCollisionType getConvexListCollisionType(const hkpCdBody& convexListBody, const hkpCdBody& otherBody,
		const hkpCollisionInput& input) const override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();
};
inline constexpr fnvd::Vtable hkpDefaultConvexListFilter_vftable{};

extern hkClass hkpDefaultConvexListFilterClass;

extern hkTypeInfo hkpDefaultConvexListFilterTypeInfo;
