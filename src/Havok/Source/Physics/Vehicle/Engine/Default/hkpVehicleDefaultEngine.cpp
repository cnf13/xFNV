#include "Havok/Source/Physics/Vehicle/Engine/Default/hkpVehicleDefaultEngine.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"

static void finishLoadedObjecthkpVehicleDefaultEngine(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultEngine();
static void cleanupLoadedObjecthkpVehicleDefaultEngine(void* p);

// The reflected members of hkpVehicleDefaultEngine.
using hkpVehicleDefaultEngineClass_MembersArray = const hkClassMember[10];
static hkpVehicleDefaultEngineClass_MembersArray hkpVehicleDefaultEngineClass_Members{
	{"minRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
	{"optRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"maxRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
	{"maxTorque", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"torqueFactorAtMinRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18,
		nullptr},
	{"torqueFactorAtMaxRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1C,
		nullptr},
	{"resistanceFactorAtMinRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20,
		nullptr},
	{"resistanceFactorAtOptRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24,
		nullptr},
	{"resistanceFactorAtMaxRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x28,
		nullptr},
	{"clutchSlipRPM", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2C, nullptr}};
hkClass hkpVehicleDefaultEngineClass("hkpVehicleDefaultEngine", &hkpVehicleEngineClass, 48, nullptr, 0, nullptr, 0,
	hkpVehicleDefaultEngineClass_Members, 10, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDefaultEngineTypeInfo = {"hkpVehicleDefaultEngine", finishLoadedObjecthkpVehicleDefaultEngine,
	cleanupLoadedObjecthkpVehicleDefaultEngine, getVtablehkpVehicleDefaultEngine(), &typeid(hkpVehicleDefaultEngine),
	sizeof(hkpVehicleDefaultEngine)};

// The type info's finish-loaded-object callback for hkpVehicleDefaultEngine: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultEngine(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpVehicleDefaultEngine(kFlag);
	}
}

static const void* getVtablehkpVehicleDefaultEngine() {
	return hkGetVtableOf<hkpVehicleDefaultEngine>();
}

static void cleanupLoadedObjecthkpVehicleDefaultEngine(void* p) {
	static_cast<hkpVehicleDefaultEngine*>(p)->~hkpVehicleDefaultEngine();
}

hkpVehicleDefaultEngine::hkpVehicleDefaultEngine()
	: m_minRPM(1000.0f), m_optRPM(4000.0f), m_maxRPM(6000.0f), m_maxTorque(800.0f), m_torqueFactorAtMinRPM(1.0f),
	  m_torqueFactorAtMaxRPM(1.0f), m_resistanceFactorAtMinRPM(0.0f), m_resistanceFactorAtOptRPM(0.0f),
	  m_resistanceFactorAtMaxRPM(0.0f), m_clutchSlipRPM(2000.0f) {}

hkpVehicleDefaultEngine::~hkpVehicleDefaultEngine() {}

void hkpVehicleDefaultEngine::calcEngineInfo(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
	const hkpVehicleTransmission::TransmissionOutput& transmissionInfo, hkpVehicleEngine::EngineOutput& engineOutput) {
	double v13;
	float engineOutputf = transmissionInfo.m_transmissionRPM;
	double v8 = engineOutputf;
	engineOutput.m_rpm = engineOutputf;
	float engineOutputa = filteredDriverInputInfo.m_acceleratorPedalInput;
	double m_minRPM = this->m_minRPM;
	bool v10 = m_minRPM < v8;
	bool v11 = m_minRPM == v8;
	double v12 = engineOutputa;
	if (v10 || v11) {
		v13 = engineOutputa;
	} else {
		double v14;
		float engineOutputb = m_clutchSlipRPM * v12;
		float transmissionInfo = this->m_minRPM * 0.5;
		if (!(transmissionInfo > v8)) {
			v14 = (v8 - transmissionInfo) * engineOutputb / (this->m_minRPM - transmissionInfo) + this->m_minRPM;
			v13 = v12;
		} else {
			v13 = v12;
			v14 = engineOutputb + this->m_minRPM;
		}
		engineOutput.m_rpm = v14;
	}
	float engineOutputc = engineOutput.m_rpm - m_optRPM;
	double v16 = engineOutputc;
	if (!(engineOutputc < 0.0)) {
		float transmissionInfob;
		float engineOutpute;
		if (!(m_maxRPM > static_cast<double>(engineOutput.m_rpm))) {
			engineOutput.m_rpm = m_maxRPM;
			transmissionInfob = 0.0;
			engineOutpute = m_resistanceFactorAtMaxRPM * m_maxTorque;
		} else {
			double v18 = engineOutputc;
			float engineOutputi = 1.0 / (m_maxRPM - m_optRPM);
			double v19 = engineOutputi;
			float engineOutputj = (m_resistanceFactorAtMaxRPM - m_resistanceFactorAtOptRPM) * engineOutputi;
			engineOutpute = (engineOutputj * v18 + m_resistanceFactorAtOptRPM) * m_maxTorque;
			float transmissionInfod = v19 * ((m_torqueFactorAtMaxRPM - 1.0) * v19);
			transmissionInfob = (v18 * v18 * transmissionInfod + 1.0) * m_maxTorque;
		}
		engineOutput.m_torque = v13 * transmissionInfob - engineOutpute;
	} else {
		float engineOutputg = 1.0 / (this->m_minRPM - m_optRPM);
		double v17 = engineOutputg;
		float engineOutputh = (m_resistanceFactorAtMinRPM - m_resistanceFactorAtOptRPM) * engineOutputg;
		float engineOutputd = (engineOutputh * v16 + m_resistanceFactorAtOptRPM) * m_maxTorque;
		float transmissionInfoc = v17 * ((m_torqueFactorAtMinRPM - 1.0) * v17);
		float transmissionInfoa = (v16 * v16 * transmissionInfoc + 1.0) * m_maxTorque;
		float filteredDriverInputInfo = transmissionInfo.m_transmissionRPM;
		if (this->m_minRPM > static_cast<double>(filteredDriverInputInfo)) {
			engineOutputd = filteredDriverInputInfo / this->m_minRPM * engineOutputd;
		}
		engineOutput.m_torque = v13 * transmissionInfoa - engineOutputd;
	}
}

const hkClass& hkpVehicleDefaultEngine::staticClass() {
	return hkpVehicleDefaultEngineClass;
}
