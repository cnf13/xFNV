#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <cmath>

// The reflected members of hkpMultiSphereShape.
using hkpMultiSphereShapeClass_MembersArray = const hkClassMember[2];
static hkpMultiSphereShapeClass_MembersArray hkpMultiSphereShapeClass_Members{
    {"numSpheres", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"spheres", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 8, {0}, 0x20, nullptr}
};

hkClass hkpMultiSphereShapeClass(
    "hkpMultiSphereShape",
    &hkpSphereRepShapeClass,
    0xA0,
    nullptr,
    0,
    nullptr,
    0,
    hkpMultiSphereShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMultiSphereShapeTypeInfo = {
    "hkpMultiSphereShape",          finishLoadedObjecthkpMultiSphereShape, cleanupLoadedObjecthkpMultiSphereShape,
    getVtablehkpMultiSphereShape(), &typeid(hkpMultiSphereShape),          sizeof(hkpMultiSphereShape)
};

void finishLoadedObjecthkpMultiSphereShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpMultiSphereShape(kFlag);
    }
}

int32_t findClosestHit(
    const hkVector4* apSpheres,
    const float* apHitFractions,
    const int32_t* apSphereIndices,
    int32_t aiNumHits,
    const hkpShapeRayCastInput* apInput,
    hkpShapeRayCastOutput* apOutput
) {
    float fBest = apOutput->m_hitFraction;
    int32_t iBest = -1;
    int32_t i = 0;
    if (aiNumHits >= 4) {
        do {
            if (fBest > apHitFractions[i]) {
                iBest = i;
                fBest = apHitFractions[i];
            }
            if (fBest > apHitFractions[i + 1]) {
                iBest = i + 1;
                fBest = apHitFractions[i + 1];
            }
            if (fBest > apHitFractions[i + 2]) {
                iBest = i + 2;
                fBest = apHitFractions[i + 2];
            }
            if (fBest > apHitFractions[i + 3]) {
                iBest = i + 3;
                fBest = apHitFractions[i + 3];
            }
            i += 4;
        }
        while (i < aiNumHits - 3);
    }
    for (; i < aiNumHits; ++i) {
        if (fBest > apHitFractions[i]) {
            iBest = i;
            fBest = apHitFractions[i];
        }
    }
    if (iBest == -1) {
        return -1;
    }
    apOutput->m_shapeKeys[apOutput->m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
    const int32_t iSphere = apSphereIndices[iBest];
    apOutput->m_hitFraction = fBest;
    const __m128 kFraction = _mm_shuffle_ps(_mm_set_ss(fBest), _mm_set_ss(fBest), 0);
    const __m128 kCentre = apSpheres[iSphere].m_quad;
    hkVector4 kNormal;
    kNormal.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(apInput->m_from.m_quad, kCentre), _mm_sub_ps(hkQuadReal1111_0, kFraction)),
        _mm_mul_ps(_mm_sub_ps(apInput->m_to.m_quad, kCentre), kFraction)
    );
    apOutput->m_normal.m_quad = kNormal.m_quad;
    const float fInvRadius = 1.0 / apSpheres[iSphere].m_quad[3];
    apOutput->m_extraInfo = iSphere;
    apOutput->m_normal.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fInvRadius), _mm_set_ss(fInvRadius), 0), kNormal.m_quad);
    return iBest;
}

