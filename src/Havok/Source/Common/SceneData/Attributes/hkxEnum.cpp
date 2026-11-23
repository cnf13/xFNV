#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxEnumItemTypeInfo = {
    "hkxEnumItem",
    finishLoadedObjecthkxEnumItem,
    cleanupLoadedObjecthkxEnumItem,
    nullptr,
    &typeid(hkxEnum::Item),
    sizeof(hkxEnum::Item)
};

// The class records' member tables.
using hkxEnumItemClassMemberArray = const hkClassMember[2];
static hkxEnumItemClassMemberArray hkxEnumItemClass_Members{
    {"value", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr}
};
hkClass hkxEnumItemClass(
    "hkxEnumItem",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkxEnumItemClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
using hkxEnumClassMemberArray = const hkClassMember[1];
static hkxEnumClassMemberArray hkxEnumClass_Members{
    {"items", &hkxEnumItemClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x8, nullptr}
};
hkClass hkxEnumClass(
    "hkxEnum",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkxEnumClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkxEnumTypeInfo = {
    "hkxEnum",
    finishLoadedObjecthkxEnum,
    cleanupLoadedObjecthkxEnum,
    getVtablehkxEnum(),
    &typeid(hkxEnum),
    sizeof(hkxEnum)
};

void cleanupLoadedObjecthkxEnum(void* p) {
    static_cast<hkxEnum*>(p)->~hkxEnum();
}

const hkClass& hkxEnum::Item::staticClass() {
    return hkxEnumItemClass;
}

// The member destroys the item names and frees the items.
hkxEnum::~hkxEnum() {}

hkResult hkxEnum::getNameOfValue(int32_t val, const char** name) const {
    int32_t m_size = m_items.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
        return HK_FAILURE;
    }
    hkxEnum::Item* m_data = m_items.m_data;
    for (hkxEnum::Item* i = m_data; i->m_value != val; ++i) {
        if (++v4 >= m_size) {
            return HK_FAILURE;
        }
    }
    *name = reinterpret_cast<const char*>(
        reinterpret_cast<uintptr_t>(m_data[v4].m_name.m_stringAndFlag) & ~static_cast<uintptr_t>(1)
    );
    return HK_SUCCESS;
}

hkResult hkxEnum::getValueOfName(const char* name, int32_t* val) const {
    int32_t v4 = 0;
    if (m_items.m_size <= 0) {
        return HK_FAILURE;
    }
    while (hkString::strCasecmp(
        name,
        reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(m_items.m_data[v4].m_name.m_stringAndFlag) & ~static_cast<uintptr_t>(1)
        )
    )) {
        if (++v4 >= m_items.m_size) {
            return HK_FAILURE;
        }
    }
    *val = m_items.m_data[v4].m_value;
    return HK_SUCCESS;
}

const hkClass& hkxEnum::staticClass() {
    return hkxEnumClass;
}
