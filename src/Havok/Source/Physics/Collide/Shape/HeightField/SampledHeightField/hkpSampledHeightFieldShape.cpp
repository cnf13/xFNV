#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/CompressedSampledHeightField/hkpCompressedSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/Plane/hkpPlaneShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/StorageSampledHeightField/hkpStorageSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

namespace {
    // Each member's offset in this struct (-1: no default), then heightfieldType's default (its hkInt8 storage).
    struct hkpSampledHeightFieldShape_DefaultStruct {
        int32_t s_defaultOffsets[9];
        int8_t m_heightfieldType;
    };
} // namespace

// The reflection's enum records: the names of HeightFieldType's values, and the enum.
using hkpSampledHeightFieldShapeHeightFieldTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpSampledHeightFieldShapeHeightFieldTypeEnumItemsArray hkpSampledHeightFieldShapeHeightFieldTypeEnumItems{
    {hkpSampledHeightFieldShape::HEIGHTFIELD_STORAGE, "HEIGHTFIELD_STORAGE"},
    {hkpSampledHeightFieldShape::HEIGHTFIELD_COMPRESSED, "HEIGHTFIELD_COMPRESSED"},
    {hkpSampledHeightFieldShape::HEIGHTFIELD_USER, "HEIGHTFIELD_USER"},
    {hkpSampledHeightFieldShape::HEIGHTFIELD_MAX_ID, "HEIGHTFIELD_MAX_ID"}
};
using hkpSampledHeightFieldShapeEnumsArray = const hkClassEnum[1];
static hkpSampledHeightFieldShapeEnumsArray hkpSampledHeightFieldShapeEnums{
    {"HeightFieldType", hkpSampledHeightFieldShapeHeightFieldTypeEnumItems, 4, nullptr, {0}}
};

// The enum record the heightfieldType member names.
static const hkClassEnum* hkpSampledHeightFieldShapeHeightFieldTypeEnum = &hkpSampledHeightFieldShapeEnums[0];

// The defaults the class record names.
static const hkpSampledHeightFieldShape_DefaultStruct hkpSampledHeightFieldShape_Default{
    {-1, -1, -1, -1, 0x24, -1, -1, -1, -1},
    hkpSampledHeightFieldShape::HEIGHTFIELD_USER
};

// The reflected members of hkpSampledHeightFieldShape. The heightfieldType member's entry holds
// hkpSampledHeightFieldShapeHeightFieldTypeEnum itself instead of having it stored by an initialiser.
using hkpSampledHeightFieldShapeClass_MembersArray = hkClassMember[9];
static hkpSampledHeightFieldShapeClass_MembersArray hkpSampledHeightFieldShapeClass_Members{
    {"xRes", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"zRes", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"heightCenter", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
    {"useProjectionBasedHeight",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr},
    {"heightfieldType",
     nullptr,
     &hkpSampledHeightFieldShapeEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x1D,
     nullptr},
    {"intToFloatScale",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"floatToIntScale",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"floatToIntOffsetFloorCorrected",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     nullptr},
    {"extents", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr}
};
hkClass hkpSampledHeightFieldShapeClass(
    "hkpSampledHeightFieldShape",
    &hkpHeightFieldShapeClass,
    0x60,
    nullptr,
    0,
    hkpSampledHeightFieldShapeEnums,
    1,
    hkpSampledHeightFieldShapeClass_Members,
    9,
    &hkpSampledHeightFieldShape_Default,
    nullptr,
    0,
    0
);

namespace {

    // The sampled height field's ray collector that keeps the nearest hit: a file-local struct with m_hasHit (hkBool)
    // and m_output (hkpShapeRayCastOutput&).
    struct NearestHitCollector : public hkpRayHitCollector {
        NearestHitCollector(hkpShapeRayCastOutput& arOutput) : m_output(arOutput) {
            m_earlyOutHitFraction = arOutput.m_hitFraction;
            m_hasHit = false;
        }

        // When the hit's fraction is below m_earlyOutHitFraction, it becomes the early-out fraction, the hit is copied
        // into m_output, the invalid shape key is appended and m_hasHit is set.
        void addRayHit(const hkpCdBody& arCdBody, const hkpShapeRayCastCollectorOutput& arHitInfo) override;

        // +0x8: whether a hit was kept.
        hkBool m_hasHit;
        // +0xC: where the kept hit is written.
        hkpShapeRayCastOutput& m_output;
    };

    // The sampled height field's ray collector that rotates each hit's normal into the body's frame and forwards the
    // hit to m_collector; a file-local struct.
    struct RotateNormalHitCollector : public hkpRayHitCollector {
        RotateNormalHitCollector(hkpRayHitCollector& arCollector) : m_collector(arCollector) {
            m_earlyOutHitFraction = arCollector.m_earlyOutHitFraction;
        }

        // Copies the hit, rotates its normal by the three rows of the transform the body holds, and hands the copy to
        // m_collector's addRayHit.
        void addRayHit(const hkpCdBody& arCdBody, const hkpShapeRayCastCollectorOutput& arHitInfo) override;

        // +0x8: the collector the rotated hits go to.
        hkpRayHitCollector& m_collector;
    };

    // The SDK's body: a hit below the early-out fraction becomes the early-out fraction and the kept hit, under the
    // invalid shape key.
    void NearestHitCollector::addRayHit(const hkpCdBody& arCdBody, const hkpShapeRayCastCollectorOutput& arHitInfo) {
        if (arHitInfo.m_hitFraction < m_earlyOutHitFraction) {
            m_earlyOutHitFraction = arHitInfo.m_hitFraction;
            // EXE-INLINED 0x9C1B00
            m_output.m_normal.m_quad = arHitInfo.m_normal.m_quad;
            m_output.m_hitFraction = arHitInfo.m_hitFraction;
            m_output.m_extraInfo = arHitInfo.m_extraInfo;
            // EXE-INLINED none: hkpShapeRayCastOutput::setKey
            m_output.m_shapeKeys[m_output.m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
            m_hasHit = true;
        }
    }

    // The hit, copied whole, with its normal turned by the body's rotation, goes to the wrapped collector.
    void
    RotateNormalHitCollector::addRayHit(const hkpCdBody& arCdBody, const hkpShapeRayCastCollectorOutput& arHitInfo) {
        hkpShapeRayCastCollectorOutput kNewInfo = arHitInfo;
        const hkRotation& kRotation = arCdBody.m_motion.pMotion->m_rotation;
        const hkQuadReal kDir = arHitInfo.m_normal.m_quad;
        // EXE-INLINED 0xC7F700
        kNewInfo.m_normal.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, _MM_SHUFFLE(1, 1, 1, 1)), kRotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, _MM_SHUFFLE(0, 0, 0, 0)), kRotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, _MM_SHUFFLE(2, 2, 2, 2)), kRotation.m_col2.m_quad)
        );
        m_collector.addRayHit(arCdBody, kNewInfo);
    }

} // namespace

