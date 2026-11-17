#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Reconstruction.hpp"

// The interface a k-d tree ray cast reports its mid phase hits to. It holds no state. The exe returns the hkBools
// through a hidden pointer, so addMidPhaseHit and hasHit take it as their first argument.
class hkKdTreeCastCollector {
public:
	virtual ~hkKdTreeCastCollector();

	// The primitive id is the SDK's hkPrimitiveId (pointer-sized): the overrides use it as the hkpCollidable*.
	virtual hkBool* addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) = 0;
	virtual hkBool* hasHit(hkBool* arResult) = 0;
	virtual float getHitFraction() = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkKdTreeCastCollector_vftable{};
