#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

class hkpMoppCode;
class hkpShapeCollection;

// The common part of the MOPP bounding volume tree shapes: the compiled MOPP code, a pointer to its bytes and a copy of
// the code's info.
class hkMoppBvTreeShapeBase : public hkpBvTreeShape {
public:
    hkMoppBvTreeShapeBase(hkpShapeType type, const hkpMoppCode* code);
    // The SDK's finish constructor, which hkpMoppBvTreeShape's builds its base with.
    hkMoppBvTreeShapeBase(hkFinishLoadedObjectFlag flag);
    ~hkMoppBvTreeShapeBase() override;

    void calcContentStatistics(hkStatisticsCollector* apCollector, const hkClass* apClass) const override = 0;
    void queryObb(
        const hkTransform& obbToMopp,
        const hkVector4& extent,
        float tolerance,
        hkArray<uint32_t>& hits
    ) const override;
    void queryAabb(const hkAabb& aabb, hkArray<uint32_t>& hits) const override;
    uint32_t queryAabbImpl(const hkAabb& aAabb, uint32_t* hits, int32_t maxNumKeys) const override;

    const hkpMoppCode* m_code;
    const uint8_t* m_moppData;
    uint32_t m_moppDataSize;
    hkVector4 m_codeInfoCopy;
};

// A bounding volume tree shape over one child shape, searched through the child's MOPP code.
class hkpMoppBvTreeShape : public hkMoppBvTreeShapeBase {
public:
    // Passes the collection to the child container and the code to hkMoppBvTreeShapeBase's constructor.
    hkpMoppBvTreeShape(const hkpShapeCollection* collection, const hkpMoppCode* code);
    hkpMoppBvTreeShape(hkFinishLoadedObjectFlag flag);
    ~hkpMoppBvTreeShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    const hkpShapeContainer* getContainer() const override {
        return m_child.m_childShape->getContainer();
    }
    int32_t calcSizeForSpu(const CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    hkVector4Comparison castRayBundleImpl(
        const hkpShapeRayBundleCastInput& input,
        hkpShapeRayBundleCastOutput& output,
        const hkVector4Comparison& mask
    ) const override;

    // The child shape as the collection it is.
    const hkpShapeCollection* getShapeCollection() const;
    const hkpMoppCode* getMoppCode() const {
        return m_code;
    }

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    hkpSingleShapeContainer m_child;
    // Mutable as in Havok's header: calcSizeForSpu (const) stores the child's size.
    mutable int32_t m_childSize;
};

// The class's reflection record.
extern hkClass hkMoppBvTreeShapeBaseClass;
extern hkClass hkpMoppBvTreeShapeClass;
extern hkTypeInfo hkpMoppBvTreeShapeTypeInfo;