hkpSampledHeightFieldShape::hkpSampledHeightFieldShape() : hkpHeightFieldShape(HK_SHAPE_SAMPLED_HEIGHT_FIELD) {}

// The extents are the grid's size times the sample scale; a height range with max below min is not known yet (height
// extent and centre -1). The float-to-int scale is the scale's reciprocal, the offset the floor correction times the
// scale with y zero.
hkpSampledHeightFieldShape::hkpSampledHeightFieldShape(
    const hkpSampledHeightFieldBaseCinfo& ci,
    hkpSampledHeightFieldShape::HeightFieldType hfType
)
  : hkpHeightFieldShape(HK_SHAPE_SAMPLED_HEIGHT_FIELD), m_xRes(ci.m_xRes), m_zRes(ci.m_zRes),
    m_useProjectionBasedHeight(ci.m_useProjectionBasedHeight) {
    m_heightfieldType.m_storage = hfType;
    m_extents.set(
        static_cast<float>(static_cast<double>(ci.m_xRes) - 1.0), ci.m_maxHeight - ci.m_minHeight,
        static_cast<float>(static_cast<double>(ci.m_zRes) - 1.0), 0.0f
    );
    m_extents.m_quad = _mm_mul_ps(m_extents.m_quad, ci.m_scale.m_quad);
    if (!(ci.m_maxHeight < static_cast<double>(ci.m_minHeight))) {
        m_heightCenter = (ci.m_maxHeight + ci.m_minHeight) * 0.5 * ci.m_scale.m_quad[1];
    }
    else {
        m_heightCenter = -1.0f;
        m_extents.m_quad[1] = -1.0f;
    }
    m_intToFloatScale = ci.m_scale;
    m_floatToIntScale.set(
        static_cast<float>(1.0 / ci.m_scale.m_quad[0]), static_cast<float>(1.0 / ci.m_scale.m_quad[1]),
        static_cast<float>(1.0 / ci.m_scale.m_quad[2]), 0.0f
    );
    float fFloorCorrection = hkVector4Util::getFloatToInt16FloorCorrection();
    m_floatToIntOffsetFloorCorrected.m_quad = _mm_mul_ps(_mm_set1_ps(fFloorCorrection), m_intToFloatScale.m_quad);
    m_floatToIntOffsetFloorCorrected.m_quad[1] = 0.0f;
}

hkpSampledHeightFieldShape::~hkpSampledHeightFieldShape() {}

