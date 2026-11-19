#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

// The distances are computed as (y + x) + z and stored as floats; the rest is computed at double precision with float
// stores, so the double constants and casts below are deliberate. The depth difference is compared with 0.0 (not 1e-9),
// and the later best values start at that 0.0.
int32_t hkGskManifold_findRedundant5thPoint(hkpProcessCdPoint* resultPointArray) {
    const hkQuadReal vNew = resultPointArray[4].m_contact.m_position.m_quad;
    float fMaxDist = 0.0f;
    float fMinDist = 3.40282e+38f;
    int32_t aiUsed[5] = {0, 0, 0, 0, 0};
    float afDist[4];
    int32_t iMax = 0;
    int32_t iMin = 0;
    for (int32_t i = 0; i < 4; ++i) {
        hkQuadReal vSq = _mm_sub_ps(vNew, resultPointArray[i].m_contact.m_position.m_quad);
        vSq = _mm_mul_ps(vSq, vSq);
        const hkQuadReal vSum =
            _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0xaa), _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0x55), vSq));
        const float fD = _mm_cvtss_f32(vSum);
        afDist[i] = fD;
        if (fMaxDist < fD) {
            fMaxDist = fD;
            iMax = i;
        }
        if (fMinDist > fD) {
            fMinDist = fD;
            iMin = i;
        }
    }
    float fDepth = resultPointArray[4].m_contact.m_separatingNormal.m_quad[3] -
        resultPointArray[iMin].m_contact.m_separatingNormal.m_quad[3];
    aiUsed[iMax] = 1;
    const hkQuadReal vA = resultPointArray[iMax].m_contact.m_position.m_quad;
    float fBest = static_cast<float>(fMaxDist * 1.0499999523162842);
    int32_t iSecond = 4;
    for (int32_t i = 0; i < 5; ++i) {
        if (aiUsed[i] == 0) {
            hkQuadReal vSq = _mm_sub_ps(vA, resultPointArray[i].m_contact.m_position.m_quad);
            vSq = _mm_mul_ps(vSq, vSq);
            const hkQuadReal vSum =
                _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0xaa), _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0x55), vSq));
            const float fD = _mm_cvtss_f32(vSum);
            if (fBest < fD) {
                fBest = fD;
                iSecond = i;
            }
        }
    }
    fBest = 0.0f;
    aiUsed[iSecond] = 1;
    const hkQuadReal vEdge = _mm_sub_ps(vA, resultPointArray[iSecond].m_contact.m_position.m_quad);
    int32_t iThird = 0;
    for (int32_t i = 0; i < 5; ++i) {
        if (aiUsed[i] == 0) {
            const hkQuadReal vRel = _mm_sub_ps(resultPointArray[i].m_contact.m_position.m_quad, vA);
            hkQuadReal vSq = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(vRel, vRel, 0xc9), _mm_shuffle_ps(vEdge, vEdge, 0xd2)),
                _mm_mul_ps(_mm_shuffle_ps(vRel, vRel, 0xd2), _mm_shuffle_ps(vEdge, vEdge, 0xc9))
            );
            vSq = _mm_mul_ps(vSq, vSq);
            const hkQuadReal vSum =
                _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0xaa), _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0x55), vSq));
            const float fD = _mm_cvtss_f32(vSum);
            if (fBest < fD) {
                fBest = fD;
                iThird = i;
            }
        }
    }
    aiUsed[iThird] = 1;
    fBest = 0.0f;
    const hkQuadReal vC = resultPointArray[iThird].m_contact.m_position.m_quad;
    int32_t iFourth = 0;
    for (int32_t i = 0; i < 5; ++i) {
        if (aiUsed[i] == 0) {
            hkQuadReal vSq = _mm_sub_ps(vC, resultPointArray[i].m_contact.m_position.m_quad);
            vSq = _mm_mul_ps(vSq, vSq);
            const hkQuadReal vSum =
                _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0xaa), _mm_add_ss(_mm_shuffle_ps(vSq, vSq, 0x55), vSq));
            const float fD = _mm_cvtss_f32(vSum);
            if (fBest < fD) {
                fBest = fD;
                iFourth = i;
            }
        }
    }
    aiUsed[iFourth] = 1;
    // The new point was not picked, it lies deeper than the closest kept point (fDepth < 0), and that point is
    // close to it: drop the point whose depth gain over its distance is largest.
    if (aiUsed[4] == 0 && fDepth < 0.0f &&
        static_cast<double>(fMinDist) < static_cast<double>(fDepth) * 526.0 * static_cast<double>(fDepth)) {
        const double dEps = 9.999999717180685e-10;
        int32_t iResult = 4;
        float fRatio = 0.030461745f;
        float fNewDepth = resultPointArray[4].m_contact.m_separatingNormal.m_quad[3];
        float fDiff = resultPointArray[0].m_contact.m_separatingNormal.m_quad[3] - fNewDepth;
        float fSq = fDiff * fDiff;
        float fT = static_cast<float>(fSq + dEps);
        if (0.030461745336651802 * afDist[0] < fT) {
            iResult = 0;
            fRatio = static_cast<float>(fT / (afDist[0] + dEps));
        }
        fDiff = resultPointArray[1].m_contact.m_separatingNormal.m_quad[3] - fNewDepth;
        fSq = fDiff * fDiff;
        fT = static_cast<float>(fSq + dEps);
        if (static_cast<double>(afDist[1]) * fRatio < fT) {
            iResult = 1;
            fRatio = static_cast<float>(fT / (afDist[1] + dEps));
        }
        fDiff = resultPointArray[2].m_contact.m_separatingNormal.m_quad[3] - fNewDepth;
        fSq = fDiff * fDiff;
        fT = static_cast<float>(fSq + dEps);
        if (static_cast<double>(afDist[2]) * fRatio < fT) {
            iResult = 2;
            fRatio = static_cast<float>(fT / (afDist[2] + dEps));
        }
        fDiff = resultPointArray[3].m_contact.m_separatingNormal.m_quad[3] - fNewDepth;
        fSq = fDiff * fDiff;
        fT = static_cast<float>(fSq + dEps);
        if (static_cast<double>(afDist[3]) * fRatio < fT) {
            iResult = 3;
        }
        return iResult;
    }
    for (int32_t i = 0; i < 5; ++i) {
        if (aiUsed[i] == 0) {
            return i;
        }
    }
    return 0;
}

// A contact point with newPoint's dimensions and vertex ids is swapped to the front and true returned. Otherwise, where
// newPoint has a single vertex on A or B, the first point with that same single vertex there is cleared (its dimensions
// zeroed), and false returned. The vertex ids follow the used contact points; each m_vert entry is a byte offset into
// them, shifted left by 3. That id area is raw bytes addressed by byte offset (hkpGskManifold::getVertexIds), hence the
// casts.
bool hkGskManifold_doesPointExistAndResort(hkpGskManifold& manifold, const hkpGskCache& newPoint) {
    for (uint32_t i = 0; i < manifold.m_numContactPoints; ++i) {
        const hkpGskManifold::ContactPoint& cp = manifold.m_contactPoints[i];
        if (newPoint.m_dimA != cp.m_dimA || newPoint.m_dimB != cp.m_dimB) {
            continue;
        }
        int32_t numVerts = newPoint.m_dimA + newPoint.m_dimB;
        // EXE-INLINED none: hkAddByteOffset
        const char* vertexIds = reinterpret_cast<const char*>(manifold.getVertexIds());
        if (newPoint.m_vertices[0] != *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[0] >> 3))) {
            continue;
        }
        if (newPoint.m_vertices[1] != *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[1] >> 3))) {
            continue;
        }
        if (numVerts > 2 &&
            newPoint.m_vertices[2] != *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[2] >> 3))) {
            continue;
        }
        if (numVerts > 3 &&
            newPoint.m_vertices[3] != *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[3] >> 3))) {
            continue;
        }
        if (i != 0) {
            hkpGskManifold::ContactPoint h = manifold.m_contactPoints[i];
            manifold.m_contactPoints[i] = manifold.m_contactPoints[0];
            manifold.m_contactPoints[0] = h;
        }
        return true;
    }
    if (newPoint.m_dimA == 1 || newPoint.m_dimB == 1) {
        uint32_t numPoints = manifold.m_numContactPoints;
        // EXE-INLINED none: hkAddByteOffset
        const char* vertexIds = reinterpret_cast<const char*>(manifold.getVertexIds());
        for (uint32_t i = 0; i < numPoints; ++i) {
            hkpGskManifold::ContactPoint& cp = manifold.m_contactPoints[i];
            uint8_t dimA = newPoint.m_dimA;
            if ((dimA == 1 && cp.m_dimA == 1 &&
                 *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[0] >> 3)) == newPoint.m_vertices[0]) ||
                (newPoint.m_dimB == 1 && cp.m_dimB == 1 &&
                 *reinterpret_cast<const uint16_t*>(vertexIds + (cp.m_vert[cp.m_dimA] >> 3)) ==
                     newPoint.m_vertices[dimA])) {
                cp.m_dimB = 0;
                cp.m_dimA = 0;
                return false;
            }
        }
    }
    return false;
}

