#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>
#include <xmmintrin.h>

float* calcClosestLineLine(const __m128* a1, const __m128* a2, const __m128* a3, const __m128* a4, float* a5, float* a6,
	float* a7, __m128* a8, __m128* a9);

// A line segment swept by a radius: two end points and the convex shape's radius.
class hkpCapsuleShape : public hkpConvexShape {
public:
	enum RayHitType : int32_t {
		HIT_CAP0 = 0x0,
		HIT_CAP1 = 0x1,
		HIT_BODY = 0x2,
	};

	// The (vertexA, vertexB, radius) constructor on apMemory.
	hkpCapsuleShape(const hkVector4& arVertexA, const hkVector4& arVertexB, float afRadius);
	// The SDK's finish constructor of a loaded object.
	hkpCapsuleShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag) { m_type.m_storage = HK_SHAPE_CAPSULE; }
	~hkpCapsuleShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	static void closestPointLineSeg(const hkVector4& A, const hkVector4& B, const hkVector4& B2, hkVector4& a4);
	static const hkClass& staticClass();

	hkVector4 m_vertexA;
	hkVector4 m_vertexB;
};

// hkpConvexShape's reflection record (the name its static initializer gives the hkClass at this address).
extern hkClass hkpConvexShapeClass;
// The class's reflection record.
extern hkClass hkpCapsuleShapeClass;
extern hkTypeInfo hkpCapsuleShapeTypeInfo;
