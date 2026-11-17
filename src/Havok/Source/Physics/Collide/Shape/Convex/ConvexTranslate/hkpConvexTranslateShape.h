#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"

#include <stdint.h>

// A convex child shape with a translation but no rotation.
class hkpConvexTranslateShape : public hkpConvexTransformShapeBase {
public:
	// Assumes hkpConvexTransformShapeBase's finish constructor (the SDK's).
	hkpConvexTranslateShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexTransformShapeBase(aFlag) {
		m_type.m_storage = HK_SHAPE_CONVEX_TRANSLATE;
	}

	// The SDK's constructor from the child, the translation (its w cleared) and the reference policy.
	hkpConvexTranslateShape(hkpConvexShape* apChildShape, const hkVector4& translation,
		hkpShapeContainer::ReferencePolicy ref)
		: hkpConvexTransformShapeBase(HK_SHAPE_CONVEX_TRANSLATE, apChildShape->m_radius, apChildShape, ref) {
		m_translation.setXYZ0(translation);
		m_childShapeSize = 0;
	}

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	float getMaximumProjection(const hkVector4& direction) const override;
	const hkpShapeContainer* getContainer() const override;
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	// The SDK's inline getChildShape: the child, a convex shape.
	const hkpConvexShape* getChildShape() const {
		return static_cast<const hkpConvexShape*>(m_childShape.m_childShape);
	}

	static const hkClass& staticClass();

	hkVector4 m_translation;
};

extern hkClass hkpConvexTranslateShapeClass;
extern hkTypeInfo hkpConvexTranslateShapeTypeInfo;
