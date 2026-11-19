#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/ConstraintSolver/Vehiclefriction/hkpVehicleFrictionSolver.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"
#include "Havok/Source/Physics/Vehicle/Brake/hkpVehicleBrake.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"
#include "Havok/Source/Physics/Vehicle/Steering/hkpVehicleSteering.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"
#include "Havok/Source/Physics/Vehicle/TyreMarks/hkpTyremarksInfo.h"
#include "Havok/Source/Physics/Vehicle/VelocityDamper/hkpVehicleVelocityDamper.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpVehicleFrictionStatus(void* p);
static void cleanupLoadedObjecthkpVehicleInstanceWheelInfo(void* p);
static void finishLoadedObjecthkpVehicleInstance(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
static const void* getVtablehkpVehicleInstance();
static void finishLoadedObjecthkpVehicleFrictionStatus(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpVehicleFrictionStatusAxisStatus(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleInstanceWheelInfoTypeInfo = {
    "hkpVehicleInstanceWheelInfo",
    nullptr,
    cleanupLoadedObjecthkpVehicleInstanceWheelInfo,
    nullptr,
    &typeid(hkpVehicleInstance::WheelInfo),
    sizeof(hkpVehicleInstance::WheelInfo)
};
hkTypeInfo hkpVehicleFrictionStatusAxisStatusTypeInfo = {
    "hkpVehicleFrictionStatusAxisStatus",
    nullptr,
    cleanupLoadedObjecthkpVehicleFrictionStatusAxisStatus,
    nullptr,
    &typeid(hkpVehicleFrictionStatus::AxisStatus),
    sizeof(hkpVehicleFrictionStatus::AxisStatus)
};
hkTypeInfo hkpVehicleFrictionStatusTypeInfo = {
    "hkpVehicleFrictionStatus",
    finishLoadedObjecthkpVehicleFrictionStatus,
    cleanupLoadedObjecthkpVehicleFrictionStatus,
    nullptr,
    &typeid(hkpVehicleFrictionStatus),
    sizeof(hkpVehicleFrictionStatus)
};

// The reflected members of hkpVehicleInstance::WheelInfo.
using hkpVehicleInstanceWheelInfoClass_MembersArray = const hkClassMember[17];
static hkpVehicleInstanceWheelInfoClass_MembersArray hkpVehicleInstanceWheelInfoClass_Members{
    {"contactPoint",
     &hkContactPointClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"contactFriction",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"contactBody",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {0x400},
     0x24,
     nullptr},
    {"contactShapeKey",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     8,
     {0},
     0x28,
     nullptr},
    {"hardPointWs", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr},
    {"rayEndPointWs",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x60,
     nullptr},
    {"currentSuspensionLength",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x70,
     nullptr},
    {"suspensionDirectionWs",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x80,
     nullptr},
    {"spinAxisChassisSpace",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x90,
     nullptr},
    {"spinAxisWs", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xA0, nullptr},
    {"steeringOrientationChassisSpace",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_QUATERNION},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB0,
     nullptr},
    {"spinVelocity", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC0, nullptr},
    {"spinAngle", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC4, nullptr},
    {"skidEnergyDensity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC8,
     nullptr},
    {"sideForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xCC, nullptr},
    {"forwardSlipVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD0,
     nullptr},
    {"sideSlipVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD4,
     nullptr}
};
hkClass hkpVehicleInstanceWheelInfoClass(
    "hkpVehicleInstanceWheelInfo",
    nullptr,
    224,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleInstanceWheelInfoClass_Members,
    17,
    nullptr,
    nullptr,
    0,
    1
);

// The reflected members of hkpVehicleInstance.
using hkpVehicleInstanceClass_MembersArray = const hkClassMember[25];
static hkpVehicleInstanceClass_MembersArray hkpVehicleInstanceClass_Members{
    {"data",
     &hkpVehicleDataClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x1C,
     nullptr},
    {"driverInput",
     &hkpVehicleDriverInputClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x20,
     nullptr},
    {"steering",
     &hkpVehicleSteeringClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x24,
     nullptr},
    {"engine",
     &hkpVehicleEngineClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x28,
     nullptr},
    {"transmission",
     &hkpVehicleTransmissionClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x2C,
     nullptr},
    {"brake",
     &hkpVehicleBrakeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x30,
     nullptr},
    {"suspension",
     &hkpVehicleSuspensionClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x34,
     nullptr},
    {"aerodynamics",
     &hkpVehicleAerodynamicsClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x38,
     nullptr},
    {"wheelCollide",
     &hkpVehicleWheelCollideClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x3C,
     nullptr},
    {"tyreMarks",
     &hkpTyremarksInfoClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x40,
     nullptr},
    {"velocityDamper",
     &hkpVehicleVelocityDamperClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x44,
     nullptr},
    {"wheelsInfo",
     &hkpVehicleInstanceWheelInfoClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x48,
     nullptr},
    {"frictionStatus",
     &hkpVehicleFrictionStatusClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x54,
     nullptr},
    {"deviceStatus",
     &hkpVehicleDriverInputStatusClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x9C,
     nullptr},
    {"isFixed", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_BOOL}, 0, {0}, 0xA0, nullptr},
    {"wheelsTimeSinceMaxPedalInput",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xAC,
     nullptr},
    {"tryingToReverse",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB0,
     nullptr},
    {"torque", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xB4, nullptr},
    {"rpm", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xB8, nullptr},
    {"mainSteeringAngle",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xBC,
     nullptr},
    {"wheelsSteeringAngle",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_REAL},
     0,
     {0},
     0xC0,
     nullptr},
    {"isReversing", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xCC, nullptr},
    {"currentGear", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xCD, nullptr},
    {"delayed", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xCE, nullptr},
    {"clutchDelayCountdown",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD0,
     nullptr}
};
hkClass hkpVehicleInstanceClass(
    "hkpVehicleInstance",
    &hkpUnaryActionClass,
    212,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleInstanceClass_Members,
    25,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleInstanceTypeInfo = {
    "hkpVehicleInstance",          finishLoadedObjecthkpVehicleInstance, cleanupLoadedObjecthkpVehicleInstance,
    getVtablehkpVehicleInstance(), &typeid(hkpVehicleInstance),          sizeof(hkpVehicleInstance)
};

// The reflected members of hkpVehicleFrictionStatus::AxisStatus.
using hkpVehicleFrictionStatusAxisStatusClass_MembersArray = const hkClassMember[9];
static hkpVehicleFrictionStatusAxisStatusClass_MembersArray hkpVehicleFrictionStatusAxisStatusClass_Members{
    {"forward_slip_velocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"side_slip_velocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x4,
     nullptr},
    {"skid_energy_density",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr},
    {"side_force", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"delayed_forward_impulse",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"sideRhs", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"forwardRhs", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
    {"relativeSideForce",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr},
    {"relativeForwardForce",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr}
};
hkClass hkpVehicleFrictionStatusAxisStatusClass(
    "hkpVehicleFrictionStatusAxisStatus",
    nullptr,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleFrictionStatusAxisStatusClass_Members,
    9,
    nullptr,
    nullptr,
    0,
    0
);

// The reflected members of hkpVehicleFrictionStatus.
using hkpVehicleFrictionStatusClass_MembersArray = const hkClassMember[1];
static hkpVehicleFrictionStatusClass_MembersArray hkpVehicleFrictionStatusClass_Members{
    {"axis",
     &hkpVehicleFrictionStatusAxisStatusClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0x0,
     nullptr}
};
hkClass hkpVehicleFrictionStatusClass(
    "hkpVehicleFrictionStatus",
    nullptr,
    72,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleFrictionStatusClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkpVehicleFrictionStatus(void* p) {
    ;
}

static void cleanupLoadedObjecthkpVehicleInstanceWheelInfo(void* p) {
    ;
}

// Havok SDK: finishes a loaded hkpVehicleInstance in place, when there is one.
static void finishLoadedObjecthkpVehicleInstance(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpVehicleInstance(aFlag);
    }
}

static const void* getVtablehkpVehicleInstance() {
    return hkGetVtableOf<hkpVehicleInstance>();
}

static void finishLoadedObjecthkpVehicleFrictionStatus(void* p, int32_t finishing) {
    ;
}

static void cleanupLoadedObjecthkpVehicleFrictionStatusAxisStatus(void* p) {
    ;
}

void cleanupLoadedObjecthkpVehicleInstance(void* p) {
    static_cast<hkpVehicleInstance*>(p)->~hkpVehicleInstance();
}

const hkClass& hkpVehicleFrictionStatus::AxisStatus::staticClass() {
    return hkpVehicleFrictionStatusAxisStatusClass;
}

const hkClass& hkpVehicleFrictionStatus::staticClass() {
    return hkpVehicleFrictionStatusClass;
}

void hkpVehicleInstance::WheelInfo::init() {
    m_contactPoint.m_position.m_quad = hkVector4Zero.m_quad;
    m_contactPoint.m_separatingNormal.m_quad = hkQuadReal0100;
    m_contactPoint.m_separatingNormal.m_quad[3] = 1.0f;
    m_hardPointWs.m_quad = _mm_setzero_ps();
    m_contactFriction = 0.0f;
    m_rayEndPointWs.m_quad = _mm_setzero_ps();
    m_currentSuspensionLength = 0.0f;
    m_suspensionDirectionWs.m_quad = _mm_setzero_ps();
    m_spinAxisWs.m_quad = _mm_setzero_ps();
    m_contactBody = nullptr;
    m_contactShapeKey[0u] = -1;
    m_spinAxisChassisSpace.m_quad = _mm_setr_ps(1.0f, zero, zero, zero);
    m_spinVelocity = 0.0f;
    m_spinAngle = 0.0f;
    m_steeringOrientationChassisSpace.m_vec.m_quad = hkQuadReal0001;
    m_skidEnergyDensity = 0.0f;
    m_sideForce = 0.0f;
    m_forwardSlipVelocity = 0.0f;
    m_sideSlipVelocity = 0.0f;
}

// The compiler's member-wise copy: the vectors as 16-byte moves, the rest member by member, the padding after
// m_contactShapeKey, m_currentSuspensionLength and m_sideSlipVelocity left alone.
hkpVehicleInstance::WheelInfo& hkpVehicleInstance::WheelInfo::operator=(const hkpVehicleInstance::WheelInfo& other) {
    m_contactPoint = other.m_contactPoint;
    m_contactFriction = other.m_contactFriction;
    m_contactBody = other.m_contactBody;
    for (int32_t i = 0; i < MAX_NUM_SHAPE_KEYS; i++) {
        m_contactShapeKey[i] = other.m_contactShapeKey[i];
    }
    m_hardPointWs = other.m_hardPointWs;
    m_rayEndPointWs = other.m_rayEndPointWs;
    m_currentSuspensionLength = other.m_currentSuspensionLength;
    m_suspensionDirectionWs = other.m_suspensionDirectionWs;
    m_spinAxisChassisSpace = other.m_spinAxisChassisSpace;
    m_spinAxisWs = other.m_spinAxisWs;
    m_steeringOrientationChassisSpace = other.m_steeringOrientationChassisSpace;
    m_spinVelocity = other.m_spinVelocity;
    m_spinAngle = other.m_spinAngle;
    m_skidEnergyDensity = other.m_skidEnergyDensity;
    m_sideForce = other.m_sideForce;
    m_forwardSlipVelocity = other.m_forwardSlipVelocity;
    m_sideSlipVelocity = other.m_sideSlipVelocity;
    return *this;
}

const hkClass& hkpVehicleInstance::WheelInfo::staticClass() {
    return hkpVehicleInstanceWheelInfoClass;
}

hkpVehicleInstance::hkpVehicleInstance(hkpRigidBody* chassis)
  : hkpUnaryAction(chassis, 0), m_data(nullptr), m_driverInput(nullptr), m_steering(nullptr), m_engine(nullptr),
    m_transmission(nullptr), m_brake(nullptr), m_suspension(nullptr), m_aerodynamics(nullptr), m_wheelCollide(nullptr),
    m_tyreMarks(nullptr), m_velocityDamper(nullptr), m_frictionStatus{}, m_deviceStatus(nullptr) {}

// Releases the components in the game's order; the arrays are freed by their own destructors.
hkpVehicleInstance::~hkpVehicleInstance() {
    if (m_data) {
        m_data->removeReference();
    }
    if (m_driverInput) {
        m_driverInput->removeReference();
    }
    if (m_steering) {
        m_steering->removeReference();
    }
    if (m_engine) {
        m_engine->removeReference();
    }
    if (m_transmission) {
        m_transmission->removeReference();
    }
    if (m_brake) {
        m_brake->removeReference();
    }
    if (m_suspension) {
        m_suspension->removeReference();
    }
    if (m_aerodynamics) {
        m_aerodynamics->removeReference();
    }
    if (m_wheelCollide) {
        m_wheelCollide->removeReference();
    }
    if (m_velocityDamper) {
        m_velocityDamper->removeReference();
    }
    if (m_deviceStatus) {
        m_deviceStatus->removeReference();
    }
    if (m_tyreMarks) {
        m_tyreMarks->removeReference();
    }
}

void hkpVehicleInstance::applyAction(const hkStepInfo& stepInfo) {
    stepVehicle(stepInfo);
}

void hkpVehicleInstance::getPhantoms(hkArray<hkpPhantom*, hkContainerHeapAllocator>& phantomsOut) {
    m_wheelCollide->getPhantoms(phantomsOut);
}

hkpAction* hkpVehicleInstance::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    hkpRigidBody* chassis = static_cast<hkpRigidBody*>(*newEntities.m_data);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpVehicleInstance* v6 = static_cast<hkpVehicleInstance*>(Instance->m_heap->blockAlloc(sizeof(hkpVehicleInstance)));
    v6->m_memSizeAndFlags = sizeof(hkpVehicleInstance);
    hkpVehicleInstance* hkpVehicleInstance_ = ::new (v6) hkpVehicleInstance(chassis);
    *hkpVehicleInstance_ = *this;
    hkpVehicleInstance_->m_world = nullptr;
    hkpVehicleInstance_->m_island = nullptr;
    hkpVehicleInstance_->m_entity = chassis;
    hkpVehicleWheelCollide* m_wheelCollide = this->m_wheelCollide->clone(*chassis, newPhantoms);
    hkpVehicleInstance_->m_wheelCollide = m_wheelCollide;
    m_wheelCollide->m_alreadyUsed.m_bool = 1;
    hkpVehicleDriverInputStatus* m_deviceStatus = this->m_deviceStatus->clone();
    hkpVehicleData* m_data = hkpVehicleInstance_->m_data;
    hkpVehicleInstance_->m_deviceStatus = m_deviceStatus;
    m_data->addReference();
    hkpVehicleInstance_->m_driverInput->addReference();
    hkpVehicleInstance_->m_steering->addReference();
    hkpVehicleInstance_->m_engine->addReference();
    hkpVehicleInstance_->m_transmission->addReference();
    hkpVehicleInstance_->m_brake->addReference();
    hkpVehicleInstance_->m_suspension->addReference();
    hkpVehicleInstance_->m_aerodynamics->addReference();
    hkpVehicleInstance_->m_velocityDamper->addReference();
    if (m_tyreMarks) {
        hkpVehicleInstance_->m_tyreMarks->addReference();
    }
    return hkpVehicleInstance_;
}

// Initialises the data (once), gives the chassis the data's unit inertia scaled by its mass, sizes the per-wheel arrays
// to the wheel count and resets them, clears the drive state and initialises the wheel collide. When the arrays are
// sized, the heap is fetched every time, and the storage grows to the larger of the wheel count and twice the capacity.
void hkpVehicleInstance::init() {
    hkpVehicleData* data = m_data;
    if (!data->m_alreadyInitialised.m_bool) {
        data->init(m_suspension->m_wheelParams, static_cast<hkpRigidBody*>(m_entity));
    }
    const hkpVehicleData* d = m_data;
    hkVector4 absCol0;
    absCol0.setAbs4(d->m_chassisOrientation.m_col0);
    hkVector4 absCol1;
    absCol1.setAbs4(d->m_chassisOrientation.m_col1);
    hkVector4 absCol2;
    absCol2.setAbs4(d->m_chassisOrientation.m_col2);
    hkVector4 diagonal;
    diagonal.setMul4(d->m_chassisUnitInertiaYaw, absCol0);
    diagonal.addMul4(d->m_chassisUnitInertiaRoll, absCol1);
    diagonal.addMul4(d->m_chassisUnitInertiaPitch, absCol2);
    diagonal.mul4(m_entity->m_motion.getMass());
    hkMatrix3 inertia;
    inertia.setDiagonal(diagonal(0), diagonal(1), diagonal(2));
    const float chassisMass = m_entity->m_motion.getMass();
    static_cast<hkpRigidBody*>(m_entity)->setMass(chassisMass);
    static_cast<hkpRigidBody*>(m_entity)->setInertiaLocal(inertia);

    // _reserve takes the arrays as untyped storage (hkArrayBase<void>).
    // EXE-INLINED none: hkArray<WheelInfo>::setSize
    int32_t numWheels = m_data->m_numWheels;
    hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
    int32_t capacity = static_cast<int32_t>(m_wheelsInfo.m_capacityAndFlags & hkArrayBase<WheelInfo>::CAPACITY_MASK);
    if (capacity < numWheels) {
        capacity *= 2;
        if (numWheels >= capacity) {
            capacity = numWheels;
        }
        hkArrayUtil::_reserve(heap, reinterpret_cast<hkArrayBase<void>*>(&m_wheelsInfo), capacity, sizeof(WheelInfo));
    }
    m_wheelsInfo.m_size = static_cast<uint32_t>(numWheels);
    for (int32_t i = 0; i < m_wheelsInfo.m_size; ++i) {
        m_wheelsInfo.m_data[i].init();
    }

    // EXE-INLINED none: hkArray<hkBool>::setSize
    numWheels = m_data->m_numWheels;
    heap = hkMemoryRouter::getInstance().m_heap;
    capacity = static_cast<int32_t>(m_isFixed.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK);
    if (capacity < numWheels) {
        capacity *= 2;
        if (numWheels >= capacity) {
            capacity = numWheels;
        }
        hkArrayUtil::_reserve(heap, reinterpret_cast<hkArrayBase<void>*>(&m_isFixed), capacity, sizeof(hkBool));
    }
    m_isFixed.m_size = static_cast<uint32_t>(numWheels);
    for (int32_t i = 0; i < m_isFixed.m_size; ++i) {
        m_isFixed.m_data[i].m_bool = false;
    }

    m_wheelsTimeSinceMaxPedalInput = 0.0f;
    m_mainSteeringAngle = 0.0f;

    m_wheelsSteeringAngle.setSize(m_data->m_numWheels);
    for (int32_t i = 0; i < m_wheelsSteeringAngle.m_size; ++i) {
        m_wheelsSteeringAngle.m_data[i] = 0.0f;
    }

    m_torque = 0.0f;
    m_rpm = 0.0f;
    m_isReversing.m_bool = false;
    m_currentGear = 0;
    m_delayed.m_bool = false;
    m_clutchDelayCountdown = 0.0f;
    m_wheelCollide->init(this);
    m_wheelCollide->m_alreadyUsed.m_bool = true;
}

void hkpVehicleInstance::addToWorld(hkpWorld* world) {
    world->addEntity(m_entity, HK_ENTITY_ACTIVATION_DO_ACTIVATE);
    m_wheelCollide->addToWorld(world);
}

void hkpVehicleInstance::removeFromWorld() {
    m_wheelCollide->removeFromWorld();
    m_entity->m_world->removeEntity(m_entity);
}

// The SDK's body: the wheel's rotation is the chassis's, then the wheel's steering, then its spin about its axle;
// its position is the hardpoint in world space moved down the suspension by its length, never less than 0.
void hkpVehicleInstance::calcCurrentPositionAndRotation(
    const hkpRigidBody* chassis,
    const hkpVehicleSuspension* suspension,
    int32_t wheelNo,
    hkVector4& posOut,
    hkQuaternion& rotOut
) {
    WheelInfo& wheelInfo = m_wheelsInfo.m_data[wheelNo];
    hkQuaternion spinRotation;
    spinRotation.setAxisAngle(wheelInfo.m_spinAxisChassisSpace, -wheelInfo.m_spinAngle);
    hkQuaternion chassisAndSteering;
    chassisAndSteering.setMul(
        chassis->m_motion.m_motionState.m_sweptTransform.m_rotation1, wheelInfo.m_steeringOrientationChassisSpace
    );
    rotOut.setMul(chassisAndSteering, spinRotation);
    float suspensionLength = wheelInfo.m_currentSuspensionLength;
    if (suspensionLength < 0.0) {
        suspensionLength = 0.0f;
    }
    posOut.setTransformedPos(
        chassis->m_motion.m_motionState.m_transform, suspension->m_wheelParams.m_data[wheelNo].m_hardpointChassisSpace
    );
    posOut.addMul4(suspensionLength, wheelInfo.m_suspensionDirectionWs);
}

float hkpVehicleInstance::calcRPM() const {
    return m_rpm;
}

float hkpVehicleInstance::calcKMPH() const {
    __m128 v1 = _mm_mul_ps(m_entity->m_motion.m_linearVelocity.m_quad, m_entity->m_motion.m_linearVelocity.m_quad);
    return _mm_cvtss_f32(
               _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v1, v1, 170)[0] + (_mm_shuffle_ps(v1, v1, 85)[0] + v1[0]))))
           ) /
        1000.0 * 3600.0;
}

