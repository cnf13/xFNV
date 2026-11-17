#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

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
inline constexpr fnvd::Vtable hkpRayCollidableFilter_vftable{};

// The class's reflection record.
extern hkClass hkpRayCollidableFilterClass;
