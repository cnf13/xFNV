#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The data types the AABB trees and their queries share: rays, ray bundles, cast volumes and the two bounding
// volume formats (full float precision, and 16 bits per axis). hkBool32 is spelled int32_t. The SDK's declarations;
// of the inline functions, only those the native build's queries call are defined (after the class).
class hkAabbTreeData {
public:
    // A ray: its start and the reciprocal of its direction.
    struct Ray {
        void setRay(const hkVector4& start, const hkVector4& end);

        hkVector4 m_start;
        hkVector4 m_invDir;
    };

    // Four vectors' worth of one component set, as the SDK's SIMD ray bundles use them.
    struct Bundle {
        hkVector4 m_data[3];
    };

    // Four rays by their endpoints, with the number in use.
    struct SetRays {
        hkVector4 m_from[4];
        hkVector4 m_to[4];
        int32_t m_numActiveRays;
    };

    // Four rays as bundles, with the lanes in use.
    struct RayBundle {
        Bundle m_from;
        Bundle m_invDir;
        hkVector4Comparison::Mask m_mask;
    };

    // A swept volume: the from and to positions and the half extents of the box.
    struct CastVolumeInput {
        hkVector4 m_from;
        hkVector4 m_to;
        hkVector4 m_halfExtents;
    };

    // A tree node's bounding volume quantised to 16 bits per axis.
    struct hkAabbTreeAabb16 {
        struct Metric {
            int32_t select(const hkAabbTreeAabb16& a, const hkAabbTreeAabb16& b) const;

            hkVector4 m_center;
        };

        Metric getMetric() const;
        void center(hkVector4& centerOut) const;
        int32_t equals(const hkAabbTreeAabb16& aabb) const;
        hkBool contains(const hkAabbTreeAabb16& other) const;
        void includeAabb(const hkAabbTreeAabb16& aabb);
        hkBool isValid() const;
        void setEmpty();
        int32_t overlaps(const hkAabbTreeAabb16& testAabb) const;
        float distanceSquared(const hkVector4& point) const;
        int32_t overlaps(const hkAabb& testAabb) const;

        static void combine(const hkAabbTreeAabb16& a, const hkAabbTreeAabb16& b, hkAabbTreeAabb16& out);
        static void convertAabbToTreeSpace(
            const hkAabb& volume,
            const hkVector4& bitOffsetLow,
            const hkVector4& bitOffsetHigh,
            const hkVector4& bitScale,
            hkAabbTreeAabb16& volumeOut
        );
        static void convertAabbToTreeSpace(
            const hkAabbUint32& volumeIn,
            const hkVector4& bitOffsetLow,
            const hkVector4& bitOffsetHigh,
            const hkVector4& bitScale,
            hkAabbTreeAabb16& volumeOut
        );
        static void restoreAabb(
            const hkAabbTreeAabb16& in,
            const hkVector4& bitScaleInv,
            const hkVector4& bitOffsetLow,
            hkAabb& out
        );

        uint16_t m_max[3];
        uint16_t m_min[3];
    };

    // A tree node's bounding volume at full float precision.
    struct hkAabbTreeAabb : public hkAabb {
        struct Metric {
            int32_t select(const hkAabbTreeAabb& a, const hkAabbTreeAabb& b) const;

            hkVector4 m_center;
        };

        Metric getMetric() const;
        void center(hkVector4& centerOut) const;
        int32_t overlaps(const Ray& r, const hkVector4& earlyOutHitFraction) const;
        int32_t overlaps(const hkAabb& aabb) const;
        void expand(const hkVector4& velocity, float margin);
        float distanceSquared(const hkVector4& point) const;

        static void combine(const hkAabbTreeAabb& a, const hkAabbTreeAabb& b, hkAabbTreeAabb& out);
        static void convertAabbToTreeSpace(
            const hkAabb& volume,
            const hkVector4& bitOffsetLow,
            const hkVector4& bitOffsetHigh,
            const hkVector4& bitScale,
            hkAabbTreeAabb& volumeOut
        );
        static void convertAabbToTreeSpace(
            const hkAabbUint32& volumeIn,
            const hkVector4& bitOffsetLow,
            const hkVector4& bitOffsetHigh,
            const hkVector4& bitScale,
            hkAabbTreeAabb& volumeOut
        );
        static void
        restoreAabb(const hkAabbTreeAabb& in, const hkVector4& bitScaleInv, const hkVector4& bitOffsetLow, hkAabb& out);
    };
};

