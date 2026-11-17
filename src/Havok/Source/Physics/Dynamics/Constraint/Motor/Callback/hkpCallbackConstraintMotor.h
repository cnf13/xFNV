#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

class hkpConstraintMotorInput;
class hkpConstraintMotorOutput;

// A force-limited motor whose solver data is supplied by a callback.
class hkpCallbackConstraintMotor : public hkpLimitedForceConstraintMotor {
public:
	using CalcMotorDataCallbackFunc = void (*)(const hkpCallbackConstraintMotor& motor,
		const hkpConstraintMotorInput* input, hkpConstraintMotorOutput* output);

	enum CallbackType : int32_t {
		CALLBACK_MOTOR_TYPE_HAVOK_DEMO_SPRING_DAMPER = 0,
		CALLBACK_MOTOR_TYPE_USER_0 = 1,
		CALLBACK_MOTOR_TYPE_USER_1 = 2,
		CALLBACK_MOTOR_TYPE_USER_2 = 3,
		CALLBACK_MOTOR_TYPE_USER_3 = 4,
	};

	hkpCallbackConstraintMotor(hkpCallbackConstraintMotor::CallbackType callbackType,
		CalcMotorDataCallbackFunc callbackFunc);
	hkpCallbackConstraintMotor(const hkpCallbackConstraintMotor& other);
	// The SDK's finish constructor of a loaded object; assumes hkpLimitedForceConstraintMotor's (the SDK's).
	hkpCallbackConstraintMotor(hkFinishLoadedObjectFlag aFlag) : hkpLimitedForceConstraintMotor(aFlag) {}
	~hkpCallbackConstraintMotor() override;

	hkpConstraintMotor* clone() const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	CalcMotorDataCallbackFunc m_callbackFunc;
	hkEnum<CallbackType, uint32_t> m_callbackType;
	hkUlong m_userData0;
	hkUlong m_userData1;
	hkUlong m_userData2;
};

extern hkClass hkpCallbackConstraintMotorClass;

extern hkTypeInfo hkpCallbackConstraintMotorTypeInfo;
