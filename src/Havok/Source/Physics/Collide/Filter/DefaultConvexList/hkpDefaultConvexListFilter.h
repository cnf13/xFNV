#pragma once

#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"

// The convex list filter worlds use unless told otherwise: the answer is a flag the collision input's dispatcher keeps
// per shape type, read for the other body's shape. No data besides hkReferencedObject's.
class hkpDefaultConvexListFilter : public hkpConvexListFilter {
public:
    // The implicit default constructor, which hkpWorld's constructor uses.
    hkpDefaultConvexListFilter() = default;
    // The SDK's finish constructor of a loaded object; assumes hkpConvexListFilter's (the SDK's).
    hkpDefaultConvexListFilter(hkFinishLoadedObjectFlag aFlag) : hkpConvexListFilter(aFlag) {}
    ~hkpDefaultConvexListFilter() override;

    ConvexListCollisionType getConvexListCollisionType(
        const hkpCdBody& convexListBody,
        const hkpCdBody& otherBody,
        const hkpCollisionInput& input
    ) const override;

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();
};

extern hkClass hkpDefaultConvexListFilterClass;

extern hkTypeInfo hkpDefaultConvexListFilterTypeInfo;
