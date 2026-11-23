#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Copies 6 quads (96 bytes) from src to dst, as hkString::memCpy16<96> does on Win32;
// hkpExtendedMeshShape::getChildShape copies a TrianglesSubpart with it.
static void copyTrianglesSubpart(void* dst, const void* src);

// Equivalent of hkString::memCpy16<96>: copies a quad (16 bytes) at a time, as 32-bit words.
static void copyTrianglesSubpart(void* dst, const void* src) {
    const uint32_t* src32 = static_cast<const uint32_t*>(src);
    uint32_t* dst32 = static_cast<uint32_t*>(dst);
    int32_t numQuads = 6;
    do {
        dst32[0] = src32[0];
        dst32[1] = src32[1];
        dst32[2] = src32[2];
        dst32[3] = src32[3];
        dst32 += 4;
        src32 += 4;
    }
    while (--numQuads > 0);
}

// Copies member by member; the padding after m_numVertices and after m_flipAlternateTriangles is not
// copied.
hkpExtendedMeshShape::TrianglesSubpart&
hkpExtendedMeshShape::TrianglesSubpart::operator=(const hkpExtendedMeshShape::TrianglesSubpart& other) {
    static_cast<hkpExtendedMeshShape::Subpart&>(*this) = static_cast<const hkpExtendedMeshShape::Subpart&>(other);
    m_numTriangleShapes = other.m_numTriangleShapes;
    m_vertexBase = other.m_vertexBase;
    m_vertexStriding = other.m_vertexStriding;
    m_numVertices = other.m_numVertices;
    m_extrusion.m_quad = other.m_extrusion.m_quad;
    m_indexBase = other.m_indexBase;
    m_indexStriding = other.m_indexStriding;
    m_stridingType = other.m_stridingType;
    m_flipAlternateTriangles = other.m_flipAlternateTriangles;
    m_triangleOffset = other.m_triangleOffset;
    m_userData = other.m_userData;
    return *this;
}

hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart() : Subpart(SUBPART_SHAPE) {}

// When finishing, the transform flags in m_translation's w are recomputed: FLAG_TRANSLATE_SET for a translation
// beyond 0.001 on any axis, FLAG_ROTATE_SET for a rotation of 0.001 or more.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(hkFinishLoadedObjectFlag flag) : Subpart(flag), m_childShapes(flag) {
    if (flag.m_finishing) {
        int32_t iFlags = !(std::fabsf(XFNV_M128_F32(m_translation.m_quad, 0)) <= 0.001f &&
                           std::fabsf(XFNV_M128_F32(m_translation.m_quad, 1)) <= 0.001f &&
                           std::fabsf(XFNV_M128_F32(m_translation.m_quad, 2)) <= 0.001f)
            ? FLAG_TRANSLATE_SET
            : FLAG_NONE;
        if (!(m_rotation.getAngle() < 0.001f)) {
            iFlags |= FLAG_ROTATE_SET;
        }
        m_translation.setInt24W(iFlags);
    }
}

// The SDK's constructor from the children and an offset: no rotation; FLAG_TRANSLATE_SET when the offset is beyond
// 0.001 on any axis. Each child is referenced.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(
    const hkpConvexShape* const* childShapes,
    int32_t numChildShapes,
    const hkVector4& offset
)
  : Subpart(SUBPART_SHAPE), m_translation(offset) {
    hkRefPtr<hkpConvexShape>* pChildren = m_childShapes.expandBy(numChildShapes);
    for (int32_t i = 0; i < numChildShapes; ++i) {
        pChildren[i] = const_cast<hkpConvexShape*>(childShapes[i]);
    }
    m_rotation.setIdentity();
    m_translation.setInt24W(
        !(std::fabsf(XFNV_M128_F32(offset.m_quad, 0)) <= 0.001f &&
          std::fabsf(XFNV_M128_F32(offset.m_quad, 1)) <= 0.001f &&
          std::fabsf(XFNV_M128_F32(offset.m_quad, 2)) <= 0.001f)
            ? FLAG_TRANSLATE_SET
            : FLAG_NONE
    );
}

// The SDK's constructor from the children and a transform: FLAG_TRANSLATE_SET when the translation is beyond 0.001
// on any axis, FLAG_ROTATE_SET when the rotation is not the identity within 0.001. Each child is referenced.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(
    const hkpConvexShape* const* childShapes,
    int32_t numChildShapes,
    const hkTransform& r
)
  : Subpart(SUBPART_SHAPE), m_translation(r.m_translation) {
    m_rotation.set(r.m_rotation);
    hkRefPtr<hkpConvexShape>* pChildren = m_childShapes.expandBy(numChildShapes);
    for (int32_t i = 0; i < numChildShapes; ++i) {
        pChildren[i] = const_cast<hkpConvexShape*>(childShapes[i]);
    }
    int32_t iFlags = !(std::fabsf(XFNV_M128_F32(m_translation.m_quad, 0)) <= 0.001f &&
                       std::fabsf(XFNV_M128_F32(m_translation.m_quad, 1)) <= 0.001f &&
                       std::fabsf(XFNV_M128_F32(m_translation.m_quad, 2)) <= 0.001f)
        ? FLAG_TRANSLATE_SET
        : FLAG_NONE;
    if (!r.m_rotation.isApproximatelyEqual(hkTransformIdentity.m_rotation, 0.001f).m_bool) {
        iFlags |= FLAG_ROTATE_SET;
    }
    m_translation.setInt24W(iFlags);
}

