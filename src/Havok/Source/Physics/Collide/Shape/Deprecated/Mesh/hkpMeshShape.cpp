#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
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
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpMeshShapeSubpart(void* p);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpMeshShape();
static void cleanupLoadedObjecthkpMeshShape(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMeshShapeSubpartTypeInfo = {"hkpMeshShapeSubpart",
                                          nullptr,
                                          cleanupLoadedObjecthkpMeshShapeSubpart,
                                          nullptr,
                                          &typeid(hkpMeshShape::Subpart),
                                          sizeof(hkpMeshShape::Subpart)};

// hkpMeshShape's and hkpMeshShape::Subpart's reflection records: the striding types' values and enums, the defaults and
// the members.
using hkpMeshShapeMeshShapeIndexStridingTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpMeshShapeMeshShapeIndexStridingTypeEnumItemsArray hkpMeshShapeMeshShapeIndexStridingTypeEnumItems{
    {0, "INDICES_INVALID"},
    {1, "INDICES_INT16"},
    {2, "INDICES_INT32"},
    {3, "INDICES_MAX_ID"}
};
using hkpMeshShapeMeshShapeMaterialIndexStridingTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpMeshShapeMeshShapeMaterialIndexStridingTypeEnumItemsArray
    hkpMeshShapeMeshShapeMaterialIndexStridingTypeEnumItems{
        {0, "MATERIAL_INDICES_INVALID"},
        {1, "MATERIAL_INDICES_INT8"},
        {2, "MATERIAL_INDICES_INT16"},
        {3, "MATERIAL_INDICES_MAX_ID"}
};
using hkpMeshShapeEnumsArray = const hkClassEnum[2];
static hkpMeshShapeEnumsArray hkpMeshShapeEnums{
    {"MeshShapeIndexStridingType", hkpMeshShapeMeshShapeIndexStridingTypeEnumItems, 4, nullptr, {0}},
    {"MeshShapeMaterialIndexStridingType", hkpMeshShapeMeshShapeMaterialIndexStridingTypeEnumItems, 4, nullptr, {0}}
};

static const hkClassEnum* hkpMeshShapeMeshShapeIndexStridingTypeEnum = &hkpMeshShapeEnums[0];
static const hkClassEnum* hkpMeshShapeMeshShapeMaterialIndexStridingTypeEnum = &hkpMeshShapeEnums[1];

using hkpMeshShapeSubpartClass_MembersArray = hkClassMember[15];
static hkpMeshShapeSubpartClass_MembersArray hkpMeshShapeSubpartClass_Members{
    {"vertexBase",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x0,
     nullptr},
    {"vertexStriding", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
    {"numVertices", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"indexBase",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC,
     nullptr},
    {"stridingType",
     nullptr,
     &hkpMeshShapeEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0x10,
     nullptr},
    {"materialIndexStridingType",
     nullptr,
     &hkpMeshShapeEnums[1],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0x11,
     nullptr},
    {"indexStriding", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"flipAlternateTriangles",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x18,
     nullptr},
    {"numTriangles", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1C, nullptr},
    {"materialIndexBase",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x20,
     nullptr},
    {"materialIndexStriding",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x24,
     nullptr},
    {"materialBase",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x28,
     nullptr},
    {"materialStriding",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x2C,
     nullptr},
    {"numMaterials", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"triangleOffset", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x34, nullptr}
};

// The weldingType member's enum is set on its line in this table rather than left null and stored by an initialiser.
using hkpMeshShapeClass_MembersArray = hkClassMember[7];
static hkpMeshShapeClass_MembersArray hkpMeshShapeClass_Members{
    {"scaling", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"numBitsForSubpartIndex",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"subparts",
     &hkpMeshShapeSubpartClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x34,
     nullptr},
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x40, nullptr},
    {"weldingType",
     nullptr,
     &hkpWeldingUtilityEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x4C,
     nullptr},
    {"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr},
    {"pad", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 3, {0}, 0x54, nullptr}
};

static void cleanupLoadedObjecthkpMeshShapeSubpart(void* p) {
    ;
}

// Returns the vptr that hkpMeshShape's finish constructor (flag 0) stores.
static const void* getVtablehkpMeshShape() {
    return hkGetVtableOf<hkpMeshShape>();
}

static void cleanupLoadedObjecthkpMeshShape(void* p) {
    static_cast<hkpMeshShape*>(p)->~hkpMeshShape();
}

namespace {
    // The SDK's default structs: each member's offset into it (-1: no default), then the one default each class has,
    // Subpart's triangleOffset and hkpMeshShape's weldingType.
    struct hkpMeshShapeSubpart_DefaultStruct {
        int32_t s_defaultOffsets[15];
        int32_t m_triangleOffset;
    };
    struct hkpMeshShape_DefaultStruct {
        int32_t s_defaultOffsets[7];
        hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    };
} // namespace
static const hkpMeshShapeSubpart_DefaultStruct hkpMeshShapeSubpart_Default{
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x3C},
    -1
};
static const hkpMeshShape_DefaultStruct hkpMeshShape_Default{
    {-1, -1, -1, -1, 0x1C, -1, -1},
    {hkpWeldingUtility::WELDING_TYPE_NONE}
};

hkClass hkpMeshShapeSubpartClass(
    "hkpMeshShapeSubpart",
    nullptr,
    0x38,
    nullptr,
    0,
    nullptr,
    0,
    hkpMeshShapeSubpartClass_Members,
    15,
    &hkpMeshShapeSubpart_Default,
    nullptr,
    0,
    0
);
hkClass hkpMeshShapeClass(
    "hkpMeshShape",
    &hkpShapeCollectionClass,
    0x60,
    nullptr,
    0,
    hkpMeshShapeEnums,
    2,
    hkpMeshShapeClass_Members,
    7,
    &hkpMeshShape_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMeshShapeTypeInfo = {
    "hkpMeshShape",          finishLoadedObjecthkpMeshShape, cleanupLoadedObjecthkpMeshShape,
    getVtablehkpMeshShape(), &typeid(hkpMeshShape),          sizeof(hkpMeshShape)
};

const hkClass& hkpMeshShape::Subpart::staticClass() {
    return hkpMeshShapeSubpartClass;
}

hkpMeshShape::hkpMeshShape(float a1, int32_t a2)
  : hkpShapeCollection(HK_SHAPE_TRIANGLE_COLLECTION, hkpShapeCollection::COLLECTION_MESH_SHAPE),
    m_numBitsForSubpartIndex(a2), m_radius(a1) {
    m_scaling.m_quad = _mm_set1_ps(1.0f);
    m_weldingType.m_storage = hkpWeldingUtility::WELDING_TYPE_NONE;
}

hkpMeshShape::hkpMeshShape(hkFinishLoadedObjectFlag flag)
  : hkpShapeCollection(flag), m_subparts(flag), m_weldingInfo(flag) {
    if (flag.m_finishing) {
        // Subparts saved before the striding type existed get the 8-bit material indices.
        for (Subpart& kPart : m_subparts) {
            if (kPart.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INVALID) {
                kPart.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
            }
        }
        m_type.m_storage = HK_SHAPE_TRIANGLE_COLLECTION;
        m_collectionType.m_storage = hkpShapeCollection::COLLECTION_MESH_SHAPE;
    }
}

hkpMeshShape::~hkpMeshShape() {}

// The subpart array's used and allocated size, then the collection's own statistics for hkpShapeCollectionClass (not
// the cls argument). The array is reported when its capacity is managed or the collector counts unmanaged memory; 56 is
// sizeof(hkpMeshShape::Subpart).
void hkpMeshShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t v4 = m_subparts.m_capacityAndFlags;
    if (v4 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v5 = hkStatisticsCollector::getAllocatedSize(
            m_subparts.m_data, 56 * static_cast<int32_t>(v4 & hkArray<Subpart>::CAPACITY_MASK)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "SubParts", m_subparts.m_data, 56 * m_subparts.m_size, v5
        );
    }
    hkpShapeCollection::calcContentStatistics(collector, &hkpShapeCollectionClass);
}

// The box of every subpart triangle's three vertices, each scaled by m_scaling and transformed by localToWorld
// (addToAabb), widened by the radius plus tolerance. The box starts from 0x1.FFFFDCp+127f and its negation, with w
// zero. An INDICES_INT16 subpart's indices are zero-extended 16-bit reads, any other striding's 32-bit ones. The radius
// plus tolerance is rounded to a float. hkpFastMeshShape and hkpStorageMeshShape do not override this.
void hkpMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_min.m_quad)[0],
        _mm_setr_ps(0x1.FFFFDCp+127f, 0x1.FFFFDCp+127f, 0x1.FFFFDCp+127f, 0.0f)
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_max.m_quad)[0],
        _mm_setr_ps(-0x1.FFFFDCp+127f, -0x1.FFFFDCp+127f, -0x1.FFFFDCp+127f, 0.0f)
    );
    for (const Subpart& part : m_subparts) {
        for (int32_t i = 0; i < part.m_numTriangles; ++i) {
            // The index and vertex buffers are untyped strided memory (void* and byte strides).
            const uint8_t* pTriangle = static_cast<const uint8_t*>(part.m_indexBase) + i * part.m_indexStriding;
            int32_t iA;
            int32_t iB;
            int32_t iC;
            if (part.m_stridingType.m_storage == INDICES_INT16) {
                const uint16_t* pIndices = reinterpret_cast<const uint16_t*>(pTriangle);
                iA = pIndices[0];
                iB = pIndices[1];
                iC = pIndices[2];
            }
            else {
                const int32_t* pIndices = reinterpret_cast<const int32_t*>(pTriangle);
                iA = pIndices[0];
                iB = pIndices[1];
                iC = pIndices[2];
            }
            const uint8_t* pVertices = reinterpret_cast<const uint8_t*>(part.m_vertexBase);
            const float* pA = reinterpret_cast<const float*>(pVertices + iA * part.m_vertexStriding);
            const float* pB = reinterpret_cast<const float*>(pVertices + iB * part.m_vertexStriding);
            const float* pC = reinterpret_cast<const float*>(pVertices + iC * part.m_vertexStriding);
            addToAabb(out, localToWorld, pA, m_scaling);
            addToAabb(out, localToWorld, pB, m_scaling);
            addToAabb(out, localToWorld, pC, m_scaling);
        }
    }
    float fTolerance = m_radius + tolerance;
    __m128 kTolerance = _mm_set1_ps(fTolerance);
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_min.m_quad)[0],
        _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0]), kTolerance)
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_max.m_quad)[0],
        _mm_add_ps(kTolerance, _mm_load_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0]))
    );
}

