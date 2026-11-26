#pragma once

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpCollidable;
struct hkpWorldRayCastInput;

// The interface that decides whether a world ray may hit a collidable.
class hkpRayCollidableFilter {
public:
    virtual ~hkpRayCollidableFilter();

    virtual hkBool isCollisionEnabled(const hkpWorldRayCastInput&, const hkpCollidable&) const = 0;

    // DeletingDestruct's free (MemoryManager::Deallocate).
    static void operator delete(void* apMemory);
};

inline void hkpRayCollidableFilter::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

// The class's reflection record.
extern hkClass hkpRayCollidableFilterClass;

inline hkpRayCollidableFilter::~hkpRayCollidableFilter() {}