hkpExtendedMeshShape::ShapesSubpart::~ShapesSubpart() {}

hkpExtendedMeshShape::ShapesSubpart&
hkpExtendedMeshShape::ShapesSubpart::operator=(const hkpExtendedMeshShape::ShapesSubpart& other) {
    static_cast<hkpExtendedMeshShape::Subpart&>(*this) = static_cast<const hkpExtendedMeshShape::Subpart&>(other);
    m_childShapes = other.m_childShapes;
    m_rotation = other.m_rotation;
    m_translation = other.m_translation;
    return *this;
}

// The loaded subparts stay as loaded. When finishing: every subpart without a material index striding type gets
// MATERIAL_INDICES_INT8, a single triangles subpart moves into m_embeddedTrianglesSubpart (the array points at it as
// user storage), and a cached child count of -1 becomes INT32_MIN (still to be counted).
hkpExtendedMeshShape::hkpExtendedMeshShape(hkFinishLoadedObjectFlag flag)
  : hkpShapeCollection(flag), m_embeddedTrianglesSubpart(flag), m_trianglesSubparts(flag), m_shapesSubparts(flag),
    m_weldingInfo(flag) {
    if (!flag.m_finishing) {
        m_type.m_storage = HK_SHAPE_EXTENDED_MESH;
        return;
    }
    m_collectionType.m_storage = COLLECTION_EXTENDED_MESH;
    for (TrianglesSubpart& part : m_trianglesSubparts) {
        if (part.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INVALID) {
            part.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
        }
    }
    for (ShapesSubpart& part : m_shapesSubparts) {
        if (part.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INVALID) {
            part.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
        }
    }
    if (m_trianglesSubparts.m_size == 1) {
        m_embeddedTrianglesSubpart = m_trianglesSubparts.m_data[0];
        m_trianglesSubparts.clearAndDeallocate();
        m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
        m_trianglesSubparts.m_size = 1;
        m_trianglesSubparts.m_capacityAndFlags = 1 | hkArrayBase<TrianglesSubpart>::DONT_DEALLOCATE_FLAG;
    }
    m_type.m_storage = HK_SHAPE_EXTENDED_MESH;
    if (m_cachedNumChildShapes == -1) {
        m_cachedNumChildShapes = INT32_MIN;
    }
}

// An empty shape: unit scaling, no welding, an empty box (half extents -HK_REAL_MAX).
hkpExtendedMeshShape::hkpExtendedMeshShape(float radius, int32_t numBitsForSubpartIndex)
  : hkpShapeCollection(HK_SHAPE_EXTENDED_MESH, COLLECTION_EXTENDED_MESH),
    m_numBitsForSubpartIndex(numBitsForSubpartIndex), m_defaultCollisionFilterInfo(0), m_cachedNumChildShapes(0),
    m_triangleRadius(radius) {
    m_scaling.setAll(1.0f);
    m_weldingType.m_storage = hkpWeldingUtility::WELDING_TYPE_NONE;
    m_aabbHalfExtents.setAll(-HK_REAL_MAX);
    m_aabbCenter.m_quad = _mm_setzero_ps();
}

