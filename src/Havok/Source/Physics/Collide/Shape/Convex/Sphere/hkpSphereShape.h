#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>
#include <xmmintrin.h>

// A sphere around the origin: the convex shape's radius is the sphere's radius, so it adds only padding.
class hkpSphereShape : public hkpConvexShape {
public:
	hkpSphereShape(float afRadius);
	// The SDK's finish constructor of a loaded object.
	hkpSphereShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag) { m_type.m_storage = HK_SHAPE_SPHERE; }
	~hkpSphereShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	hkVector4Comparison* castRayBundleImpl(hkVector4Comparison* apResult, const hkpShapeRayBundleCastInput& input,
		hkpShapeRayBundleCastOutput& output, const hkVector4Comparison& mask) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	static const hkClass& staticClass();

	uint32_t m_pad16[3];
};

extern hkClass hkpSphereShapeClass;
extern hkTypeInfo hkpSphereShapeTypeInfo;