// KEEP: uninitialised reads. The remap loop writes remap[] only for the m_numVertsA + m_numVertsB vertex ids, and
// the exe then reads remap[m_numVertsA] (0xDAE2D6), never written when m_numVertsB is 0; the second pass remaps all
// four m_vert bytes of each point (0xDAE2F7), also those past the point's dimensions, which may index entries the
// loop did not write. Usercall (0xDAE220, ret 0): manifold in edx.
void hkGskManifold_garbageCollect(hkpGskManifold& manifold) {
    alignas(16) uint8_t used[16] = {};
    int32_t remap[16];

    // Mark the vertex ids the points use; every point uses at least two.
    const int32_t numPoints = manifold.m_numContactPoints;
    for (int32_t i = 0; i < numPoints; ++i) {
        const hkpGskManifold::ContactPoint& cp = manifold.m_contactPoints[i];
        const int32_t dim = cp.m_dimA + cp.m_dimB;
        used[cp.m_vert[0] >> 4] = 1;
        used[cp.m_vert[1] >> 4] = 1;
        if (dim >= 3) {
            used[cp.m_vert[2] >> 4] = 1;
        }
        if (dim == 4) {
            used[cp.m_vert[3] >> 4] = 1;
        }
    }

    // Pack the used vertex ids to the front; remap[] holds each old id's new byte offset (16 per vertex).
    uint16_t* vertexIds = manifold.getVertexIds();
    const int32_t numVerts = manifold.m_numVertsA + manifold.m_numVertsB;
    int32_t numUsed = 0;
    int32_t offset = 0;
    for (int32_t v = 0; v < numVerts; ++v) {
        remap[v] = offset;
        if (used[v]) {
            vertexIds[numUsed] = vertexIds[v];
            ++numUsed;
            offset += 16;
        }
    }
    const uint8_t numVertsA = static_cast<uint8_t>(static_cast<uint32_t>(remap[manifold.m_numVertsA]) >> 4);
    manifold.m_numVertsA = numVertsA;
    manifold.m_numVertsB = static_cast<uint8_t>(numUsed - numVertsA);

    for (uint32_t i = 0; i < manifold.m_numContactPoints; ++i) {
        hkpGskManifold::ContactPoint& cp = manifold.m_contactPoints[i];
        cp.m_vert[0] = static_cast<uint8_t>(remap[cp.m_vert[0] >> 4]);
        cp.m_vert[1] = static_cast<uint8_t>(remap[cp.m_vert[1] >> 4]);
        cp.m_vert[2] = static_cast<uint8_t>(remap[cp.m_vert[2] >> 4]);
        cp.m_vert[3] = static_cast<uint8_t>(remap[cp.m_vert[3] >> 4]);
    }
}

// The vertex ids sit right after the contact points, so dropping a point moves them down by one point. That span (the
// old last point and the ids after it, into m_padding) is treated as untyped 32-bit words and copied a word at a time,
// hence the casts.
void hkGskManifold_removePoint(hkpGskManifold& manifold, int32_t index) {
    // Indexes the last point with the 32-bit count - 1, not the byte stored back (they differ when the count was 0), as
    // the game does.
    const int32_t last = manifold.m_numContactPoints - 1;
    manifold.m_numContactPoints = static_cast<uint8_t>(last);
    hkpGskManifold::ContactPoint* pLast = &manifold.m_contactPoints[last];
    manifold.m_contactPoints[index] = *pLast;
    const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(pLast + 1);
    uint32_t* pDst = reinterpret_cast<uint32_t*>(pLast);
    for (int32_t i = (manifold.m_numVertsA + manifold.m_numVertsB - 1) >> 1; i >= 0; --i) {
        *pDst++ = *pSrc++;
    }
    hkGskManifold_garbageCollect(manifold);
}