// The mesh shape's welding, scaling and radius are taken over and each of its subparts becomes a triangles subpart
// (a single one in m_embeddedTrianglesSubpart); then the box and the child count are computed.
hkpExtendedMeshShape::hkpExtendedMeshShape(const hkpMeshShape* apMeshShape)
  : hkpShapeCollection(HK_SHAPE_EXTENDED_MESH, COLLECTION_EXTENDED_MESH), m_scaling(apMeshShape->m_scaling),
    m_numBitsForSubpartIndex(apMeshShape->m_numBitsForSubpartIndex), m_defaultCollisionFilterInfo(0),
    m_triangleRadius(apMeshShape->m_radius) {
    m_weldingType.m_storage = apMeshShape->m_weldingType.m_storage;

    const int32_t iNumWelding = apMeshShape->m_weldingInfo.m_size;
    m_weldingInfo.reserveExactly(iNumWelding);
    m_weldingInfo.setSizeUnchecked(iNumWelding);
    for (int32_t i = 0; i < iNumWelding; ++i) {
        m_weldingInfo.m_data[i] = apMeshShape->m_weldingInfo.m_data[i];
    }

    const int32_t iNumSubparts = apMeshShape->m_subparts.m_size;
    if (iNumSubparts == 1) {
        m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
        m_trianglesSubparts.m_size = 1;
        m_trianglesSubparts.m_capacityAndFlags = 1 | hkArrayBase<TrianglesSubpart>::DONT_DEALLOCATE_FLAG;
    }
    else {
        m_trianglesSubparts.reserve(iNumSubparts);
        m_trianglesSubparts.setSizeUnchecked(iNumSubparts);
        for (int32_t i = 0; i < iNumSubparts; ++i) {
            ::new (&m_trianglesSubparts.m_data[i]) TrianglesSubpart;
        }
    }
    for (int32_t i = 0; i < iNumSubparts; ++i) {
        TrianglesSubpart& rPart = m_trianglesSubparts.m_data[i];
        const hkpMeshShape::Subpart& rMeshPart = apMeshShape->m_subparts.m_data[i];
        rPart.m_numTriangleShapes = rMeshPart.m_numTriangles;
        rPart.m_materialIndexBase = rMeshPart.m_materialIndexBase;
        rPart.m_materialIndexStriding = rMeshPart.m_materialIndexStriding;
        rPart.m_materialIndexStridingType.m_storage = rMeshPart.m_materialIndexStridingType.m_storage;
        rPart.m_materialBase = rMeshPart.m_materialBase;
        rPart.m_materialStriding = rMeshPart.m_materialStriding;
        rPart.m_numMaterials = rMeshPart.m_numMaterials;
        rPart.m_vertexBase = rMeshPart.m_vertexBase;
        rPart.m_vertexStriding = rMeshPart.m_vertexStriding;
        rPart.m_numVertices = rMeshPart.m_numVertices;
        rPart.m_indexBase = rMeshPart.m_indexBase;
        rPart.m_stridingType.m_storage = rMeshPart.m_stridingType.m_storage;
        rPart.m_indexStriding = rMeshPart.m_indexStriding;
        rPart.m_triangleOffset = rMeshPart.m_triangleOffset;
        rPart.m_extrusion.m_quad = _mm_setzero_ps();
        rPart.m_flipAlternateTriangles = rMeshPart.m_flipAlternateTriangles;
    }
    recalcAabbExtents();
    m_cachedNumChildShapes = hkpShapeContainer::getNumChildShapes();
}

// The three arrays free their own storage, m_shapesSubparts after each subpart's destructor.
hkpExtendedMeshShape::~hkpExtendedMeshShape() {}

int32_t
hkpExtendedMeshShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    hkpExtendedMeshShape::ShapesSubpart* v4;
    int32_t v5;
    int32_t i;
    m_shapesSubparts.m_size;
    int32_t v11 = 0;
    if (m_shapesSubparts.m_size <= 0) {
        return 240;
    }
    for (i = 0;; ++i) {
        v4 = &m_shapesSubparts.m_data[i];
        for (v5 = 0; v5 < v4->m_childShapes.m_size; ++v5) {
            hkpShape* v6 = v4->m_childShapes.m_data[v5];
            int32_t v7 = v6->calcSizeForSpu(input, 256);
            int32_t v8 = (std::bit_cast<int32_t>(XFNV_M128_F32(v4->m_translation.m_quad, 3)) & 0xC0FFFFFFu) != 0
                ? (std::bit_cast<int32_t>(XFNV_M128_F32(v4->m_translation.m_quad, 3)) & 0xC0FFFFFFu) != 1 ? 416 : 464
                : 512;
            if (v7 < 0 || v7 > v8) {
                return -1;
            }
        }
        if (++v11 >= m_shapesSubparts.m_size) {
            return 240;
        }
    }
}

void hkpExtendedMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkAabbUtil::calcAabb(localToWorld, m_aabbHalfExtents, m_aabbCenter, tolerance, out);
}

void hkpExtendedMeshShape::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    m_weldingType.m_storage = static_cast<uint8_t>(weldingType);
    if (weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
        hkMemoryAllocator* m_heap = hkMemoryRouter::getInstance().m_heap;
        int32_t m_capacityAndFlags = m_weldingInfo.m_capacityAndFlags;
        m_weldingInfo.m_size = 0;
        if (m_capacityAndFlags >= 0) {
            m_heap->bufFree(
                m_weldingInfo.begin(),
                sizeof(uint16_t) * (m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)
            );
        }
        m_weldingInfo.m_data = nullptr;
        m_weldingInfo.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
    }
    else {
        int32_t v3 = 0;
        int32_t v4 = 0;
        if (m_trianglesSubparts.m_size > 0) {
            hkpExtendedMeshShape::TrianglesSubpart* m_data = m_trianglesSubparts.begin();
            int32_t v6 = 0;
            do {
                m_data[v6].m_triangleOffset = v3;
                m_data = m_trianglesSubparts.begin();
                v3 += m_data[v6].m_numTriangleShapes;
                ++v4;
                ++v6;
            }
            while (v4 < m_trianglesSubparts.m_size);
        }
        hkArray<uint16_t, hkContainerHeapAllocator>* p_m_weldingInfo = &m_weldingInfo;
        p_m_weldingInfo->reserveExactly(v3);
        p_m_weldingInfo->setSize(v3);
    }
}

