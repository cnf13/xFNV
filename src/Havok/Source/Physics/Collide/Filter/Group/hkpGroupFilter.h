#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The collision filter of the game's world: a 32 x 32 layer table (one bit mask per layer) plus the filter info's
// system group and sub-system rules. The members follow the PDB (the Xbox build, where m_collisionLookupTable is
// at 0x34 and m_pad256 at 0xC0); the exe's constructor (0xE489B0) stores type 2 (HK_FILTER_GROUP), sets
// m_nextFreeSystemGroup to 0 and every table entry to -1, which confirms 0x30 and 0x34 on the PC.
class hkpGroupFilter : public hkpCollisionFilter {
public:
	// Havok SDK (hkpGroupFilter.h): the filter info a shape container returns for "use the collidable's info".
	enum : uint32_t {
		USE_COLLIDABLE_FILTER_INFO = 0xffffffff
	};

	hkpGroupFilter();
	hkpGroupFilter(hkFinishLoadedObjectFlag aFlag);
	~hkpGroupFilter() override;

	// Slot 3, the exe's 0xE7CD40 (an empty body shared with other classes, returning 0).
	virtual int32_t dummyUnused();

	// The five filter interfaces of hkpCollisionFilter, answered from the layer table and the system groups.
	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;

	// The test of two filter infos the five interfaces share (thiscall, ret 0xC, the hkBool through a hidden result
	// pointer, 0xE48830).
	hkBool isCollisionEnabled(uint32_t infoA, uint32_t infoB) const;
	void disableCollisionsBetween(int32_t layerA, int32_t layerB);
	// Havok SDK: enables collisions between the layers of two bit sets.
	void enableCollisionsUsingBitfield(uint32_t layerBitsA, uint32_t layerBitsB);
	void enableCollisionsBetween(int32_t layerA, int32_t layerB);
	// Havok SDK: disables collisions between the layers of two bit sets.
	void disableCollisionsUsingBitfield(uint32_t layerBitsA, uint32_t layerBitsB);

	static const hkClass& staticClass();

	int32_t m_nextFreeSystemGroup;
	uint32_t m_collisionLookupTable[32];
	hkVector4 m_pad256[4];
};

extern hkClass hkpGroupFilterClass;
extern hkTypeInfo hkpGroupFilterTypeInfo;
inline constexpr fnvd::Vtable hkpGroupFilter_vftable{};
inline constexpr fnvd::Vtable hkpGroupFilter_vftable_hkpCollidableCollidableFilter{};
inline constexpr fnvd::Vtable hkpGroupFilter_vftable_hkpRayCollidableFilter{};
inline constexpr fnvd::Vtable hkpGroupFilter_vftable_hkpRayShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpGroupFilter_vftable_hkpShapeCollectionFilter{};