// Each contact point from firstPointIndex on is turned into a contact (its dimA * 4 + dimB picks the feature pair) and
// appended to processOut when it is still within the keep distance; any other point is dropped (its contact id removed,
// the last point moved into its place). When points were dropped, the vertex ids after the points move down and the
// unused ones are collected. A vertex id is a byte offset into work.m_vertices, and the ids after the points are
// untyped 32-bit words copied one word at a time, hence the casts.
void hkGskManifold_verifyAndGetPoints(
    hkpGskManifold& manifold,
    const hkpGskManifoldWork& work,
    int32_t firstPointIndex,
    hkpProcessCollisionOutput& processOut,
    hkpContactMgr* contactMgr
) {
    const uint32_t numPointsBefore = manifold.m_numContactPoints;
    uint32_t numPoints = numPointsBefore;
    uint32_t index = static_cast<uint32_t>(firstPointIndex);
    if (index >= numPoints) {
        return;
    }
    hkpGskManifold::ContactPoint* const pOldEnd = &manifold.m_contactPoints[numPoints];
    hkpGskManifold::ContactPoint* pLast = pOldEnd;
    hkpGskManifold::ContactPoint* cp = &manifold.m_contactPoints[index];
    // EXE-INLINED none: hkAddByteOffsetConst
    const char* pVertexBytes = reinterpret_cast<const char*>(work.m_vertices);
    do {
        hkQuadReal pointA = _mm_setzero_ps();
        hkQuadReal aMinusB = pointA;
        hkQuadReal normal = pointA;
        int32_t invalid = 1;
        // Cases 7 and 13 (a vertex against a triangle) end alike: the triangle's unit normal and the four edge
        // products, whose lane sums say whether the point lies inside the triangle.
        bool bTriangle = false;
        hkQuadReal triNormal;
        hkQuadReal edges01Lo;
        hkQuadReal edges23Lo;
        hkQuadReal edges01Hi;
        hkQuadReal edges23Hi;
        // EXE-INLINED 0xDAD990
        switch (cp->m_dimB + 4 * cp->m_dimA) {
            case 5: {
                // Vertex against vertex.
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                const hkQuadReal diff = _mm_sub_ps(rA0, rB0);
                const hkQuadReal diff2 = _mm_mul_ps(diff, diff);
                const float fLen2 = (diff2[2] + static_cast<float>(diff2[1] + diff2[0]));
                if (work.m_radiusSumSqrd.m_storage < static_cast<double>(fLen2)) {
                    break;
                }
                normal = work.m_masterNormal.m_quad;
                aMinusB = diff;
                pointA = rA0;
                invalid = 0;
                break;
            }
            case 6: {
                // Vertex of A against an edge of B.
                const hkQuadReal& rB1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[2]);
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                pointA = rA0;
                const hkQuadReal edge = _mm_sub_ps(rB1, rB0);
                const hkQuadReal toA = _mm_sub_ps(rB1, pointA);
                const hkQuadReal cross = _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(toA, toA, 210), _mm_shuffle_ps(edge, edge, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(toA, toA, 201), _mm_shuffle_ps(edge, edge, 210))
                );
                const hkQuadReal cross2 = _mm_mul_ps(cross, cross);
                const hkQuadReal prod1 = _mm_mul_ps(toA, edge);
                normal = work.m_masterNormal.m_quad;
                const hkQuadReal prod0 = _mm_mul_ps(_mm_sub_ps(pointA, rB0), edge);
                const hkQuadReal lo = _mm_unpacklo_ps(prod1, prod0);
                const hkQuadReal mixed = _mm_movelh_ps(lo, cross2);
                const hkQuadReal edge2 = _mm_mul_ps(edge, edge);
                hkQuadReal sums = _mm_add_ps(
                    _mm_shuffle_ps(_mm_unpackhi_ps(prod1, prod0), cross2, 228),
                    _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(mixed, lo), cross2, 212), mixed)
                );
                const float fEdgeLen2 = (edge2[2] + static_cast<float>(edge2[1] + edge2[0]));
                if (work.m_radiusSumSqrd.m_storage * fEdgeLen2 < sums[3] ||
                    (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
                    break;
                }
                sums[0] = sums[0] / (sums[0] + sums[1]);
                hkVector4 pointB;
                pointB.setInterpolate4(
                    reinterpret_cast<const hkVector4&>(rB0), reinterpret_cast<const hkVector4&>(rB1), hkSimdReal(sums)
                );
                aMinusB = _mm_sub_ps(pointA, pointB.m_quad);
                invalid = 0;
                break;
            }
            case 7: {
                // Vertex of A against a triangle of B.
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                const hkQuadReal& rB1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[2]);
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal& rB2 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[3]);
                pointA = rA0;
                const hkQuadReal e12 = _mm_sub_ps(rB2, rB1);
                const hkQuadReal e12Yzx = _mm_shuffle_ps(e12, e12, 201);
                const hkQuadReal e12Zxy = _mm_shuffle_ps(e12, e12, 210);
                const hkQuadReal e20 = _mm_sub_ps(rB0, rB2);
                const hkQuadReal e20Yzx = _mm_shuffle_ps(e20, e20, 201);
                const hkQuadReal e20Zxy = _mm_shuffle_ps(e20, e20, 210);
                const hkQuadReal e01 = _mm_sub_ps(rB1, rB0);
                aMinusB = _mm_sub_ps(pointA, rB0);
                const hkQuadReal cross = _mm_sub_ps(_mm_mul_ps(e12Yzx, e20Zxy), _mm_mul_ps(e12Zxy, e20Yzx));
                hkVector4 normalized;
                normalized.m_quad = cross;
                normalized.normalize3();
                triNormal = normalized.m_quad;
                const hkQuadReal toB1 = _mm_sub_ps(pointA, rB1);
                const hkQuadReal toB2 = _mm_sub_ps(pointA, rB2);
                const hkQuadReal c1 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(toB1, toB1, 201), e12Zxy),
                        _mm_mul_ps(_mm_shuffle_ps(toB1, toB1, 210), e12Yzx)
                    ),
                    triNormal
                );
                const hkQuadReal c2 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(toB2, toB2, 201), e20Zxy),
                        _mm_mul_ps(_mm_shuffle_ps(toB2, toB2, 210), e20Yzx)
                    ),
                    triNormal
                );
                const hkQuadReal c0 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(aMinusB, aMinusB, 201), _mm_shuffle_ps(e01, e01, 210)),
                        _mm_mul_ps(_mm_shuffle_ps(aMinusB, aMinusB, 210), _mm_shuffle_ps(e01, e01, 201))
                    ),
                    triNormal
                );
                const hkQuadReal master = _mm_mul_ps(work.m_masterNormal.m_quad, triNormal);
                edges01Lo = _mm_shuffle_ps(c1, c2, 68);
                edges23Lo = _mm_shuffle_ps(c0, master, 68);
                edges01Hi = _mm_shuffle_ps(c1, c2, 238);
                edges23Hi = _mm_shuffle_ps(c0, master, 238);
                bTriangle = true;
                break;
            }
            case 9: {
                // Edge of A against a vertex of B.
                const hkQuadReal& rA1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal edge = _mm_sub_ps(rA1, rA0);
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[2]);
                const hkQuadReal toB = _mm_sub_ps(rA1, rB0);
                const hkQuadReal cross = _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(toB, toB, 210), _mm_shuffle_ps(edge, edge, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(toB, toB, 201), _mm_shuffle_ps(edge, edge, 210))
                );
                const hkQuadReal prod0 = _mm_mul_ps(_mm_sub_ps(rB0, rA0), edge);
                const hkQuadReal cross2 = _mm_mul_ps(cross, cross);
                const hkQuadReal prod1 = _mm_mul_ps(toB, edge);
                normal = work.m_masterNormal.m_quad;
                const hkQuadReal lo = _mm_unpacklo_ps(prod1, prod0);
                const hkQuadReal mixed = _mm_movelh_ps(lo, cross2);
                const hkQuadReal edge2 = _mm_mul_ps(edge, edge);
                hkQuadReal sums = _mm_add_ps(
                    _mm_shuffle_ps(_mm_unpackhi_ps(prod1, prod0), cross2, 228),
                    _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(mixed, lo), cross2, 212), mixed)
                );
                const float fEdgeLen2 = (edge2[2] + static_cast<float>(edge2[1] + edge2[0]));
                if (work.m_radiusSumSqrd.m_storage * fEdgeLen2 < sums[3] ||
                    (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
                    break;
                }
                sums[0] = sums[0] / (sums[0] + sums[1]);
                hkVector4 interpolated;
                interpolated.setInterpolate4(
                    reinterpret_cast<const hkVector4&>(rA1), reinterpret_cast<const hkVector4&>(rA0), hkSimdReal(sums)
                );
                pointA = interpolated.m_quad;
                aMinusB = _mm_sub_ps(pointA, rB0);
                invalid = 0;
                break;
            }
            case 10: {
                // Edge of A against an edge of B.
                const hkQuadReal& rB1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[3]);
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal& rA1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[2]);
                const hkQuadReal edgeA = _mm_sub_ps(rA1, rA0);
                const hkQuadReal a1MinusB1 = _mm_sub_ps(rA1, rB1);
                const hkQuadReal edgeB = _mm_sub_ps(rB1, rB0);
                const hkQuadReal edgeBYzx = _mm_shuffle_ps(edgeB, edgeB, 201);
                const hkQuadReal edgeAZxy = _mm_shuffle_ps(edgeA, edgeA, 210);
                const hkQuadReal a0MinusB0 = _mm_sub_ps(rA0, rB0);
                const hkQuadReal edgeBZxy = _mm_shuffle_ps(edgeB, edgeB, 210);
                const hkQuadReal edgeAYzx = _mm_shuffle_ps(edgeA, edgeA, 201);
                const hkQuadReal cross = _mm_sub_ps(_mm_mul_ps(edgeAYzx, edgeBZxy), _mm_mul_ps(edgeAZxy, edgeBYzx));
                const hkQuadReal crossYzx = _mm_shuffle_ps(cross, cross, 201);
                const hkQuadReal crossZxy = _mm_shuffle_ps(cross, cross, 210);
                const hkQuadReal planeB = _mm_sub_ps(_mm_mul_ps(crossZxy, edgeBYzx), _mm_mul_ps(crossYzx, edgeBZxy));
                const hkQuadReal planeA = _mm_sub_ps(_mm_mul_ps(crossZxy, edgeAYzx), _mm_mul_ps(crossYzx, edgeAZxy));
                const hkQuadReal pB1 = _mm_mul_ps(planeB, a1MinusB1);
                const hkQuadReal pB0 = _mm_mul_ps(planeB, a0MinusB0);
                const hkQuadReal pA1 = _mm_mul_ps(planeA, a1MinusB1);
                const hkQuadReal pA0 = _mm_mul_ps(planeA, a0MinusB0);
                const hkQuadReal lo1 = _mm_shuffle_ps(pB1, pA1, 68);
                const hkQuadReal hi1 = _mm_shuffle_ps(pB1, pA1, 238);
                const hkQuadReal lo0 = _mm_shuffle_ps(pB0, pA0, 68);
                hkQuadReal sums = _mm_add_ps(
                    _mm_shuffle_ps(_mm_shuffle_ps(pB0, pA0, 238), hi1, 136),
                    _mm_add_ps(_mm_shuffle_ps(lo0, lo1, 221), _mm_shuffle_ps(lo0, lo1, 136))
                );
                const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps()));
                normal = work.m_masterNormal.m_quad;
                if (signs != 3 && signs != 12) {
                    break;
                }
                hkQuadReal tA = sums;
                tA[0] = sums[0] / (sums[0] - sums[2]);
                hkVector4 interpolatedA;
                interpolatedA.setInterpolate4(
                    reinterpret_cast<const hkVector4&>(rA0), reinterpret_cast<const hkVector4&>(rA1), hkSimdReal(tA)
                );
                pointA = interpolatedA.m_quad;
                hkQuadReal tB = _mm_shuffle_ps(sums, sums, 85);
                tB[0] = tB[0] / (tB[0] - sums[3]);
                hkVector4 pointB;
                pointB.setInterpolate4(
                    reinterpret_cast<const hkVector4&>(rB0), reinterpret_cast<const hkVector4&>(rB1), hkSimdReal(tB)
                );
                aMinusB = _mm_sub_ps(pointA, pointB.m_quad);
                invalid = 0;
                break;
            }
            case 13: {
                // Triangle of A against a vertex of B.
                const hkQuadReal& rA2 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[2]);
                const hkQuadReal& rA1 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[1]);
                const hkQuadReal& rA0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[0]);
                const hkQuadReal& rB0 = *reinterpret_cast<const hkQuadReal*>(pVertexBytes + cp->m_vert[3]);
                const hkQuadReal e20 = _mm_sub_ps(rA0, rA2);
                const hkQuadReal e12 = _mm_sub_ps(rA2, rA1);
                const hkQuadReal e12Yzx = _mm_shuffle_ps(e12, e12, 201);
                const hkQuadReal e01 = _mm_sub_ps(rA1, rA0);
                const hkQuadReal e20Zxy = _mm_shuffle_ps(e20, e20, 210);
                const hkQuadReal e20Yzx = _mm_shuffle_ps(e20, e20, 201);
                const hkQuadReal e12Zxy = _mm_shuffle_ps(e12, e12, 210);
                const hkQuadReal cross = _mm_sub_ps(_mm_mul_ps(e12Yzx, e20Zxy), _mm_mul_ps(e12Zxy, e20Yzx));
                hkVector4 normalized;
                normalized.m_quad = cross;
                normalized.normalize3();
                const hkQuadReal fromA2 = _mm_sub_ps(rB0, rA2);
                const hkQuadReal fromA0 = _mm_sub_ps(rB0, rA0);
                aMinusB = _mm_sub_ps(rA0, rB0);
                triNormal = normalized.m_quad;
                const hkQuadReal fromA1 = _mm_sub_ps(rB0, rA1);
                const hkQuadReal c1 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(fromA1, fromA1, 201), e12Zxy),
                        _mm_mul_ps(_mm_shuffle_ps(fromA1, fromA1, 210), e12Yzx)
                    ),
                    triNormal
                );
                const hkQuadReal master = _mm_mul_ps(work.m_masterNormal.m_quad, triNormal);
                const hkQuadReal c2 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(fromA2, fromA2, 201), e20Zxy),
                        _mm_mul_ps(_mm_shuffle_ps(fromA2, fromA2, 210), e20Yzx)
                    ),
                    triNormal
                );
                edges01Lo = _mm_shuffle_ps(c1, c2, 68);
                const hkQuadReal c0 = _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(e01, e01, 210), _mm_shuffle_ps(fromA0, fromA0, 201)),
                        _mm_mul_ps(_mm_shuffle_ps(e01, e01, 201), _mm_shuffle_ps(fromA0, fromA0, 210))
                    ),
                    triNormal
                );
                edges23Hi = _mm_shuffle_ps(c0, master, 238);
                edges23Lo = _mm_shuffle_ps(c0, master, 68);
                const hkQuadReal depth = _mm_mul_ps(triNormal, aMinusB);
                const float fDepth = (depth[2] + static_cast<float>(depth[1] + depth[0]));
                pointA = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fDepth), triNormal), rB0);
                edges01Hi = _mm_shuffle_ps(c1, c2, 238);
                bTriangle = true;
                break;
            }
            default: break;
        }
        if (bTriangle) {
            const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(
                _mm_add_ps(
                    _mm_shuffle_ps(edges01Hi, edges23Hi, 136),
                    _mm_add_ps(_mm_shuffle_ps(edges01Lo, edges23Lo, 221), _mm_shuffle_ps(edges01Lo, edges23Lo, 136))
                ),
                _mm_setzero_ps()
            ));
            normal = triNormal;
            if ((signs & 8) != 0) {
                normal = _mm_xor_ps(triNormal, hkQuadRealNegateMask);
            }
            invalid = ~signs & 7;
        }
        if (invalid == 0) {
            const hkQuadReal prod = _mm_mul_ps(normal, aMinusB);
            const float fDist = (prod[2] + static_cast<float>(prod[1] + prod[0]));
            const float fOffsetB = work.m_radiusB.m_storage - fDist;
            const float fRadiusSum = work.m_radiusA.m_storage + work.m_radiusB.m_storage;
            const float fDistance = fDist - fRadiusSum;
            if (work.m_keepContact.m_storage > fDistance) {
                hkpProcessCdPoint* pOut = processOut.m_firstFreeContactPoint.m_storage;
                pOut->m_contact.m_separatingNormal.m_quad = normal;
                pOut->m_contact.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fOffsetB), normal), pointA);
                pOut->m_contact.m_separatingNormal.m_quad[3] = fDistance;
                pOut->m_contactPointId.m_storage = cp->m_id;
                processOut.m_firstFreeContactPoint.m_storage = pOut + 1;
                ++cp;
                ++index;
                continue;
            }
        }
        // The point is gone: its contact goes and the last point takes its slot, which is verified next.
        const uint16_t id = cp->m_id;
        if (id != HK_INVALID_CONTACT_POINT) {
            contactMgr->removeContactPoint(id, *processOut.m_constraintOwner.m_storage);
        }
        --pLast;
        *cp = *pLast;
        --numPoints;
    }
    while (index < numPoints);
    if (numPointsBefore > numPoints) {
        manifold.m_numContactPoints = static_cast<uint8_t>(numPoints);
        uint32_t* pDst = reinterpret_cast<uint32_t*>(&manifold.m_contactPoints[numPoints]);
        const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(pOldEnd);
        for (int32_t i = (manifold.m_numVertsB + manifold.m_numVertsA - 1) >> 1; i >= 0; --i) {
            *pDst++ = *pSrc++;
        }
        hkGskManifold_garbageCollect(manifold);
    }
}

