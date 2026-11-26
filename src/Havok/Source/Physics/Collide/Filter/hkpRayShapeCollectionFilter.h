#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpShape;
class hkpShapeContainer;
struct hkpShapeRayCastInput;

// The interface that decides whether a ray may hit a child of a shape collection.
class hkpRayShapeCollectionFilter {
public:
    virtual hkBool isCollisionEnabled(
        const hkpShapeRayCastInput& aInput,
        const hkpShape& bShape,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const = 0;
    virtual ~hkpRayShapeCollectionFilter();

    // DeletingDestruct's free (MemoryManager::Deallocate).
    static void operator delete(void* apMemory);
};

inline hkpRayShapeCollectionFilter::~hkpRayShapeCollectionFilter() {}

// The class's reflection record.
extern hkClass hkpRayShapeCollectionFilterClass;
