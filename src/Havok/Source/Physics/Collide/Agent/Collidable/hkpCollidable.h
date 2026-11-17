#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidableQualityType.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkAabbUint32;

// A collidable: the collision node the broad phase knows the body by, holding its shape, filter and AABB data.
class hkpCollidable : public hkpCdBody {
public:
	// Why a collidable is forced to collide on the PPU instead of the SPU.
	enum ForceCollideOntoPpuReasons {
		FORCE_PPU_USER_REQUEST = 0x1,
		FORCE_PPU_SHAPE_REQUEST = 0x2,
		FORCE_PPU_MODIFIER_REQUEST = 0x4,
	};

	// The child AABBs of a compound shape, used by the broad phase.
	struct BoundingVolumeData {
		BoundingVolumeData();
		// The SDK's finish constructor: when finishing a load, no cached child AABBs and an invalid volume;
		// otherwise the loaded bytes are left as they are.
		BoundingVolumeData(hkFinishLoadedObjectFlag flag);

		bool hasAllocations() const { return nullptr != m_childShapeAabbs; }
		void deallocate();
		void allocate(int32_t numChildShapes);

		void invalidate() {
			m_min[0] = 1;
			m_max[0] = 0;
		}

		bool isValid() const { return m_min[0] <= m_max[0]; }

		static const hkClass& staticClass();

		uint32_t m_min[3];
		uint8_t m_expansionMin[3];
		uint8_t m_expansionShift;
		uint32_t m_max[3];
		uint8_t m_expansionMax[3];
		uint8_t m_padding;
		uint16_t m_numChildShapeAabbs;
		uint16_t m_capacityChildShapeAabbs;
		hkAabbUint32* m_childShapeAabbs;
		uint32_t* m_childShapeKeys;
	};

	// The reflection's member table (the PDB's hkpCollidable::Members, .rdata).
	using MembersArray = const hkClassMember[6];

	// The remaining SDK methods.
	// The implicit constructor, for the collidables the agents fill in on the stack: only the volume data's
	// constructor runs, which leaves no child AABBs for the destructor to free.
	hkpCollidable() = default;
	hkpCollidable(const hkpShape* shape, const hkTransform* ms, int32_t type);
	hkpCollidable(const hkpShape* shape, const hkMotionState* ms, int32_t type);
	// The SDK's finish constructor (inlined into hkpLinkedCollidable's): the volume data's, then the handle's owner
	// offset when finishing.
	hkpCollidable(hkFinishLoadedObjectFlag flag);
	~hkpCollidable();

	// The owner sits m_ownerOffset bytes from the collidable's address. The exe's copy is 0x4B59D0.
	void* getOwner() const { return const_cast<char*>(reinterpret_cast<const char*>(this) + m_ownerOffset); }
	// The exe's copy is 0x7D6BB0.
	hkpTypedBroadPhaseHandle* getBroadPhaseHandle() { return &m_broadPhaseHandle; }
	// The exe's copy is 0x43B540; it reads the handle's field through the handle's one-field getter (0x44DDC0).
	uint32_t getCollisionFilterInfo() const { return m_broadPhaseHandle.m_collisionFilterInfo; }
	// The exe's copy is 0x4B5980.
	int32_t getType() const { return m_broadPhaseHandle.getType(); }
	const hkpShape* getShape() const;
	// The SDK's debug build also runs checkPerformance here.
	void setShape(const hkpShape* shape) { m_shape = shape; }

	void setOwner(void* owner) {
		m_ownerOffset = static_cast<int8_t>(static_cast<char*>(owner) - reinterpret_cast<char*>(this));
	}

	hkBool checkPerformance();

	hkpCollidableQualityType getQualityType() const {
		return hkpCollidableQualityType(m_broadPhaseHandle.m_objectQualityType);
	}

	void setQualityType(hkpCollidableQualityType type) { m_broadPhaseHandle.m_objectQualityType = int8_t(type); }
	float getAllowedPenetrationDepth() const { return m_allowedPenetrationDepth; }
	void setAllowedPenetrationDepth(float val) { m_allowedPenetrationDepth = val; }
	void setCollisionFilterInfo(uint32_t info) { m_broadPhaseHandle.m_collisionFilterInfo = info; }

	static const hkClass& staticClass();

	// The owner sits m_ownerOffset bytes from the collidable's address.
	int8_t m_ownerOffset;
	uint8_t m_forceCollideOntoPpu;
	uint16_t m_shapeSizeOnSpu;
	hkpTypedBroadPhaseHandle m_broadPhaseHandle;
	BoundingVolumeData m_boundingVolumeData;
	float m_allowedPenetrationDepth;

	static MembersArray Members;
};

extern hkClass hkpCollidableBoundingVolumeDataClass;
extern hkClass hkpCollidableClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpCollidableBoundingVolumeDataTypeInfo;
extern hkTypeInfo hkpCollidableTypeInfo;

// The SDK's inline one-field getter. The exe has no copy of its own: it reads m_shape through the shared one-field
// getter it also uses for NiPointer<NiRefObject>::operator-> (0x559450, a trivial accessor), as hkpCdBody's does.
inline const hkpShape* hkpCollidable::getShape() const {
	return m_shape;
}
