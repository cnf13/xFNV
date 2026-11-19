#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpConvexPenetrationUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpWingedEdge.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"

#include <cmath>
#include <xmmintrin.h>

// The pair of triangles, one of each polytope, whose planes come closest, and the point between them: a3 gets the
// normalized blend of the two normals weighted by the closest points' parameters, its w the square root of the least
// squared distance found (0 when that is negative). a1 and a2 are walked through m_triangles down from the last. The
// caller, hkCalcMultiPenetrationDepth, passes two works' m_wingedEdge and, as a4 and a5, their m_nearestTriangle.
static void findNearestTrianglePair(
    hkpWingedEdge* a1,
    hkpWingedEdge* a2,
    hkVector4* a3,
    hkpWingedEdge::Triangle** a4,
    hkpWingedEdge::Triangle** a5
);
// The fallback when the penetration solver gives up: support pairs
// along the three axes both ways, then refined around the best direction with a shrinking step, written into
// pointsAinA[0], pointsBinB[0] and output; returns output.
// pointsAinA, pointsBinB: hkpCdVertex*, output: hkpGskOut*: the caller hkCalculatePenetrationDepth passes the support
// point arrays it gives hkpConvexPenetrationUtil::init and the hkpGskOut it gives calculatePenetrationDepth.
static hkpGskOut* sampleSupportPenetration(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkTransform* transform,
    hkpCdVertex* pointsAinA,
    hkpCdVertex* pointsBinB,
    hkpGskOut* output
);
// Shape A's support vertex along -direction and shape B's along
// direction (rotated into B's space); keeps them in best when their distance along direction is greater than best's.
static void updateBestSupportPair(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    hkSupportPair* best,
    const hkVector4* direction,
    const hkTransform* transform
);

// Pairs each triangle of a1 with each of a2 but the first, both from the last down, skipping those whose distance
// to the origin squared exceeds the best so far, and keeps the pair whose closest points come nearest.
static void findNearestTrianglePair(
    hkpWingedEdge* a1,
    hkpWingedEdge* a2,
    hkVector4* a3,
    hkpWingedEdge::Triangle** a4,
    hkpWingedEdge::Triangle** a5
) {
    float v47 = 3.40282e+38f;
    double v5 = 0.0;
    float v53 = 0.0f;
    float v54 = 0.0f;
    int32_t v6 = a1->m_numTriangles - 1;
    if (v6 >= 0) {
        int32_t v10;
        double v7 = 1.0;
        hkpWingedEdge::Triangle* triA = &a1->m_triangles[v6];
        while (true) {
            if (!(v47 < triA->m_distanceToOrigin * triA->m_distanceToOrigin)) {
                v10 = a2->m_numTriangles - 1;
                if (v10 >= 1) {
                    break;
                }
            }
        LABEL_24:;
            --v6;
            --triA;
            if (v6 < 0) {
                goto LABEL_25;
            }
        }
        hkpWingedEdge::Triangle* triB = &a2->m_triangles[v10];
        int32_t v12 = a2->m_numTriangles - 1;
        while (true) {
            __m128 v13;
            float v14;
            double v15;
            double v16;
            double v17;
            double v18;
            double v26;
            double v27;
            double v28;
            double v29;
            double v30;
            float v41;
            float v42;
            float v43;
            float v46;
            float v48;
            if (v47 < triB->m_distanceToOrigin * triB->m_distanceToOrigin) {
                goto LABEL_23;
            }
            v13 = _mm_mul_ps(triB->m_normal.m_quad, triA->m_normal.m_quad);
            v14 = _mm_shuffle_ps(v13, v13, 170)[0] + (_mm_shuffle_ps(v13, v13, 85)[0] + v13[0]);
            v15 = v14;
            if (v14 < -0.5f) {
                goto LABEL_23;
            }
            v16 = triA->m_distanceToOrigin;
            v17 = triB->m_distanceToOrigin;
            v48 = v16 - v15 * v17;
            v43 = v17 - v15 * v16;
            v18 = v48;
            if (!(v48 > 1.1920929e-07f)) {
                if (!(v16 > 1.1920929e-07f)) {
                    goto LABEL_18;
                }
                v41 = v5;
                v42 = 1.0f;
                v28 = v17 * v17;
            }
            else {
                double v19;
                v19 = v43;
                if (v43 > 1.1920929e-07f) {
                    double v21;
                    double v22;
                    float v44;
                    if (!(v19 < v18)) {
                        float v51 = v18 / v19;
                        double v23 = v15 * v51 + v7;
                        v42 = v7 / v23;
                        v41 = v51 * v42;
                        double v24 = v51;
                        float v52 = triB->m_distanceToOrigin / v23;
                        v44 = v24 * v52;
                        v22 = v52;
                        v21 = v14;
                    }
                    else {
                        float v49 = v19 / v18;
                        double v20 = v15 * v49 + v7;
                        v41 = v7 / v20;
                        v42 = v41 * v49;
                        v44 = v16 / v20;
                        v21 = v14;
                        float v50 = v49 * v44;
                        v22 = v50;
                    }
                    float v45 = v21 * v22 + v44;
                    double v25 = v22 * ((v7 - v21 * v21) * v22);
                    v26 = v7;
                    v46 = v45 * v45 + v25;
                    v27 = 0.0;
                    goto LABEL_20;
                }
                if (!(v17 > v5)) {
                LABEL_18:;
                    v41 = 0.5f;
                    v42 = 0.5f;
                    v28 = v15 * -v15;
                    goto LABEL_19;
                }
                v41 = 1.0f;
                v42 = v5;
                v28 = v16 * v16;
            }
        LABEL_19:;
            v46 = v28;
            v29 = v7;
            v27 = v5;
            v26 = v29;
        LABEL_20:;
            if (v47 > static_cast<double>(v46)) {
                v53 = v41;
                *a4 = triA;
                v54 = v42;
                *a5 = triB;
                v47 = v46;
            }
            v30 = v27;
            v7 = v26;
            v5 = v30;
        LABEL_23:;
            --triB;
            if (!--v12) {
                goto LABEL_24;
            }
        }
    }
LABEL_25:;
    __m128 v31 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v53), _mm_set_ss(v53), 0), (*a4)->m_normal.m_quad);
    a3->m_quad = v31;
    __m128 v32 =
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v54), _mm_set_ss(v54), 0), (*a5)->m_normal.m_quad), v31);
    a3->m_quad = v32;
    a3->normalize3();
    if (!(v47 < 0.0f)) {
        v5 = v47;
    }
    float v55 = v5;
    float v56 = std::sqrt((double)v55);
    a3->m_quad[3] = v56;
}

