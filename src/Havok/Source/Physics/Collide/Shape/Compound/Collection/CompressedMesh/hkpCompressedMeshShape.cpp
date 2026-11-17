#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/CompressedMesh/hkpCompressedMeshShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <bit>
#include <xmmintrin.h>

// The class records of hkpCompressedMeshShape and its parts, as the Havok SDK's generated file has them.

namespace {
    // Each member's offset in this struct (-1: no default), then the default of weldingType.
    struct hkpCompressedMeshShape_DefaultStruct {
        int32_t s_defaultOffsets[21];
        hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    };
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpCompressedMeshShapeChunkTypeInfo = {
    "hkpCompressedMeshShapeChunk",
    finishLoadedObjecthkpCompressedMeshShapeChunk,
    cleanupLoadedObjecthkpCompressedMeshShapeChunk,
    nullptr,
    &typeid(hkpCompressedMeshShape::Chunk),
    sizeof(hkpCompressedMeshShape::Chunk)
};
hkTypeInfo hkpCompressedMeshShapeBigTriangleTypeInfo = {
    "hkpCompressedMeshShapeBigTriangle",
    nullptr,
    cleanupLoadedObjecthkpCompressedMeshShapeBigTriangle,
    nullptr,
    &typeid(hkpCompressedMeshShape::BigTriangle),
    sizeof(hkpCompressedMeshShape::BigTriangle)
};
hkTypeInfo hkpCompressedMeshShapeConvexPieceTypeInfo = {
    "hkpCompressedMeshShapeConvexPiece",
    finishLoadedObjecthkpCompressedMeshShapeConvexPiece,
    cleanupLoadedObjecthkpCompressedMeshShapeConvexPiece,
    nullptr,
    &typeid(hkpCompressedMeshShape::ConvexPiece),
    sizeof(hkpCompressedMeshShape::ConvexPiece)
};

// The parts' member tables.
using hkpCompressedMeshShape_ChunkClass_MembersArray = const hkClassMember[8];
static hkpCompressedMeshShape_ChunkClass_MembersArray hkpCompressedMeshShape_ChunkClass_Members{
    {"offset", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x10, nullptr},
    {"indices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x1C, nullptr},
    {"stripLengths", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x28, nullptr},
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x34, nullptr},
    {"materialInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x40, nullptr},
    {"reference", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x44, nullptr},
    {"transformIndex", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x46, nullptr}
};
using hkpCompressedMeshShape_BigTriangleClass_MembersArray = const hkClassMember[5];
static hkpCompressedMeshShape_BigTriangleClass_MembersArray hkpCompressedMeshShape_BigTriangleClass_Members{
    {"a", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"b", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x2, nullptr},
    {"c", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
    {"material", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr}
};
using hkpCompressedMeshShape_ConvexPieceClass_MembersArray = const hkClassMember[6];
static hkpCompressedMeshShape_ConvexPieceClass_MembersArray hkpCompressedMeshShape_ConvexPieceClass_Members{
    {"offset", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x10, nullptr},
    {"faceVertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x1C, nullptr},
    {"faceOffsets", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x28, nullptr},
    {"reference", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x34, nullptr},
    {"transformIndex", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x36, nullptr}
};

using hkpCompressedMeshShapeMaterialTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpCompressedMeshShapeMaterialTypeEnumItemsArray hkpCompressedMeshShapeMaterialTypeEnumItems{
    {hkpCompressedMeshShape::MATERIAL_NONE, "MATERIAL_NONE"},
    {hkpCompressedMeshShape::MATERIAL_SINGLE_VALUE_PER_CHUNK, "MATERIAL_SINGLE_VALUE_PER_CHUNK"},
    {hkpCompressedMeshShape::MATERIAL_ONE_BYTE_PER_TRIANGLE, "MATERIAL_ONE_BYTE_PER_TRIANGLE"},
    {hkpCompressedMeshShape::MATERIAL_TWO_BYTES_PER_TRIANGLE, "MATERIAL_TWO_BYTES_PER_TRIANGLE"},
    {hkpCompressedMeshShape::MATERIAL_FOUR_BYTES_PER_TRIANGLE, "MATERIAL_FOUR_BYTES_PER_TRIANGLE"}
};
using hkpCompressedMeshShapeEnumsArray = const hkClassEnum[1];
static hkpCompressedMeshShapeEnumsArray hkpCompressedMeshShapeEnums{
    {"MaterialType", hkpCompressedMeshShapeMaterialTypeEnumItems, 5, nullptr, 0}
};
static const hkClassEnum* hkpCompressedMeshShapeMaterialTypeEnum = &hkpCompressedMeshShapeEnums[0];

// The shape record's member table. Its two enum members' enums are null in the static data and are stored at start-up
// by the initialiser below.
using hkpCompressedMeshShapeClass_MembersArray = hkClassMember[21];
static hkpCompressedMeshShapeClass_MembersArray hkpCompressedMeshShapeClass_Members{
    {"bitsPerIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x18, nullptr},
    {"bitsPerWIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1C, nullptr},
    {"wIndexMask", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
    {"indexMask", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x24, nullptr},
    {"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x28, nullptr},
    {"weldingType",
     nullptr,
     &hkpWeldingUtilityEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x2C,
     nullptr},
    {"materialType",
     nullptr,
     &hkpCompressedMeshShapeEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x2D,
     nullptr},
    {"materials", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, 0, 0x30, nullptr},
    {"materials16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x3C, nullptr},
    {"materials8", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x48, nullptr},
    {"transforms", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_TRANSFORM}, 0, 0, 0x54, nullptr},
    {"bigVertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, 0, 0x60, nullptr},
    {"bigTriangles",
     &hkpCompressedMeshShapeBigTriangleClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x6C,
     nullptr},
    {"chunks",
     &hkpCompressedMeshShapeChunkClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x78,
     nullptr},
    {"convexPieces",
     &hkpCompressedMeshShapeConvexPieceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x84,
     nullptr},
    {"error", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x90, nullptr},
    {"bounds", &hkAabbClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA0, nullptr},
    {"meshMaterials",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC0,
     nullptr},
    {"materialStriding",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0xC4,
     nullptr},
    {"numMaterials", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC6, nullptr},
    {"namedMaterials",
     &hkpNamedMeshMaterialClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0xC8,
     nullptr}
};

// The defaults the class record names.
static const hkpCompressedMeshShape_DefaultStruct hkpCompressedMeshShape_Default{
    {-1, -1, -1, -1, -1, 0x54, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {hkpWeldingUtility::WELDING_TYPE_NONE}
};
hkClass hkpCompressedMeshShapeChunkClass(
    "hkpCompressedMeshShapeChunk",
    nullptr,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpCompressedMeshShape_ChunkClass_Members,
    8,
    nullptr,
    nullptr,
    0,
    4
);
hkClass hkpCompressedMeshShapeBigTriangleClass(
    "hkpCompressedMeshShapeBigTriangle",
    nullptr,
    16,
    nullptr,
    0,
    nullptr,
    0,
    hkpCompressedMeshShape_BigTriangleClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    1
);
hkClass hkpCompressedMeshShapeConvexPieceClass(
    "hkpCompressedMeshShapeConvexPiece",
    nullptr,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpCompressedMeshShape_ConvexPieceClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    3
);
hkClass hkpCompressedMeshShapeClass(
    "hkpCompressedMeshShape",
    &hkpShapeCollectionClass,
    240,
    nullptr,
    0,
    hkpCompressedMeshShapeEnums,
    1,
    hkpCompressedMeshShapeClass_Members,
    21,
    &hkpCompressedMeshShape_Default,
    nullptr,
    0,
    8
);
hkTypeInfo hkpCompressedMeshShapeTypeInfo = {
    "hkpCompressedMeshShape",       finishLoadedObjecthkpCompressedMeshShape, cleanupLoadedObjecthkpCompressedMeshShape,
    getCompressedMeshShapeVtable(), &typeid(hkpCompressedMeshShape),          sizeof(hkpCompressedMeshShape)
};

// The cleanup function of hkpCompressedMeshShape's type info, paired with finishLoadedObjecthkpCompressedMeshShape:
// runs the shape's virtual destructor in place.
void cleanupLoadedObjecthkpCompressedMeshShape(void* p) {
    static_cast<hkpCompressedMeshShape*>(p)->~hkpCompressedMeshShape();
}

__m128* calcVerticesAabb(hkArrayBase<uint16_t>* a1, hkVector4* a2, float a3, __m128* a4, const hkVector4* a5) {
    uint32_t v11;
    uint32_t v12;
    uint32_t v13;
    uint16_t v5 = 0;
    int32_t v6 = a1->m_size / 3;
    uint16_t v7 = 0;
    uint16_t v10 = 0;
    if (v6 > 0) {
        uint16_t* m_data = a1->begin();
        do {
            if (*m_data > v10) {
                v10 = *m_data;
            }
            if (m_data[1] > v5) {
                v5 = m_data[1];
            }
            if (m_data[2] > v7) {
                v7 = m_data[2];
            }
            m_data += 3;
            --v6;
        }
        while (v6);
    }
    *reinterpret_cast<float*>(&v12) = static_cast<float>(v10);
    *reinterpret_cast<float*>(&v13) = static_cast<float>(v5);
    *reinterpret_cast<float*>(&v11) = static_cast<float>(v7);
    a4[1] = _mm_add_ps(
        _mm_mul_ps(
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v12)), _mm_set_ss(std::bit_cast<float>(v11))),
                _mm_unpacklo_ps(
                    _mm_set_ss(std::bit_cast<float>(v13)),
                    _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
                )
            ),
            _mm_shuffle_ps(_mm_set_ss(a3), _mm_set_ss(a3), 0)
        ),
        a2->m_quad
    );
    *a4 = a2->m_quad;
    a4[1] = _mm_mul_ps(a4[1], a5->m_quad);
    *a4 = _mm_mul_ps(*a4, a5->m_quad);
    return a4;
}

const hkClass& hkpCompressedMeshShape::BigTriangle::staticClass() {
    return hkpCompressedMeshShapeBigTriangleClass;
}

hkpCompressedMeshShape::Chunk::~Chunk() {}

void hkpCompressedMeshShape::Chunk::getTriangle(
    int32_t a1,
    float afScale,
    hkVector4* arVertex0,
    hkVector4* arVertex1,
    hkVector4* arVertex2
) {
    hkpCompressedMeshShape::getChunkVertex(
        this, 3 * m_indices.m_data[a1], 3 * m_indices.m_data[a1 + 1], 3 * m_indices.m_data[a1 + 2], afScale, *arVertex0,
        *arVertex1, *arVertex2
    );
}

int32_t hkpCompressedMeshShape::Chunk::getNextIndex(int32_t index, int32_t& winding) {
    hkpCompressedMeshShape::Chunk* pThis = this;
    uint16_t* m_data;
    hkpCompressedMeshShape::Chunk* v11 = pThis;
    winding = 0;
    if (index < 0) {
        return (pThis->m_indices.m_size != 0) - 1;
    }
    int32_t m_size = pThis->m_stripLengths.m_size;
    int32_t v5 = index + 1;
    int32_t v6 = 0;
    int32_t v7 = 0;
    char v8 = 0;
    if (m_size && (m_data = pThis->m_stripLengths.begin(), (v7 = *m_data, v5 >= v7))) {
        while (true) {
            ++v6;
            ++m_data;
            if (v6 >= m_size) {
                break;
            }
            v8 = v7;
            v7 += *m_data;
            pThis = v11;
            if (v5 < v7) {
                goto LABEL_7;
            }
        }
    }
    else {
    LABEL_7:;
        if (v6 < m_size) {
            if (v5 < v7 - 2) {
                winding = static_cast<uint8_t>(v5) - v8 & 1;
            }
            else {
                v5 = v7;
            }
            goto LABEL_14;
        }
    }
    for (int32_t i = v5 - v7; i % 3; ++v5) {
        ++i;
    }
LABEL_14:;
    if (v5 < pThis->m_indices.m_size) {
        return v5;
    }

    return -1;
}

void hkpCompressedMeshShape::Chunk::calcAabb(float error, hkAabb& out, const hkVector4& scaling) {
    calcVerticesAabb(&m_vertices, &m_offset, error, &out.m_min.m_quad, &scaling);
}

// The triangles of the strips (a strip of n indices holds n - 2), counted two strips per step as the game does, plus
// the loose triangles after the strips (three indices each).
int32_t hkpCompressedMeshShape::Chunk::getNumTriangles() {
    int32_t iNumStrips = m_stripLengths.m_size;
    int32_t iStrip = 0;
    int32_t iEvenLengths = 0;
    int32_t iTriangles = 0;
    int32_t iOddLengths = 0;
    int32_t iLastLength = 0;
    if (iNumStrips >= 2) {
        uint16_t* pLengths = m_stripLengths.begin();
        uint32_t uiPairs = (static_cast<uint32_t>(iNumStrips - 2) >> 1) + 1;
        int32_t iPairStrips = 2 * uiPairs;
        do {
            int32_t iOdd = pLengths[1];
            int32_t iEven = *pLengths;
            iOddLengths += iOdd;
            iEvenLengths += iEven;
            pLengths += 2;
            --uiPairs;
            iTriangles = iTriangles + (iEven + iOdd) - 4;
        }
        while (uiPairs);
        iStrip = iPairStrips;
    }
    if (iStrip < m_stripLengths.m_size) {
        iLastLength = m_stripLengths.m_data[iStrip];
        iTriangles = iTriangles + iLastLength - 2;
    }
    return iTriangles + (m_indices.m_size - iOddLengths - iEvenLengths - iLastLength) / 3;
}

const hkClass& hkpCompressedMeshShape::Chunk::staticClass() {
    return hkpCompressedMeshShapeChunkClass;
}

hkpCompressedMeshShape::ConvexPiece::~ConvexPiece() {}

// The piece's vertices, dequantized (quantization times each component plus m_offset, w 0) and moved by transform,
// into vertices sized to one per three components.
void hkpCompressedMeshShape::ConvexPiece::getVertices(
    float quantization,
    const hkTransform& transform,
    hkArray<hkVector4, hkContainerHeapAllocator>& vertices
) {
    int32_t iNumVertices = m_vertices.m_size / 3;
    // EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::setSize
    hkMemoryAllocator* pHeap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t iCapacity = vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (iCapacity < iNumVertices) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumVertices >= iNewCapacity) {
            iNewCapacity = iNumVertices;
        }
        hkArrayUtil::_reserve(*pHeap, &vertices, iNewCapacity, sizeof(hkVector4));
    }
    vertices.m_size = iNumVertices;
    for (int32_t i = 0; i < m_vertices.m_size; i += 3) {
        uint16_t* pQuantized = m_vertices.begin();
        float fX = static_cast<float>(pQuantized[i]);
        float fY = static_cast<float>(pQuantized[i + 1]);
        float fZ = static_cast<float>(pQuantized[i + 2]);
        hkVector4 kVertex;
        kVertex.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_unpacklo_ps(
                    _mm_unpacklo_ps(_mm_set_ss(fX), _mm_set_ss(fZ)), _mm_unpacklo_ps(_mm_set_ss(fY), _mm_set_ss(0.0f))
                ),
                _mm_shuffle_ps(_mm_set_ss(quantization), _mm_set_ss(quantization), 0)
            ),
            m_offset.m_quad
        );
        kVertex.setTransformedPos(transform, kVertex);
        vertices.m_data[i / 3] = kVertex;
    }
}

const hkClass& hkpCompressedMeshShape::ConvexPiece::staticClass() {
    return hkpCompressedMeshShapeConvexPieceClass;
}

hkpCompressedMeshShape::hkpCompressedMeshShape(float radius)
  : hkpShapeCollection(HK_SHAPE_COMPRESSED_MESH, hkpShapeCollection::COLLECTION_COMPRESSED_MESH), m_bitsPerIndex(17),
    m_bitsPerWIndex(18), m_wIndexMask(0x3FFFF), m_indexMask(0x1FFFF), m_radius(radius), m_meshMaterials(nullptr),
    m_materialStriding(0), m_numMaterials(0) {
    m_weldingType.m_storage = hkpWeldingUtility::WELDING_TYPE_NONE;
    m_scaling.setAll(1.0f);
}

hkpCompressedMeshShape::hkpCompressedMeshShape(hkFinishLoadedObjectFlag flag)
  : hkpShapeCollection(flag), m_materials(flag), m_materials16(flag), m_materials8(flag), m_transforms(flag),
    m_bigVertices(flag), m_bigTriangles(flag), m_chunks(flag), m_convexPieces(flag), m_namedMaterials(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = HK_SHAPE_COMPRESSED_MESH;
        m_collectionType.m_storage = hkpShapeCollection::COLLECTION_COMPRESSED_MESH;
        if (m_namedMaterials.m_size != 0) {
            m_meshMaterials = m_namedMaterials.m_data;
        }
    }
}

// The arrays free themselves.
hkpCompressedMeshShape::~hkpCompressedMeshShape() {}

void hkpCompressedMeshShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkpShapeCollection::calcContentStatistics(collector, &hkpShapeCollectionClass);
}

