#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Crc32/hkCrc32StreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

static void cleanupLoadedObjecthkClassEnum(void* p);
static void cleanupLoadedObjecthkClassEnumItem(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkClassEnumItemTypeInfo = {
    "hkClassEnumItem",        nullptr, cleanupLoadedObjecthkClassEnumItem, nullptr, &typeid(hkClassEnum::Item),
    sizeof(hkClassEnum::Item)
};
hkTypeInfo hkClassEnumTypeInfo = {
    "hkClassEnum", nullptr, cleanupLoadedObjecthkClassEnum, nullptr, &typeid(hkClassEnum), sizeof(hkClassEnum)
};

// hkClassEnumClass's enum FlagValues and its items.
using hkClassEnumFlagValuesEnumItemArray = const hkClassEnum::Item[1];
static hkClassEnumFlagValuesEnumItemArray hkClassEnumFlagValuesEnumItems{{0, "FLAGS_NONE"}};
using hkClassEnumEnumArray = const hkClassEnum[1];
static hkClassEnumEnumArray hkClassEnumEnums{{"FlagValues", hkClassEnumFlagValuesEnumItems, 1, nullptr, 0}};
hkClassEnum::Item::MemberArray hkClassEnum::Item::Members{
    {"value", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr}
};

// The SDK's pointer to the class's enum (hkClassEnumClass.cpp; .data): the flags member's m_enum below reads it at
// start-up.
static const hkClassEnum* hkClassEnumFlagValuesEnum = &hkClassEnumEnums[0];
hkClassEnum::MemberArray hkClassEnum::Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"items",
     &hkClassEnumItemClass,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x4,
     nullptr},
    {"attributes",
     &hkCustomAttributesClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0xC,
     nullptr},
    {"flags",
     nullptr,
     hkClassEnumFlagValuesEnum,
     {hkClassMember::TYPE_FLAGS},
     {hkClassMember::TYPE_UINT32},
     0,
     0,
     0x10,
     nullptr}
};
hkClass hkClassEnumItemClass(
    "hkClassEnumItem",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkClassEnum::Item::Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkClassEnumClass(
    "hkClassEnum",
    nullptr,
    20,
    nullptr,
    0,
    hkClassEnumEnums,
    1,
    hkClassEnum::Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkClassEnumItem(void* p) {
    ;
}

static void cleanupLoadedObjecthkClassEnum(void* p) {
    ;
}

hkClass* getClassEnumItemClass() {
    return &hkClassEnumItemClass;
}

hkClass* getClassEnumClass() {
    return &hkClassEnumClass;
}

// Looks the attribute up in the enum's attribute set, when it has one.
const hkVariant* hkClassEnum::getAttribute(const char* id) const {
    if (m_attributes) {
        return m_attributes->getAttribute(id);
    }
    return nullptr;
}

uint32_t hkClassEnum::getSignature() {
    FNVD_INLINED_LOCAL(hkCrc32StreamWriter, a1, std::nullopt, std::nullopt);
    ::new (&a1) hkCrc32StreamWriter(0);
    writeSignature(&a1);
    return a1.getCrc32();
}

hkResult hkClassEnum::getNameOfValue(int32_t val, const char** name) const {
    int32_t m_numItems = this->m_numItems;
    int32_t v4 = 0;
    if (m_numItems <= 0) {
        return HK_FAILURE;
    }
    const hkClassEnum::Item* m_items = this->m_items;
    for (const hkClassEnum::Item* i = m_items; i->m_value != val; ++i) {
        if (++v4 >= m_numItems) {
            return HK_FAILURE;
        }
    }
    *name = m_items[v4].m_name;
    return HK_SUCCESS;
}

hkResult hkClassEnum::getValueOfName(const char* name, int32_t* val) const {
    int32_t v4 = 0;
    if (m_numItems <= 0) {
        return HK_FAILURE;
    }
    while (hkString::strCasecmp(name, m_items[v4].m_name)) {
        if (++v4 >= m_numItems) {
            return HK_FAILURE;
        }
    }
    *val = m_items[v4].m_value;
    return HK_SUCCESS;
}

// Writes, through an archive on w, the enum's name, each item's name and value, and the item count (read once). The
// archive is created without byte-swapping.
void hkClassEnum::writeSignature(hkStreamWriter* w) const {
    hkOArchive oa(w, hkBool(false));
    oa.writeRaw(m_name, hkString::strLen(m_name));
    int32_t iNumItems = m_numItems;
    for (int32_t i = 0; i < iNumItems; ++i) {
        const hkClassEnum::Item& item = m_items[i];
        oa.writeRaw(item.m_name, hkString::strLen(item.m_name));
        oa.write32(item.m_value);
    }
    oa.write32(iNumItems);
}

hkResult hkClassEnum::decomposeFlags(
    int32_t flagValue,
    hkArray<const char*, hkContainerHeapAllocator>& bitsOut,
    int32_t& bitsOver
) const {
    const hkClassEnum* pThis = this;
    bitsOut.m_size = 0;
    int32_t apArray_1 = pThis->m_numItems - 1;
    const hkClassEnum* this_1 = pThis;
    for (int32_t apArray = apArray_1; apArray_1 >= 0; apArray = --apArray_1) {
        if (!flagValue) {
            break;
        }
        const hkClassEnum::Item* m_items = pThis->m_items;
        int32_t m_value = m_items[apArray_1].m_value;
        const hkClassEnum::Item* v9 = &m_items[apArray_1];
        if ((flagValue & m_value) == m_value) {
            const char* m_name = v9->m_name;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (bitsOut.m_size == (bitsOut.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &bitsOut, sizeof(const char*));
            }
            apArray_1 = apArray;
            pThis = this_1;
            bitsOut.m_data[bitsOut.m_size++] = m_name;
            flagValue &= ~m_value;
        }
    }
    bitsOver = flagValue;
    return static_cast<hkResult>(flagValue != 0);
}