void hkpExtendedMeshShape::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    if (!m_weldingInfo.m_size) {
        int32_t v4 = 0;
        int32_t v5 = 0;
        if (m_trianglesSubparts.m_size > 0) {
            hkpExtendedMeshShape::TrianglesSubpart* m_data = m_trianglesSubparts.begin();
            int32_t v7 = 0;
            do {
                m_data[v7].m_triangleOffset = v5;
                m_data = m_trianglesSubparts.begin();
                v5 += m_data[v7].m_numTriangleShapes;
                ++v4;
                ++v7;
            }
            while (v4 < m_trianglesSubparts.m_size);
        }
        m_weldingInfo.reserveExactly(v5);
        m_weldingInfo.setSize(v5);
    }
    if ((key & 0x80000000u) == 0) {
        m_weldingInfo.m_data
            [m_trianglesSubparts.m_data[(key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex)].m_triangleOffset +
             (key & (~0u >> m_numBitsForSubpartIndex))] = weldingInfo;
    }
}

void hkpExtendedMeshShape::addTrianglesSubpart(const hkpExtendedMeshShape::TrianglesSubpart& part) {
    hkVector4 v4;
    hkVector4 v5;
    hkAabb arOut;
    hkpExtendedMeshShape::TrianglesSubpart* v3 = expandOneTriangleSubparts();
    *v3 = part;
    v4.m_quad = m_aabbCenter.m_quad;
    v5.m_quad = m_aabbHalfExtents.m_quad;
    __m128 v10 = _mm_sub_ps(v4.m_quad, v5.m_quad);
    __m128 v11 = _mm_add_ps(v5.m_quad, v4.m_quad);
    calcAabbExtents(*v3, m_scaling, arOut);
    __m128 v6 = _mm_shuffle_ps(_mm_set_ss(m_triangleRadius), _mm_set_ss(m_triangleRadius), 0);
    __m128 v7 = _mm_min_ps(v10, _mm_sub_ps(arOut.m_min.m_quad, v6));
    __m128 v8 = _mm_max_ps(v11, _mm_add_ps(arOut.m_max.m_quad, v6));
    int32_t v9 = m_trianglesSubparts.m_size - 1;
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(v8, v7), hkQuadRealHalf);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(v8, v7), hkQuadRealHalf);
    m_cachedNumChildShapes += _getNumChildShapesInTrianglesSubpart(part, v9);
}

// The SDK's addShapesSubpart: a new subpart is appended and assigned from aSubpart, the shape's AABB grown by the
// subpart's, and the subpart's child shapes added to the cached count. Returns the new subpart's index.
int32_t hkpExtendedMeshShape::addShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& part) {
    hkpExtendedMeshShape::ShapesSubpart* pPart = expandOneShapesSubparts();
    *pPart = part;
    const hkQuadReal kOldMin = _mm_sub_ps(m_aabbCenter.m_quad, m_aabbHalfExtents.m_quad);
    const hkQuadReal kOldMax = _mm_add_ps(m_aabbHalfExtents.m_quad, m_aabbCenter.m_quad);
    hkAabb kPartAabb;
    calcAabbExtents(pPart, &kPartAabb);
    const hkQuadReal kMin = _mm_min_ps(kOldMin, kPartAabb.m_min.m_quad);
    const hkQuadReal kMax = _mm_max_ps(kOldMax, kPartAabb.m_max.m_quad);
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(kMax, kMin), hkQuadRealHalf);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(kMax, kMin), hkQuadRealHalf);
    m_cachedNumChildShapes += _getNumChildShapesInShapesSubpart(part);
    return m_shapesSubparts.m_size - 1;
}

// The count is cached in the mutable m_cachedNumChildShapes on first use.
int32_t hkpExtendedMeshShape::getNumChildShapes() const {
    if (m_cachedNumChildShapes < 0) {
        m_cachedNumChildShapes = static_cast<const hkpShapeContainer*>(this)->hkpShapeContainer::getNumChildShapes();
    }
    return m_cachedNumChildShapes;
}