static void updateBestSupportPair(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    hkSupportPair* best,
    const hkVector4* direction,
    const hkTransform* transform
) {
    hkVector4 negDirection;
    hkpCdVertex vertexA;
    negDirection.m_quad = _mm_xor_ps(direction->m_quad, hkQuadRealNegateMask);
    shapeA->getSupportingVertexImpl(negDirection, vertexA);
    hkVector4 directionInB;
    directionInB._setRotatedInverseDir(transform->m_rotation, *direction);
    hkpCdVertex vertexB;
    shapeB->getSupportingVertexImpl(directionInB, vertexB);
    const __m128 b = vertexB.m_quad;
    const __m128 dir = direction->m_quad;
    // EXE-INLINED none: hkVector4::_setTransformedPos
    const __m128 bInA = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(b, b, 85), transform->m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(b, b, 0), transform->m_rotation.m_col0.m_quad)
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(b, b, 170), transform->m_rotation.m_col2.m_quad), transform->m_translation.m_quad
        )
    );
    const __m128 v14 = _mm_mul_ps(_mm_sub_ps(vertexA.m_quad, bInA), dir);
    // Sums as (y + x) + z in the low lane.
    const float distance =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v14, v14, 170), _mm_add_ss(_mm_shuffle_ps(v14, v14, 85), v14)));
    if (best->m_direction.m_quad[3] < distance) {
        best->m_direction.m_quad = dir;
        best->m_vertexA.m_quad = vertexA.m_quad;
        best->m_vertexB.m_quad = b;
        best->m_direction.m_quad[3] = distance;
    }
}

// Deliberately keeps an uninitialised read on some path, as the game does. best's vertices and the xyz of its direction
// are written only when updateBestSupportPair finds a pair beyond the one before; when no call does (every distance NaN
// or at most -3.40282e38), the refinement reads best's direction and the end copies best's vertices, which nothing
// wrote.
static hkpGskOut* sampleSupportPenetration(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkTransform* transform,
    hkpCdVertex* pointsAinA,
    hkpCdVertex* pointsBinB,
    hkpGskOut* output
) {
    hkSupportPair best;
    best.m_direction.m_quad[3] = -3.40282e+38f;
    for (int32_t i = 0; i < 3; ++i) {
        hkVector4 direction = hkTransformIdentity.m_rotation.getColumn(i);
        updateBestSupportPair(shapeA, shapeB, &best, &direction, transform);
        direction.m_quad = _mm_xor_ps(hkQuadRealNegateMask, direction.m_quad);
        updateBestSupportPair(shapeA, shapeB, &best, &direction, transform);
    }
    const __m128 zero = _mm_setzero_ps();
    float scale = 1.0f;
    while (true) {
        float step = scale * 0.9900000095367432;
        float absX = std::fabs(best.m_direction.m_quad[0]);
        float absY = std::fabs(best.m_direction.m_quad[1]);
        float absZ = std::fabs(best.m_direction.m_quad[2]);
        // The axis of the direction's least component (iMin) and the two others (i0, i1): the perpendicular swaps
        // those two and negates one.
        int32_t iMin = 0;
        int32_t i0 = 1;
        int32_t i1 = 2;
        if (absX > absY) {
            absX = absY;
            i0 = 0;
            iMin = 1;
        }
        if (absX > absZ) {
            i1 = iMin;
        }
        hkVector4 perp;
        perp.m_quad = zero;
        perp.m_quad[i0] = best.m_direction.m_quad[i1];
        perp.m_quad[i1] = -best.m_direction.m_quad[i0];
        perp.normalize3();
        const __m128 perpN = perp.m_quad;
        const __m128 stepV = _mm_set1_ps(step);
        const __m128 offset = _mm_mul_ps(stepV, perpN);
        const __m128 bd = best.m_direction.m_quad;
        hkVector4 perpCross;
        perpCross.setCross(best.m_direction, perp);
        const __m128 cross = _mm_mul_ps(perpCross.m_quad, stepV);
        hkVector4 moved;
        moved.m_quad = _mm_add_ps(bd, offset);
        const __m128 inv = moved.lengthInverse3().broadcast().m_quad;
        hkVector4 direction0;
        hkVector4 direction1;
        hkVector4 direction2;
        hkVector4 direction3;
        direction0.m_quad = _mm_mul_ps(inv, moved.m_quad);
        direction1.m_quad = _mm_mul_ps(_mm_sub_ps(bd, offset), inv);
        direction2.m_quad = _mm_mul_ps(_mm_add_ps(cross, bd), inv);
        direction3.m_quad = _mm_mul_ps(_mm_sub_ps(bd, cross), inv);
        updateBestSupportPair(shapeA, shapeB, &best, &direction0, transform);
        updateBestSupportPair(shapeA, shapeB, &best, &direction1, transform);
        updateBestSupportPair(shapeA, shapeB, &best, &direction2, transform);
        updateBestSupportPair(shapeA, shapeB, &best, &direction3, transform);
        float next = step * 0.5;
        scale = next;
        if (!(next > 0.001f)) {
            break;
        }
    }
    *pointsAinA = best.m_vertexA;
    *pointsBinB = best.m_vertexB;
    output->m_distance.m_storage = best.m_direction.m_quad[3];
    output->m_normalInA = best.m_direction;
    output->m_pointAinA = best.m_vertexA;
    return output;
}

