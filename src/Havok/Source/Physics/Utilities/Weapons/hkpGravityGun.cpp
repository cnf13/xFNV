#include "Havok/Source/Physics/Utilities/Weapons/hkpGravityGun.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The reflection data of hkpGravityGun's class record, as the Havok SDK's class file has it: per member its attribute
// objects, the attribute list naming them and the custom attributes record over the list, then the member table.
using hkpGravityGunAttributeArray2 = const hkCustomAttributes::Attribute[2];

namespace {
    // Holds, per member, its offset into this struct (-1: no default), then the defaults. The vectors sit at 4-byte
    // offsets in this struct (0x34, 0x44), so they are plain floats.
    struct hkpGravityGun_DefaultStruct {
        int32_t s_defaultOffsets[8];
        int32_t m_maxNumObjectsPicked;
        float m_maxMassOfObjectPicked;
        float m_maxDistOfObjectPicked;
        float m_impulseAppliedWhenObjectNotPicked;
        float m_throwVelocity;
        float m_capturedObjectPosition[4];
        float m_capturedObjectsOffset[4];
    };
} // namespace

using hkpGravityGunAttributeArray3 = const hkCustomAttributes::Attribute[3];
static hkUiAttribute hkpGravityGun_maxNumObjectsPicked_Ui{
    true,
    {hkUiAttribute::NONE},
    "Max. Objects Picked",
    "",
    "",
    false,
    false,
    false
};
static hkRangeInt32Attribute hkpGravityGun_maxNumObjectsPicked_RangeInt32{0, 1000, 0, 1000};
static hkpGravityGunAttributeArray2 hkpGravityGun_maxNumObjectsPicked_AttributeList{
    {"hk.Ui", {&hkpGravityGun_maxNumObjectsPicked_Ui, &hkUiAttributeClass}},
    {"hk.RangeInt32", {&hkpGravityGun_maxNumObjectsPicked_RangeInt32, &hkRangeInt32AttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_maxNumObjectsPicked_Attributes{
    hkpGravityGun_maxNumObjectsPicked_AttributeList,
    2
};
static hkRangeRealAttribute hkpGravityGun_maxMassOfObjectPicked_RangeReal{0.0f, 1000000.0f, 0.0f, 1000000.0f};
static hkUiAttribute hkpGravityGun_maxMassOfObjectPicked_Ui{
    true,
    {hkUiAttribute::NONE},
    "Max. Object Mass",
    "",
    "",
    false,
    false,
    false
};
static hkpGravityGunAttributeArray2 hkpGravityGun_maxMassOfObjectPicked_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_maxMassOfObjectPicked_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_maxMassOfObjectPicked_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_maxMassOfObjectPicked_Attributes{
    hkpGravityGun_maxMassOfObjectPicked_AttributeList,
    2
};
static hkRangeRealAttribute hkpGravityGun_maxDistOfObjectPicked_RangeReal{0.0f, 10000.0f, 0.0f, 10000.0f};
static hkUiAttribute
    hkpGravityGun_maxDistOfObjectPicked_Ui{true, {hkUiAttribute::NONE}, "Max. Distance", "", "", false, false, false};
static hkSemanticsAttribute hkpGravityGun_maxDistOfObjectPicked_Semantics{{hkSemanticsAttribute::DISTANCE}};
static hkpGravityGunAttributeArray3 hkpGravityGun_maxDistOfObjectPicked_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_maxDistOfObjectPicked_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_maxDistOfObjectPicked_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpGravityGun_maxDistOfObjectPicked_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_maxDistOfObjectPicked_Attributes{
    hkpGravityGun_maxDistOfObjectPicked_AttributeList,
    3
};
static hkRangeRealAttribute hkpGravityGun_impulseAppliedWhenObjectNotPicked_RangeReal{0.0f, 1000.0f, 0.0f, 1000.0f};
static hkUiAttribute hkpGravityGun_impulseAppliedWhenObjectNotPicked_Ui{
    true,
    {hkUiAttribute::NONE},
    "Impulse on Unpicked",
    "",
    "",
    false,
    false,
    false
};
static hkpGravityGunAttributeArray2 hkpGravityGun_impulseAppliedWhenObjectNotPicked_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_impulseAppliedWhenObjectNotPicked_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_impulseAppliedWhenObjectNotPicked_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_impulseAppliedWhenObjectNotPicked_Attributes{
    hkpGravityGun_impulseAppliedWhenObjectNotPicked_AttributeList,
    2
};
static hkRangeRealAttribute hkpGravityGun_throwVelocity_RangeReal{0.0f, 1000.0f, 0.0f, 100.0f};
static hkUiAttribute
    hkpGravityGun_throwVelocity_Ui{true, {hkUiAttribute::NONE}, "Throw Velocity", "", "", false, false, false};
static hkpGravityGunAttributeArray2 hkpGravityGun_throwVelocity_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_throwVelocity_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_throwVelocity_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_throwVelocity_Attributes{hkpGravityGun_throwVelocity_AttributeList, 2};
static hkRangeRealAttribute hkpGravityGun_capturedObjectPosition_RangeReal{-10000.0f, 100000.0f, -10000.0f, 100000.0f};
static hkUiAttribute hkpGravityGun_capturedObjectPosition_Ui{
    true,
    {hkUiAttribute::NONE},
    "Captured Object's Position",
    "",
    "",
    false,
    false,
    true
};
static hkSemanticsAttribute hkpGravityGun_capturedObjectPosition_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpGravityGunAttributeArray3 hkpGravityGun_capturedObjectPosition_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_capturedObjectPosition_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_capturedObjectPosition_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpGravityGun_capturedObjectPosition_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_capturedObjectPosition_Attributes{
    hkpGravityGun_capturedObjectPosition_AttributeList,
    3
};
static hkRangeRealAttribute hkpGravityGun_capturedObjectsOffset_RangeReal{-10000.0f, 100000.0f, -10000.0f, 100000.0f};
static hkUiAttribute hkpGravityGun_capturedObjectsOffset_Ui{
    true,
    {hkUiAttribute::NONE},
    "Captured Object's Offset",
    "",
    "",
    false,
    false,
    true
};
static hkSemanticsAttribute hkpGravityGun_capturedObjectsOffset_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpGravityGunAttributeArray3 hkpGravityGun_capturedObjectsOffset_AttributeList{
    {"hk.RangeReal", {&hkpGravityGun_capturedObjectsOffset_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpGravityGun_capturedObjectsOffset_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpGravityGun_capturedObjectsOffset_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpGravityGun_capturedObjectsOffset_Attributes{
    hkpGravityGun_capturedObjectsOffset_AttributeList,
    3
};
using hkpGravityGunClass_MembersArray = const hkClassMember[8];
static hkpGravityGunClass_MembersArray hkpGravityGunClass_Members{
    {"grabbedBodies",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x20,
     nullptr},
    {"maxNumObjectsPicked",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x2C,
     &hkpGravityGun_maxNumObjectsPicked_Attributes},
    {"maxMassOfObjectPicked",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     &hkpGravityGun_maxMassOfObjectPicked_Attributes},
    {"maxDistOfObjectPicked",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x34,
     &hkpGravityGun_maxDistOfObjectPicked_Attributes},
    {"impulseAppliedWhenObjectNotPicked",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x38,
     &hkpGravityGun_impulseAppliedWhenObjectNotPicked_Attributes},
    {"throwVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x3C,
     &hkpGravityGun_throwVelocity_Attributes},
    {"capturedObjectPosition",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     &hkpGravityGun_capturedObjectPosition_Attributes},
    {"capturedObjectsOffset",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x50,
     &hkpGravityGun_capturedObjectsOffset_Attributes}
};

// The defaults the class record names.
static const hkpGravityGun_DefaultStruct hkpGravityGun_Default{
    {-1, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x44},
    10,
    200.0f,
    50.0f,
    100.0f,
    50.0f,
    {2.5f, 0.6f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f}
};
hkClass hkpGravityGunClass(
    "hkpGravityGun",
    &hkpFirstPersonGunClass,
    96,
    nullptr,
    0,
    nullptr,
    0,
    hkpGravityGunClass_Members,
    8,
    &hkpGravityGun_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpGravityGunTypeInfo = {
    "hkpGravityGun",
    finishLoadedObjecthkpGravityGun,
    cleanupLoadedObjecthkpGravityGun,
    getVtablehkpGravityGun(),
    &typeid(hkpGravityGun),
    sizeof(hkpGravityGun)
};

hkpGravityGun::hkpGravityGun()
  : m_maxNumObjectsPicked(10), m_maxMassOfObjectPicked(200.0f), m_maxDistOfObjectPicked(50.0f),
    m_impulseAppliedWhenObjectNotPicked(100.0f), m_throwVelocity(50.0f) {
    m_type.m_storage = WEAPON_TYPE_GRAVITYGUN;
    m_name = "GravityGun";
    XFNV_M128_F32(m_capturedObjectPosition.m_quad, 0) = 2.5f;
    XFNV_M128_F32(m_capturedObjectPosition.m_quad, 1) = 0.6f;
    XFNV_M128_F32(m_capturedObjectPosition.m_quad, 2) = 0.0f;
    XFNV_M128_F32(m_capturedObjectPosition.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_capturedObjectsOffset.m_quad, 0) = 0.0f;
    XFNV_M128_F32(m_capturedObjectsOffset.m_quad, 1) = 1.0f;
    XFNV_M128_F32(m_capturedObjectsOffset.m_quad, 2) = 0.0f;
    XFNV_M128_F32(m_capturedObjectsOffset.m_quad, 3) = 0.0f;
}

// The grabbed bodies array frees itself.
hkpGravityGun::~hkpGravityGun() {}

void hkpGravityGun::stepGun(
    float timeStep,
    hkpWorld* world,
    const hkpRigidBody* body,
    const hkTransform& viewTransform,
    bool fire,
    bool fireRmb
) {
    bool v9;
    hkVector4 v15;
    hkVector4 v16;
    if (fire) {
        gravityGunPush(viewTransform, body, world);
        int32_t v8 = 0;
        v9 = m_listeners.m_size <= 0;
        v15.m_quad = body->m_motion.m_motionState.m_transform.m_translation.m_quad;
        if (!v9) {
            do {
                hkpFirstPersonGunBulletListener* v10 = m_listeners.m_data[v8];
                v10->gunFiredCallback(&v15, nullptr);
                ++v8;
            }
            while (v8 < m_listeners.m_size);
        }
    }
    else {
        if (fireRmb) {
            if (m_grabbedBodies.m_size == m_maxNumObjectsPicked) {
                gravityGunPutDown(viewTransform, body, world);
                int32_t v13 = 0;
                v9 = m_listeners.m_size <= 0;
                v16.m_quad = body->m_motion.m_motionState.m_transform.m_translation.m_quad;
                if (!v9) {
                    do {
                        hkpFirstPersonGunBulletListener* v14 = m_listeners.m_data[v13];
                        v14->gunFiredCallback(&v16, nullptr);
                        ++v13;
                    }
                    while (v13 < m_listeners.m_size);
                }
            }
            else {
                gravityGunPull(viewTransform, body, world);
                int32_t v11 = 0;
                v9 = m_listeners.m_size <= 0;
                v16.m_quad = viewTransform.m_translation.m_quad;
                v15.m_quad = body->m_motion.m_motionState.m_transform.m_translation.m_quad;
                if (!v9) {
                    do {
                        hkpFirstPersonGunBulletListener* v12 = m_listeners.m_data[v11];
                        v12->gunFiredCallback(&v16, &v15);
                        ++v11;
                    }
                    while (v11 < m_listeners.m_size);
                }
            }
        }
    }
    gravityGunStep(timeStep, viewTransform, body);
}

void hkpGravityGun::reset(hkpWorld* world) {
    int32_t v3 = 0;
    if (m_grabbedBodies.m_size <= 0) {
        m_grabbedBodies.m_size = 0;
    }
    else {
        do {
            m_grabbedBodies.m_data[v3++]->removeReference();
        }
        while (v3 < m_grabbedBodies.m_size);
        m_grabbedBodies.m_size = 0;
    }
}

hkpRigidBody*
hkpGravityGun::pickBody(const hkTransform& viewTransform, const hkpRigidBody* characterBody, const hkpWorld* world) {
    float v26 = 0.35f;
    const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>*
        p_m_activeSimulationIslands = &world->m_activeSimulationIslands;
    hkpRigidBody* v24 = nullptr;
    const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>*
        p_m_inactiveSimulationIslands = &world->m_activeSimulationIslands;
    int32_t v25 = 2;
    while (true) {
        int32_t v5 = 0;
        for (int32_t i = 0; v5 < p_m_activeSimulationIslands->m_size; i = v5) {
            hkpSimulationIsland* v6 = p_m_activeSimulationIslands->m_data[v5];
            hkpSimulationIsland* v27 = v6;
            int32_t v21 = 0;
            if (v6->m_entities.m_size > 0) {
                while (true) {
                    hkVector4 v29;
                    hkpRigidBody* v7 = static_cast<hkpRigidBody*>(v6->m_entities.m_data[v21]);
                    v29.setTransformedInversePos(
                        viewTransform,
                        v7->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1
                    );
                    double v8 = XFNV_M128_F32(v29.m_quad, 0);
                    if (!(XFNV_M128_F32(v29.m_quad, 0) < 0.10000000149011612) &&
                        !(m_maxDistOfObjectPicked < v8) &&
                        v7 != characterBody) {
                        float v16 = std::fabs(static_cast<double>(XFNV_M128_F32(v29.m_quad, 2)));
                        double v9 = v16;
                        float v17 = std::fabs(static_cast<double>(XFNV_M128_F32(v29.m_quad, 1)));
                        float v18 = v9 + v17;
                        float v19 = v18 / (v8 + 1.0);
                        float Mass = v7->m_motion.getMass();
                        if (m_maxMassOfObjectPicked < static_cast<double>(Mass)) {
                            v19 = v19 + 0.10000000149011612;
                        }
                        if (!(v26 < static_cast<double>(v19))) {
                            hkVector4 v13;
                            hkpWorldRayCastInput input;
                            int32_t m_size = m_grabbedBodies.m_size;
                            int32_t v11 = 0;
                            // Declared before the branch, so that the else branch's goto to not_grabbed does not cross
                            // its initialisation.
                            hkpClosestRayHitCollector collector;
                            if (m_size <= 0) {
                            not_grabbed:;
                                input.m_from = viewTransform.m_translation;
                                v13.m_quad = v7->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad;
                                input.m_enableShapeCollectionFilter.m_bool = 0;
                                input.m_filterInfo = 0;
                                input.m_to.m_quad = v13.m_quad;
                                world->castRay(input, collector);
                                if (!collector.m_rayHit.m_rootCollidable ||
                                    collector.m_rayHit.m_rootCollidable == &v7->m_collidable) {
                                    v24 = v7;
                                    v26 = v19;
                                }
                            }
                            else {
                                hkpRigidBody** m_data = m_grabbedBodies.begin();
                                while (*m_data != v7) {
                                    ++v11;
                                    ++m_data;
                                    if (v11 >= m_size) {
                                        goto not_grabbed;
                                    }
                                }
                            }
                        }
                    }
                    if (++v21 >= v27->m_entities.m_size) {
                        break;
                    }
                    v6 = v27;
                }
                v5 = i;
                p_m_activeSimulationIslands = p_m_inactiveSimulationIslands;
            }
            ++v5;
        }
        bool v14 = v25-- == 1;
        p_m_inactiveSimulationIslands = &world->m_inactiveSimulationIslands;
        if (v14) {
            return v24;
        }
        p_m_activeSimulationIslands = &world->m_inactiveSimulationIslands;
    }
}

// Pulls each grabbed body towards its slot in front of the view: the velocity blends the body's own with the
// character's and adds the gap to the slot scaled by 0.4/timeStep; the angular velocity is damped by 0.8. Each new
// velocity is set (the body activated first) only when a lane differs from the current one by more than 0.001.
void hkpGravityGun::gravityGunStep(
    float timeStep,
    const hkTransform& viewTransform,
    const hkpRigidBody* characterBody
) {
    if (m_grabbedBodies.m_size > 0) {
        // The double value of 0.4f is divided by timeStep once, before the loop, and kept as a float.
        const float fGain = 0.4000000059604645 / timeStep;
        int32_t i = 0;
        do {
            hkVector4 vSlot;
            hkVector4 vTarget;
            hkVector4 vLinear;
            hkVector4 vAngular;
            const float fIndex = static_cast<float>(i);
            vSlot.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fIndex), _mm_set_ss(fIndex), 0), m_capturedObjectsOffset.m_quad),
                m_capturedObjectPosition.m_quad
            );
            vTarget.setTransformedPos(viewTransform, vSlot);
            hkpRigidBody* pBody = m_grabbedBodies.m_data[i];
            const __m128 kBlend = _mm_shuffle_ps(_mm_set_ss(0.8f), _mm_set_ss(0.8f), 0);
            vLinear.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, kBlend), pBody->m_motion.m_linearVelocity.m_quad),
                _mm_mul_ps(characterBody->m_motion.m_linearVelocity.m_quad, kBlend)
            );
            vLinear.m_quad = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(vTarget.m_quad, pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad),
                    _mm_shuffle_ps(_mm_set_ss(fGain), _mm_set_ss(fGain), 0)
                ),
                vLinear.m_quad
            );
            pBody->setLinearVelocity(vLinear);
            const __m128 kDamped = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(0.8f), _mm_set_ss(0.8f), 0),
                pBody->m_motion.m_angularVelocity.m_quad
            );
            const __m128 kSquares = _mm_mul_ps(kDamped, kDamped);
            vAngular.m_quad = kDamped;
            const float fLengthSquared = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kSquares, kSquares, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
            ));
            if (fLengthSquared > 1.0f) {
                pBody->setAngularVelocity(vAngular);
            }
            ++i;
        }
        while (i < m_grabbedBodies.m_size);
    }
}

