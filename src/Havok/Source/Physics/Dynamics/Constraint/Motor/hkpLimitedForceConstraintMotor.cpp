#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflection data of hkpLimitedForceConstraintMotor.

// The reflected members of hkpLimitedForceConstraintMotor.
using hkpLimitedForceConstraintMotorClass_MembersArray = const hkClassMember[2];
static hkpLimitedForceConstraintMotorClass_MembersArray hkpLimitedForceConstraintMotorClass_Members{
    {"minForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"maxForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}
};

hkClass hkpLimitedForceConstraintMotorClass(
    "hkpLimitedForceConstraintMotor",
    &hkpConstraintMotorClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpLimitedForceConstraintMotorClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

hkpLimitedForceConstraintMotor::~hkpLimitedForceConstraintMotor() {}
