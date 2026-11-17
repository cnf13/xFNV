#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkAabb;
class hkpBvTreeShape;
class hkpCdBody;
class hkpCollisionInput;
class hkpShapeContainer;

// The interface that decides whether the children of a shape collection may collide.
class hkpShapeCollectionFilter {
public:
	virtual hkBool isCollisionEnabled(const hkpCollisionInput&, const hkpCdBody&, const hkpCdBody&,
		const hkpShapeContainer&, uint32_t) const = 0;
	virtual hkBool isCollisionEnabled(const hkpCollisionInput&, const hkpCdBody&, const hkpCdBody&,
		const hkpShapeContainer&, const hkpShapeContainer&, uint32_t, uint32_t) const = 0;
	virtual int32_t numShapeKeyHitsLimitBreached(const hkpCollisionInput&, const hkpCdBody&, const hkpCdBody&,
		const hkpBvTreeShape*, hkAabb&, uint32_t*, int32_t) const = 0;
	virtual ~hkpShapeCollectionFilter();

	// DeletingDestruct's free (MemoryManager::Deallocate).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpShapeCollectionFilter_vftable{};

// The class's reflection record.
extern hkClass hkpShapeCollectionFilterClass;
