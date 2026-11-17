#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <cmath>
#include <xmmintrin.h>

hkDisplayCylinder::hkDisplayCylinder(
    const hkVector4& top,
    const hkVector4& bottom,
    float radius,
    int32_t numSides,
    int32_t numHeightSegments
)
  : hkDisplayGeometry(HK_DISPLAY_CYLINDER), m_top(top), m_bottom(bottom), m_radius(radius), m_numSides(numSides),
    m_numHeightSegments(numHeightSegments) {}

void hkDisplayCylinder::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
    ;
}

// Builds the cylinder's triangle mesh: two cap centres and, for each side, the points of the wall along the axis, then
// the wall's two triangles per height segment and each cap's fan. The axis step is (m_top - m_bottom) over the height
// segment count, its normalized form picks a perpendicular through one of the three unit axes, and the radius vector is
// that perpendicular normalized and scaled by m_radius; a quaternion turns it by 2*pi/numSides per side. normalize3
// uses rsqrt plus one Newton step, a zero vector staying zero.
void hkDisplayCylinder::buildGeometry() {
    auto normalize3 = [](__m128 akValue) -> __m128 {
        __m128 const kSquares = _mm_mul_ps(akValue, akValue);
        __m128 const kSquareSum = _mm_add_ss(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        );
        float const fSquareSum = _mm_cvtss_f32(kSquareSum);
        __m128 const kRsqrt = _mm_rsqrt_ss(_mm_set_ss(fSquareSum));
        __m128 const kNewton = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), kRsqrt),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(_mm_set_ss(fSquareSum), kRsqrt), kRsqrt))
        );
        // Masks the scale to zero for a zero vector and still multiplies, so the signs of the lanes stay.
        __m128 const kScale = _mm_andnot_ps(_mm_cmpeq_ss(_mm_set_ss(fSquareSum), _mm_setzero_ps()), kNewton);
        return _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), akValue);
    };
    // Does what hkArray<hkVector4>::pushBack does.
    auto pushVertex = [this](__m128 akVertex) {
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        hkArray<hkVector4>& rVertices = m_geometry->m_vertices;
        int32_t const iSize = rVertices.m_size;
        if (static_cast<int32_t>(rVertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < iSize + 1) {
            int32_t iCapacity =
                2 * static_cast<int32_t>(rVertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
            if (iSize + 1 > iCapacity) {
                iCapacity = iSize + 1;
            }
            hkArrayUtil::_reserve(rRouter.m_heap, &rVertices, iCapacity, sizeof(hkVector4));
        }
        _mm_store_ps(&reinterpret_cast<float*>(&rVertices.m_data[iSize].m_quad)[0], akVertex);
        rVertices.m_size += 1;
    };
    // The material stored with every triangle is -1.
    auto pushTriangle = [this](int32_t aiA, int32_t aiB, int32_t aiC) {
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        hkArray<hkGeometry::Triangle>& rTriangles = m_geometry->m_triangles;
        int32_t const iSize = rTriangles.m_size;
        if (static_cast<int32_t>(rTriangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK) <
            iSize + 1) {
            int32_t iCapacity = 2 *
                static_cast<int32_t>(rTriangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK);
            if (iSize + 1 > iCapacity) {
                iCapacity = iSize + 1;
            }
            hkArrayUtil::_reserve(rRouter.m_heap, &rTriangles, iCapacity, sizeof(hkGeometry::Triangle));
        }
        hkGeometry::Triangle& rTriangle = rTriangles.m_data[iSize];
        rTriangle.m_a = aiA;
        rTriangle.m_b = aiB;
        rTriangle.m_c = aiC;
        rTriangle.m_material = -1;
        rTriangles.m_size += 1;
    };

    // The per-segment axis step, in float, then its normalized form.
    float const fInvHeight = static_cast<float>(1.0 / static_cast<double>(m_numHeightSegments));
    __m128 const kStep = _mm_mul_ps(
        _mm_set1_ps(fInvHeight),
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<float*>(&m_top.m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&m_bottom.m_quad)[0])
        )
    );
    __m128 const kDirection = normalize3(kStep);
    hkVector4 kDirectionV;
    _mm_store_ps(&reinterpret_cast<float*>(&kDirectionV.m_quad)[0], kDirection);
    // The unit axis that is least aligned with the direction: y when |x| is over 0.5, else x when |y| is not, else z.
    __m128 kAxis;
    if (fabsf(kDirectionV.m_quad[0]) > 0.5f) {
        kAxis = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    }
    else if (fabsf(kDirectionV.m_quad[1]) > 0.5f) {
        kAxis = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    }
    else {
        kAxis = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    }
    // cross(axis, direction) in shuffle form, normalized and scaled by the radius: the vector that sweeps the wall.
    __m128 const kPerpendicular = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kAxis, kAxis, 0xC9), _mm_shuffle_ps(kDirection, kDirection, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kAxis, kAxis, 0xD2), _mm_shuffle_ps(kDirection, kDirection, 0xC9))
    );
    __m128 const kRadiusVector = _mm_mul_ps(normalize3(kPerpendicular), _mm_set1_ps(m_radius));
    // The rotation from one side to the next: 2*pi (as a double constant equal to the float nearest 2*pi) over the side
    // count, in double before it is rounded to float.
    float const fSideAngle = static_cast<float>(6.2831854820251465 / static_cast<double>(m_numSides));
    hkQuaternion kSideRotation;
    kSideRotation.setAxisAngle(kDirectionV, fSideAngle);
    hkQuaternion kRotation;
    kRotation.setAxisAngle(kDirectionV, 0.0f);

    // The mesh: two empty arrays that own their storage, behind hkDisplayGeometry::m_geometry.
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    hkGeometry* pGeometry = static_cast<hkGeometry*>(pHeap->blockAlloc(sizeof(hkGeometry)));
    if (pGeometry) {
        pGeometry->m_vertices.m_data = nullptr;
        pGeometry->m_vertices.m_size = 0;
        pGeometry->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        pGeometry->m_triangles.m_data = nullptr;
        pGeometry->m_triangles.m_size = 0;
        pGeometry->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
    }
    m_geometry = pGeometry;

    // The two cap centres come first, the second one a segment count of steps along the axis.
    pushVertex(_mm_load_ps(&reinterpret_cast<float*>(&m_bottom.m_quad)[0]));
    __m128 const kTopCentre = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<float*>(&m_bottom.m_quad)[0]),
        _mm_mul_ps(_mm_set1_ps(static_cast<float>(m_numHeightSegments)), kStep)
    );
    pushVertex(kTopCentre);
    // Each side adds its wall points: the radius vector turned by the accumulated rotation, on the bottom plane, then
    // one point per height segment and one more.
    for (int32_t i = 0; i < m_numSides; ++i) {
        __m128 const kRotationV = _mm_load_ps(&reinterpret_cast<float*>(&kRotation.m_vec.m_quad)[0]);
        __m128 const kRadius = kRadiusVector;
        __m128 const kRotatedRadius = _mm_mul_ps(kRotationV, kRadius);
        __m128 const kRotatedSum = _mm_add_ss(
            _mm_shuffle_ps(kRotatedRadius, kRotatedRadius, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(kRotatedRadius, kRotatedRadius, 0x55), kRotatedRadius)
        );
        __m128 const kQw = _mm_shuffle_ps(kRotationV, kRotationV, 0xFF);
        __m128 const kCross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kRotationV, kRotationV, 0xC9), _mm_shuffle_ps(kRadius, kRadius, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(kRotationV, kRotationV, 0xD2), _mm_shuffle_ps(kRadius, kRadius, 0xC9))
        );
        __m128 const kRotated = _mm_add_ps(
            _mm_mul_ps(kCross, kQw),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kQw, kQw), _mm_set1_ps(-0.5f)), kRadius),
                _mm_mul_ps(_mm_shuffle_ps(kRotatedSum, kRotatedSum, 0), kRotationV)
            )
        );
        __m128 kVertex =
            _mm_add_ps(_mm_add_ps(kRotated, kRotated), _mm_load_ps(&reinterpret_cast<float*>(&m_bottom.m_quad)[0]));
        for (int32_t j = 0; j < m_numHeightSegments; ++j) {
            pushVertex(kVertex);
            kVertex = _mm_add_ps(kVertex, kStep);
        }
        pushVertex(kVertex);
        hkQuaternion const kPrevious = kRotation;
        kRotation.setMul(kPrevious, kSideRotation);
    }

    // The wall: two triangles per height segment and side.
    int32_t const iHeightPlusOne = m_numHeightSegments + 1;
    int32_t const iWrapLimit = iHeightPlusOne * m_numSides + 2;
    int32_t iCorner = 2;
    int32_t iUpper = 3;
    int32_t iUpperNext = m_numHeightSegments + 4;
    int32_t iLowerNext = m_numHeightSegments + 3;
    for (int32_t i = 0; i < m_numSides; ++i) {
        for (int32_t j = 0; j < m_numHeightSegments; ++j) {
            pushTriangle(iCorner, iLowerNext, iUpper);
            pushTriangle(iUpper, iLowerNext, iUpperNext);
            iCorner += 1;
            iUpper += 1;
            iUpperNext += 1;
            iLowerNext += 1;
        }
        iCorner += 1;
        iUpper += 1;
        iUpperNext += 1;
        iLowerNext += 1;
        if (iUpperNext >= iWrapLimit) {
            iLowerNext += 2 - iWrapLimit;
            iUpperNext += 2 - iWrapLimit;
        }
    }

    // The caps: a fan around each centre, the bottom one (vertex 0) turned one way and the top one (vertex 1) the
    // other.
    int32_t iCentre = 0;
    for (int32_t iCap = 0; iCap < 2; ++iCap) {
        int32_t iFirst = (iCap != 0 ? m_numHeightSegments : 0) + 2;
        for (int32_t i = 0; i < m_numSides; ++i) {
            int32_t iThird = iFirst + iHeightPlusOne;
            if (iThird >= iWrapLimit) {
                iThird += 2 - iWrapLimit;
            }
            if (iCap != 0) {
                pushTriangle(iCentre, iFirst, iThird);
            }
            else {
                pushTriangle(0, iThird, iFirst);
            }
            iFirst = iThird;
        }
        iCentre = iCap + 1;
    }
}
