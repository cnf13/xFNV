#include "Havok/Source/Physics/Vehicle/DriverInput/Default/hkpVehicleDefaultAnalogDriverInput.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <cmath>
#include <xmmintrin.h>

static void finishLoadedObjecthkpVehicleDriverInputAnalogStatus(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDriverInputAnalogStatus();
static void finishLoadedObjecthkpVehicleDefaultAnalogDriverInput(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultAnalogDriverInput();
static void cleanupLoadedObjecthkpVehicleDriverInputAnalogStatus(void* p);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleDriverInputAnalogStatus, as the exe's .rdata holds them.
using hkpVehicleDriverInputAnalogStatusClass_MembersArray = const hkClassMember[4];
static hkpVehicleDriverInputAnalogStatusClass_MembersArray hkpVehicleDriverInputAnalogStatusClass_Members{
	{"positionX", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
	{"positionY", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"handbrakeButtonPressed", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10,
		nullptr},
	{"reverseButtonPressed", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x11,
		nullptr}};
hkClass hkpVehicleDriverInputAnalogStatusClass("hkpVehicleDriverInputAnalogStatus", &hkpVehicleDriverInputStatusClass,
	20, nullptr, 0, nullptr, 0, hkpVehicleDriverInputAnalogStatusClass_Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDriverInputAnalogStatusTypeInfo = {"hkpVehicleDriverInputAnalogStatus",
	finishLoadedObjecthkpVehicleDriverInputAnalogStatus, cleanupLoadedObjecthkpVehicleDriverInputAnalogStatus,
	getVtablehkpVehicleDriverInputAnalogStatus(), &typeid(hkpVehicleDriverInputAnalogStatus),
	sizeof(hkpVehicleDriverInputAnalogStatus)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleDefaultAnalogDriverInput, as the exe's .rdata holds them.
using hkpVehicleDefaultAnalogDriverInputClass_MembersArray = const hkClassMember[4];
static hkpVehicleDefaultAnalogDriverInputClass_MembersArray hkpVehicleDefaultAnalogDriverInputClass_Members{
	{"slopeChangePointX", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8,
		nullptr},
	{"initialSlope", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"deadZone", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
	{"autoReverse", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr}};
hkClass hkpVehicleDefaultAnalogDriverInputClass("hkpVehicleDefaultAnalogDriverInput", &hkpVehicleDriverInputClass, 24,
	nullptr, 0, nullptr, 0, hkpVehicleDefaultAnalogDriverInputClass_Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDefaultAnalogDriverInputTypeInfo = {"hkpVehicleDefaultAnalogDriverInput",
	finishLoadedObjecthkpVehicleDefaultAnalogDriverInput, cleanupLoadedObjecthkpVehicleDefaultAnalogDriverInput,
	getVtablehkpVehicleDefaultAnalogDriverInput(), &typeid(hkpVehicleDefaultAnalogDriverInput),
	sizeof(hkpVehicleDefaultAnalogDriverInput)};

// The type info's finish-loaded-object callback for hkpVehicleDriverInputAnalogStatus: the null check, then its
// finish constructor, which the exe inlines.
static void finishLoadedObjecthkpVehicleDriverInputAnalogStatus(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpVehicleDriverInputAnalogStatus(kFlag);
	}
}

static const void* getVtablehkpVehicleDriverInputAnalogStatus() {
	return hkGetVtableOf<hkpVehicleDriverInputAnalogStatus>();
}

// The type info's finish-loaded-object callback for hkpVehicleDefaultAnalogDriverInput: the null check, then its
// finish constructor, which the exe inlines.
static void finishLoadedObjecthkpVehicleDefaultAnalogDriverInput(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpVehicleDefaultAnalogDriverInput(kFlag);
	}
}

static const void* getVtablehkpVehicleDefaultAnalogDriverInput() {
	return hkGetVtableOf<hkpVehicleDefaultAnalogDriverInput>();
}

static void cleanupLoadedObjecthkpVehicleDriverInputAnalogStatus(void* p) {
	static_cast<hkpVehicleDriverInputAnalogStatus*>(p)->~hkpVehicleDriverInputAnalogStatus();
}

void cleanupLoadedObjecthkpVehicleDefaultAnalogDriverInput(void* p) {
	static_cast<hkpVehicleDefaultAnalogDriverInput*>(p)->~hkpVehicleDefaultAnalogDriverInput();
}

// The exe's slot holds hkBaseObject's deleting destructor (0xDE10B0): the status adds nothing to destroy.
hkpVehicleDriverInputAnalogStatus::~hkpVehicleDriverInputAnalogStatus() {}

hkpVehicleDriverInputStatus* hkpVehicleDriverInputAnalogStatus::clone() const {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpVehicleDriverInputAnalogStatus* result = static_cast<hkpVehicleDriverInputAnalogStatus*>(Instance->m_heap
			->blockAlloc(sizeof(hkpVehicleDriverInputAnalogStatus)));
	return ::new (result) hkpVehicleDriverInputAnalogStatus(*this);
}

const hkClass& hkpVehicleDriverInputAnalogStatus::staticClass() {
	return hkpVehicleDriverInputAnalogStatusClass;
}

hkpVehicleDefaultAnalogDriverInput::hkpVehicleDefaultAnalogDriverInput()
	: m_slopeChangePointX(0.0f), m_initialSlope(0.0f), m_deadZone(0.0f), m_autoReverse(false) {}

hkpVehicleDefaultAnalogDriverInput::~hkpVehicleDefaultAnalogDriverInput() {}

void hkpVehicleDefaultAnalogDriverInput::calcDriverInput(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInputStatus* deviceStatus,
	hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInputOut) {
	// The SDK's downcast: this driver input is given an analog device's status.
	const hkpVehicleDriverInputAnalogStatus* analogStatus =
		static_cast<const hkpVehicleDriverInputAnalogStatus*>(deviceStatus);
	filteredInputOut.m_acceleratorPedalInput = calcAcceleratorInput(deltaTime, vehicle, analogStatus, filteredInputOut);
	filteredInputOut.m_brakePedalInput = calcBrakeInput(deltaTime, vehicle, analogStatus, filteredInputOut);
	filteredInputOut.m_steeringWheelInput = calcSteeringInput(deltaTime, vehicle, analogStatus, filteredInputOut);
	filteredInputOut.m_handbrakeOn.m_bool = analogStatus->m_handbrakeButtonPressed.m_bool;
	hkBool tryingToReverse;
	filteredInputOut.m_tryingToReverse.m_bool =
		calcTryingToReverse(&tryingToReverse, deltaTime, vehicle, analogStatus, filteredInputOut)->m_bool;
}

// The pedal is the stick's y negated where it is not positive (y flipped while auto-reversing), and full while
// reversing without auto-reverse. The negation is a multiply by the double -1.0, stored as a float.
float hkpVehicleDefaultAnalogDriverInput::calcAcceleratorInput(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInputAnalogStatus* deviceStatus, FilteredDriverInputOutput& input) const {
	bool bReversing = vehicle->m_isReversing.m_bool;
	float fInput;
	if (bReversing && m_autoReverse.m_bool) {
		fInput = -deviceStatus->m_positionY;
	} else {
		fInput = deviceStatus->m_positionY;
	}
	if (!m_autoReverse.m_bool && bReversing) {
		return 1.0f;
	}
	if (fInput > 0.0f) {
		return 0.0f;
	}
	return static_cast<float>(fInput * -1.0);
}

float hkpVehicleDefaultAnalogDriverInput::calcBrakeInput(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInputAnalogStatus* deviceStatus,
	FilteredDriverInputOutput& FilteredDriverInputOutputOut) const {
	double v5;
	if (vehicle->m_isReversing.m_bool && m_autoReverse.m_bool) {
		v5 = (double)(-deviceStatus->m_positionY);
	} else {
		v5 = (double)deviceStatus->m_positionY;
	}
	float v7 = (float)v5;
	double result = 0.0;
	if (!(v7 < 0.0)) {
		return v7;
	}
	return result;
}

hkBool* hkpVehicleDefaultAnalogDriverInput::calcTryingToReverse(hkBool* arResult, float deltaTime,
	const hkpVehicleInstance* vehicle, const hkpVehicleDriverInputAnalogStatus* deviceStatus,
	hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputOutputOut) {
	if (!m_autoReverse.m_bool) {
		arResult->m_bool = deviceStatus->m_reverseButtonPressed.m_bool;
		return arResult;
	}
	const hkpEntity* entity = vehicle->m_entity;
	const __m128 velocity = entity->m_motion.m_linearVelocity.m_quad;
	const __m128 velocitySq = _mm_mul_ps(velocity, velocity);
	// z + (y + x), in lane 0.
	const __m128 velocityLenSq = _mm_add_ss(_mm_shuffle_ps(velocitySq, velocitySq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(velocitySq, velocitySq, 0x55), velocitySq));
	const float speed = _mm_cvtss_f32(_mm_sqrt_ss(velocityLenSq));
	const uint8_t isReversing = vehicle->m_isReversing.m_bool;
	const bool isSlow = speed < 1.388889f;
	uint8_t wantsReverse;
	if (!(deviceStatus->m_positionY > 0.10000000149011612)) {
		wantsReverse = 0;
	} else {
		wantsReverse = 1;
		if (speed > 1.1920929e-07f) {
			// The chassis' forward axis in world space: (y * col1 + x * col0) + z * col2.
			const __m128 forward = vehicle->m_data->m_chassisOrientation.m_col1.m_quad;
			const hkRotation& rotation = entity->m_motion.m_motionState.m_transform.m_rotation;
			const __m128 worldForward =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(forward, forward, 0x55), rotation.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(forward, forward, 0x00), rotation.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(forward, forward, 0xAA), rotation.m_col2.m_quad));
			const __m128 zero = _mm_set_ss(0.0f);
			const __m128 three = _mm_set_ss(3.0f);
			const __m128 half = _mm_set_ss(0.5f);
			// The inverse lengths: one Newton step on rsqrt, 0 for a zero vector.
			const __m128 velocityRsqrt = _mm_rsqrt_ss(velocityLenSq);
			const __m128 velocityInvLen = _mm_andnot_ps(_mm_cmpeq_ss(velocityLenSq, zero),
				_mm_mul_ss(_mm_mul_ss(half, velocityRsqrt),
					_mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(velocityLenSq, velocityRsqrt), velocityRsqrt))));
			const __m128 forwardSq = _mm_mul_ps(worldForward, worldForward);
			const __m128 forwardLenSq = _mm_add_ss(_mm_shuffle_ps(forwardSq, forwardSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(forwardSq, forwardSq, 0x55), forwardSq));
			const __m128 forwardRsqrt = _mm_rsqrt_ss(forwardLenSq);
			const __m128 forwardInvLen = _mm_andnot_ps(_mm_cmpeq_ss(forwardLenSq, zero),
				_mm_mul_ss(_mm_mul_ss(half, forwardRsqrt),
					_mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(forwardLenSq, forwardRsqrt), forwardRsqrt))));
			const __m128 product =
				_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(velocityInvLen, velocityInvLen, 0x00), velocity),
					_mm_mul_ps(_mm_shuffle_ps(forwardInvLen, forwardInvLen, 0x00), worldForward));
			const float dot = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(product, product, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)));
			// Braking while moving sideways or backwards against the chassis' forward axis: trying to reverse.
			if (dot < 1.1920929e-07f) {
				arResult->m_bool = 1;
				return arResult;
			}
		}
	}
	if (isReversing) {
		arResult->m_bool = isSlow ? wantsReverse : 1;
	} else {
		arResult->m_bool = (isSlow && wantsReverse) ? 1 : 0;
	}
	return arResult;
}

