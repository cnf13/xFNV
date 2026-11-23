#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkpVehicleFrictionDescriptionAxisDescription(void* p);
static void cleanupLoadedObjecthkpVehicleFrictionDescription(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleFrictionDescriptionAxisDescriptionTypeInfo = {
    "hkpVehicleFrictionDescriptionAxisDescription",
    nullptr,
    cleanupLoadedObjecthkpVehicleFrictionDescriptionAxisDescription,
    nullptr,
    &typeid(hkpVehicleFrictionDescription::AxisDescription),
    sizeof(hkpVehicleFrictionDescription::AxisDescription)
};
hkTypeInfo hkpVehicleFrictionDescriptionTypeInfo = {
    "hkpVehicleFrictionDescription",
    nullptr,
    cleanupLoadedObjecthkpVehicleFrictionDescription,
    nullptr,
    &typeid(hkpVehicleFrictionDescription),
    sizeof(hkpVehicleFrictionDescription)
};

// The reflected members of hkpVehicleFrictionDescription::AxisDescription.
using hkpVehicleFrictionDescription_AxisDescriptionClass_MembersArray = const hkClassMember[10];
static hkpVehicleFrictionDescription_AxisDescriptionClass_MembersArray
    hkpVehicleFrictionDescription_AxisDescriptionClass_Members{
        {"frictionCircleYtab",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         16,
         {0},
         0x0,
         nullptr},
        {"xStep", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
        {"xStart", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x44, nullptr},
        {"wheelSurfaceInertia",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x48,
         nullptr},
        {"wheelSurfaceInertiaInv",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x4C,
         nullptr},
        {"wheelChassisMassRatio",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x50,
         nullptr},
        {"wheelRadius",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x54,
         nullptr},
        {"wheelRadiusInv",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x58,
         nullptr},
        {"wheelDownForceFactor",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x5C,
         nullptr},
        {"wheelDownForceSumFactor",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_REAL},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x60,
         nullptr}
};

// The reflected members of hkpVehicleFrictionDescription.
using hkpVehicleFrictionDescriptionClass_MembersArray = const hkClassMember[3];
static hkpVehicleFrictionDescriptionClass_MembersArray hkpVehicleFrictionDescriptionClass_Members{
    {"wheelDistance", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"chassisMassInv", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
    {"axleDescr",
     &hkpVehicleFrictionDescriptionAxisDescriptionClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0x8,
     nullptr}
};
hkClass hkpVehicleFrictionDescriptionAxisDescriptionClass(
    "hkpVehicleFrictionDescriptionAxisDescription",
    nullptr,
    100,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleFrictionDescription_AxisDescriptionClass_Members,
    10,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpVehicleFrictionDescriptionClass(
    "hkpVehicleFrictionDescription",
    nullptr,
    208,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleFrictionDescriptionClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkpVehicleFrictionDescriptionAxisDescription(void* p) {
    ;
}

static void cleanupLoadedObjecthkpVehicleFrictionDescription(void* p) {
    ;
}

const hkClass& hkpVehicleFrictionDescription::AxisDescription::staticClass() {
    return hkpVehicleFrictionDescriptionAxisDescriptionClass;
}

const hkClass& hkpVehicleFrictionDescription::staticClass() {
    return hkpVehicleFrictionDescriptionClass;
}