float hkpVehicleInstance::calcMPH() const {
    __m128 v1 = _mm_mul_ps(m_entity->m_motion.m_linearVelocity.m_quad, m_entity->m_motion.m_linearVelocity.m_quad);
    return _mm_cvtss_f32(
               _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v1, v1, 170)[0] + (_mm_shuffle_ps(v1, v1, 85)[0] + v1[0]))))
           ) /
        1609.300048828125 * 3600.0;
}

void hkpVehicleInstance::handleFixedGroundAccum(hkpRigidBody* ground, hkpVelocityAccumulator& accum) {
    accum.m_gravityFactor = 0.0f;
    accum.m_type.m_storage = 1;
    accum.m_invMasses.m_quad = _mm_setzero_ps();
    accum.m_linearVel.m_quad = _mm_setzero_ps();
    accum.m_angularVel.m_quad = _mm_setzero_ps();
    accum.m_scratch0.m_quad = _mm_setzero_ps();
    accum.m_scratch123.m_col0.m_quad = _mm_setzero_ps();
    accum.m_scratch123.m_col1.m_quad = _mm_setzero_ps();
    accum.m_scratch123.m_col2.m_quad = _mm_setzero_ps();
}

// The wheels' collision output goes into an hkLocalArray over the thread's stack allocator.
void hkpVehicleInstance::stepVehicle(const hkStepInfo& stepInfo) {
    updateBeforeCollisionDetection();

    using WheelOutput = hkpVehicleWheelCollide::CollisionDetectionWheelOutput;
    const int32_t numWheels = m_data->m_numWheels;
    hkLocalArray<WheelOutput> cdInfo(numWheels);

    float deltaTime = stepInfo.m_deltaTime;
    m_wheelCollide->collideWheels(deltaTime, this, cdInfo.m_data);
    stepVehicleUsingWheelCollideOutput(stepInfo, cdInfo.m_data);
}

