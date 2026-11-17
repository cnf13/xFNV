#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>

// A convex child shape placed under a transform with scale, plus an extra scale vector.
class hkpConvexTransformShape : public hkpConvexTransformShapeBase {
public:
    hkpConvexTransformShape(
        hkpConvexShape* apChildShape,
        const hkTransform& arTransform,
        hkpShapeContainer::ReferencePolicy aeRef
    );

    // Assumes hkpConvexTransformShapeBase's finish constructor (the SDK's).
    hkpConvexTransformShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexTransformShapeBase(aFlag) {
        m_type.m_storage = HK_SHAPE_CONVEX_TRANSFORM;
    }

    ~hkpConvexTransformShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    float getMaximumProjection(const hkVector4& direction) const override;
    const hkpShapeContainer* getContainer() const override;
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    int32_t getNumCollisionSpheresImpl() const override;
    const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
    void getFirstVertex(hkVector4& aVertexOut) const override;
    void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
    void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
    void getCentreImpl(hkVector4& centreOut) const override;

    void setTransform(const hkTransform& transform);

    // The SDK's inline getChildShape: the child, a convex shape.
    const hkpConvexShape* getChildShape() const {
        return static_cast<const hkpConvexShape*>(m_childShape.m_childShape);
    }

    static const hkClass& staticClass();

    hkTransform m_transform;
};

// The class's reflection record and type info (hkpConvexTransformShape.cpp).
extern hkClass hkpConvexTransformShapeClass;
extern hkTypeInfo hkpConvexTransformShapeTypeInfo;