// Removes the grabbed body at index, gives it back a hundredth of its inverse mass and drops the gun's reference. The
// constant is 0.01f widened to double.
hkpRigidBody* hkpGravityGun::dropObject(int32_t index) {
    hkpRigidBody* pBody = m_grabbedBodies.m_data[index];
    m_grabbedBodies.removeAtAndCopy(index);
    const __m128 kInertiaAndMassInv = pBody->m_motion.m_inertiaAndMassInv.m_quad;
    float afMassInv = XFNV_M128_F32(_mm_shuffle_ps(kInertiaAndMassInv, kInertiaAndMassInv, 255), 0) *
        0.009999999776482582;
    pBody->setMassInv(afMassInv);
    pBody->removeReference();
    return pBody;
}

void hkpGravityGun::gravityGunPush(
    const hkTransform& viewTransform,
    const hkpRigidBody* characterBody,
    const hkpWorld* world
) {
    hkVector4 arVelocity;
    if (m_grabbedBodies.m_size) {
        hkVector4 v6;
        hkpRigidBody* v5 = dropObject(0);
        v6.m_quad = characterBody->m_motion.m_linearVelocity.m_quad;
        arVelocity.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(m_throwVelocity), _mm_set_ss(m_throwVelocity), 0),
            viewTransform.m_rotation.m_col0.m_quad
        );
        XFNV_M128_F32(arVelocity.m_quad, 1) = XFNV_M128_F32(arVelocity.m_quad, 1) + 0.5;
        arVelocity.m_quad = _mm_add_ps(v6.m_quad, arVelocity.m_quad);
        v5->setLinearVelocity(arVelocity);
        v5->m_collidable.setQualityType(HK_COLLIDABLE_QUALITY_BULLET);
    }
    else {
        hkpRigidBody* v7 = pickBody(viewTransform, characterBody, world);
        if (v7) {
            arVelocity.m_quad = _mm_mul_ps(
                _mm_shuffle_ps(
                    _mm_set_ss(m_impulseAppliedWhenObjectNotPicked),
                    _mm_set_ss(m_impulseAppliedWhenObjectNotPicked),
                    0
                ),
                viewTransform.m_rotation.m_col0.m_quad
            );
            v7->applyLinearImpulse(arVelocity);
        }
    }
}