void hkpVehicleInstance::updateBeforeCollisionDetection() {
    HK_TIMER_BEGIN("UpdateBeforeCD", nullptr);
    const hkMotionState* motionState = m_entity->m_motion.getMotionState();
    for (int32_t w_it = 0; w_it < m_data->m_numWheels; ++w_it) {
        hkpVehicleInstance::WheelInfo& wheelInfo = m_wheelsInfo.m_data[w_it];
        wheelInfo.m_suspensionDirectionWs.setRotatedDir(
            motionState->m_transform.m_rotation, m_suspension->m_wheelParams.m_data[w_it].m_directionChassisSpace
        );
        wheelInfo.m_hardPointWs.setTransformedPos(
            motionState->m_transform, m_suspension->m_wheelParams.m_data[w_it].m_hardpointChassisSpace
        );
        // The ray's length is rounded to a float before it is splatted.
        float rayLength =
            m_suspension->m_wheelParams.m_data[w_it].m_length + m_data->m_wheelParams.m_data[w_it].m_radius;
        wheelInfo.m_rayEndPointWs.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(rayLength), _mm_set_ss(rayLength), 0),
                wheelInfo.m_suspensionDirectionWs.m_quad
            ),
            wheelInfo.m_hardPointWs.m_quad
        );
    }
    m_wheelCollide->updateBeforeCollisionDetection(this);
    HK_TIMER_END();
}

// The per-wheel forces go into two hkInplaceArrays of 16 floats.
void hkpVehicleInstance::stepVehicleUsingWheelCollideOutput(
    const hkStepInfo& stepInfo,
    const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo
) {
    HK_TIMER_BEGIN("DoVehicle", nullptr);
    alignas(16) hkInplaceArray<float, 16> suspensionForceAtWheel;
    suspensionForceAtWheel.m_size = m_data->m_numWheels;
    suspensionForceAtWheel.m_data = suspensionForceAtWheel.m_storage;
    suspensionForceAtWheel.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG | 16;
    alignas(16) hkInplaceArray<float, 16> totalLinearForceAtWheel;
    totalLinearForceAtWheel.m_size = m_data->m_numWheels;
    totalLinearForceAtWheel.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG | 16;
    totalLinearForceAtWheel.m_data = totalLinearForceAtWheel.m_storage;
    hkpVehicleAerodynamics::AerodynamicsDragOutput aerodynamicsDragInfo;

    updateComponents(stepInfo, cdInfo, aerodynamicsDragInfo, suspensionForceAtWheel, totalLinearForceAtWheel);
    simulateVehicle(stepInfo, aerodynamicsDragInfo, suspensionForceAtWheel, totalLinearForceAtWheel);
    HK_TIMER_END();
}

// The steering output starts from the current angles: its wheel angle array (no storage, not ours) is sized and filled
// inline, calcSteering runs through the vtable, and the angles are copied back. The compares are signed.
void hkpVehicleInstance::updateSteering(
    float deltaTime,
    const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo
) {
    const int32_t numWheels = m_wheelsSteeringAngle.m_size;
    hkpVehicleSteering::SteeringAnglesOutput steeringOutput;
    steeringOutput.m_mainSteeringAngle = m_mainSteeringAngle;
    steeringOutput.m_wheelsSteeringAngle.setSize(numWheels);
    for (int32_t i = 0; i < m_wheelsSteeringAngle.m_size; ++i) {
        steeringOutput.m_wheelsSteeringAngle.m_data[i] = m_wheelsSteeringAngle.m_data[i];
    }

    m_steering->calcSteering(deltaTime, this, filteredDriverInputInfo, steeringOutput);

    m_mainSteeringAngle = steeringOutput.m_mainSteeringAngle;
    for (int32_t i = 0; i < m_wheelsSteeringAngle.m_size; ++i) {
        m_wheelsSteeringAngle.m_data[i] = steeringOutput.m_wheelsSteeringAngle.m_data[i];
    }
}

void hkpVehicleInstance::updateDriverInput(
    float deltaTime,
    hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo
) {
    filteredDriverInputInfo.m_tryingToReverse.m_bool = m_tryingToReverse.m_bool;
    m_driverInput->calcDriverInput(deltaTime, this, m_deviceStatus, filteredDriverInputInfo);
    m_tryingToReverse.m_bool = filteredDriverInputInfo.m_tryingToReverse.m_bool;
}

void hkpVehicleInstance::updateTransmission(
    float deltaTime,
    hkpVehicleTransmission::TransmissionOutput& transmissionInfo
) {
    transmissionInfo.m_numWheelsTramsmittedTorque = m_data->m_numWheels;
    transmissionInfo.m_isReversing.m_bool = m_isReversing.m_bool;
    transmissionInfo.m_currentGear = m_currentGear;
    transmissionInfo.m_delayed.m_bool = m_delayed.m_bool;
    transmissionInfo.m_clutchDelayCountdown = m_clutchDelayCountdown;
    m_transmission->calcTransmission(deltaTime, this, transmissionInfo);
    m_isReversing.m_bool = transmissionInfo.m_isReversing.m_bool;
    m_currentGear = transmissionInfo.m_currentGear;
    m_delayed.m_bool = transmissionInfo.m_delayed.m_bool;
    m_clutchDelayCountdown = transmissionInfo.m_clutchDelayCountdown;
}

void hkpVehicleInstance::updateEngine(
    float deltaTime,
    const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
    const hkpVehicleTransmission::TransmissionOutput& transmissionInfo
) {
    hkpVehicleEngine::EngineOutput engineOutput;
    engineOutput.m_rpm = m_rpm;
    engineOutput.m_torque = m_torque;
    m_engine->calcEngineInfo(deltaTime, this, filteredDriverInputInfo, transmissionInfo, engineOutput);
    m_rpm = engineOutput.m_rpm;
    m_torque = engineOutput.m_torque;
}

void hkpVehicleInstance::applySuspensionForcesAtWheel(float deltaTime, int32_t w_it, float suspensionForceAtWheel) {
    uint32_t v12;
    uint32_t v13;
    uint32_t v19;
    hkVector4 imp_;
    int32_t w_it_1 = w_it;
    *reinterpret_cast<float*>(&v12) = deltaTime * suspensionForceAtWheel;
    hkpVehicleInstance::WheelInfo* v6 = &m_wheelsInfo.m_data[w_it];
    __m128 m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v12)), _mm_set_ss(std::bit_cast<float>(v12)), 0),
        v6->m_contactPoint.m_separatingNormal.m_quad
    );
    imp_.m_quad = m_quad;
    if (suspensionForceAtWheel > 0.0f) {
        hkpEntity* m_entity = this->m_entity;
        m_entity->activate();
        m_entity->m_motion.applyPointImpulse(imp_, v6->m_hardPointWs);
        m_quad = imp_.m_quad;
        w_it_1 = w_it;
    }
    hkpRigidBody* m_contactBody = v6->m_contactBody;
    if (m_contactBody) {
        hkpMaxSizeMotion* p_m_motion = &m_contactBody->m_motion;
        *reinterpret_cast<float*>(&v13) = -m_data->m_wheelParams.m_data[w_it_1].m_forceFeedbackMultiplier;
        imp_.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v13)), _mm_set_ss(std::bit_cast<float>(v13)), 0), m_quad
        );
        float Mass = m_contactBody->m_motion.getMass();
        __m128 v11 = _mm_mul_ps(imp_.m_quad, imp_.m_quad);
        float v15 = m_data->m_wheelParams.m_data[w_it_1].m_maxContactBodyAcceleration * Mass * deltaTime;
        float v16 = _mm_shuffle_ps(v11, v11, 170)[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]);
        if (v16 > v15 * v15) {
            float v17 = std::sqrt((double)v16);
            float v18 = 1.0 / v17;
            *reinterpret_cast<float*>(&v19) = v18 * v15;
            imp_.m_quad = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v19)), _mm_set_ss(std::bit_cast<float>(v19)), 0),
                imp_.m_quad
            );
        }
        p_m_motion->applyPointImpulse(imp_, v6->m_hardPointWs);
    }
}

void hkpVehicleInstance::perWheelBehavior(
    float deltaTime,
    const hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
    const hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel,
    hkpRigidBody** groundBody,
    hkpVehicleFrictionSolverAxleParams* axleParams
) {
    hkpVehicleData* m_data = this->m_data;
    __m128 v8 = _mm_mul_ps(m_entity->m_motion.m_linearVelocity.m_quad, m_entity->m_motion.m_linearVelocity.m_quad);
    int32_t w_it = 0;
    float estimatedCarSpeed = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]))))
    );
    if (m_data->m_numWheels > 0) {
        int32_t v14 = 0;
        int32_t v13 = 0;
        do {
            int32_t m_axle = m_data->m_wheelParams.m_data[v13].m_axle;
            hkpVehicleInstance::WheelInfo* v15 = &m_wheelsInfo.m_data[v14];
            applySuspensionForcesAtWheel(deltaTime, w_it, suspensionForceAtWheel.m_data[w_it]);
            hkpRigidBody* m_contactBody = v15->m_contactBody;
            if (m_contactBody) {
                hkpRigidBody* v12 = groundBody[m_axle];
                if (!v12 ||
                    _mm_shuffle_ps(
                        v12->m_motion.m_inertiaAndMassInv.m_quad, v12->m_motion.m_inertiaAndMassInv.m_quad, 255
                    )[0] >
                        static_cast<double>(_mm_shuffle_ps(
                            m_contactBody->m_motion.m_inertiaAndMassInv.m_quad,
                            m_contactBody->m_motion.m_inertiaAndMassInv.m_quad, 255
                        )[0])) {
                    groundBody[m_axle] = m_contactBody;
                }
            }
            getAxleParamsFromWheel(
                w_it, totalLinearForceAtWheel.m_data[w_it], suspensionForceAtWheel.m_data[w_it], estimatedCarSpeed,
                axleParams[m_axle]
            );
            m_data = this->m_data;
            ++v13;
            ++v14;
            ++w_it;
        }
        while (w_it < m_data->m_numWheels);
    }
}