void hkpMeshShape::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    m_weldingType.m_storage = weldingType;
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
        int32_t v3 = 0;
        int32_t v4 = 0;
        if (m_subparts.m_size > 0) {
            hkpMeshShape::Subpart* m_data = m_subparts.begin();
            int32_t v6 = 0;
            do {
                m_data[v6].m_triangleOffset = v3;
                m_data = m_subparts.begin();
                v3 += m_data[v6].m_numTriangles;
                ++v4;
                ++v6;
            }
            while (v4 < m_subparts.m_size);
        }
        hkArray<uint16_t, hkContainerHeapAllocator>* p_m_weldingInfo = &m_weldingInfo;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if ((p_m_weldingInfo->m_capacityAndFlags & 0x3FFFFFFF) < v3) {
            hkArrayUtil::_reserve(Instance->m_heap, p_m_weldingInfo, v3, sizeof(uint16_t));
        }
        hkMemoryAllocator* v9 = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t v10 = p_m_weldingInfo->m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
        if (v10 < v3) {
            int32_t v11 = 2 * v10;
            if (v3 >= v11) {
                v11 = v3;
            }
            hkArrayUtil::_reserve(v9, p_m_weldingInfo, v11, sizeof(uint16_t));
        }
        p_m_weldingInfo->m_size = v3;
    }
}

