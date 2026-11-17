#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"
#include "Havok/Source/Physics/ConstraintSolver/Vehiclefriction/hkpVehicleFrictionSolver.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <bit>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpVehicleDataWheelComponentParams(void* p);
static void finishLoadedObjecthkpVehicleData(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc).
static const void* getVtablehkpVehicleData();
static void cleanupLoadedObjecthkpVehicleData(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleDataWheelComponentParamsTypeInfo = {"hkpVehicleDataWheelComponentParams", nullptr,
	cleanupLoadedObjecthkpVehicleDataWheelComponentParams, nullptr, &typeid(hkpVehicleData::WheelComponentParams),
	sizeof(hkpVehicleData::WheelComponentParams)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleData::WheelComponentParams, as the exe's .rdata holds them.
using hkpVehicleDataWheelComponentParamsClass_MembersArray = const hkClassMember[10];
static hkpVehicleDataWheelComponentParamsClass_MembersArray hkpVehicleDataWheelComponentParamsClass_Members{
	{"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"mass", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
	{"width", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
	{"friction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"viscosityFriction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10,
		nullptr},
	{"maxFriction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"slipAngle", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
	{"forceFeedbackMultiplier", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1C,
		nullptr},
	{"maxContactBodyAcceleration", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0},
		0x20, nullptr},
	{"axle", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr}};
hkClass hkpVehicleDataWheelComponentParamsClass("hkpVehicleDataWheelComponentParams", nullptr, 40, nullptr, 0, nullptr,
	0, hkpVehicleDataWheelComponentParamsClass_Members, 10, nullptr, nullptr, 0, 0);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleData, as the exe's .rdata holds them.
using hkpVehicleDataClass_MembersArray = const hkClassMember[19];
static hkpVehicleDataClass_MembersArray hkpVehicleDataClass_Members{
	{"gravity", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
	{"numWheels", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
	{"chassisOrientation", nullptr, nullptr, {hkClassMember::TYPE_ROTATION}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		nullptr},
	{"torqueRollFactor", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x60,
		nullptr},
	{"torquePitchFactor", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x64,
		nullptr},
	{"torqueYawFactor", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x68,
		nullptr},
	{"extraTorqueFactor", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x6C,
		nullptr},
	{"maxVelocityForPositionalFriction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0,
		{0}, 0x70, nullptr},
	{"chassisUnitInertiaYaw", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x74,
		nullptr},
	{"chassisUnitInertiaRoll", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x78,
		nullptr},
	{"chassisUnitInertiaPitch", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x7C,
		nullptr},
	{"frictionEqualizer", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x80,
		nullptr},
	{"normalClippingAngleCos", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x84,
		nullptr},
	{"maxFrictionSolverMassRatio", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0},
		0x88, nullptr},
	{"wheelParams", &hkpVehicleDataWheelComponentParamsClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8C, nullptr},
	{"numWheelsPerAxle", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT8}, 0, {0}, 0x98,
		nullptr},
	{"frictionDescription", &hkpVehicleFrictionDescriptionClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {0}, 0xA4, nullptr},
	{"chassisFrictionInertiaInvDiag", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0,
		{0}, 0x180, nullptr},
	{"alreadyInitialised", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x190,
		nullptr}};

static void cleanupLoadedObjecthkpVehicleDataWheelComponentParams(void* p) {
	;
}

// The type info's finish-loaded-object callback for hkpVehicleData: its finish constructor, inlined.
static void finishLoadedObjecthkpVehicleData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpVehicleData(flag);
	}
}

static const void* getVtablehkpVehicleData() {
	return hkGetVtableOf<hkpVehicleData>();
}

static void cleanupLoadedObjecthkpVehicleData(void* p) {
	static_cast<hkpVehicleData*>(p)->~hkpVehicleData();
}

namespace {
// The PDB's `anonymous-namespace'::hkpVehicleData_DefaultStruct: per member its offset into this struct (-1: no
// default), then the defaults (maxFrictionSolverMassRatio 30.0).
struct hkpVehicleData_DefaultStruct {
	int32_t s_defaultOffsets[19];
	float m_maxFrictionSolverMassRatio;
};
} // namespace
// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkpVehicleData_DefaultStruct hkpVehicleData_Default{
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x4C, -1, -1, -1, -1, -1}, 30.0f};