// The per-kind conversions (convertPointPoint, convertPointEdge, convertEdgePoint above, and the point-face, edge-edge
// and face-point ones) are handled in a switch on dimA * 4 + dimB, each writing out in the game's order (some fields
// before the validity test). The vertex ids are byte offsets of 16-byte vertices. The face cases return the lane mask
// of the edges the point lies inside, with the normal flipped toward the master normal.
int32_t hkGskManifold_convertFeature(
    const hkpGskManifoldWork& work,
    hkGskManifoldConvertOut& out,
    const hkpGskManifold::ContactPoint& feature
) {
    // The vertex ids are byte offsets into work.m_vertices, added to the array's address as raw memory.
    // EXE-INLINED none: hkAddByteOffsetConst
    const char* vertexBytes = reinterpret_cast<const char*>(work.m_vertices);
    switch (feature.m_dimB + 4 * feature.m_dimA) {
        case 5: {
            // EXE-INLINED 0xDAD6A0
            const __m128 pA = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0])->m_quad;
            const __m128 aMinusB =
                _mm_sub_ps(pA, reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1])->m_quad);
            const __m128 sq = _mm_mul_ps(aMinusB, aMinusB);
            const float distanceSq =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (work.m_radiusSumSqrd.m_storage < distanceSq) {
                break;
            }
            out.m_AminusB.m_quad = aMinusB;
            out.m_normal.m_quad = work.m_masterNormal.m_quad;
            out.m_pointOnA.m_quad = pA;
            return HK_FEATURE_OK;
        }
        case 6: {
            // EXE-INLINED 0xDAD720
            const hkVector4& a = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
            const hkVector4& b0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
            const hkVector4& b1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
            out.m_pointOnA.m_quad = a.m_quad;
            out.m_normal.m_quad = work.m_masterNormal.m_quad;
            const __m128 pB0 = b0.m_quad;
            const __m128 pA = a.m_quad;
            const __m128 pB1 = b1.m_quad;
            const __m128 b1MinusA = _mm_sub_ps(pB1, pA);
            const __m128 edge = _mm_sub_ps(pB1, pB0);
            const __m128 cross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(b1MinusA, b1MinusA, 0xD2), _mm_shuffle_ps(edge, edge, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(b1MinusA, b1MinusA, 0xC9), _mm_shuffle_ps(edge, edge, 0xD2))
            );
            const __m128 dotsB1 = _mm_mul_ps(b1MinusA, edge);
            const __m128 crossSq = _mm_mul_ps(cross, cross);
            const __m128 dotsB0 = _mm_mul_ps(_mm_sub_ps(pA, pB0), edge);
            const __m128 hi = _mm_unpackhi_ps(dotsB1, dotsB0);
            const __m128 lo = _mm_unpacklo_ps(dotsB1, dotsB0);
            const __m128 loCross = _mm_movelh_ps(lo, crossSq);
            const __m128 sums = _mm_add_ps(
                _mm_shuffle_ps(hi, crossSq, 0xE4),
                _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(loCross, lo), crossSq, 0xD4), loCross)
            );
            const __m128 edgeSq = _mm_mul_ps(edge, edge);
            const float edgeLengthSq = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(edgeSq, edgeSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0x55), edgeSq)
            ));
            if (work.m_radiusSumSqrd.m_storage * edgeLengthSq < sums[3] ||
                (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
                break;
            }
            __m128 t = _mm_div_ss(sums, _mm_add_ss(sums, _mm_shuffle_ps(sums, sums, 0x55)));
            t = _mm_shuffle_ps(t, t, 0);
            out.m_AminusB.m_quad =
                _mm_sub_ps(pA, _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pB0), _mm_mul_ps(t, pB1)));
            return HK_FEATURE_OK;
        }
        case 7: {
            // The point A against the triangle B0 B1 B2.
            const __m128 pA = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0])->m_quad;
            out.m_pointOnA.m_quad = pA;
            out.m_AminusB.m_quad =
                _mm_sub_ps(pA, reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1])->m_quad);
            const __m128 pB2 = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[3])->m_quad;
            const __m128 pB1 = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2])->m_quad;
            const __m128 pB0 = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1])->m_quad;
            const __m128 e20 = _mm_sub_ps(pB0, pB2);
            const __m128 e20Yzx = _mm_shuffle_ps(e20, e20, 201);
            const __m128 e12 = _mm_sub_ps(pB2, pB1);
            const __m128 e12Yzx = _mm_shuffle_ps(e12, e12, 201);
            const __m128 e12Zxy = _mm_shuffle_ps(e12, e12, 210);
            const __m128 e20Zxy = _mm_shuffle_ps(e20, e20, 210);
            const __m128 e01 = _mm_sub_ps(pB1, pB0);
            const __m128 cross = _mm_sub_ps(_mm_mul_ps(e12Yzx, e20Zxy), _mm_mul_ps(e12Zxy, e20Yzx));
            hkVector4 normalized;
            normalized.m_quad = cross;
            normalized.normalize3();
            const __m128 triNormal = normalized.m_quad;
            const __m128 toB2 = _mm_sub_ps(pA, pB2);
            const __m128 toB1 = _mm_sub_ps(pA, pB1);
            const __m128 toB0 = _mm_sub_ps(pA, pB0);
            const __m128 c0 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(e01, e01, 210), _mm_shuffle_ps(toB0, toB0, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(e01, e01, 201), _mm_shuffle_ps(toB0, toB0, 210))
                ),
                triNormal
            );
            const __m128 c1 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(toB1, toB1, 201), e12Zxy),
                    _mm_mul_ps(_mm_shuffle_ps(toB1, toB1, 210), e12Yzx)
                ),
                triNormal
            );
            const __m128 c2 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(toB2, toB2, 201), e20Zxy),
                    _mm_mul_ps(_mm_shuffle_ps(toB2, toB2, 210), e20Yzx)
                ),
                triNormal
            );
            const __m128 master = _mm_mul_ps(work.m_masterNormal.m_quad, triNormal);
            const __m128 edgesLo01 = _mm_shuffle_ps(c1, c2, 68);
            const __m128 edgesLo23 = _mm_shuffle_ps(c0, master, 68);
            const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(
                _mm_add_ps(
                    _mm_shuffle_ps(_mm_shuffle_ps(c1, c2, 238), _mm_shuffle_ps(c0, master, 238), 136),
                    _mm_add_ps(_mm_shuffle_ps(edgesLo01, edgesLo23, 221), _mm_shuffle_ps(edgesLo01, edgesLo23, 136))
                ),
                _mm_setzero_ps()
            ));
            out.m_normal.m_quad = triNormal;
            if ((signs & 8) != 0) {
                out.m_normal.m_quad = _mm_xor_ps(triNormal, hkQuadRealNegateMask);
            }
            return ~signs & 7;
        }
        case 9: {
            // EXE-INLINED 0xDAD860
            const hkVector4& a0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
            const hkVector4& a1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
            const hkVector4& b = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
            out.m_normal.m_quad = work.m_masterNormal.m_quad;
            const __m128 pA1 = a1.m_quad;
            const __m128 pA0 = a0.m_quad;
            const __m128 pB = b.m_quad;
            const __m128 edge = _mm_sub_ps(pA1, pA0);
            const __m128 a1MinusB = _mm_sub_ps(pA1, pB);
            const __m128 cross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(a1MinusB, a1MinusB, 0xD2), _mm_shuffle_ps(edge, edge, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(a1MinusB, a1MinusB, 0xC9), _mm_shuffle_ps(edge, edge, 0xD2))
            );
            const __m128 dotsA1 = _mm_mul_ps(a1MinusB, edge);
            const __m128 dotsA0 = _mm_mul_ps(_mm_sub_ps(pB, pA0), edge);
            const __m128 hi = _mm_unpackhi_ps(dotsA1, dotsA0);
            const __m128 crossSq = _mm_mul_ps(cross, cross);
            const __m128 lo = _mm_unpacklo_ps(dotsA1, dotsA0);
            const __m128 loCross = _mm_movelh_ps(lo, crossSq);
            const __m128 sums = _mm_add_ps(
                _mm_shuffle_ps(hi, crossSq, 0xE4),
                _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(loCross, lo), crossSq, 0xD4), loCross)
            );
            const __m128 edgeSq = _mm_mul_ps(edge, edge);
            const float edgeLengthSq = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(edgeSq, edgeSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0x55), edgeSq)
            ));
            if (work.m_radiusSumSqrd.m_storage * edgeLengthSq < sums[3] ||
                (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
                break;
            }
            __m128 t = _mm_div_ss(sums, _mm_add_ss(sums, _mm_shuffle_ps(sums, sums, 0x55)));
            t = _mm_shuffle_ps(t, t, 0);
            const __m128 pointOnA = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pA1), _mm_mul_ps(t, pA0));
            out.m_pointOnA.m_quad = pointOnA;
            out.m_AminusB.m_quad = _mm_sub_ps(pointOnA, b.m_quad);
            return HK_FEATURE_OK;
        }
        case 10: {
            // The edge A0 A1 against the edge B0 B1: valid when each edge's ends lie on both sides of the other's plane
            // through the common normal (sign mask 3 or 12).
            const hkVector4& a0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
            const hkVector4& a1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
            const hkVector4& b0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
            const hkVector4& b1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[3]);
            const __m128 pA1 = a1.m_quad;
            const __m128 pB1 = b1.m_quad;
            const __m128 pA0 = a0.m_quad;
            const __m128 pB0 = b0.m_quad;
            const __m128 edgeA = _mm_sub_ps(pA1, pA0);
            const __m128 a1MinusB1 = _mm_sub_ps(pA1, pB1);
            const __m128 a0MinusB0 = _mm_sub_ps(pA0, pB0);
            const __m128 edgeB = _mm_sub_ps(pB1, pB0);
            const __m128 edgeAZxy = _mm_shuffle_ps(edgeA, edgeA, 210);
            const __m128 edgeBYzx = _mm_shuffle_ps(edgeB, edgeB, 201);
            const __m128 edgeAYzx = _mm_shuffle_ps(edgeA, edgeA, 201);
            const __m128 edgeBZxy = _mm_shuffle_ps(edgeB, edgeB, 210);
            const __m128 cross = _mm_sub_ps(_mm_mul_ps(edgeAYzx, edgeBZxy), _mm_mul_ps(edgeAZxy, edgeBYzx));
            const __m128 crossZxy = _mm_shuffle_ps(cross, cross, 210);
            const __m128 crossYzx = _mm_shuffle_ps(cross, cross, 201);
            const __m128 planeB = _mm_sub_ps(_mm_mul_ps(crossZxy, edgeBYzx), _mm_mul_ps(crossYzx, edgeBZxy));
            out.m_normal.m_quad = work.m_masterNormal.m_quad;
            const __m128 planeA = _mm_sub_ps(_mm_mul_ps(crossZxy, edgeAYzx), _mm_mul_ps(crossYzx, edgeAZxy));
            const __m128 pB1Dots = _mm_mul_ps(planeB, a1MinusB1);
            const __m128 pB0Dots = _mm_mul_ps(planeB, a0MinusB0);
            const __m128 pA0Dots = _mm_mul_ps(planeA, a0MinusB0);
            const __m128 pA1Dots = _mm_mul_ps(planeA, a1MinusB1);
            const __m128 lo1 = _mm_shuffle_ps(pB1Dots, pA1Dots, 68);
            const __m128 hi1 = _mm_shuffle_ps(pB1Dots, pA1Dots, 238);
            const __m128 lo0 = _mm_shuffle_ps(pB0Dots, pA0Dots, 68);
            const __m128 sums = _mm_add_ps(
                _mm_shuffle_ps(_mm_shuffle_ps(pB0Dots, pA0Dots, 238), hi1, 136),
                _mm_add_ps(_mm_shuffle_ps(lo0, lo1, 221), _mm_shuffle_ps(lo0, lo1, 136))
            );
            const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps()));
            if (signs != 3 && signs != 12) {
                break;
            }
            const __m128 tA = _mm_div_ss(sums, _mm_sub_ss(sums, _mm_shuffle_ps(sums, sums, 0xAA)));
            hkVector4 pointOnA;
            pointOnA.setInterpolate4(a0, a1, hkSimdReal(tA));
            const __m128 sumsY = _mm_shuffle_ps(sums, sums, 0x55);
            const __m128 tB = _mm_div_ss(sumsY, _mm_sub_ss(sumsY, _mm_shuffle_ps(sums, sums, 0xFF)));
            out.m_pointOnA = pointOnA;
            hkVector4 pointOnB;
            pointOnB.setInterpolate4(b0, b1, hkSimdReal(tB));
            out.m_AminusB.m_quad = _mm_sub_ps(pointOnA.m_quad, pointOnB.m_quad);
            return HK_FEATURE_OK;
        }
        case 13: {
            // The triangle A0 A1 A2 against the point B: the point on A is B moved onto the triangle's plane.
            const __m128 pB = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[3])->m_quad;
            out.m_pointOnA.m_quad = pB;
            const __m128 aMinusB =
                _mm_sub_ps(reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0])->m_quad, pB);
            out.m_AminusB.m_quad = aMinusB;
            const hkVector4& a0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
            const hkVector4& a2 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
            const __m128 pA1 = reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1])->m_quad;
            const __m128 e20 = _mm_sub_ps(a0.m_quad, a2.m_quad);
            const __m128 e12 = _mm_sub_ps(a2.m_quad, pA1);
            const __m128 e12Yzx = _mm_shuffle_ps(e12, e12, 201);
            const __m128 e12Zxy = _mm_shuffle_ps(e12, e12, 210);
            const __m128 e01 = _mm_sub_ps(pA1, a0.m_quad);
            const __m128 e20Yzx = _mm_shuffle_ps(e20, e20, 201);
            const __m128 e20Zxy = _mm_shuffle_ps(e20, e20, 210);
            const __m128 cross = _mm_sub_ps(_mm_mul_ps(e12Yzx, e20Zxy), _mm_mul_ps(e12Zxy, e20Yzx));
            hkVector4 normalized;
            normalized.m_quad = cross;
            normalized.normalize3();
            const __m128 triNormal = normalized.m_quad;
            const __m128 fromA1 = _mm_sub_ps(pB, pA1);
            const __m128 fromA2 = _mm_sub_ps(pB, a2.m_quad);
            const __m128 fromA0 = _mm_sub_ps(pB, a0.m_quad);
            const __m128 c0 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(fromA0, fromA0, 201), _mm_shuffle_ps(e01, e01, 210)),
                    _mm_mul_ps(_mm_shuffle_ps(fromA0, fromA0, 210), _mm_shuffle_ps(e01, e01, 201))
                ),
                triNormal
            );
            const __m128 c1 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(fromA1, fromA1, 201), e12Zxy),
                    _mm_mul_ps(_mm_shuffle_ps(fromA1, fromA1, 210), e12Yzx)
                ),
                triNormal
            );
            const __m128 c2 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(fromA2, fromA2, 201), e20Zxy),
                    _mm_mul_ps(_mm_shuffle_ps(fromA2, fromA2, 210), e20Yzx)
                ),
                triNormal
            );
            const __m128 master = _mm_mul_ps(work.m_masterNormal.m_quad, triNormal);
            const __m128 depthSq = _mm_mul_ps(triNormal, aMinusB);
            const __m128 depth = _mm_add_ss(
                _mm_shuffle_ps(depthSq, depthSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(depthSq, depthSq, 0x55), depthSq)
            );
            const __m128 edgesLo01 = _mm_shuffle_ps(c1, c2, 68);
            const __m128 edgesLo23 = _mm_shuffle_ps(c0, master, 68);
            const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(
                _mm_add_ps(
                    _mm_shuffle_ps(_mm_shuffle_ps(c1, c2, 238), _mm_shuffle_ps(c0, master, 238), 136),
                    _mm_add_ps(_mm_shuffle_ps(edgesLo01, edgesLo23, 221), _mm_shuffle_ps(edgesLo01, edgesLo23, 136))
                ),
                _mm_setzero_ps()
            ));
            out.m_pointOnA.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(depth, depth, 0), triNormal), pB);
            out.m_normal.m_quad = triNormal;
            if ((signs & 8) != 0) {
                out.m_normal.m_quad = _mm_xor_ps(triNormal, hkQuadRealNegateMask);
            }
            return ~signs & 7;
        }
        default: break;
    }
    return HK_FEATURE_INVALID;
}

