#include "Havok/Source/Physics/Vehicle/Steering/Default/hkpVehicleDefaultSteering.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/Steering/hkpVehicleSteering.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <xmmintrin.h>

static void finishLoadedObjecthkpVehicleDefaultSteering(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultSteering();
static void cleanupLoadedObjecthkpVehicleDefaultSteering(void* p);

// The reflected members of hkpVehicleDefaultSteering.
using hkpVehicleDefaultSteeringClass_MembersArray = const hkClassMember[3];
static hkpVehicleDefaultSteeringClass_MembersArray hkpVehicleDefaultSteeringClass_Members{
    {"maxSteeringAngle",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr},
    {"maxSpeedFullSteeringAngle",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"doesWheelSteer", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_BOOL}, 0, {0}, 0x10, nullptr}
};
hkClass hkpVehicleDefaultSteeringClass(
    "hkpVehicleDefaultSteering",
    &hkpVehicleSteeringClass,
    28,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleDefaultSteeringClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleDefaultSteeringTypeInfo = {
    "hkpVehicleDefaultSteering",
    finishLoadedObjecthkpVehicleDefaultSteering,
    cleanupLoadedObjecthkpVehicleDefaultSteering,
    getVtablehkpVehicleDefaultSteering(),
    &typeid(hkpVehicleDefaultSteering),
    sizeof(hkpVehicleDefaultSteering)
};

// The type info's finish-loaded-object callback for hkpVehicleDefaultSteering: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultSteering(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleDefaultSteering(kFlag);
    }
}

static const void* getVtablehkpVehicleDefaultSteering() {
    return hkGetVtableOf<hkpVehicleDefaultSteering>();
}

static void cleanupLoadedObjecthkpVehicleDefaultSteering(void* p) {
    static_cast<hkpVehicleDefaultSteering*>(p)->~hkpVehicleDefaultSteering();
}

hkpVehicleDefaultSteering::hkpVehicleDefaultSteering() : m_maxSteeringAngle(0.0f), m_maxSpeedFullSteeringAngle(0.0f) {}

// The wheel steering flags array frees itself.
hkpVehicleDefaultSteering::~hkpVehicleDefaultSteering() {}

void hkpVehicleDefaultSteering::calcSteering(
    float deltaTime,
    const hkpVehicleInstance* vehicle,
    const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInfoOutput,
    hkpVehicleSteering::SteeringAnglesOutput& steeringOutput
) {
    calcMainSteeringAngle(deltaTime, vehicle, filteredInfoOutput, steeringOutput);
    for (int32_t i = 0; i < m_doesWheelSteer.m_size; ++i) {
        if (m_doesWheelSteer.m_data[i].m_bool) {
            steeringOutput.m_wheelsSteeringAngle.m_data[i] = steeringOutput.m_mainSteeringAngle;
        }
        else {
            steeringOutput.m_wheelsSteeringAngle.m_data[i] = 0.0;
        }
    }
}

void hkpVehicleDefaultSteering::calcMainSteeringAngle(
    float deltaTime,
    const hkpVehicleInstance* vehicle,
    const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInfoOutput,
    hkpVehicleSteering::SteeringAnglesOutput& steeringOutput
) {
    hkVector4 v11;
    hkVector4* p_m_col1 = &vehicle->m_data->m_chassisOrientation.m_col1;
    hkMotionState* p_m_motionState = vehicle->m_entity->m_motion.getMotionState();
    steeringOutput.m_mainSteeringAngle = filteredInfoOutput.m_steeringWheelInput * m_maxSteeringAngle;
    v11.setRotatedDir(p_m_motionState->m_transform.m_rotation, *p_m_col1);
    __m128 v7 = _mm_mul_ps(vehicle->m_entity->m_motion.m_linearVelocity.m_quad, v11.m_quad);
    float v8 = _mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
    if (m_maxSpeedFullSteeringAngle < static_cast<double>(v8)) {
        float v10 = m_maxSpeedFullSteeringAngle / v8;
        steeringOutput.m_mainSteeringAngle = steeringOutput.m_mainSteeringAngle * v10 * v10;
    }
}

const hkClass& hkpVehicleDefaultSteering::staticClass() {
    return hkpVehicleDefaultSteeringClass;
}
