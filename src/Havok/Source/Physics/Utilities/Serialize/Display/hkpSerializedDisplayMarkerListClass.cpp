#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarkerList.h"

// The reflected members of hkpSerializedDisplayMarkerList.
using hkpSerializedDisplayMarkerListClass_MembersArray = const hkClassMember[1];
static hkpSerializedDisplayMarkerListClass_MembersArray hkpSerializedDisplayMarkerListClass_Members{
    {"markers",
     &hkpSerializedDisplayMarkerClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x8,
     nullptr}
};

hkClass hkpSerializedDisplayMarkerListClass(
    "hkpSerializedDisplayMarkerList",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpSerializedDisplayMarkerListClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSerializedDisplayMarkerListTypeInfo = {
    "hkpSerializedDisplayMarkerList",
    finishLoadedObjecthkpSerializedDisplayMarkerList,
    cleanupLoadedObjecthkpSerializedDisplayMarkerList,
    getVtablehkpSerializedDisplayMarkerList(),
    &typeid(hkpSerializedDisplayMarkerList),
    sizeof(hkpSerializedDisplayMarkerList)
};

const void* getVtablehkpSerializedDisplayMarkerList() {
    return hkGetVtableOf<hkpSerializedDisplayMarkerList>();
}

void cleanupLoadedObjecthkpSerializedDisplayMarkerList(void* p) {
    static_cast<hkpSerializedDisplayMarkerList*>(p)->~hkpSerializedDisplayMarkerList();
}