int32_t hkpCompressedMeshShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
    return 240;
}

void hkpCompressedMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 v4;
    hkVector4 center;
    hkVector4 halfExtents;
    v4.m_quad = m_bounds.m_min.m_quad;
    center.m_quad = _mm_mul_ps(_mm_add_ps(m_bounds.m_max.m_quad, v4.m_quad), hkQuadRealHalf);
    halfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(m_bounds.m_max.m_quad, v4.m_quad), hkQuadRealHalf);
    hkAabbUtil::calcAabb(localToWorld, halfExtents, center, tolerance, out);
}

void hkpCompressedMeshShape::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    m_weldingType.m_storage = (uint8_t)((int8_t)weldingType);
}

void hkpCompressedMeshShape::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    uint32_t v3 = key >> m_bitsPerWIndex;
    if (v3) {
        hkpCompressedMeshShape::Chunk* m_data =
            static_cast<hkArrayBase<hkpCompressedMeshShape::Chunk>&>(m_chunks).begin();
        uint32_t v5 = key & m_indexMask;
        uint32_t v6 = v3 - 1;
        uint16_t m_reference = m_data[v6].m_reference;
        // Compares the 16-bit reference with 0xFFFF, the null reference.
        if (m_reference != 0xFFFF) {
            v6 = m_reference;
        }
        m_data[v6].m_weldingInfo.m_data[v5] = weldingInfo;
    }
    else {
        m_bigTriangles.m_data[key & m_wIndexMask].m_weldingInfo = weldingInfo;
    }
}