// The first child's key, or the next one when that child is a degenerate triangle.
uint32_t hkpExtendedMeshShape::getFirstKey() const {
    const hkpShapeContainer* pContainer = this;
    int32_t numTrianglesSubparts = m_trianglesSubparts.m_size;
    if (!(numTrianglesSubparts + m_shapesSubparts.m_size)) {
        return 0xFFFFFFFFu;
    }
    uint32_t key = numTrianglesSubparts != 0 ? 0 : 0x80000000u;
    alignas(16) char buffer[512];
    const hkpShape* pChild = pContainer->getChildShape(key, buffer);
    if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
        return key;
    }
    const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pChild);
    if (!hkpTriangleUtil::isDegenerate(
             pTriangle->getVertex(0),
             pTriangle->getVertex(1),
             pTriangle->getVertex(2),
             hkDefaultTriangleDegeneracyTolerance
        )
             .m_bool) {
        return key;
    }
    return pContainer->getNextKey(key);
}

// Steps to the next child, past the end of each subpart into the next and from the triangles subparts into the
// shapes subparts, skipping degenerate triangles.
uint32_t hkpExtendedMeshShape::getNextKey(uint32_t initialKey) const {
    const hkpShapeContainer* pContainer = this;
    uint32_t subpartIndex = (initialKey & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
    int32_t terminalIndex = initialKey & (~0u >> m_numBitsForSubpartIndex);
    uint32_t shapesBit = initialKey & 0x80000000u;
    alignas(16) char buffer[512];
    while (true) {
        ++terminalIndex;
        if (!shapesBit) {
            if (terminalIndex >= m_trianglesSubparts.m_data[subpartIndex].m_numTriangleShapes) {
                ++subpartIndex;
                terminalIndex = 0;
                if (subpartIndex >= static_cast<uint32_t>(m_trianglesSubparts.m_size)) {
                    if (m_shapesSubparts.m_size == 0) {
                        return 0xFFFFFFFFu;
                    }
                    subpartIndex = 0;
                    shapesBit = 0x80000000u;
                    terminalIndex = -1;
                    continue;
                }
            }
        }
        else if (terminalIndex >= m_shapesSubparts.m_data[subpartIndex].m_childShapes.m_size) {
            ++subpartIndex;
            if (subpartIndex >= static_cast<uint32_t>(m_shapesSubparts.m_size)) {
                return 0xFFFFFFFFu;
            }
            terminalIndex = 0;
        }
        uint32_t key = (subpartIndex << (32 - m_numBitsForSubpartIndex)) | shapesBit | terminalIndex;
        const hkpShape* pChild = pContainer->getChildShape(key, buffer);
        if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
            return key;
        }
        const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pChild);
        if (!hkpTriangleUtil::isDegenerate(
                 pTriangle->getVertex(0),
                 pTriangle->getVertex(1),
                 pTriangle->getVertex(2),
                 hkDefaultTriangleDegeneracyTolerance
            )
                 .m_bool) {
            return key;
        }
    }
}

uint32_t hkpExtendedMeshShape::getCollisionFilterInfo(uint32_t key) const {
    const hkpMeshMaterial* MeshMaterial = getMeshMaterial(key);
    if (MeshMaterial) {
        return MeshMaterial->m_filterInfo;
    }

    return m_defaultCollisionFilterInfo;
}

