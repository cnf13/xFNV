#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"

#include <xmmintrin.h>

hkpTriSampledHeightFieldCollection::MembersArray hkpTriSampledHeightFieldCollection::Members{
    {"heightfield",
     &hkpSampledHeightFieldShapeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"childSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x1C, nullptr},
    {"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x24, nullptr},
    {"triangleExtrusion",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr}
};

hkClass hkpTriSampledHeightFieldCollectionClass(
    "hkpTriSampledHeightFieldCollection",
    &hkpShapeCollectionClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpTriSampledHeightFieldCollection::Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpTriSampledHeightFieldCollectionTypeInfo = {
    "hkpTriSampledHeightFieldCollection",
    finishLoadedObjecthkpTriSampledHeightFieldCollection,
    cleanupLoadedObjecthkpTriSampledHeightFieldCollection,
    getVtablehkpTriSampledHeightFieldCollection(),
    &typeid(hkpTriSampledHeightFieldCollection),
    sizeof(hkpTriSampledHeightFieldCollection)
};

hkpTriSampledHeightFieldCollection::hkpTriSampledHeightFieldCollection(
    const hkpSampledHeightFieldShape* shape,
    float radius
)
  : hkpShapeCollection(HK_SHAPE_COLLECTION, hkpShapeCollection::COLLECTION_TRISAMPLED_HEIGHTFIELD),
    m_heightfield(shape), m_radius(radius) {
    m_triangleExtrusion.m_quad = _mm_setzero_ps();
    shape->addReference();
}

hkpTriSampledHeightFieldCollection::~hkpTriSampledHeightFieldCollection() {
    m_heightfield->removeReference();
}

// The heightfield's size with the buffer left after this object; when the heightfield sits right after this object
// (copied with it) the two count as one block and m_childSize is 0.
int32_t hkpTriSampledHeightFieldCollection::calcSizeForSpu(
    const hkpShape::CalcSizeForSpuInput& input,
    int32_t spuBufferSizeLeft
) const {
    const int32_t iSpaceLeft = spuBufferSizeLeft - static_cast<int32_t>(sizeof(*this));
    const int32_t iChildSize = m_heightfield->calcSizeForSpu(input, iSpaceLeft);
    if (iChildSize < 0 || iChildSize > iSpaceLeft) {
        return -1;
    }
    if (static_cast<const void*>(m_heightfield) == static_cast<const void*>(this + 1)) {
        m_childSize = 0;
        return iChildSize + static_cast<int32_t>(sizeof(*this));
    }
    m_childSize = iChildSize;
    return sizeof(*this);
}

void hkpTriSampledHeightFieldCollection::getAabbImpl(
    const hkTransform& localToWorld,
    float tolerance,
    hkAabb& out
) const {
    hkVector4 v6;
    hkAabb v8;
    const hkpSampledHeightFieldShape* HeightFieldShape = getHeightFieldShape();
    float v7 = m_radius + tolerance;
    HeightFieldShape->getAabbImpl(localToWorld, v7, v8);
    v6.m_quad = _mm_max_ps(v8.m_max.m_quad, _mm_add_ps(m_triangleExtrusion.m_quad, v8.m_max.m_quad));
    out.m_min.m_quad = _mm_min_ps(v8.m_min.m_quad, _mm_add_ps(m_triangleExtrusion.m_quad, v8.m_min.m_quad));
    out.m_max.m_quad = v6.m_quad;
}

void hkpTriSampledHeightFieldCollection::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    if (weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t m_capacityAndFlags = m_weldingInfo.m_capacityAndFlags;
        m_weldingInfo.m_size = 0;
        if (m_capacityAndFlags >= 0) {
            m_heap->bufFree(m_weldingInfo.begin(), sizeof(uint16_t) * (m_capacityAndFlags & 0x3FFFFFFF));
        }
        m_weldingInfo.m_data = nullptr;
        m_weldingInfo.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
    }
    else {
        const hkpSampledHeightFieldShape* HeightFieldShape = getHeightFieldShape();
        int32_t v4 = (HeightFieldShape->m_xRes - 1) * (2 * HeightFieldShape->m_zRes - 2);
        hkArray<uint16_t, hkContainerHeapAllocator>* p_m_weldingInfo = &m_weldingInfo;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if ((p_m_weldingInfo->m_capacityAndFlags & 0x3FFFFFFF) < v4) {
            hkArrayUtil::_reserve(Instance->m_heap, p_m_weldingInfo, v4, sizeof(uint16_t));
        }
        hkMemoryAllocator* v7 = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t v8 = p_m_weldingInfo->m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
        if (v8 < v4) {
            int32_t v9 = 2 * v8;
            if (v4 >= v9) {
                v9 = v4;
            }
            hkArrayUtil::_reserve(v7, p_m_weldingInfo, v9, sizeof(uint16_t));
        }
        p_m_weldingInfo->m_size = v4;
    }
}

// Stores a triangle's welding word. The key is (z << 16) | (x << 1) | triangle and the welding array holds one word per
// triangle of the grid, at (z * (xRes - 1) + x) * 2 + triangle. The key is shifted and then the x half is masked with
// 0x7fff.
void hkpTriSampledHeightFieldCollection::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    const int32_t iZ = static_cast<int32_t>(key >> 16);
    const int32_t iX = static_cast<int32_t>((key >> 1) & 0x7fff);
    const int32_t iIndex = (iZ * (getHeightFieldShape()->m_xRes - 1) + iX) * 2 | static_cast<int32_t>(key & 1);
    m_weldingInfo.m_data[iIndex] = weldingInfo;
}

uint32_t hkpTriSampledHeightFieldCollection::getFirstKey() const {
    return (uint32_t)0;
}

// The key after oldKey: the cell's second triangle, else the next cell along x, wrapping to the next row of cells;
// HK_INVALID_SHAPE_KEY after the last cell.
uint32_t hkpTriSampledHeightFieldCollection::getNextKey(uint32_t oldKey) const {
    if ((oldKey & 1) == 0) {
        return oldKey | 1;
    }
    int32_t iX = static_cast<int32_t>((oldKey >> 1) & 0x7fff) + 1;
    int32_t iZ = static_cast<int32_t>(oldKey >> 16);
    const hkpSampledHeightFieldShape* pHeightField = m_heightfield;
    if (iX == pHeightField->m_xRes - 1) {
        iX = 0;
        ++iZ;
        if (iZ == pHeightField->m_zRes - 1) {
            return HK_INVALID_SHAPE_KEY;
        }
    }
    return ((static_cast<uint32_t>(iZ) << 15) + static_cast<uint32_t>(iX)) * 2;
}

uint32_t hkpTriSampledHeightFieldCollection::getCollisionFilterInfo(uint32_t a1) const {
    return (uint32_t)0;
}

// The child triangle of key, built in buffer: the triangle shape with the collection's radius; the four corners of the
// key's grid cell (x in bits 1-15, z in the high half), each scaled to shape space; three of them picked by the cell's
// diagonal flip and the key's low bit; the welding info when the collection has any; and the extrusion. The corners,
// the copies and the extrusion's squared length are computed in single precision. The buffer is null-checked but the
// triangle is still filled at null, as the game does.
const hkpShape* hkpTriSampledHeightFieldCollection::getChildShape(uint32_t key, char (&buffer)[512]) const {
    const hkpSampledHeightFieldShape* pHeightField = getHeightFieldShape();
    const int32_t iX = (key & 0xFFFF) >> 1;
    const int32_t iZ = key >> 16;
    hkpTriangleShape* pTriangle = nullptr;
    char* pBuffer = buffer;
    if (pBuffer) {
        pTriangle = ::new (pBuffer) hkpTriangleShape(m_radius, 0, hkpWeldingUtility::WELDING_TYPE_NONE);
    }

    hkVector4 vertices[4];
    for (int32_t i = 0; i < 4; ++i) {
        const int32_t iVertexX = iX + (i >> 1);
        const int32_t iVertexZ = iZ + (i & 1);
        const float fHeight = pHeightField->getHeightAtImpl(iVertexX, iVertexZ);
        const __m128 kVertex = _mm_setr_ps(static_cast<float>(iVertexX), fHeight, static_cast<float>(iVertexZ), 0.0f);
        _mm_store_ps(
            &reinterpret_cast<float*>(&vertices[i].m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&pHeightField->m_intToFloatScale.m_quad)[0]), kVertex
            )
        );
    }

    hkBool kFlip;
    pHeightField->getTriangleFlipImpl(&kFlip);
    const int32_t iFlip = kFlip.m_bool != 0;
    const int32_t iOdd = key & 1;
    _mm_store_ps(
        &reinterpret_cast<float*>(&pTriangle->m_vertexA.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&vertices[(~iFlip & iOdd) * 3].m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&pTriangle->m_vertexB.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&vertices[(iFlip + 1) * iOdd + 1].m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&pTriangle->m_vertexC.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&vertices[iFlip - iOdd + 2].m_quad)[0])
    );

    const int32_t iWeldingInfoCount = m_weldingInfo.m_size;
    uint16_t usWeldingInfo = 0;
    if (iWeldingInfoCount != 0) {
        // EXE-INLINED none: hkpTriSampledHeightFieldCollection::getWeldingInfo
        // EXE-INLINED none: hkpTriSampledHeightFieldCollection::getIndexFromShapeKey
        const int32_t iIndex = ((iZ * (getHeightFieldShape()->m_xRes - 1) + iX) << 1) | iOdd;
        usWeldingInfo = m_weldingInfo.m_data[iIndex];
    }
    pTriangle->m_weldingInfo = usWeldingInfo;
    pTriangle->m_weldingType.m_storage =
        iWeldingInfoCount == 0 ? hkpWeldingUtility::WELDING_TYPE_NONE : hkpWeldingUtility::WELDING_TYPE_ANTICLOCKWISE;

    // EXE-INLINED none: hkpTriangleShape::setExtrusion (hkVector4::lengthSquared3)
    const __m128 kExtrusion = _mm_load_ps(&reinterpret_cast<const float*>(&m_triangleExtrusion.m_quad)[0]);
    const __m128 kSquares = _mm_mul_ps(kExtrusion, kExtrusion);
    const __m128 kSumXY = _mm_add_ps(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares);
    const __m128 kSumXYZ = _mm_add_ps(_mm_shuffle_ps(kSquares, kSquares, 0xAA), kSumXY);
    pTriangle->m_isExtruded = 0.0f < _mm_cvtss_f32(kSumXYZ);
    _mm_store_ps(
        &reinterpret_cast<float*>(&pTriangle->m_extrusion.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&m_triangleExtrusion.m_quad)[0])
    );
    return pTriangle;
}

// KEEP: the exe's getChildShape (0xD26510) reads ecx and edx after its second call of this function (the welding type
// in dl, `mov byte ptr [edi+0x16], dl` at 0xD266B1, and `mov esi, [ecx+0x24]` at 0xD266A4), which the exe body
// leaves intact; clang does not guarantee it.
const hkpSampledHeightFieldShape* hkpTriSampledHeightFieldCollection::getHeightFieldShape() const {
    return m_heightfield;
}

const hkClass& hkpTriSampledHeightFieldCollection::staticClass() {
    return hkpTriSampledHeightFieldCollectionClass;
}