// Solves |from + t (to - from) - centre|^2 = r^2 per sphere, as the game does: each dot product is rounded to float,
// and every intermediate is stored as a float.
int32_t castRayInternal(
    const hkpShapeRayCastInput* apInput,
    const hkVector4* apSpheres,
    int32_t aiNumSpheres,
    float* apHitFractions,
    int32_t* apSphereIndices
) {
    HK_TIMER_BEGIN("rcMultiSpher", nullptr);
    int32_t iNumHits = 0;
    for (int32_t i = 0; i < aiNumSpheres; ++i) {
        const __m128 kCentre = apSpheres[i].m_quad;
        const __m128 kFrom = _mm_sub_ps(apInput->m_from.m_quad, kCentre);
        const __m128 kFromSq = _mm_mul_ps(kFrom, kFrom);
        const __m128 kDir = _mm_sub_ps(_mm_sub_ps(apInput->m_to.m_quad, kCentre), kFrom);
        // The dot products sum as (y + x) + z.
        const float fC = _mm_cvtss_f32(_mm_add_ps(
            _mm_shuffle_ps(kFromSq, kFromSq, 0xAA), _mm_add_ps(_mm_shuffle_ps(kFromSq, kFromSq, 0x55), kFromSq)
        ));
        const __m128 kDot = _mm_mul_ps(kDir, kFrom);
        const float fHalfB = _mm_cvtss_f32(
            _mm_add_ps(_mm_shuffle_ps(kDot, kDot, 0xAA), _mm_add_ps(_mm_shuffle_ps(kDot, kDot, 0x55), kDot))
        );
        float fB = fHalfB + fHalfB;
        // The x87 compares are unordered-true: a NaN goes on.
        if (!(fB >= 0.0f)) {
            const __m128 kDirSq = _mm_mul_ps(kDir, kDir);
            const float fA = _mm_cvtss_f32(_mm_add_ps(
                _mm_shuffle_ps(kDirSq, kDirSq, 0xAA), _mm_add_ps(_mm_shuffle_ps(kDirSq, kDirSq, 0x55), kDirSq)
            ));
            float fRadiusSq = apSpheres[i].m_quad[3] * apSpheres[i].m_quad[3];
            float fCMinusRadiusSq = fC - fRadiusSq;
            float fDiscriminant = fB * fB - fCMinusRadiusSq * (fA * 4.0);
            if (!(fDiscriminant <= 0.0f)) {
                float fMinusB = -fB;
                float fSqrt = std::sqrt(fDiscriminant);
                float fTwiceT = fMinusB - fSqrt;
                float fT = fTwiceT * 0.5;
                if (!(fA <= fT) && !(fT < 0.0f)) {
                    apHitFractions[iNumHits] = fT / fA;
                    apSphereIndices[iNumHits] = i;
                    ++iNumHits;
                }
            }
        }
    }
    HK_TIMER_END();
    return iNumHits;
}

hkpMultiSphereShape::hkpMultiSphereShape(const hkVector4* apSpheres, int32_t aiNumSpheres)
  : hkpSphereRepShape(HK_SHAPE_MULTI_SPHERE) {
    for (int32_t i = 0; i < aiNumSpheres; ++i) {
        m_spheres[i] = apSpheres[i];
    }
    m_numSpheres = aiNumSpheres;
}

hkpMultiSphereShape::~hkpMultiSphereShape() {}

void hkpMultiSphereShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

// The centres go to world space through hkVector4Util::transformPoints. No pass is made for a count of 0 or less.
void hkpMultiSphereShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 kCol0;
    hkVector4 kCol1;
    hkVector4 kCol2;
    hkVector4 kTranslation;
    hkVector4 akWorldSpheres[MAX_SPHERES];
    kCol0.m_quad = localToWorld.m_rotation.m_col0.m_quad;
    kCol1.m_quad = localToWorld.m_rotation.m_col1.m_quad;
    kCol2.m_quad = localToWorld.m_rotation.m_col2.m_quad;
    kTranslation.m_quad = localToWorld.m_translation.m_quad;
    const int32_t iNumSpheres = this->m_numSpheres;
    // EXE-INLINED none: hkVector4Util::transformPoints
    for (int32_t i = iNumSpheres - 1; i >= 0; --i) {
        const __m128 kCentre = m_spheres[i].m_quad;
        akWorldSpheres[i].m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kCentre, kCentre, 85), kCol1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kCentre, kCentre, 0), kCol0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCentre, kCentre, 170), kCol2.m_quad), kTranslation.m_quad)
        );
    }
    __m128 kMin = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    __m128 kMax = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
    for (int32_t i = 0; i < iNumSpheres; ++i) {
        const __m128 kCentre = akWorldSpheres[i].m_quad;
        const __m128 kRadius = _mm_shuffle_ps(m_spheres[i].m_quad, m_spheres[i].m_quad, 255);
        kMin = _mm_min_ps(kMin, _mm_sub_ps(kCentre, kRadius));
        kMax = _mm_max_ps(kMax, _mm_add_ps(kCentre, kRadius));
    }
    const __m128 kTolerance = _mm_shuffle_ps(_mm_set_ss(tolerance), _mm_set_ss(tolerance), 0);
    out.m_min.m_quad = _mm_sub_ps(kMin, kTolerance);
    out.m_max.m_quad = _mm_add_ps(kTolerance, kMax);
}

