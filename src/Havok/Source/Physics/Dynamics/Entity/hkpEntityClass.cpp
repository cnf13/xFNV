#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

static void cleanupLoadedObjecthkpEntitySmallArraySerializeOverrideType(void* p);
static void cleanupLoadedObjecthkpEntitySpuCollisionCallback(void* p);
static void finishLoadedObjecthkpEntitySpuCollisionCallback(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpEntitySmallArraySerializeOverrideTypeTypeInfo = {
    "hkpEntitySmallArraySerializeOverrideType",
    nullptr,
    cleanupLoadedObjecthkpEntitySmallArraySerializeOverrideType,
    nullptr,
    &typeid(hkpEntity::SmallArraySerializeOverrideType),
    sizeof(hkpEntity::SmallArraySerializeOverrideType)
};
hkTypeInfo hkpEntitySpuCollisionCallbackTypeInfo = {
    "hkpEntitySpuCollisionCallback",
    finishLoadedObjecthkpEntitySpuCollisionCallback,
    cleanupLoadedObjecthkpEntitySpuCollisionCallback,
    nullptr,
    &typeid(hkpEntity::SpuCollisionCallback),
    sizeof(hkpEntity::SpuCollisionCallback)
};
hkTypeInfo hkpEntityExtendedListenersTypeInfo = {
    "hkpEntityExtendedListeners",
    nullptr,
    cleanupLoadedObjecthkpEntityExtendedListeners,
    nullptr,
    &typeid(hkpEntity::ExtendedListeners),
    sizeof(hkpEntity::ExtendedListeners)
};

// The reflection data of hkpEntity's class records.
using hkpEntitySmallArraySerializeOverrideTypeClass_MembersArray = const hkClassMember[3];
static hkpEntitySmallArraySerializeOverrideTypeClass_MembersArray hkpEntitySmallArraySerializeOverrideTypeClass_Members{
    {"data",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x0,
     nullptr},
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
using hkpEntitySpuCollisionCallbackClass_MembersArray = const hkClassMember[4];
static hkpEntitySpuCollisionCallbackClass_MembersArray hkpEntitySpuCollisionCallbackClass_Members{
    {"util",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x0,
     nullptr},
    {"capacity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x4,
     nullptr},
    {"eventFilter", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x6, nullptr},
    {"userFilter", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x7, nullptr}
};
using hkpEntityExtendedListenersClass_MembersArray = const hkClassMember[2];
static hkpEntityExtendedListenersClass_MembersArray hkpEntityExtendedListenersClass_Members{
    {"activationListeners",
     &hkpEntitySmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x0,
     nullptr},
    {"entityListeners",
     &hkpEntitySmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x8,
     nullptr}
};
hkClass hkpEntitySmallArraySerializeOverrideTypeClass(
    "hkpEntitySmallArraySerializeOverrideType",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkpEntitySmallArraySerializeOverrideTypeClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    1
);
hkClass hkpEntitySpuCollisionCallbackClass(
    "hkpEntitySpuCollisionCallback",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkpEntitySpuCollisionCallbackClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpEntityExtendedListenersClass(
    "hkpEntityExtendedListeners",
    nullptr,
    16,
    nullptr,
    0,
    nullptr,
    0,
    hkpEntityExtendedListenersClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
using hkpEntitySpuCollisionCallbackEventFilterEnumItemsArray = const hkClassEnum::Item[5];
static hkpEntitySpuCollisionCallbackEventFilterEnumItemsArray hkpEntitySpuCollisionCallbackEventFilterEnumItems{
    {0, "SPU_SEND_NONE"},
    {1, "SPU_SEND_CONTACT_POINT_ADDED"},
    {2, "SPU_SEND_CONTACT_POINT_PROCESS"},
    {4, "SPU_SEND_CONTACT_POINT_REMOVED"},
    {3, "SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS"}
};
using hkpEntityEnumsArray = const hkClassEnum[1];
static hkpEntityEnumsArray hkpEntityEnums{
    {"SpuCollisionCallbackEventFilter", hkpEntitySpuCollisionCallbackEventFilterEnumItems, 5, nullptr, {0}}
};

// The motion member's custom attribute: hk.DataObjectType "hkpMotion". The SDK's m_typeName is a char*, and
// hkVariant's object pointer is not const.
static hkDataObjectTypeAttribute hkpEntity_MotionDataObjectType{const_cast<char*>("hkpMotion")};
using hkpEntity_MotionAttributeArray = const hkCustomAttributes::Attribute[1];
static hkpEntity_MotionAttributeArray hkpEntity_MotionAttributeList{
    {"hk.DataObjectType", {&hkpEntity_MotionDataObjectType, &hkDataObjectTypeAttributeClass}}
};
static const hkCustomAttributes hkpEntity_MotionAttributes{hkpEntity_MotionAttributeList, 1};
using hkpEntityClass_MembersArray = const hkClassMember[21];
static hkpEntityClass_MembersArray hkpEntityClass_Members{
    {"material",
     &hkpMaterialClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8C,
     nullptr},
    {"limitContactImpulseUtil",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x98,
     nullptr},
    {"damageMultiplier",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x9C,
     nullptr},
    {"breakableBody",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xA0,
     nullptr},
    {"solverData",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xA4,
     nullptr},
    {"storageIndex", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xA8, nullptr},
    {"contactPointCallbackDelay",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xAA,
     nullptr},
    {"constraintsMaster",
     &hkpEntitySmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xAC,
     nullptr},
    {"constraintsSlave",
     &hkpConstraintInstanceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::NOT_OWNED | hkClassMember::SERIALIZE_IGNORED},
     0xB4,
     nullptr},
    {"constraintRuntime",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT8},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC0,
     nullptr},
    {"simulationIsland",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xCC,
     nullptr},
    {"autoRemoveLevel",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD0,
     nullptr},
    {"numShapeKeysInContactPointProperties",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD1,
     nullptr},
    {"responseModifierFlags",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD2,
     nullptr},
    {"uid", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xD4, nullptr},
    {"spuCollisionCallback",
     &hkpEntitySpuCollisionCallbackClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD8,
     nullptr},
    {"motion",
     &hkpMaxSizeMotionClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE0,
     &hkpEntity_MotionAttributes},
    {"contactListeners",
     &hkpEntitySmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x200,
     nullptr},
    {"actions",
     &hkpEntitySmallArraySerializeOverrideTypeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x208,
     nullptr},
    {"localFrame",
     &hkLocalFrameClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x210,
     nullptr},
    {"extendedListeners",
     &hkpEntityExtendedListenersClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x214,
     nullptr}
};

static void cleanupLoadedObjecthkpEntitySmallArraySerializeOverrideType(void* p) {
    ;
}

static void cleanupLoadedObjecthkpEntitySpuCollisionCallback(void* p) {
    ;
}

static void finishLoadedObjecthkpEntitySpuCollisionCallback(void* p, int32_t finishing) {
    ;
}

namespace {
    // Per member its offset into this struct (-1: no default), then the defaults (damageMultiplier 1.0, uid
    // 0xFFFFFFFF).
    struct hkpEntity_DefaultStruct {
        int32_t s_defaultOffsets[21];
        float m_damageMultiplier;
        uint32_t m_uid;
    };
} // namespace
// The defaults the class record names.
static const hkpEntity_DefaultStruct hkpEntity_Default{
    {-1, -1, 0x54, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x58, -1, -1, -1, -1, -1, -1},
    1.0f,
    0xFFFFFFFFu
};

hkClass hkpEntityClass(
    "hkpEntity",
    &hkpWorldObjectClass,
    544,
    nullptr,
    0,
    hkpEntityEnums,
    1,
    hkpEntityClass_Members,
    21,
    &hkpEntity_Default,
    nullptr,
    0,
    2
);
hkTypeInfo hkpEntityTypeInfo = {
    "hkpEntity",
    finishLoadedObjecthkpEntity,
    cleanupLoadedObjecthkpEntity,
    getVtablehkpEntity(),
    &typeid(hkpEntity),
    sizeof(hkpEntity)
};

// The reflection's getVtable for hkpEntity: runs its finish constructor (flag 0) on a 16-aligned stack object and
// returns the vtable that constructor stores.
const void* getVtablehkpEntity() {
    return hkGetVtableOf<hkpEntity>();
}

// The type info's finish-loaded-object callback for hkpEntity: the null check, then the entity's finish constructor.
void finishLoadedObjecthkpEntity(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpEntity(flag);
    }
}

// The type info's cleanup callback of hkpEntity::ExtendedListeners: its destructor, by a tail jump.
void cleanupLoadedObjecthkpEntityExtendedListeners(void* p) {
    static_cast<hkpEntity::ExtendedListeners*>(p)->~ExtendedListeners();
}

void cleanupLoadedObjecthkpEntity(void* p) {
    static_cast<hkpEntity*>(p)->~hkpEntity();
}

hkClass* getClassType() {
    return &hkpEntityClass;
}

const hkClass& hkpEntity::ExtendedListeners::staticClass() {
    return hkpEntityExtendedListenersClass;
}

const hkClass& hkpEntity::SpuCollisionCallback::staticClass() {
    return hkpEntitySpuCollisionCallbackClass;
}

const hkClass& hkpEntity::SmallArraySerializeOverrideType::staticClass() {
    return hkpEntitySmallArraySerializeOverrideTypeClass;
}
