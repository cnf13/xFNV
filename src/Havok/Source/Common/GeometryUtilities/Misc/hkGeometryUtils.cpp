#include "Havok/Source/Common/GeometryUtilities/Misc/hkGeometryUtils.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/1AxisSweep/hk1AxisSweep.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

#include <cmath>
#include <xmmintrin.h>

static hkBool isTriangleDegenerate(const hkGeometry::Triangle* triangle);
// Welds the vertices, producing the unique vertices and the remap.
static void weldVertexArray(
    const hkArray<hkVector4>& vertices,
    hkArray<hkVector4>& uniqueVertsOut,
    hkArray<int32_t>& remapOut,
    float threshold
);
// Rename candidate: hkGeometryUtils::weldVertices.
static void weldGeometryVertices(
    hkGeometry& geom,
    float threshold,
    hkBool keepVertexOrder,
    hkArray<int32_t>& vertexRemapOut,
    hkArray<int32_t>& triangleRemapOut
);

// The SDK's hkGeometryUtils::weldVertices(hkGeometry&, hkReal, hkBool, hkArray<int>&, hkArray<int>&)
// (hkGeometryUtilities.lib): welds the vertices (weldVertexArray), keeping their first-use order when asked, remaps the
// triangles and removes the degenerate ones, triangleRemapOut holding each triangle's new index or -1.
static void weldGeometryVertices(
    hkGeometry& geom,
    float threshold,
    hkBool keepVertexOrder,
    hkArray<int32_t>& vertexRemapOut,
    hkArray<int32_t>& triangleRemapOut
) {
    const int32_t numVertices = geom.m_vertices.m_size;
    const int32_t numTriangles = geom.m_triangles.m_size;
    vertexRemapOut.setSize(numVertices);
    hkArray<hkVector4> uniqueVerts;
    weldVertexArray(geom.m_vertices, uniqueVerts, vertexRemapOut, threshold);

    // The source of the new vertices: the unique ones as welded, or in their first-use order.
    hkArray<hkVector4>* pNewVerts = &uniqueVerts;
    hkArray<hkVector4> orderedVerts;
    hkArray<int32_t> uniqueToOrdered;
    if (keepVertexOrder.m_bool) {
        const int32_t numUnique = uniqueVerts.m_size;
        // EXE-INLINED none: hkArray<int>::setSize(numUnique, -1)
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = uniqueToOrdered.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
        if (capacity < numUnique) {
            int32_t newCapacity = 2 * capacity;
            if (numUnique >= newCapacity) {
                newCapacity = numUnique;
            }
            hkArrayUtil::_reserve(heap, &uniqueToOrdered, newCapacity, sizeof(int32_t));
        }
        for (int32_t i = uniqueToOrdered.m_size; i < numUnique; ++i) {
            uniqueToOrdered.m_data[i] = -1;
        }
        uniqueToOrdered.m_size = numUnique;
        for (int32_t i = 0; i < vertexRemapOut.m_size; ++i) {
            const int32_t unique = vertexRemapOut.m_data[i];
            int32_t ordered = uniqueToOrdered.m_data[unique];
            if (ordered == -1) {
                ordered = orderedVerts.m_size;
                uniqueToOrdered.m_data[unique] = ordered;
                orderedVerts.pushBack(uniqueVerts.m_data[unique]);
            }
            vertexRemapOut.m_data[i] = ordered;
        }
        pNewVerts = &orderedVerts;
    }

    geom.m_vertices = *pNewVerts;

    for (int32_t i = 0; i < numTriangles; ++i) {
        hkGeometry::Triangle& triangle = geom.m_triangles.m_data[i];
        triangle.m_a = vertexRemapOut.m_data[triangle.m_a];
        triangle.m_b = vertexRemapOut.m_data[triangle.m_b];
        triangle.m_c = vertexRemapOut.m_data[triangle.m_c];
    }

    triangleRemapOut.setSize(numTriangles);
    hkGeometry::Triangle* pOut = geom.m_triangles.m_data;
    for (int32_t i = 0; i < numTriangles; ++i) {
        const hkGeometry::Triangle* pTriangle = &geom.m_triangles.m_data[i];
        int32_t newIndex = -1;
        if (!isTriangleDegenerate(pTriangle).m_bool) {
            newIndex = static_cast<int32_t>(pOut - geom.m_triangles.m_data);
            pOut->m_a = pTriangle->m_a;
            pOut->m_b = pTriangle->m_b;
            pOut->m_c = pTriangle->m_c;
            pOut->m_material = pTriangle->m_material;
            ++pOut;
        }
        triangleRemapOut.m_data[i] = newIndex;
    }
    const int32_t numKept = static_cast<int32_t>(pOut - geom.m_triangles.m_data);
    geom.m_triangles.setSize(numKept);
}

