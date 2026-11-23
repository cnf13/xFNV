#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>

// Exposes a sampled heightfield as a collection of triangles (two per grid cell), with a convex radius, per-triangle
// welding information and an extrusion applied to every child triangle.
class hkpTriSampledHeightFieldCollection : public hkpShapeCollection {
public:
    // The reflection's member table.
    using MembersArray = const hkClassMember[5];

    hkpTriSampledHeightFieldCollection(const hkpSampledHeightFieldShape* shape, float radius);
    hkpTriSampledHeightFieldCollection(hkFinishLoadedObjectFlag aFlag);
    ~hkpTriSampledHeightFieldCollection() override;

    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) override;
    void setWeldingInfo(uint32_t key, int16_t weldingInfo) override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getFirstKey() const override;
    // hkpShapeContainer's vtable at +0x10.
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    uint32_t getNextKey(uint32_t oldKey) const override;
    // hkpShapeContainer's vtable at +0x10.
    // Havok SDK signature (hkpCollide.lib, tied by vtable).
    uint32_t getCollisionFilterInfo(uint32_t key) const override;
    // hkpShapeContainer's vtable at +0x10.
    const hkpShape* getChildShape(uint32_t, char (&)[512]) const override;

    // The heightfield the triangles come from: m_heightfield.
    const hkpSampledHeightFieldShape* getHeightFieldShape() const;

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    // A key holds its cell's x in bits 1-15, its z in the high half and the cell's triangle in bit 0.
    int32_t getXFromShapeKey(uint32_t key) const {
        return (key & 0xFFFF) >> 1;
    }
    int32_t getZFromShapeKey(uint32_t key) const {
        return key >> 16;
    }
    int32_t getIndexFromShapeKey(uint32_t key) const {
        const int32_t x = getXFromShapeKey(key);
        const int32_t z = getZFromShapeKey(key);
        return ((z * (getHeightFieldShape()->m_xRes - 1) + x) << 1) | (key & 1);
    }
    // The caller checks that the collection has welding info.
    uint16_t getWeldingInfo(uint32_t key) const {
        return m_weldingInfo.m_data[getIndexFromShapeKey(key)];
    }

    const hkpSampledHeightFieldShape* m_heightfield;
    // The SDK's: mutable, calcSizeForSpu (const) stores it.
    mutable int32_t m_childSize;
    float m_radius;
    hkArray<uint16_t> m_weldingInfo;
    hkVector4 m_triangleExtrusion;

    static MembersArray Members;
};