void hkpMeshShape::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    m_weldingInfo.m_data
        [m_subparts.m_data[key >> (32 - m_numBitsForSubpartIndex)].m_triangleOffset +
         (key & 0xFFFFFFFFu >> m_numBitsForSubpartIndex)] = weldingInfo;
}

// The SDK's m_subparts.expandOne() = part: the array's _reserveMore when it is full, Subpart's default constructor
// on the new element (placement new, null-checked), then the copy. A part without material indices gets one
// material, its material and index bases pointing at hkVector4Zero (zero bytes: material 0, filter info 0).
void hkpMeshShape::addSubpart(const Subpart& part) {
    hkArray<Subpart>& subparts = m_subparts;
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (subparts.m_size == (subparts.m_capacityAndFlags & hkArray<Subpart>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*router.m_heap, &subparts, sizeof(Subpart));
    }
    Subpart* pNew = subparts.m_data + subparts.m_size;
    if (pNew) {
        // EXE-INLINED none: hkpMeshShape::Subpart::Subpart
        pNew->m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
        pNew->m_materialIndexStriding = 0;
        pNew->m_materialStriding = 0;
        pNew->m_numMaterials = 1;
        pNew->m_materialBase = nullptr;
        pNew->m_materialIndexBase = nullptr;
        pNew->m_triangleOffset = -1;
        pNew->m_flipAlternateTriangles = 0;
    }
    Subpart& added = subparts.m_data[subparts.m_size++];
    added = part;
    if (!added.m_materialIndexBase) {
        added.m_numMaterials = 1;
        added.m_materialBase = static_cast<const hkpMeshMaterial*>(static_cast<const void*>(&hkVector4Zero));
        added.m_materialIndexBase = &hkVector4Zero;
    }
}