hkClass hkpVehicleDataClass("hkpVehicleData", &hkReferencedObjectClass, 416, nullptr, 0, nullptr, 0,
	hkpVehicleDataClass_Members, 19, &hkpVehicleData_Default, nullptr, 0, 1);
hkTypeInfo hkpVehicleDataTypeInfo = {"hkpVehicleData", finishLoadedObjecthkpVehicleData,
	cleanupLoadedObjecthkpVehicleData, getVtablehkpVehicleData(), &typeid(hkpVehicleData), sizeof(hkpVehicleData)};

const hkClass& hkpVehicleData::WheelComponentParams::staticClass() {
	return hkpVehicleDataWheelComponentParamsClass;
}

hkpVehicleData::hkpVehicleData()
	: m_numWheels(0), m_torqueRollFactor(0.0f), m_torquePitchFactor(0.0f), m_torqueYawFactor(1.0f),
	  m_extraTorqueFactor(0.0f), m_maxVelocityForPositionalFriction(10.0f), m_chassisUnitInertiaYaw(1.0f),
	  m_chassisUnitInertiaRoll(1.0f), m_chassisUnitInertiaPitch(1.0f), m_frictionEqualizer(0.0f),
	  m_normalClippingAngleCos(0.1f), m_maxFrictionSolverMassRatio(30.0f), m_alreadyInitialised(false) {
	m_gravity.setZero4();
	m_chassisOrientation.m_col0.setZero4();
	m_chassisOrientation.m_col1.setZero4();
	m_chassisOrientation.m_col2.setZero4();
	m_chassisFrictionInertiaInvDiag.setZero4();
}

// The arrays are freed by their own destructors.
hkpVehicleData::~hkpVehicleData() {}

