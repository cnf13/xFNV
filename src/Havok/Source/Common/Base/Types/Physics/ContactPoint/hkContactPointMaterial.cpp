#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkContactPointMaterial(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkContactPointMaterialTypeInfo = {
    "hkContactPointMaterial",
    nullptr,
    cleanupLoadedObjecthkContactPointMaterial,
    nullptr,
    &typeid(hkContactPointMaterial),
    sizeof(hkContactPointMaterial)
};

// The reflection's enum records: the names of FlagEnum's values, and the enum.
using hkContactPointMaterialFlagEnumEnumItemsArray = const hkClassEnum::Item[4];
static hkContactPointMaterialFlagEnumEnumItemsArray hkContactPointMaterialFlagEnumEnumItems{
    {hkContactPointMaterial::CONTACT_IS_NEW, "CONTACT_IS_NEW"},
    {hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2, "CONTACT_USES_SOLVER_PATH2"},
    {hkContactPointMaterial::CONTACT_BREAKOFF_OBJECT_ID, "CONTACT_BREAKOFF_OBJECT_ID"},
    {hkContactPointMaterial::CONTACT_IS_DISABLED, "CONTACT_IS_DISABLED"}
};
using hkContactPointMaterialEnumsArray = const hkClassEnum[1];
static hkContactPointMaterialEnumsArray hkContactPointMaterialEnums{
    {"FlagEnum", hkContactPointMaterialFlagEnumEnumItems, 4, nullptr, {0}}
};
hkContactPointMaterial::MembersArray hkContactPointMaterial::Members{
    {"userData",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ULONG},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkContactPointMaterial, m_userData),
     nullptr},
    {"friction",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkContactPointMaterial, m_friction),
     nullptr},
    {"restitution",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkContactPointMaterial, m_restitution),
     nullptr},
    {"maxImpulse",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkContactPointMaterial, m_maxImpulse),
     nullptr},
    {"flags",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkContactPointMaterial, m_flags),
     nullptr}
};
hkClass hkContactPointMaterialClass(
    "hkContactPointMaterial",
    nullptr,
    sizeof(hkContactPointMaterial),
    nullptr,
    0,
    hkContactPointMaterialEnums,
    1,
    hkContactPointMaterial::Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkContactPointMaterial(void* p) {
    ;
}

// The reflection data of hkContactPointMaterial.