// The point A against the edge B0 B1 (the vertex ids are byte offsets into work.m_vertices). The cross product's square
// and the two edge dots are summed into one vector (x: (B1 - A).e, y: (A - B0).e, w: |(B1 - A) x e|^2); the feature is
// invalid when the distance squared times |e|^2 is beyond the radius sum or either dot is negative.
int32_t hkGskManifold_convertPointEdge(
    const hkpGskManifoldWork& work,
    const hkpGskManifold::ContactPoint& feature,
    hkGskManifoldConvertOut& out
) {
    // The vertex ids are byte offsets into work.m_vertices, added to the array's address as raw memory.
    // EXE-INLINED none: hkAddByteOffsetConst
    const char* vertexBytes = reinterpret_cast<const char*>(work.m_vertices);
    const hkVector4& a = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
    const hkVector4& b0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
    const hkVector4& b1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
    out.m_pointOnA.m_quad = a.m_quad;
    out.m_normal.m_quad = work.m_masterNormal.m_quad;
    const __m128 pB0 = b0.m_quad;
    const __m128 pB1 = b1.m_quad;
    const __m128 pA = a.m_quad;
    const __m128 edge = _mm_sub_ps(pB1, pB0);
    const __m128 b1MinusA = _mm_sub_ps(pB1, pA);
    const __m128 cross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(b1MinusA, b1MinusA, 0xD2), _mm_shuffle_ps(edge, edge, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(b1MinusA, b1MinusA, 0xC9), _mm_shuffle_ps(edge, edge, 0xD2))
    );
    const __m128 crossSq = _mm_mul_ps(cross, cross);
    const __m128 dotsB1 = _mm_mul_ps(b1MinusA, edge);
    const __m128 dotsB0 = _mm_mul_ps(_mm_sub_ps(pA, pB0), edge);
    const __m128 hi = _mm_unpackhi_ps(dotsB1, dotsB0);
    const __m128 lo = _mm_unpacklo_ps(dotsB1, dotsB0);
    const __m128 loCross = _mm_movelh_ps(lo, crossSq);
    const __m128 sums = _mm_add_ps(
        _mm_shuffle_ps(hi, crossSq, 0xE4),
        _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(loCross, lo), crossSq, 0xD4), loCross)
    );
    const __m128 edgeSq = _mm_mul_ps(edge, edge);
    const float edgeLengthSq = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0x55), edgeSq))
    );
    if (work.m_radiusSumSqrd.m_storage * edgeLengthSq < sums[3] ||
        (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
        return HK_FEATURE_INVALID;
    }
    __m128 t = _mm_div_ss(sums, _mm_add_ss(sums, _mm_shuffle_ps(sums, sums, 0x55)));
    t = _mm_shuffle_ps(t, t, 0);
    out.m_AminusB.m_quad =
        _mm_sub_ps(pA, _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pB0), _mm_mul_ps(pB1, t)));
    return HK_FEATURE_OK;
}

