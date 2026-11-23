#include "Havok/Source/Common/Internal/ConvexHull/hkPlaneEquationUtil.h"

#include "Havok/Source/Common/Base/Math/hkMath.h"

#include <xmmintrin.h>

// Cramer's rule: the cross products B x C, C x A and A x B, and the determinant A . (B x C) summed z + (y + x). Its
// fabs is stored as a float and compared with 1.1920929e-07f; a NaN determinant goes on. The right-hand side is (-0.0f
// - A.w, -0.0f - B.w, -0.0f - C.w, 0.0f), and the point (B x C) * a + (C x A) * b + (A x B) * c is summed in that
// order. It is stored once unscaled, then multiplied by the float of 1.0 / det. The plane indices are stored before
// IntersectionPoint::sort's signed swap.
hkResult hkPlaneEquationUtil::findTriPlaneIntersectionPoint(
    const hkArray<hkVector4>& planeEquations,
    int32_t planeAIndex,
    int32_t planeBIndex,
    int32_t planeCIndex,
    IntersectionPoint& intersectionPointOut
) {
    const hkVector4* pPlaneA = &planeEquations.m_data[planeAIndex];
    const hkVector4* pPlaneB = &planeEquations.m_data[planeBIndex];
    const hkVector4* pPlaneC = &planeEquations.m_data[planeCIndex];
    const __m128 kA = _mm_load_ps(&reinterpret_cast<const float*>(&pPlaneA->m_quad)[0]);
    const __m128 kB = _mm_load_ps(&reinterpret_cast<const float*>(&pPlaneB->m_quad)[0]);
    const __m128 kC = _mm_load_ps(&reinterpret_cast<const float*>(&pPlaneC->m_quad)[0]);
    const __m128 kBC = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kC, kC, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kC, kC, 0xC9))
    );
    const __m128 kCA = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xC9), _mm_shuffle_ps(kA, kA, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xD2), _mm_shuffle_ps(kA, kA, 0xC9))
    );
    const __m128 kAB = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xC9), _mm_shuffle_ps(kB, kB, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xD2), _mm_shuffle_ps(kB, kB, 0xC9))
    );
    const __m128 kDot = _mm_mul_ps(kA, kBC);
    const float fDet = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0x55), kDot))
    );
    const float fAbsDet = hkMath::fabs(fDet);
    if (fAbsDet < HK_REAL_EPSILON) {
        return HK_FAILURE;
    }

    const float fNegZero = -0.0f;
    const float fRhsA = _mm_cvtss_f32(
        _mm_sub_ss(_mm_set_ss(fNegZero), _mm_load_ss(&reinterpret_cast<const float*>(&pPlaneA->m_quad)[3]))
    );
    const float fRhsB = _mm_cvtss_f32(
        _mm_sub_ss(_mm_set_ss(fNegZero), _mm_load_ss(&reinterpret_cast<const float*>(&pPlaneB->m_quad)[3]))
    );
    const float fRhsC = _mm_cvtss_f32(
        _mm_sub_ss(_mm_set_ss(fNegZero), _mm_load_ss(&reinterpret_cast<const float*>(&pPlaneC->m_quad)[3]))
    );
    const __m128 kPoint = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fRhsB), kCA), _mm_mul_ps(_mm_set1_ps(fRhsA), kBC)),
        _mm_mul_ps(_mm_set1_ps(fRhsC), kAB)
    );
    const float fInvDet = static_cast<float>(1.0 / fDet);
    _mm_store_ps(
        &reinterpret_cast<float*>(&intersectionPointOut.m_location.m_quad)[0],
        _mm_mul_ps(_mm_set1_ps(fInvDet), kPoint)
    );
    intersectionPointOut.m_planeA = planeAIndex;
    intersectionPointOut.m_planeB = planeBIndex;
    intersectionPointOut.m_planeC = planeCIndex;
    intersectionPointOut.sort();
    return HK_SUCCESS;
}

// Welds vertices sorted by x in place: a vertex is dropped when one already kept, at most 0.01 below it on x (the
// search runs back from the last kept one), lies closer than threshold (squared distance), and so are the vertices
// right after it that lie as close to that kept one; numOut and the array's size get the number kept. Each squared
// distance is stored as a float.
void removeNearDuplicateSortedVertices(float threshold, hkArray<hkVector4>* vertices, int32_t* numOut) {
    hkVector4* cur = vertices->m_data;
    hkVector4* out = vertices->m_data;
    int32_t remaining = vertices->m_size - 1;
    if (remaining >= 0) {
        const double kThreshold = threshold;
        do {
            const double kLimit = (*cur)(0) - 0.009999999776482582;
            hkVector4* kept = nullptr;
            for (hkVector4* k = out - 1; k >= vertices->m_data && !((*k)(0) < kLimit); --k) {
                const __m128 kDiff = _mm_sub_ps(k->m_quad, cur->m_quad);
                const __m128 kSq = _mm_mul_ps(kDiff, kDiff);
                const float distSq = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))
                );
                if (distSq < kThreshold) {
                    kept = k;
                    break;
                }
            }
            if (kept) {
                for (int32_t j = remaining - 1; j >= 0; --j) {
                    const __m128 kDiff = _mm_sub_ps(kept->m_quad, cur[1].m_quad);
                    const __m128 kSq = _mm_mul_ps(kDiff, kDiff);
                    const float distSq = _mm_cvtss_f32(
                        _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))
                    );
                    if (!(distSq < kThreshold)) {
                        break;
                    }
                    ++cur;
                    --remaining;
                }
            }
            else {
                *out = *cur;
                ++out;
            }
            ++cur;
        }
        while (--remaining >= 0);
    }
    const int32_t numKept = static_cast<int32_t>(out - vertices->m_data);
    *numOut = numKept;
    vertices->setSize(numKept);
}

