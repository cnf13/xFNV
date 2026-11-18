#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaKeyFrameHierarchyUtility.h"

#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <cmath>
#include <xmmintrin.h>

static void finishLoadedObjecthkaKeyFrameHierarchyUtilityControlData(void* p, int32_t finishing);
static void cleanupLoadedObjecthkaKeyFrameHierarchyUtilityControlData(void* p);
static void cleanupLoadedObjecthkaKeyFrameHierarchyUtility(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaKeyFrameHierarchyUtilityControlDataTypeInfo = {
    "hkaKeyFrameHierarchyUtilityControlData",
    finishLoadedObjecthkaKeyFrameHierarchyUtilityControlData,
    cleanupLoadedObjecthkaKeyFrameHierarchyUtilityControlData,
    nullptr,
    &typeid(hkaKeyFrameHierarchyUtility::ControlData),
    sizeof(hkaKeyFrameHierarchyUtility::ControlData)
};
hkTypeInfo hkaKeyFrameHierarchyUtilityTypeInfo = {
    "hkaKeyFrameHierarchyUtility",
    nullptr,
    cleanupLoadedObjecthkaKeyFrameHierarchyUtility,
    nullptr,
    &typeid(hkaKeyFrameHierarchyUtility),
    sizeof(hkaKeyFrameHierarchyUtility)
};

// The reflection data of hkaKeyFrameHierarchyUtility::ControlData's class record, as the Havok SDK's class file has it:
// per member a range attribute, a description attribute, the two-attribute list naming both and the custom attributes
// record over the list.
using ControlDataAttributeArray = const hkCustomAttributes::Attribute[2];
static hkRangeRealAttribute ControlData_hierarchyGain_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_hierarchyGain_Description{
    "This parameter blends the desired target for a bone between model space: zero, or local space: one. Usually the "
    "controller will be much stiffer and more stable when driving to model space. However local space targeting can "
    "look more natural."
};
static ControlDataAttributeArray ControlData_hierarchyGain_AttributeList{
    {"hk.RangeReal", {&ControlData_hierarchyGain_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_hierarchyGain_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_hierarchyGain_Attributes{ControlData_hierarchyGain_AttributeList, 2};
static hkRangeRealAttribute ControlData_velocityDamping_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_velocityDamping_Description{
    "This gain dampens the velocities of the bodies. The current velocity of the body is scaled by this parameter on "
    "every frame before the controller is applied. It is applied every step and is generally more aggressive than "
    "standard linear or angular damping. A value of 0 means no damping."
};
static ControlDataAttributeArray ControlData_velocityDamping_AttributeList{
    {"hk.RangeReal", {&ControlData_velocityDamping_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_velocityDamping_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_velocityDamping_Attributes{ControlData_velocityDamping_AttributeList, 2};
static hkRangeRealAttribute ControlData_accelerationGain_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_accelerationGain_Description{
    "This gain controls the proportion of the difference in acceleration that is applied to the bodies. It dampens "
    "the effects of the velocity control."
};
static ControlDataAttributeArray ControlData_accelerationGain_AttributeList{
    {"hk.RangeReal", {&ControlData_accelerationGain_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_accelerationGain_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_accelerationGain_Attributes{ControlData_accelerationGain_AttributeList, 2};
static hkRangeRealAttribute ControlData_velocityGain_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_velocityGain_Description{
    "This gain controls the proportion of the difference in velocity that is applied to the bodies. It dampens the "
    "effects of the position control."
};
static ControlDataAttributeArray ControlData_velocityGain_AttributeList{
    {"hk.RangeReal", {&ControlData_velocityGain_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_velocityGain_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_velocityGain_Attributes{ControlData_velocityGain_AttributeList, 2};
static hkRangeRealAttribute ControlData_positionGain_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_positionGain_Description{
    "This gain controls the proportion of the difference in position that is applied to the bodies. It has the most "
    "immediate effect. High gain values make the controller very stiff. Once the controller is too stiff it will "
    "tend to overshoot. The velocity gain can help control this."
};
static ControlDataAttributeArray ControlData_positionGain_AttributeList{
    {"hk.RangeReal", {&ControlData_positionGain_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_positionGain_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_positionGain_Attributes{ControlData_positionGain_AttributeList, 2};
static hkRangeRealAttribute ControlData_positionMaxLinearVelocity_Range{0.0f, 100.0f, 0.0f, 10.0f};
static const hkDescriptionAttribute ControlData_positionMaxLinearVelocity_Description{
    "The position difference is scaled by the inverse delta time to compute a velocity to be applied to the rigid "
    "body. The velocity is first clamped to this limit before it is applied."
};
static ControlDataAttributeArray ControlData_positionMaxLinearVelocity_AttributeList{
    {"hk.RangeReal", {&ControlData_positionMaxLinearVelocity_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_positionMaxLinearVelocity_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_positionMaxLinearVelocity_Attributes{
    ControlData_positionMaxLinearVelocity_AttributeList, 2
};
static hkRangeRealAttribute ControlData_positionMaxAngularVelocity_Range{0.0f, 100.0f, 0.0f, 10.0f};
static const hkDescriptionAttribute ControlData_positionMaxAngularVelocity_Description{
    "The orientation difference is scaled by the inverse delta time to compute an angular velocity to be applied to "
    "the rigid body. The velocity is first clamped to this limit before it is applied."
};
static ControlDataAttributeArray ControlData_positionMaxAngularVelocity_AttributeList{
    {"hk.RangeReal", {&ControlData_positionMaxAngularVelocity_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_positionMaxAngularVelocity_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_positionMaxAngularVelocity_Attributes{
    ControlData_positionMaxAngularVelocity_AttributeList, 2
};
static hkRangeRealAttribute ControlData_snapGain_Range{0.0f, 1.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_snapGain_Description{
    "This gain allows for precise matching between keyframes and the current position. It works like the "
    "m_positionGain: it calculates an optimal deltaVelocity, scales it by m_snapGain, clips it against "
    "m_snapMaxXXXVelocity and scales it down if the implied positional difference is larger than "
    "m_snapMaxXXXXDistance."
};
static ControlDataAttributeArray ControlData_snapGain_AttributeList{
    {"hk.RangeReal", {&ControlData_snapGain_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_snapGain_Description), &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_snapGain_Attributes{ControlData_snapGain_AttributeList, 2};
static hkRangeRealAttribute ControlData_snapMaxLinearVelocity_Range{0.0f, 100.0f, 0.0f, 10.0f};
static const hkDescriptionAttribute ControlData_snapMaxLinearVelocity_Description{
    "See m_snapGain. The linear velocity calculated from the snapGain is clamped to this limit before being applied."
};
static ControlDataAttributeArray ControlData_snapMaxLinearVelocity_AttributeList{
    {"hk.RangeReal", {&ControlData_snapMaxLinearVelocity_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_snapMaxLinearVelocity_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_snapMaxLinearVelocity_Attributes{
    ControlData_snapMaxLinearVelocity_AttributeList, 2
};
static hkRangeRealAttribute ControlData_snapMaxAngularVelocity_Range{0.0f, 100.0f, 0.0f, 10.0f};
static const hkDescriptionAttribute ControlData_snapMaxAngularVelocity_Description{
    "See m_snapGain. The angular velocity calculated from the snapGain is clamped to this limit before being "
    "applied."
};
static ControlDataAttributeArray ControlData_snapMaxAngularVelocity_AttributeList{
    {"hk.RangeReal", {&ControlData_snapMaxAngularVelocity_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_snapMaxAngularVelocity_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_snapMaxAngularVelocity_Attributes{
    ControlData_snapMaxAngularVelocity_AttributeList, 2
};
static hkRangeRealAttribute ControlData_snapMaxLinearDistance_Range{0.0f, 10.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_snapMaxLinearDistance_Description{
    "This sets the max linear distance for the snap gain to work at full strength. The strength of the controller "
    "peaks at this distance."
};
static ControlDataAttributeArray ControlData_snapMaxLinearDistance_AttributeList{
    {"hk.RangeReal", {&ControlData_snapMaxLinearDistance_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_snapMaxLinearDistance_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_snapMaxLinearDistance_Attributes{
    ControlData_snapMaxLinearDistance_AttributeList, 2
};
static hkRangeRealAttribute ControlData_snapMaxAngularDistance_Range{0.0f, 10.0f, 0.0f, 1.0f};
static const hkDescriptionAttribute ControlData_snapMaxAngularDistance_Description{
    "This sets the max angular distance for the snap gain to work at full strength. The strength of the controller "
    "peaks at this distance."
};
static ControlDataAttributeArray ControlData_snapMaxAngularDistance_AttributeList{
    {"hk.RangeReal", {&ControlData_snapMaxAngularDistance_Range, &hkRangeRealAttributeClass}},
    {"hk.Description",
     {const_cast<hkDescriptionAttribute*>(&ControlData_snapMaxAngularDistance_Description),
      &hkDescriptionAttributeClass}}
};
static const hkCustomAttributes ControlData_snapMaxAngularDistance_Attributes{
    ControlData_snapMaxAngularDistance_AttributeList, 2
};

// The class record's member table.
using hkaKeyFrameHierarchyUtilityControlDataClassMemberArray = const hkClassMember[12];
static hkaKeyFrameHierarchyUtilityControlDataClassMemberArray hkaKeyFrameHierarchyUtilityControlDataClass_Members{
    {"hierarchyGain",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x0,
     &ControlData_hierarchyGain_Attributes},
    {"velocityDamping",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x4,
     &ControlData_velocityDamping_Attributes},
    {"accelerationGain",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x8,
     &ControlData_accelerationGain_Attributes},
    {"velocityGain",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0xC,
     &ControlData_velocityGain_Attributes},
    {"positionGain",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x10,
     &ControlData_positionGain_Attributes},
    {"positionMaxLinearVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x14,
     &ControlData_positionMaxLinearVelocity_Attributes},
    {"positionMaxAngularVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x18,
     &ControlData_positionMaxAngularVelocity_Attributes},
    {"snapGain",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x1C,
     &ControlData_snapGain_Attributes},
    {"snapMaxLinearVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x20,
     &ControlData_snapMaxLinearVelocity_Attributes},
    {"snapMaxAngularVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x24,
     &ControlData_snapMaxAngularVelocity_Attributes},
    {"snapMaxLinearDistance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x28,
     &ControlData_snapMaxLinearDistance_Attributes},
    {"snapMaxAngularDistance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x2C,
     &ControlData_snapMaxAngularDistance_Attributes}
};

static void finishLoadedObjecthkaKeyFrameHierarchyUtilityControlData(void* p, int32_t finishing) {
    ;
}

static void cleanupLoadedObjecthkaKeyFrameHierarchyUtilityControlData(void* p) {
    ;
}

static void cleanupLoadedObjecthkaKeyFrameHierarchyUtility(void* p) {
    ;
}

namespace {
    // Each member's offset in this struct (-1: velocityDamping has no default), then the defaults of the others.
    struct hkaKeyFrameHierarchyUtilityControlData_DefaultStruct {
        int32_t s_defaultOffsets[12];
        float m_hierarchyGain;
        float m_accelerationGain;
        float m_velocityGain;
        float m_positionGain;
        float m_positionMaxLinearVelocity;
        float m_positionMaxAngularVelocity;
        float m_snapGain;
        float m_snapMaxLinearVelocity;
        float m_snapMaxAngularVelocity;
        float m_snapMaxLinearDistance;
        float m_snapMaxAngularDistance;
    };
} // namespace
// The defaults the class record names.
static const hkaKeyFrameHierarchyUtilityControlData_DefaultStruct hkaKeyFrameHierarchyUtilityControlData_Default{
    {0x30, -1, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58},
    0.17f,
    1.0f,
    0.6f,
    0.05f,
    1.4f,
    1.8f,
    0.1f,
    0.3f,
    0.3f,
    0.03f,
    0.1f
};
hkClass hkaKeyFrameHierarchyUtilityControlDataClass(
    "hkaKeyFrameHierarchyUtilityControlData",
    nullptr,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkaKeyFrameHierarchyUtilityControlDataClass_Members,
    12,
    &hkaKeyFrameHierarchyUtilityControlData_Default,
    nullptr,
    0,
    0
);

hkClass hkaKeyFrameHierarchyUtilityClass(
    "hkaKeyFrameHierarchyUtility",
    nullptr,
    1,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);

hkaKeyFrameHierarchyUtility::ControlData::ControlData()
  : m_hierarchyGain(0.17f), m_velocityDamping(0.0f), m_accelerationGain(1.0f), m_velocityGain(0.6f),
    m_positionGain(0.05f), m_positionMaxLinearVelocity(1.4f), m_positionMaxAngularVelocity(1.8f), m_snapGain(0.1f),
    m_snapMaxLinearVelocity(0.3f), m_snapMaxAngularVelocity(0.3f), m_snapMaxLinearDistance(0.03f),
    m_snapMaxAngularDistance(0.1f) {}

hkBool hkaKeyFrameHierarchyUtility::ControlData::isValid() {
    hkBool result;
    char v9;
    double v2 = 0.0;
    bool v3 = m_hierarchyGain > 0.0f;
    bool v4 = 0.0f == m_hierarchyGain;
    result.m_bool = 1;
    char v5 = (v3 || v4) && m_hierarchyGain <= 1.0f;
    result.m_bool = v5;
    char v6 = v5 && (m_velocityDamping >= 0.0f && m_velocityDamping <= 1.0f);
    result.m_bool = v6;
    char v7 = v6 && (m_accelerationGain >= 0.0f && m_accelerationGain <= 1.0f);
    result.m_bool = v7;
    char v8 = v7 && (m_velocityGain >= 0.0f && m_velocityGain <= 1.0f);
    result.m_bool = v8;
    if (!v8) {
        goto LABEL_26;
    }
    if (!(m_positionGain >= 0.0f)) {
        v2 = 0.0;
        goto LABEL_26;
    }
    v2 = 0.0;
    if (!(m_positionGain <= 1.0f)) {
    LABEL_26:;
        v9 = 0;
        goto LABEL_27;
    }
    v9 = 1;
LABEL_27:;
    result.m_bool = v9;
    char v10 = v9 && v2 <= m_positionMaxLinearVelocity;
    result.m_bool = v10;
    char v11 = v10 && v2 <= m_positionMaxAngularVelocity;
    result.m_bool = v11;
    char v12 = v11 && v2 <= m_snapGain;
    result.m_bool = v12;
    char v13 = v12 && v2 <= m_snapMaxLinearVelocity;
    result.m_bool = v13;
    char v14 = v13 && v2 <= m_snapMaxAngularVelocity;
    result.m_bool = v14;
    char v15 = v14 && v2 <= m_snapMaxLinearDistance;
    result.m_bool = v15;
    result.m_bool = v15 && v2 <= m_snapMaxAngularDistance;
    return result;
}

const hkClass& hkaKeyFrameHierarchyUtility::ControlData::staticClass() {
    return hkaKeyFrameHierarchyUtilityControlDataClass;
}

void hkaKeyFrameHierarchyUtility::initialize(
    const hkaKeyFrameHierarchyUtility::BodyData& bodies,
    hkaKeyFrameHierarchyUtility::WorkElem* internalReferencePose
) {
    int32_t v2 = 0;
    if (bodies.m_numRigidBodies > 0) {
        hkVector4* p_m_prevAngularVelocity = &internalReferencePose->m_prevAngularVelocity;
        do {
            hkpRigidBody* v4 = bodies.m_rigidBodies[v2];
            p_m_prevAngularVelocity[-1] = v4->m_motion.m_linearVelocity;
            *p_m_prevAngularVelocity = v4->m_motion.m_angularVelocity;
            p_m_prevAngularVelocity[-3] = v4->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1;
            p_m_prevAngularVelocity[-2] = v4->m_motion.m_motionState.m_sweptTransform.m_rotation1.m_vec;
            ++v2;
            p_m_prevAngularVelocity += 4;
        }
        while (v2 < bodies.m_numRigidBodies);
    }
}

// Drives each rigid body towards its key-framed pose through its velocities: the velocity the pose moves at, damped and
// blended with the body's current one, corrected towards the pose's position (clamped by clampVectorLength), then
// snapped to the bone's model transform when the snap gain is nonzero (clamped by clampSnapVelocity, which returns the
// stress); the same for the angular velocity. A body's velocity is set only when it moves by more than 0.001 on some
// axis.
void hkaKeyFrameHierarchyUtility::applyKeyFrame(
    float m_deltaTime,
    const hkaKeyFrameHierarchyUtility::KeyFrameData& pose,
    const hkaKeyFrameHierarchyUtility::BodyData& bodydata,
    const hkaKeyFrameHierarchyUtility::ControlData* controlPalette,
    hkaKeyFrameHierarchyUtility::Output* output
) {
    const int32_t numBodies = bodydata.m_numRigidBodies;
    // EXE-INLINED none: hkAllocateStack<hkQsTransform>
    hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
    void* pBlock = stack.m_next.m_storage;
    stack.m_next.m_storage =
        static_cast<char*>(pBlock) + ((numBodies * static_cast<int32_t>(2 * sizeof(hkQsTransform)) + 127) & ~127);
    hkQsTransform* pDesired = static_cast<hkQsTransform*>(pBlock);
    calcComPoseUsingHierarchy(pose.m_desiredPoseLocalSpace, bodydata, controlPalette, pose.m_worldFromRoot, pDesired);

    const float fInvDeltaTime = 1.0 / m_deltaTime;
    for (int32_t i = 0; i < bodydata.m_numRigidBodies; ++i) {
        int32_t iControl = 0;
        if (bodydata.m_controlDataIndices) {
            iControl = bodydata.m_controlDataIndices[i];
            if (iControl == -1) {
                continue;
            }
        }
        hkpRigidBody* pBody = bodydata.m_rigidBodies[i];
        const ControlData& rControl = controlPalette[iControl];
        float fAccelerationGain = rControl.m_accelerationGain;
        float fVelocityGain = rControl.m_velocityGain;
        float fPositionGain = rControl.m_positionGain;
        float fSnapGain = rControl.m_snapGain;
        if (bodydata.m_boneWeights) {
            const float fWeight = bodydata.m_boneWeights[i];
            fAccelerationGain = fWeight * fAccelerationGain;
            fVelocityGain = fWeight * fVelocityGain;
            fPositionGain = fWeight * fPositionGain;
            fSnapGain = fWeight * fSnapGain;
        }
        const float fDamping = 1.0f - rControl.m_velocityDamping;
        WorkElem& rWork = pose.m_internalReferencePose[i];
        const hkQsTransform& rBodyDesired = pDesired[2 * i];
        const hkQsTransform& rBoneModel = pDesired[2 * i + 1];
        const __m128 kInvDeltaTime = _mm_set1_ps(fInvDeltaTime);
        const __m128 kDamping = _mm_set1_ps(fDamping);

        // Linear velocity.
        const __m128 kDesiredPosition = rBodyDesired.m_translation.m_quad;
        const __m128 kPrevPosition = rWork.m_prevPosition.m_quad;
        rWork.m_prevPosition.m_quad = kDesiredPosition;
        const __m128 kPositionVelocity = _mm_mul_ps(
            _mm_sub_ps(kDesiredPosition, pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad),
            kInvDeltaTime
        );
        const __m128 kDesiredLinearVelocity = _mm_mul_ps(_mm_sub_ps(kDesiredPosition, kPrevPosition), kInvDeltaTime);
        const __m128 kPrevLinearVelocity = rWork.m_prevLinearVelocity.m_quad;
        rWork.m_prevLinearVelocity.m_quad = kDesiredLinearVelocity;
        const hkVector4 kCurrentLinearVelocity = pBody->m_motion.m_linearVelocity;
        hkVector4 kLinearVelocity;
        kLinearVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(fAccelerationGain), _mm_sub_ps(kDesiredLinearVelocity, kPrevLinearVelocity)),
            _mm_mul_ps(kCurrentLinearVelocity.m_quad, kDamping)
        );
        kLinearVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(fVelocityGain), _mm_sub_ps(kDesiredLinearVelocity, kLinearVelocity.m_quad)),
            kLinearVelocity.m_quad
        );
        hkVector4 kPositionCorrection;
        kPositionCorrection.m_quad =
            _mm_mul_ps(_mm_set1_ps(fPositionGain), _mm_sub_ps(kPositionVelocity, kLinearVelocity.m_quad));
        clampVectorLength(rControl.m_positionMaxLinearVelocity, &kPositionCorrection);
        kLinearVelocity.m_quad = _mm_add_ps(kPositionCorrection.m_quad, kLinearVelocity.m_quad);
        if (fSnapGain != 0.0f) {
            const float fMaxDistance = fSnapGain * (rControl.m_snapMaxLinearDistance * fInvDeltaTime);
            hkVector4 kSnapCenterOfMass;
            kSnapCenterOfMass.setRotatedDir(
                rBoneModel.m_rotation, pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal
            );
            hkVector4 kSnap;
            kSnap.m_quad = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(
                            _mm_add_ps(kSnapCenterOfMass.m_quad, rBoneModel.m_translation.m_quad),
                            pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad
                        ),
                        kInvDeltaTime
                    ),
                    kLinearVelocity.m_quad
                ),
                _mm_set1_ps(fSnapGain)
            );
            const float fStress = clampSnapVelocity(rControl.m_snapMaxLinearVelocity, fMaxDistance, &kSnap);
            if (output) {
                output[i].m_stressSquared = fStress;
            }
            kLinearVelocity.m_quad = _mm_add_ps(kSnap.m_quad, kLinearVelocity.m_quad);
        }
        // EXE-INLINED none: hkVector4::equals3 (with the 0.001 tolerance)
        const __m128 kTolerance = _mm_set1_ps(0.001f);
        if ((_mm_movemask_ps(_mm_cmple_ps(
                 _mm_and_ps(
                     _mm_sub_ps(pBody->m_motion.m_linearVelocity.m_quad, kLinearVelocity.m_quad),
                     hkVector4_setAbs4_absMask
                 ),
                 kTolerance
             )) &
             15) != 15) {
            pBody->setLinearVelocity(kLinearVelocity);
        }

        // Angular velocity.
        const hkQuaternion& rDesiredRotation = rBodyDesired.m_rotation;
        hkVector4 kAngleToDesired;
        pBody->m_motion.m_motionState.m_sweptTransform.m_rotation1.estimateAngleTo(rDesiredRotation, kAngleToDesired);
        hkVector4 kAngleMoved;
        rWork.m_prevRotation.estimateAngleTo(rDesiredRotation, kAngleMoved);
        rWork.m_prevRotation = rDesiredRotation;
        const __m128 kPrevAngularVelocity = rWork.m_prevAngularVelocity.m_quad;
        const __m128 kDesiredAngularVelocity = _mm_mul_ps(kAngleMoved.m_quad, kInvDeltaTime);
        rWork.m_prevAngularVelocity.m_quad = kDesiredAngularVelocity;
        const hkVector4 kCurrentAngularVelocity = pBody->m_motion.m_angularVelocity;
        hkVector4 kAngularVelocity;
        kAngularVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(fAccelerationGain), _mm_sub_ps(kDesiredAngularVelocity, kPrevAngularVelocity)),
            _mm_mul_ps(kCurrentAngularVelocity.m_quad, kDamping)
        );
        kAngularVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(fVelocityGain), _mm_sub_ps(kDesiredAngularVelocity, kAngularVelocity.m_quad)),
            kAngularVelocity.m_quad
        );
        hkVector4 kAngleCorrection;
        kAngleCorrection.m_quad = _mm_mul_ps(
            _mm_set1_ps(fPositionGain),
            _mm_sub_ps(_mm_mul_ps(kAngleToDesired.m_quad, kInvDeltaTime), kAngularVelocity.m_quad)
        );
        clampVectorLength(rControl.m_positionMaxAngularVelocity, &kAngleCorrection);
        kAngularVelocity.m_quad = _mm_add_ps(kAngleCorrection.m_quad, kAngularVelocity.m_quad);
        if (0.0f != fSnapGain) {
            hkVector4 kSnapAngle;
            pBody->m_motion.m_motionState.m_sweptTransform.m_rotation1.estimateAngleTo(
                rBoneModel.m_rotation, kSnapAngle
            );
            const float fMaxAngle = rControl.m_snapMaxAngularDistance * fInvDeltaTime * fSnapGain;
            kSnapAngle.m_quad = _mm_mul_ps(
                _mm_sub_ps(_mm_mul_ps(kSnapAngle.m_quad, kInvDeltaTime), kAngularVelocity.m_quad),
                _mm_set1_ps(fSnapGain)
            );
            const float fStress = clampSnapVelocity(rControl.m_snapMaxAngularVelocity, fMaxAngle, &kSnapAngle);
            if (output) {
                output[i].m_stressSquared = output[i].m_stressSquared + fStress;
            }
            kAngularVelocity.m_quad = _mm_add_ps(kSnapAngle.m_quad, kAngularVelocity.m_quad);
        }
        // EXE-INLINED none: hkVector4::equals3 (with the 0.001 tolerance)
        if ((_mm_movemask_ps(_mm_cmple_ps(
                 _mm_and_ps(
                     _mm_sub_ps(pBody->m_motion.m_angularVelocity.m_quad, kAngularVelocity.m_quad),
                     hkVector4_setAbs4_absMask
                 ),
                 kTolerance
             )) &
             15) != 15) {
            pBody->setAngularVelocity(kAngularVelocity);
        }
    }

    // EXE-INLINED none: hkDeallocateStack<hkQsTransform>
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(pBlock);
}

const hkClass& hkaKeyFrameHierarchyUtility::staticClass() {
    return hkaKeyFrameHierarchyUtilityClass;
}

// Scales the vector r_esi down to length a1 when it is longer.
void clampVectorLength(float a1, hkVector4* r_esi) {
    __m128 v = r_esi->m_quad;
    __m128 sq = _mm_mul_ps(v, v);
    float lenSq = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
    float maxSq = a1 * a1;
    if (maxSq < lenSq) {
        float len = std::sqrt(lenSq);
        float invLen = 1.0 / len;
        float scale = invLen * a1;
        r_esi->m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(scale), _mm_set_ss(scale), 0), v);
    }
}

// Returns the squared length of r_esi over a1 squared; when the vector is longer than a1, scales it to a1 * a2 / its
// squared length, or, when a2 squared is above its squared length, to length a1.
double clampSnapVelocity(float a1, float a2, hkVector4* r_esi) {
    __m128 v = r_esi->m_quad;
    float maxSq = a1 * a1;
    __m128 sq = _mm_mul_ps(v, v);
    float lenSq = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
    float ratio = lenSq / maxSq;
    if (!(maxSq < lenSq)) {
        return ratio;
    }
    float minSq = a2 * a2;
    if (!(minSq > lenSq)) {
        float scale = a1 * a2 / lenSq;
        r_esi->m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(scale), _mm_set_ss(scale), 0), v);
    }
    else {
        float q = maxSq / lenSq;
        float scale = std::sqrt(q);
        r_esi->m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(scale), _mm_set_ss(scale), 0), v);
    }
    return ratio;
}

// For each rigid body: its bone's model transform (the parent bone's, or worldFromRoot for a root, times its desired
// local transform) into apOut[2 * i + 1]; that transform's position and rotation, blended by the control data's
// hierarchy gain (times the bone weight) towards the parent body's current transform times the local one; and from them
// the body's desired centre of mass and rotation into apOut[2 * i], at scale zero. A body without control data (index
// -1) gets only its bone's model transform.
void calcComPoseUsingHierarchy(
    const hkQsTransform* desiredPoseLocalSpace,
    const hkaKeyFrameHierarchyUtility::BodyData& bodyData,
    const hkaKeyFrameHierarchyUtility::ControlData* controlPalette,
    const hkQsTransform& worldFromRoot,
    hkQsTransform* apOut
) {
    for (int32_t i = 0; i < bodyData.m_numRigidBodies; ++i) {
        const hkQsTransform& rLocal = desiredPoseLocalSpace[i];
        const int32_t iParent = bodyData.m_parentIndices[i];
        const hkQsTransform& rParentModel = iParent < 0 ? worldFromRoot : apOut[2 * iParent + 1];
        hkQsTransform& rModel = apOut[2 * i + 1];
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(rParentModel.m_rotation, rLocal.m_translation);
        rModel.m_translation.m_quad = _mm_add_ps(kRotated.m_quad, rParentModel.m_translation.m_quad);
        rModel.m_rotation.setMul(rParentModel.m_rotation, rLocal.m_rotation);
        rModel.m_scale.m_quad = _mm_mul_ps(rParentModel.m_scale.m_quad, rLocal.m_scale.m_quad);

        hkVector4 kPosition = rModel.m_translation;
        hkQuaternion kRotation = rModel.m_rotation;
        int32_t iControl = 0;
        if (bodyData.m_controlDataIndices) {
            iControl = bodyData.m_controlDataIndices[i];
            if (iControl == -1) {
                continue;
            }
        }
        float fGain = controlPalette[iControl].m_hierarchyGain;
        if (bodyData.m_boneWeights) {
            fGain = bodyData.m_boneWeights[i] * fGain;
        }
        if (fGain > 0.0f && iParent >= 0) {
            const hkpRigidBody* pParentBody = bodyData.m_rigidBodies[iParent];
            // The parent body's current transform, at scale 1.
            hkQsTransform kParentCurrent;
            kParentCurrent.m_translation = pParentBody->m_motion.m_motionState.m_transform.m_translation;
            kParentCurrent.m_rotation = pParentBody->m_motion.m_motionState.m_sweptTransform.m_rotation1;
            kParentCurrent.m_scale.m_quad = _mm_set1_ps(1.0f);
            // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
            hkQsTransform kTarget;
            hkVector4 kTargetRotated;
            kTargetRotated.setRotatedDir(kParentCurrent.m_rotation, rLocal.m_translation);
            kTarget.m_translation.m_quad = _mm_add_ps(kTargetRotated.m_quad, kParentCurrent.m_translation.m_quad);
            kTarget.m_rotation.setMul(kParentCurrent.m_rotation, rLocal.m_rotation);
            kTarget.m_scale.m_quad = _mm_mul_ps(kParentCurrent.m_scale.m_quad, rLocal.m_scale.m_quad);
            kRotation.setSlerp(kRotation, kTarget.m_rotation, fGain);
            // EXE-INLINED none: hkVector4::setInterpolate4
            const __m128 kT = _mm_set1_ps(fGain);
            kPosition.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, kT), kPosition.m_quad),
                _mm_mul_ps(kT, kTarget.m_translation.m_quad)
            );
        }

        const hkpRigidBody* pBody = bodyData.m_rigidBodies[i];
        hkQsTransform& rBodyOut = apOut[2 * i];
        hkVector4 kCenterOfMass;
        kCenterOfMass.setRotatedDir(kRotation, pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal);
        rBodyOut.m_translation.m_quad = _mm_add_ps(kCenterOfMass.m_quad, kPosition.m_quad);
        rBodyOut.m_rotation = kRotation;
        rBodyOut.m_scale = hkVector4Zero;
    }
}
