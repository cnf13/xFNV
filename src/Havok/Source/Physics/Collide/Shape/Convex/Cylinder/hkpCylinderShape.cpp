#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The smallest value counted up from 0 in steps of 0.01 (a double added to the float) that hkToIntFast rounds to a
// non-zero int in the FPU's current mode; 1 once the value is no longer below 1.1.
// KEEP: both callers, hkpCylinderShape's constructor and its finish-loading constructor, keep `this` live across the
// call and store through it afterwards, relying on this function changing nothing but its return value, which clang
// does not guarantee.
float minValueRoundedUpTo1() {
    float value = 0.0f;
    while (hkMath::hkToIntFast(value) == 0) {
        value = value + 0.009999999776482582;
        if (!(value < 1.1f)) {
            return 1.0f;
        }
    }
    return value;
}

void assertRoundUpThreshold() {
    ;
}

int32_t setNumberOfVirtualSideSegements(int32_t numSegments) {
    int32_t result = numSegments / 8;
    float tesselationParameter = (static_cast<double>(numSegments / 8) - 0.10000000149011612) / 0.7070000171661377;
    hkpCylinderShape::s_virtualTesselationParameter = tesselationParameter;
    hkpCylinderShape::s_virtualTesselationParameterInv = 1.0 / tesselationParameter;
    return result;
}

hkpCylinderShape::hkpCylinderShape(
    const hkVector4& vertexA,
    const hkVector4& vertexB,
    float cylinderRadius,
    float paddingRadius
)
  : hkpConvexShape(HK_SHAPE_CYLINDER, paddingRadius), m_cylBaseRadiusFactorForHeightFieldCollisions(0.8f),
    m_vertexA(vertexA), m_vertexB(vertexB) {
    if (s_intRoundUpThreshold < 0.0f) {
        s_intRoundUpThreshold = 1.0f - minValueRoundedUpTo1();
    }
    setCylinderRadius(cylinderRadius);
    presetPerpendicularVector();
}

hkpCylinderShape::hkpCylinderShape(hkFinishLoadedObjectFlag flag) : hkpConvexShape(flag) {
    if (flag.m_finishing && (s_intRoundUpThreshold < 0.0f)) {
        s_intRoundUpThreshold = 1.0f - minValueRoundedUpTo1();
    }

    m_type = HK_SHAPE_CYLINDER;
}

hkpCylinderShape::~hkpCylinderShape() {}

void hkpCylinderShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

int32_t hkpCylinderShape::calcSizeForSpu(const CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    return 96;
}

// The box around both end points in world space, grown across the axis: per lane sqrt(1.01 - n * n) of the unit world
// axis n, times the cylinder radius, plus the convex radius and the tolerance. The end points go to world space from
// the last point down.
void hkpCylinderShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 akWorld[2];
    hkVector4Util::transformPoints(localToWorld, &m_vertexA, 2, akWorld);
    const __m128 kDir = _mm_sub_ps(akWorld[1].m_quad, akWorld[0].m_quad);
    const __m128 kSquares = _mm_mul_ps(kDir, kDir);
    const __m128 kLength2 = _mm_add_ss(
        _mm_shuffle_ps(kSquares, kSquares, 170),
        _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 85), kSquares)
    );
    const __m128 kInvLength = _mm_rsqrt_ss(kLength2);
    hkVector4 axis;
    axis.m_quad = _mm_mul_ps(_mm_shuffle_ps(kInvLength, kInvLength, 0), kDir);
    const float fSinX2 = 1.0099999904632568 - XFNV_M128_F32(axis.m_quad, 0) * XFNV_M128_F32(axis.m_quad, 0);
    const float fSinX = std::sqrt(fSinX2);
    XFNV_M128_F32(axis.m_quad, 0) = fSinX;
    const float fSinY2 = 1.0099999904632568 - XFNV_M128_F32(axis.m_quad, 1) * XFNV_M128_F32(axis.m_quad, 1);
    const float fSinY = std::sqrt(fSinY2);
    XFNV_M128_F32(axis.m_quad, 1) = fSinY;
    const float fSinZ2 = 1.0099999904632568 - XFNV_M128_F32(axis.m_quad, 2) * XFNV_M128_F32(axis.m_quad, 2);
    const float fSinZ = std::sqrt(fSinZ2);
    XFNV_M128_F32(axis.m_quad, 2) = fSinZ;
    const float fRadius = m_radius + tolerance;
    const __m128 kExtent = _mm_add_ps(
        _mm_shuffle_ps(_mm_set_ss(fRadius), _mm_set_ss(fRadius), 0),
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(m_cylRadius), _mm_set_ss(m_cylRadius), 0), axis.m_quad)
    );
    const __m128 kWorldA = akWorld[0].m_quad;
    const __m128 kWorldB = akWorld[1].m_quad;
    out.m_max.m_quad = _mm_max_ps(kWorldA, kWorldB);
    out.m_min.m_quad = _mm_sub_ps(_mm_min_ps(kWorldA, kWorldB), kExtent);
    out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, kExtent);
}