uint32_t hkpCompressedMeshShape::getFirstKey() const {
    if (m_bigTriangles.m_size) {
        return 0;
    }
    if (m_chunks.m_size) {
        return 1 << m_bitsPerWIndex;
    }
    if (m_convexPieces.m_size) {
        return ((1 << (32 - m_bitsPerWIndex)) - 1) << m_bitsPerWIndex;
    }
    return -1;
}

// Keys: the chunk or convex piece number above m_bitsPerWIndex bits (0 the big triangles, the all-ones value the
// convex pieces), the winding bit at m_bitsPerIndex, the index below it. A chunk whose m_reference is not 0xFFFF
// takes its triangles from the chunk it names.
uint32_t hkpCompressedMeshShape::getNextKey(uint32_t oldKey) const {
    const int32_t bitsPerWIndex = m_bitsPerWIndex;
    int32_t chunkId = oldKey >> bitsPerWIndex;
    int32_t index = oldKey & m_indexMask;
    if (chunkId == 0) {
        index++;
        if (index < m_bigTriangles.m_size) {
            return index;
        }
        chunkId = 1;
        index = -1;
    }
    const int32_t convexId = (1 << (32 - bitsPerWIndex)) - 1;
    if (chunkId != convexId) {
        const int32_t numChunks = m_chunks.m_size;
        for (int32_t i = chunkId - 1; i < numChunks; i++) {
            Chunk& rChunk = m_chunks.m_data[i];
            const uint16_t reference = rChunk.m_reference;
            int32_t winding = 0;
            const int32_t next = reference == 0xFFFF ? rChunk.getNextIndex(index, winding)
                                                     : m_chunks.m_data[reference].getNextIndex(index, winding);
            if (next != -1 && next <= m_indexMask) {
                return (winding & 1) << m_bitsPerIndex | (i + 1) << bitsPerWIndex | (m_wIndexMask & next);
            }
            index = -1;
        }
    }
    index++;
    if (index >= m_convexPieces.m_size) {
        return -1;
    }
    return convexId << bitsPerWIndex | index;
}