// KEEP: the exe body writes only eax (it saves and restores ecx), and randomVector (0xDF9B20) keeps vecOut in edx
// across the call, which clang does not guarantee: it stores through edx at 0xDF9B3B after the call at 0xDF9B20.
// The LCG step is computed unsigned: the exe's imul wraps.
float hkpConvexPenetrationUtil_rand01(int32_t seed) {
    const int32_t bits = static_cast<int32_t>((0x3039u - 0x3E39B193u * static_cast<uint32_t>(seed)) & 0x7FFFFFFFu);
    return static_cast<double>(bits) * 4.656612873077393e-10;
}

// Adds and multiplies in double and rounds to a float only for the store. The expression is (r + min) * (max - min).
void hkpConvexPenetrationUtil_randomVector(int32_t seed, float min, float max, hkVector4& vecOut) {
    const double r = hkpConvexPenetrationUtil_rand01(seed);
    const float f = static_cast<float>((r + min) * (static_cast<double>(max) - min));
    // Three float stores, not a whole-vector store: w stays untouched.
    vecOut(0) = f;
    vecOut(1) = f;
    vecOut(2) = f;
}

// One work block per shape B on the thread's stack memory: each gets a solver set up as init does, with epsTolerance
// 1e-4 and its square as constants, and its tetrahedron; a shape whose tetrahedron fails is dropped and its block
// reused. The first two blocks are then expanded together: findNearestTrianglePair finds the nearest pair of their
// triangles and the plane between them, and a polytope is expanded only once it has 55 vertices (it is skipped below
// the limit; initializePolytopeExpansion leaves 4), until none expands. Each contact is that plane's normal and its
// point on A in transformA's space, with the plane's distance less both radii in w.
hkResult hkCalcMultiPenetrationDepth(
    const hkTransform& transformA,
    const hkpConvexShape* shapeA,
    const hkpConvexShape** shapeBs,
    int32_t numShapeBs,
    const hkTransform& aTb,
    hkContactPoint** pointsOut
) {
    // The stack allocator hands out raw memory.
    hkCalcMultiPenetrationDepthWork* const workData =
        hkAllocateStack<hkCalcMultiPenetrationDepthWork>(numShapeBs, nullptr);

    hkpGskStatus gjkStatus[2];
    alignas(16) hkpGskOut output;
    hkCalcMultiPenetrationDepthWork* work = workData;
    for (int32_t i = 0; i < numShapeBs; ++i) {
        work->m_shapeB = shapeBs[i];
        work->m_pointOut = pointsOut[i];
        hkpConvexPenetrationUtil& util = work->m_util;
        // EXE-INLINED 0xDFB1E0
        util.m_aTb.m_rotation = aTb.m_rotation;
        util.m_epsTolSqrd.m_storage = 9.999999e-09f;
        util.m_shapeB.m_storage = shapeBs[i];
        util.m_shapeA.m_storage = shapeA;
        util.m_pointsAinA.m_storage = work->m_pointsAinA;
        util.m_backupTries.m_storage = 0;
        util.m_simplexSize.m_storage = 0;
        util.m_pointsBinB.m_storage = work->m_pointsBinB;
        util.m_simplex.m_storage = work->m_simplex;
        util.m_aTb.m_translation.m_quad = aTb.m_translation.m_quad;
        util.m_epsTol.m_quad = _mm_set1_ps(0.0001f);
        if (util.builtTetrahedron(output, gjkStatus[0]) == HK_FAILURE) {
            --numShapeBs;
        }
        else {
            work->m_wingedEdge.initializePolytopeExpansion(work->m_pointsAinA, work->m_pointsBinB, work->m_simplex);
            ++work;
        }
    }

    if (numShapeBs > 2) {
        numShapeBs = 2;
    }
    else if (numShapeBs < 2) {
        hkDeallocateStack(workData);
        return HK_FAILURE;
    }

    gjkStatus[0] = HK_GSK_OK;
    gjkStatus[1] = HK_GSK_OK;
    alignas(16) hkVector4 plane;
    bool expanded;
    do {
        findNearestTrianglePair(
            &workData[0].m_wingedEdge, &workData[1].m_wingedEdge, &plane, &workData[0].m_nearestTriangle,
            &workData[1].m_nearestTriangle
        );
        expanded = false;
        for (int32_t i = 0; i < numShapeBs; ++i) {
            hkCalcMultiPenetrationDepthWork& current = workData[i];
            hkpWingedEdge& we = current.m_wingedEdge;
            if (gjkStatus[i] == HK_GSK_PD_OUT_OF_MEMORY ||
                we.m_numVertices.m_storage < hkpWingedEdge::HK_WINGED_EDGE_VERTICIES_LIMIT) {
                continue;
            }
            hkpWingedEdge::Vertex* newVertex = &we.m_vertex[we.m_numVertices.m_storage++];
            newVertex->m_referee.m_storage = 0;
            current.m_util.getPointForSimplex(current.m_nearestTriangle->m_normal, *newVertex);
            if (current.m_util.tryToExpandPolytope(we, current.m_nearestTriangle, newVertex, gjkStatus[i]) ==
                HK_FAILURE) {
                if (gjkStatus[i] == HK_GSK_PD_DISTANCE_TOO_BIG) {
                    hkDeallocateStack(workData);
                    return HK_FAILURE;
                }
            }
            else {
                expanded = true;
            }
        }
    }
    while (expanded);

    hkVector4 normal;
    normal.setNeg4(plane);
    const float planeDistance = -plane(3);
    // x87 copy of shapeA->m_radius.
    const float radiusA = shapeA->m_radius;
    if (numShapeBs > 0) {
        const double distance = static_cast<double>(planeDistance) - radiusA;
        const __m128 normalY = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x55);
        const __m128 normalX = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x00);
        const __m128 normalZ = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0xAA);
        hkCalcMultiPenetrationDepthWork* current = workData;
        int32_t remaining = numShapeBs;
        do {
            current->m_util.convertResultToFeature(current->m_wingedEdge, current->m_nearestTriangle, output);
            // EXE-INLINED none: hkVector4::_setRotatedDir
            current->m_pointOut->m_separatingNormal.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(normalY, transformA.m_rotation.m_col1.m_quad),
                    _mm_mul_ps(transformA.m_rotation.m_col0.m_quad, normalX)
                ),
                _mm_mul_ps(normalZ, transformA.m_rotation.m_col2.m_quad)
            );
            // x87 copy of m_shapeB->m_radius.
            const float radiusB = current->m_shapeB->m_radius;
            const float contactDistance = static_cast<float>(distance - radiusB);
            const __m128 pointA = output.m_pointAinA.m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            current->m_pointOut->m_position.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(pointA, pointA, 0x55), transformA.m_rotation.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(pointA, pointA, 0x00), transformA.m_rotation.m_col0.m_quad)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(pointA, pointA, 0xAA), transformA.m_rotation.m_col2.m_quad),
                    transformA.m_translation.m_quad
                )
            );
            current->m_pointOut->m_separatingNormal(3) = contactDistance;
            ++current;
        }
        while (--remaining);
    }
    hkDeallocateStack(workData);
    return HK_SUCCESS;
}

