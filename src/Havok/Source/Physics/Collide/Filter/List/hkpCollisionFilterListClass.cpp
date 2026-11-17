#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Filter/List/hkpCollisionFilterList.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/CompressedSampledHeightField/hkpCompressedSampledHeightFieldShape.h"

hkpCollisionFilterList::MembersArray hkpCollisionFilterList::Members{
    {"collisionFilters",
     &hkpCollisionFilterClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x30,
     nullptr}
};
hkClass hkpCollisionFilterListClass(
    "hkpCollisionFilterList",
    &hkpCollisionFilterClass,
    60,
    nullptr,
    0,
    nullptr,
    0,
    hkpCollisionFilterList::Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpCollisionFilterListTypeInfo = {"hkpCollisionFilterList",
                                             finishLoadedObjecthkpCollisionFilterList,
                                             cleanupLoadedObjecthkpCollisionFilterList,
                                             getVtablehkpCollisionFilterList(),
                                             &typeid(hkpCollisionFilterList),
                                             sizeof(hkpCollisionFilterList)};

const void* getVtablehkpCollisionFilterList() {
    return hkGetVtableOf<hkpCollisionFilterList>();
}

const hkClass& hkpCollisionFilterList::staticClass() {
    return hkpCollisionFilterListClass;
}

void cleanupLoadedObjecthkpCollisionFilterList(void* p) {
    static_cast<hkpCollisionFilterList*>(p)->~hkpCollisionFilterList();
}