uint32_t hkpCompressedMeshShape::getCollisionFilterInfo(uint32_t a1) const {
    return (uint32_t)0;
}

// The key's part above m_bitsPerWIndex chooses the kind of child. All ones is a convex piece: an hkpConvexVerticesShape
// over the vertices of the piece (or of its m_reference piece when it has one), dequantized four at a time -- m_error
// times each quantized component plus the piece's m_offset -- and transformed by the piece's m_transformIndex
// transform, transposed into FourVectors in a static buffer. The bounds are those of the untransformed vertices, halved
// into m_aabbHalfExtents and m_aabbCenter. Any other value is a triangle in an hkpTriangleShape: with a zero part above
// m_bitsPerWIndex a big triangle, three of m_bigVertices scaled by m_scaling; otherwise the chunk at that part minus
// one (or its m_reference chunk), the three consecutive strip indices at the key's index, their vertices dequantized by
// getChunkVertex and transformed, the key's part above m_bitsPerIndex choosing whether the first goes to vertex A or
// vertex C. Both shapes are placement new on the buffer, which gives a null shape for a null buffer, and the code then
// writes through it, as the game does.
const hkpShape* hkpCompressedMeshShape::getChildShape(uint32_t aKey, char (&arBuffer)[512]) const {
    // Shift counts are taken modulo 32, as the game does.
    const uint32_t uiWIndex = aKey >> (m_bitsPerWIndex & 31);
    const uint32_t uiKeyIndex = aKey & static_cast<uint32_t>(m_wIndexMask);
    const int32_t iSubIndex = static_cast<int32_t>(uiKeyIndex) >> (m_bitsPerIndex & 31);
    char* pBuffer = arBuffer;
    if (uiWIndex == (1u << ((32 - m_bitsPerWIndex) & 31)) - 1) {
        hkTransform kTransform = hkTransformIdentity;
        hkpConvexVerticesShape* pShape = nullptr;
        if (pBuffer) {
            pShape = ::new (pBuffer) hkpConvexVerticesShape(m_radius);
        }
        const ConvexPiece* pPiece = m_convexPieces.m_data + uiKeyIndex;
        if (pPiece->m_transformIndex != 0xFFFF) {
            kTransform = m_transforms.m_data[pPiece->m_transformIndex];
        }
        uint32_t uiPieceIndex = uiKeyIndex;
        if (pPiece->m_reference != 0xFFFF) {
            uiPieceIndex = pPiece->m_reference;
        }
        pPiece = m_convexPieces.m_data + uiPieceIndex;
        const uint16_t* pVertices = pPiece->m_vertices.m_data;
        const int32_t iNumVertices = pPiece->m_vertices.m_size / 3;
        const int32_t iNumBlocks = (iNumVertices + 3) >> 2;
        // The initial bounds are 3.40282e38 and its negation, in every lane.
        __m128 kMin = _mm_set1_ps(3.40282e38f);
        __m128 kMax = _mm_set1_ps(-3.40282e38f);
        const __m128 kQuantization = _mm_set1_ps(m_error);
        int32_t iVertex = 0;
        hkpConvexVerticesShape::FourVectors* pOut = &s_convexVertices[0];
        for (int32_t iBlock = 0; iBlock < iNumBlocks; ++iBlock) {
            hkVector4 akVertices[4];
            for (int32_t i = 0; i < 4; ++i) {
                const float fX = static_cast<float>(pVertices[0]);
                const float fY = static_cast<float>(pVertices[1]);
                const float fZ = static_cast<float>(pVertices[2]);
                // (x, y, z, 0) times m_error plus the offset, in all four lanes.
                hkVector4 kQuantized;
                kQuantized.m_quad = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_unpacklo_ps(
                            _mm_unpacklo_ps(_mm_set_ss(fX), _mm_set_ss(fZ)),
                            _mm_unpacklo_ps(_mm_set_ss(fY), _mm_set_ss(0.0f))
                        ),
                        kQuantization
                    ),
                    pPiece->m_offset.m_quad
                );
                akVertices[i].setTransformedPos(kTransform, kQuantized);
                kMin = _mm_min_ps(kMin, kQuantized.m_quad);
                kMax = _mm_max_ps(kMax, kQuantized.m_quad);
                if (iVertex < iNumVertices - 1) {
                    ++iVertex;
                    pVertices += 3;
                }
            }
            // The block's four vertices are transposed into the buffer's FourVectors with shuffles (0x44/0xEE, then
            // 0x88/0xDD): lanes are moved as bits, never through float loads.
            const __m128 kXY01 = _mm_shuffle_ps(akVertices[0].m_quad, akVertices[1].m_quad, 0x44);
            const __m128 kZW01 = _mm_shuffle_ps(akVertices[0].m_quad, akVertices[1].m_quad, 0xEE);
            const __m128 kXY23 = _mm_shuffle_ps(akVertices[2].m_quad, akVertices[3].m_quad, 0x44);
            const __m128 kZW23 = _mm_shuffle_ps(akVertices[2].m_quad, akVertices[3].m_quad, 0xEE);
            pOut->m_x.m_quad = _mm_shuffle_ps(kXY01, kXY23, 0x88);
            pOut->m_y.m_quad = _mm_shuffle_ps(kXY01, kXY23, 0xDD);
            pOut->m_z.m_quad = _mm_shuffle_ps(kZW01, kZW23, 0x88);
            ++pOut;
        }
        pShape->m_rotatedVertices.m_data = &s_convexVertices[0];
        pShape->m_rotatedVertices.m_size = static_cast<uint32_t>(iNumBlocks);
        pShape->m_rotatedVertices.m_capacityAndFlags = 8;
        pShape->m_numVertices = iNumVertices;
        pShape->m_convexPiece = pPiece;
        pShape->m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(kMax, kMin), _mm_set1_ps(0.5f));
        pShape->m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(kMax, kMin), _mm_set1_ps(0.5f));
        return pShape;
    }

    hkpTriangleShape* pTriangle = nullptr;
    if (pBuffer) {
        // The caller's buffer is raw memory: the SDK's placement new builds the triangle in it.
        pTriangle = ::new (pBuffer) hkpTriangleShape(0.05f, 0, hkpWeldingUtility::WELDING_TYPE_NONE);
    }
    pTriangle->m_radius = m_radius;
    pTriangle->m_weldingType.m_storage = m_weldingType.m_storage;
    if (uiWIndex == 0) {
        const BigTriangle* pBig = m_bigTriangles.m_data + uiKeyIndex;
        pTriangle->m_weldingInfo = pBig->m_weldingInfo;
        pTriangle->m_vertexA = m_bigVertices.m_data[pBig->m_a];
        pTriangle->m_vertexB = m_bigVertices.m_data[pBig->m_b];
        pTriangle->m_vertexC = m_bigVertices.m_data[pBig->m_c];
        const hkVector4 kScaling = m_scaling;
        pTriangle->m_vertexA.m_quad = _mm_mul_ps(kScaling.m_quad, pTriangle->m_vertexA.m_quad);
        pTriangle->m_vertexB.m_quad = _mm_mul_ps(pTriangle->m_vertexB.m_quad, kScaling.m_quad);
        pTriangle->m_vertexC.m_quad = _mm_mul_ps(pTriangle->m_vertexC.m_quad, kScaling.m_quad);
        return pTriangle;
    }

    // EXE-INLINED 0x577070
    hkTransform kTransform;
    kTransform.m_rotation.m_col0.m_quad = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
    kTransform.m_rotation.m_col1.m_quad = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
    kTransform.m_rotation.m_col2.m_quad = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    kTransform.m_translation.m_quad = _mm_setzero_ps();
    Chunk* pChunk = m_chunks.m_data + (uiWIndex - 1);
    if (pChunk->m_transformIndex != 0xFFFF) {
        kTransform = m_transforms.m_data[pChunk->m_transformIndex];
    }
    uint32_t uiChunkIndex = uiWIndex - 1;
    if (pChunk->m_reference != 0xFFFF) {
        uiChunkIndex = pChunk->m_reference;
    }
    pChunk = m_chunks.m_data + uiChunkIndex;
    const uint32_t uiIndex = uiKeyIndex & static_cast<uint32_t>(m_indexMask);
    pTriangle->m_weldingInfo = pChunk->m_weldingInfo.m_data[uiIndex];
    const uint16_t* pIndices = pChunk->m_indices.m_data + uiIndex;
    hkVector4 kVertex0;
    hkVector4 kVertex1;
    hkVector4 kVertex2;
    getChunkVertex(pChunk, 3 * pIndices[0], 3 * pIndices[1], 3 * pIndices[2], m_error, kVertex0, kVertex1, kVertex2);
    kVertex0.setTransformedPos(kTransform, kVertex0);
    kVertex1.setTransformedPos(kTransform, kVertex1);
    kVertex2.setTransformedPos(kTransform, kVertex2);
    const hkVector4 kScaling = m_scaling;
    // The first vertex goes to vertex A (sub-index 0) or C (1), the last one to the other.
    hkVector4* pVertices = &pTriangle->m_vertexA;
    pVertices[2 * iSubIndex].m_quad = _mm_mul_ps(kVertex0.m_quad, kScaling.m_quad);
    pTriangle->m_vertexB.m_quad = _mm_mul_ps(kVertex1.m_quad, kScaling.m_quad);
    pVertices[2 * (iSubIndex ^ 1)].m_quad = _mm_mul_ps(kVertex2.m_quad, kScaling.m_quad);
    return pTriangle;
}