float hkpVehicleDefaultAnalogDriverInput::calcSteeringInput(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInputAnalogStatus* deviceStatus,
	hkpVehicleDriverInput::FilteredDriverInputOutput& FilteredDriverInputOutputOut) const {
	double m_positionX = deviceStatus->m_positionX;
	float deviceStatusa = std::fabs(m_positionX);
	if (!(m_deadZone > static_cast<double>(deviceStatusa))) {
		double v9;
		float v14;
		bool v7 = m_positionX > 0.0;
		double v8 = deviceStatusa;
		if (v7) {
			v9 = 1.0;
		} else {
			v9 = -1.0f;
		}
		float deviceStatusb = v9;
		if (!(m_slopeChangePointX > v8)) {
			double v10 = m_slopeChangePointX - m_deadZone;
			float v15 = m_initialSlope * v10;
			double v11 = (1.0 - v15) / (1.0 - m_deadZone - v10);
			double v12 = v15;
			float v16 = v11;
			v14 = (v8 - m_slopeChangePointX) * v16 + v12;
		} else {
			v14 = (v8 - m_deadZone) * m_initialSlope;
		}
		return v14 * deviceStatusb;
	} else {
		return 0.0;
	}
}

const hkClass& hkpVehicleDefaultAnalogDriverInput::staticClass() {
	return hkpVehicleDefaultAnalogDriverInputClass;
}
