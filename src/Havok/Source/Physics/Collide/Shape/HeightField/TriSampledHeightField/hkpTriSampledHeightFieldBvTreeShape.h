#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"

#include <xmmintrin.h>

// A bounding volume tree shape over a sampled heightfield's triangles (an hkpTriSampledHeightFieldCollection): the
// tree queries are answered from the heightfield's grid. m_wantAabbRejectionTest makes the queries drop the children
// whose boxes the query box does not overlap.
class hkpTriSampledHeightFieldBvTreeShape : public hkpBvTreeShape {
public:
    hkpTriSampledHeightFieldBvTreeShape(const hkpTriSampledHeightFieldCollection* c, hkBool doAabbRejection);
    hkpTriSampledHeightFieldBvTreeShape(hkFinishLoadedObjectFlag aFlag);
    ~hkpTriSampledHeightFieldBvTreeShape() override;

    void queryObb(
        const hkTransform& obbTransform,
        const hkVector4& obbExtent,
        float tolerance,
        hkArray<uint32_t>& hits
    ) const override;
    void queryAabb(const hkAabb& aabbIn, hkArray<uint32_t>& hits) const override;
    uint32_t queryAabbImpl(const hkAabb& aabbIn, uint32_t* hits, int32_t maxNumKeys) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& output) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    const hkpShapeContainer* getContainer() const override {
        return getShapeCollection()->getContainer();
    }
    int32_t calcSizeForSpu(const CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;

    // The SDK's inline getShapeCollection: the collection the shape was made over, its child.
    const hkpTriSampledHeightFieldCollection* getShapeCollection() const {
        return static_cast<const hkpTriSampledHeightFieldCollection*>(m_childContainer.m_childShape);
    }

    hkBool getWantAabbRejectionTest() const {
        return m_wantAabbRejectionTest;
    }
    void setWantAabbRejectionTest(hkBool newVal) {
        m_wantAabbRejectionTest = newVal;
    }

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();
    // Grows aabb by the collection's radius, clips it to the height field and returns its sample ranges; false when
    // nothing is left.
    static hkBool getExtentsForQueryAabb(
        hkAabb& aabb,
        const hkpTriSampledHeightFieldCollection* collection,
        uint32_t& minX,
        uint32_t& maxX,
        uint32_t& minZ,
        uint32_t& maxZ
    );

    hkpSingleShapeContainer m_childContainer;
    // The SDK's: mutable, calcSizeForSpu (const) stores it.
    mutable int32_t m_childSize;
    hkBool m_wantAabbRejectionTest;
    uint8_t m_padding[12];
};

inline hkQuadReal hkpTriSampledHeightFieldBvTreeShape_s_quadReal65536{65536.0f, 65536.0f, 65536.0f, 65536.0f};