hkpGskStatus hkCalculatePenetrationDepth(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkTransform* aTb,
    float epsTolerance,
    hkpCdVertex* pointsAinA,
    hkpCdVertex* pointsBinB,
    hkVector4* simplex,
    int32_t simplexSize,
    hkpGskOut* output
) {
    HK_TIMER_BEGIN("Penetration", nullptr);
    hkpConvexPenetrationUtil util;
    util.init(shapeA, shapeB, *aTb, epsTolerance, pointsAinA, pointsBinB, simplex, simplexSize);
    hkpGskStatus status = util.calculatePenetrationDepth(*output);
    if (status == HK_GSK_PD_DISTANCE_TOO_BIG) {
        sampleSupportPenetration(shapeA, shapeB, aTb, pointsAinA, pointsBinB, output);
        status = HK_GSK_PD_HEURISTIC_SAMPLING;
    }
    HK_TIMER_END();
    return status;
}

// A's support point along direction, B's along the negated direction turned into B's space (the transpose of
// m_aTb's rotation), and their difference with B's point taken back into A's space.
void hkpConvexPenetrationUtil::getPointForSimplex(const hkVector4& direction, hkpWingedEdge::Vertex& result) {
    m_shapeA.m_storage->getSupportingVertexImpl(direction, result.m_pointAinA);
    alignas(16) hkVector4 negDirection;
    negDirection.setNeg4(direction);
    alignas(16) hkVector4 negDirectionInB;
    negDirectionInB._setRotatedInverseDir(m_aTb.m_rotation, negDirection);
    m_shapeB.m_storage->getSupportingVertexImpl(negDirectionInB, result.m_pointBinB);
    const __m128 pointB = result.m_pointBinB.m_quad;
    // EXE-INLINED none: hkVector4::_setTransformedPos
    result.m_position.m_quad = _mm_sub_ps(
        result.m_pointAinA.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pointB, pointB, 0x55), m_aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(pointB, pointB, 0x00), m_aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pointB, pointB, 0xAA), m_aTb.m_rotation.m_col2.m_quad),
                m_aTb.m_translation.m_quad
            )
        )
    );
}

// getPointForSimplex writes only pA's m_position, m_pointAinA and m_pointBinB; pA.m_referee is never written, yet it is
// copied into result on the fDistB <= fDistA path, an uninitialised read kept deliberately as the game does. Each dot
// product adds y + x, then z. The compare keeps the negated direction's result when it is larger or either is NaN. The
// copy of pA into result writes the three vectors and m_referee, not the struct's padding.
void hkpConvexPenetrationUtil::findBestProjection(
    const hkVector4& direction,
    const hkVector4& from,
    hkpWingedEdge::Vertex& result,
    float& bestDistanceOut
) {
    alignas(16) hkVector4 directionNeg;
    directionNeg.setNeg4(direction);
    alignas(16) hkpWingedEdge::Vertex pA;
    getPointForSimplex(direction, pA);
    getPointForSimplex(directionNeg, result);

    const __m128 kFrom = _mm_load_ps(&reinterpret_cast<const float*>(&from.m_quad)[0]);
    hkVector4 kA;
    kA.m_quad = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&pA.m_position.m_quad)[0]), kFrom);
    const float fDistA = kA.dot3(direction);
    hkVector4 kB;
    kB.m_quad = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&result.m_position.m_quad)[0]), kFrom);
    const float fDistB = kB.dot3(directionNeg);

    if (!(fDistB <= fDistA)) {
        bestDistanceOut = fDistB;
    }
    else {
        result.m_position = pA.m_position;
        result.m_pointAinA = pA.m_pointAinA;
        result.m_pointBinB = pA.m_pointBinB;
        result.m_referee.m_storage = pA.m_referee.m_storage;
        bestDistanceOut = fDistA;
    }
}