void hkpVehicleInstance::updateWheels(
    float deltaTime,
    const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo
) {
    const hkMotionState* motionState = m_entity->m_motion.getMotionState();
    for (int32_t w_it = 0; w_it < m_data->m_numWheels; ++w_it) {
        hkpVehicleInstance::WheelInfo& wheelInfo = m_wheelsInfo.m_data[w_it];
        const hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdWheelInfo = cdInfo[w_it];
        wheelInfo.m_currentSuspensionLength = cdWheelInfo.m_currentSuspensionLength;
        wheelInfo.m_contactPoint = cdWheelInfo.m_contactPoint;
        wheelInfo.m_contactFriction = cdWheelInfo.m_contactFriction;
        wheelInfo.m_contactBody = cdWheelInfo.m_contactBody;
        for (int32_t i = 0; i < hkpVehicleWheelCollide::CollisionDetectionWheelOutput::MAX_NUM_SHAPE_KEYS; ++i) {
            wheelInfo.m_contactShapeKey[i] = cdWheelInfo.m_contactShapeKey[i];
        }
        // The steering rotation: the axis scaled by half the angle, w = 1, normalized over four lanes.
        float halfAngle = m_wheelsSteeringAngle.m_data[w_it] * 0.5;
        hkVector4 steering;
        steering.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(halfAngle), _mm_set_ss(halfAngle), 0),
            m_suspension->m_wheelParams.m_data[w_it].m_directionChassisSpace.m_quad
        );
        steering(3) = 1.0f;
        steering.normalize4();
        wheelInfo.m_steeringOrientationChassisSpace.m_vec = steering;
        wheelInfo.m_spinAxisChassisSpace = m_data->m_chassisOrientation.m_col2;
        hkVector4 spinAxisCs;
        spinAxisCs.setRotatedDir(wheelInfo.m_steeringOrientationChassisSpace, wheelInfo.m_spinAxisChassisSpace);
        wheelInfo.m_spinAxisWs.setRotatedDir(motionState->m_transform.m_rotation, spinAxisCs);
    }
}

// The SDK's body: for each axle standing on a moving body (only once where both axles stand on the same one), the
// velocities the friction solver left in the ground's accumulator, the angular one turned back from the
// accumulator's core frame to world space, each moved by at most deltaTime * 10 per lane from the body's current
// velocity, and set on the body's motion through its vtable.
void hkpVehicleInstance::applyResultsToGround(
    float deltaTime,
    const hkpVehicleFrictionSolverAxleParams* axleParams,
    hkpRigidBody** groundBody
) {
    for (int32_t ax_it = 0; ax_it < m_data->m_numWheelsPerAxle.m_size; ++ax_it) {
        hkpRigidBody* body = groundBody[ax_it];
        if (!body || body->m_motion.m_type == hkpMotion::MOTION_FIXED) {
            continue;
        }
        if (ax_it != 0 && groundBody[0] == groundBody[1]) {
            continue;
        }
        hkpMotion* motion = body->getMotion();
        const hkpVelocityAccumulator* accum = axleParams[ax_it].m_groundObject;
        const float maxVelocityChange = deltaTime * 10.0;
        hkVector4 angularVel;
        angularVel._setRotatedInverseDir(accum->m_scratch123, accum->m_angularVel);
        hkVector4 linearVel = accum->m_linearVel;
        const hkQuadReal maxChange = _mm_shuffle_ps(_mm_set_ss(maxVelocityChange), _mm_set_ss(maxVelocityChange), 0);
        const hkQuadReal minChange = _mm_xor_ps(maxChange, hkQuadRealNegateMask);
        const hkQuadReal oldAngularVel = motion->m_angularVelocity.m_quad;
        angularVel.m_quad = _mm_add_ps(
            _mm_max_ps(_mm_min_ps(_mm_sub_ps(angularVel.m_quad, oldAngularVel), maxChange), minChange), oldAngularVel
        );
        const hkQuadReal oldLinearVel = motion->m_linearVelocity.m_quad;
        linearVel.m_quad = _mm_add_ps(
            _mm_max_ps(_mm_min_ps(_mm_sub_ps(linearVel.m_quad, oldLinearVel), maxChange), minChange), oldLinearVel
        );
        motion->setAngularVelocity(angularVel);
        motion->setLinearVelocity(linearVel);
    }
}

void hkpVehicleInstance::simulateVehicle(
    const hkStepInfo& stepInfo,
    const hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo,
    const hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
    const hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel
) {
    HK_TIMER_BEGIN("SimulateVehicle", nullptr);
    const float deltaTime = stepInfo.m_deltaTime;

    // EXE-INLINED none: hkpRigidBody::applyForce
    hkpEntity* chassis = m_entity;
    chassis->activate();
    chassis->getMotion()->applyForce(deltaTime, aerodynamicsDragInfo.m_aerodynamicsForce);
    // EXE-INLINED none: hkpRigidBody::applyTorque
    chassis = m_entity;
    chassis->activate();
    chassis->getMotion()->applyTorque(deltaTime, aerodynamicsDragInfo.m_aerodynamicsTorque);

    if (m_velocityDamper) {
        m_velocityDamper->applyVelocityDamping(deltaTime, *this);
    }

    hkpVehicleFrictionSolverParams frictionParams;
    for (int32_t i = 0; i < 2; ++i) {
        // EXE-INLINED none: hkpVehicleFrictionSolverAxleParams::initialize
        hkpVehicleFrictionSolverAxleParams& axleParams = frictionParams.m_axleParams[i];
        axleParams.m_contactPointWs.setZero4();
        axleParams.m_constraintNormalWs.setZero4();
        axleParams.m_forwardDirWs.setZero4();
        axleParams.m_frictionCoefficient = 0.0f;
        axleParams.m_viscosityFrictionCoefficient = 0.0f;
        axleParams.m_maxFrictionCoefficient = 0.0f;
        axleParams.m_wheelDownForce = 0.0f;
        axleParams.m_forwardForce = 0.0f;
        axleParams.m_wheelFixed = 0;
        axleParams.m_groundObject = nullptr;
        axleParams.m_groundObjectAtLastIntegration = nullptr;
        axleParams.m_slipVelocityFactor = 0.0f;
    }

    hkpRigidBody* groundBody[2];
    groundBody[0] = nullptr;
    groundBody[1] = nullptr;
    perWheelBehavior(
        deltaTime, suspensionForceAtWheel, totalLinearForceAtWheel, groundBody, frictionParams.m_axleParams
    );

    frictionParams.m_maxVelocityForPositionalFriction = m_data->m_maxVelocityForPositionalFriction;
    hkpVelocityAccumulator groundAccum[2];
    hkpVelocityAccumulator groundAccumAtIntegration[2];
    getAxleParams(stepInfo, groundBody, groundAccum, groundAccumAtIntegration, frictionParams.m_axleParams);
    prepareFrictionSolverParams(stepInfo, frictionParams);
    getExtraTorqueFactor(deltaTime, frictionParams.m_chassis);
    applyFrictionSolver(stepInfo, frictionParams);
    applyResultsToChassis(frictionParams.m_chassis);
    applyResultsToGround(deltaTime, frictionParams.m_axleParams, groundBody);
    applyResultsToWheelInfo(deltaTime, groundBody, frictionParams);
    HK_TIMER_END();
}

void hkpVehicleInstance::updateBrake(
    float deltaTime,
    const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
    hkpVehicleBrake::WheelBreakingOutput& wheelBreakingInfo
) {
    int32_t m_size = m_isFixed.m_size;
    hkArray<hkBool, hkContainerHeapAllocator>* p_m_isFixed = &wheelBreakingInfo.m_isFixed;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v8 = wheelBreakingInfo.m_isFixed.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK;
    if (v8 < m_size) {
        int32_t v9 = 2 * v8;
        if (m_size >= v9) {
            v9 = m_size;
        }
        hkArrayUtil::_reserve(m_heap, p_m_isFixed, v9, sizeof(hkBool));
    }
    wheelBreakingInfo.m_isFixed.m_size = m_size;
    int32_t v10 = m_isFixed.m_size;
    hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v12 = wheelBreakingInfo.m_brakingTorque.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (v12 < v10) {
        int32_t v13 = 2 * v12;
        if (v10 >= v13) {
            v13 = v10;
        }
        hkArrayUtil::_reserve(v11, &wheelBreakingInfo.m_brakingTorque, v13, sizeof(float));
    }
    int32_t v14 = 0;
    for (wheelBreakingInfo.m_brakingTorque.m_size = v10; v14 < m_isFixed.m_size; ++v14) {
        p_m_isFixed->m_data[v14].m_bool = m_isFixed.m_data[v14].m_bool;
    }
    wheelBreakingInfo.m_wheelsTimeSinceMaxPedalInput = m_wheelsTimeSinceMaxPedalInput;
    m_brake->calcBreakingInfo(deltaTime, this, filteredDriverInputInfo, wheelBreakingInfo);
    for (int32_t i = 0; i < wheelBreakingInfo.m_isFixed.m_size; ++i) {
        m_isFixed.m_data[i].m_bool = p_m_isFixed->m_data[i].m_bool;
    }
    m_wheelsTimeSinceMaxPedalInput = wheelBreakingInfo.m_wheelsTimeSinceMaxPedalInput;
}

void hkpVehicleInstance::updateAerodynamics(
    float deltaTime,
    hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo
) {
    m_aerodynamics->calcAerodynamics(deltaTime, this, aerodynamicsDragInfo);
}

void hkpVehicleInstance::setChassis(hkpRigidBody* chassis) {
    setEntity(chassis);
}

// The transmission's per-wheel torques go into a block of the thread's stack allocator, which is taken and given back
// within this function.
void hkpVehicleInstance::updateComponents(
    const hkStepInfo& stepInfo,
    const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo,
    hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo,
    hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
    hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel
) {
    HK_TIMER_BEGIN("UpdateComponents", nullptr);
    hkpVehicleDriverInput::FilteredDriverInputOutput filteredDriverInputInfo;
    hkpVehicleTransmission::TransmissionOutput transmissionInfo;
    hkpVehicleBrake::WheelBreakingOutput wheelBreakingInfo;

    const int32_t numWheels = m_data->m_numWheels;
    transmissionInfo.m_wheelsTransmittedTorque = hkAllocateStack<float>(numWheels, nullptr);
    float deltaTime = stepInfo.m_deltaTime;

    updateWheels(deltaTime, cdInfo);
    updateDriverInput(deltaTime, filteredDriverInputInfo);
    updateSteering(deltaTime, filteredDriverInputInfo);
    updateTransmission(deltaTime, transmissionInfo);
    updateEngine(deltaTime, filteredDriverInputInfo, transmissionInfo);
    updateBrake(deltaTime, filteredDriverInputInfo, wheelBreakingInfo);
    updateSuspension(deltaTime, cdInfo, suspensionForceAtWheel);
    updateAerodynamics(deltaTime, aerodynamicsDragInfo);

    // Loads the vehicle data, the force array's data and the wheel parameters once, and the wheel count on each pass.
    const hkpVehicleData* data = m_data;
    float* linearForces = totalLinearForceAtWheel.m_data;
    const hkpVehicleData::WheelComponentParams* wheelParams = data->m_wheelParams.m_data;
    for (int32_t w_it = 0; w_it < data->m_numWheels; ++w_it) {
        // The wheel's total torque is rounded to a float before it is divided.
        float totalTorque =
            wheelBreakingInfo.m_brakingTorque.m_data[w_it] + transmissionInfo.m_wheelsTransmittedTorque[w_it];
        linearForces[w_it] = totalTorque / wheelParams[w_it].m_radius;
    }

    hkDeallocateStack(transmissionInfo.m_wheelsTransmittedTorque);
    HK_TIMER_END();
}

