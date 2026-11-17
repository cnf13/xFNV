#include "Havok/Source/Common/Serialize/Util/hkRootLevelContainer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"

// Havok SDK signature (hkSerialize.lib, tied by target).
static void finishLoadedObjecthkRootLevelContainerNamedVariant(void* a1, int32_t a2);
static void finishLoadedObjecthkRootLevelContainer(void* a1, int32_t a2);
// Havok SDK signature (hkSerialize.lib, tied by bytes).
static void cleanupLoadedObjecthkRootLevelContainerNamedVariant(void* a1);
// Havok SDK signature (hkSerialize.lib, tied by bytes).
static void cleanupLoadedObjecthkRootLevelContainer(void* a1);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkRootLevelContainerNamedVariantTypeInfo = {
    "hkRootLevelContainerNamedVariant",
    finishLoadedObjecthkRootLevelContainerNamedVariant,
    cleanupLoadedObjecthkRootLevelContainerNamedVariant,
    nullptr,
    &typeid(hkRootLevelContainer::NamedVariant),
    sizeof(hkRootLevelContainer::NamedVariant)
};
hkTypeInfo hkRootLevelContainerTypeInfo = {
    "hkRootLevelContainer",
    finishLoadedObjecthkRootLevelContainer,
    cleanupLoadedObjecthkRootLevelContainer,
    nullptr,
    &typeid(hkRootLevelContainer),
    sizeof(hkRootLevelContainer)
};
hkRootLevelContainer::NamedVariant::MembersArray hkRootLevelContainer::NamedVariant::Members{
    {"name",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkRootLevelContainer::NamedVariant, m_name),
     nullptr},
    {"className",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkRootLevelContainer::NamedVariant, m_className),
     nullptr},
    {"variant",
     &hkReferencedObjectClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkRootLevelContainer::NamedVariant, m_variant),
     nullptr}
};
hkClass hkRootLevelContainerNamedVariantClass(
    "hkRootLevelContainerNamedVariant",
    nullptr,
    sizeof(hkRootLevelContainer::NamedVariant),
    nullptr,
    0,
    nullptr,
    0,
    hkRootLevelContainer::NamedVariant::Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);

// The reflected members of hkRootLevelContainer.
using hkRootLevelContainerClass_MembersArray = const hkClassMember[1];
static hkRootLevelContainerClass_MembersArray hkRootLevelContainerClass_Members{
    {"namedVariants",
     &hkRootLevelContainerNamedVariantClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkRootLevelContainer, m_namedVariants),
     nullptr}
};
hkClass hkRootLevelContainerClass(
    "hkRootLevelContainer",
    nullptr,
    sizeof(hkRootLevelContainer),
    nullptr,
    0,
    nullptr,
    0,
    hkRootLevelContainerClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

// The type info's finish callback: NamedVariant's finish constructor on a1.
static void finishLoadedObjecthkRootLevelContainerNamedVariant(void* a1, int32_t a2) {
    if (a1) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = a2;
        ::new (a1) hkRootLevelContainer::NamedVariant(kFlag);
    }
}

// The type info's finish callback: the container's finish constructor on a1.
static void finishLoadedObjecthkRootLevelContainer(void* a1, int32_t a2) {
    if (a1) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = a2;
        ::new (a1) hkRootLevelContainer(kFlag);
    }
}

// The type info's cleanup callback: runs NamedVariant's destructor on a1: m_variant releases its object and clears it,
// then m_className's and m_name's destructors run.
static void cleanupLoadedObjecthkRootLevelContainerNamedVariant(void* a1) {
    hkRootLevelContainer::NamedVariant* pVariant = static_cast<hkRootLevelContainer::NamedVariant*>(a1);
    hkReferencedObject* pObject = pVariant->m_variant.m_pntr;
    if (pObject) {
        pObject->removeReference();
    }
    pVariant->m_variant.m_pntr = nullptr;
    pVariant->m_className.~hkStringPtr();
    pVariant->m_name.~hkStringPtr();
}

static void cleanupLoadedObjecthkRootLevelContainer(void* a1) {
    static_cast<hkRootLevelContainer*>(a1)->~hkRootLevelContainer();
}

hkRootLevelContainer::hkRootLevelContainer(hkFinishLoadedObjectFlag flag) : m_namedVariants(flag) {}

// Skips past prevObject first, then compares each entry's class name; entries without one are passed over.
void* hkRootLevelContainer::findObjectByType(const char* typeName, const void* prevObject) const {
    int32_t index = 0;
    while (prevObject && index < m_namedVariants.m_size && m_namedVariants.m_data[index++].getObject() != prevObject) {}
    for (int32_t i = index; i < m_namedVariants.m_size; ++i) {
        if (m_namedVariants.m_data[i].getTypeName() &&
            hkString::strCmp(typeName, m_namedVariants.m_data[i].getTypeName()) == 0) {
            return m_namedVariants.m_data[i].getObject();
        }
    }
    return nullptr;
}

void* hkRootLevelContainer::findObjectByName(const char* objectName, const void* prevObject) const {
    hkRootLevelContainer::NamedVariant* v6;
    int32_t v4 = 0;
    int32_t v5 = 0;
    do {
        if (!prevObject) {
            break;
        }
        if (v4 >= m_namedVariants.m_size) {
            break;
        }
        v6 = &m_namedVariants.m_data[v5];
        ++v4;
        ++v5;
    }
    while (v6->m_variant.m_pntr != prevObject);
    int32_t v7 = v4;
    if (v4 >= m_namedVariants.m_size) {
        return nullptr;
    }
    for (int32_t i = v4;; ++i) {
        const char* m_stringAndFlag = m_namedVariants.m_data[i].m_name.m_stringAndFlag;
        if ((reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)) != 0 &&
            !hkString::strCmp(
                objectName, reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1))
            )) {
            break;
        }
        if (++v7 >= m_namedVariants.m_size) {
            return nullptr;
        }
    }
    return m_namedVariants.m_data[v7].m_variant.m_pntr;
}

void hkRootLevelContainer::operator delete(void* apMemory, size_t aSize) {
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
}

const hkClass& hkRootLevelContainer::staticClass() {
    return hkRootLevelContainerClass;
}