// Up to ten triangles are tried until the origin's barycentric coordinates in one are all >= 0 (a NaN weight counts as
// inside, as the game does: only 0 > weight leaves). m_simplex is reloaded for every store. The point on A sums weight
// 1, then weight 0, then weight 2 terms, as the game does.
void hkpConvexPenetrationUtil::convertResultToFeature(
    hkpWingedEdge& polytope,
    hkpWingedEdge::Triangle* nearestTriangle,
    hkpGskOut& output
) {
    float lambda[3];
    for (int32_t i = 0; i < 10; ++i) {
        m_simplex.m_storage[0] = nearestTriangle->m_edge[0].m_vertex->m_position;
        m_simplex.m_storage[1] = nearestTriangle->m_edge[1].m_vertex->m_position;
        m_simplex.m_storage[2] = nearestTriangle->m_edge[2].m_vertex->m_position;
        hkVector4* pSimplex = m_simplex.m_storage;
        hkpCollideTriangleUtil::calcBarycentricCoordinates(
            hkVector4Zero, pSimplex[0], pSimplex[1], pSimplex[2], reinterpret_cast<hkPadSpu<float>*>(lambda)
        );
        if (!(0.0f > lambda[0]) && !(0.0f > lambda[1]) && !(0.0f > lambda[2])) {
            break;
        }
        nearestTriangle = polytope.findTriangleNearestOrigin(nearestTriangle);
    }

    for (int32_t i = 0; i < 3; ++i) {
        const hkpWingedEdge::Vertex* pVertex = nearestTriangle->m_edge[i].m_vertex;
        m_pointsAinA.m_storage[i] = pVertex->m_pointAinA;
        m_pointsBinB.m_storage[i] = pVertex->m_pointBinB;
    }

    m_simplexSize.m_storage = 3;
    const hkpCdVertex* pPointsA = m_pointsAinA.m_storage;
    const __m128 kSum = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(lambda[1]), _mm_load_ps(&reinterpret_cast<const float*>(&pPointsA[1].m_quad)[0])),
            _mm_mul_ps(_mm_set1_ps(lambda[0]), _mm_load_ps(&reinterpret_cast<const float*>(&pPointsA[0].m_quad)[0]))
        ),
        _mm_mul_ps(_mm_set1_ps(lambda[2]), _mm_load_ps(&reinterpret_cast<const float*>(&pPointsA[2].m_quad)[0]))
    );
    _mm_store_ps(&reinterpret_cast<float*>(&output.m_pointAinA.m_quad)[0], kSum);
    output.m_distance.m_storage = -nearestTriangle->m_distanceToOrigin;
    output.m_normalInA.setNeg4(nearestTriangle->m_normal);
}

void hkpConvexPenetrationUtil::addPointToSimplex(const hkpWingedEdge::Vertex& p) {
    int32_t m_storage = m_simplexSize.m_storage;
    m_simplex.m_storage[m_storage] = p.m_position;
    m_pointsAinA.m_storage[m_storage] = p.m_pointAinA;
    m_pointsBinB.m_storage[m_storage] = p.m_pointBinB;
    m_simplexSize.m_storage = m_storage + 1;
}

