#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Reconstruction.hpp"

class hkpCollidable;
class hkpWorld;

// The kd-tree primitives of a list of collidables: the primitive id is the collidable's pointer and its bounds come
// from the collidable's shape.
class hkpCollidableMediator : public hkKdTreeBuildInput::Mediator {
public:
	hkpCollidableMediator(hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables, const hkpWorld* world);
	~hkpCollidableMediator() override;

	int32_t getNumPrimitives() const override;
	hkUlong getPrimitiveId(int32_t primitiveIdx) const override;
	void projectPrimitive(hkUlong primitiveId, hkKdTreeProjectedEntry& bounds) override;

	// A reference in the SDK; a pointer here, as IDA has it.
	hkArray<const hkpCollidable*>* m_collidables;
};
inline constexpr fnvd::Vtable hkpCollidableMediator_vftable{};