// Member-wise copy. The arrays are copied with hkArray's copyFromArray: for the wheels (a class element type) _reserve,
// WheelInfo::operator= on the shared elements and the copy constructor for the new ones; for the plain-data arrays a
// new storage of the source's size where the old one is smaller, then the elements.
hkpVehicleInstance& hkpVehicleInstance::operator=(const hkpVehicleInstance& other) {
    m_memSizeAndFlags = other.m_memSizeAndFlags;
    m_referenceCount = other.m_referenceCount;
    m_world = other.m_world;
    m_island = other.m_island;
    m_userData = other.m_userData;
    m_name = other.m_name;
    m_entity = other.m_entity;
    m_data = other.m_data;
    m_driverInput = other.m_driverInput;
    m_steering = other.m_steering;
    m_engine = other.m_engine;
    m_transmission = other.m_transmission;
    m_brake = other.m_brake;
    m_suspension = other.m_suspension;
    m_aerodynamics = other.m_aerodynamics;
    m_wheelCollide = other.m_wheelCollide;
    m_tyreMarks = other.m_tyreMarks;
    m_velocityDamper = other.m_velocityDamper;

    m_wheelsInfo = other.m_wheelsInfo;

    m_frictionStatus = other.m_frictionStatus;
    m_deviceStatus = other.m_deviceStatus;
    m_isFixed.copyFromArray(hkContainerHeapAllocator().get(&m_isFixed), other.m_isFixed, hkTypeIsPod());
    m_wheelsTimeSinceMaxPedalInput = other.m_wheelsTimeSinceMaxPedalInput;
    m_tryingToReverse.m_bool = other.m_tryingToReverse.m_bool;
    m_torque = other.m_torque;
    m_rpm = other.m_rpm;
    m_mainSteeringAngle = other.m_mainSteeringAngle;

    m_wheelsSteeringAngle = other.m_wheelsSteeringAngle;

    m_isReversing.m_bool = other.m_isReversing.m_bool;
    m_currentGear = other.m_currentGear;
    m_delayed.m_bool = other.m_delayed.m_bool;
    m_clutchDelayCountdown = other.m_clutchDelayCountdown;
    return *this;
}

void hkpVehicleInstance::getAxleParamsFromWheel(
    int32_t w_it,
    float linearForceAtWheel,
    float suspensionForceAtWheel,
    float estimatedCarSpeed,
    hkpVehicleFrictionSolverAxleParams& axle_params
) {
    __m128 v25 = _mm_undefined_ps();
    hkpVehicleData* m_data = this->m_data;
    int32_t m_axle = m_data->m_wheelParams.m_data[w_it].m_axle;
    hkpVehicleInstance::WheelInfo* m_data_1 = m_wheelsInfo.begin();
    __m128 m_quad = m_data_1[w_it].m_spinAxisWs.m_quad;
    __m128 m_quad_1 = m_data_1[w_it].m_contactPoint.m_separatingNormal.m_quad;
    __m128* p_m_quad = &m_data_1[w_it].m_contactPoint.m_position.m_quad;
    __m128 v13 = _mm_shuffle_ps(m_quad, m_quad, 201);
    __m128 v14 = _mm_shuffle_ps(m_quad_1, m_quad_1, 210);
    __m128 v15 = _mm_shuffle_ps(m_quad_1, m_quad_1, 201);
    __m128 v16 = _mm_shuffle_ps(m_quad, m_quad, 210);
    __m128 v17 = _mm_sub_ps(_mm_mul_ps(v15, v16), _mm_mul_ps(v14, v13));
    __m128 v18 = _mm_mul_ps(v17, v17);
    if ((_mm_shuffle_ps(v18, v18, 170)[0] + (_mm_shuffle_ps(v18, v18, 85)[0] + v18[0])) < 1.1920929e-07f) {
        v17 = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(p_m_quad[8], p_m_quad[8], 210), v13),
            _mm_mul_ps(_mm_shuffle_ps(p_m_quad[8], p_m_quad[8], 201), v16)
        );
    }
    int8_t* m_data_2 = m_data->m_numWheelsPerAxle.begin();
    int32_t m_axle_1 = m_data->m_wheelParams.m_data[w_it].m_axle;
    __m128 v21 = _mm_mul_ps(v17, v17);
    float v22 = _mm_shuffle_ps(v21, v21, 85)[0] + v21[0];
    __m128 v23 = _mm_shuffle_ps(v21, v21, 170);
    v23[0] = v23[0] + v22;
    float v24 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v23[0])));
    v25[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v26 = _mm_cmpeq_ss(v23, v25);
    v25[0] = 3.0f - ((v23[0] * v24) * v24);
    __m128 v27 = _mm_setzero_ps();
    v27[0] = (0.5f * v24) * v25[0];
    __m128 v28 = _mm_andnot_ps(v26, v27);
    __m128 v29 = _mm_mul_ps(_mm_shuffle_ps(v28, v28, 0), v17);
    float v33 = 1.0 / static_cast<double>(m_data_2[m_axle]);
    double v30 = p_m_quad[2][0] * v33;
    axle_params.m_contactPointWs.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v33), _mm_set_ss(v33), 0), *p_m_quad), axle_params.m_contactPointWs.m_quad
    );
    float v35 = v30;
    axle_params.m_constraintNormalWs.m_quad = _mm_add_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v29, v29, 201), v14), _mm_mul_ps(_mm_shuffle_ps(v29, v29, 210), v15)),
        axle_params.m_constraintNormalWs.m_quad
    );
    axle_params.m_forwardDirWs.m_quad = _mm_add_ps(axle_params.m_forwardDirWs.m_quad, v29);
    axle_params.m_frictionCoefficient =
        this->m_data->m_wheelParams.m_data[w_it].m_friction * v35 + axle_params.m_frictionCoefficient;
    bool v31 = axle_params.m_wheelFixed == 0;
    axle_params.m_viscosityFrictionCoefficient =
        this->m_data->m_wheelParams.m_data[w_it].m_viscosityFriction * v35 + axle_params.m_viscosityFrictionCoefficient;
    axle_params.m_maxFrictionCoefficient =
        v35 * this->m_data->m_wheelParams.m_data[w_it].m_maxFriction + axle_params.m_maxFrictionCoefficient;
    axle_params.m_wheelDownForce = suspensionForceAtWheel + axle_params.m_wheelDownForce;
    axle_params.m_forwardForce = axle_params.m_forwardForce + linearForceAtWheel;
    uint8_t m_wheelFixed = !v31 || m_isFixed.m_data[w_it].m_bool;
    axle_params.m_wheelFixed = m_wheelFixed;
    axle_params.m_slipVelocityFactor = v33 * this->m_data->m_wheelParams.m_data[w_it].m_slipAngle * estimatedCarSpeed +
        axle_params.m_slipVelocityFactor;
}

// The SDK's body: the chassis accumulator's angular velocity gains the steering angle times the extra torque factor
// times deltaTime about the chassis's first axis, through the friction inertia.
// KEEP: simulateVehicle (0xE385E0) calls applyFrictionSolver (0xE38756) with the ecx this body leaves (the exe's
// body never writes ecx); clang does not keep ecx across the call.
void hkpVehicleInstance::getExtraTorqueFactor(float deltaTime, hkpVelocityAccumulator& accumulatorForChassis) const {
    const hkpVehicleData* data = m_data;
    if (0.0f != data->m_extraTorqueFactor) {
        const float extraTorque = m_mainSteeringAngle * data->m_extraTorqueFactor * deltaTime;
        const hkQuadReal torque = _mm_set_ss(extraTorque);
        accumulatorForChassis.m_angularVel.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_mul_ps(_mm_shuffle_ps(torque, torque, 0), data->m_chassisOrientation.m_col0.m_quad),
                data->m_chassisFrictionInertiaInvDiag.m_quad
            ),
            accumulatorForChassis.m_angularVel.m_quad
        );
    }
}

void hkpVehicleInstance::prepareFrictionSolverParams(
    const hkStepInfo& stepInfo,
    hkpVehicleFrictionSolverParams& frictionParams
) {
    hkVector4 v5;
    hkVector4 v6;
    hkVector4 v8;
    hkpMotion* assertBuf;
    hkVector4 b_;
    hkVector4 v13;
    assertBuf = &this->m_entity->m_motion;
    hkRigidMotionUtilApplyForcesAndBuildAccumulators(stepInfo, &assertBuf, 1, 0, &frictionParams.m_chassis);
    hkpEntity* m_entity = this->m_entity;
    v5.m_quad = m_entity->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad;
    v6.m_quad = m_entity->m_motion.m_motionState.m_sweptTransform.m_centerOfMass0.m_quad;
    uint32_t v11 = std::bit_cast<uint32_t>(m_entity->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad[3]);
    __m128 v7 = _mm_set_ss(std::bit_cast<float>(v11));
    v11 = std::bit_cast<uint32_t>(m_entity->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad[3]);
    v13.m_quad = _mm_mul_ps(_mm_sub_ps(v5.m_quad, v6.m_quad), _mm_shuffle_ps(v7, v7, 0));
    v8.m_quad = m_entity->m_motion.m_motionState.m_deltaAngle.m_quad;
    hkpVehicleData* m_data = this->m_data;
    b_.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v11)), 0), v8.m_quad
    );
    frictionParams.m_chassis.m_invMasses = m_data->m_chassisFrictionInertiaInvDiag;
    frictionParams.m_chassisAtLastIntegration = frictionParams.m_chassis;
    frictionParams.m_chassisAtLastIntegration.m_angularVel.setRotatedDir(frictionParams.m_chassis.m_scratch123, b_);
    frictionParams.m_chassisAtLastIntegration.m_linearVel.m_quad = v13.m_quad;
}