// Whether the triangle is degenerate: two of its vertex indices are the same.
static hkBool isTriangleDegenerate(const hkGeometry::Triangle* triangle) {
    const int32_t b = triangle->m_b;
    if (triangle->m_a == b || triangle->m_a == triangle->m_c || b == triangle->m_c) {
        return 1;
    }
    return 0;
}

// Each vertex's AABB grown by half the threshold goes into a sweep array sorted along x; each vertex not yet welded
// becomes a unique vertex, and every later one whose AABB still overlaps in x and lies within the threshold is welded
// to it (remapOut, its key cleared). The sweep array's four end markers are one local AabbInt of which only the first
// word is written (-1), copied whole into the heap array; the rest is deliberately left unwritten, as the game does.
// Its one caller is weldGeometryVertices.
static void weldVertexArray(
    const hkArray<hkVector4>& vertices,
    hkArray<hkVector4>& uniqueVertsOut,
    hkArray<int32_t>& remapOut,
    float threshold
) {
    const int32_t numVertices = vertices.m_size;
    remapOut.setSize(numVertices);

    // EXE-INLINED none: hkArray<hk1AxisSweep::AabbInt>::hkArray(int) (numVertices + 4)
    const int32_t iDontDeallocate = static_cast<int32_t>(hkArrayBase<hk1AxisSweep::AabbInt>::DONT_DEALLOCATE_FLAG);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    hk1AxisSweep::AabbInt* pAabbs = nullptr;
    int32_t iAabbsCapacity = 0;
    if (numVertices + 4 != 0) {
        int32_t iNumBytes = (numVertices + 4) * static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt));
        pAabbs = static_cast<hk1AxisSweep::AabbInt*>(pHeap->bufAlloc(iNumBytes));
        iAabbsCapacity = iNumBytes / static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt));
    }
    if (!iAabbsCapacity) {
        iAabbsCapacity = iDontDeallocate;
    }

    const float halfThreshold = static_cast<float>(threshold * 0.5);
    const __m128 vHalfThreshold = _mm_set1_ps(halfThreshold);
    for (int32_t i = 0; i < numVertices; ++i) {
        const __m128 v = vertices.m_data[i].m_quad;
        alignas(16) hkAabb aabb;
        aabb.m_min.m_quad = _mm_sub_ps(v, vHalfThreshold);
        aabb.m_max.m_quad = _mm_add_ps(vHalfThreshold, v);
        pAabbs[i].set(aabb, i);
    }
    alignas(16) hk1AxisSweep::AabbInt endMarker;
    endMarker.m_min[0] = 0xFFFFFFFFu;
    for (int32_t j = 0; j < 4; ++j) {
        pAabbs[numVertices + j] = endMarker;
    }
    hk1AxisSweep::sortAabbs(pAabbs, numVertices);

    const float thresholdSq = (threshold * threshold);
    for (int32_t i = 0; i < numVertices; ++i) {
        const uint32_t key = pAabbs[i].getKey();
        if (key == 0xFFFFFFFFu) {
            continue;
        }
        const __m128 vertex = vertices.m_data[key].m_quad;
        remapOut.m_data[key] = uniqueVertsOut.m_size;
        uniqueVertsOut.pushBack(vertices.m_data[key]);
        for (int32_t j = i + 1; j < numVertices; ++j) {
            if (pAabbs[j].m_min[0] > pAabbs[i].m_max[0]) {
                break;
            }
            const uint32_t otherKey = pAabbs[j].getKey();
            if (otherKey == 0xFFFFFFFFu) {
                continue;
            }
            const __m128 diff = _mm_sub_ps(vertex, vertices.m_data[otherKey].m_quad);
            const __m128 squared = _mm_mul_ps(diff, diff);
            const float distanceSq = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(squared, squared, 0xAA), _mm_add_ss(_mm_shuffle_ps(squared, squared, 0x55), squared)
            ));
            // Welded unless thresholdSq is below distanceSq.
            if (!(thresholdSq < distanceSq)) {
                remapOut.m_data[otherKey] = remapOut.m_data[key];
                pAabbs[j].getKey() = 0xFFFFFFFFu;
            }
        }
    }

    // EXE-INLINED none: hkArray<hk1AxisSweep::AabbInt>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iAabbsCapacity >= 0) {
        pHeap->bufFree(
            pAabbs,
            (iAabbsCapacity & hkArrayBase<hk1AxisSweep::AabbInt>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt))
        );
    }
}