// The ray against the cylinder. No hit when the ray starts inside it: nearer the axis segment between the cap centres
// (each end point moved in along the axis by the convex radius) than both radii, and between the caps. Otherwise
// calcClosestLineLine gives the squared distance of the ray's and the segment's lines and the ray fraction of their
// closest approach, from which the hit steps back by the chord half length over the ray's direction across the axis
// (-1.0f for a ray parallel to it). A hit nearer than the output's fraction and between the caps is a side hit, its
// normal away from the axis; any other is cast against the plane of the cap on the start's side, a hit within the radii
// giving the axis as the normal. Each scalar step is stored to a float local.
hkBool hkpCylinderShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    hkBool result;

    HK_TIMER_BEGIN("rcCylinder", nullptr);
    float totalRadius = m_cylRadius + m_radius;
    const __m128 p1 = m_perpendicular1.m_quad;
    const __m128 p2 = m_perpendicular2.m_quad;
    const __m128 capShift = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(p1, p1, 210), _mm_shuffle_ps(p2, p2, 201)),
            _mm_mul_ps(_mm_shuffle_ps(p1, p1, 201), _mm_shuffle_ps(p2, p2, 210))
        ),
        _mm_set1_ps(m_radius)
    );
    hkVector4 capA;
    capA.m_quad = _mm_add_ps(m_vertexA.m_quad, capShift);
    hkVector4 capB;
    capB.m_quad = _mm_sub_ps(m_vertexB.m_quad, capShift);
    hkVector4 closest;
    hkpCapsuleShape::closestPointLineSeg(input.m_from, capA, capB, closest);
    hkVector4 fromClosest;
    fromClosest.setSub4(input.m_from, closest);
    float distToAxis = fromClosest.length3();
    bool startsInside = false;
    if (totalRadius > distToAxis) {
        hkVector4 capToCap;
        capToCap.setSub4(capB, capA);
        hkVector4 fromA;
        fromA.setSub4(input.m_from, capA);
        float fromBeyondA = fromA.dot3(capToCap);
        if (fromBeyondA > 0.0f) {
            hkVector4 fromB;
            fromB.setSub4(input.m_from, capB);
            float fromBeyondB = fromB.dot3(capToCap);
            startsInside = fromBeyondB < 0.0f;
        }
    }
    if (!startsInside) {
        hkVector4 rayDir;
        rayDir.m_quad = _mm_sub_ps(input.m_to.m_quad, input.m_from.m_quad);
        hkVector4 axis;
        axis.m_quad = _mm_sub_ps(capB.m_quad, capA.m_quad);
        float distSq = HK_REAL_MAX;
        float rayFraction;
        float axisFraction;
        hkVector4 closestOnRay;
        hkVector4 closestDiff;
        calcClosestLineLine(
            &input.m_from.m_quad,
            &rayDir.m_quad,
            &capA.m_quad,
            &axis.m_quad,
            &distSq,
            &rayFraction,
            &axisFraction,
            &closestOnRay.m_quad,
            &closestDiff.m_quad
        );
        const double radiusSq = totalRadius * totalRadius;
        if (!(distSq > radiusSq)) {
            const hkSimdReal axisLen2 = axis.dot3(axis);
            if (static_cast<float>(axisLen2) > HK_REAL_EPSILON) {
                const hkVector4 axisInv = axis.lengthInverse3().broadcast();
                float axisLen = _mm_cvtss_f32(_mm_mul_ss(axisLen2.getQuad(), axisInv.m_quad));
                hkVector4 axisDir;
                axisDir.m_quad = _mm_mul_ps(axisInv.m_quad, axis.m_quad);
                float negRayDotAxis = -static_cast<float>(rayDir.dot3(axisDir));
                hkVector4 rayAcross;
                rayAcross.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negRayDotAxis), axisDir.m_quad), rayDir.m_quad);
                float hitFraction;
                if (0.0f == static_cast<float>(rayAcross.dot3(rayAcross))) {
                    hitFraction = -1.0f;
                }
                else {
                    float chordSq = radiusSq - distSq;
                    float acrossInv = rayAcross.lengthInverse3();
                    float chordHalf = std::sqrt(chordSq);
                    float stepBack = chordHalf * acrossInv;
                    hitFraction = static_cast<double>(rayFraction) - stepBack;
                }
                if (!(results.m_hitFraction <= hitFraction)) {
                    const __m128 from = input.m_from.m_quad;
                    const __m128 to = input.m_to.m_quad;
                    float capADotAxis = axisDir.dot3(capA);
                    const __m128 hitFraction4 = _mm_set1_ps(hitFraction);
                    hkVector4 hitPoint;
                    hitPoint.m_quad = _mm_add_ps(
                        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, hitFraction4), from),
                        _mm_mul_ps(to, hitFraction4)
                    );
                    float hitAlongAxis = static_cast<float>(hitPoint.dot3(axisDir)) - capADotAxis;
                    if (hitFraction >= 0.0f && hitAlongAxis > 0.0f && hitAlongAxis < axisLen) {
                        float alongFraction = hitAlongAxis / axisLen;
                        const int32_t keyIndex = results.m_shapeKeyIndex;
                        const __m128 alongFraction4 = _mm_set1_ps(alongFraction);
                        results.m_hitFraction = hitFraction;
                        hkVector4 fromAxis;
                        fromAxis.m_quad = _mm_sub_ps(
                            hitPoint.m_quad,
                            _mm_add_ps(
                                _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, alongFraction4), capA.m_quad),
                                _mm_mul_ps(alongFraction4, capB.m_quad)
                            )
                        );
                        results.m_normal.m_quad = _mm_mul_ps(
                            fromAxis.lengthInverse3().broadcast().m_quad,
                            fromAxis.m_quad
                        );
                        results.m_shapeKeys[keyIndex] = HK_INVALID_SHAPE_KEY;
                        HK_TIMER_END();
                        result.m_bool = 1;
                        return result;
                    }
                    float fromDotAxis = input.m_from.dot3(axisDir);
                    bool capSide = true;
                    float normalSign = 0.0f;
                    hkVector4 cap;
                    cap.m_quad = _mm_setzero_ps();
                    if (fromDotAxis < capADotAxis) {
                        cap = capA;
                        normalSign = -1.0f;
                    }
                    else {
                        float capBDotAxis = axisLen + capADotAxis;
                        if (capBDotAxis < fromDotAxis) {
                            normalSign = 1.0f;
                            cap = capB;
                        }
                        else {
                            capSide = false;
                        }
                    }
                    if (capSide) {
                        hkVector4 fromToCap;
                        fromToCap.setSub4(cap, input.m_from);
                        float capDist = fromToCap.dot3(axisDir);
                        float planeDist = capDist * normalSign * -1.0;
                        if (!(planeDist < 0.0f)) {
                            float approach = -1.0 * normalSign * static_cast<float>(rayDir.dot3(axisDir));
                            if (!(approach * results.m_hitFraction <= planeDist)) {
                                float capFraction = planeDist / approach;
                                const __m128 capFraction4 = _mm_set1_ps(capFraction);
                                hkVector4 fromCap;
                                fromCap.m_quad = _mm_sub_ps(
                                    _mm_add_ps(
                                        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, capFraction4), from),
                                        _mm_mul_ps(capFraction4, to)
                                    ),
                                    cap.m_quad
                                );
                                if (!(static_cast<float>(fromCap.dot3(fromCap)) > radiusSq)) {
                                    const int32_t keyIndex = results.m_shapeKeyIndex;
                                    results.m_hitFraction = capFraction;
                                    results.m_normal.m_quad = _mm_mul_ps(_mm_set1_ps(normalSign), axisDir.m_quad);
                                    results.m_shapeKeys[keyIndex] = HK_INVALID_SHAPE_KEY;
                                    HK_TIMER_END();
                                    result.m_bool = 1;
                                    return result;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    HK_TIMER_END();
    result.m_bool = 0;
    return result;
}

int32_t hkpCylinderShape::getNumCollisionSpheresImpl() const {
    return 18;
}

// Eight spheres around each end point (the cap ring at m_cylRadius, or at the height-field factor of it when the
// cylinder is at least twice its radius long), with w the convex radius, then two on the axis that the long case
// moves out past the caps by m_cylRadius, with w the convex radius plus that distance.
const hkSphere* hkpCylinderShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    float cylRadius = m_cylRadius;
    float baseRadius = cylRadius;
    float capOffset = 0.0f;
    const __m128 axis = _mm_sub_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    const __m128 axisSq = _mm_mul_ps(axis, axis);
    float axisLen2 = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(axisSq, axisSq, 170), _mm_add_ss(_mm_shuffle_ps(axisSq, axisSq, 85), axisSq))
    );
    if (cylRadius * 4.0 * cylRadius <= axisLen2) {
        capOffset = cylRadius;
        baseRadius = cylRadius * m_cylBaseRadiusFactorForHeightFieldCollisions;
    }
    float diagRadius = baseRadius * 0.7071067690849304;
    float radius = m_radius;
    const __m128 p1 = m_perpendicular1.m_quad;
    const __m128 p2 = m_perpendicular2.m_quad;
    const __m128 ring1 = _mm_and_ps(_mm_mul_ps(_mm_set1_ps(baseRadius), p1), hkVector4::zeroElementMasks[3]);
    const __m128 ring2 = _mm_and_ps(_mm_mul_ps(_mm_set1_ps(baseRadius), p2), hkVector4::zeroElementMasks[3]);
    const __m128 diagSum = _mm_and_ps(
        _mm_mul_ps(_mm_set1_ps(diagRadius), _mm_add_ps(p2, p1)),
        hkVector4::zeroElementMasks[3]
    );
    const __m128 diagDiff = _mm_and_ps(
        _mm_mul_ps(_mm_set1_ps(diagRadius), _mm_sub_ps(p1, p2)),
        hkVector4::zeroElementMasks[3]
    );
    const hkVector4* pVertex = &m_vertexA;
    hkSphere* pSphere = sphereBuffer;
    for (int32_t n = 2; n != 0; --n) {
        hkVector4 centre;
        centre.m_quad = pVertex->m_quad;
        XFNV_M128_F32(centre.m_quad, 3) = radius;
        pSphere[0].m_pos.m_quad = _mm_add_ps(centre.m_quad, ring1);
        pSphere[1].m_pos.m_quad = _mm_add_ps(centre.m_quad, diagSum);
        pSphere[2].m_pos.m_quad = _mm_add_ps(centre.m_quad, ring2);
        pSphere[3].m_pos.m_quad = _mm_sub_ps(centre.m_quad, diagDiff);
        pSphere[4].m_pos.m_quad = _mm_sub_ps(centre.m_quad, ring1);
        pSphere[5].m_pos.m_quad = _mm_sub_ps(centre.m_quad, diagSum);
        pSphere[6].m_pos.m_quad = _mm_sub_ps(centre.m_quad, ring2);
        pSphere[7].m_pos.m_quad = _mm_add_ps(centre.m_quad, diagDiff);
        ++pVertex;
        pSphere += 8;
    }
    float negCapOffset = -capOffset;
    const __m128 q1 = m_perpendicular1.m_quad;
    const __m128 q2 = m_perpendicular2.m_quad;
    const __m128 normal = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(q1, q1, 201), _mm_shuffle_ps(q2, q2, 210)),
        _mm_mul_ps(_mm_shuffle_ps(q1, q1, 210), _mm_shuffle_ps(q2, q2, 201))
    );
    float capRadius = radius + capOffset;
    sphereBuffer[16].m_pos.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(capOffset), normal), m_vertexA.m_quad);
    sphereBuffer[17].m_pos.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negCapOffset), normal), m_vertexB.m_quad);
    XFNV_M128_F32(sphereBuffer[16].m_pos.m_quad, 3) = capRadius;
    XFNV_M128_F32(sphereBuffer[17].m_pos.m_quad, 3) = capRadius;
    return sphereBuffer;
}

