#include "Havok/Source/Physics/Vehicle/Transmission/Default/hkpVehicleDefaultTransmission.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"

static void finishLoadedObjecthkpVehicleDefaultTransmission(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultTransmission();
static void cleanupLoadedObjecthkpVehicleDefaultTransmission(void* p);

// The reflected members of hkpVehicleDefaultTransmission.
using hkpVehicleDefaultTransmissionClass_MembersArray = const hkClassMember[7];
static hkpVehicleDefaultTransmissionClass_MembersArray hkpVehicleDefaultTransmissionClass_Members{
	{"downshiftRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
	{"upshiftRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"primaryTransmissionRatio", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10,
		nullptr},
	{"clutchDelayTime", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14,
		nullptr},
	{"reverseGearRatio", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18,
		nullptr},
	{"gearsRatio", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x1C, nullptr},
	{"wheelsTorqueRatio", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x28,
		nullptr}};
hkClass hkpVehicleDefaultTransmissionClass("hkpVehicleDefaultTransmission", &hkpVehicleTransmissionClass, 52, nullptr,
	0, nullptr, 0, hkpVehicleDefaultTransmissionClass_Members, 7, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDefaultTransmissionTypeInfo = {"hkpVehicleDefaultTransmission",
	finishLoadedObjecthkpVehicleDefaultTransmission, cleanupLoadedObjecthkpVehicleDefaultTransmission,
	getVtablehkpVehicleDefaultTransmission(), &typeid(hkpVehicleDefaultTransmission),
	sizeof(hkpVehicleDefaultTransmission)};

// The type info's finish-loaded-object callback for hkpVehicleDefaultTransmission: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultTransmission(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpVehicleDefaultTransmission(kFlag);
	}
}

static const void* getVtablehkpVehicleDefaultTransmission() {
	return hkGetVtableOf<hkpVehicleDefaultTransmission>();
}

static void cleanupLoadedObjecthkpVehicleDefaultTransmission(void* p) {
	static_cast<hkpVehicleDefaultTransmission*>(p)->~hkpVehicleDefaultTransmission();
}

hkpVehicleDefaultTransmission::hkpVehicleDefaultTransmission()
	: m_downshiftRPM(0.0f), m_upshiftRPM(0.0f), m_primaryTransmissionRatio(0.0f), m_clutchDelayTime(0.0f),
	  m_reverseGearRatio(0.0f) {}

// The arrays are freed by their own destructors.
hkpVehicleDefaultTransmission::~hkpVehicleDefaultTransmission() {}

// Fills the transmission output: reversing, the main torque, the RPM, each wheel's share of the torque (its torque
// ratio times the main torque, rounded to float), then the gear update.
void hkpVehicleDefaultTransmission::calcTransmission(float deltaTime, const hkpVehicleInstance* vehicle,
	hkpVehicleTransmission::TransmissionOutput& transmissionOut) {
	hkBool isReversing;
	transmissionOut.m_isReversing.m_bool = calcIsReversing(&isReversing, vehicle, transmissionOut)->m_bool;
	transmissionOut.m_mainTransmittedTorque = calcMainTransmittedTorque(vehicle, transmissionOut);
	transmissionOut.m_transmissionRPM = calcTransmissionRPM(vehicle, transmissionOut);
	for (int32_t i = 0; i < m_wheelsTorqueRatio.m_size; ++i) {
		const float fTorque = m_wheelsTorqueRatio.m_data[i] * transmissionOut.m_mainTransmittedTorque;
		transmissionOut.m_wheelsTransmittedTorque[i] = fTorque;
	}
	updateCurrentGear(deltaTime, vehicle, transmissionOut);
}

float hkpVehicleDefaultTransmission::calcMainTransmittedTorque(const hkpVehicleInstance* vehicle,
	hkpVehicleTransmission::TransmissionOutput& transmissionOut) const {
	if (transmissionOut.m_delayed.m_bool) {
		return 0.0;
	} else {
		float transmissionOuta = vehicle->m_torque;
		return getCurrentRatio(vehicle, transmissionOut) * transmissionOuta;
	}
}

// The wheels' RPM weighted by their torque share, times the current gear ratio, never below zero. Each wheel's RPM
// (spin velocity * 60.0 * 0.15915493667125702, in double) is rounded to a float, then added to the running float sum;
// the order of the additions matters. Only a result below zero (not NaN) becomes zero.
float hkpVehicleDefaultTransmission::calcTransmissionRPM(const hkpVehicleInstance* vehicle,
	TransmissionOutput& transmissionOut) const {
	float fRpm = 0.0f;
	int32_t iNumWheels = vehicle->m_data->m_numWheels;
	for (int32_t i = 0; i < iNumWheels; ++i) {
		float fWheelRpm = static_cast<float>(static_cast<double>(vehicle->m_wheelsInfo.m_data[i].m_spinVelocity) *
			60.0 * 0.15915493667125702);
		fRpm = static_cast<float>(static_cast<double>(fWheelRpm) * m_wheelsTorqueRatio.m_data[i] +
			static_cast<double>(fRpm));
	}
	float fResult = static_cast<float>(static_cast<double>(getCurrentRatio(vehicle, transmissionOut)) * fRpm);
	if (fResult < 0.0) {
		fResult = 0.0f;
	}
	return fResult;
}

hkBool* hkpVehicleDefaultTransmission::calcIsReversing(hkBool* arResult, const hkpVehicleInstance* vehicle,
	hkpVehicleTransmission::TransmissionOutput& transmissionOut) const {
	arResult->m_bool = vehicle->m_tryingToReverse.m_bool && transmissionOut.m_currentGear <= 0;
	return arResult;
}

void hkpVehicleDefaultTransmission::updateCurrentGear(float deltaTime, const hkpVehicleInstance* vehicle,
	hkpVehicleTransmission::TransmissionOutput& transmissionOut) {
	bool v5 = transmissionOut.m_delayed.m_bool == 0;
	float transmissionOuta = transmissionOut.m_clutchDelayCountdown - deltaTime;
	transmissionOut.m_clutchDelayCountdown = transmissionOuta;
	if (!v5 && transmissionOuta <= 0.0f) {
		transmissionOut.m_delayed.m_bool = 0;
	}
	if (!transmissionOut.m_isReversing.m_bool) {
		if (m_downshiftRPM > static_cast<double>(transmissionOut.m_transmissionRPM)) {
			int8_t m_currentGear = transmissionOut.m_currentGear;
			if (m_currentGear > 0) {
				transmissionOut.m_currentGear = m_currentGear - 1;
				transmissionOut.m_clutchDelayCountdown = m_clutchDelayTime;
				transmissionOut.m_delayed.m_bool = 1;
			}
		}
		if (m_upshiftRPM < static_cast<double>(transmissionOut.m_transmissionRPM)) {
			int8_t v7 = transmissionOut.m_currentGear;
			if (v7 + 1 < m_gearsRatio.m_size) {
				transmissionOut.m_currentGear = v7 + 1;
				transmissionOut.m_clutchDelayCountdown = m_clutchDelayTime;
				transmissionOut.m_delayed.m_bool = 1;
			}
		}
	}
}

float hkpVehicleDefaultTransmission::getCurrentRatio(const hkpVehicleInstance* vehicle,
	hkpVehicleTransmission::TransmissionOutput& transmissionOut) const {
	float transmissionOuta;
	if (transmissionOut.m_isReversing.m_bool) {
		transmissionOuta = -m_reverseGearRatio;
	} else {
		transmissionOuta = m_gearsRatio.m_data[transmissionOut.m_currentGear];
	}
	return m_primaryTransmissionRatio * transmissionOuta;
}

float hkpVehicleDefaultTransmission::calculatePrimaryTransmissionRatio(const float vehicleTopSpeed,
	const float wheelRadius, const float maxEngineRpm, const float topGearRatio) {
	double v4 = maxEngineRpm;
	float maxEngineRpma = vehicleTopSpeed * 1.6050000190734863 / 3.5999999046325684;
	float maxEngineRpmb = maxEngineRpma / wheelRadius;
	float maxEngineRpmc = maxEngineRpmb * 60.0 / 6.2831854820251465;
	return v4 / maxEngineRpmc / topGearRatio;
}

const hkClass& hkpVehicleDefaultTransmission::staticClass() {
	return hkpVehicleDefaultTransmissionClass;
}