void hkpCompressedMeshShape::setScaling(const hkVector4& scaling) {
    m_scaling = scaling;
}

int32_t hkpCompressedMeshShape::isValidShapeKey(uint32_t a1) {
    uint32_t v2 = a1 >> m_bitsPerWIndex;
    int32_t v3 = a1 & m_wIndexMask;
    int32_t v4 = m_indexMask & v3;
    int32_t v5 = v3 >> m_bitsPerIndex;
    if (v2) {
        int32_t v7 = v2 - 1;
        if (v7 >= m_chunks.m_size) {
            return 2;
        }
        if (m_chunks.m_data[v7].getNextIndex(v4 - 1, *reinterpret_cast<int32_t*>(&a1)) != v4) {
            return 3;
        }
        if (a1 != v5) {
            return 4;
        }
    }
    else {
        if (v5) {
            return 1;
        }
    }
    return 0;
}

// The material of the triangle with this key: a big triangle's own material, else the chunk's (or the referenced
// chunk's) single material or its per-triangle entry in the array m_materialType names, as an index of
// m_materialStriding-byte steps into m_meshMaterials. The last chunk index of the key's range has none.
const hkpMeshMaterial* hkpCompressedMeshShape::getMaterial(uint32_t key) {
    const hkpMeshMaterial* pMaterials = m_meshMaterials;
    if (!pMaterials) {
        return nullptr;
    }
    int32_t iBitsPerWIndex = m_bitsPerWIndex;
    uint32_t uiChunk = key >> iBitsPerWIndex;
    uint32_t uiIndex = key & m_indexMask;
    uint32_t uiMaterial;
    if (!uiChunk) {
        uiMaterial = m_bigTriangles.m_data[uiIndex].m_material;
    }
    else {
        if (uiChunk == static_cast<uint32_t>((1 << (32 - iBitsPerWIndex)) - 1)) {
            return nullptr;
        }
        hkpCompressedMeshShape::Chunk* pChunks = m_chunks.begin();
        uint16_t usReference = pChunks[uiChunk - 1].m_reference;
        uint32_t uiMaterialChunk = usReference == 0xFFFF ? uiChunk - 1 : usReference;
        uint32_t uiMaterialInfo = pChunks[uiMaterialChunk].m_materialInfo;
        switch (m_materialType.m_storage) {
            case MATERIAL_SINGLE_VALUE_PER_CHUNK: uiMaterial = uiMaterialInfo; break;
            case MATERIAL_ONE_BYTE_PER_TRIANGLE: uiMaterial = m_materials8.m_data[uiMaterialInfo + uiIndex]; break;
            case MATERIAL_TWO_BYTES_PER_TRIANGLE: uiMaterial = m_materials16.m_data[uiIndex + uiMaterialInfo]; break;
            case MATERIAL_FOUR_BYTES_PER_TRIANGLE: uiMaterial = m_materials.m_data[uiIndex + uiMaterialInfo]; break;
            default: return nullptr;
        }
    }
    // m_meshMaterials is untyped memory of m_materialStriding-byte records (a game material class may be larger than
    // hkpMeshMaterial), so the byte offset is added to the base.
    // EXE-INLINED none: hkAddByteOffsetConst
    return reinterpret_cast<const hkpMeshMaterial*>(
        reinterpret_cast<const char*>(pMaterials) + uiMaterial * m_materialStriding
    );
}

