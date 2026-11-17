#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/SimpleMesh/hkpSimpleMeshShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"

namespace {
    // Each member's offset in this struct (-1: no default), then weldingType's default.
    struct hkpSimpleMeshShape_DefaultStruct {
        int32_t s_defaultOffsets[5];
        uint8_t m_weldingType;
    };
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSimpleMeshShapeTriangleTypeInfo = {
    "hkpSimpleMeshShapeTriangle",
    nullptr,
    cleanupLoadedObjecthkpSimpleMeshShapeTriangle,
    nullptr,
    &typeid(hkpSimpleMeshShape::Triangle),
    sizeof(hkpSimpleMeshShape::Triangle)
};

// The reflection's member tables.
using hkpSimpleMeshShape_TriangleClass_MembersArray = const hkClassMember[4];
static hkpSimpleMeshShape_TriangleClass_MembersArray hkpSimpleMeshShape_TriangleClass_Members{
    {"a", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"b", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
    {"c", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr}
};

using hkpSimpleMeshShapeClass_MembersArray = hkClassMember[5];
static hkpSimpleMeshShapeClass_MembersArray hkpSimpleMeshShapeClass_Members{
    {"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, {0}, 0x18, nullptr},
    {"triangles",
     &hkpSimpleMeshShapeTriangleClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x24,
     nullptr},
    {"materialIndices",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x30,
     nullptr},
    {"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x3C, nullptr},
    {"weldingType",
     nullptr,
     &hkpWeldingUtilityEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x40,
     nullptr}
};

// The defaults the class record names.
static const hkpSimpleMeshShape_DefaultStruct hkpSimpleMeshShape_Default{
    {-1, -1, -1, -1, 0x14},
    hkpWeldingUtility::WELDING_TYPE_NONE
};
hkClass hkpSimpleMeshShapeTriangleClass(
    "hkpSimpleMeshShapeTriangle",
    nullptr,
    0x10,
    nullptr,
    0,
    nullptr,
    0,
    hkpSimpleMeshShape_TriangleClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpSimpleMeshShapeClass(
    "hkpSimpleMeshShape",
    &hkpShapeCollectionClass,
    0x44,
    nullptr,
    0,
    nullptr,
    0,
    hkpSimpleMeshShapeClass_Members,
    5,
    &hkpSimpleMeshShape_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSimpleMeshShapeTypeInfo{
    "hkpSimpleMeshShape",          finishLoadedObjecthkpSimpleMeshShape, cleanupLoadedObjecthkpSimpleMeshShape,
    getVtablehkpSimpleMeshShape(), &typeid(hkpSimpleMeshShape),          sizeof(hkpSimpleMeshShape)
};

void cleanupLoadedObjecthkpSimpleMeshShape(void* p) {
    static_cast<hkpSimpleMeshShape*>(p)->~hkpSimpleMeshShape();
}

const hkClass& hkpSimpleMeshShape::Triangle::staticClass() {
    return hkpSimpleMeshShapeTriangleClass;
}

hkpSimpleMeshShape::hkpSimpleMeshShape(float radius)
  : hkpShapeCollection(HK_SHAPE_TRIANGLE_COLLECTION, COLLECTION_SIMPLE_MESH), m_radius(radius) {
    m_weldingType.m_storage = hkpWeldingUtility::WELDING_TYPE_NONE;
}

// The three arrays free their own storage.
hkpSimpleMeshShape::~hkpSimpleMeshShape() {}

// Each array's used and allocated size, then the collection's own statistics for hkpShapeCollectionClass.
void hkpSimpleMeshShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t v4 = m_vertices.m_capacityAndFlags;
    if (v4 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v5 = hkStatisticsCollector::getAllocatedSize(
            m_vertices.m_data, 16 * static_cast<int32_t>(v4 & hkArray<hkVector4>::CAPACITY_MASK)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "Vertices", m_vertices.m_data, 16 * m_vertices.m_size, v5
        );
    }
    int32_t v6 = m_triangles.m_capacityAndFlags;
    if (v6 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v7 = hkStatisticsCollector::getAllocatedSize(
            m_triangles.m_data, 16 * static_cast<int32_t>(v6 & hkArray<Triangle>::CAPACITY_MASK)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "Triangles", m_triangles.m_data, 16 * m_triangles.m_size, v7
        );
    }
    int32_t v8 = m_materialIndices.m_capacityAndFlags;
    if (v8 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v9 = hkStatisticsCollector::getAllocatedSize(
            m_materialIndices.m_data, static_cast<int32_t>(v8 & hkArray<uint8_t>::CAPACITY_MASK)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "MaterialIds", m_materialIndices.m_data, m_materialIndices.m_size, v9
        );
    }
    hkpShapeCollection::calcContentStatistics(collector, &hkpShapeCollectionClass);
}

// The bounds of the transformed vertices, widened by the radius plus the tolerance on all four lanes. Starts from
// 0x7F7FFFEE and its negation with w zero; min and max take the box first, so a NaN lane takes the vertex's.
void hkpSimpleMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    out.m_min.m_quad[0] = 0x1.FFFFDCp+127f;
    out.m_min.m_quad[1] = 0x1.FFFFDCp+127f;
    out.m_min.m_quad[2] = 0x1.FFFFDCp+127f;
    out.m_min.m_quad[3] = 0.0f;
    out.m_max.m_quad[0] = -0x1.FFFFDCp+127f;
    out.m_max.m_quad[1] = -0x1.FFFFDCp+127f;
    out.m_max.m_quad[2] = -0x1.FFFFDCp+127f;
    out.m_max.m_quad[3] = 0.0f;
    for (int32_t i = 0; i < m_vertices.m_size; ++i) {
        alignas(16) hkVector4 kVertex;
        kVertex.setTransformedPos(localToWorld, m_vertices.m_data[i]);
        hkQuadReal& kMin = out.m_min.m_quad;
        kMin[0] = kMin[0] < kVertex.m_quad[0] ? kMin[0] : kVertex.m_quad[0];
        kMin[1] = kMin[1] < kVertex.m_quad[1] ? kMin[1] : kVertex.m_quad[1];
        kMin[2] = kMin[2] < kVertex.m_quad[2] ? kMin[2] : kVertex.m_quad[2];
        kMin[3] = kMin[3] < kVertex.m_quad[3] ? kMin[3] : kVertex.m_quad[3];
        hkQuadReal& kMax = out.m_max.m_quad;
        kMax[0] = kMax[0] > kVertex.m_quad[0] ? kMax[0] : kVertex.m_quad[0];
        kMax[1] = kMax[1] > kVertex.m_quad[1] ? kMax[1] : kVertex.m_quad[1];
        kMax[2] = kMax[2] > kVertex.m_quad[2] ? kMax[2] : kVertex.m_quad[2];
        kMax[3] = kMax[3] > kVertex.m_quad[3] ? kMax[3] : kVertex.m_quad[3];
    }
    // The sum is rounded to a float.
    float fRadius = m_radius + tolerance;
    out.m_min.m_quad[0] -= fRadius;
    out.m_min.m_quad[1] -= fRadius;
    out.m_min.m_quad[2] -= fRadius;
    out.m_min.m_quad[3] -= fRadius;
    out.m_max.m_quad[0] += fRadius;
    out.m_max.m_quad[1] += fRadius;
    out.m_max.m_quad[2] += fRadius;
    out.m_max.m_quad[3] += fRadius;
}

void hkpSimpleMeshShape::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    m_weldingType.m_storage = (uint8_t)((int8_t)weldingType);
}

void hkpSimpleMeshShape::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    m_triangles.m_data[key].m_weldingInfo = weldingInfo;
}

// The key of the first triangle that is not degenerate, or HK_INVALID_SHAPE_KEY.
uint32_t hkpSimpleMeshShape::getFirstKey() const {
    for (int32_t i = 0; i < m_triangles.m_size; ++i) {
        const Triangle& kTriangle = m_triangles.m_data[i];
        hkBool kDegenerate;
        if (!hkpTriangleUtil::isDegenerate(
                 &kDegenerate, m_vertices.m_data[kTriangle.m_a], m_vertices.m_data[kTriangle.m_b],
                 m_vertices.m_data[kTriangle.m_c], hkDefaultTriangleDegeneracyTolerance
            )
                 ->m_bool) {
            return static_cast<uint32_t>(i);
        }
    }
    return HK_INVALID_SHAPE_KEY;
}

// The key of the first triangle after oldKey that is not degenerate, or HK_INVALID_SHAPE_KEY. The key is compared
// signed with the triangle count.
uint32_t hkpSimpleMeshShape::getNextKey(uint32_t oldKey) const {
    for (int32_t i = static_cast<int32_t>(oldKey) + 1; i < m_triangles.m_size; ++i) {
        const Triangle& kTriangle = m_triangles.m_data[i];
        hkBool kDegenerate;
        if (!hkpTriangleUtil::isDegenerate(
                 &kDegenerate, m_vertices.m_data[kTriangle.m_a], m_vertices.m_data[kTriangle.m_b],
                 m_vertices.m_data[kTriangle.m_c], hkDefaultTriangleDegeneracyTolerance
            )
                 ->m_bool) {
            return static_cast<uint32_t>(i);
        }
    }
    return HK_INVALID_SHAPE_KEY;
}

// The triangle of key, built in buffer: the triangle shape is constructed with the mesh's radius, the triangle's
// welding info and the mesh's welding type, then the three vertices are stored. The buffer is null-checked but the
// vertices are stored even when it is null, as the game does.
const hkpShape* hkpSimpleMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
    hkpTriangleShape* pTriangle = nullptr;
    char* pBuffer = buffer;
    if (pBuffer) {
        const uint8_t ucWeldingType = m_weldingType.m_storage;
        const uint16_t usWeldingInfo = m_triangles.m_data[key].m_weldingInfo;
        pTriangle = ::new (pBuffer)
            hkpTriangleShape(m_radius, usWeldingInfo, static_cast<hkpWeldingUtility::WeldingType>(ucWeldingType));
    }
    pTriangle->m_vertexA = m_vertices.m_data[m_triangles.m_data[key].m_a];
    pTriangle->m_vertexB = m_vertices.m_data[m_triangles.m_data[key].m_b];
    pTriangle->m_vertexC = m_vertices.m_data[m_triangles.m_data[key].m_c];
    return pTriangle;
}

const hkClass& hkpSimpleMeshShape::staticClass() {
    return hkpSimpleMeshShapeClass;
}