void hkpCylinderShape::getFirstVertex(hkVector4& v) const {
    v = m_vertexB;
}

// The supporting vertex as a vertex id convertVertexIdsToVerticesImpl decodes: the direction's components along the
// two perpendiculars normalized (cos 1, sin 0 when both are nearly 0), the lesser of their magnitudes quantised with
// hkToIntFast against s_intRoundUpThreshold, which one is the sine, both signs and the cap facing the direction;
// w keeps the id.
void hkpCylinderShape::getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const {
    const __m128 dir = direction.m_quad;
    const __m128 axis = _mm_sub_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    const __m128 dot1 = _mm_mul_ps(dir, m_perpendicular1.m_quad);
    float cosAngle = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(dot1, dot1, 170), _mm_add_ss(_mm_shuffle_ps(dot1, dot1, 85), dot1))
    );
    const __m128 dot2 = _mm_mul_ps(dir, m_perpendicular2.m_quad);
    float sinAngle = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(dot2, dot2, 170), _mm_add_ss(_mm_shuffle_ps(dot2, dot2, 85), dot2))
    );
    float len2 = sinAngle * sinAngle + cosAngle * cosAngle;
    int32_t sinPositive;
    if (!(len2 >= 1.4210855e-14f)) {
        sinPositive = 1;
        sinAngle = 0.0f;
        cosAngle = 1.0f;
    }
    else {
        float len = std::sqrt(len2);
        float invLen = 1.0 / len;
        sinAngle = sinAngle * invLen;
        cosAngle = invLen * cosAngle;
        sinPositive = sinAngle >= 0.0f;
    }
    int32_t cosPositive = cosAngle >= 0.0f;
    float absSin = std::fabs(sinAngle);
    float absCos = std::fabs(cosAngle);
    float lesserAbs;
    int32_t sinIsLesser;
    if (absCos >= absSin) {
        lesserAbs = absSin;
        sinIsLesser = 1;
    }
    else {
        lesserAbs = absCos;
        sinIsLesser = 0;
    }
    const __m128 dotAxis = _mm_mul_ps(dir, axis);
    float dirDotAxis = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(dotAxis, dotAxis, 170), _mm_add_ss(_mm_shuffle_ps(dotAxis, dotAxis, 85), dotAxis))
    );
    int32_t isBaseA = 0.0f >= dirDotAxis;
    float scaled = s_virtualTesselationParameter * lesserAbs - s_intRoundUpThreshold + 0.05000000074505806;
    const int32_t id = hkMath::hkToIntFast(scaled) +
        16 * (sinIsLesser + 2 * (cosPositive + 2 * (sinPositive + 2 * isBaseA)));
    decodeVertexId(static_cast<uint16_t>(id), supportingVertexOut);
    supportingVertexOut.setInt24W(id);
}