const hkpShape* hkpExtendedMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
    const uint32_t subpartIndex = (key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
    const uint32_t terminalIndex = key & (0xFFFFFFFFu >> m_numBitsForSubpartIndex);
    // The key's top bit is the subpart type: set for SUBPART_SHAPE.
    if ((key & 0x80000000u) != 0) {
        // The subpart is read in place rather than copied: a byte copy would be destroyed as an owner of the child
        // shapes.
        const ShapesSubpart& part = m_shapesSubparts.m_data[subpartIndex];
        hkpConvexShape* childShape = part.m_childShapes.m_data[terminalIndex];
        const int32_t flags = part.m_translation.getInt24W();
        if (flags == ShapesSubpart::FLAG_NONE) {
            return childShape;
        }
        if (flags == ShapesSubpart::FLAG_TRANSLATE_SET) {
            if (!&buffer) {
                return nullptr;
            }
            return ::new (&buffer[0])
                hkpConvexTranslateShape(childShape, part.m_translation, hkpShapeContainer::REFERENCE_POLICY_IGNORE);
        }
        hkTransform transform;
        transform.m_translation.m_quad = part.m_translation.m_quad;
        transform.m_rotation.set(part.m_rotation);
        if (!&buffer) {
            return nullptr;
        }
        return ::new (&buffer[0])
            hkpConvexTransformShape(childShape, transform, hkpShapeContainer::REFERENCE_POLICY_IGNORE);
    }

    // A single triangles subpart is the embedded one; any other is copied to an aligned local first. The local is
    // built with the constructor that stores nothing: the copy fills it.
    const TrianglesSubpart* part;
    TrianglesSubpart partCopy(hkFinishLoadedObjectFlag{});
    if (m_trianglesSubparts.m_size == 1) {
        part = &m_embeddedTrianglesSubpart;
    }
    else {
        copyTrianglesSubpart(&partCopy, &m_trianglesSubparts.m_data[subpartIndex]);
        part = &partCopy;
    }
    // The index buffer is untyped strided memory (the SDK's hkAddByteOffsetConst), read at the striding's width.
    const uint8_t* triangle = static_cast<const uint8_t*>(part->m_indexBase) + terminalIndex * part->m_indexStriding;
    // Every other triangle of a strip is flipped: its second and third indices swap.
    const int32_t flip = static_cast<int32_t>(terminalIndex) & part->m_flipAlternateTriangles;
    const int16_t b = static_cast<int16_t>(flip + 1);
    const int16_t c = static_cast<int16_t>((flip ^ 1) + 1);
    int32_t index0;
    int32_t index1;
    int32_t index2;
    switch (part->m_stridingType.m_storage) {
        case INDICES_INT8:
            index0 = triangle[0];
            index1 = triangle[b];
            index2 = triangle[c];
            break;
        case INDICES_INT16: {
            const uint16_t* indices = reinterpret_cast<const uint16_t*>(triangle);
            index0 = indices[0];
            index1 = indices[b];
            index2 = indices[c];
            break;
        }
        case INDICES_INT32: {
            const int32_t* indices = reinterpret_cast<const int32_t*>(triangle);
            index0 = indices[0];
            index1 = indices[b];
            index2 = indices[c];
            break;
        }
        default:
            index0 = 0;
            index1 = 0;
            index2 = 0;
            break;
    }
    // The vertex buffer is untyped strided memory as well (three floats at each vertexStriding bytes).
    const char* vertexBase = reinterpret_cast<const char*>(part->m_vertexBase);
    const int32_t vertexStriding = part->m_vertexStriding;

    // Placement new of the triangle shape in the buffer (default radius, no welding).
    hkpTriangleShape* pTriangle = nullptr;
    if (&buffer) {
        pTriangle = ::new (&buffer[0])
            hkpTriangleShape(hkConvexShapeDefaultRadius, 0, hkpWeldingUtility::WELDING_TYPE_NONE);
    }
    pTriangle->m_weldingType.m_storage = m_weldingType.m_storage;
    pTriangle->m_radius = m_triangleRadius;
    // The extrusion's squared length, summed x + y, then + z.
    const hkQuadReal extrusion = part->m_extrusion.m_quad;
    const hkQuadReal squares = _mm_mul_ps(extrusion, extrusion);
    const float xy = XFNV_M128_F32(squares, 1) + XFNV_M128_F32(squares, 0);
    const float lengthSquared = XFNV_M128_F32(squares, 2) + xy;
    pTriangle->m_type.m_storage = HK_SHAPE_TRIANGLE;
    pTriangle->m_isExtruded = lengthSquared > 0.0f ? 1 : 0;
    pTriangle->m_extrusion.m_quad = extrusion;
    pTriangle->m_weldingInfo = m_weldingInfo.m_size == 0 ? 0
                                                         : m_weldingInfo.m_data[part->m_triangleOffset + terminalIndex];
    // Starts at 0 (a departure): load3 writes only x, y and z, and the whole vector is scaled into each vertex; the
    // exe reads its w unset.
    hkVector4 vertex;
    vertex.setZero4();
    vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index0));
    pTriangle->m_vertexA.setMul4(vertex, m_scaling);
    vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index1));
    pTriangle->m_vertexB.setMul4(vertex, m_scaling);
    vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index2));
    pTriangle->m_vertexC.setMul4(vertex, m_scaling);
    return pTriangle;
}

// The SDK's count of a triangles subpart's child shapes: its triangles that are not degenerate. Each triangle is
// fetched by its shape key into a 512-byte buffer.
int32_t hkpExtendedMeshShape::_getNumChildShapesInTrianglesSubpart(
    const hkpExtendedMeshShape::TrianglesSubpart& subpart,
    int32_t subpartIndex
) const {
    int32_t iNumChildShapes = 0;
    for (int32_t i = 0; i < subpart.m_numTriangleShapes; ++i) {
        alignas(16) char buffer[512];
        const uint32_t uiKey = static_cast<uint32_t>(subpartIndex) << (32 - m_numBitsForSubpartIndex) |
            static_cast<uint32_t>(i);
        const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(
            hkpExtendedMeshShape::getChildShape(uiKey, buffer)
        );
        if (!hkpTriangleUtil::isDegenerate(
                 pTriangle->getVertex(0),
                 pTriangle->getVertex(1),
                 pTriangle->getVertex(2),
                 hkDefaultTriangleDegeneracyTolerance
            )
                 .m_bool) {
            ++iNumChildShapes;
        }
    }
    return iNumChildShapes;
}

