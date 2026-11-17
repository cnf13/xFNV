#pragma once

#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkaPose;

// Havok's two-joint IK solver: places a chain of two joints and an end bone on a target. Its Setup carries the
// bone indexes, the hinge axis, the angle and gain limits and the target; solve runs the chain over a pose.
class hkaTwoJointsIkSolver : public hkReferencedObject {
public:
    // What one solve call is given.
    struct Setup {
        Setup();

        int16_t m_firstJointIdx;
        int16_t m_secondJointIdx;
        int16_t m_endBoneIdx;
        int16_t m_firstJointTwistIdx;
        int16_t m_secondJointTwistIdx;
        // The 16-byte alignment of the hkVector4/hkQuaternion members is what pads the setup (0xA, 0x34, 0x82).
        hkVector4 m_hingeAxisLS;
        float m_cosineMaxHingeAngle;
        float m_cosineMinHingeAngle;
        float m_maxHingeAngleDifference;
        float m_firstJointIkGain;
        float m_secondJointIkGain;
        hkVector4 m_endTargetMS;
        hkQuaternion m_endTargetRotationMS;
        hkVector4 m_endBoneOffsetLS;
        hkQuaternion m_endBoneRotationOffsetLS;
        hkBool m_enforceEndPosition;
        hkBool m_enforceEndRotation;
    };

    // Poses the setup's chain on its target inside poseInOut.
    static hkBool* solve(hkBool* apResult, const Setup& setup, hkaPose& poseInOut);
};