// Each id decoded as getSupportingVertexImpl encodes it; w keeps the id.
void hkpCylinderShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t remaining = numIds - 1; remaining >= 0; --remaining) {
        const uint16_t id = *ids;
        decodeVertexId(id, *verticesOut);
        verticesOut->setInt24W(id);
        ++ids;
        ++verticesOut;
    }
}

void hkpCylinderShape::getCentreImpl(hkVector4& centreOut) const {
    hkVector4 v2;
    v2.m_quad = _mm_add_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    centreOut.m_quad = v2.m_quad;
    centreOut.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fHalf), _mm_set_ss(fHalf), 0), v2.m_quad);
}

void hkpCylinderShape::setCylinderRadius(float radius) {
    m_cylRadius = radius;
    XFNV_M128_F32(m_vertexA.m_quad, 3) = m_radius + radius;
    XFNV_M128_F32(m_vertexB.m_quad, 3) = radius + m_radius;
}

float hkpCylinderShape::getCylinderRadius() const {
    return m_cylRadius;
}

// The axis is normalized (reciprocal square root refined by one Newton step, 0 for a zero axis); the first
// perpendicular takes the two components other than the one of smallest magnitude, swapped and one negated, and is
// normalized the same way; the second is their cross product.
void hkpCylinderShape::presetPerpendicularVector() {
    const __m128 zero = _mm_setzero_ps();
    const __m128 three = _mm_set_ss(3.0f);
    const __m128 half = _mm_set_ss(0.5f);
    const __m128 axisRaw = _mm_sub_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    const __m128 axisSq = _mm_mul_ps(axisRaw, axisRaw);
    const __m128 axisLen2 = _mm_add_ss(
        _mm_shuffle_ps(axisSq, axisSq, 170),
        _mm_add_ss(_mm_shuffle_ps(axisSq, axisSq, 85), axisSq)
    );
    const __m128 axisApprox = _mm_rsqrt_ss(axisLen2);
    const __m128 axisLenInv = _mm_andnot_ps(
        _mm_cmpeq_ss(axisLen2, zero),
        _mm_mul_ss(
            _mm_mul_ss(half, axisApprox),
            _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(axisLen2, axisApprox), axisApprox))
        )
    );
    hkVector4 axis;
    axis.m_quad = _mm_mul_ps(_mm_shuffle_ps(axisLenInv, axisLenInv, 0), axisRaw);
    float absX = std::fabs(XFNV_M128_F32(axis.m_quad, 0));
    float absY = std::fabs(XFNV_M128_F32(axis.m_quad, 1));
    float absZ = std::fabs(XFNV_M128_F32(axis.m_quad, 2));
    int32_t i = 0;
    int32_t j = 1;
    int32_t k = 2;
    float minAbs = absX;
    if (minAbs > absY) {
        minAbs = absY;
        j = 0;
        i = 1;
    }
    if (minAbs > absZ) {
        k = i;
    }
    m_perpendicular1.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(m_perpendicular1.m_quad, j) = XFNV_M128_F32(axis.m_quad, k);
    XFNV_M128_F32(m_perpendicular1.m_quad, k) = -XFNV_M128_F32(axis.m_quad, j);
    const __m128 perp = m_perpendicular1.m_quad;
    const __m128 perpSq = _mm_mul_ps(perp, perp);
    const __m128 perpLen2 = _mm_add_ss(
        _mm_shuffle_ps(perpSq, perpSq, 170),
        _mm_add_ss(_mm_shuffle_ps(perpSq, perpSq, 85), perpSq)
    );
    const __m128 perpApprox = _mm_rsqrt_ss(perpLen2);
    const __m128 perpLenInv = _mm_andnot_ps(
        _mm_cmpeq_ss(perpLen2, zero),
        _mm_mul_ss(
            _mm_mul_ss(half, perpApprox),
            _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(perpLen2, perpApprox), perpApprox))
        )
    );
    m_perpendicular1.m_quad = _mm_mul_ps(_mm_shuffle_ps(perpLenInv, perpLenInv, 0), perp);
    const __m128 p1 = m_perpendicular1.m_quad;
    const __m128 axisQ = axis.m_quad;
    m_perpendicular2.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(p1, p1, 210), _mm_shuffle_ps(axisQ, axisQ, 201)),
        _mm_mul_ps(_mm_shuffle_ps(p1, p1, 201), _mm_shuffle_ps(axisQ, axisQ, 210))
    );
}

const hkClass& hkpCylinderShape::staticClass() {
    return hkpCylinderShapeClass;
}