// Collects into pointsOut the points where planes planeA and planeB meet each other plane that faces along dir (dot
// product above 1e-08), the nearest ones first: vertex (its w set to 1) lies at distance -plane.vertex / |dot| from the
// plane; a point no more than 1e-4 beyond the nearest so far joins the list, one more than 1e-4 nearer replaces it. The
// result is true when any point was found. Each dot product, distance and difference is stored as a float before it is
// compared.
// KEEP: hkPlaneEquationUtil::findTriPlaneIntersectionPoint writes the point's members only, and each push copies the
// whole point into the array: the padding after m_planeC, which nothing wrote, reaches pointsOut (partcopy).
hkBool findNextIntersectionPoints(
    const hkVector4& dir,
    const hkArray<hkVector4>& planes,
    int32_t planeA,
    int32_t planeB,
    hkVector4& vertex,
    hkArray<hkPlaneEquationUtil::IntersectionPoint>& pointsOut
) {
    using Point_t = hkPlaneEquationUtil::IntersectionPoint;
    float nearest = HK_REAL_MAX;
    bool found = false;
    for (int32_t i = 0; i < planes.m_size; ++i) {
        if (i == planeA || i == planeB) {
            continue;
        }
        const __m128 kProduct = _mm_mul_ps(planes.m_data[i].m_quad, dir.m_quad);
        const float dot = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(kProduct, kProduct, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)
        ));
        if (!(dot > 1e-08f)) {
            continue;
        }
        vertex(3) = 1.0f;
        const __m128 kTerms = _mm_mul_ps(planes.m_data[i].m_quad, vertex.m_quad);
        const __m128 kHalf = _mm_add_ps(_mm_shuffle_ps(kTerms, kTerms, 0x4E), kTerms);
        const __m128 kSum = _mm_add_ps(_mm_shuffle_ps(kHalf, kHalf, 0xB1), kHalf);
        const float numerator = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), kSum));
        const float absDot = std::fabs(dot);
        const float distance = numerator / absDot;
        if (!(nearest + 9.999999747378752e-05 > distance)) {
            continue;
        }
        Point_t point;
        if (hkPlaneEquationUtil::findTriPlaneIntersectionPoint(planes, planeA, planeB, i, point) != HK_SUCCESS) {
            continue;
        }
        found = true;
        const float delta = distance - nearest;
        const float absDelta = std::fabs(delta);
        if (!(absDelta < 9.999999747378752e-05)) {
            pointsOut.m_size = 0;
            pointsOut.pushBack(point);
            nearest = distance;
        }
        else {
            pointsOut.pushBack(point);
        }
    }
    return found;
}

// Unless the edge of planes m_planeA and m_planeB is in visitedEdges already (in either order), records it, finds the
// next points along its line from its point (findNextIntersectionPoints) and, when there are any, adds the first one's
// location to vertexCloudOut and walks on along the two edges each point opens (its planes A and C, then B and C, each
// line from calculateOrientedIntersectionLine).
// KEEP: with 4-byte pointers the two edges built here leave the four padding bytes after m_fromPoint unwritten, and
// the recursive call copies its edge whole into visitedEdges: those bytes reach the array (partcopy).
void visitEdge(
    const hkPlaneEquationUtil::VisitedEdge& edge,
    const hkArray<hkVector4>& planes,
    hkArray<hkPlaneEquationUtil::VisitedEdge>& visitedEdges,
    hkArray<hkVector4>& vertexCloudOut
) {
    using Edge_t = hkPlaneEquationUtil::VisitedEdge;
    using Point_t = hkPlaneEquationUtil::IntersectionPoint;
    if (visitedEdges.indexOf(edge) != -1) {
        return;
    }
    visitedEdges.pushBack(edge);

    hkArray<Point_t> points;
    if (findNextIntersectionPoints(
            edge.m_intersectionLineDir,
            planes,
            edge.m_planeA,
            edge.m_planeB,
            edge.m_fromPoint->m_location,
            points
        )) {
        vertexCloudOut.pushBack(points.m_data[0].m_location);
        for (Point_t& point : points) {
            Point_t* pPoint = &point;
            Edge_t edgeAC;
            edgeAC.m_planeA = pPoint->m_planeA;
            edgeAC.m_planeB = pPoint->m_planeC;
            edgeAC.m_fromPoint = pPoint;
            calculateOrientedIntersectionLine(
                &planes,
                pPoint->m_planeA,
                pPoint->m_planeC,
                pPoint->m_planeB,
                &edgeAC.m_intersectionLineDir
            );
            visitEdge(edgeAC, planes, visitedEdges, vertexCloudOut);
            Edge_t edgeBC;
            edgeBC.m_planeA = pPoint->m_planeB;
            edgeBC.m_planeB = pPoint->m_planeC;
            edgeBC.m_fromPoint = pPoint;
            calculateOrientedIntersectionLine(
                &planes,
                pPoint->m_planeB,
                pPoint->m_planeC,
                pPoint->m_planeA,
                &edgeBC.m_intersectionLineDir
            );
            visitEdge(edgeBC, planes, visitedEdges, vertexCloudOut);
        }
    }
}
