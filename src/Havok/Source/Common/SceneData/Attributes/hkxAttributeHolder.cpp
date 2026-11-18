#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
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
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

static void finishLoadedObjecthkxAttributeHolder(void* p, int32_t finishing);
static void cleanupLoadedObjecthkxAttributeHolder(void* p);

// The class record's member table.
using hkxAttributeHolderClassMemberArray = const hkClassMember[1];
static hkxAttributeHolderClassMemberArray hkxAttributeHolderClass_Members{
    {"attributeGroups",
     &hkxAttributeGroupClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x8,
     nullptr}
};
hkClass hkxAttributeHolderClass(
    "hkxAttributeHolder",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkxAttributeHolderClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxAttributeHolderTypeInfo = {
    "hkxAttributeHolder",          finishLoadedObjecthkxAttributeHolder, cleanupLoadedObjecthkxAttributeHolder,
    getVtablehkxAttributeHolder(), &typeid(hkxAttributeHolder),          sizeof(hkxAttributeHolder)
};

// The type info's finish-loaded-object callback for hkxAttributeHolder: runs its finish constructor when the object is
// not null.
static void finishLoadedObjecthkxAttributeHolder(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxAttributeHolder(flag);
    }
}

static void cleanupLoadedObjecthkxAttributeHolder(void* p) {
    static_cast<hkxAttributeHolder*>(p)->~hkxAttributeHolder();
}

const void* getVtablehkxAttributeHolder() {
    return hkGetVtableOf<hkxAttributeHolder>();
}

// Starts empty, then takes a copy of each of other's groups.
hkxAttributeHolder::hkxAttributeHolder(const hkxAttributeHolder& other) : hkReferencedObject(), m_attributeGroups() {
    const int32_t iCount = other.m_attributeGroups.m_size;
    m_attributeGroups.setSize(iCount);
    for (int32_t i = 0; i < iCount; ++i) {
        m_attributeGroups.begin()[i] = other.m_attributeGroups.begin()[i];
    }
}

// The member destroys the groups and frees their storage.
hkxAttributeHolder::~hkxAttributeHolder() {}

const hkxAttributeGroup* hkxAttributeHolder::findAttributeGroupByName(const char* name) const {
    hkxAttributeGroup* v8;
    if (!name) {
        return nullptr;
    }
    int32_t v4 = 0;
    if (m_attributeGroups.m_size <= 0) {
        return nullptr;
    }
    for (int32_t i = 0;; ++i) {
        hkxAttributeGroup* m_data = m_attributeGroups.m_data;
        const char* m_stringAndFlag = m_data[i].m_name.m_stringAndFlag;
        v8 = &m_data[i];
        if ((reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)) != 0 &&
            !hkString::strCasecmp(
                reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)), name
            )) {
            break;
        }
        if (++v4 >= m_attributeGroups.m_size) {
            return nullptr;
        }
    }
    return v8;
}

// The object of the first attribute called apName in any group, when apType is null or its class has apType's name;
// else null. The variant's reference is released before the object is returned.
hkReferencedObject* hkxAttributeHolder::findAttributeObjectByName(const char* name, const hkClass* type) const {
    if (!name) {
        return nullptr;
    }
    XFNV_INLINED_LOCAL(hkRefVariant, variant, std::nullopt, std::nullopt);
    findAttributeVariantByName(&variant, name);
    if (!variant.m_pntr) {
        return nullptr;
    }
    if (type) {
        const char* pcVariantClassName = variant.getClass()->getName();
        if (hkString::strCasecmp(type->getName(), pcVariantClassName) != 0) {
            // EXE-INLINED 0xC6CCA0
            if (variant.m_pntr) {
                variant.m_pntr->removeReference();
            }
            return nullptr;
        }
    }
    hkReferencedObject* pObject = variant.m_pntr;
    // EXE-INLINED 0xC6CCA0
    if (pObject) {
        pObject->removeReference();
    }
    return pObject;
}

hkRefVariant* hkxAttributeHolder::findAttributeVariantByName(hkRefVariant* apResult, const char* name) const {
    int32_t v4;
    hkRefVariant* v7;
    // The variant's reference is released by hand below.
    XFNV_INLINED_LOCAL(hkRefVariant, apResulta, std::nullopt, std::nullopt);
    if (!name || (v4 = 0, m_attributeGroups.m_size <= 0)) {
        hkVariant v11;
    LABEL_6:;
        v7 = apResult;
        v11.m_object = nullptr;
        v11.m_class = nullptr;
        apResult->assignVariant(v11);
        return v7;
    }
    int32_t v5 = 0;
    while (true) {
        m_attributeGroups.m_data[v5].findAttributeVariantByName(&apResulta, name);
        if (apResulta.m_pntr) {
            break;
        }
        ++v4;
        ++v5;
        bool v6 = v4 < m_attributeGroups.m_size;
        apResulta.m_pntr = nullptr;
        if (!v6) {
            goto LABEL_6;
        }
    }
    apResulta.m_pntr->addReference();
    hkReferencedObject* m_pntr = apResulta.m_pntr;
    v7 = apResult;
    apResult->m_pntr = apResulta.m_pntr;
    if (!m_pntr) {
        return v7;
    }
    m_pntr->removeReference();
    return apResult;
}

const hkClass& hkxAttributeHolder::staticClass() {
    return hkxAttributeHolderClass;
}