// The SDK's body: per axle, the contact's normal and forward direction normalised and the ground's accumulators set
// up: fixed where the axle has no ground body, by handleFixedGroundAccum on a fixed one, axle 0's where both axles
// stand on one body, else built from the body's motion, its inverse masses capped at the chassis's times
// m_maxFrictionSolverMassRatio, with a copy holding the velocities of the body's last integration.
void hkpVehicleInstance::getAxleParams(
    const hkStepInfo& stepInfo,
    hkpRigidBody** groundBody,
    hkpVelocityAccumulator* groundAccum,
    hkpVelocityAccumulator* groundAccumAtIntegration,
    hkpVehicleFrictionSolverAxleParams* axleParams
) {
    for (int32_t ax_it = 0; ax_it < m_data->m_numWheelsPerAxle.m_size; ++ax_it) {
        hkpVehicleFrictionSolverAxleParams& axle = axleParams[ax_it];
        axle.m_constraintNormalWs.normalize3();
        axle.m_forwardDirWs.normalize3();
        hkpVelocityAccumulator* accum = &groundAccum[ax_it];
        axle.m_groundObject = accum;
        axle.m_groundObjectAtLastIntegration = accum;
        hkpRigidBody* body = groundBody[ax_it];
        if (!body) {
            // EXE-INLINED none: hkpVelocityAccumulator::setFixed
            accum->m_gravityFactor = 0.0f;
            accum->m_type = hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY;
            accum->m_invMasses.m_quad = _mm_setzero_ps();
            accum->m_linearVel.m_quad = _mm_setzero_ps();
            accum->m_angularVel.m_quad = _mm_setzero_ps();
            accum->m_scratch0.m_quad = _mm_setzero_ps();
            accum->m_scratch123.m_col0.m_quad = _mm_setzero_ps();
            accum->m_scratch123.m_col1.m_quad = _mm_setzero_ps();
            accum->m_scratch123.m_col2.m_quad = _mm_setzero_ps();
            continue;
        }
        if (body->m_motion.m_type == hkpMotion::MOTION_FIXED) {
            handleFixedGroundAccum(body, *accum);
            continue;
        }
        if (ax_it > 0 && body == groundBody[0]) {
            axle.m_groundObject = axleParams[0].m_groundObject;
            axle.m_groundObjectAtLastIntegration = axleParams[0].m_groundObjectAtLastIntegration;
            continue;
        }
        alignas(16) hkpMotion* motion = body->getMotion();
        hkRigidMotionUtilApplyForcesAndBuildAccumulators(stepInfo, &motion, 1, 0, accum);
        const float chassisMassInv = m_entity->m_motion.m_inertiaAndMassInv.m_quad[3];
        const float maxMassRatio = m_data->m_maxFrictionSolverMassRatio;
        const float groundMassInv = motion->m_inertiaAndMassInv.m_quad[3];
        if (groundMassInv > chassisMassInv * maxMassRatio) {
            const float massRatio = chassisMassInv / groundMassInv;
            const float scale = maxMassRatio * massRatio;
            accum->m_invMasses.m_quad =
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(scale), _mm_set_ss(scale), 0), accum->m_invMasses.m_quad);
        }
        // The swept transform's m_centerOfMass1.w is the inverse of the step's delta time.
        const hkSweptTransform& swept = motion->m_motionState.m_sweptTransform;
        const float invDeltaTime = swept.m_centerOfMass1.m_quad[3];
        const hkQuadReal invDt = _mm_shuffle_ps(_mm_set_ss(invDeltaTime), _mm_set_ss(invDeltaTime), 0);
        const hkQuadReal linearVel =
            _mm_mul_ps(_mm_sub_ps(swept.m_centerOfMass1.m_quad, swept.m_centerOfMass0.m_quad), invDt);
        hkVector4 angularVel;
        angularVel.m_quad = _mm_mul_ps(invDt, motion->m_motionState.m_deltaAngle.m_quad);
        hkpVelocityAccumulator* accumAtIntegration = &groundAccumAtIntegration[ax_it];
        *accumAtIntegration = *accum;
        axle.m_groundObjectAtLastIntegration = accumAtIntegration;
        accumAtIntegration->m_angularVel.setRotatedDir(accum->m_scratch123, angularVel);
        accumAtIntegration->m_linearVel.m_quad = linearVel;
    }
}

// The SDK's body: the step's times copied into the friction solver's step info.
void hkpVehicleInstance::applyFrictionSolver(
    const hkStepInfo& stepInfo,
    hkpVehicleFrictionSolverParams& frictionParams
) {
    hkpVehicleStepInfo frictionStepInfo;
    frictionStepInfo.m_deltaTime = stepInfo.m_deltaTime;
    frictionStepInfo.m_invDeltaTime = stepInfo.m_invDeltaTime;
    hkVehicleFrictionApplyVehicleFriction(
        frictionStepInfo, m_data->m_frictionDescription, frictionParams, m_frictionStatus
    );
}

// The SDK's body: the solver's angular velocity turned from the chassis's frame to world space, then both velocities
// set on the chassis's motion through its vtable.
void hkpVehicleInstance::applyResultsToChassis(const hkpVelocityAccumulator& accumulatorForChassis) {
    hkpEntity* chassis = m_entity;
    hkVector4 angularVel;
    angularVel.setRotatedDir(
        chassis->m_motion.m_motionState.m_transform.m_rotation, accumulatorForChassis.m_angularVel
    );
    hkpMotion* motion = chassis->getMotion();
    motion->setAngularVelocity(angularVel);
    motion->setLinearVelocity(accumulatorForChassis.m_linearVel);
}

// The SDK's body: per wheel, its axle's friction results copied into its info (the skid energy only on fixed
// ground), then, unless the wheel is fixed, its spin velocity from the chassis's forward velocity plus the forward
// slip, less the ground's forward velocity where the axle has a ground body, over the radius, and its spin angle
// advanced by it over deltaTime.
void hkpVehicleInstance::applyResultsToWheelInfo(
    float deltaTime,
    hkpRigidBody* const* groundBody,
    const hkpVehicleFrictionSolverParams& frictionParams
) {
    for (int32_t w_it = 0; w_it < m_data->m_numWheels; ++w_it) {
        const int32_t axle = m_data->m_wheelParams.m_data[w_it].m_axle;
        WheelInfo& wheelInfo = m_wheelsInfo.m_data[w_it];
        const hkpVehicleFrictionStatus::AxisStatus& axisStatus = m_frictionStatus.m_axis[axle];
        float skidEnergyDensity;
        if (wheelInfo.m_contactBody && wheelInfo.m_contactBody->m_motion.m_type == hkpMotion::MOTION_FIXED) {
            skidEnergyDensity = axisStatus.m_skid_energy_density;
        }
        else {
            skidEnergyDensity = 0.0f;
        }
        wheelInfo.m_skidEnergyDensity = skidEnergyDensity;
        wheelInfo.m_sideForce = axisStatus.m_side_force;
        wheelInfo.m_sideSlipVelocity = axisStatus.m_side_slip_velocity;
        wheelInfo.m_forwardSlipVelocity = axisStatus.m_forward_slip_velocity;
        hkVector4 forwardWs;
        forwardWs.setRotatedDir(
            m_entity->m_motion.m_motionState.m_transform.m_rotation, m_data->m_chassisOrientation.m_col1
        );
        const float chassisForwardVel = m_entity->m_motion.m_linearVelocity.dot3(forwardWs);
        if (m_isFixed.m_data[w_it].m_bool) {
            wheelInfo.m_spinVelocity = 0.0f;
            continue;
        }
        float virtualForwardVel = chassisForwardVel + wheelInfo.m_forwardSlipVelocity;
        if (groundBody[axle]) {
            const hkVector4 groundVel = frictionParams.m_axleParams[axle].m_groundObject->m_linearVel;
            const float groundForwardVel = groundVel.dot3(forwardWs);
            virtualForwardVel = virtualForwardVel - groundForwardVel;
        }
        const float spinVelocity = virtualForwardVel / m_data->m_wheelParams.m_data[w_it].m_radius;
        wheelInfo.m_spinVelocity = spinVelocity;
        const float spinDelta = spinVelocity * deltaTime;
        wheelInfo.m_spinAngle = spinDelta + wheelInfo.m_spinAngle;
    }
}

void hkpVehicleInstance::updateSuspension(
    float deltaTime,
    const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo,
    hkArray<float, hkContainerHeapAllocator>& suspensionForces
) {
    m_suspension->calcSuspension(deltaTime, this, cdInfo, (&suspensionForces)->begin());
}

const hkClass& hkpVehicleInstance::staticClass() {
    return hkpVehicleInstanceClass;
}