// The SDK's hkGeometryUtils::computeVolume (hkGeometryUtilities.lib): the sum over the triangles of a . (b x c),
// accumulated as a float, divided by 6.
float computeVolume(const hkGeometry& geometry) {
    float volume = 0.0f;
    for (int32_t i = 0; i < geometry.m_triangles.m_size; ++i) {
        const hkGeometry::Triangle& triangle = geometry.m_triangles.m_data[i];
        const __m128 c = geometry.m_vertices.m_data[triangle.m_c].m_quad;
        const __m128 b = geometry.m_vertices.m_data[triangle.m_b].m_quad;
        const __m128 cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(b, b, 0xC9), _mm_shuffle_ps(c, c, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(b, b, 0xD2), _mm_shuffle_ps(c, c, 0xC9))
        );
        const __m128 product = _mm_mul_ps(cross, geometry.m_vertices.m_data[triangle.m_a].m_quad);
        const float dot = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(product, product, 0xAA), _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
        ));
        volume = dot + volume;
    }
    return static_cast<float>(volume / 6.0);
}

// The SDK's hkGeometryUtils::transformGeometry (hkGeometryUtilities.lib): each vertex becomes
// col0 * x + col1 * y + col2 * z + col3, the matrix columns read again for each vertex.
void transformGeometry(const hkMatrix4& transform, hkGeometry& geometryInOut) {
    for (int32_t i = 0; i < geometryInOut.m_vertices.m_size; ++i) {
        hkVector4& vertex = geometryInOut.m_vertices.m_data[i];
        const __m128 p = vertex.m_quad;
        __m128 result = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), transform.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), transform.m_col0.m_quad)
        );
        result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), transform.m_col2.m_quad));
        vertex.m_quad = _mm_add_ps(result, transform.m_col3.m_quad);
    }
}

