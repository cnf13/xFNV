#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <xmmintrin.h>

// Helpers that move a motion state's swept transform.
namespace hkSweptTransformUtil {
    // Moves the centre of rotation of ms to the local offset.
    void setCentreOfRotationLocal(const hkVector4& newCenterOfRotation, hkMotionState& motionState);
    // Moves ms to pos without sweeping.
    void warpToPosition(const hkVector4& position, hkMotionState& ms);
    // Turns ms to rotation without sweeping, keeping its position.
    void warpToRotation(const hkQuaternion& rotation, hkMotionState& ms);
    // Stores the step's start time and inverse duration in the w of the swept centres of mass.
    void setTimeInformation(float startTime, float invDeltaTime, hkMotionState& motionState);
    // Moves ms to position and rotation without sweeping.
    void warpTo(const hkVector4& position, const hkQuaternion& rotation, hkMotionState& ms);
    // Moves ms to transform without sweeping.
    void warpTo(const hkTransform& transform, hkMotionState& ms);
    // The transform sweptTrans has at time.
    void lerp2(const hkSweptTransform& sweptTrans, float t, hkTransform& transformOut);
    // Moves motionState back to time on its swept transform.
    void backStepMotionState(float time, hkMotionState& motionState);
    // Stops motionState at time: both ends of its swept transform become the transform at time.
    void freezeMotionState(float time, hkMotionState& motionState);
    // Keys ms to reach pos1 and rot1 at the end of the step stepInfo describes.
    void
    keyframeMotionState(const hkStepInfo& stepInfo, const hkVector4& pos1, const hkQuaternion& rot1, hkMotionState& ms);
    // The largest velocity component a motion state may step with, in each lane (the SDK's extern of
    // hkSweptTransformUtil.h, read by _stepMotionState and hkRigidMotionUtilStep's validity test).
    inline hkQuadReal _stepMotionStateMaxVelf{1e6f, 1e6f, 1e6f, 1e6f};
} // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
    void lerp2Ha(const hkSweptTransform& sweptTrans, float t, float tAddOn, hkTransform& transformOut);
} // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
    void lerp2Rel(const hkSweptTransform& sweptTrans, float r, hkTransform& transformOut);
} // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
    // The relative movement of ms1 against ms0 over deltaTime of the step: in xyz the two centres' linear movements,
    // (com0 - com1 of ms0) * f0 + (com1 - com0 of ms1) * f1 with f = deltaTime * getInvDeltaTime(); in w the sum of
    // both bodies' worst-case angular movements, f * the delta angle's w * the object radius.
    inline void calcTimInfo(const hkMotionState& ms0, const hkMotionState& ms1, float deltaTime, hkVector4& timInfo) {
        const hkSweptTransform& st0 = ms0.m_sweptTransform;
        const hkSweptTransform& st1 = ms1.m_sweptTransform;
        hkVector4 diff0;
        diff0.setSub4(st0.m_centerOfMass0, st0.m_centerOfMass1);
        hkVector4 diff1;
        diff1.setSub4(st1.m_centerOfMass1, st1.m_centerOfMass0);
        const float f0 = deltaTime * st0.getInvDeltaTime();
        const float f1 = deltaTime * st1.getInvDeltaTime();
        timInfo.m_quad = _mm_add_ps(
            _mm_mul_ps(diff0.m_quad, _mm_set1_ps(f0)),
            _mm_mul_ps(diff1.m_quad, _mm_set1_ps(f1))
        );
        timInfo(3) = f0 * ms0.m_deltaAngle(3) * ms0.m_objectRadius + f1 * ms1.m_deltaAngle(3) * ms1.m_objectRadius;
    }

    // The transform at the start of the step: rotation0, with the translation the start centre of mass less the local
    // centre of mass turned by it.
    inline void calcTransAtT0(const hkSweptTransform& sweptTrans, hkTransform& transformOut) {
        transformOut.m_rotation.set(sweptTrans.m_rotation0);
        hkVector4 centerShift;
        centerShift._setRotatedDir(transformOut.m_rotation, sweptTrans.m_centerOfMassLocal);
        transformOut.m_translation.setSub4(sweptTrans.m_centerOfMass0, centerShift);
    }

    // As calcTransAtT0, at the end of the step.
    inline void calcTransAtT1(const hkSweptTransform& sweptTrans, hkTransform& transformOut) {
        transformOut.m_rotation.set(sweptTrans.m_rotation1);
        hkVector4 centerShift;
        centerShift._setRotatedDir(transformOut.m_rotation, sweptTrans.m_centerOfMassLocal);
        transformOut.m_translation.setSub4(sweptTrans.m_centerOfMass1, centerShift);
    }

    // The centre of mass at time t, interpolated between the step's two.
    inline void calcCenterOfMassAt(const hkMotionState& ms, float t, hkVector4& centerOut) {
        const hkSweptTransform& st = ms.m_sweptTransform;
        centerOut.setInterpolate4(st.m_centerOfMass0, st.m_centerOfMass1, st.getInterpolationValue(t));
    }

    // The step's velocities: the centre of mass's movement and the delta angle, each times getInvDeltaTime().
    inline void getVelocity(const hkMotionState& ms, hkVector4& linearVelOut, hkVector4& angularVelOut) {
        linearVelOut.setSub4(ms.m_sweptTransform.m_centerOfMass1, ms.m_sweptTransform.m_centerOfMass0);
        const float idt = ms.m_sweptTransform.getInvDeltaTime();
        linearVelOut.m_quad = _mm_mul_ps(_mm_set1_ps(idt), linearVelOut.m_quad);
        angularVelOut.m_quad = _mm_mul_ps(_mm_set1_ps(idt), ms.m_deltaAngle.m_quad);
    }
} // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
    // One integration step of motionState by the velocities: velocities that are not finite or not below
    // _stepMotionStateMaxVelf in x, y and z are both reset to hkTransformIdentity's first column; the linear velocity
    // is clipped to the state's maximum and moves the centre of mass; the angular one, clipped so that the step's
    // angle stays below the maximum (at most 0.9), turns the rotation by the quaternion of half its step, whose w
    // comes from a polynomial in the squared angle. The swept transform's start becomes the old end, with the step's
    // times in the w lanes, and the transform is taken from the new end.
    inline void _stepMotionState(
        const hkStepInfo& info,
        hkVector4& linearVelocity,
        hkVector4& angularVelocity,
        hkMotionState& motionState
    ) {
        hkSweptTransform& st = motionState.m_sweptTransform;
        {
            const hkQuadReal lin = linearVelocity.m_quad;
            const hkQuadReal ang = angularVelocity.m_quad;
            const int32_t ok = _mm_movemask_ps(
                                   _mm_cmplt_ps(_mm_and_ps(ang, hkVector4::absMask), _stepMotionStateMaxVelf)
                               ) &
                _mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(lin, hkVector4::absMask), _stepMotionStateMaxVelf)) &
                _mm_movemask_ps(_mm_cmpeq_ps(ang, ang)) & _mm_movemask_ps(_mm_cmpeq_ps(lin, lin));
            if ((ok & 7) != 7) {
                linearVelocity = hkTransformIdentity.m_rotation.m_col0;
                angularVelocity = hkTransformIdentity.m_rotation.m_col0;
            }
        }

        st.m_centerOfMass0 = st.m_centerOfMass1;
        st.m_centerOfMass0(3) = info.m_startTime;

        // The squared length summed z + (y + x), against the squared maximum.
        {
            const float maxLinVel = hkUFloat8_intToReal[motionState.m_maxLinearVelocity.m_value];
            const hkQuadReal sq = _mm_mul_ps(linearVelocity.m_quad, linearVelocity.m_quad);
            const float linVelSqrd = XFNV_M128_F32(sq, 2) +
                static_cast<float>(XFNV_M128_F32(sq, 1) + XFNV_M128_F32(sq, 0));
            if (linVelSqrd > maxLinVel * maxLinVel) {
                const float f = (1.0f / std::sqrt(linVelSqrd)) * maxLinVel;
                linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linearVelocity.m_quad);
            }
        }

        st.m_centerOfMass1.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(info.m_deltaTime), linearVelocity.m_quad),
            st.m_centerOfMass1.m_quad
        );
        st.m_centerOfMass1(3) = info.m_invDeltaTime;

        hkQuaternion rotation = st.m_rotation1;
        st.m_rotation0 = rotation;

        // dq = angularVelocity * dt / 2; pre is its squared length times 4 / pi^2.
        const float halfDeltaTime = info.m_deltaTime * 0.5;
        hkQuaternion dq;
        dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(halfDeltaTime), angularVelocity.m_quad);
        const hkQuadReal dqSq = _mm_mul_ps(dq.m_vec.m_quad, dq.m_vec.m_quad);
        const float dqLenSqrd = XFNV_M128_F32(dqSq, 2) +
            static_cast<float>(XFNV_M128_F32(dqSq, 1) + XFNV_M128_F32(dqSq, 0));
        float pre = dqLenSqrd * 0.40528471201320165;
        float maxAngle = hkUFloat8_intToReal[motionState.m_maxAngularVelocity.m_value] * info.m_deltaTime;
        if (maxAngle > 0.8999999761581421) {
            maxAngle = 0.9f;
        }
        const float maxAngleSqrd = maxAngle * maxAngle;
        // Taken also when pre is a NaN.
        if (!(maxAngleSqrd >= pre)) {
            const float f = (1.0f / std::sqrt(pre)) * maxAngle;
            angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), angularVelocity.m_quad);
            dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(f), dq.m_vec.m_quad);
            pre = maxAngleSqrd;
        }
        const float pre2 = pre * pre;
        dq.m_vec(3) = 1.0 - 0.8229479789733887 * pre - 0.1305290013551712 * pre2 - pre2 * (pre * 0.04440800100564957);

        rotation.setMul(dq, rotation);
        rotation.normalize();
        motionState.m_deltaAngle.setAdd4(dq.m_vec, dq.m_vec);
        motionState.m_deltaAngle(3) = std::sqrt(pre) * 3.1415927410125732;
        st.m_rotation1 = rotation;
        motionState.m_transform.m_rotation.set(st.m_rotation1);
        hkVector4 centerShift;
        centerShift.setRotatedDir(motionState.m_transform.m_rotation, st.m_centerOfMassLocal);
        motionState.m_transform.m_translation.m_quad = _mm_sub_ps(st.m_centerOfMass1.m_quad, centerShift.m_quad);
    }

    // Each velocity whose squared length (z + (y + x)) is above its squared maximum (rounded to float), or a NaN, is
    // scaled to the maximum; a NaN one is then reset to hkTransformIdentity's first column.
    inline void _clipVelocities(hkMotionState& motionState, hkVector4& linearVelocity, hkVector4& angularVelocity) {
        const float maxAngular = hkUFloat8_intToReal[motionState.m_maxAngularVelocity.m_value] *
            hkUFloat8_intToReal[motionState.m_maxAngularVelocity.m_value];
        const hkQuadReal linSq = _mm_mul_ps(linearVelocity.m_quad, linearVelocity.m_quad);
        const float linVelSqrd = XFNV_M128_F32(linSq, 2) +
            static_cast<float>(XFNV_M128_F32(linSq, 1) + XFNV_M128_F32(linSq, 0));
        const hkQuadReal angSq = _mm_mul_ps(angularVelocity.m_quad, angularVelocity.m_quad);
        const float angVelSqrd = XFNV_M128_F32(angSq, 2) +
            static_cast<float>(XFNV_M128_F32(angSq, 1) + XFNV_M128_F32(angSq, 0));
        const float maxLinear = hkUFloat8_intToReal[motionState.m_maxLinearVelocity.m_value] *
            hkUFloat8_intToReal[motionState.m_maxLinearVelocity.m_value];
        if (linVelSqrd > maxLinear || linVelSqrd != linVelSqrd) {
            const float f = (1.0f / std::sqrt(linVelSqrd)) *
                hkUFloat8_intToReal[motionState.m_maxLinearVelocity.m_value];
            linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linearVelocity.m_quad);
            if (linVelSqrd != linVelSqrd) {
                linearVelocity = hkTransformIdentity.m_rotation.m_col0;
            }
        }
        if (angVelSqrd > maxAngular || angVelSqrd != angVelSqrd) {
            const float f = (1.0f / std::sqrt(angVelSqrd)) *
                hkUFloat8_intToReal[motionState.m_maxAngularVelocity.m_value];
            angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), angularVelocity.m_quad);
            if (angVelSqrd != angVelSqrd) {
                angularVelocity = hkTransformIdentity.m_rotation.m_col0;
            }
        }
    }
} // namespace hkSweptTransformUtil
