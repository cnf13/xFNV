#include "Havok/Source/Physics/Vehicle/Suspension/Default/hkpVehicleDefaultSuspension.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"

static void finishLoadedObjecthkpVehicleDefaultSuspension(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultSuspension();
static void cleanupLoadedObjecthkpVehicleDefaultSuspension(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersTypeInfo = {
	"hkpVehicleDefaultSuspensionWheelSpringSuspensionParameters", nullptr,
	cleanupLoadedObjecthkpVehicleDefaultSuspensionWheelSpringSuspensionParameters, nullptr,
	&typeid(hkpVehicleDefaultSuspension::WheelSpringSuspensionParameters),
	sizeof(hkpVehicleDefaultSuspension::WheelSpringSuspensionParameters)};

// The reflected members of hkpVehicleDefaultSuspension::WheelSpringSuspensionParameters.
using hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass_MembersArray = const hkClassMember[3];
static hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass_MembersArray
	hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass_Members{
		{"strength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
		{"dampingCompression", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4,
			nullptr},
		{"dampingRelaxation", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8,
			nullptr}};
hkClass hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass("hkpVehicleDefaultSuspensionWheelSpringSuspensi"
																		"onParameters",
	nullptr, 12, nullptr, 0, nullptr, 0, hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass_Members, 3,
	nullptr, nullptr, 0, 0);

// The reflected members of hkpVehicleDefaultSuspension.
using hkpVehicleDefaultSuspensionClass_MembersArray = const hkClassMember[1];
static hkpVehicleDefaultSuspensionClass_MembersArray hkpVehicleDefaultSuspensionClass_Members{
	{"wheelSpringParams", &hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass, nullptr,
		{hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x14, nullptr}};
hkClass hkpVehicleDefaultSuspensionClass("hkpVehicleDefaultSuspension", &hkpVehicleSuspensionClass, 32, nullptr, 0,
	nullptr, 0, hkpVehicleDefaultSuspensionClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVehicleDefaultSuspensionTypeInfo = {"hkpVehicleDefaultSuspension",
	finishLoadedObjecthkpVehicleDefaultSuspension, cleanupLoadedObjecthkpVehicleDefaultSuspension,
	getVtablehkpVehicleDefaultSuspension(), &typeid(hkpVehicleDefaultSuspension), sizeof(hkpVehicleDefaultSuspension)};

// The type info's finish-loaded-object callback for hkpVehicleDefaultSuspension: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultSuspension(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpVehicleDefaultSuspension(kFlag);
	}
}

static const void* getVtablehkpVehicleDefaultSuspension() {
	return hkGetVtableOf<hkpVehicleDefaultSuspension>();
}

static void cleanupLoadedObjecthkpVehicleDefaultSuspension(void* p) {
	static_cast<hkpVehicleDefaultSuspension*>(p)->~hkpVehicleDefaultSuspension();
}

const hkClass& hkpVehicleDefaultSuspension::WheelSpringSuspensionParameters::staticClass() {
	return hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass;
}

// The spring parameters and the base's wheel parameters free themselves.
hkpVehicleDefaultSuspension::~hkpVehicleDefaultSuspension() {}

// Per wheel in contact: a spring force from the compression and a damping force from the closing speed (the compression
// damping while closing, the relaxation damping otherwise or on NaN), scaled by the chassis mass. A wheel without
// contact gets no force.
void hkpVehicleDefaultSuspension::calcSuspension(float deltaTime, const hkpVehicleInstance* vehicle,
	const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo, float* suspensionForceOut) {
	float chassisMass = vehicle->m_entity->m_motion.getMass();
	const hkpVehicleData* data = vehicle->m_data;
	for (int32_t i = 0; i < data->m_numWheels; ++i) {
		const hkpVehicleWheelCollide::CollisionDetectionWheelOutput& wheel = cdInfo[i];
		if (wheel.m_contactBody) {
			const WheelSpringSuspensionParameters& spring = m_wheelSpringParams.m_data[i];
			float lengthDiff = m_wheelParams.m_data[i].m_length - wheel.m_currentSuspensionLength;
			float springForce = lengthDiff * spring.m_strength * wheel.m_suspensionScalingFactor;
			const float closingSpeed = wheel.m_suspensionClosingSpeed;
			const float damping = (closingSpeed < 0.0f) ? spring.m_dampingCompression : spring.m_dampingRelaxation;
			float force = springForce - closingSpeed * damping;
			suspensionForceOut[i] = force * chassisMass;
		} else {
			suspensionForceOut[i] = 0.0f;
		}
	}
}

const hkClass& hkpVehicleDefaultSuspension::staticClass() {
	return hkpVehicleDefaultSuspensionClass;
}