hkBool hkpMultiSphereShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    int32_t aiSphereIndices[MAX_SPHERES];
    float afHitFractions[MAX_SPHERES];
    const int32_t iNumHits = castRayInternal(&input, m_spheres, m_numSpheres, afHitFractions, aiSphereIndices);
    const int32_t iHit = findClosestHit(m_spheres, afHitFractions, aiSphereIndices, iNumHits, &input, &results);
    hkBool result;
    result.m_bool = iHit != -1;
    return result;
}

// KEEP (partcopy:): the output handed to collector.addRayHit has m_pad (bytes 0x18-0x1F) and m_shapeKeys[1..7]
// written by nothing here nor by findClosestHit, and the collectors copy the whole collector output, m_pad included
// (hkpAllRayHitCollector::addRayHit, the inlined 0x9C1B00): stack garbage reaches the collector's hits.
// The SDK's body: the hits of castRayImpl's first step, reported closest first, each removed by moving the last one
// into its place (the fraction copied with fld/fstp, 0xD2ACCB).
void hkpMultiSphereShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& inputLocal,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    alignas(16) int32_t aiSphereIndices[MAX_SPHERES];
    alignas(16) float afHitFractions[MAX_SPHERES];
    int32_t iNumHits = castRayInternal(&inputLocal, m_spheres, m_numSpheres, afHitFractions, aiSphereIndices);
    XFNV_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
    // EXE-INLINED 0x4A3D20
    output.m_hitFraction = 1.0f;
    output.m_extraInfo = -1;
    output.m_shapeKeyIndex = 0;
    output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
    while (iNumHits) {
        // EXE-INLINED 0x4A3D80
        output.m_hitFraction = 1.0f;
        output.m_extraInfo = -1;
        output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
        output.m_shapeKeyIndex = 0;
        const int32_t iHit = findClosestHit(m_spheres, afHitFractions, aiSphereIndices, iNumHits, &inputLocal, &output);
        // EXE-INLINED 0xC7F700
        const hkRotation& rRotation = cdBody.m_motion.pMotion->getRotation();
        const __m128 kNormal = output.m_normal.m_quad;
        output.m_normal.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x55), rRotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x00), rRotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xAA), rRotation.m_col2.m_quad)
        );
        collector.addRayHit(cdBody, output);
        afHitFractions[iHit] = afHitFractions[iNumHits - 1];
        aiSphereIndices[iHit] = aiSphereIndices[iNumHits - 1];
        --iNumHits;
    }
}

int32_t hkpMultiSphereShape::getNumCollisionSpheresImpl() const {
    return (int32_t)((uint32_t)m_numSpheres);
}

const hkSphere* hkpMultiSphereShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    int32_t v3 = 0;
    if (m_numSpheres > 0) {
        hkSphere* v4 = sphereBuffer;
        const hkVector4* m_spheres = this->m_spheres;
        do {
            (*v4).m_pos.m_quad = m_spheres->m_quad;
            ++v3;
            ++m_spheres;
            ++v4;
        }
        while (v3 < m_numSpheres);
    }
    return sphereBuffer;
}

const hkClass& hkpMultiSphereShape::staticClass() {
    return hkpMultiSphereShapeClass;
}