void hkpExtendedMeshShape::recalcAabbExtents() {
    hkAabb aabb;
    __m128 vMin = _mm_shuffle_ps(_mm_set_ss(HK_REAL_MAX), _mm_set_ss(HK_REAL_MAX), 0);
    __m128 vMax = _mm_xor_ps(hkQuadRealNegateMask, vMin);
    __m128 vRadius = _mm_shuffle_ps(_mm_set_ss(m_triangleRadius), _mm_set_ss(m_triangleRadius), 0);
    for (const TrianglesSubpart& part : m_trianglesSubparts) {
        calcAabbExtents(part, m_scaling, aabb);
        vMin = _mm_min_ps(vMin, _mm_sub_ps(aabb.m_min.m_quad, vRadius));
        vMax = _mm_max_ps(vMax, _mm_add_ps(aabb.m_max.m_quad, vRadius));
    }
    for (const ShapesSubpart& part : m_shapesSubparts) {
        calcAabbExtents(&part, &aabb);
        vMin = _mm_min_ps(vMin, aabb.m_min.m_quad);
        vMax = _mm_max_ps(vMax, aabb.m_max.m_quad);
    }
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(vMax, vMin), hkQuadRealHalf);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(vMax, vMin), hkQuadRealHalf);
}

hkpExtendedMeshShape::TrianglesSubpart* hkpExtendedMeshShape::expandOneTriangleSubparts() {
    if (m_trianglesSubparts.m_size) {
        return m_trianglesSubparts.expandBy(1);
    }

    m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
    m_trianglesSubparts.m_size = 1;
    m_trianglesSubparts.m_capacityAndFlags = -0x7FFFFFFF;
    return &m_embeddedTrianglesSubpart;
}

hkpExtendedMeshShape::ShapesSubpart* hkpExtendedMeshShape::expandOneShapesSubparts() {
    return m_shapesSubparts.expandBy(1);
}

void hkpExtendedMeshShape::setScaling(const hkVector4& scaling) {
    m_scaling.m_quad = scaling.m_quad;
}

int32_t
hkpExtendedMeshShape::_getNumChildShapesInShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& subpart) const {
    return subpart.m_childShapes.m_size;
}

