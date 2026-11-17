#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

// The reflection data of hkpConstraintMotor's class record, as the Havok SDK's class file has it: the names of
// MotorType's values and the enum record, then the enum pointer and the member table.
using hkpConstraintMotorMotorTypeEnumItemsArray = const hkClassEnum::Item[6];
static hkpConstraintMotorMotorTypeEnumItemsArray hkpConstraintMotorMotorTypeEnumItems{{0, "TYPE_INVALID"},
	{1, "TYPE_POSITION"}, {2, "TYPE_VELOCITY"}, {3, "TYPE_SPRING_DAMPER"}, {4, "TYPE_CALLBACK"}, {5, "TYPE_MAX"}};
using hkpConstraintMotorEnumsArray = const hkClassEnum[1];
static hkpConstraintMotorEnumsArray hkpConstraintMotorEnums{
	{"MotorType", hkpConstraintMotorMotorTypeEnumItems, 6, nullptr, {0}}};
static const hkClassEnum* hkpConstraintMotorMotorTypeEnum = hkpConstraintMotorEnums;
// The type member's enum is null in the image; the initialiser below fills it.
using hkpConstraintMotorClass_MembersArray = hkClassMember[1];
static hkpConstraintMotorClass_MembersArray hkpConstraintMotorClass_Members{{"type", nullptr,
	&hkpConstraintMotorEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, {0}, 0x8, nullptr}};

hkClass hkpConstraintMotorClass("hkpConstraintMotor", &hkReferencedObjectClass, 12, nullptr, 0, hkpConstraintMotorEnums,
	1, hkpConstraintMotorClass_Members, 1, nullptr, nullptr, 0, 0);