// Not registered: uninitialised reads. The exe builds a hkpVehicleFrictionDescription::Cinfo in a 16-aligned stack
// slot and fills it field by field, but m_wheelPosition[axle] and m_wheelRadius[axle] are written only for the axles
// that carry a wheel (the wheel loop at 0xE39D30); the friction description's builder (0xE593F0) then reads both
// entries of each array (its first instructions divide by m_wheelRadius[1], at Cinfo+0xA0), so a vehicle whose wheels
// all sit on one axle, or with no wheels, reads stack garbage that src cannot reproduce.
void hkpVehicleData::init(const hkArray<hkpVehicleSuspension::SuspensionWheelParameters, hkContainerHeapAllocator>&
							  suspensionWheelParams,
	hkpRigidBody* chassis) {
	hkVector4 orientation;
	hkMatrix3 inertiaInvLocal;
	hkpVehicleFrictionDescription::Cinfo cinfo;
	m_alreadyInitialised.m_bool = 1;
	// The axle count: the highest m_axle + 1 over the wheels.
	int32_t numAxles = 0;
	if (m_numWheels > 0) {
		const WheelComponentParams* wheel = m_wheelParams.m_data;
		int32_t count = m_numWheels;
		do {
			if (wheel->m_axle + 1 > numAxles) {
				numAxles = wheel->m_axle + 1;
			}
			++wheel;
		} while (--count);
	}
	// EXE-INLINED none: hkArray<int8_t>::setSize
	if (numAxles > m_numWheelsPerAxle.m_size) {
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		int32_t capacity = m_numWheelsPerAxle.m_capacityAndFlags & hkArrayBase<int8_t>::CAPACITY_MASK;
		if (capacity < numAxles) {
			int32_t newCapacity = 2 * capacity;
			if (numAxles >= newCapacity) {
				newCapacity = numAxles;
			}
			hkArrayUtil::_reserve(*heap, &m_numWheelsPerAxle, newCapacity, sizeof(int8_t));
		}
		m_numWheelsPerAxle.m_size = numAxles;
	}
	for (int32_t i = 0; i < m_numWheelsPerAxle.m_size; ++i) {
		m_numWheelsPerAxle.m_data[i] = 0;
	}
	for (int32_t i = 0; i < m_numWheels; ++i) {
		++m_numWheelsPerAxle.m_data[m_wheelParams.m_data[i].m_axle];
	}
	// Each quotient is rounded to float (fstp dword) before it is splatted.
	const float yaw = m_torqueYawFactor / m_chassisUnitInertiaYaw;
	const __m128 yawSplat = _mm_set_ss(yaw);
	const float roll = m_torqueRollFactor / m_chassisUnitInertiaRoll;
	const __m128 rollSplat = _mm_set_ss(roll);
	__m128 sum = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rollSplat, rollSplat, 0),
								_mm_and_ps(m_chassisOrientation.m_col1.m_quad, hkVector4_setAbs4_absMask)),
		_mm_mul_ps(_mm_shuffle_ps(yawSplat, yawSplat, 0),
			_mm_and_ps(hkVector4_setAbs4_absMask, m_chassisOrientation.m_col0.m_quad)));
	const float pitch = m_torquePitchFactor / m_chassisUnitInertiaPitch;
	const __m128 pitchSplat = _mm_set_ss(pitch);
	orientation.m_quad = _mm_add_ps(sum,
		_mm_mul_ps(_mm_shuffle_ps(pitchSplat, pitchSplat, 0),
			_mm_and_ps(m_chassisOrientation.m_col2.m_quad, hkVector4_setAbs4_absMask)));
	orientation.m_quad[3] = 1.0f;
	const float invMass = 1.0 / chassis->m_motion.getMass();
	const __m128 invMassSplat = _mm_set_ss(invMass);
	m_chassisFrictionInertiaInvDiag.m_quad =
		_mm_mul_ps(_mm_shuffle_ps(invMassSplat, invMassSplat, 0), orientation.m_quad);
	cinfo.m_chassisCenterOfMass.m_quad = chassis->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad;
	const hkRotation& rotation = chassis->m_motion.m_motionState.m_transform.m_rotation;
	const __m128 diag0 = _mm_set_ss(m_chassisFrictionInertiaInvDiag.m_quad[0]);
	inertiaInvLocal.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(diag0, diag0, 0), rotation.m_col0.m_quad);
	const __m128 diag1 = _mm_set_ss(m_chassisFrictionInertiaInvDiag.m_quad[1]);
	inertiaInvLocal.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(diag1, diag1, 0), rotation.m_col1.m_quad);
	const __m128 diag2 = _mm_set_ss(m_chassisFrictionInertiaInvDiag.m_quad[2]);
	inertiaInvLocal.m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps(diag2, diag2, 0), rotation.m_col2.m_quad);
	cinfo.m_chassisFrictionInertiaInv.setMulInverse(inertiaInvLocal, rotation);
	cinfo.m_chassisMassInv = _mm_shuffle_ps(chassis->m_motion.m_inertiaAndMassInv.m_quad,
		chassis->m_motion.m_inertiaAndMassInv.m_quad, 255)[0];
	cinfo.m_directionUp.m_quad = _mm_and_ps(hkVector4_setAbs4_absMask, m_chassisOrientation.m_col0.m_quad);
	cinfo.m_frictionEqualizer = m_frictionEqualizer;
	cinfo.m_directionFront.m_quad = _mm_and_ps(m_chassisOrientation.m_col1.m_quad, hkVector4_setAbs4_absMask);
	cinfo.m_wheelAxleAngularInertia[0] = 0.0f;
	cinfo.m_wheelAxleAngularInertia[1] = 0.0f;
	cinfo.m_directionRight.m_quad = _mm_and_ps(m_chassisOrientation.m_col2.m_quad, hkVector4_setAbs4_absMask);
	if (m_numWheels > 0) {
		const WheelComponentParams* wheel = m_wheelParams.m_data;
		const hkpVehicleSuspension::SuspensionWheelParameters* suspension = suspensionWheelParams.m_data;
		int32_t count = m_numWheels;
		do {
			const int32_t axle = wheel->m_axle;
			cinfo.m_wheelRadius[axle] = wheel->m_radius;
			const __m128 length = _mm_set_ss(suspension->m_length);
			cinfo.m_wheelPosition[axle].m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(length, length, 0), suspension->m_directionChassisSpace.m_quad),
					suspension->m_hardpointChassisSpace.m_quad);
			cinfo.m_wheelAxleAngularInertia[axle] =
				wheel->m_mass * wheel->m_radius + cinfo.m_wheelAxleAngularInertia[axle];
			++suspension;
			++wheel;
		} while (--count);
	}
	hkVehicleFrictionDescriptionInitValues(cinfo, m_frictionDescription);
}

const hkClass& hkpVehicleData::staticClass() {
	return hkpVehicleDataClass;
}
