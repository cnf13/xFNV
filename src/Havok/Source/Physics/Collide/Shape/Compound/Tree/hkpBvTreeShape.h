#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

class hkAabb;
class hkTransform;

// The most shape keys one AABB query returns (the SDK's hkpBvTreeShape.h).
enum {
	HK_MAX_NUM_HITS_PER_AABB_QUERY = 4096
};

// Base of the shapes that answer queries through a bounding volume tree (the MOPP code, the sampled heightfield):
// they return the keys of the child shapes that overlap a box. Every query slot is abstract here.
class hkpBvTreeShape : public hkpShape {
public:
	enum BvTreeType : int32_t {
		BVTREE_MOPP = 0x0,
		BVTREE_TRISAMPLED_HEIGHTFIELD = 0x1,
		BVTREE_USER = 0x2,
		BVTREE_MAX = 0x3,
	};

	// A finished tree is a user tree until its class says otherwise.
	hkpBvTreeShape(hkpShapeType type, BvTreeType bvType) : hkpShape(type) { m_bvTreeType = bvType; }

	hkpBvTreeShape(hkFinishLoadedObjectFlag flag) : hkpShape(flag) {
		if (flag.m_finishing) {
			m_type = HK_SHAPE_BV_TREE;
			m_bvTreeType = BVTREE_USER;
		}
	}

	const hkpShapeContainer* getContainer() const override = 0;
	// Appends the keys of the children that overlap the oriented box.
	virtual void queryObb(const hkTransform& aObbToBvTree, const hkVector4& aHalfExtents, float aTolerance,
		hkArray<uint32_t>& aHits) const = 0;
	// Appends the keys of the children that overlap the box.
	virtual void queryAabb(const hkAabb& aAabb, hkArray<uint32_t>& aHits) const = 0;
	// The query into a fixed key buffer.
	virtual uint32_t queryAabbImpl(const hkAabb& aAabb, uint32_t* aHits, int32_t aMaxNumKeys) const = 0;

	hkEnum<BvTreeType, uint8_t> m_bvTreeType;
};

// The class's reflection record.
extern hkClass hkpBvTreeShapeClass;
