#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkpMeshMaterial(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMeshMaterialTypeInfo = {
    "hkpMeshMaterial",
    nullptr,
    cleanupLoadedObjecthkpMeshMaterial,
    nullptr,
    &typeid(hkpMeshMaterial),
    sizeof(hkpMeshMaterial)
};

// The reflection's member table.
using hkpMeshMaterialClass_MembersArray = const hkClassMember[1];
static hkpMeshMaterialClass_MembersArray hkpMeshMaterialClass_Members{
    {"filterInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr}
};
hkClass hkpMeshMaterialClass(
    "hkpMeshMaterial",
    nullptr,
    4,
    nullptr,
    0,
    nullptr,
    0,
    hkpMeshMaterialClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkpMeshMaterial(void* p) {
    ;
}

const hkClass& hkpMeshMaterial::staticClass() {
    return hkpMeshMaterialClass;
}