// A negative height extent means the height range is not known yet: the first query scans every sample, and the SDK
// stores the range into the shape through a const_cast of this.
void hkpSampledHeightFieldShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 center;
    hkVector4 halfExtents;
    float minHeight = getHeightAtImpl(0, 0);
    float maxHeight = minHeight;
    if (m_extents.m_quad[1] < 0.0f) {
        for (int32_t i = 0; i < m_xRes; ++i) {
            for (int32_t j = 0; j < m_zRes; ++j) {
                float height = getHeightAtImpl(i, j);
                if (!(height > static_cast<double>(minHeight))) {
                    minHeight = height;
                }
                if (!(maxHeight > static_cast<double>(height))) {
                    maxHeight = height;
                }
            }
        }
        float scaledMin = m_intToFloatScale.m_quad[1] * minHeight;
        float scaledMax = m_intToFloatScale.m_quad[1] * maxHeight;
        double lo = scaledMin;
        double hi = scaledMax;
        if (!(scaledMax > static_cast<double>(scaledMin))) {
            lo = scaledMax;
        }
        if (scaledMin > hi) {
            hi = scaledMin;
        }
        hkpSampledHeightFieldShape* self = const_cast<hkpSampledHeightFieldShape*>(this);
        self->m_heightCenter = (hi + lo) * 0.5;
        self->m_extents.m_quad[1] = hi - lo;
    }
    float heightCenter = m_heightCenter;
    __m128 m_quad = _mm_mul_ps(m_extents.m_quad, hkQuadRealHalf);
    center.m_quad = m_quad;
    center.m_quad[1] = heightCenter;
    halfExtents.m_quad = m_quad;
    hkAabbUtil::calcAabb(localToWorld, halfExtents, center, tolerance, out);
}

// Collects into a NearestHitCollector over the output, and walks the ray with no body, as `*(hkpCdBody*)HK_NULL`.
hkBool
hkpSampledHeightFieldShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    NearestHitCollector collector(results);
    castRayInternal(input, *static_cast<const hkpCdBody*>(nullptr), false, 0.0f, collector);
    return collector.m_hasHit;
}

// Wraps the caller's collector in a RotateNormalHitCollector and walks the ray against the body.
void hkpSampledHeightFieldShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    RotateNormalHitCollector myCollector(collector);
    castRayInternal(input, cdBody, false, 0.0f, myCollector);
}

void hkpSampledHeightFieldShape::castSphere(
    const hkpHeightFieldShape::hkpSphereCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    hkpShapeRayCastInput v6;
    float m_radius = input.m_radius;
    bool v4 = m_intToFloatScale.m_quad[1] > 0.0f;
    // The ray input is copied bytewise, padding included; it holds only plain vectors and a filter pointer, so this is
    // harmless.
    __builtin_memcpy(static_cast<void*>(&v6), &input, sizeof(v6));
    if (v4) {
        m_radius = -m_radius;
    }
    v6.m_from.m_quad[1] = v6.m_from.m_quad[1] + m_radius;
    v6.m_to.m_quad[1] = m_radius + v6.m_to.m_quad[1];
    castRayInternal(v6, cdBody, static_cast<hkBool>(true), input.m_maxExtraPenetration, collector);
}

// The cell's two triangles are split along the diagonal the triangle flip picks. Reads the four corner heights it needs
// through the getHeightAtImpl virtual, keeps each height and each slope as a float, and sets the normal's four lanes to
// 1 first, then overwrites x and z with the negated slopes. The comparisons send NaN to the second triangle. `subX -
// 1.0` subtracts the double 1.0.
float* hkpSampledHeightFieldShape::getHeightAndNormalAt(
    int32_t xPos,
    int32_t zPos,
    float subX,
    float subZ,
    hkVector4& normalOut,
    float& heightOut,
    int32_t& triangleIndexOut
) const {
    _mm_store_ps(&reinterpret_cast<float*>(&normalOut.m_quad)[0], _mm_set1_ps(1.0f));
    hkBool kFlip;
    getTriangleFlipImpl(&kFlip);
    if (kFlip.m_bool) {
        const float h00 = getHeightAtImpl(xPos, zPos);
        const float h11 = getHeightAtImpl(xPos + 1, zPos + 1);
        if (subZ < subX) {
            const float h10 = getHeightAtImpl(xPos + 1, zPos);
            const float fSlopeX = h10 - h00;
            const float fSlopeZ = h11 - h10;
            heightOut = h00 + fSlopeZ * subZ + fSlopeX * subX;
            normalOut.m_quad[0] = -fSlopeX;
            normalOut.m_quad[2] = -fSlopeZ;
            triangleIndexOut = 1;
        }
        else {
            const float h01 = getHeightAtImpl(xPos, zPos + 1);
            const float fSlopeX = h11 - h01;
            const float fSlopeZ = h01 - h00;
            heightOut = h00 + fSlopeZ * subZ + fSlopeX * subX;
            normalOut.m_quad[0] = -fSlopeX;
            normalOut.m_quad[2] = -fSlopeZ;
            triangleIndexOut = 0;
        }
    }
    else {
        const float h10 = getHeightAtImpl(xPos + 1, zPos);
        const float h01 = getHeightAtImpl(xPos, zPos + 1);
        if (subX + subZ > 1.0) {
            const float h11 = getHeightAtImpl(xPos + 1, zPos + 1);
            const float fSlopeX = h11 - h01;
            const float fSlopeZ = h11 - h10;
            heightOut = h10 + fSlopeZ * subZ + (subX - 1.0) * fSlopeX;
            normalOut.m_quad[0] = -fSlopeX;
            normalOut.m_quad[2] = -fSlopeZ;
            triangleIndexOut = 1;
        }
        else {
            const float h00 = getHeightAtImpl(xPos, zPos);
            const float fSlopeX = h10 - h00;
            const float fSlopeZ = h01 - h00;
            heightOut = h00 + fSlopeZ * subZ + fSlopeX * subX;
            normalOut.m_quad[0] = -fSlopeX;
            normalOut.m_quad[2] = -fSlopeZ;
            triangleIndexOut = 0;
        }
    }
    return &heightOut;
}