void hkpGravityGun::gravityGunPutDown(
    const hkTransform& viewTransform,
    const hkpRigidBody* characterBody,
    const hkpWorld* world
) {
    if (m_grabbedBodies.m_size) {
        dropObject(0);
    }
}

void hkpGravityGun::takeObject(hkpRigidBody* body) {
    body->addReference();
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>* p_m_grabbedBodies = &m_grabbedBodies;
    p_m_grabbedBodies->pushBack(body);
    float v4 = XFNV_M128_F32(
                   _mm_shuffle_ps(
                       body->m_motion.m_inertiaAndMassInv.m_quad,
                       body->m_motion.m_inertiaAndMassInv.m_quad,
                       255
                   ),
                   0
               ) *
        100.0;
    body->setMassInv(v4);
}

void hkpGravityGun::gravityGunPull(
    const hkTransform& viewTransform,
    const hkpRigidBody* characterBody,
    const hkpWorld* world
) {
    hkpRigidBody* body = pickBody(viewTransform, characterBody, world);
    if (body) {
        if (m_maxMassOfObjectPicked *
                    XFNV_M128_F32(
                        _mm_shuffle_ps(
                            body->m_motion.m_inertiaAndMassInv.m_quad,
                            body->m_motion.m_inertiaAndMassInv.m_quad,
                            255
                        ),
                        0
                    ) <
                1.0 ||
            m_grabbedBodies.m_size >= m_maxNumObjectsPicked) {
            float impulse = -m_impulseAppliedWhenObjectNotPicked;
            hkVector4 v8;
            v8.m_quad = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(impulse), _mm_set_ss(impulse), 0),
                viewTransform.m_rotation.m_col0.m_quad
            );
            // The SDK's inline: activate, then the motion's applyLinearImpulse through its vtable.
            body->applyLinearImpulse(v8);
        }
        else {
            takeObject(body);
        }
    }
}

const hkClass& hkpGravityGun::staticClass() {
    return hkpGravityGunClass;
}