// The first triangle's key, or the next one when the first triangle is degenerate; HK_INVALID_SHAPE_KEY without
// subparts.
uint32_t hkpMeshShape::getFirstKey() const {
    const hkpShapeContainer* pContainer = this;
    if (!m_subparts.m_size) {
        return HK_INVALID_SHAPE_KEY;
    }
    alignas(16) char buffer[512];
    const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pContainer->getChildShape(0, buffer));
    if (!hkpTriangleUtil::isDegenerate(
             pTriangle->getVertex(0), pTriangle->getVertex(1), pTriangle->getVertex(2),
             hkDefaultTriangleDegeneracyTolerance
        )
             .m_bool) {
        return 0;
    }
    return pContainer->getNextKey(0);
}

// Steps to the next triangle, past the end of each subpart into the next (the new subpart's triangle count is not
// checked), skipping degenerate triangles. The triangle index compares signed, the subpart index unsigned.
uint32_t hkpMeshShape::getNextKey(uint32_t initialKey) const {
    const hkpShapeContainer* pContainer = this;
    alignas(16) char buffer[512];
    hkBool result;
    uint32_t subPart = initialKey >> (32 - m_numBitsForSubpartIndex);
    int32_t triIndex = static_cast<int32_t>(initialKey & (~0u >> m_numBitsForSubpartIndex));
    while (true) {
        if (++triIndex >= m_subparts.m_data[subPart].m_numTriangles) {
            if (++subPart >= static_cast<uint32_t>(m_subparts.m_size)) {
                return HK_INVALID_SHAPE_KEY;
            }
            triIndex = 0;
        }
        const uint32_t key = (subPart << (32 - m_numBitsForSubpartIndex)) | static_cast<uint32_t>(triIndex);
        const hkpTriangleShape* pTriangle =
            static_cast<const hkpTriangleShape*>(pContainer->getChildShape(key, buffer));
        if (!hkpTriangleUtil::isDegenerate(
                 pTriangle->getVertex(0), pTriangle->getVertex(1), pTriangle->getVertex(2),
                 hkDefaultTriangleDegeneracyTolerance
            )
                 .m_bool) {
            return key;
        }
    }
}

// The filter info of the key's material, or 0 where the subpart has no material indices or the material's address
// is null (the SDK's hkAddByteOffsetConst of the material base by index times stride: the materials are strided
// memory, stepped through as bytes).
uint32_t hkpMeshShape::getCollisionFilterInfo(uint32_t key) const {
    const int32_t materialIndex = getMaterialIndex(key);
    if (materialIndex == -1) {
        return 0;
    }
    const Subpart& part = m_subparts.m_data[key >> (32 - m_numBitsForSubpartIndex)];
    const uint8_t* pMaterialBytes = static_cast<const uint8_t*>(static_cast<const void*>(part.m_materialBase));
    const hkpMeshMaterial* pMaterial = static_cast<const hkpMeshMaterial*>(
        static_cast<const void*>(pMaterialBytes + materialIndex * part.m_materialStriding)
    );
    if (!pMaterial) {
        return 0;
    }
    return pMaterial->m_filterInfo;
}