// The tetrahedron the polytope expansion starts from, grown from the simplex GSK ended with (m_simplexSize 0 to 4) by
// support points along the directions each smaller simplex leaves open. HK_SUCCESS: the tetrahedron is in m_simplex.
// HK_FAILURE with HK_GSK_OK: the shapes touch at a point, an edge or a face, written to output. HK_FAILURE with
// HK_GSK_PD_DISTANCE_TOO_BIG: the simplex degenerated again at or below the size the last point was added at. The
// compares keep the game's unordered outcomes. The squared height stays a double (exact for a float product); the
// scaled squared area is rounded to a float.
hkResult hkpConvexPenetrationUtil::builtTetrahedron(hkpGskOut& output, hkpGskStatus& gjkStatus) {
    // The simplex size the last point was added at.
    int32_t dimension = -1;
    // The support points of every case are kept in two variables: vertexA holds each case's only or first one (the
    // point against the direction in the one-point case), vertexB the second. A case's point is thus written over what
    // an earlier case left there.
    alignas(16) hkpWingedEdge::Vertex vertexA;
    alignas(16) hkpWingedEdge::Vertex vertexB;
    switch (m_simplexSize.m_storage) {
        case 0: goto simplex0;
        case 1: goto simplex1;
        case 2: goto simplex2;
        case 3: goto simplex3;
        case 4: goto simplex4;
        default: gjkStatus = HK_GSK_PD_DISTANCE_TOO_BIG; return HK_FAILURE;
    }

simplex4:
    {
        const hkVector4* simplex = m_simplex.m_storage;
        hkVector4 e0;
        e0.m_quad = _mm_sub_ps(simplex[0].m_quad, simplex[1].m_quad);
        hkVector4 e1;
        e1.m_quad = _mm_sub_ps(simplex[1].m_quad, simplex[2].m_quad);
        hkVector4 n;
        n.setCross(e0, e1);
        const float areaSq = n.dot3(n);
        hkVector4 h;
        h.m_quad = _mm_sub_ps(simplex[3].m_quad, simplex[0].m_quad);
        const float height = h.dot3(n);
        const float scaledAreaSq = m_epsTolSqrd.m_storage * areaSq;
        if (!(scaledAreaSq >= static_cast<double>(height) * height)) {
            gjkStatus = HK_GSK_OK;
            return HK_SUCCESS;
        }
        if (dimension >= 3) {
            gjkStatus = HK_GSK_PD_DISTANCE_TOO_BIG;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 3;
    }

simplex3:
    {
        const hkVector4* simplex = m_simplex.m_storage;
        hkVector4 e0;
        e0.m_quad = _mm_sub_ps(simplex[0].m_quad, simplex[1].m_quad);
        hkVector4 e1;
        e1.m_quad = _mm_sub_ps(simplex[1].m_quad, simplex[2].m_quad);
        hkVector4 n;
        n.setCross(e0, e1);
        const float len2 = n.dot3(n);
        // A NaN area counts as a face, as the game does.
        if (len2 != 0.0f) {
            alignas(16) hkVector4 normal = n;
            normal.normalize3();
            dimension = 3;
            hkpWingedEdge::Vertex& vertex = vertexA;
            float distance;
            findBestProjection(normal, m_simplex.m_storage[0], vertex, distance);
            if (m_epsTol(0) > distance) {
                // The origin lies in the face: its barycentric coordinates weight the support points on A.
                output.m_distance.m_storage = 0.0f;
                output.m_normalInA.m_quad = normal.m_quad;
                const hkVector4* face = m_simplex.m_storage;
                alignas(16) hkPadSpu<float> lambda[3];
                hkpCollideTriangleUtil::calcBarycentricCoordinates(hkVector4Zero, face[0], face[1], face[2], lambda);
                const hkpCdVertex* pointsA = m_pointsAinA.m_storage;
                output.m_pointAinA.m_quad = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_set1_ps(lambda[1].m_storage), pointsA[1].m_quad),
                        _mm_mul_ps(_mm_set1_ps(lambda[0].m_storage), pointsA[0].m_quad)
                    ),
                    _mm_mul_ps(_mm_set1_ps(lambda[2].m_storage), pointsA[2].m_quad)
                );
                gjkStatus = HK_GSK_OK;
                return HK_FAILURE;
            }
            addPointToSimplex(vertex);
            goto simplex4;
        }
        if (dimension >= 2) {
            // A degenerate triangle again: the edge s0 - s1 becomes the result, with a normal perpendicular to it.
            output.m_distance.m_storage = 0.0f;
            alignas(16) hkVector4 edge;
            edge.m_quad = _mm_sub_ps(m_simplex.m_storage[0].m_quad, m_simplex.m_storage[1].m_quad);
            // Lane choice: i0 and i1 swap where |x| > |y|; i2 becomes i1 where min(|x|, |y|) > |z|.
            float minAbs = std::fabs(edge(0));
            const float abs1 = std::fabs(edge(1));
            const float abs2 = std::fabs(edge(2));
            int32_t i0 = 1;
            int32_t i1 = 0;
            int32_t i2 = 2;
            if (minAbs > abs1) {
                minAbs = abs1;
                i0 = 0;
                i1 = 1;
            }
            if (minAbs > abs2) {
                i2 = i1;
            }
            output.m_normalInA.m_quad = _mm_setzero_ps();
            output.m_normalInA(i0) = edge(i2);
            output.m_normalInA(i2) = -edge(i0);
            output.m_normalInA.normalize3();
            hkVector4 abs;
            abs.setAbs4(m_simplex.m_storage[0]);
            const __m128 a = abs.m_quad;
            const float sumAbs =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(a, a, 0xAA), _mm_add_ss(_mm_shuffle_ps(a, a, 0x55), a)));
            const float t =
                static_cast<float>(sumAbs / (static_cast<double>(sumAbs) + sumAbs + 1.1920928955078125e-07));
            const hkpCdVertex* pointsA = m_pointsAinA.m_storage;
            output.m_pointAinA.setInterpolate4(pointsA[0], pointsA[1], t);
            gjkStatus = HK_GSK_OK;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 2;
    }

simplex2:
    {
        hkVector4 e;
        e.m_quad = _mm_sub_ps(m_simplex.m_storage[0].m_quad, m_simplex.m_storage[1].m_quad);
        const float lengthSq = e.dot3(e);
        if (m_epsTolSqrd.m_storage >= lengthSq) {
            if (dimension >= 1) {
                gjkStatus = HK_GSK_PD_DISTANCE_TOO_BIG;
                return HK_FAILURE;
            }
            m_simplexSize.m_storage = 1;
            goto simplex1;
        }
        // An edge: the support points along a direction perpendicular to it and along its cross with the edge; the
        // one further out is added.
        dimension = 2;
        alignas(16) hkVector4 edge;
        edge.m_quad = _mm_sub_ps(m_simplex.m_storage[0].m_quad, m_simplex.m_storage[1].m_quad);
        // Lane choice: i0 and i1 swap where |x| > |y|; i2 becomes i1 where min(|x|, |y|) > |z|.
        float minAbs = std::fabs(edge(0));
        const float abs1 = std::fabs(edge(1));
        const float abs2 = std::fabs(edge(2));
        int32_t i0 = 1;
        int32_t i1 = 0;
        int32_t i2 = 2;
        if (minAbs > abs1) {
            minAbs = abs1;
            i0 = 0;
            i1 = 1;
        }
        if (minAbs > abs2) {
            i2 = i1;
        }
        alignas(16) hkVector4 perpendicular;
        perpendicular.m_quad = _mm_setzero_ps();
        perpendicular(i0) = edge(i2);
        perpendicular(i2) = -edge(i0);
        alignas(16) hkVector4 crossDirection;
        crossDirection.setCross(edge, perpendicular);
        perpendicular.normalize3();
        crossDirection.normalize3();
        hkpWingedEdge::Vertex& vertex1 = vertexA;
        alignas(16) float distance1;
        findBestProjection(perpendicular, m_simplex.m_storage[0], vertex1, distance1);
        hkpWingedEdge::Vertex& vertex2 = vertexB;
        float distance2;
        findBestProjection(crossDirection, m_simplex.m_storage[0], vertex2, distance2);
        if (distance2 < distance1) {
            addPointToSimplex(vertex1);
        }
        else {
            addPointToSimplex(vertex2);
        }
        goto simplex3;
    }

