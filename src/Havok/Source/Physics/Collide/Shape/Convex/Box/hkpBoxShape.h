#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>

// An axis-aligned box around the origin, given by its half extents and rounded off by the convex shape's radius.
class hkpBoxShape : public hkpConvexShape {
public:
	hkpBoxShape(const hkVector4& a1, float a2);
	// The SDK's finish constructor of a loaded object.
	hkpBoxShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag) { m_type.m_storage = HK_SHAPE_BOX; }
	~hkpBoxShape() override;

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

	void setHalfExtents(const hkVector4& halfExtents);

	static const hkClass& staticClass();

	hkVector4 m_halfExtents;
};

extern hkClass hkpBoxShapeClass;
extern hkTypeInfo hkpBoxShapeTypeInfo;

// The signs of the box's eight corners, which are multiplied by the half extents.
using hkpBoxShapeVertexSigns_t = const hkQuadReal[8];
inline hkpBoxShapeVertexSigns_t hkpBoxShapeVertexSigns{{1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, -1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f, 1.0f},
	{1.0f, -1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f}};