// Does the work of getSubPart, getSubPartIndex and getTerminalIndexInSubPart; a single triangles subpart is the
// embedded one.
const hkpMeshMaterial* hkpExtendedMeshShape::getMeshMaterial(uint32_t key) const {
    uint32_t subpartIndex = (key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
    int32_t terminalIndex = key & (~0u >> m_numBitsForSubpartIndex);
    const Subpart* pPart;
    if (key & 0x80000000u) {
        pPart = &m_shapesSubparts.m_data[subpartIndex];
    }
    else {
        uint32_t numTrianglesSubparts = m_trianglesSubparts.m_size;
        if (numTrianglesSubparts == 1) {
            pPart = &m_embeddedTrianglesSubpart;
        }
        else {
            if (subpartIndex >= numTrianglesSubparts) {
                return nullptr;
            }
            pPart = &m_trianglesSubparts.m_data[subpartIndex];
        }
    }
    const uint8_t* pIndexBase = static_cast<const uint8_t*>(pPart->m_materialIndexBase);
    int32_t materialStriding = pPart->m_materialStriding;
    if (!pIndexBase) {
        return nullptr;
    }
    const uint8_t* pIndex = pIndexBase + terminalIndex * pPart->m_materialIndexStriding;
    int32_t materialOffset;
    if (pPart->m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
        materialOffset = materialStriding * *pIndex;
    }
    else {
        // The material index buffer is the user's untyped memory: 16-bit indices here.
        materialOffset = materialStriding * *reinterpret_cast<const uint16_t*>(pIndex);
    }
    // The SDK's hkAddByteOffsetConst over the user's untyped material buffer: materials are m_materialStriding
    // bytes apart.
    return reinterpret_cast<const hkpMeshMaterial*>(
        reinterpret_cast<const char*>(pPart->m_materialBase) + materialOffset
    );
}

// Each vertex is scaled into floats, packed as (x, y, z, 0), transformed by the identity transform, and added to the
// box with min/max (the box as the first operand).
void calcAabbExtents(const hkpExtendedMeshShape::TrianglesSubpart& part, hkVector4& scaling, hkAabb& out) {
    // HK_REAL_MAX; the maximum is its sign flipped.
    __m128 kMax = _mm_set1_ps(HK_REAL_MAX);
    _mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0], kMax);
    _mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0], _mm_xor_ps(kMax, _mm_set1_ps(-0.0f)));
    // The vertex and index buffers are untyped strided memory: the casts below address them by bytes.
    for (int32_t i = 0; i < part.m_numTriangleShapes; ++i) {
        const char* pIndex = static_cast<const char*>(part.m_indexBase) + part.m_indexStriding * i;
        uint32_t uiA;
        uint32_t uiB;
        uint32_t uiC;
        if (part.m_stridingType.m_storage == hkpExtendedMeshShape::INDICES_INT8) {
            const uint8_t* pIndices = reinterpret_cast<const uint8_t*>(pIndex);
            uiA = pIndices[0];
            uiB = pIndices[1];
            uiC = pIndices[2];
        }
        else if (part.m_stridingType.m_storage == hkpExtendedMeshShape::INDICES_INT16) {
            const uint16_t* pIndices = reinterpret_cast<const uint16_t*>(pIndex);
            uiA = pIndices[0];
            uiB = pIndices[1];
            uiC = pIndices[2];
        }
        else {
            const uint32_t* pIndices = reinterpret_cast<const uint32_t*>(pIndex);
            uiA = pIndices[0];
            uiB = pIndices[1];
            uiC = pIndices[2];
        }
        const char* pVertices = reinterpret_cast<const char*>(part.m_vertexBase);
        int32_t iStriding = part.m_vertexStriding;
        const float* apTriangle[3] = {
            reinterpret_cast<const float*>(pVertices + uiA * iStriding),
            reinterpret_cast<const float*>(pVertices + uiB * iStriding),
            reinterpret_cast<const float*>(pVertices + uiC * iStriding),
        };
        // Each vertex gets its own pair of slots in akFrame, the scaled vertex followed by the transformed point, so no
        // vertex reuses the slot the previous call wrote.
        alignas(16) hkVector4 akFrame[6];
        for (int32_t j = 0; j < 3; ++j) {
            const float* pVertex = apTriangle[j];
            float fX = pVertex[0] * XFNV_M128_F32(scaling.m_quad, 0);
            float fY = pVertex[1] * XFNV_M128_F32(scaling.m_quad, 1);
            float fZ = pVertex[2] * XFNV_M128_F32(scaling.m_quad, 2);
            hkVector4& kScaled = akFrame[2 * j];
            XFNV_M128_F32(kScaled.m_quad, 0) = fX;
            XFNV_M128_F32(kScaled.m_quad, 1) = fY;
            XFNV_M128_F32(kScaled.m_quad, 2) = fZ;
            XFNV_M128_F32(kScaled.m_quad, 3) = 0.0f;
            hkVector4& kPoint = akFrame[2 * j + 1];
            kPoint.setTransformedPos(hkTransformIdentity, kScaled);
            __m128 kP = _mm_load_ps(&reinterpret_cast<float*>(&kPoint.m_quad)[0]);
            _mm_store_ps(
                &reinterpret_cast<float*>(&out.m_min.m_quad)[0],
                _mm_min_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0]), kP)
            );
            _mm_store_ps(
                &reinterpret_cast<float*>(&out.m_max.m_quad)[0],
                _mm_max_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0]), kP)
            );
        }
    }
}

// vertex scaled by pScale (each product rounded to a float), w 0, moved by the transform and added to
// the box aabb.
void includeScaledTransformedPoint(
    const float* pScale,
    const hkTransform* pTransform,
    const float* vertex,
    hkAabb* aabb
) {
    const float x = pScale[0] * vertex[0];
    const float y = pScale[1] * vertex[1];
    const float z = pScale[2] * vertex[2];
    hkVector4 point;
    point.m_quad = _mm_setr_ps(x, y, z, 0.0f);
    hkVector4 transformed;
    transformed.setTransformedPos(*pTransform, point);
    aabb->includePoint(transformed);
}

// As above, hkString::memCpy16<64>.
void copy64Bytes(void* dst, const void* src) {
    const uint32_t* src32 = static_cast<const uint32_t*>(src);
    uint32_t* dst32 = static_cast<uint32_t*>(dst);
    int32_t numQuads = 4;
    do {
        dst32[0] = src32[0];
        dst32[1] = src32[1];
        dst32[2] = src32[2];
        dst32[3] = src32[3];
        dst32 += 4;
        src32 += 4;
    }
    while (--numQuads > 0);
}

// The AABB of a shapes subpart in the mesh's space: apAabbOut starts empty (+/- HK_REAL_MAX), then takes in each
// child shape's AABB (tolerance 0) under the subpart's rotation and translation.
void calcAabbExtents(const hkpExtendedMeshShape::ShapesSubpart* part, hkAabb* scaling) {
    scaling->m_min.setAll(HK_REAL_MAX);
    scaling->m_max.setNeg4(scaling->m_min);
    hkTransform transform;
    transform.getTranslation() = part->m_translation;
    transform.getRotation().set(part->m_rotation);
    hkAabb childAabb;
    for (hkpConvexShape* pChild : part->m_childShapes) {
        pChild->getAabb(transform, 0.0f, childAabb);
        scaling->m_min.setMin4(scaling->m_min, childAabb.m_min);
        scaling->m_max.setMax4(scaling->m_max, childAabb.m_max);
    }
}
