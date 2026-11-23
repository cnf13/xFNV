#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflection data of hkpVehicleWheelCollide.

// The reflection's enum records: the names of WheelCollideType's values, and the enum, as file-local data.
using hkpVehicleWheelCollideWheelCollideTypeEnumItemsArray = const hkClassEnum::Item[8];
static hkpVehicleWheelCollideWheelCollideTypeEnumItemsArray hkpVehicleWheelCollideWheelCollideTypeEnumItems{
    {hkpVehicleWheelCollide::INVALID_WHEEL_COLLIDE, "INVALID_WHEEL_COLLIDE"},
    {hkpVehicleWheelCollide::RAY_CAST_WHEEL_COLLIDE, "RAY_CAST_WHEEL_COLLIDE"},
    {hkpVehicleWheelCollide::LINEAR_CAST_WHEEL_COLLIDE, "LINEAR_CAST_WHEEL_COLLIDE"},
    {hkpVehicleWheelCollide::USER_WHEEL_COLLIDE1, "USER_WHEEL_COLLIDE1"},
    {hkpVehicleWheelCollide::USER_WHEEL_COLLIDE2, "USER_WHEEL_COLLIDE2"},
    {hkpVehicleWheelCollide::USER_WHEEL_COLLIDE3, "USER_WHEEL_COLLIDE3"},
    {hkpVehicleWheelCollide::USER_WHEEL_COLLIDE4, "USER_WHEEL_COLLIDE4"},
    {hkpVehicleWheelCollide::USER_WHEEL_COLLIDE5, "USER_WHEEL_COLLIDE5"}
};
using hkpVehicleWheelCollideEnumsArray = const hkClassEnum[1];
static hkpVehicleWheelCollideEnumsArray hkpVehicleWheelCollideEnums{
    {"WheelCollideType", hkpVehicleWheelCollideWheelCollideTypeEnumItems, 8, nullptr, {0}}
};

// The reflection's member table. The type member is not serialized and has no enum pointer.
using hkpVehicleWheelCollideClass_MembersArray = const hkClassMember[2];
static hkpVehicleWheelCollideClass_MembersArray hkpVehicleWheelCollideClass_Members{
    {"alreadyUsed", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"type",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x9,
     nullptr}
};

// The class record.
hkClass hkpVehicleWheelCollideClass(
    "hkpVehicleWheelCollide",
    &hkReferencedObjectClass,
    sizeof(hkpVehicleWheelCollide),
    nullptr,
    0,
    hkpVehicleWheelCollideEnums,
    1,
    hkpVehicleWheelCollideClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