// The mirror of convertPointEdge: the point on the edge A0 A1 nearest B is the point on A.
int32_t hkGskManifold_convertEdgePoint(
    const hkpGskManifoldWork& work,
    const hkpGskManifold::ContactPoint& feature,
    hkGskManifoldConvertOut& out
) {
    // EXE-INLINED none: hkAddByteOffsetConst
    const char* vertexBytes = reinterpret_cast<const char*>(work.m_vertices);
    const hkVector4& a0 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
    const hkVector4& a1 = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
    const hkVector4& b = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[2]);
    out.m_normal.m_quad = work.m_masterNormal.m_quad;
    const __m128 pA1 = a1.m_quad;
    const __m128 pA0 = a0.m_quad;
    const __m128 pB = b.m_quad;
    const __m128 edge = _mm_sub_ps(pA1, pA0);
    const __m128 a1MinusB = _mm_sub_ps(pA1, pB);
    const __m128 cross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(a1MinusB, a1MinusB, 0xD2), _mm_shuffle_ps(edge, edge, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(a1MinusB, a1MinusB, 0xC9), _mm_shuffle_ps(edge, edge, 0xD2))
    );
    const __m128 dotsA1 = _mm_mul_ps(a1MinusB, edge);
    const __m128 dotsA0 = _mm_mul_ps(_mm_sub_ps(pB, pA0), edge);
    const __m128 hi = _mm_unpackhi_ps(dotsA1, dotsA0);
    const __m128 crossSq = _mm_mul_ps(cross, cross);
    const __m128 lo = _mm_unpacklo_ps(dotsA1, dotsA0);
    const __m128 loCross = _mm_movelh_ps(lo, crossSq);
    const __m128 sums = _mm_add_ps(
        _mm_shuffle_ps(hi, crossSq, 0xE4),
        _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(loCross, lo), crossSq, 0xD4), loCross)
    );
    const __m128 edgeSq = _mm_mul_ps(edge, edge);
    const float edgeLengthSq = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(edgeSq, edgeSq, 0x55), edgeSq))
    );
    if (work.m_radiusSumSqrd.m_storage * edgeLengthSq < sums[3] ||
        (_mm_movemask_ps(_mm_cmplt_ps(sums, _mm_setzero_ps())) & 3) != 0) {
        return HK_FEATURE_INVALID;
    }
    __m128 t = _mm_div_ss(sums, _mm_add_ss(sums, _mm_shuffle_ps(sums, sums, 0x55)));
    t = _mm_shuffle_ps(t, t, 0);
    const __m128 pointOnA = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pA1), _mm_mul_ps(pA0, t));
    out.m_pointOnA.m_quad = pointOnA;
    out.m_AminusB.m_quad = _mm_sub_ps(pointOnA, b.m_quad);
    return HK_FEATURE_OK;
}