// The AABB of apChunk in shape space: the quantized bounds of apChunk (of the chunk it references, when it has one),
// then, for a chunk with a transform, both corners moved by that transform with its translation scaled by m_scaling.
void hkpCompressedMeshShape::getChunkAabb(Chunk* apChunk, hkAabb& arOut) {
    Chunk* pChunk = apChunk;
    uint16_t usReference = apChunk->m_reference;
    if (usReference != 0xFFFF) {
        pChunk = &m_chunks.m_data[usReference];
    }
    pChunk->calcAabb(m_error, arOut, m_scaling);
    uint16_t usTransform = apChunk->m_transformIndex;
    if (usTransform != 0xFFFF) {
        hkTransform kTransform = m_transforms.m_data[usTransform];
        kTransform.m_translation.m_quad = _mm_mul_ps(kTransform.m_translation.m_quad, m_scaling.m_quad);
        arOut.m_max.setTransformedPos(kTransform, arOut.m_max);
        arOut.m_min.setTransformedPos(kTransform, arOut.m_min);
    }
}

void hkpCompressedMeshShape::recalcAabb(hkAabb& out) {
    out.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    int32_t v2 = 0;
    out.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
    m_chunks.m_size;
    for (int32_t i = 0; i < m_chunks.m_size; ++i) {
        hkVector4 v4;
        hkAabb outa;
        m_chunks.m_data[v2].calcAabb(m_error, outa, m_scaling);
        v4.m_quad = outa.m_max.m_quad;
        out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, outa.m_min.m_quad);
        out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, v4.m_quad);
        ++v2;
    }
}