// The triangle of key, built in buffer: the subpart's three vertex indices (16- or 32-bit; the second and third swapped
// on odd triangles where m_flipAlternateTriangles is set), each vertex scaled by m_scaling with w zero; then the
// triangle shape is constructed with the mesh's radius, the triangle's welding info (0 where the mesh has none) and the
// mesh's welding type, then the three vertices.
const hkpShape* hkpMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
    const uint32_t triangleIndex = key & (~0u >> m_numBitsForSubpartIndex);
    const uint32_t subPartIndex = key >> (32 - m_numBitsForSubpartIndex);
    const Subpart& part = m_subparts.m_data[subPartIndex];
    const int32_t flip = static_cast<int32_t>(triangleIndex) & part.m_flipAlternateTriangles;
    // The index and vertex buffers are untyped strided memory (void* and byte strides).
    const uint8_t* pVertices = static_cast<const uint8_t*>(static_cast<const void*>(part.m_vertexBase));
    const uint8_t* pIndices =
        static_cast<const uint8_t*>(part.m_indexBase) + part.m_indexStriding * static_cast<int32_t>(triangleIndex);
    const float* pV0;
    const float* pV1;
    const float* pV2;
    if (part.m_stridingType.m_storage == INDICES_INT16) {
        const uint16_t* pTriangle16 = reinterpret_cast<const uint16_t*>(pIndices);
        pV0 = reinterpret_cast<const float*>(pVertices + part.m_vertexStriding * pTriangle16[0]);
        pV1 = reinterpret_cast<const float*>(pVertices + part.m_vertexStriding * pTriangle16[1 + flip]);
        pV2 = reinterpret_cast<const float*>(pVertices + part.m_vertexStriding * pTriangle16[1 + (flip ^ 1)]);
    }
    else {
        const uint32_t* pTriangle32 = reinterpret_cast<const uint32_t*>(pIndices);
        pV0 = reinterpret_cast<const float*>(pVertices + part.m_vertexStriding * static_cast<int32_t>(pTriangle32[0]));
        pV1 = reinterpret_cast<const float*>(
            pVertices + part.m_vertexStriding * static_cast<int32_t>(pTriangle32[1 + flip])
        );
        pV2 = reinterpret_cast<const float*>(
            pVertices + part.m_vertexStriding * static_cast<int32_t>(pTriangle32[1 + (flip ^ 1)])
        );
    }
    hkVector4 v0;
    v0.set(
        pV0[0] * XFNV_M128_F32(m_scaling.m_quad, 0), pV0[1] * XFNV_M128_F32(m_scaling.m_quad, 1),
        pV0[2] * XFNV_M128_F32(m_scaling.m_quad, 2), 0.0f
    );
    hkVector4 v1;
    v1.set(
        XFNV_M128_F32(m_scaling.m_quad, 0) * pV1[0], pV1[1] * XFNV_M128_F32(m_scaling.m_quad, 1),
        pV1[2] * XFNV_M128_F32(m_scaling.m_quad, 2), 0.0f
    );
    hkVector4 v2;
    v2.set(
        XFNV_M128_F32(m_scaling.m_quad, 0) * pV2[0], pV2[1] * XFNV_M128_F32(m_scaling.m_quad, 1),
        pV2[2] * XFNV_M128_F32(m_scaling.m_quad, 2), 0.0f
    );
    const uint16_t weldingInfo =
        m_weldingInfo.m_size == 0 ? 0 : m_weldingInfo.m_data[part.m_triangleOffset + triangleIndex];
    hkpTriangleShape* pTriangle = nullptr;
    char* pBuffer = buffer;
    if (pBuffer) {
        const uint8_t ucWeldingType = m_weldingType.m_storage;
        pTriangle = ::new (pBuffer)
            hkpTriangleShape(m_radius, weldingInfo, static_cast<hkpWeldingUtility::WeldingType>(ucWeldingType));
    }
    pTriangle->m_vertexA = v0;
    pTriangle->m_vertexB = v1;
    pTriangle->m_vertexC = v2;
    return pTriangle;
}

void hkpMeshShape::setScaling(const hkVector4& scaling) {
    m_scaling = scaling;
}

void hkpMeshShape::addToAabb(
    hkAabb& aabb,
    const hkTransform& localToWorld,
    const float* apVertex,
    const hkVector4& scaling
) {
    uint32_t v5;
    uint32_t v6;
    uint32_t v7;
    hkVector4 a3;
    hkVector4 v9;
    *reinterpret_cast<float*>(&v5) = *apVertex * XFNV_M128_F32(scaling.m_quad, 0);
    *reinterpret_cast<float*>(&v6) = apVertex[1] * XFNV_M128_F32(scaling.m_quad, 1);
    *reinterpret_cast<float*>(&v7) = apVertex[2] * XFNV_M128_F32(scaling.m_quad, 2);
    a3.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v5)), _mm_set_ss(std::bit_cast<float>(v7))),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(v6)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    v9.setTransformedPos(localToWorld, a3);
    __m128 m_quad = v9.m_quad;
    aabb.m_min.m_quad = _mm_min_ps(aabb.m_min.m_quad, v9.m_quad);
    aabb.m_max.m_quad = _mm_max_ps(aabb.m_max.m_quad, m_quad);
}

const hkClass& hkpMeshShape::staticClass() {
    return hkpMeshShapeClass;
}
