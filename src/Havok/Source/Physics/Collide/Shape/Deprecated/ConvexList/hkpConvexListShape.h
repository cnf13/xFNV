#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A convex shape made of a list of convex child shapes (Havok files it under the deprecated shapes), with an
// optionally cached AABB and the distance from which closest-point queries use the children's convex hull.
class hkpConvexListShape : public hkpConvexShape, public hkpShapeContainer {
public:
	hkpConvexListShape(const hkpConvexShape* const* apChildShapes, int32_t aiNumChildShapes);

	// Assumes hkpConvexShape's finish constructor (the SDK's); the child list stays as loaded.
	hkpConvexListShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag), m_childShapes(aFlag) {
		m_type.m_storage = HK_SHAPE_CONVEX_LIST;
	}

	~hkpConvexListShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	const hkpShapeContainer* getContainer() const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& dir, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	// hkpShapeContainer's vtable at +0x14.
	int32_t getNumChildShapes() const override;
	// hkpShapeContainer's vtable at +0x14.
	uint32_t getFirstKey() const override;
	// hkpShapeContainer's vtable at +0x14.
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	uint32_t getNextKey(uint32_t oldKey) const override;
	// hkpShapeContainer's vtable at +0x14.
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	uint32_t getCollisionFilterInfo(uint32_t key) const override;
	// hkpShapeContainer's vtable at +0x14.
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;
	// hkpConvexShape's pure virtual, concrete in the exe: vtable slot 17 holds hkpConvexShape::getCentreImpl
	// (0xC9E250), the defined pure virtual's body.
	void getCentreImpl(hkVector4& aCentreOut) const override { hkpConvexShape::getCentreImpl(aCentreOut); }

	bool getUseCachedAabb();
	void setUseCachedAabb(bool useCachedAabb);
	void setShapesAndRadius(const hkpConvexShape* const* shapeArray, int32_t numShapes);

	// DeletingDestruct's free (the heap's blockFree of m_memSizeAndFlags, hkReferencedObject's); declared here
	// because hkpShapeContainer's makes the inherited one ambiguous.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	static const hkClass& staticClass();

	float m_minDistanceToUseConvexHullForGetClosestPoints;
	hkVector4 m_aabbHalfExtents;
	hkVector4 m_aabbCenter;
	bool m_useCachedAabb;
	hkArray<const hkpConvexShape*> m_childShapes;
};

extern hkClass hkpConvexListShapeClass;
extern hkTypeInfo hkpConvexListShapeTypeInfo;
inline constexpr fnvd::Vtable hkpConvexListShape_vftable_hkpShapeContainer{};
inline constexpr fnvd::Vtable hkpConvexListShape_vftable{};
