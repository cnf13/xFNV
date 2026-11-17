#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/ResourceDatabase/hkResourceHandle.h"

// Havok SDK signature (hkSerialize.lib, tied by reloc,target).
static const void* getVtablehkMemoryResourceContainer();
static void cleanupLoadedObjecthkMemoryResourceContainer(void* p);

// The reflection's enum records: the names of hkResourceBase::Type's values, and the enum.
using hkResourceBaseTypeEnumItemsArray = const hkClassEnum::Item[2];
static hkResourceBaseTypeEnumItemsArray hkResourceBaseTypeEnumItems{
    {hkResourceBase::TYPE_RESOURCE, "TYPE_RESOURCE"},
    {hkResourceBase::TYPE_CONTAINER, "TYPE_CONTAINER"}
};
using hkResourceBaseEnumsArray = const hkClassEnum[1];
static hkResourceBaseEnumsArray hkResourceBaseEnums{{"Type", hkResourceBaseTypeEnumItems, 2, nullptr, {0}}};
hkClass hkResourceBaseClass(
    "hkResourceBase",
    &hkReferencedObjectClass,
    8,
    nullptr,
    0,
    hkResourceBaseEnums,
    1,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkResourceHandleClass(
    "hkResourceHandle",
    &hkResourceBaseClass,
    8,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkResourceContainerClass(
    "hkResourceContainer",
    &hkResourceBaseClass,
    8,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);

// The reflection's member table of ExternalLink.
using hkMemoryResourceHandle_ExternalLinkClass_MembersArray = const hkClassMember[2];
static hkMemoryResourceHandle_ExternalLinkClass_MembersArray hkMemoryResourceHandle_ExternalLinkClass_Members{
    {"memberName", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"externalId", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}
};
hkClass hkMemoryResourceHandleExternalLinkClass(
    "hkMemoryResourceHandleExternalLink",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkMemoryResourceHandle_ExternalLinkClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkMemoryResourceHandle::MembersArray hkMemoryResourceHandle::Members{
    {"variant",
     &hkReferencedObjectClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x8,
     nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"references",
     &hkMemoryResourceHandleExternalLinkClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr}
};
hkClass hkMemoryResourceHandleClass(
    "hkMemoryResourceHandle",
    &hkResourceHandleClass,
    0x1C,
    nullptr,
    0,
    nullptr,
    0,
    hkMemoryResourceHandle::Members,
    3,
    nullptr,
    nullptr,
    0,
    2
);
hkTypeInfo hkMemoryResourceHandleTypeInfo{"hkMemoryResourceHandle",
                                          finishLoadedObjecthkMemoryResourceHandle,
                                          cleanupLoadedObjecthkMemoryResourceHandle,
                                          getVtablehkMemoryResourceHandle(),
                                          &typeid(hkMemoryResourceHandle),
                                          sizeof(hkMemoryResourceHandle)};
hkMemoryResourceContainer::MembersArray hkMemoryResourceContainer::Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"parent",
     &hkMemoryResourceContainerClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC,
     nullptr},
    {"resourceHandles",
     &hkMemoryResourceHandleClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x10,
     nullptr},
    {"children",
     &hkMemoryResourceContainerClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x1C,
     nullptr}
};
hkClass hkMemoryResourceContainerClass(
    "hkMemoryResourceContainer",
    &hkResourceContainerClass,
    0x28,
    nullptr,
    0,
    nullptr,
    0,
    hkMemoryResourceContainer::Members,
    4,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkMemoryResourceContainerTypeInfo{
    "hkMemoryResourceContainer",
    finishLoadedObjecthkMemoryResourceContainer,
    cleanupLoadedObjecthkMemoryResourceContainer,
    getVtablehkMemoryResourceContainer(),
    &typeid(hkMemoryResourceContainer),
    sizeof(hkMemoryResourceContainer)
};

static const void* getVtablehkMemoryResourceContainer() {
    return hkGetVtableOf<hkMemoryResourceContainer>();
}

static void cleanupLoadedObjecthkMemoryResourceContainer(void* p) {
    static_cast<hkMemoryResourceContainer*>(p)->~hkMemoryResourceContainer();
}

void cleanupLoadedObjecthkMemoryResourceHandleExternalLink(void* p) {
    hkStringPtr* a1 = static_cast<hkStringPtr*>(p);
    (a1 + 1)->~hkStringPtr();
    a1->~hkStringPtr();
}

const hkClass& hkMemoryResourceHandle::staticClass() {
    return hkMemoryResourceHandleClass;
}

const hkClass& hkMemoryResourceContainer::staticClass() {
    return hkMemoryResourceContainerClass;
}