simplex1:
    {
        const float lengthSq = m_simplex.m_storage[0].dot3(m_simplex.m_storage[0]);
        if (m_epsTolSqrd.m_storage >= lengthSq) {
            if (dimension >= 0) {
                gjkStatus = HK_GSK_PD_DISTANCE_TOO_BIG;
                return HK_FAILURE;
            }
            m_simplexSize.m_storage = 0;
            goto simplex0;
        }
        // A point: the support points along it and against it; the one further from it is added.
        const hkVector4* simplex = m_simplex.m_storage;
        dimension = 1;
        hkpWingedEdge::Vertex& vertexAlong = vertexB;
        getPointForSimplex(simplex[0], vertexAlong);
        hkVector4 dA;
        dA.m_quad = _mm_sub_ps(vertexAlong.m_position.m_quad, m_simplex.m_storage[0].m_quad);
        const float distanceAlongSq = dA.dot3(dA);
        alignas(16) hkVector4 negDirection;
        negDirection.setNeg4(simplex[0]);
        hkpWingedEdge::Vertex& vertexAgainst = vertexA;
        getPointForSimplex(negDirection, vertexAgainst);
        hkVector4 dB;
        dB.m_quad = _mm_sub_ps(vertexAgainst.m_position.m_quad, m_simplex.m_storage[0].m_quad);
        const float distanceAgainstSq = dB.dot3(dB);
        if (distanceAgainstSq <= distanceAlongSq) {
            addPointToSimplex(vertexAlong);
        }
        else {
            addPointToSimplex(vertexAgainst);
        }
        goto simplex2;
    }

simplex0:
    {
        const float translationSq = m_aTb.m_translation.dot3(m_aTb.m_translation);
        // The support point against B's offset, along y where the offset is within the tolerance.
        alignas(16) hkVector4 direction;
        direction.setNeg4(m_aTb.m_translation);
        if (m_epsTolSqrd.m_storage >= translationSq) {
            direction.m_quad = hkTransformIdentity.m_rotation.m_col1.m_quad;
        }
        hkpWingedEdge::Vertex& vertex = vertexA;
        getPointForSimplex(direction, vertex);
        addPointToSimplex(vertex);
        const float lengthSq = m_simplex.m_storage[0].dot3(m_simplex.m_storage[0]);
        if (m_epsTolSqrd.m_storage >= lengthSq) {
            // The shapes touch at that point.
            output.m_distance.m_storage = m_simplex.m_storage[0].length3();
            output.m_normalInA.m_quad = direction.m_quad;
            output.m_pointAinA.m_quad = vertex.m_pointAinA.m_quad;
            gjkStatus = HK_GSK_OK;
            return HK_FAILURE;
        }
        dimension = 0;
        goto simplex1;
    }
}

// On a retry after the first (m_backupTries 2..19) the random offset is written to x, y and z of a vector whose w is
// never written, and the whole vector is added to m_aTb.m_translation, so an uninitialised value reaches
// m_aTb.m_translation's w, as the game does. The status is reset to HK_GSK_OK before every builtTetrahedron.
hkpGskStatus hkpConvexPenetrationUtil::calculatePenetrationDepth(hkpGskOut& output) {
    hkpGskStatus gjkStatus;
    hkpWingedEdge we;
    hkpWingedEdge::Triangle* nearestTriangle;
    for (;;) {
        gjkStatus = HK_GSK_OK;
        if (builtTetrahedron(output, gjkStatus) == HK_FAILURE) {
            if (gjkStatus != HK_GSK_PD_DISTANCE_TOO_BIG) {
                return gjkStatus;
            }
        }
        else {
            we.initializePolytopeExpansion(m_pointsAinA.m_storage, m_pointsBinB.m_storage, m_simplex.m_storage);
            nearestTriangle = we.findTriangleOnNearestPlane();
            hkpWingedEdge::Vertex* newVertex = &we.m_vertex[we.m_numVertices.m_storage++];
            newVertex->m_referee.m_storage = 0;
            getPointForSimplex(nearestTriangle->m_normal, *newVertex);
            while (tryToExpandPolytope(we, nearestTriangle, newVertex, gjkStatus) != HK_FAILURE) {
                const int32_t numVertices = we.m_numVertices.m_storage;
                if (numVertices >= hkpWingedEdge::HK_WINGED_EDGE_VERTICIES_LIMIT) {
                    convertResultToFeature(we, nearestTriangle, output);
                    return HK_GSK_PD_OUT_OF_MEMORY;
                }
                nearestTriangle = we.findTriangleOnNearestPlane();
                ++we.m_numVertices.m_storage;
                newVertex = &we.m_vertex[numVertices];
                newVertex->m_referee.m_storage = 0;
                getPointForSimplex(nearestTriangle->m_normal, *newVertex);
            }
            if (gjkStatus != HK_GSK_PD_DISTANCE_TOO_BIG) {
                convertResultToFeature(we, nearestTriangle, output);
                return gjkStatus;
            }
        }

        const int32_t backupTries = ++m_backupTries.m_storage;
        if (backupTries == 1) {
            m_simplexSize.m_storage = 1;
        }
        else {
            if (backupTries >= 20) {
                return gjkStatus;
            }
            m_simplexSize.m_storage = 0;
            // EXE-INLINED 0xDF9B20
            alignas(16) hkVector4 offset;
            const float r = static_cast<float>(
                (hkpConvexPenetrationUtil_rand01(backupTries) - 9.999999747378752e-05) * 0.00019999999494757503
            );
            offset.m_quad[0] = r;
            offset.m_quad[1] = r;
            offset.m_quad[2] = r;
            m_aTb.m_translation.m_quad = _mm_add_ps(m_aTb.m_translation.m_quad, offset.m_quad);
        }
    }
}

