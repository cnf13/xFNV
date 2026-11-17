#include "Havok/Source/Physics/Vehicle/Brake/Default/hkpVehicleDefaultBrake.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Brake/hkpVehicleBrake.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <cmath>

static void cleanupLoadedObjecthkpVehicleDefaultBrakeWheelBrakingProperties(void* p);
static void finishLoadedObjecthkpVehicleDefaultBrake(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultBrake();
static void cleanupLoadedObjecthkpVehicleDefaultBrake(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleDefaultBrakeWheelBrakingPropertiesTypeInfo = {"hkpVehicleDefaultBrakeWheelBrakingProperties",
	nullptr, cleanupLoadedObjecthkpVehicleDefaultBrakeWheelBrakingProperties, nullptr,
	&typeid(hkpVehicleDefaultBrake::WheelBrakingProperties), sizeof(hkpVehicleDefaultBrake::WheelBrakingProperties)};

// The reflected members of hkpVehicleDefaultBrake::WheelBrakingProperties and of hkpVehicleDefaultBrake.
using hkpVehicleDefaultBrakeWheelBrakingPropertiesClass_MembersArray = const hkClassMember[3];
static hkpVehicleDefaultBrakeWheelBrakingPropertiesClass_MembersArray
	hkpVehicleDefaultBrakeWheelBrakingPropertiesClass_Members{{"maxBreakingTorque", nullptr, nullptr,
																  {hkClassMember::TYPE_REAL},
																  {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
		{"minPedalInputToBlock", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4,
			nullptr},
		{"isConnectedToHandbrake", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0},
			0x8, nullptr}};
using hkpVehicleDefaultBrakeClass_MembersArray = const hkClassMember[2];
static hkpVehicleDefaultBrakeClass_MembersArray hkpVehicleDefaultBrakeClass_Members{
	{"wheelBrakingProperties", &hkpVehicleDefaultBrakeWheelBrakingPropertiesClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr},
	{"wheelsMinTimeToBlock", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14,
		nullptr}};
hkClass hkpVehicleDefaultBrakeWheelBrakingPropertiesClass("hkpVehicleDefaultBrakeWheelBrakingProperties", nullptr, 12,
	nullptr, 0, nullptr, 0, hkpVehicleDefaultBrakeWheelBrakingPropertiesClass_Members, 3, nullptr, nullptr, 0, 0);
hkClass hkpVehicleDefaultBrakeClass("hkpVehicleDefaultBrake", &hkpVehicleBrakeClass, 24, nullptr, 0, nullptr, 0,
	hkpVehicleDefaultBrakeClass_Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDefaultBrakeTypeInfo = {"hkpVehicleDefaultBrake", finishLoadedObjecthkpVehicleDefaultBrake,
	cleanupLoadedObjecthkpVehicleDefaultBrake, getVtablehkpVehicleDefaultBrake(), &typeid(hkpVehicleDefaultBrake),
	sizeof(hkpVehicleDefaultBrake)};

static void cleanupLoadedObjecthkpVehicleDefaultBrakeWheelBrakingProperties(void* p) {
	;
}

// The type info's finish-loaded-object callback for hkpVehicleDefaultBrake: runs its finish constructor.
static void finishLoadedObjecthkpVehicleDefaultBrake(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpVehicleDefaultBrake(flag);
	}
}

static const void* getVtablehkpVehicleDefaultBrake() {
	return hkGetVtableOf<hkpVehicleDefaultBrake>();
}

static void cleanupLoadedObjecthkpVehicleDefaultBrake(void* p) {
	static_cast<hkpVehicleDefaultBrake*>(p)->~hkpVehicleDefaultBrake();
}

const hkClass& hkpVehicleDefaultBrake::WheelBrakingProperties::staticClass() {
	return hkpVehicleDefaultBrakeWheelBrakingPropertiesClass;
}

hkpVehicleDefaultBrake::hkpVehicleDefaultBrake() : m_wheelsMinTimeToBlock(0.0f) {}

// The braking properties array frees itself.
hkpVehicleDefaultBrake::~hkpVehicleDefaultBrake() {}

void hkpVehicleDefaultBrake::calcBreakingInfo(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleDriverInput::FilteredDriverInputOutput& FilteredDriverInputOutput,
	hkpVehicleBrake::WheelBreakingOutput& breakingInfo) {
	double v10;
	int32_t v11;
	int32_t v12;
	double v13;
	double v14;
	int32_t v21;
	bool vehiclea;
	char m_bool = FilteredDriverInputOutput.m_handbrakeOn.m_bool;
	float m_brakePedalInput = FilteredDriverInputOutput.m_brakePedalInput;
	hkpVehicleBrake::WheelBreakingOutput* v6 = &breakingInfo;
	hkpVehicleDefaultBrake* v8 = this;
	int32_t v9 = 0;
	char filteredDriverInfo = 0;
	if (vehicle->m_data->m_numWheels <= 0) {
		goto LABEL_20;
	}
	v10 = deltaTime;
	vehiclea = m_bool != 0;
	v11 = 0;
	v12 = 0;
	v13 = 1.0 / deltaTime;
	v21 = 0;
	v14 = m_brakePedalInput;
	while (1u) {
		breakingInfo.m_isFixed.m_data[v9].m_bool =
			vehiclea && v8->m_wheelBrakingProperties.m_data[v11].m_isConnectedToHandbrake.m_bool != 0;
		float* p_m_maxBreakingTorque = &v8->m_wheelBrakingProperties.m_data[v11].m_maxBreakingTorque;
		if (p_m_maxBreakingTorque[1] < v14) {
			filteredDriverInfo = 1;
		}
		hkpVehicleData::WheelComponentParams* m_data =
			static_cast<hkArrayBase<hkpVehicleData::WheelComponentParams>&>(vehicle->m_data->m_wheelParams).begin();
		double m_radius = m_data[v21].m_radius;
		float deltaTimeb = -(vehicle->m_wheelsInfo.m_data[v12].m_spinVelocity * m_radius * (m_data[v21].m_mass * v13));
		float deltaTimea = m_radius * deltaTimeb;
		float v25 = *p_m_maxBreakingTorque * v14;
		float v24 = std::fabs((double)deltaTimea);
		if (v25 < static_cast<double>(v24)) {
			double v18 = v25;
			if (!(deltaTimea > 0.0)) {
				v18 = -v18;
			}
			deltaTimea = v18;
		}
		v6 = &breakingInfo;
		++v21;
		breakingInfo.m_brakingTorque.m_data[v9++] = deltaTimea;
		++v12;
		++v11;
		if (v9 >= vehicle->m_data->m_numWheels) {
			break;
		}
		v8 = this;
	}
	if (filteredDriverInfo) {
		if (!(m_wheelsMinTimeToBlock <= static_cast<double>(breakingInfo.m_wheelsTimeSinceMaxPedalInput))) {
			breakingInfo.m_wheelsTimeSinceMaxPedalInput = v10 + breakingInfo.m_wheelsTimeSinceMaxPedalInput;
		} else {
			int32_t v19 = 0;
			if (vehicle->m_data->m_numWheels > 0) {
				int32_t v20 = 0;
				do {
					if (m_wheelBrakingProperties.m_data[v20].m_minPedalInputToBlock < v14) {
						breakingInfo.m_isFixed.m_data[v19].m_bool = 1;
					}
					++v19;
					++v20;
				} while (v19 < vehicle->m_data->m_numWheels);
			}
		}
	} else {
	LABEL_20:;
		v6->m_wheelsTimeSinceMaxPedalInput = 0.0;
	}
}

const hkClass& hkpVehicleDefaultBrake::staticClass() {
	return hkpVehicleDefaultBrakeClass;
}