// The ray is walked in grid space (the cast's points times m_floatToIntScale) with y replaced by the diagonal
// coordinate (x - z where the cells are split the other way, else x + z) and w keeping the ray's height. A ray whose
// direction is degenerate in two of the three axes is tested against its start's cell alone.
void hkpSampledHeightFieldShape::castRayInternal(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkBool reportPenetratingStartPosition,
    float maxExtraPenetration,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcHeightFild", nullptr);

    // The start's cell, computed as hkVector4Util::convertToUint16 does; only x and z are read.
    const hkQuadReal startInt = _mm_add_ps(
        _mm_mul_ps(_mm_add_ps(m_floatToIntOffsetFloorCorrected.m_quad, input.m_from.m_quad), m_floatToIntScale.m_quad),
        hkVector4Util_floatToUint16Magic
    );
    int32_t cell[3];
    cell[0] = static_cast<int16_t>(std::bit_cast<uint32_t>(startInt[0]) >> 6);
    cell[2] = static_cast<int16_t>(std::bit_cast<uint32_t>(startInt[2]) >> 6);
    hkVector4 rayFrom;
    rayFrom.m_quad = _mm_mul_ps(m_floatToIntScale.m_quad, input.m_from.m_quad);
    hkVector4 rayTo;
    rayTo.m_quad = _mm_mul_ps(input.m_to.m_quad, m_floatToIntScale.m_quad);
    const hkBool flip = getTriangleFlip();
    rayFrom(3) = rayFrom(1);
    rayTo(3) = rayTo(1);
    if (flip.m_bool) {
        rayFrom(1) = rayFrom(0) - rayFrom(2);
        rayTo(1) = rayTo(0) - rayTo(2);
    }
    else {
        rayFrom(1) = rayFrom(0) + rayFrom(2);
        rayTo(1) = rayTo(0) + rayTo(2);
    }

    // Per axis (x, diagonal, z): the step between cells, the ray fraction between two grid lines and the fraction of
    // the next grid line.
    int32_t step[3];
    float tDelta[3];
    float tNext[3];
    hkVector4 invDir;
    float dirX = rayTo(0) - rayFrom(0);
    float absDirX = std::fabs(dirX);
    if (absDirX < 3.0517578e-05f) {
        step[0] = -1;
        tDelta[0] = 0.0f;
        tNext[0] = 3.40282e+38f;
    }
    else {
        float inv = 1.0f / dirX;
        invDir(0) = inv;
        if (dirX < 0.0f) {
            step[0] = -1;
            tDelta[0] = -inv;
        }
        else {
            cell[0]++;
            tDelta[0] = inv;
            step[0] = 1;
        }
        tNext[0] = (static_cast<double>(cell[0]) - rayFrom(0)) * inv;
    }
    float dirDiag = rayTo(1) - rayFrom(1);
    float absDirDiag = std::fabs(dirDiag);
    if (absDirDiag < 3.0517578e-05f) {
        step[1] = -1;
        tDelta[1] = 0.0f;
        tNext[1] = 3.40282e+38f;
    }
    else {
        float inv = 1.0f / dirDiag;
        if (dirDiag < 0.0f) {
            step[1] = -1;
            tDelta[1] = -inv;
        }
        else {
            tDelta[1] = inv;
            step[1] = 1;
        }
    }
    float dirZ = rayTo(2) - rayFrom(2);
    float absDirZ = std::fabs(dirZ);
    if (absDirZ < 3.0517578e-05f) {
        step[2] = -1;
        tDelta[2] = 0.0f;
        tNext[2] = 3.40282e+38f;
    }
    else {
        float inv = 1.0f / dirZ;
        invDir(2) = inv;
        if (dirZ < 0.0f) {
            step[2] = -1;
            tDelta[2] = -inv;
        }
        else {
            cell[2]++;
            tDelta[2] = inv;
            step[2] = 1;
        }
        tNext[2] = (static_cast<double>(cell[2]) - rayFrom(2)) * inv;
    }

    if (tDelta[2] + tDelta[0] == 0.0f || tDelta[1] + tDelta[0] == 0.0f || tDelta[1] + tDelta[2] == 0.0f) {
        // A ray degenerate in two axes: the surface of the start's cell only.
        if (static_cast<uint32_t>(cell[0]) >= static_cast<uint32_t>(m_xRes - 1) ||
            static_cast<uint32_t>(cell[2]) >= static_cast<uint32_t>(m_zRes - 1)) {
            goto end;
        }
        XFNV_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
        // EXE-INLINED 0x4A3D20
        output.m_hitFraction = 1.0f;
        output.m_extraInfo = -1;
        output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
        output.m_shapeKeyIndex = 0;
        // EXE-INLINED 0x4589C0
        output.m_normal.m_quad = _mm_set_ps1(1.0f);
        float subX = rayFrom(0) - static_cast<double>(cell[0]);
        float subZ = rayFrom(2) - static_cast<double>(cell[2]);
        const hkBool cellFlip = getTriangleFlip();
        int32_t triangleIndex;
        float dhdx;
        float dhdz;
        float height;
        if (cellFlip.m_bool) {
            float h00 = getHeightAt(cell[0], cell[2]);
            float h11 = getHeightAt(cell[0] + 1, cell[2] + 1);
            if (subZ < subX) {
                float h10 = getHeightAt(cell[0] + 1, cell[2]);
                triangleIndex = 1;
                dhdx = h10 - h00;
                dhdz = h11 - h10;
            }
            else {
                float h01 = getHeightAt(cell[0], cell[2] + 1);
                dhdx = h11 - h01;
                dhdz = h01 - h00;
                triangleIndex = 0;
            }
            height = h00 + dhdz * subZ + dhdx * subX;
        }
        else {
            float h10 = getHeightAt(cell[0] + 1, cell[2]);
            float h01 = getHeightAt(cell[0], cell[2] + 1);
            if (subX + subZ > 1.0) {
                float h11 = getHeightAt(cell[0] + 1, cell[2] + 1);
                triangleIndex = 1;
                dhdx = h11 - h01;
                dhdz = h11 - h10;
                height = h10 + dhdz * subZ + (subX - 1.0) * dhdx;
            }
            else {
                float h00 = getHeightAt(cell[0], cell[2]);
                dhdx = h10 - h00;
                dhdz = h01 - h00;
                triangleIndex = 0;
                height = h00 + dhdz * subZ + dhdx * subX;
            }
        }
        output.m_normal(0) = -dhdx;
        output.m_normal(2) = -dhdz;
        float startDist = rayFrom(3) - height;
        float endDist = rayTo(3) - height;
        if (startDist < endDist) {
            goto end;
        }
        float hitFraction;
        if (reportPenetratingStartPosition.m_bool && startDist < 0.0f) {
            if (endDist > startDist - maxExtraPenetration) {
                goto end;
            }
            hitFraction = 0.0f;
        }
        else {
            if (startDist < 0.0f || endDist >= 0.0f) {
                goto end;
            }
            hitFraction = startDist / (startDist - endDist);
        }
        if (collector.m_earlyOutHitFraction > hitFraction) {
            output.m_hitFraction = hitFraction;
            // EXE-INLINED 0x62AEA0: src's normalize3 builds its results through hkSimdReal::Construct (0x4A3C90),
            // which the exe's inlined copy does not call.
            const hkQuadReal n = _mm_mul_ps(output.m_normal.m_quad, m_floatToIntScale.m_quad);
            const hkQuadReal n2 = _mm_mul_ps(n, n);
            const hkQuadReal len2 = _mm_add_ps(
                _mm_shuffle_ps(n2, n2, _MM_SHUFFLE(2, 2, 2, 2)),
                _mm_add_ps(_mm_shuffle_ps(n2, n2, _MM_SHUFFLE(1, 1, 1, 1)), n2)
            );
            const hkQuadReal approx = _mm_rsqrt_ss(len2);
            const hkQuadReal isZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
            const hkQuadReal lenInv = _mm_andnot_ps(
                isZero,
                _mm_mul_ps(
                    _mm_mul_ps(_mm_set_ss(0.5f), approx),
                    _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(len2, approx), approx))
                )
            );
            output.m_extraInfo = triangleIndex + 2 * (cell[0] + (cell[2] << 15));
            output.m_normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), n);
            collector.addRayHit(cdBody, output);
        }
    }
    else {
        hkVector4 pos = rayFrom;
        const int32_t startX = cell[0];
        float clipT;

        // A start outside the grid in x: the ray from where it enters, z's cell found from there.
        bool clipped = false;
        if (step[0] > 0) {
            if (cell[0] <= 0) {
                if (rayTo(0) < 0.0f) {
                    goto end;
                }
                clipT = -rayFrom(0) * tDelta[0];
                cell[0] = 1;
                clipped = true;
            }
        }
        else if (cell[0] > m_xRes - 2) {
            float maxX = static_cast<float>(m_xRes - 1);
            if (maxX < rayTo(0)) {
                goto end;
            }
            clipT = (rayFrom(0) - maxX) * tDelta[0];
            cell[0] = m_xRes - 2;
            clipped = true;
        }
        if (clipped) {
            const hkQuadReal t = _mm_set_ps1(clipT);
            tNext[0] = clipT + tDelta[0];
            // EXE-INLINED 0x5DBF70
            pos.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), rayFrom.m_quad), _mm_mul_ps(rayTo.m_quad, t));
            cell[2] = cell[2] + step[2] * hkMath::hkToIntFast(clipT * absDirZ) - 2;
            while (static_cast<double>(cell[2]) <= pos(2)) {
                cell[2]++;
            }
            cell[2] += step[2] >> 1;
            if (tDelta[2] != 0.0f) {
                tNext[2] = (static_cast<double>(cell[2]) - rayFrom(2)) * invDir(2);
            }
        }

        // The same in z; x's cell is found again from the start's.
        clipped = false;
        if (step[2] > 0) {
            if (cell[2] <= 0) {
                if (rayTo(2) < 0.0f) {
                    goto end;
                }
                clipT = -rayFrom(2) * tDelta[2];
                cell[2] = 1;
                clipped = true;
            }
        }
        else if (cell[2] > m_zRes - 2) {
            float maxZ = static_cast<float>(m_zRes - 1);
            if (maxZ < rayTo(2)) {
                goto end;
            }
            clipT = (rayFrom(2) - maxZ) * tDelta[2];
            cell[2] = m_zRes - 2;
            clipped = true;
        }
        if (clipped) {
            const hkQuadReal t = _mm_set_ps1(clipT);
            tNext[2] = clipT + tDelta[2];
            // EXE-INLINED 0x5DBF70
            pos.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), rayFrom.m_quad), _mm_mul_ps(rayTo.m_quad, t));
            cell[0] = startX + step[0] * hkMath::hkToIntFast(clipT * absDirX) - 2;
            while (static_cast<double>(cell[0]) <= pos(0)) {
                cell[0]++;
            }
            cell[0] += step[0] >> 1;
            if (tDelta[0] != 0.0f) {
                tNext[0] = (static_cast<double>(cell[0]) - rayFrom(0)) * invDir(0);
            }
        }

        if (static_cast<uint32_t>(cell[0]) >= static_cast<uint32_t>(m_xRes) ||
            static_cast<uint32_t>(cell[0] - step[0]) >= static_cast<uint32_t>(m_xRes)) {
            goto end;
        }

        // Whether the ray crosses the diagonals the cells are split along (their direction against the ray's), and
        // the diagonal cell; diagonalPending is 2 while the next diagonal crossing is not due.
        bool crossesDiagonals = (((step[2] ^ step[0]) >= 0) ^ (flip.m_bool != 0)) != 0;
        if (tDelta[1] == 0.0f) {
            crossesDiagonals = false;
        }
        int32_t diagonalPending = 2;
        if (flip.m_bool) {
            cell[1] = cell[0] - cell[2];
        }
        else {
            cell[1] = cell[0] + cell[2];
        }
        float diagOffset = pos(1) - static_cast<double>(cell[1]);
        if (crossesDiagonals) {
            diagOffset = std::fabs(diagOffset);
            if (diagOffset > 1.0f) {
                cell[1] -= step[1];
            }
            else {
                diagonalPending = 0;
            }
        }
        else {
            diagOffset = static_cast<double>(step[1]) * diagOffset;
            if (diagOffset > 0.0f) {
                diagonalPending = 0;
                cell[1] += step[1];
            }
        }
        if (tDelta[1] == 0.0f) {
            tNext[1] = 3.40282e+38f;
        }
        else {
            tNext[1] = (static_cast<double>(cell[1]) - rayFrom(1)) * tDelta[1] * static_cast<double>(step[1]);
        }

        // The axis whose grid line the start lies past last; the walk steps back over it, so that its first step
        // is the start's own cell.
        float tPrev[3];
        tPrev[0] = (tDelta[0] == 0.0f) ? -3.40282e+38f : tNext[0] - tDelta[0];
        tPrev[1] = (tDelta[1] == 0.0f) ? -3.40282e+38f : tNext[1] - tDelta[1];
        tPrev[2] = (tDelta[2] == 0.0f) ? -3.40282e+38f : tNext[2] - tDelta[2];
        int32_t axis;
        if (crossesDiagonals) {
            if (diagonalPending == 0) {
                axis = 1;
            }
            else {
                axis = (tPrev[2] < tPrev[0]) ? 0 : 2;
            }
        }
        else if (absDirZ > absDirX) {
            if (diagonalPending != 0) {
                axis = 2;
            }
            else {
                axis = (tPrev[1] < tPrev[0]) ? 0 : 1;
            }
        }
        else if (diagonalPending != 0) {
            axis = 0;
        }
        else {
            axis = (tPrev[2] < tPrev[1]) ? 1 : 2;
        }
        tNext[axis] = tNext[axis] - tDelta[axis];
        cell[axis] -= step[axis];

        // The surface and ray heights and the ray fraction at the last grid line crossed.
        float lastSurfaceHeight = 3.40282e+38f;
        float lastRayHeight = 0.0f;
        float lastT = -1.0f;
        if (static_cast<uint32_t>(cell[0]) >= static_cast<uint32_t>(m_xRes)) {
            goto end;
        }
        for (;;) {
            if (static_cast<uint32_t>(cell[2]) >= static_cast<uint32_t>(m_zRes)) {
                goto end;
            }
            if (collector.m_earlyOutHitFraction < lastT) {
                goto end;
            }
            float t = tNext[axis];
            {
                const hkQuadReal tq = _mm_set_ps1(t);
                // EXE-INLINED 0x5DBF70
                pos.m_quad = _mm_add_ps(
                    _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tq), rayFrom.m_quad), _mm_mul_ps(rayTo.m_quad, tq)
                );
            }
            const int32_t prevX = cell[0] - step[0];

            // The surface's height where the ray crosses the grid line: between the line's two grid points.
            float h;
            if (axis == 0) {
                float hA = getHeightAt(cell[0], cell[2] - step[2]);
                float hB = getHeightAt(cell[0], cell[2]);
                float f = std::fabs(pos(2) - static_cast<double>(cell[2]));
                h = hB + (hA - hB) * f;
            }
            else if (axis == 2) {
                float hA = getHeightAt(prevX, cell[2]);
                float hB = getHeightAt(cell[0], cell[2]);
                float f = std::fabs(pos(0) - static_cast<double>(cell[0]));
                h = hB + (hA - hB) * f;
            }
            else {
                float f = std::fabs(pos(0) - static_cast<double>(cell[0]));
                int32_t zA = cell[2];
                int32_t zB = cell[2];
                if (crossesDiagonals) {
                    zB = cell[2] - step[2];
                }
                else {
                    zA = cell[2] - step[2];
                }
                float hA = getHeightAt(prevX, zA);
                float hB = getHeightAt(cell[0], zB);
                h = hB + (hA - hB) * f;
            }
            float rayHeight = pos(3);
            float hitT;
            if (reportPenetratingStartPosition.m_bool) {
                if (h <= rayHeight) {
                    goto noHit;
                }
                {
                    float lastDist = lastRayHeight - lastSurfaceHeight;
                    float dist = rayHeight - h;
                    float dt = t - lastT;
                    if (dist >= lastDist - dt * maxExtraPenetration) {
                        goto noHit;
                    }
                    if (lastDist <= 0.0f) {
                        hitT = hkMath::max2<float>(0.0f, lastT);
                        goto haveHitT;
                    }
                }
            }
            else {
                if (h <= rayHeight) {
                    goto noHit;
                }
                if (lastSurfaceHeight > lastRayHeight) {
                    goto noHit;
                }
            }
            {
                // Where the ray went below the surface: between the last line and this one, or, before the first
                // line, from the surface extrapolated back to the ray's start.
                float dist = rayHeight - h;
                if (lastT >= 0.0f) {
                    float lastDist = lastRayHeight - lastSurfaceHeight;
                    float frac = lastDist / (lastDist - dist);
                    hitT = lastT + frac * (t - lastT);
                }
                else {
                    float r = t / (t - lastT);
                    float startHeight = r * lastSurfaceHeight + (1.0f - r) * h;
                    float startDist = rayFrom(3) - startHeight;
                    if (startDist < 0.0f) {
                        if (!reportPenetratingStartPosition.m_bool) {
                            goto noHit;
                        }
                        startDist = 0.0f;
                    }
                    float frac = startDist / (startDist - dist);
                    hitT = frac * t;
                }
            }
        haveHitT:
            lastSurfaceHeight = h;
            lastRayHeight = pos(3);
            lastT = t;
            if (collector.m_earlyOutHitFraction < hitT) {
                goto end;
            }
            {
                XFNV_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
                // EXE-INLINED 0x4A3D20
                output.m_hitFraction = 1.0f;
                output.m_normal(3) = 0.0f;
                output.m_normal(1) = 1.0f;
                output.m_extraInfo = -1;
                output.m_shapeKeyIndex = 0;
                output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
                const int32_t prevZ = cell[2] - step[2];
                // The normal of the cell's triangle behind the line, from three of its grid points.
                float normalX;
                float normalZ;
                int32_t triangleIndex;
                if (crossesDiagonals) {
                    float hP = getHeightAt(cell[0], prevZ);
                    float hQ = getHeightAt(prevX, cell[2]);
                    if (axis == 1) {
                        float hR = getHeightAt(prevX, prevZ);
                        normalX = hR - hP;
                        normalZ = hR - hQ;
                        triangleIndex = prevZ > cell[2];
                    }
                    else {
                        float hR = getHeightAt(cell[0], cell[2]);
                        triangleIndex = cell[2] > prevZ;
                        normalX = hQ - hR;
                        normalZ = hP - hR;
                    }
                }
                else {
                    float hS = getHeightAt(prevX, prevZ);
                    float hT = getHeightAt(cell[0], cell[2]);
                    if (axis == 0 || (axis == 1 && absDirZ > absDirX)) {
                        float hU = getHeightAt(cell[0], prevZ);
                        normalX = hS - hU;
                        normalZ = hU - hT;
                        triangleIndex = prevZ > cell[2];
                    }
                    else {
                        float hU = getHeightAt(prevX, cell[2]);
                        triangleIndex = cell[2] > prevZ;
                        normalX = hU - hT;
                        normalZ = hS - hU;
                    }
                }
                triangleIndex ^= (flip.m_bool != 0);
                output.m_normal(0) = static_cast<double>(step[0]) * normalX;
                output.m_normal(2) = static_cast<double>(step[2]) * normalZ;
                output.m_hitFraction = hitT;
                // EXE-INLINED 0x62AEA0: src's normalize3 builds its results through hkSimdReal::Construct
                // (0x4A3C90), which the exe's inlined copy does not call.
                const hkQuadReal n = _mm_mul_ps(m_floatToIntScale.m_quad, output.m_normal.m_quad);
                const hkQuadReal n2 = _mm_mul_ps(n, n);
                const hkQuadReal len2 = _mm_add_ps(
                    _mm_shuffle_ps(n2, n2, _MM_SHUFFLE(2, 2, 2, 2)),
                    _mm_add_ps(_mm_shuffle_ps(n2, n2, _MM_SHUFFLE(1, 1, 1, 1)), n2)
                );
                const hkQuadReal approx = _mm_rsqrt_ss(len2);
                const hkQuadReal isZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
                const hkQuadReal lenInv = _mm_andnot_ps(
                    isZero,
                    _mm_mul_ps(
                        _mm_mul_ps(_mm_set_ss(0.5f), approx),
                        _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(len2, approx), approx))
                    )
                );
                // The key of the cell behind the line: its x in the low 15 bits, its z above, times two plus the
                // triangle.
                const int32_t hitX = cell[0] - (step[0] >> 1) - 1;
                const int32_t hitZ = cell[2] - (step[2] >> 1) - 1;
                output.m_extraInfo = triangleIndex + 2 * (hitX + (hitZ << 15));
                output.m_normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), n);
                collector.addRayHit(cdBody, output);
            }
            if (reportPenetratingStartPosition.m_bool) {
                reportPenetratingStartPosition.m_bool = 0;
            }
            goto advance;
        noHit:
            lastSurfaceHeight = h;
            lastRayHeight = pos(3);
            lastT = t;
        advance:
            // Past the line: the next line of this axis, then the nearest of the next lines.
            cell[axis] += step[axis];
            tNext[axis] = tDelta[axis] + tNext[axis];
            diagonalPending ^= 2;
            if (crossesDiagonals) {
                if (diagonalPending == 0) {
                    axis = 1;
                }
                else {
                    axis = (tNext[2] > tNext[0]) ? 0 : 2;
                }
            }
            else if (absDirZ > absDirX) {
                if (diagonalPending != 0) {
                    axis = 2;
                }
                else {
                    axis = (tNext[1] > tNext[0]) ? 0 : 1;
                }
            }
            else if (diagonalPending != 0) {
                axis = 0;
            }
            else {
                axis = (tNext[2] > tNext[1]) ? 1 : 2;
            }
            if (static_cast<uint32_t>(cell[0]) >= static_cast<uint32_t>(m_xRes)) {
                goto end;
            }
        }
    }
end:
    HK_TIMER_END();
}

// The destructor has nothing to free.
hkpHeightFieldShape::~hkpHeightFieldShape() {}