void hkpConvexPenetrationUtil::init(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkTransform& aTb,
    float epsTolerance,
    hkpCdVertex* pointsAinA,
    hkpCdVertex* pointsBinB,
    hkVector4* simplex,
    int32_t simplexSize
) {
    hkVector4 v9;
    m_aTb.m_rotation = aTb.m_rotation;
    v9.m_quad = aTb.m_translation.m_quad;
    m_epsTolSqrd.m_storage = epsTolerance * epsTolerance;
    m_shapeA.m_storage = shapeA;
    m_shapeB.m_storage = shapeB;
    m_aTb.m_translation.m_quad = v9.m_quad;
    m_pointsAinA.m_storage = pointsAinA;
    m_pointsBinB.m_storage = pointsBinB;
    m_epsTol.m_quad = _mm_shuffle_ps(_mm_set_ss(epsTolerance), _mm_set_ss(epsTolerance), 0);
    m_backupTries.m_storage = 0;
    m_simplex.m_storage = simplex;
    m_simplexSize.m_storage = simplexSize;
}

// The edge vectors from the new vertex to the triangle's corners, the normal turned into their frame: when any of its
// x, y, z is below m_epsTol the vertex adds nothing. Otherwise the triangles it sees are replaced by a fan to it.
hkResult hkpConvexPenetrationUtil::tryToExpandPolytope(
    hkpWingedEdge& we,
    hkpWingedEdge::Triangle* nearestTriangle,
    hkpWingedEdge::Vertex* newVertex,
    hkpGskStatus& failureReason
) {
    alignas(16) hkRotation edges;
    const __m128 position = newVertex->m_position.m_quad;
    edges.m_col0.m_quad = _mm_sub_ps(position, nearestTriangle->m_edge[0].m_vertex->m_position.m_quad);
    edges.m_col1.m_quad = _mm_sub_ps(position, nearestTriangle->m_edge[1].m_vertex->m_position.m_quad);
    edges.m_col2.m_quad = _mm_sub_ps(position, nearestTriangle->m_edge[2].m_vertex->m_position.m_quad);
    alignas(16) hkVector4 normalInEdges;
    normalInEdges.setRotatedInverseDir(edges, nearestTriangle->m_normal);
    if ((_mm_movemask_ps(_mm_cmplt_ps(normalInEdges.m_quad, m_epsTol.m_quad)) & 7) != 0) {
        failureReason = HK_GSK_OK;
        return HK_FAILURE;
    }
    hkPadSpu<int32_t> numBoundaryEdges;
    hkPadSpu<hkpWingedEdge::Edge*> boundaryEdge;
    numBoundaryEdges.m_storage = 0;
    if (we.findVisibilityBoundaryEdges(nearestTriangle, newVertex->m_position, numBoundaryEdges, boundaryEdge) !=
        hkpWingedEdge::HK_WINGED_EDGE_OK) {
        failureReason = HK_GSK_PD_DISTANCE_TOO_BIG;
        return HK_FAILURE;
    }
    if (we.m_numFreeTriangle.m_storage - we.m_numTriangles.m_storage + hkpWingedEdge::HK_WINGED_EDGE_TRIANGLE_LIMIT <
        numBoundaryEdges.m_storage) {
        failureReason = HK_GSK_PD_OUT_OF_MEMORY;
        return HK_FAILURE;
    }
    if (we.patchInBoundaryToPointOfViewExpansion(numBoundaryEdges.m_storage, boundaryEdge.m_storage, newVertex) != 0) {
        failureReason = HK_GSK_PD_DISTANCE_TOO_BIG;
        return HK_FAILURE;
    }
    return HK_SUCCESS;
}

hkpWingedEdge::Triangle::TriangleState hkpWingedEdge::Triangle::calcStateAndUpdateTolerance(
    const hkVector4& arPoint,
    float* apToleranceBorder,
    float* apPreviousMinimumDistance
) {
    __m128 v4 = _mm_mul_ps(_mm_sub_ps(arPoint.m_quad, m_edge[0].m_vertex->m_position.m_quad), m_normal.m_quad);
    float v9 = _mm_shuffle_ps(v4, v4, 170)[0] + (_mm_shuffle_ps(v4, v4, 85)[0] + v4[0]);
    double v5 = v9;
    hkpWingedEdge::Triangle::TriangleState v6 = hkpWingedEdge::Triangle::HK_VISIBLE;
    if (!(*apToleranceBorder < static_cast<double>(v9))) {
        double v7 = v5 + 1.0000000116860974e-07;
        if (*apToleranceBorder < v7) {
            *apToleranceBorder = v7;
        }
        v6 = hkpWingedEdge::Triangle::HK_INVISIBLE;
    }
    else {
        if (*apPreviousMinimumDistance > v5) {
            *apPreviousMinimumDistance = v9;
        }
    }
    hkpWingedEdge::Triangle::TriangleState result = hkpWingedEdge::Triangle::HK_UNKNOWN;
    if (*apPreviousMinimumDistance > static_cast<double>(*apToleranceBorder)) {
        return v6;
    }
    return result;
}

hkpWingedEdge::WingedEdgeStatus hkpWingedEdge::findVisibilityBoundaryEdges(
    hkpWingedEdge::Triangle* initialTriangle,
    const hkVector4& pointOfView,
    hkPadSpu<int32_t>& chainLength,
    hkPadSpu<hkpWingedEdge::Edge*>& edgeChain
) {
    edgeChain.m_storage = 0;
    chainLength.m_storage = 0;
    m_toleranceBorder.m_storage = 1e-07f;
    m_previousMinimumDistance.m_storage = 3.40282e+38f;
    hkpWingedEdge::Triangle::TriangleState v6 = initialTriangle->calcStateAndUpdateTolerance(
        pointOfView, &m_toleranceBorder.m_storage, &m_previousMinimumDistance.m_storage
    );
    initialTriangle->m_state = v6;
    if (v6 == hkpWingedEdge::Triangle::HK_VISIBLE) {
        return findVisibleTriangles(initialTriangle, pointOfView, chainLength, edgeChain);
    }

    return hkpWingedEdge::HK_WINGED_EDGE_FAIL;
}
