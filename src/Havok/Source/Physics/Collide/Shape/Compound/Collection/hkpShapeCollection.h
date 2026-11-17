#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>

// Base of the shapes made of child shapes (lists, meshes, heightfields): it is both a shape and the container of its
// children, and says whether contact welding applies.
class hkpShapeCollection : public hkpShape, public hkpShapeContainer {
public:
	enum CollectionType : int32_t {
		COLLECTION_LIST = 0x0,
		COLLECTION_EXTENDED_MESH = 0x1,
		COLLECTION_TRISAMPLED_HEIGHTFIELD = 0x2,
		COLLECTION_USER = 0x3,
		COLLECTION_SIMPLE_MESH = 0x4,
		COLLECTION_MESH_SHAPE = 0x5,
		COLLECTION_COMPRESSED_MESH = 0x6,
		COLLECTION_MAX = 0x7,
	};

	hkpShapeCollection(hkpShapeType a1, hkpShapeCollection::CollectionType a2);
	// The SDK's finish constructor, which the derived classes' finish constructors build their base with.
	hkpShapeCollection(hkFinishLoadedObjectFlag flag);
	~hkpShapeCollection() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	float getMaximumProjection(const hkVector4& direction) const override;
	const hkpShapeContainer* getContainer() const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	virtual void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType);
	virtual void setWeldingInfo(uint32_t key, int16_t weldingInfo);
	bool isWeldingEnabled() const override { return m_disableWelding.m_bool == 0; }

	// DeletingDestruct's free (the heap's blockFree of m_memSizeAndFlags, hkReferencedObject's); declared here
	// because hkpShapeContainer's makes the inherited one ambiguous.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkBool m_disableWelding;
	hkEnum<CollectionType, uint8_t> m_collectionType;
};

// hkpShape's reflection record (the name its static initializer gives the hkClass at this address).
extern hkClass hkpShapeClass;

// The class's reflection record, which the shape collections' calcContentStatistics pass to
// hkpShapeCollection::calcContentStatistics.
extern hkClass hkpShapeCollectionClass;
