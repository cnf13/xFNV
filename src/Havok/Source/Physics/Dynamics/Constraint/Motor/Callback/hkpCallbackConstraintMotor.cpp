#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Callback/hkpCallbackConstraintMotor.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

static void finishLoadedObjecthkpCallbackConstraintMotor(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpCallbackConstraintMotor();
static void cleanupLoadedObjecthkpCallbackConstraintMotor(void* p);

// The reflected enum of hkpCallbackConstraintMotor.
using hkpCallbackConstraintMotorCallbackTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpCallbackConstraintMotorCallbackTypeEnumItemsArray hkpCallbackConstraintMotorCallbackTypeEnumItems{
	{hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_HAVOK_DEMO_SPRING_DAMPER,
		"CALLBACK_MOTOR_TYPE_HAVOK_DEMO_SPRING_DAMPER"},
	{hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_0, "CALLBACK_MOTOR_TYPE_USER_0"},
	{hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_1, "CALLBACK_MOTOR_TYPE_USER_1"},
	{hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_2, "CALLBACK_MOTOR_TYPE_USER_2"},
	{hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_3, "CALLBACK_MOTOR_TYPE_USER_3"}};
using hkpCallbackConstraintMotorEnumsArray = const hkClassEnum[1];
static hkpCallbackConstraintMotorEnumsArray hkpCallbackConstraintMotorEnums{
	{"CallbackType", hkpCallbackConstraintMotorCallbackTypeEnumItems, 5, nullptr, {0}}};

static const hkClassEnum* hkpCallbackConstraintMotorCallbackTypeEnum = &hkpCallbackConstraintMotorEnums[0];

// The class record's member table: the callbackType member names its enum by the pointer above, which is stored by the
// array's dynamic initializer.
using hkpCallbackConstraintMotorClass_MembersArray = hkClassMember[5];
static hkpCallbackConstraintMotorClass_MembersArray hkpCallbackConstraintMotorClass_Members{
	{"callbackFunc", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x14,
		nullptr},
	{"callbackType", nullptr, hkpCallbackConstraintMotorCallbackTypeEnum, {hkClassMember::TYPE_ENUM},
		{hkClassMember::TYPE_UINT32}, 0, {0}, 0x18, nullptr},
	{"userData0", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1C, nullptr},
	{"userData1", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
	{"userData2", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr}};
hkClass hkpCallbackConstraintMotorClass("hkpCallbackConstraintMotor", &hkpLimitedForceConstraintMotorClass, 40, nullptr,
	0, hkpCallbackConstraintMotorEnums, 1, hkpCallbackConstraintMotorClass_Members, 5, nullptr, nullptr, 0, 0);
hkTypeInfo hkpCallbackConstraintMotorTypeInfo = {"hkpCallbackConstraintMotor",
	finishLoadedObjecthkpCallbackConstraintMotor, cleanupLoadedObjecthkpCallbackConstraintMotor,
	getVtablehkpCallbackConstraintMotor(), &typeid(hkpCallbackConstraintMotor), sizeof(hkpCallbackConstraintMotor)};

// The type info's finish-loaded-object callback for hkpCallbackConstraintMotor: its finish constructor.
static void finishLoadedObjecthkpCallbackConstraintMotor(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpCallbackConstraintMotor(kFlag);
	}
}

static const void* getVtablehkpCallbackConstraintMotor() {
	return hkGetVtableOf<hkpCallbackConstraintMotor>();
}

static void cleanupLoadedObjecthkpCallbackConstraintMotor(void* p) {
	static_cast<hkpCallbackConstraintMotor*>(p)->~hkpCallbackConstraintMotor();
}

hkpCallbackConstraintMotor::hkpCallbackConstraintMotor(hkpCallbackConstraintMotor::CallbackType callbackType,
	CalcMotorDataCallbackFunc callbackFunc)
	: m_callbackFunc(callbackFunc) {
	m_callbackType.m_storage = callbackType;
	m_minForce = -1e+06f;
	m_type.m_storage = 4;
	m_maxForce = 1e+06f;
}

// The base copy constructors copy the motor type and force limits; hkReferencedObject's starts the count at 1.
hkpCallbackConstraintMotor::hkpCallbackConstraintMotor(const hkpCallbackConstraintMotor& other)
	: hkpLimitedForceConstraintMotor(other), m_callbackFunc(other.m_callbackFunc), m_userData0(other.m_userData0),
	  m_userData1(other.m_userData1), m_userData2(other.m_userData2) {
	m_callbackType.m_storage = other.m_callbackType.m_storage;
}

hkpCallbackConstraintMotor::~hkpCallbackConstraintMotor() {}

hkpConstraintMotor* hkpCallbackConstraintMotor::clone() const {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpCallbackConstraintMotor* v3 =
		static_cast<hkpCallbackConstraintMotor*>(Instance->m_heap->blockAlloc(sizeof(hkpCallbackConstraintMotor)));
	// The native sizeof, because the heap frees by this value.
	v3->m_memSizeAndFlags = sizeof(hkpCallbackConstraintMotor);
	return ::new (v3) hkpCallbackConstraintMotor(*this);
}

const hkClass& hkpCallbackConstraintMotor::staticClass() {
	return hkpCallbackConstraintMotorClass;
}