// Whether ray r passes through the box before earlyOutHitFraction, from the slab test's x lane.
inline int32_t hkAabbTreeData::hkAabbTreeAabb::overlaps(const Ray& r, const hkVector4& earlyOutHitFraction) const {
    hkVector4 dNearTemp;
    hkVector4 dFarTemp;
    dNearTemp.setSub4(m_min, r.m_start);
    dFarTemp.setSub4(m_max, r.m_start);
    dNearTemp.mul4(r.m_invDir);
    dFarTemp.mul4(r.m_invDir);
    hkVector4 dNear;
    dNear.setMin4(dNearTemp, dFarTemp);
    hkVector4 dFar;
    dFar.setMax4(dNearTemp, dFarTemp);
    hkVector4 dNearY;
    dNearY.setBroadcast(dNear, 1);
    hkVector4 dNearZ;
    dNearZ.setBroadcast(dNear, 2);
    hkVector4 dFarY;
    dFarY.setBroadcast(dFar, 1);
    hkVector4 dFarZ;
    dFarZ.setBroadcast(dFar, 2);
    hkVector4 nearYZ;
    nearYZ.setMax4(dNearY, dNearZ);
    hkVector4 farYZ;
    farYZ.setMin4(dFarY, dFarZ);
    hkVector4 nearX;
    nearX.setMax4(dNear, hkVector4::getZero());
    hkVector4 farX;
    farX.setMin4(dFar, earlyOutHitFraction);
    nearX.setMax4(nearX, nearYZ);
    farX.setMin4(farX, farYZ);
    // The SDK's nearX.compareLessThan4(farX).anyIsSet(MASK_X).
    return _mm_movemask_ps(_mm_cmplt_ps(nearX.m_quad, farX.m_quad)) & hkVector4Comparison::MASK_X;
}

// The box's centre in tree space, half the sum of its corners, each set from its three 16-bit values with a zero w.
inline void hkAabbTreeData::hkAabbTreeAabb16::center(hkVector4& centerOut) const {
    centerOut.set(m_max[0], m_max[1], m_max[2], 0.0f);
    hkVector4 myMin;
    myMin.set(m_min[0], m_min[1], m_min[2], 0.0f);
    centerOut.add4(myMin);
    hkVector4 half;
    half.setAll(0.5f);
    centerOut.mul4(half);
}

// Inlines for the full-precision box: the centre, half the sum of its corners, and the box around two boxes.
inline void hkAabbTreeData::hkAabbTreeAabb::center(hkVector4& centerOut) const {
    centerOut.setAdd4(m_min, m_max);
    hkVector4 half;
    half.setAll(0.5f);
    centerOut.mul4(half);
}

inline void
hkAabbTreeData::hkAabbTreeAabb::combine(const hkAabbTreeAabb& a, const hkAabbTreeAabb& b, hkAabbTreeAabb& out) {
    out.m_min.setMin4(a.m_min, b.m_min);
    out.m_max.setMax4(a.m_max, b.m_max);
}

// The child whose centre is nearer the metric's: the sign of the difference of the squared distances' sums (z + (y +
// x)); 1 when the second child's is the smaller, as the SDK's SIMD Win32 build has it.
inline int32_t
hkAabbTreeData::hkAabbTreeAabb16::Metric::select(const hkAabbTreeAabb16& a, const hkAabbTreeAabb16& b) const {
    hkVector4 centerA;
    a.center(centerA);
    hkVector4 centerB;
    b.center(centerB);
    centerA.sub4(m_center);
    centerB.sub4(m_center);
    centerA.mul4(centerA);
    centerB.mul4(centerB);
    centerB.sub4(centerA);
    const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(centerB.m_quad, centerB.m_quad, 0x55), centerB.m_quad);
    const __m128 kSum = _mm_add_ss(_mm_shuffle_ps(centerB.m_quad, centerB.m_quad, 0xAA), kXY);
    return _mm_movemask_ps(kSum) & 1;
}

// The metric of a box: its centre.
inline hkAabbTreeData::hkAabbTreeAabb16::Metric hkAabbTreeData::hkAabbTreeAabb16::getMetric() const {
    Metric metric;
    center(metric.m_center);
    return metric;
}

// The box holding a and b.
inline void
hkAabbTreeData::hkAabbTreeAabb16::combine(const hkAabbTreeAabb16& a, const hkAabbTreeAabb16& b, hkAabbTreeAabb16& out) {
    for (int32_t i = 0; i < 3; ++i) {
        out.m_min[i] = a.m_min[i] < b.m_min[i] ? a.m_min[i] : b.m_min[i];
        out.m_max[i] = a.m_max[i] > b.m_max[i] ? a.m_max[i] : b.m_max[i];
    }
}

// Whether both boxes have the same corners.
inline int32_t hkAabbTreeData::hkAabbTreeAabb16::equals(const hkAabbTreeAabb16& aabb) const {
    for (int32_t i = 0; i < 3; ++i) {
        if (aabb.m_min[i] != m_min[i] || aabb.m_max[i] != m_max[i]) {
            return 0;
        }
    }
    return 1;
}

// Whether other lies inside the box.
inline hkBool hkAabbTreeData::hkAabbTreeAabb16::contains(const hkAabbTreeAabb16& other) const {
    bool outside = false;
    for (int32_t i = 0; i < 3; ++i) {
        outside |= m_max[i] < other.m_max[i] || other.m_min[i] < m_min[i];
    }
    hkBool kResult;
    kResult.m_bool = !outside;
    return kResult;
}