// Builds the capsule's frame from start to end, its vertices (the top pole, the top cap's rings, the cylinder's rings,
// the bottom cap's rings, the bottom pole) in that frame, moved by capsuleToLocal and then by transform into a new
// hkGeometry with its fan and quad triangles.
void hkGeometryUtils::createCapsuleGeometry(
    const hkVector4& start,
    const hkVector4& end,
    float radius,
    int32_t heightSamples,
    int32_t thetaSamples,
    const hkTransform& transform,
    hkGeometry*& geometryOut
) {
    hkArray<hkVector4> verts;
    const int32_t phiSamples = thetaSamples >> 1;

    // The capsule's frame: z along end - start, the origin halfway between them.
    hkTransform capsuleToLocal;
    hkVector4 axis;
    axis.m_quad = _mm_sub_ps(end.m_quad, start.m_quad);
    const hkQuadReal axisSq = _mm_mul_ps(axis.m_quad, axis.m_quad);
    const hkQuadReal axisLenSq =
        _mm_add_ss(_mm_shuffle_ps(axisSq, axisSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(axisSq, axisSq, 0x55), axisSq));
    const float height = _mm_cvtss_f32(_mm_sqrt_ss(axisLenSq));
    bool bRotated = false;
    if (height > 0.0f) {
        // normalize3: _mm_rsqrt_ss refined by one Newton step, 0 where the length is 0.
        const hkQuadReal axisApprox = _mm_rsqrt_ss(axisLenSq);
        const hkQuadReal axisInv = _mm_andnot_ps(
            _mm_cmpeq_ss(axisLenSq, _mm_setzero_ps()),
            _mm_mul_ss(
                _mm_mul_ss(_mm_set_ss(0.5f), axisApprox),
                _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(axisLenSq, axisApprox), axisApprox))
            )
        );
        axis.m_quad = _mm_mul_ps(_mm_shuffle_ps(axisInv, axisInv, 0), axis.m_quad);
        const hkQuadReal canonicalZ = hkQuadReal0010;
        const hkQuadReal zSq = _mm_mul_ps(canonicalZ, axis.m_quad);
        const float zDot =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(zSq, zSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(zSq, zSq, 0x55), zSq)));
        const float zDotAbs = std::fabs(zDot);
        if (zDotAbs < 0.9999899864196777) {
            // col1 = axis x canonicalZ, normalised; col0 = col1 x axis; col2 = axis.
            const hkQuadReal axisYZX = _mm_shuffle_ps(axis.m_quad, axis.m_quad, 0xC9);
            const hkQuadReal axisZXY = _mm_shuffle_ps(axis.m_quad, axis.m_quad, 0xD2);
            hkQuadReal col1 = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(canonicalZ, canonicalZ, 0xD2), axisYZX),
                _mm_mul_ps(_mm_shuffle_ps(canonicalZ, canonicalZ, 0xC9), axisZXY)
            );
            const hkQuadReal col1Sq = _mm_mul_ps(col1, col1);
            const hkQuadReal col1LenSq = _mm_add_ss(
                _mm_shuffle_ps(col1Sq, col1Sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(col1Sq, col1Sq, 0x55), col1Sq)
            );
            const hkQuadReal col1Approx = _mm_rsqrt_ss(col1LenSq);
            const hkQuadReal col1Inv = _mm_andnot_ps(
                _mm_cmpeq_ss(col1LenSq, _mm_setzero_ps()),
                _mm_mul_ss(
                    _mm_mul_ss(_mm_set_ss(0.5f), col1Approx),
                    _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(col1LenSq, col1Approx), col1Approx))
                )
            );
            col1 = _mm_mul_ps(_mm_shuffle_ps(col1Inv, col1Inv, 0), col1);
            capsuleToLocal.m_rotation.m_col0.m_quad = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(col1, col1, 0xC9), axisZXY),
                _mm_mul_ps(_mm_shuffle_ps(col1, col1, 0xD2), axisYZX)
            );
            capsuleToLocal.m_rotation.m_col1.m_quad = col1;
            capsuleToLocal.m_rotation.m_col2.m_quad = axis.m_quad;
            bRotated = true;
        }
    }
    if (!bRotated) {
        capsuleToLocal.m_rotation.m_col0.set(1.0f, 0.0f, 0.0f, 0.0f);
        capsuleToLocal.m_rotation.m_col1.set(0.0f, 1.0f, 0.0f, 0.0f);
        capsuleToLocal.m_rotation.m_col2.set(0.0f, 0.0f, 1.0f, 0.0f);
    }
    capsuleToLocal.m_translation.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(end.m_quad, start.m_quad));

    // The capsule in its own frame: z from bottomInGeom to topInGeom.
    const double halfHeight = height * 0.5;
    hkVector4 bottomInGeom;
    bottomInGeom.set(0.0f, 0.0f, static_cast<float>(-height * 0.5), 0.0f);
    hkVector4 topInGeom;
    topInGeom.set(0.0f, 0.0f, static_cast<float>(halfHeight), 0.0f);
    hkVector4 axisInGeom;
    axisInGeom.setSub4(topInGeom, bottomInGeom);
    hkVector4 axisNInGeom;
    axisNInGeom.set(0.0f, 0.0f, 1.0f, 0.0f);
    hkVector4 normalInGeom;
    normalInGeom.set(1.0f, 0.0f, 0.0f, 0.0f);
    hkVector4 binormalInGeom;
    binormalInGeom.set(0.0f, -1.0f, 0.0f, 0.0f);

    const int32_t numVerts = thetaSamples * (heightSamples + 2 * phiSamples - 1) + 2;
    hkMemoryRouter& reserveRouter = hkMemoryRouter::getInstance();
    if (static_cast<int32_t>(verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < numVerts) {
        hkArrayUtil::_reserve(*reserveRouter.m_heap, &verts, numVerts, sizeof(hkVector4));
    }

    // The top pole.
    const double poleDist = radius + halfHeight;
    hkVector4 vert;
    vert.set(0.0f, 0.0f, static_cast<float>(poleDist), 0.0f);
    vert.setTransformedPos(capsuleToLocal, vert);
    {
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (verts.m_size == (verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &verts, sizeof(hkVector4));
        }
        verts.m_data[verts.m_size].m_quad = vert.m_quad;
        verts.m_size++;
    }

    // The top cap's rings, from the equator's neighbour up to the pole's.
    for (int32_t j = phiSamples - 1; j >= 0; j--) {
        const float phiF = static_cast<float>(phiSamples);
        hkQuaternion rotationTop;
        rotationTop.setAxisAngle(
            binormalInGeom, static_cast<float>(static_cast<double>(j) / phiF * 3.1415927410125732 * 0.5)
        );
        hkVector4 topDirection;
        topDirection.setRotatedDir(rotationTop, normalInGeom);
        const float thetaF = static_cast<float>(thetaSamples);
        for (int32_t i = 0; i < thetaSamples; i++) {
            const double dist = static_cast<double>(i) / thetaF * 3.1415927410125732;
            hkQuaternion q;
            q.setAxisAngle(axisNInGeom, static_cast<float>(dist + dist));
            hkVector4 direction;
            direction.setRotatedDir(q, topDirection);
            vert.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radius), direction.m_quad), topInGeom.m_quad);
            vert.setTransformedPos(capsuleToLocal, vert);
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (verts.m_size == (verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &verts, sizeof(hkVector4));
            }
            verts.m_data[verts.m_size].m_quad = vert.m_quad;
            verts.m_size++;
        }
    }

    // The cylinder's rings, from the top down, between the caps' equators.
    for (int32_t j = heightSamples - 1; j > 0; j--) {
        const float thetaF = static_cast<float>(thetaSamples);
        const float t = static_cast<float>(static_cast<double>(j) / heightSamples);
        for (int32_t i = 0; i < thetaSamples; i++) {
            const double dist = static_cast<double>(i) / thetaF * 3.1415927410125732;
            hkQuaternion q;
            q.setAxisAngle(axisNInGeom, static_cast<float>(dist + dist));
            hkVector4 direction;
            direction.setRotatedDir(q, normalInGeom);
            vert.m_quad = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_set1_ps(t), axisInGeom.m_quad), bottomInGeom.m_quad),
                _mm_mul_ps(_mm_set1_ps(radius), direction.m_quad)
            );
            vert.setTransformedPos(capsuleToLocal, vert);
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (verts.m_size == (verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &verts, sizeof(hkVector4));
            }
            verts.m_data[verts.m_size].m_quad = vert.m_quad;
            verts.m_size++;
        }
    }

    // The bottom cap's rings, from the equator down to the pole's neighbour.
    for (int32_t j = 0; j < phiSamples; j++) {
        const float phiF = static_cast<float>(phiSamples);
        hkQuaternion rotationBottom;
        rotationBottom.setAxisAngle(
            binormalInGeom, static_cast<float>(-static_cast<double>(j) / phiF * 3.1415927410125732 * 0.5)
        );
        hkVector4 bottomDirection;
        bottomDirection.setRotatedDir(rotationBottom, normalInGeom);
        const float thetaF = static_cast<float>(thetaSamples);
        for (int32_t i = 0; i < thetaSamples; i++) {
            const double dist = static_cast<double>(i) / thetaF * 3.1415927410125732;
            hkQuaternion q;
            q.setAxisAngle(axisNInGeom, static_cast<float>(dist + dist));
            hkVector4 direction;
            direction.setRotatedDir(q, bottomDirection);
            vert.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radius), direction.m_quad), bottomInGeom.m_quad);
            vert.setTransformedPos(capsuleToLocal, vert);
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (verts.m_size == (verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &verts, sizeof(hkVector4));
            }
            verts.m_data[verts.m_size].m_quad = vert.m_quad;
            verts.m_size++;
        }
    }

    // The bottom pole.
    vert.set(0.0f, 0.0f, static_cast<float>(-poleDist), 0.0f);
    vert.setTransformedPos(capsuleToLocal, vert);
    {
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (verts.m_size == (verts.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &verts, sizeof(hkVector4));
        }
        verts.m_data[verts.m_size].m_quad = vert.m_quad;
        verts.m_size++;
    }

    // EXE-INLINED 0xCCE5E0
    hkGeometry* geom = static_cast<hkGeometry*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkGeometry)));
    if (geom) {
        geom->m_vertices.m_data = nullptr;
        geom->m_vertices.m_size = 0;
        geom->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        geom->m_triangles.m_data = nullptr;
        geom->m_triangles.m_size = 0;
        geom->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
    }

    // geom->m_vertices = verts (hkArray's operator=): a buffer of verts' size where the capacity is short.
    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
    const int32_t capacityAndFlags = geom->m_vertices.m_capacityAndFlags;
    const int32_t capacity = static_cast<int32_t>(capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
    if (capacity < verts.m_size) {
        if (capacityAndFlags >= 0) {
            heap.bufFree(geom->m_vertices.m_data, capacity * static_cast<int32_t>(sizeof(hkVector4)));
        }
        int32_t numBytes = verts.m_size * static_cast<int32_t>(sizeof(hkVector4));
        geom->m_vertices.m_data = static_cast<hkVector4*>(heap.bufAlloc(numBytes));
        geom->m_vertices.m_capacityAndFlags = numBytes / static_cast<int32_t>(sizeof(hkVector4));
    }
    hkVector4* pDst = geom->m_vertices.m_data;
    geom->m_vertices.m_size = verts.m_size;
    const hkVector4* pSrc = verts.m_data;
    for (int32_t n = verts.m_size; n > 0; n--) {
        pDst->m_quad = pSrc->m_quad;
        pDst++;
        pSrc++;
    }
    for (int32_t vi = 0; vi < geom->m_vertices.m_size; vi++) {
        geom->m_vertices.m_data[vi].setTransformedPos(transform, geom->m_vertices.m_data[vi]);
    }

    // The top fan: the top pole (vertex 0) with the first ring.
    hkArray<hkGeometry::Triangle>& triangles = geom->m_triangles;
    int32_t currentBaseIndex = 1;
    for (int32_t i = 1; i <= thetaSamples; i++) {
        const int32_t next = i % thetaSamples + 1;
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (triangles.m_size == (triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &triangles, sizeof(hkGeometry::Triangle));
        }
        hkGeometry::Triangle& tr = triangles.m_data[triangles.m_size];
        tr.m_a = 0;
        tr.m_b = i;
        tr.m_c = next;
        tr.m_material = -1;
        triangles.m_size++;
    }

    // Two triangles per quad between each ring and the next.
    for (int32_t j = heightSamples + 2 * phiSamples - 2; j > 0; j--) {
        for (int32_t i = 1; i <= thetaSamples; i++) {
            const int32_t a = currentBaseIndex + i - 1;
            const int32_t b = currentBaseIndex + i % thetaSamples;
            const int32_t c = b + thetaSamples;
            {
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (triangles.m_size ==
                    (triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*router.m_heap, &triangles, sizeof(hkGeometry::Triangle));
                }
                hkGeometry::Triangle& tr = triangles.m_data[triangles.m_size];
                tr.m_a = a;
                tr.m_b = a + thetaSamples;
                tr.m_c = c;
                tr.m_material = -1;
                triangles.m_size++;
            }
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (triangles.m_size == (triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &triangles, sizeof(hkGeometry::Triangle));
            }
            hkGeometry::Triangle& tr = triangles.m_data[triangles.m_size];
            tr.m_a = b;
            tr.m_b = a;
            tr.m_c = c;
            tr.m_material = -1;
            triangles.m_size++;
        }
        currentBaseIndex += thetaSamples;
    }

    // The bottom fan: the last ring with the bottom pole.
    for (int32_t i = 1; i <= thetaSamples; i++) {
        const int32_t prev = currentBaseIndex + i - 1;
        const int32_t next = i % thetaSamples + currentBaseIndex;
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (triangles.m_size == (triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &triangles, sizeof(hkGeometry::Triangle));
        }
        hkGeometry::Triangle& tr = triangles.m_data[triangles.m_size];
        tr.m_a = next;
        tr.m_b = prev;
        tr.m_c = currentBaseIndex + thetaSamples;
        tr.m_material = -1;
        triangles.m_size++;
    }
    geometryOut = geom;
}

// Performs the full weld (weldGeometryVertices) with keepVertexOrder false; its two remap arrays are discarded locals.
void hkGeometryUtils::weldVertices(hkGeometry& meshGeometry, float threshold) {
    hkArray<int32_t> vertexRemap;
    hkArray<int32_t> triangleRemap;
    const float kThreshold = threshold;
    hkBool keepVertexOrder;
    keepVertexOrder.m_bool = 0;
    weldGeometryVertices(meshGeometry, kThreshold, keepVertexOrder, vertexRemap, triangleRemap);
}
