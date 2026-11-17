#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// An axis aligned bounding box.
class hkAabb {
public:
    hkAabb();
    hkAabb(const hkVector4& min, const hkVector4& max);

    // Grows the box to hold point.
    void includePoint(const hkVector4& point) {
        m_min.setMin4(m_min, point);
        m_max.setMax4(m_max, point);
    }

    // Whether the boxes overlap in x, y and z. The debug asserts are left out.
    int32_t overlaps(const hkAabb& other) const {
        hkVector4Comparison mincomp = m_min.compareLessThanEqual4(other.m_max);
        hkVector4Comparison maxcomp = other.m_min.compareLessThanEqual4(m_max);
        hkVector4Comparison both;
        both.setAnd(mincomp, maxcomp);
        return both.allAreSet(hkVector4Comparison::MASK_XYZ);
    }

    // Whether other lies inside the box in x, y and z.
    hkBool contains(const hkAabb& other) const {
        hkVector4Comparison mincomp = m_min.compareLessThanEqual4(other.m_min);
        hkVector4Comparison maxcomp = other.m_max.compareLessThanEqual4(m_max);
        hkVector4Comparison both;
        both.setAnd(mincomp, maxcomp);
        return both.allAreSet(hkVector4Comparison::MASK_XYZ) != 0;
    }

    // The inverted box: m_min at HK_REAL_MAX, m_max at -HK_REAL_MAX.
    void setEmpty() {
        m_min.setAll(3.40282e+38f);
        m_max.setAll(-3.40282e+38f);
    }

    // Whether other lies inside the box in x, y and z.
    int32_t containsPoint(const hkVector4& other) const {
        hkVector4Comparison mincomp = m_min.compareLessThanEqual4(other);
        hkVector4Comparison maxcomp = other.compareLessThanEqual4(m_max);
        hkVector4Comparison both;
        both.setAnd(mincomp, maxcomp);
        return both.allAreSet(hkVector4Comparison::MASK_XYZ);
    }

    // Both compares are of the max corners, so only m_max decides, as the game does.
    int32_t equals(const hkAabb& aabb) const {
        hkVector4Comparison mi;
        mi.m_mask = _mm_cmpeq_ps(aabb.m_max.m_quad, m_max.m_quad);
        hkVector4Comparison ma;
        ma.m_mask = _mm_cmpeq_ps(aabb.m_max.m_quad, m_max.m_quad);
        hkVector4Comparison co;
        co.setAnd(mi, ma);
        return co.allAreSet(hkVector4Comparison::MASK_XYZ);
    }

    hkBool* isValid(hkBool* apResult);

    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_min;
    hkVector4 m_max;
};

// An axis aligned bounding box in integer space, as the broadphase and hk1AxisSweep sort them; the expansion bytes and
// m_shapeKeyByte fill the words after m_min and m_max. 16-byte aligned.
struct alignas(16) hkAabbUint32 {
    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    uint32_t m_min[3];
    uint8_t m_expansionMin[3];
    uint8_t m_expansionShift;
    uint32_t m_max[3];
    uint8_t m_expansionMax[3];
    uint8_t m_shapeKeyByte;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkAabbTypeInfo;
extern hkTypeInfo hkAabbUint32TypeInfo;
extern hkClass hkAabbClass;
extern hkClass hkAabbUint32Class;

// hkVector4Util::hkAabbUint32MaxVal is a static member of the hkVector4Util class: see hkVector4Util.h.
