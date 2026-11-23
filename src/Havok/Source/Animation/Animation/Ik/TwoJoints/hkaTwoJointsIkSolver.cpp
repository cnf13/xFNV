#include "Havok/Source/Animation/Animation/Ik/TwoJoints/hkaTwoJointsIkSolver.h"

#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

hkaTwoJointsIkSolver::Setup::Setup()
  : m_firstJointIdx(-1), m_secondJointIdx(-1), m_endBoneIdx(-1), m_firstJointTwistIdx(-1), m_secondJointTwistIdx(-1),
    m_hingeAxisLS(hkVector4Zero), m_cosineMaxHingeAngle(-1.0f), m_cosineMinHingeAngle(1.0f),
    m_maxHingeAngleDifference(1.0f), m_firstJointIkGain(1.0f), m_secondJointIkGain(1.0f), m_endTargetMS(hkVector4Zero),
    m_endTargetRotationMS(hkQuaternionIdentity), m_endBoneOffsetLS(hkVector4Zero),
    m_endBoneRotationOffsetLS(hkQuaternionIdentity), m_enforceEndPosition(true), m_enforceEndRotation(false) {}

// The twist bones' offsets from their joints are kept first. With the end position enforced, the second joint turns
// about its hinge axis until the chain's length reaches the target (the law of cosines, clamped to the hinge limits,
// scaled by a gain), then the first joint turns the end towards the target (a damped shortest rotation). With the end
// rotation enforced, the end bone's rotation is slerped towards the target's. The twist bones are put back at their
// offsets last. The intermediates are double, rounded to float or double where they are stored.
hkBool hkaTwoJointsIkSolver::solve(const Setup& setup, hkaPose& poseInOut) {
    auto load = [](const hkVector4& v) { return _mm_load_ps(&reinterpret_cast<const float*>(&v.m_quad)[0]); };
    auto loadQuad = [](const hkQuadReal& q) { return _mm_load_ps(&reinterpret_cast<const float*>(&q)[0]); };
    auto store = [](hkVector4& v, __m128 x) { _mm_store_ps(&reinterpret_cast<float*>(&v.m_quad)[0], x); };
    auto dot3 = [](__m128 a, __m128 b) {
        hkVector4 kA;
        kA.m_quad = a;
        hkVector4 kB;
        kB.m_quad = b;
        return kA.dot3(kB).getQuad();
    };
    auto normalize3 = [](__m128 v) {
        hkVector4 kV;
        kV.m_quad = v;
        kV.normalize3();
        return kV.m_quad;
    };
    HK_TIMER_BEGIN_LIST("TWOJOINTSOLVER", "INIT");

    // With nothing enforced it returns at once without closing the timer list.
    if (setup.m_enforceEndPosition.m_bool || setup.m_enforceEndRotation.m_bool) {
        // The twist bones' transforms relative to their joints, the identity without a twist bone.
        hkQsTransform firstTwistOffset;
        hkQsTransform secondTwistOffset;
        firstTwistOffset.setIdentity();
        secondTwistOffset.setIdentity();
        if (setup.m_firstJointTwistIdx >= 0) {
            const hkQsTransform& firstTwistMS = poseInOut.getBoneModelSpace(setup.m_firstJointTwistIdx);
            const hkQsTransform& firstJointMS = poseInOut.getBoneModelSpace(setup.m_firstJointIdx);
            firstTwistOffset.setMulInverseMul(firstJointMS, firstTwistMS);
        }
        if (setup.m_secondJointTwistIdx >= 0) {
            const hkQsTransform& secondTwistMS = poseInOut.getBoneModelSpace(setup.m_secondJointTwistIdx);
            const hkQsTransform& secondJointMS = poseInOut.getBoneModelSpace(setup.m_secondJointIdx);
            secondTwistOffset.setMulInverseMul(secondJointMS, secondTwistMS);
        }

        if (setup.m_enforceEndPosition.m_bool) {
            HK_TIMER_SPLIT_LIST("FIXLENGTH");
            const hkQsTransform& firstJointMS = poseInOut.getBoneModelSpace(setup.m_firstJointIdx);
            const hkQsTransform& secondJointMS = poseInOut.getBoneModelSpace(setup.m_secondJointIdx);
            const hkQsTransform& endBoneMS = poseInOut.getBoneModelSpace(setup.m_endBoneIdx);
            // Where the end bone's offset point is and where it has to go: with the end rotation enforced, the
            // target moves back by the offset turned into the target rotation.
            hkVector4 endTargetMS;
            hkVector4 endPosMS;
            if (!setup.m_enforceEndRotation.m_bool) {
                store(endTargetMS, load(setup.m_endTargetMS));
                endPosMS.setTransformedPos(endBoneMS, setup.m_endBoneOffsetLS);
            }
            else {
                hkVector4 kOffset;
                kOffset.setRotatedInverseDir(setup.m_endBoneRotationOffsetLS, setup.m_endBoneOffsetLS);
                hkVector4 kOffsetMS;
                kOffsetMS.setRotatedDir(setup.m_endTargetRotationMS, kOffset);
                endTargetMS.setSub4(setup.m_endTargetMS, kOffsetMS);
                store(endPosMS, load(endBoneMS.m_translation));
            }
            const __m128 kFirstToSecond = _mm_sub_ps(
                load(secondJointMS.m_translation),
                load(firstJointMS.m_translation)
            );
            const __m128 kSecondToEnd = _mm_sub_ps(load(endPosMS), load(secondJointMS.m_translation));
            const __m128 kFirstToTarget = _mm_sub_ps(load(endTargetMS), load(firstJointMS.m_translation));

            // The hinge axis in model space, from the second joint's normalized rotation.
            hkQuaternion kSecondRotation = poseInOut.getBoneModelSpace(setup.m_secondJointIdx).m_rotation;
            kSecondRotation.normalize();
            hkVector4 hingeAxis;
            hingeAxis.setRotatedDir(kSecondRotation, setup.m_hingeAxisLS);
            const __m128 kHingeAxis = hingeAxis.m_quad;
            // The two bones split into their parts along the hinge axis and across it.
            const float fFirstAlong = _mm_cvtss_f32(dot3(kHingeAxis, kFirstToSecond));
            const __m128 kFirstAlong = _mm_mul_ps(_mm_set1_ps(fFirstAlong), kHingeAxis);
            const __m128 kFirstAcross = _mm_sub_ps(kFirstToSecond, kFirstAlong);
            const float fSecondAlong = _mm_cvtss_f32(dot3(kHingeAxis, kSecondToEnd));
            const __m128 kSecondAlong = _mm_mul_ps(_mm_set1_ps(fSecondAlong), kHingeAxis);
            const __m128 kSecondAcross = _mm_sub_ps(kSecondToEnd, kSecondAlong);
            const __m128 kAlongDiff = _mm_sub_ps(kFirstAlong, kSecondAlong);
            const float fAlongSq = _mm_cvtss_f32(dot3(kAlongDiff, kAlongDiff));
            const float fCosineMinHingeAngle = setup.m_cosineMinHingeAngle;
            const float fTargetSq = _mm_cvtss_f32(dot3(kFirstToTarget, kFirstToTarget));
            const float fTargetAcrossSq = static_cast<float>(static_cast<double>(fTargetSq) - fAlongSq);
            const float fFirstSq = _mm_cvtss_f32(dot3(kFirstAcross, kFirstAcross));
            const float fSecondSq = _mm_cvtss_f32(dot3(kSecondAcross, kSecondAcross));
            // The law of cosines for the hinge angle that puts the end at the target's distance.
            const double dNumerator = (static_cast<double>(fTargetAcrossSq) - fFirstSq) - fSecondSq;
            const float fFirstLength = std::sqrt(fFirstSq);
            const double dMinusTwoFirst = static_cast<double>(fFirstLength) * -2.0;
            const float fSecondLength = std::sqrt(fSecondSq);
            float fCosine = static_cast<float>(dNumerator / (static_cast<double>(fSecondLength) * dMinusTwoFirst));
            // The hinge limits: a NaN cosine is kept.
            if (fCosine > fCosineMinHingeAngle) {
                fCosine = fCosineMinHingeAngle;
            }
            const float fCosineMaxHingeAngle = setup.m_cosineMaxHingeAngle;
            if (fCosine < fCosineMaxHingeAngle) {
                fCosine = fCosineMaxHingeAngle;
            }
            // The hinge angle now: between the reversed first bone and the second, across the axis.
            const __m128 kFirstAcrossN = _mm_xor_ps(normalize3(kFirstAcross), loadQuad(hkQuadRealNegateXYZMask));
            const float fCurrentCosine = _mm_cvtss_f32(dot3(kFirstAcrossN, normalize3(kSecondAcross)));

            hkQsTransform& secondJointOut = poseInOut.accessBoneModelSpace(setup.m_secondJointIdx, hkaPose::PROPAGATE);
            const hkQuaternion kOldSecondRotation = secondJointOut.m_rotation;
            hkVector4 hingeAxisMS;
            hingeAxisMS.setRotatedDir(kOldSecondRotation, setup.m_hingeAxisLS);
            hingeAxisMS.fastNormalize3();
            const float fTargetAngle = hkMath::acos(fCosine);
            const float fAngleDiff = fTargetAngle - hkMath::acos(fCurrentCosine);
            const float fAngle = fAngleDiff * setup.m_firstJointIkGain;
            hkQuaternion kHingeRotation;
            kHingeRotation.setAxisAngle(hingeAxisMS, fAngle);
            secondJointOut.m_rotation.setMul(kHingeRotation, kOldSecondRotation);
            secondJointOut.m_rotation.normalize();

            // The first joint turns the end's direction towards the target's.
            HK_TIMER_SPLIT_LIST("FIXDIRECTION");
            const hkQsTransform& firstJointNowMS = poseInOut.getBoneModelSpace(setup.m_firstJointIdx);
            hkVector4 endNowMS;
            if (setup.m_enforceEndRotation.m_bool) {
                store(endNowMS, load(poseInOut.getBoneModelSpace(setup.m_endBoneIdx).m_translation));
            }
            else {
                endNowMS.setTransformedPos(poseInOut.getBoneModelSpace(setup.m_endBoneIdx), setup.m_endBoneOffsetLS);
            }
            const float fGain = setup.m_maxHingeAngleDifference;
            hkVector4 kFrom;
            store(kFrom, normalize3(_mm_sub_ps(load(endNowMS), load(firstJointNowMS.m_translation))));
            hkVector4 kTo;
            store(kTo, normalize3(_mm_sub_ps(load(endTargetMS), load(firstJointNowMS.m_translation))));
            hkQuaternion kDirectionRotation;
            kDirectionRotation.setShortestRotationDamped(fGain, kFrom, kTo);
            hkQsTransform& firstJointOut = poseInOut.accessBoneModelSpace(setup.m_firstJointIdx, hkaPose::PROPAGATE);
            const hkQuaternion kOldFirstRotation = firstJointOut.m_rotation;
            firstJointOut.m_rotation.setMul(kDirectionRotation, kOldFirstRotation);
            firstJointOut.m_rotation.normalize();
        }

        if (setup.m_enforceEndRotation.m_bool) {
            hkQuaternion kEndRotationMS;
            kEndRotationMS.setMulInverse(setup.m_endTargetRotationMS, setup.m_endBoneRotationOffsetLS);
            const hkQsTransform& endBoneMS = poseInOut.getBoneModelSpace(setup.m_endBoneIdx);
            hkQsTransform kNewEndBoneMS = endBoneMS;
            kNewEndBoneMS.m_rotation.setSlerp(endBoneMS.m_rotation, kEndRotationMS, setup.m_secondJointIkGain);
            poseInOut.setBoneModelSpace(setup.m_endBoneIdx, kNewEndBoneMS, hkaPose::PROPAGATE);
        }

        // The twist bones back at their offsets from the moved joints.
        if (setup.m_firstJointTwistIdx >= 0) {
            const hkQsTransform& firstJointMS = poseInOut.getBoneModelSpace(setup.m_firstJointIdx);
            hkQsTransform kTwistMS;
            kTwistMS.setMul(firstJointMS, firstTwistOffset);
            poseInOut.setBoneModelSpace(setup.m_firstJointTwistIdx, kTwistMS, hkaPose::PROPAGATE);
        }
        if (setup.m_secondJointTwistIdx >= 0) {
            const hkQsTransform& secondJointMS = poseInOut.getBoneModelSpace(setup.m_secondJointIdx);
            hkQsTransform kTwistMS;
            kTwistMS.setMul(secondJointMS, secondTwistOffset);
            poseInOut.setBoneModelSpace(setup.m_secondJointTwistIdx, kTwistMS, hkaPose::PROPAGATE);
        }

        HK_TIMER_END_LIST();
    }
    return 1;
}