// Invalid when the points are further apart than the radius sum (compared on the float distance squared), else A, A - B
// and the master normal.
int32_t hkGskManifold_convertPointPoint(
    const hkpGskManifoldWork& work,
    const hkpGskManifold::ContactPoint& feature,
    hkGskManifoldConvertOut& out
) {
    // EXE-INLINED none: hkAddByteOffsetConst
    const char* vertexBytes = reinterpret_cast<const char*>(work.m_vertices);
    const hkVector4& a = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[0]);
    const hkVector4& b = *reinterpret_cast<const hkVector4*>(vertexBytes + feature.m_vert[1]);
    const __m128 aMinusB = _mm_sub_ps(a.m_quad, b.m_quad);
    const __m128 sq = _mm_mul_ps(aMinusB, aMinusB);
    const float distanceSq =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
    if (work.m_radiusSumSqrd.m_storage < distanceSq) {
        return HK_FEATURE_INVALID;
    }
    out.m_AminusB.m_quad = aMinusB;
    out.m_normal.m_quad = work.m_masterNormal.m_quad;
    out.m_pointOnA.m_quad = a.m_quad;
    return HK_FEATURE_OK;
}

// The vertex ids sit right after the contact points (getVertexIds()), so adding or dropping a point moves them by one
// point; that span is copied as untyped words, hence the casts (as in hkGskManifold_removePoint).
hkpGskManifoldAddStatus hkGskManifold_addPoint(
    hkpGskManifold& manifold,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& output,
    const hkpGskCache& newPoint,
    hkpProcessCdPoint* newCdPointInResultArray,
    hkpProcessCdPoint* resultPointArray,
    hkpContactMgr* mgr,
    hkpGskManifoldUtilMgrHandling mgrHandling
) {
    hkpGskManifoldAddStatus status = HK_GSK_MANIFOLD_POINT_ADDED;
    uint16_t id;
    if (manifold.m_numContactPoints == hkpGskManifold::MAX_POINTS) {
        // The manifold is full: drop the point whose loss costs least area, or reject the new one.
        const int32_t redundant = hkGskManifold_findRedundant5thPoint(resultPointArray);
        if (redundant == 4) {
            return HK_GSK_MANIFOLD_POINT_REJECTED;
        }
        mgr->removeContactPointImpl(manifold.m_contactPoints[redundant].m_id, *output.m_constraintOwner.m_storage);
        // The new point goes to slot 0: its old point moves to the freed slot (for slot 0, point 1 is copied).
        if (redundant != 0) {
            manifold.m_contactPoints[redundant] = manifold.m_contactPoints[0];
        }
        else {
            manifold.m_contactPoints[0] = manifold.m_contactPoints[1];
        }
        hkGskManifold_garbageCollect(manifold);
        id = mgr->addContactPointImpl(bodyA, bodyB, input, output, &newPoint, newCdPointInResultArray->m_contact);
        if (id == 0xFFFF) {
            // No contact point for the new one: drop slot 0 (the last point takes it) and its result.
            manifold.m_contactPoints[0] = manifold.m_contactPoints[3];
            const int32_t numPoints = manifold.m_numContactPoints;
            manifold.m_numContactPoints = static_cast<uint8_t>(numPoints - 1);
            const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(&manifold.m_contactPoints[numPoints]);
            uint32_t* pDst = reinterpret_cast<uint32_t*>(&manifold.m_contactPoints[numPoints - 1]);
            for (int32_t i = (manifold.m_numVertsB + manifold.m_numVertsA - 1) >> 1; i >= 0; --i) {
                *pDst++ = *pSrc++;
            }
            hkpProcessCdPoint& result = resultPointArray[redundant];
            result.m_contact.m_position = resultPointArray[3].m_contact.m_position;
            result.m_contact.m_separatingNormal = resultPointArray[3].m_contact.m_separatingNormal;
            result.m_contactPointId.m_storage = resultPointArray[3].m_contactPointId.m_storage;
            return HK_GSK_MANIFOLD_TWO_POINT2_REJECTED;
        }
        newCdPointInResultArray->m_contactPointId.m_storage = id;
        status = static_cast<hkpGskManifoldAddStatus>(redundant);
        hkpProcessCdPoint& result = resultPointArray[redundant];
        result.m_contact.m_position = newCdPointInResultArray->m_contact.m_position;
        result.m_contact.m_separatingNormal = newCdPointInResultArray->m_contact.m_separatingNormal;
        result.m_contactPointId.m_storage = newCdPointInResultArray->m_contactPointId.m_storage;
    }
    else {
        if (mgrHandling == HK_GSK_MANIFOLD_CREATE_ID_ALWAYS || newPoint.m_dimB == 3) {
            id = mgr->addContactPointImpl(bodyA, bodyB, input, output, &newPoint, newCdPointInResultArray->m_contact);
            if (id == 0xFFFF) {
                return HK_GSK_MANIFOLD_POINT_REJECTED;
            }
        }
        else {
            id = 0xFFFF;
        }
        newCdPointInResultArray->m_contactPointId.m_storage = id;
        // Make room for one more point: move the vertex ids up by one point, last 32-bit word first, and move point 0
        // to the new last slot.
        const int32_t numPoints = manifold.m_numContactPoints;
        const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(&manifold.m_contactPoints[numPoints]);
        uint32_t* pDst = reinterpret_cast<uint32_t*>(&manifold.m_contactPoints[numPoints + 1]);
        for (int32_t i = (manifold.m_numVertsB + manifold.m_numVertsA - 1) >> 1; i >= 0; --i) {
            pDst[i] = pSrc[i];
        }
        manifold.m_contactPoints[numPoints] = manifold.m_contactPoints[0];
        manifold.m_numContactPoints = static_cast<uint8_t>(numPoints + 1);
    }

    // The new point in slot 0; its vertices are entered below.
    hkpGskManifold::ContactPoint& cp = manifold.m_contactPoints[0];
    cp.m_dimA = newPoint.m_dimA;
    cp.m_dimB = newPoint.m_dimB;
    cp.m_id = id;
    cp.m_allVerts = 0;
    uint16_t* vertexIds = manifold.getVertexIds();

    // A's vertex ids: find each among the A ids, else insert it at the end of A (moving B's ids up one).
    int32_t addedA = 0;
    for (int32_t i = 0; i < newPoint.m_dimA; ++i) {
        const uint16_t vertexId = newPoint.m_vertices[i];
        uint32_t j = 0;
        while (j < manifold.m_numVertsA && vertexIds[j] != vertexId) {
            ++j;
        }
        if (j == manifold.m_numVertsA) {
            for (uint32_t k = manifold.m_numVertsB + manifold.m_numVertsA; k > j; --k) {
                vertexIds[k] = vertexIds[k - 1];
            }
            vertexIds[j] = vertexId;
            manifold.m_numVertsA++;
            addedA += 16;
        }
        cp.m_vert[i] = static_cast<uint8_t>(j << 4);
    }

    // The other points' B vertices moved up with B's ids: shift their offsets by the A ids added.
    if (addedA != 0) {
        for (uint32_t p = 1; p < manifold.m_numContactPoints; ++p) {
            hkpGskManifold::ContactPoint& other = manifold.m_contactPoints[p];
            for (uint32_t k = other.m_dimA; k < static_cast<uint32_t>(other.m_dimA + other.m_dimB); ++k) {
                other.m_vert[k] += static_cast<uint8_t>(addedA);
            }
        }
    }

    // B's vertex ids: find each among the B ids, else append it.
    for (int32_t i = 0; i < newPoint.m_dimB; ++i) {
        const uint16_t vertexId = newPoint.m_vertices[newPoint.m_dimA + i];
        int32_t j = manifold.m_numVertsA;
        const int32_t end = manifold.m_numVertsB + manifold.m_numVertsA;
        while (j < end && vertexIds[j] != vertexId) {
            ++j;
        }
        if (j >= end) {
            vertexIds[j] = vertexId;
            manifold.m_numVertsB++;
        }
        cp.m_vert[newPoint.m_dimA + i] = static_cast<uint8_t>(j << 4);
    }
    return status;
}

void hkGskManifold_cleanup(hkpGskManifold& manifold, hkpContactMgr* mgr, hkpConstraintOwner& constraintOwner) {
    uint32_t v3 = 0;
    if (manifold.m_numContactPoints) {
        uint16_t* p_m_id = &manifold.m_contactPoints[0u].m_id;
        do {
            int32_t v5 = *p_m_id;
            if (static_cast<uint16_t>(v5) != 0xFFFF) {
                mgr->removeContactPointImpl(v5, constraintOwner);
            }
            ++v3;
            p_m_id += 4;
        }
        while (v3 < manifold.m_numContactPoints);
    }
    manifold.m_numContactPoints = 0;
    manifold.m_numVertsA = 0;
    manifold.m_numVertsB = 0;
}