// Dequantizes the three vertices of apChunk at the given indices of its m_vertices (each already multiplied by 3) into
// the three vectors: m_error times each component plus the chunk's m_offset, the w lane the offset's w.
void hkpCompressedMeshShape::getChunkVertex(
    hkpCompressedMeshShape::Chunk* apChunk,
    int32_t aiIndex0,
    int32_t aiIndex1,
    int32_t aiIndex2,
    float afScale,
    hkVector4& arVertex0,
    hkVector4& arVertex1,
    hkVector4& arVertex2
) {
    __m128 v8 = _mm_undefined_ps();
    __m128 v9 = _mm_undefined_ps();
    hkVector4 v12;
    uint32_t v15;
    uint32_t v16;
    uint32_t v17;
    uint32_t v18;
    uint32_t v19;
    uint32_t v20;
    uint32_t v21;
    uint32_t v22;
    uint32_t v23;
    uint16_t* m_data = apChunk->m_vertices.begin();
    *reinterpret_cast<float*>(&v18) = static_cast<float>(m_data[aiIndex0]);
    v9[0] = 0.0f;
    v8[0] = afScale;
    *reinterpret_cast<float*>(&v21) = static_cast<float>(m_data[aiIndex0 + 1]);
    *reinterpret_cast<float*>(&v15) = static_cast<float>(m_data[aiIndex0 + 2]);
    __m128 v11 = _mm_mul_ps(
        _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v18)), _mm_set_ss(std::bit_cast<float>(v15))),
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v21)), v9)
        ),
        _mm_shuffle_ps(v8, v8, 0)
    );
    v12.m_quad = apChunk->m_offset.m_quad;
    arVertex0.m_quad = _mm_add_ps(v11, apChunk->m_offset.m_quad);
    uint16_t* v13 = apChunk->m_vertices.begin();
    *reinterpret_cast<float*>(&v19) = static_cast<float>(v13[aiIndex1]);
    v12.m_quad[0] = afScale;
    *reinterpret_cast<float*>(&v16) = static_cast<float>(v13[aiIndex1 + 1]);
    *reinterpret_cast<float*>(&v22) = static_cast<float>(v13[aiIndex1 + 2]);
    arVertex1.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v19)), _mm_set_ss(std::bit_cast<float>(v22))),
                _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v16)), v9)
            ),
            _mm_shuffle_ps(v12.m_quad, v12.m_quad, 0)
        ),
        apChunk->m_offset.m_quad
    );
    uint16_t* v14 = apChunk->m_vertices.begin();
    *reinterpret_cast<float*>(&v20) = static_cast<float>(v14[aiIndex2]);
    *reinterpret_cast<float*>(&v17) = static_cast<float>(v14[aiIndex2 + 1]);
    *reinterpret_cast<float*>(&v23) = static_cast<float>(v14[aiIndex2 + 2]);
    arVertex2.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v20)), _mm_set_ss(std::bit_cast<float>(v23))),
                _mm_unpacklo_ps(
                    _mm_set_ss(std::bit_cast<float>(v17)),
                    _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
                )
            ),
            _mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0)
        ),
        apChunk->m_offset.m_quad
    );
}

const hkClass& hkpCompressedMeshShape::staticClass() {
    return hkpCompressedMeshShapeClass;
}
