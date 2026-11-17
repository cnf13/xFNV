#include "Havok/Source/Common/SceneData/Mesh/hkxIndexBuffer.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/string.h"

static void finishLoadedObjecthkxIndexBuffer(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxIndexBuffer();
static void cleanupLoadedObjecthkxIndexBuffer(void* p);

// The reflection's enum records: the names of
// IndexType's values, and the enum.
using hkxIndexBufferIndexTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkxIndexBufferIndexTypeEnumItemsArray hkxIndexBufferIndexTypeEnumItems{
    {0, "INDEX_TYPE_INVALID"},
    {1, "INDEX_TYPE_TRI_LIST"},
    {2, "INDEX_TYPE_TRI_STRIP"},
    {3, "INDEX_TYPE_TRI_FAN"},
    {4, "INDEX_TYPE_MAX_ID"}
};
using hkxIndexBufferEnumsArray = const hkClassEnum[1];
static hkxIndexBufferEnumsArray hkxIndexBufferEnums{{"IndexType", hkxIndexBufferIndexTypeEnumItems, 5, nullptr, {0}}};

static const hkClassEnum* hkxIndexBufferIndexTypeEnum = &hkxIndexBufferEnums[0];

using hkxIndexBufferClass_MembersArray = const hkClassMember[5];
static hkxIndexBufferClass_MembersArray hkxIndexBufferClass_Members{
    {"indexType", nullptr, &hkxIndexBufferEnums[0], hkClassMember::TYPE_ENUM, hkClassMember::TYPE_INT8, 0, 0, 0x8,
     nullptr},
    {"indices16", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT16, 0, 0, 0xC, nullptr},
    {"indices32", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x18, nullptr},
    {"vertexBaseOffset", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x24, nullptr},
    {"length", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr}
};
hkClass hkxIndexBufferClass(
    "hkxIndexBuffer",
    &hkReferencedObjectClass,
    0x2C,
    nullptr,
    0,
    hkxIndexBufferEnums,
    1,
    hkxIndexBufferClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxIndexBufferTypeInfo = {
    "hkxIndexBuffer",          finishLoadedObjecthkxIndexBuffer, cleanupLoadedObjecthkxIndexBuffer,
    getVtablehkxIndexBuffer(), &typeid(hkxIndexBuffer),          sizeof(hkxIndexBuffer)
};

// The type info's finish-loaded-object callback for hkxIndexBuffer: its finish constructor, when the object is not
// null.
static void finishLoadedObjecthkxIndexBuffer(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxIndexBuffer(flag);
    }
}

static const void* getVtablehkxIndexBuffer() {
    return hkGetVtableOf<hkxIndexBuffer>();
}

static void cleanupLoadedObjecthkxIndexBuffer(void* p) {
    static_cast<hkxIndexBuffer*>(p)->~hkxIndexBuffer();
}

// The members' destructors free both index arrays.
hkxIndexBuffer::~hkxIndexBuffer() {}

int32_t hkxIndexBuffer::getNumTriangles() {
    int32_t m_size = m_indices16.m_size;
    if (!m_size) {
        m_size = m_indices32.m_size;
    }
    if (m_indexType.m_storage == 1) {
        return m_size / 3;
    }
    else {
        return 0;
    }
}

char hkxIndexBuffer::getTriangleIndicesPtr(uint32_t a1, uint32_t* a2, uint32_t* a3, uint32_t* a4) {
    if (m_indices16.m_size) {
        if (m_indexType.m_storage == 1) {
            *a2 = m_indices16.m_data[3 * a1];
            *a3 = m_indices16.m_data[3 * a1 + 1];
            *a4 = m_indices16.m_data[3 * a1 + 2];
            return 1;
        }
        if (m_indexType.m_storage == 2) {
            *a2 = m_indices16.m_data[a1];
            *a3 = m_indices16.m_data[a1 + 1];
            *a4 = m_indices16.m_data[a1 + 2];
            return 1;
        }
    }
    else {
        if (m_indices32.m_size) {
            if (m_indexType.m_storage == 1) {
                *a2 = m_indices32.m_data[3 * a1];
                *a3 = m_indices32.m_data[3 * a1 + 1];
                *a4 = m_indices32.m_data[3 * a1 + 2];
                return 1;
            }
            if (m_indexType.m_storage == 2) {
                *a2 = m_indices32.m_data[a1];
                *a3 = m_indices32.m_data[a1 + 1];
                *a4 = m_indices32.m_data[a1 + 2];
                return 1;
            }
        }
    }
    *a2 = -1;
    *a3 = -1;
    *a4 = -1;
    return 0;
}

const hkClass& hkxIndexBuffer::staticClass() {
    return hkxIndexBufferClass;
}
