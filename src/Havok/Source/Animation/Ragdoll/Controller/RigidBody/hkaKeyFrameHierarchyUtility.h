#pragma once

#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

class hkpRigidBody;

// Static helpers that drive a ragdoll's rigid bodies to a key-framed pose through velocities. It has no instances;
// only its nested types carry data.
class hkaKeyFrameHierarchyUtility {
public:
    // The previous frame's state of one rigid body.
    struct WorkElem {
        hkVector4 m_prevPosition;
        hkQuaternion m_prevRotation;
        hkVector4 m_prevLinearVelocity;
        hkVector4 m_prevAngularVelocity;
    };

    // The gains and limits that drive a rigid body towards its key-framed pose.
    struct ControlData {
        ControlData();

        hkBool isValid();

        // Havok SDK signature (hkaRagdoll.lib, tied by target).
        static const hkClass& staticClass();

        float m_hierarchyGain;
        float m_velocityDamping;
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

    // One skeleton's key-framed pose, as applyKeyFrame takes it: the world transform of the root, the desired
    // local-space poses of the bones and the previous frame's state of each rigid body.
    struct KeyFrameData {
        hkQsTransform m_worldFromRoot;
        const hkQsTransform* m_desiredPoseLocalSpace;
        WorkElem* m_internalReferencePose;
    };

    // The rigid bodies of a ragdoll with their parents, control data indices and per-bone weights.
    struct BodyData {
        int32_t m_numRigidBodies;
        hkpRigidBody** m_rigidBodies;
        const int16_t* m_parentIndices;
        int32_t* m_controlDataIndices;
        const float* m_boneWeights;
    };

    // The per-body result of a key-frame step (hkaRagdollRigidBodyController::driveToPose passes an array of these to
    // applyKeyFrame).
    struct Output {
        float m_stressSquared;
    };

    // Seeds workElemsOut with each rigid body's current state.
    static void initialize(const BodyData& bodies, WorkElem* internalReferencePose);
    static void applyKeyFrame(
        float m_deltaTime,
        const hkaKeyFrameHierarchyUtility::KeyFrameData& pose,
        const hkaKeyFrameHierarchyUtility::BodyData& bodydata,
        const hkaKeyFrameHierarchyUtility::ControlData* controlPalette,
        hkaKeyFrameHierarchyUtility::Output* output
    );
    // The reflection class hkaKeyFrameHierarchyUtilityClass.
    static const hkClass& staticClass();
};

// A helper of applyKeyFrame, its only caller.
void clampVectorLength(float a1, hkVector4* r_esi);
// A helper of applyKeyFrame, its only caller.
double clampSnapVelocity(float a1, float a2, hkVector4* r_esi);
// applyKeyFrame's first step: for each rigid body, its bone's model transform (its parent's, from apOut, or
// worldFromRoot for a root, times desiredPoseLocalSpace[i]) blended towards the body's current one by the control
// data's m_hierarchyGain (times the bone weight), and the body's desired transform from it.
void calcComPoseUsingHierarchy(
    const hkQsTransform* desiredPoseLocalSpace,
    const hkaKeyFrameHierarchyUtility::BodyData& bodyData,
    const hkaKeyFrameHierarchyUtility::ControlData* controlPalette,
    const hkQsTransform& worldFromRoot,
    hkQsTransform* apOut
);

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaKeyFrameHierarchyUtilityControlDataTypeInfo;
extern hkTypeInfo hkaKeyFrameHierarchyUtilityTypeInfo;
extern hkClass hkaKeyFrameHierarchyUtilityControlDataClass;
extern hkClass hkaKeyFrameHierarchyUtilityClass;
