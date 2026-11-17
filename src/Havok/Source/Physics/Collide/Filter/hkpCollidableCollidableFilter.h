#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkpCollidable;

// The interface that decides whether two collidables may collide.
class hkpCollidableCollidableFilter {
public:
	virtual ~hkpCollidableCollidableFilter();

	virtual hkBool isCollisionEnabled(const hkpCollidable&, const hkpCollidable&) const = 0;

	// DeletingDestruct's free (MemoryManager::Deallocate).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpCollidableCollidableFilter_vftable{};

// The class's reflection record.
extern hkClass hkpCollidableCollidableFilterClass;