// The friction solver step: builds each axle's forward and side Jacobians and virtual masses and adds last step's
// velocities along them to the right-hand sides, applies the delayed forward impulses, sets up the forward impulses
// within the friction limits, solves the coupled side impulses through the 2x2 inverse and, where an axle's forces
// leave the friction circle, clips them per axle, then reports the status and applies the side impulses. The velocity
// lane sums are taken as y + x then z, and each scalar value is rounded to float where the game stores it.
void hkVehicleFrictionApplyVehicleFriction(
    const hkpVehicleStepInfo& stepInfo,
    const hkpVehicleFrictionDescription& descr,
    hkpVehicleFrictionSolverParams& params,
    hkpVehicleFrictionStatus& status
) {
    if (!HK_flyingcolors_vehicle) {
        hkpProcessFlyingColors(&HK_flyingcolors_vehicle);
        if (!HK_flyingcolors_vehicle) {
            return;
        }
    }

    hkpVehicleFrictionAxisTmp axisTmp[2];
    hkpVelocityAccumulator& chassis = params.m_chassis;
    const hkpVelocityAccumulator& chassisLast = params.m_chassisAtLastIntegration;
    const hkRotation& chassisRot = chassis.m_scratch123;

    // The Jacobians, the virtual masses and last step's velocities along them.
    for (int32_t i = 0; i < 2; ++i) {
        const hkpVehicleFrictionSolverAxleParams& axle = params.m_axleParams[i];
        hkpVehicleFrictionAxisTmp& tmp = axisTmp[i];
        const hkpVelocityAccumulator* pGround = axle.m_groundObject;
        const hkRotation& groundRot = pGround->m_scratch123;
        const hkQuadReal chassisInv = chassis.m_invMasses.m_quad;
        const hkQuadReal groundInv = pGround->m_invMasses.m_quad;
        const hkQuadReal invMassSum = _mm_add_ps(chassisInv, groundInv);
        hkVector4 chassisArm;
        chassisArm.m_quad = _mm_sub_ps(axle.m_contactPointWs.m_quad, chassis.m_scratch0.m_quad);
        hkVector4 groundArm;
        groundArm.m_quad = _mm_sub_ps(axle.m_contactPointWs.m_quad, pGround->m_scratch0.m_quad);

        // Forward.
        tmp.m_forwardJac.m_linear0.m_quad = axle.m_forwardDirWs.m_quad;
        hkVector4 cross;
        cross.setCross(chassisArm, axle.m_forwardDirWs);
        hkQuadReal c = cross.m_quad;
        const hkQuadReal forwardAng0 = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x55), chassisRot.getColumn(1).m_quad),
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x00), chassisRot.getColumn(0).m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(c, c, 0xAA), chassisRot.getColumn(2).m_quad)
        );
        cross.setCross(axle.m_forwardDirWs, groundArm);
        c = cross.m_quad;
        const hkQuadReal forwardAng1 = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x55), groundRot.getColumn(1).m_quad),
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x00), groundRot.getColumn(0).m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(c, c, 0xAA), groundRot.getColumn(2).m_quad)
        );
        tmp.m_forwardJac.m_angular[0].m_quad = forwardAng0;
        tmp.m_forwardJac.m_angular[1].m_quad = forwardAng1;
        hkQuadReal sum = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(forwardAng0, forwardAng0), chassisInv),
            _mm_mul_ps(_mm_mul_ps(forwardAng1, forwardAng1), groundInv)
        );
        sum = _mm_shuffle_ps(sum, _mm_unpackhi_ps(sum, invMassSum), 0xC4);
        sum = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x4E), sum);
        sum = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xB1), sum);
        const float fForwardInvMass = _mm_cvtss_f32(sum) + 1.1920928955078125e-07;
        tmp.m_forwardJac.m_angular[1](3) = fForwardInvMass;
        tmp.m_forwardJac.m_angular[0](3) = 1.0 / fForwardInvMass;
        const float fWheelInvMass = fForwardInvMass + descr.m_axleDescr[i].m_wheelSurfaceInertiaInv;
        tmp.m_invWheelForwardVirtMass = fWheelInvMass;
        tmp.m_wheelForwardVirtMass = 1.0 / fWheelInvMass;

        // Side.
        tmp.m_sideJac.m_linear0.m_quad = axle.m_constraintNormalWs.m_quad;
        cross.setCross(chassisArm, axle.m_constraintNormalWs);
        c = cross.m_quad;
        const hkQuadReal sideAng0 = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x55), chassisRot.getColumn(1).m_quad),
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x00), chassisRot.getColumn(0).m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(c, c, 0xAA), chassisRot.getColumn(2).m_quad)
        );
        cross.setCross(axle.m_constraintNormalWs, groundArm);
        c = cross.m_quad;
        const hkQuadReal sideAng1 = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x55), groundRot.getColumn(1).m_quad),
                _mm_mul_ps(_mm_shuffle_ps(c, c, 0x00), groundRot.getColumn(0).m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(c, c, 0xAA), groundRot.getColumn(2).m_quad)
        );
        tmp.m_sideJac.m_angular[1].m_quad = sideAng1;
        tmp.m_sideJac.m_angular[0].m_quad = sideAng0;
        sum = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(sideAng1, sideAng1), groundInv),
            _mm_mul_ps(_mm_mul_ps(sideAng0, sideAng0), chassisInv)
        );
        sum = _mm_shuffle_ps(sum, _mm_unpackhi_ps(sum, invMassSum), 0xC4);
        sum = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x4E), sum);
        sum = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xB1), sum);
        const float fSideInvMass = _mm_cvtss_f32(sum) + 1.1920928955078125e-07;
        tmp.m_sideJac.m_angular[1](3) = fSideInvMass;
        tmp.m_sideJac.m_angular[0](3) = 1.0 / fSideInvMass;
        tmp.m_sideVirtMass = tmp.m_sideJac.m_angular[0](3);
        tmp.m_invSideVirtMass = tmp.m_sideJac.m_angular[1](3);

        // Last step's velocities along both Jacobians, added to the right-hand sides.
        const hkpVelocityAccumulator* pGroundLast = axle.m_groundObjectAtLastIntegration;
        const hkQuadReal groundAngLast = pGroundLast->m_angularVel.m_quad;
        const hkQuadReal chassisAngLast = chassisLast.m_angularVel.m_quad;
        const hkQuadReal relLinLast = _mm_sub_ps(chassisLast.m_linearVel.m_quad, pGroundLast->m_linearVel.m_quad);
        const hkQuadReal forwardVel = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(groundAngLast, tmp.m_forwardJac.m_angular[1].m_quad),
                _mm_mul_ps(chassisAngLast, tmp.m_forwardJac.m_angular[0].m_quad)
            ),
            _mm_mul_ps(relLinLast, tmp.m_forwardJac.m_linear0.m_quad)
        );
        const hkQuadReal sideVel = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(groundAngLast, tmp.m_sideJac.m_angular[1].m_quad),
                _mm_mul_ps(chassisAngLast, tmp.m_sideJac.m_angular[0].m_quad)
            ),
            _mm_mul_ps(relLinLast, tmp.m_sideJac.m_linear0.m_quad)
        );
        const float fForwardVelLast = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(forwardVel, forwardVel, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(forwardVel, forwardVel, 0x55), forwardVel)
        ));
        const float fSideVelLast = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(sideVel, sideVel, 0xAA), _mm_add_ss(_mm_shuffle_ps(sideVel, sideVel, 0x55), sideVel)
        ));
        status.m_axis[i].m_sideRhs = status.m_axis[i].m_sideRhs + fSideVelLast;
        status.m_axis[i].m_forwardRhs = status.m_axis[i].m_forwardRhs + fForwardVelLast;
    }

    // The coupling of the two side constraints through the chassis, and through the ground when both wheels stand on
    // the same body: the off-diagonal of the side system's matrix.
    const hkpVelocityAccumulator* pGround0 = params.m_axleParams[0].m_groundObject;
    const hkp1Lin2AngJacobian& side0 = axisTmp[0].m_sideJac;
    const hkp1Lin2AngJacobian& side1 = axisTmp[1].m_sideJac;
    const hkQuadReal chassisInv = chassis.m_invMasses.m_quad;
    const hkQuadReal sideLin = _mm_mul_ps(side1.m_linear0.m_quad, side0.m_linear0.m_quad);
    hkQuadReal coupling;
    if (pGround0 == params.m_axleParams[1].m_groundObject) {
        const hkQuadReal groundInv = pGround0->m_invMasses.m_quad;
        const hkQuadReal linearPart = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(chassisInv, chassisInv, 0xFF), sideLin),
            _mm_mul_ps(_mm_shuffle_ps(groundInv, groundInv, 0xFF), sideLin)
        );
        const hkQuadReal angularPart = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(side1.m_angular[1].m_quad, side0.m_angular[1].m_quad), groundInv),
            _mm_mul_ps(_mm_mul_ps(side1.m_angular[0].m_quad, side0.m_angular[0].m_quad), chassisInv)
        );
        coupling = _mm_add_ps(linearPart, angularPart);
    }
    else {
        coupling = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(chassisInv, chassisInv, 0xFF), sideLin),
            _mm_mul_ps(_mm_mul_ps(side1.m_angular[0].m_quad, side0.m_angular[0].m_quad), chassisInv)
        );
    }
    const float fCoupling = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(coupling, coupling, 0xAA), _mm_add_ss(_mm_shuffle_ps(coupling, coupling, 0x55), coupling)
    ));
    float matrix[4];
    matrix[0] = axisTmp[0].m_invSideVirtMass;
    matrix[1] = fCoupling;
    matrix[2] = fCoupling;
    matrix[3] = axisTmp[1].m_invSideVirtMass;

    // Last step's forward impulses, applied to the chassis and the ground.
    for (int32_t i = 0; i < 2; ++i) {
        const float fImpulse = status.m_axis[i].m_delayed_forward_impulse;
        if (0.0f != fImpulse) {
            hkpVelocityAccumulator* pGround = params.m_axleParams[i].m_groundObject;
            const hkp1Lin2AngJacobian& jac = axisTmp[i].m_forwardJac;
            const hkQuadReal impulse = _mm_set1_ps(fImpulse);
            const hkQuadReal chassisScale = _mm_mul_ps(chassis.m_invMasses.m_quad, impulse);
            const hkQuadReal groundScale = _mm_mul_ps(pGround->m_invMasses.m_quad, impulse);
            chassis.m_linearVel.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(chassisScale, chassisScale, 0xFF), jac.m_linear0.m_quad),
                chassis.m_linearVel.m_quad
            );
            pGround->m_linearVel.m_quad = _mm_sub_ps(
                pGround->m_linearVel.m_quad,
                _mm_mul_ps(_mm_shuffle_ps(groundScale, groundScale, 0xFF), jac.m_linear0.m_quad)
            );
            chassis.m_angularVel.m_quad =
                _mm_add_ps(_mm_mul_ps(jac.m_angular[0].m_quad, chassisScale), chassis.m_angularVel.m_quad);
            pGround->m_angularVel.m_quad =
                _mm_add_ps(_mm_mul_ps(groundScale, jac.m_angular[1].m_quad), pGround->m_angularVel.m_quad);
        }
    }

    // The wheels' down forces, each its own share plus a share of both.
    const float fDownForce0 = params.m_axleParams[0].m_wheelDownForce;
    const float fDownForce1 = params.m_axleParams[1].m_wheelDownForce;
    const float fDownForceSum = fDownForce0 + fDownForce1;
    axisTmp[0].m_wheel_down_force = fDownForce0 * descr.m_axleDescr[0].m_wheelDownForceFactor +
        fDownForceSum * descr.m_axleDescr[0].m_wheelDownForceSumFactor;
    axisTmp[1].m_wheel_down_force = fDownForceSum * descr.m_axleDescr[1].m_wheelDownForceSumFactor +
        fDownForce1 * descr.m_axleDescr[1].m_wheelDownForceFactor;
    status.m_axis[0].m_skid_energy_density = 0.0f;
    status.m_axis[1].m_skid_energy_density = 0.0f;

    // The slip velocities, the forward impulses and the impulse limits.
    for (int32_t i = 0; i < 2; ++i) {
        const hkpVehicleFrictionSolverAxleParams& axle = params.m_axleParams[i];
        hkpVehicleFrictionAxisTmp& tmp = axisTmp[i];
        hkpVehicleFrictionStatus::AxisStatus& axisStatus = status.m_axis[i];
        const hkpVelocityAccumulator* pGround = axle.m_groundObject;
        const hkQuadReal groundAng = pGround->m_angularVel.m_quad;
        const hkQuadReal chassisAng = chassis.m_angularVel.m_quad;
        const hkQuadReal relLin = _mm_sub_ps(chassis.m_linearVel.m_quad, pGround->m_linearVel.m_quad);
        const hkQuadReal forwardVel = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(tmp.m_forwardJac.m_angular[0].m_quad, chassisAng),
                _mm_mul_ps(tmp.m_forwardJac.m_angular[1].m_quad, groundAng)
            ),
            _mm_mul_ps(tmp.m_forwardJac.m_linear0.m_quad, relLin)
        );
        const float fForwardVel = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(forwardVel, forwardVel, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(forwardVel, forwardVel, 0x55), forwardVel)
        ));
        tmp.m_forward_vel = fForwardVel;
        const hkQuadReal sideVel = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(tmp.m_sideJac.m_angular[1].m_quad, groundAng),
                _mm_mul_ps(chassisAng, tmp.m_sideJac.m_angular[0].m_quad)
            ),
            _mm_mul_ps(tmp.m_sideJac.m_linear0.m_quad, relLin)
        );
        const float fSideVel = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(sideVel, sideVel, 0xAA), _mm_add_ss(_mm_shuffle_ps(sideVel, sideVel, 0x55), sideVel)
        ));
        const float fSideSlipVel = axisStatus.m_sideRhs * 0.25 + fSideVel;
        tmp.m_sideSlipVel = fSideSlipVel;
        const float fAbsDownForce = std::fabs(tmp.m_wheel_down_force);
        const float fDownImpulse = fAbsDownForce * stepInfo.m_deltaTime;
        float fForwardForce = axle.m_forwardForce;
        float fForwardSlipVel;
        if (!axle.m_wheelFixed) {
            axisStatus.m_forwardRhs = 0.0f;
            fForwardSlipVel = axisStatus.m_forward_slip_velocity;
            tmp.m_forwardSlipVel = fForwardSlipVel;
        }
        else {
            // A fixed wheel: the forward impulse that stops it, clipped to the friction limit.
            const float fNegForwardVel = -fForwardVel;
            tmp.m_forwardSlipVel = fNegForwardVel;
            fForwardSlipVel = fNegForwardVel;
            const float fMassRatio = tmp.m_sideVirtMass * tmp.m_invWheelForwardVirtMass;
            const float fDelta = 0.5 * ((fForwardVel + axisStatus.m_forwardRhs * 0.5) * fMassRatio);
            const float fMaxDelta = axle.m_frictionCoefficient * fDownImpulse * tmp.m_invWheelForwardVirtMass;
            const float fAbsDelta = std::fabs(fDelta);
            if (!(fMaxDelta < fAbsDelta)) {
                tmp.m_forwardSlipVel = fNegForwardVel - fDelta;
            }
            else {
                const float fClipped = fDelta > 0.0 ? fMaxDelta : -fMaxDelta;
                tmp.m_forwardSlipVel = fNegForwardVel - fClipped;
            }
            fForwardForce = 0.0f;
        }
        float fFriction = axle.m_frictionCoefficient;
        if (0.0f != axle.m_viscosityFrictionCoefficient) {
            const float fSlipSq = fSideSlipVel * fSideSlipVel + fForwardSlipVel * fForwardSlipVel;
            const float fSlip = std::sqrt(fSlipSq);
            fFriction = fSlip * axle.m_viscosityFrictionCoefficient + fFriction;
            const float fMaxFriction = axle.m_maxFrictionCoefficient;
            if (!(fMaxFriction > fFriction)) {
                fFriction = fMaxFriction;
            }
            if (fFriction < 0.0) {
                fFriction = 0.0f;
            }
        }
        const float fMaxImpulse = fFriction * fDownImpulse;
        tmp.m_maxForwardImpulse = fMaxImpulse;
        tmp.m_maxSideImpulse = fMaxImpulse;
        const float fInvMaxImpulse = 1.0 / (fMaxImpulse + 1.1920928955078125e-07);
        tmp.m_invMaxForwardImpulse = fInvMaxImpulse;
        tmp.m_invMaxSideImpulse = fInvMaxImpulse;
        tmp.m_forwardImpulse = tmp.m_wheelForwardVirtMass * tmp.m_forwardSlipVel + stepInfo.m_deltaTime * fForwardForce;
    }

    // The slip velocity factors scale the side system so that the axles share the chassis's down force.
    if (params.m_axleParams[1].m_slipVelocityFactor + params.m_axleParams[0].m_slipVelocityFactor > 0.0) {
        const float fAbsDownForce0 = std::fabs(params.m_axleParams[0].m_wheelDownForce);
        const float fAbsDownForce1 = std::fabs(params.m_axleParams[1].m_wheelDownForce);
        const float fAbsDownForceSum = fAbsDownForce0 + fAbsDownForce1;
        const float fChassisDown = fAbsDownForceSum * descr.m_chassisMassInv;
        const float fChassisDownImpulse = stepInfo.m_deltaTime * fChassisDown;
        const float fSlipFactor0 = params.m_axleParams[0].m_slipVelocityFactor;
        const float fFactor0 = fSlipFactor0 > 0.0 ? fSlipFactor0 : 0.0f;
        float fScale0 = 1.0f;
        if (!(fFactor0 <= fChassisDownImpulse)) {
            fScale0 = fChassisDownImpulse / fFactor0;
        }
        if (fScale0 < 0.0001f) {
            fScale0 = 0.0001f;
        }
        const float fSlipFactor1 = params.m_axleParams[1].m_slipVelocityFactor;
        const float fFactor1 = fSlipFactor1 > 0.0 ? fSlipFactor1 : 0.0f;
        float fScale1 = 1.0f;
        if (!(fFactor1 <= fChassisDownImpulse)) {
            fScale1 = fChassisDownImpulse / fFactor1;
        }
        if (fScale1 < 0.0001f) {
            fScale1 = 0.0001f;
        }
        matrix[0] = fScale0 * matrix[0];
        matrix[2] = fScale1 * fCoupling;
        const float fScaleProduct = fScale1 * fScale0;
        const float fScaleRoot = std::sqrt(fScaleProduct);
        matrix[1] = fScaleRoot * fCoupling;
        matrix[3] = fScaleRoot * matrix[3];
        axisTmp[0].m_sideVirtMass = fScale0 * axisTmp[0].m_sideVirtMass;
        axisTmp[0].m_invSideVirtMass = axisTmp[0].m_invSideVirtMass / fScale0;
        axisTmp[1].m_sideVirtMass = fScale1 * axisTmp[1].m_sideVirtMass;
        axisTmp[1].m_invSideVirtMass = axisTmp[1].m_invSideVirtMass / fScale1;
    }

    // The side impulses that cancel both side slip velocities together.
    float inverse[4];
    inverse[0] = 1.0f;
    inverse[3] = 1.0f;
    inverse[1] = 0.0f;
    inverse[2] = 0.0f;
    invert2x2(matrix, inverse, 0.0f);
    const float fSideSlip0 = axisTmp[0].m_sideSlipVel;
    const float fSideSlip1 = axisTmp[1].m_sideSlipVel;
    axisTmp[0].m_sideImpulse = -(fSideSlip1 * inverse[1] + fSideSlip0 * inverse[0]);
    axisTmp[1].m_sideImpulse = -(fSideSlip0 * inverse[2] + fSideSlip1 * inverse[3]);

    // EXE-INLINED 0xE593D0
    const float fRelForward0 = axisTmp[0].m_forwardImpulse * axisTmp[0].m_invMaxForwardImpulse;
    const float fRelSide0 = axisTmp[0].m_invMaxSideImpulse * axisTmp[0].m_sideImpulse;
    const float fRelForceSq0 = fRelSide0 * fRelSide0 + fRelForward0 * fRelForward0;
    // EXE-INLINED 0xE593D0
    const float fRelForward1 = axisTmp[1].m_forwardImpulse * axisTmp[1].m_invMaxForwardImpulse;
    const float fRelSide1 = axisTmp[1].m_invMaxSideImpulse * axisTmp[1].m_sideImpulse;
    const float fRelForceSq1 = fRelSide1 * fRelSide1 + fRelForward1 * fRelForward1;

    // Too fast for positional friction: no right-hand sides carried over.
    const float fAbsForwardVel = std::fabs(axisTmp[0].m_forward_vel);
    if (params.m_maxVelocityForPositionalFriction <= fAbsForwardVel) {
        status.m_axis[0].m_sideRhs = 0.0f;
        status.m_axis[0].m_forwardRhs = 0.0f;
        status.m_axis[1].m_sideRhs = 0.0f;
        status.m_axis[1].m_forwardRhs = 0.0f;
    }

    if (fRelForceSq0 > 1.0f || fRelForceSq1 > 1.0f) {
        // An axle outside its friction circle: scale its right-hand sides back and clip each axle in turn, the one
        // further outside last, adding each clipped impulse's coupling to that axle's side slip velocity.
        if (fRelForceSq0 > 1.0f) {
            const float fInv = 1.0 / fRelForceSq0;
            status.m_axis[0].m_sideRhs = fInv * status.m_axis[0].m_sideRhs;
            status.m_axis[0].m_forwardRhs = fInv * status.m_axis[0].m_forwardRhs;
        }
        if (1.0f < fRelForceSq1) {
            const float fInv = 1.0 / fRelForceSq1;
            status.m_axis[1].m_sideRhs = fInv * status.m_axis[1].m_sideRhs;
            status.m_axis[1].m_forwardRhs = fInv * status.m_axis[1].m_forwardRhs;
        }
        int32_t iLast;
        int32_t iFirst;
        if (fRelForceSq1 < fRelForceSq0) {
            iLast = 1;
            iFirst = 0;
        }
        else {
            iLast = 0;
            iFirst = 1;
        }
        float* pSlipVel = &axisTmp[iLast].m_sideSlipVel;
        int32_t iAxle = iFirst;
        for (int32_t n = 0; n < 2; ++n) {
            hkpVehicleFrictionAxisTmp& tmp = axisTmp[iAxle];
            tmp.m_sideImpulse = -tmp.m_sideSlipVel * tmp.m_sideVirtMass;
            hkVehicleFrictionSolverClipImpulse(&descr.m_axleDescr[iAxle], &status.m_axis[iAxle], &tmp);
            *pSlipVel = tmp.m_sideImpulse * matrix[1] + *pSlipVel;
            iAxle = iLast;
        }
        status.m_axis[0].m_forward_slip_velocity =
            axisTmp[0].m_invWheelForwardVirtMass * axisTmp[0].m_restForwardSlipImpulse;
        status.m_axis[1].m_forward_slip_velocity =
            axisTmp[1].m_invWheelForwardVirtMass * axisTmp[1].m_restForwardSlipImpulse;
        status.m_axis[0].m_side_slip_velocity = axisTmp[0].m_sideSlipVel;
        status.m_axis[1].m_side_slip_velocity = axisTmp[1].m_sideSlipVel;
    }
    else {
        status.m_axis[0].m_relativeSideForce = fRelSide0;
        status.m_axis[0].m_relativeForwardForce = fRelForward0;
        status.m_axis[1].m_relativeSideForce = fRelSide1;
        status.m_axis[1].m_relativeForwardForce = fRelForward1;
        status.m_axis[0].m_forward_slip_velocity = 0.0f;
        status.m_axis[1].m_forward_slip_velocity = 0.0f;
        status.m_axis[0].m_side_slip_velocity = 0.0f;
        status.m_axis[1].m_side_slip_velocity = 0.0f;
    }

    status.m_axis[0].m_delayed_forward_impulse = axisTmp[0].m_forwardImpulse;
    status.m_axis[1].m_delayed_forward_impulse = axisTmp[1].m_forwardImpulse;
    status.m_axis[0].m_side_force = stepInfo.m_invDeltaTime * axisTmp[0].m_sideImpulse;
    status.m_axis[1].m_side_force = stepInfo.m_invDeltaTime * axisTmp[1].m_sideImpulse;

    // The side impulses, applied to the chassis and the ground.
    for (int32_t i = 0; i < 2; ++i) {
        hkpVelocityAccumulator* pGround = params.m_axleParams[i].m_groundObject;
        const hkp1Lin2AngJacobian& jac = axisTmp[i].m_sideJac;
        const hkQuadReal impulse = _mm_set1_ps(axisTmp[i].m_sideImpulse);
        const hkQuadReal chassisScale = _mm_mul_ps(impulse, chassis.m_invMasses.m_quad);
        const hkQuadReal groundScale = _mm_mul_ps(pGround->m_invMasses.m_quad, impulse);
        chassis.m_linearVel.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(chassisScale, chassisScale, 0xFF), jac.m_linear0.m_quad),
            chassis.m_linearVel.m_quad
        );
        pGround->m_linearVel.m_quad = _mm_sub_ps(
            pGround->m_linearVel.m_quad,
            _mm_mul_ps(_mm_shuffle_ps(groundScale, groundScale, 0xFF), jac.m_linear0.m_quad)
        );
        chassis.m_angularVel.m_quad =
            _mm_add_ps(_mm_mul_ps(jac.m_angular[0].m_quad, chassisScale), chassis.m_angularVel.m_quad);
        pGround->m_angularVel.m_quad =
            _mm_add_ps(_mm_mul_ps(jac.m_angular[1].m_quad, groundScale), pGround->m_angularVel.m_quad);
    }
}