// The box quantised to the tree's 32-bit space, then each corner's top 16 bits.
inline void hkAabbTreeData::hkAabbTreeAabb16::convertAabbToTreeSpace(
    const hkAabb& volume,
    const hkVector4& bitOffsetLow,
    const hkVector4& bitOffsetHigh,
    const hkVector4& bitScale,
    hkAabbTreeAabb16& volumeOut
) {
    hkAabbUint32 qAabb;
    hkVector4Util::convertAabbToUint32(volume, bitOffsetLow, bitOffsetHigh, bitScale, qAabb);
    convertAabbToTreeSpace(qAabb, bitOffsetLow, bitOffsetHigh, bitScale, volumeOut);
}

inline void hkAabbTreeData::hkAabbTreeAabb16::convertAabbToTreeSpace(
    const hkAabbUint32& volumeIn,
    const hkVector4& bitOffsetLow,
    const hkVector4& bitOffsetHigh,
    const hkVector4& bitScale,
    hkAabbTreeAabb16& volumeOut
) {
    for (int32_t i = 0; i < 3; ++i) {
        volumeOut.m_max[i] = static_cast<uint16_t>(volumeIn.m_max[i] >> 15);
        volumeOut.m_min[i] = static_cast<uint16_t>(volumeIn.m_min[i] >> 15);
    }
}

// As the 16-bit metric's select, on the sums of the corners, but 0 when the second child's distance is the smaller.
inline int32_t hkAabbTreeData::hkAabbTreeAabb::Metric::select(const hkAabbTreeAabb& a, const hkAabbTreeAabb& b) const {
    hkVector4 centerA;
    centerA.setAdd4(a.m_min, a.m_max);
    hkVector4 centerB;
    centerB.setAdd4(b.m_min, b.m_max);
    centerA.sub4(m_center);
    centerB.sub4(m_center);
    centerA.mul4(centerA);
    centerB.mul4(centerB);
    centerB.sub4(centerA);
    const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(centerB.m_quad, centerB.m_quad, 0x55), centerB.m_quad);
    const __m128 kSum = _mm_add_ss(_mm_shuffle_ps(centerB.m_quad, centerB.m_quad, 0xAA), kXY);
    return (_mm_movemask_ps(kSum) & 1) ? 0 : 1;
}

// The metric of a box: the sum of its corners (twice its centre).
inline hkAabbTreeData::hkAabbTreeAabb::Metric hkAabbTreeData::hkAabbTreeAabb::getMetric() const {
    Metric metric;
    metric.m_center.setAdd4(m_min, m_max);
    return metric;
}

// The full-precision tree keeps the box as it is.
inline void hkAabbTreeData::hkAabbTreeAabb::convertAabbToTreeSpace(
    const hkAabb& volume,
    const hkVector4& bitOffsetLow,
    const hkVector4& bitOffsetHigh,
    const hkVector4& bitScale,
    hkAabbTreeAabb& volumeOut
) {
    volumeOut.m_max = volume.m_max;
    volumeOut.m_min = volume.m_min;
}

// The ray from start to end. A zero lane of the direction is taken as 1 for the reciprocal and its inverse set to
// HK_REAL_MAX (3.40282e+38f).
inline void hkAabbTreeData::Ray::setRay(const hkVector4& start, const hkVector4& end) {
    m_start = start;
    hkVector4 delta;
    delta.setSub4(end, start);
    const __m128 isZero = _mm_cmpeq_ps(delta.m_quad, hkVector4::getZero().m_quad);
    hkVector4 maskedDelta;
    maskedDelta.m_quad = _mm_or_ps(_mm_and_ps(isZero, hkQuadReal1111), _mm_andnot_ps(isZero, delta.m_quad));
    hkVector4 invDelta;
    invDelta.setReciprocal3(maskedDelta);
    m_invDir.m_quad = _mm_or_ps(_mm_andnot_ps(isZero, invDelta.m_quad), _mm_and_ps(isZero, _mm_set1_ps(3.40282e+38f)));
}

// The SDK's inline (hkAabbTreeData.inl): the 16-bit box back in world space, each bound shifted back to 31 bits.
inline void hkAabbTreeData::hkAabbTreeAabb16::restoreAabb(
    const hkAabbTreeAabb16& in,
    const hkVector4& bitScaleInv,
    const hkVector4& bitOffsetLow,
    hkAabb& out
) {
    out.m_max.set(
        static_cast<float>(in.m_max[0] << 15), static_cast<float>(in.m_max[1] << 15),
        static_cast<float>(in.m_max[2] << 15), 0.0f
    );
    out.m_min.set(
        static_cast<float>(in.m_min[0] << 15), static_cast<float>(in.m_min[1] << 15),
        static_cast<float>(in.m_min[2] << 15), 0.0f
    );
    out.m_min.mul4(bitScaleInv);
    out.m_max.mul4(bitScaleInv);
    out.m_min.sub4(bitOffsetLow);
    out.m_max.sub4(bitOffsetLow);
}

// The SDK's inline (hkAabbTreeData.inl): the full-precision box is kept as it is.
inline void hkAabbTreeData::hkAabbTreeAabb::restoreAabb(
    const hkAabbTreeAabb& in,
    const hkVector4& bitScaleInv,
    const hkVector4& bitOffsetLow,
    hkAabb& out
) {
    out = in;
}
