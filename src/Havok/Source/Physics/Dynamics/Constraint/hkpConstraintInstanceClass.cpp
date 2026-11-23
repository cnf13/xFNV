#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"

static void cleanupLoadedObjecthkpConstraintInstanceSmallArraySerializeOverrideType(void* p);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
static const void* getVtablehkpConstraintInstance();
static void finishLoadedObjecthkpConstraintInstance(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpConstraintInstanceSmallArraySerializeOverrideTypeTypeInfo = {
    "hkpConstraintInstanceSmallArraySerializeOverrideType",
    nullptr,
    cleanupLoadedObjecthkpConstraintInstanceSmallArraySerializeOverrideType,
    nullptr,
    &typeid(hkpConstraintInstance::SmallArraySerializeOverrideType),
    sizeof(hkpConstraintInstance::SmallArraySerializeOverrideType)
};

// The reflected enums of hkpConstraintInstance.
using hkpConstraintInstanceConstraintPriorityEnumItemsArray = const hkClassEnum::Item[6];
static hkpConstraintInstanceConstraintPriorityEnumItemsArray hkpConstraintInstanceConstraintPriorityEnumItems{
    {hkpConstraintInstance::PRIORITY_INVALID, "PRIORITY_INVALID"},
    {hkpConstraintInstance::PRIORITY_PSI, "PRIORITY_PSI"},
    {hkpConstraintInstance::PRIORITY_SIMPLIFIED_TOI_UNUSED, "PRIORITY_SIMPLIFIED_TOI_UNUSED"},
    {hkpConstraintInstance::PRIORITY_TOI, "PRIORITY_TOI"},
    {hkpConstraintInstance::PRIORITY_TOI_HIGHER, "PRIORITY_TOI_HIGHER"},
    {hkpConstraintInstance::PRIORITY_TOI_FORCED, "PRIORITY_TOI_FORCED"}
};
using hkpConstraintInstanceInstanceTypeEnumItemsArray = const hkClassEnum::Item[2];
static hkpConstraintInstanceInstanceTypeEnumItemsArray hkpConstraintInstanceInstanceTypeEnumItems{
    {hkpConstraintInstance::TYPE_NORMAL, "TYPE_NORMAL"},
    {hkpConstraintInstance::TYPE_CHAIN, "TYPE_CHAIN"}
};
using hkpConstraintInstanceAddReferencesEnumItemsArray = const hkClassEnum::Item[2];
static hkpConstraintInstanceAddReferencesEnumItemsArray hkpConstraintInstanceAddReferencesEnumItems{
    {hkpConstraintInstance::DO_NOT_ADD_REFERENCES, "DO_NOT_ADD_REFERENCES"},
    {hkpConstraintInstance::DO_ADD_REFERENCES, "DO_ADD_REFERENCES"}
};
using hkpConstraintInstanceCloningModeEnumItemsArray = const hkClassEnum::Item[3];
static hkpConstraintInstanceCloningModeEnumItemsArray hkpConstraintInstanceCloningModeEnumItems{
    {hkpConstraintInstance::CLONE_INSTANCES_ONLY, "CLONE_INSTANCES_ONLY"},
    {hkpConstraintInstance::CLONE_DATAS_WITH_MOTORS, "CLONE_DATAS_WITH_MOTORS"},
    {hkpConstraintInstance::CLONE_FORCE_SHALLOW, "CLONE_FORCE_SHALLOW"}
};
using hkpConstraintInstanceOnDestructionRemapInfoEnumItemsArray = const hkClassEnum::Item[3];
static hkpConstraintInstanceOnDestructionRemapInfoEnumItemsArray hkpConstraintInstanceOnDestructionRemapInfoEnumItems{
    {hkpConstraintInstance::ON_DESTRUCTION_REMAP, "ON_DESTRUCTION_REMAP"},
    {hkpConstraintInstance::ON_DESTRUCTION_REMOVE, "ON_DESTRUCTION_REMOVE"},
    {hkpConstraintInstance::ON_DESTRUCTION_RESET_REMOVE, "ON_DESTRUCTION_RESET_REMOVE"}
};
using hkpConstraintInstanceEnumsArray = const hkClassEnum[5];
static hkpConstraintInstanceEnumsArray hkpConstraintInstanceEnums{
    {"ConstraintPriority", hkpConstraintInstanceConstraintPriorityEnumItems, 6, nullptr, {0}},
    {"InstanceType", hkpConstraintInstanceInstanceTypeEnumItems, 2, nullptr, {0}},
    {"AddReferences", hkpConstraintInstanceAddReferencesEnumItems, 2, nullptr, {0}},
    {"CloningMode", hkpConstraintInstanceCloningModeEnumItems, 3, nullptr, {0}},
    {"OnDestructionRemapInfo", hkpConstraintInstanceOnDestructionRemapInfoEnumItems, 3, nullptr, {0}}
};

// The reflected members of hkpConstraintInstance::SmallArraySerializeOverrideType.
using hkpConstraintInstanceSmallArraySerializeOverrideTypeClass_MembersArray = const hkClassMember[3];
static hkpConstraintInstanceSmallArraySerializeOverrideTypeClass_MembersArray
    hkpConstraintInstanceSmallArraySerializeOverrideTypeClass_Members{
        {"data", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x0, nullptr},
        {"size", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
        {"capacityAndFlags",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_UINT16},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x6,
         nullptr}
};
hkClass hkpConstraintInstanceSmallArraySerializeOverrideTypeClass(
    "hkpConstraintInstanceSmallArraySerializeOverrideType",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkpConstraintInstanceSmallArraySerializeOverrideTypeClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    1
);

static const hkClassEnum* hkpConstraintInstanceConstraintPriorityEnum = &hkpConstraintInstanceEnums[0];
static const hkClassEnum* hkpConstraintInstanceOnDestructionRemapInfoEnum = &hkpConstraintInstanceEnums[4];

// The class record's member table: the priority and destructionRemapInfo members name their enums by the pointers
// above, so the array's dynamic initializer stores them.
hkpConstraintInstance::MembersArray hkpConstraintInstance::Members{
    {"owner", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x8, nullptr},
    {"data",
     &hkpConstraintDataClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0xC,
     nullptr},
    {"constraintModifiers",
     &hkpModifierConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"entities",
     &hkpEntityClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     2,
     {0},
     0x14,
     nullptr},
    {"priority",
     nullptr,
     hkpConstraintInstanceConstraintPriorityEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x1C,
     nullptr},
    {"wantRuntime", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1D, nullptr},
    {"destructionRemapInfo",
     nullptr,
     hkpConstraintInstanceOnDestructionRemapInfoEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x1E,
     nullptr},
    {"listeners",
     &hkpConstraintInstanceSmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0x400},
     0x20,
     nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x28, nullptr},
    {"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2C, nullptr},
    {"internal",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {0x400},
     0x30,
     nullptr},
    {"uid", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x34, nullptr}
};
hkClass hkpConstraintInstanceClass(
    "hkpConstraintInstance",
    &hkReferencedObjectClass,
    56,
    nullptr,
    0,
    hkpConstraintInstanceEnums,
    5,
    hkpConstraintInstance::Members,
    12,
    nullptr,
    nullptr,
    0,
    1
);

static void cleanupLoadedObjecthkpConstraintInstanceSmallArraySerializeOverrideType(void* p) {
    ;
}

static const void* getVtablehkpConstraintInstance() {
    return hkGetVtableOf<hkpConstraintInstance>();
}

hkTypeInfo hkpConstraintInstanceTypeInfo = {
    "hkpConstraintInstance",
    finishLoadedObjecthkpConstraintInstance,
    cleanupLoadedObjecthkpConstraintInstance,
    getVtablehkpConstraintInstance(),
    &typeid(hkpConstraintInstance),
    sizeof(hkpConstraintInstance)
};

// The finish function the reflection uses for hkpConstraintInstance: constructs the loaded instance in place with its
// finish constructor.
static void finishLoadedObjecthkpConstraintInstance(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpConstraintInstance(flag);
    }
}

void cleanupLoadedObjecthkpConstraintInstance(void* p) {
    static_cast<hkpConstraintInstance*>(p)->~hkpConstraintInstance();
}

const hkClass& hkpConstraintInstance::SmallArraySerializeOverrideType::staticClass() {
    return hkpConstraintInstanceSmallArraySerializeOverrideTypeClass;
}

const hkClass& hkpConstraintInstance::staticClass() {
    return hkpConstraintInstanceClass;
}
