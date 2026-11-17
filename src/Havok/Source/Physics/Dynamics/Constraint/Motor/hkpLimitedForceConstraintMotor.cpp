#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflection data of hkpLimitedForceConstraintMotor (the PDB's hkpLimitedForceConstraintMotorClass.obj; the SDK
// has no .cpp here to name the file, so it takes the header's stem).

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpLimitedForceConstraintMotor, as the exe's .rdata holds them.
using hkpLimitedForceConstraintMotorClass_MembersArray = const hkClassMember[2];
static hkpLimitedForceConstraintMotorClass_MembersArray hkpLimitedForceConstraintMotorClass_Members{
	{"minForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"maxForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}};

hkClass hkpLimitedForceConstraintMotorClass("hkpLimitedForceConstraintMotor", &hkpConstraintMotorClass, 20, nullptr, 0,
	nullptr, 0, hkpLimitedForceConstraintMotorClass_Members, 2, nullptr, nullptr, 0, 0);

// The exe's slot holds hkBaseObject's deleting destructor (0xDE10B0): the motor adds nothing to destroy.
hkpLimitedForceConstraintMotor::~hkpLimitedForceConstraintMotor() {}
