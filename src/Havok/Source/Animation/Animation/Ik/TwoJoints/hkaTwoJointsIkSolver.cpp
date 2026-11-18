#include "Havok/Source/Animation/Animation/Ik/TwoJoints/hkaTwoJointsIkSolver.h"

#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <bit>
#include <cmath>
#include <x86intrin.h>
#include <xmmintrin.h>

// The timer names that solve's HK_TIMER macros store.
using TimerName3 = const char[3];
static TimerName3 s_szTwoJointsEndListTimer{"lt"};
using TimerName15 = const char[15];
static TimerName15 s_szTwoJointsFixDirectionTimer{"StFIXDIRECTION"};
using TimerName12 = const char[12];
static TimerName12 s_szTwoJointsFixLengthTimer{"StFIXLENGTH"};
using TimerName7 = const char[7];
static TimerName7 s_szTwoJointsInitTimer{"StINIT"};
using TimerName17 = const char[17];
static TimerName17 s_szTwoJointsSolverTimer{"LtTWOJOINTSOLVER"};

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
// offsets last; each model-space write is hkaPose::setBoneModelSpace with PROPAGATE. The hkVector4 and hkQsTransform
// helpers are written out with a fixed operation order: dot products sum z + (y + x); normalize3 is a reciprocal square
// root estimate with one Newton step, zero for a zero vector; setReciprocal3 is a reciprocal estimate with one Newton
// step. The intermediates are double, rounded to float or double where the game stores them.
hkBool hkaTwoJointsIkSolver::solve(const Setup& setup, hkaPose& poseInOut) {
    auto load = [](const hkVector4& v) { return _mm_load_ps(&reinterpret_cast<const float*>(&v.m_quad)[0]); };
    auto loadQuad = [](const hkQuadReal& q) { return _mm_load_ps(&reinterpret_cast<const float*>(&q)[0]); };
    auto store = [](hkVector4& v, __m128 x) { _mm_store_ps(&reinterpret_cast<float*>(&v.m_quad)[0], x); };
    auto splat = [](__m128 x) { return _mm_shuffle_ps(x, x, 0x00); };
    // EXE-INLINED none: hkVector4::dot3
    auto dot3 = [](__m128 a, __m128 b) {
        const __m128 p = _mm_mul_ps(a, b);
        const __m128 xy = _mm_add_ps(_mm_shuffle_ps(p, p, 0x55), p);
        return _mm_add_ps(_mm_shuffle_ps(p, p, 0xAA), xy);
    };
    // EXE-INLINED none: hkVector4::setCross
    auto cross = [](__m128 a, __m128 b) {
        return _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(a, a, 0xC9), _mm_shuffle_ps(b, b, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(a, a, 0xD2), _mm_shuffle_ps(b, b, 0xC9))
        );
    };
    // EXE-INLINED none: hkVector4::setRotatedDir (quaternion): 2 * ((imag x dir) * w + ((w * w - 0.5) * dir +
    // (imag . dir) * quat)), the -0.5 lanes hkQuadRealMinusHalf.
    auto rotatedDir = [&](__m128 q, __m128 d) {
        const __m128 kDot = dot3(d, q);
        const __m128 kW = _mm_shuffle_ps(q, q, 0xFF);
        const __m128 kScaled = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kW, kW), loadQuad(hkQuadRealMinusHalf)), d);
        const __m128 kSum = _mm_add_ps(_mm_mul_ps(cross(q, d), kW), _mm_add_ps(kScaled, _mm_mul_ps(splat(kDot), q)));
        return _mm_add_ps(kSum, kSum);
    };
    // EXE-INLINED none: hkVector4::setRotatedInverseDir (quaternion): as setRotatedDir with the cross product
    // reversed.
    auto rotatedInverseDir = [&](__m128 q, __m128 d) {
        const __m128 kDot = dot3(d, q);
        const __m128 kW = _mm_shuffle_ps(q, q, 0xFF);
        const __m128 kScaled = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kW, kW), loadQuad(hkQuadRealMinusHalf)), d);
        const __m128 kSum = _mm_add_ps(_mm_mul_ps(cross(d, q), kW), _mm_add_ps(kScaled, _mm_mul_ps(splat(kDot), q)));
        return _mm_add_ps(kSum, kSum);
    };
    // EXE-INLINED none: hkVector4::normalize3
    auto normalize3 = [&](__m128 v) {
        const __m128 len2 = dot3(v, v);
        const __m128 r = _mm_rsqrt_ss(len2);
        const __m128 t = _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(len2, r), r));
        const __m128 f =
            _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_set_ss(0.0f)), _mm_mul_ps(_mm_mul_ps(_mm_set_ss(0.5f), r), t));
        return _mm_mul_ps(splat(f), v);
    };
    // EXE-INLINED none: hkaPose::getBoneModelSpace
    auto getBoneModelSpace = [&poseInOut](int32_t boneIdx) -> const hkQsTransform& {
        if (poseInOut.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
            return poseInOut.calculateBoneModelSpace(boneIdx);
        }
        return poseInOut.m_modelPose.m_data[boneIdx];
    };
    // EXE-INLINED none: hkQsTransform::setMulInverseMul: the inverse of a (hkQsTransform::setInverse, kept in
    // arInverse, whose rotation setMul reads), then arInverse * b into arOut.
    auto setMulInverseMul = [&](hkQsTransform& arOut, hkQsTransform& arInverse, const hkQsTransform& a,
                                const hkQsTransform& b) {
        const __m128 kTranslation = _mm_xor_ps(
            rotatedInverseDir(load(a.m_rotation.m_vec), load(a.m_translation)), loadQuad(hkQuadRealNegateMask)
        );
        const __m128 kRotation = _mm_xor_ps(load(a.m_rotation.m_vec), loadQuad(rdata_10C4C00));
        // EXE-INLINED none: hkVector4::setReciprocal3 (w taken as 1, then masked off)
        const __m128 kScale = load(a.m_scale);
        const __m128 kScale3 = _mm_shuffle_ps(kScale, _mm_unpackhi_ps(kScale, loadQuad(hkQuadReal1111)), 0xC4);
        const __m128 kRcp = _mm_rcp_ps(kScale3);
        store(
            arInverse.m_scale,
            _mm_and_ps(
                _mm_sub_ps(_mm_add_ps(kRcp, kRcp), _mm_mul_ps(_mm_mul_ps(kRcp, kScale3), kRcp)), loadQuad(rdata_10C4C10)
            )
        );
        store(arInverse.m_translation, kTranslation);
        store(arInverse.m_rotation.m_vec, kRotation);
        store(arOut.m_translation, _mm_add_ps(rotatedDir(kRotation, load(b.m_translation)), kTranslation));
        arOut.m_rotation.setMul(arInverse.m_rotation, b.m_rotation);
        store(arOut.m_scale, _mm_mul_ps(load(b.m_scale), load(arInverse.m_scale)));
    };
    // EXE-INLINED none: hkaPose::setBoneModelSpace (PROPAGATE): the descendants keep their local transforms, so a
    // descendant whose local transform is dirty gets it from its parent's model transform first. The exe tests the
    // parent index against -1 a second time inside (copying the model transform as the local one, through
    // hkQsTransform::operator= 0xC6CD30 at the last site); the outer test already excludes it, so that branch never
    // runs and is left out. The bone count is read once.
    auto setBoneModelSpace = [&](int32_t boneIdx, const hkQsTransform& arBoneModel) {
        const int32_t numBones = poseInOut.m_skeleton->m_bones.m_size;
        poseInOut.m_boneFlags.m_data[boneIdx] |= hkaPose::F_BONE_INTERNAL_FLAG1;
        for (int32_t b = boneIdx + 1; b < numBones; ++b) {
            const int16_t parentId = poseInOut.m_skeleton->m_parentIndices.m_data[b];
            if (parentId != -1 && (poseInOut.m_boneFlags.m_data[parentId] & hkaPose::F_BONE_INTERNAL_FLAG1)) {
                if (poseInOut.m_boneFlags.m_data[b] & hkaPose::F_BONE_LOCAL_DIRTY) {
                    // Reads the model pose's data before the parent's model transform is computed.
                    const hkQsTransform* pModelPose = poseInOut.m_modelPose.m_data;
                    const hkQsTransform& parentMS = getBoneModelSpace(parentId);
                    hkQsTransform kInverse;
                    setMulInverseMul(poseInOut.m_localPose.m_data[b], kInverse, parentMS, pModelPose[b]);
                    poseInOut.m_boneFlags.m_data[b] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
                }
                poseInOut.m_boneFlags.m_data[b] |= hkaPose::F_BONE_INTERNAL_FLAG1;
                poseInOut.m_modelInSync.m_bool = 0;
            }
        }
        for (int32_t b = boneIdx + 1; b < numBones; ++b) {
            uint32_t& ruiFlags = poseInOut.m_boneFlags.m_data[b];
            if (ruiFlags & hkaPose::F_BONE_INTERNAL_FLAG1) {
                ruiFlags =
                    (ruiFlags & ~static_cast<uint32_t>(hkaPose::F_BONE_INTERNAL_FLAG1)) | hkaPose::F_BONE_MODEL_DIRTY;
            }
        }
        poseInOut.m_modelPose.m_data[boneIdx] = arBoneModel;
        poseInOut.m_boneFlags.m_data[boneIdx] = hkaPose::F_BONE_LOCAL_DIRTY;
        poseInOut.m_localInSync.m_bool = 0;
    };
    // EXE-INLINED none: HK_TIMER_SPLIT_LIST and HK_TIMER_END_LIST
    auto timer = [](const char* apName) {
        hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
        if (pStream->m_end < pStream->m_capacityMinus16) {
            // The stream's buffer is raw memory.
            auto* pCommand = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
            pCommand->m_commandAndMonitor = apName;
            pCommand->m_time0 = static_cast<uint32_t>(__rdtsc());
            pStream->m_end = reinterpret_cast<char*>(pCommand + 1);
        }
    };

    // EXE-INLINED none: HK_TIMER_BEGIN_LIST
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        // The stream's buffer is raw memory.
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerBeginListCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szTwoJointsSolverTimer;
        pBegin->m_nameOfFirstSplit = s_szTwoJointsInitTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }

    // With nothing enforced it returns at once without closing the timer list, as the game does.
    if (setup.m_enforceEndPosition.m_bool || setup.m_enforceEndRotation.m_bool) {
        // The twist bones' transforms relative to their joints, the identity without a twist bone.
        hkQsTransform firstTwistOffset;
        hkQsTransform secondTwistOffset;
        // EXE-INLINED none: hkQsTransform::setIdentity
        store(firstTwistOffset.m_translation, _mm_setzero_ps());
        store(firstTwistOffset.m_rotation.m_vec, hkQuadReal0001);
        store(firstTwistOffset.m_scale, _mm_set1_ps(1.0f));
        store(secondTwistOffset.m_translation, _mm_setzero_ps());
        store(secondTwistOffset.m_rotation.m_vec, hkQuadReal0001);
        store(secondTwistOffset.m_scale, _mm_set1_ps(1.0f));
        if (setup.m_firstJointTwistIdx >= 0) {
            const hkQsTransform& firstTwistMS = getBoneModelSpace(setup.m_firstJointTwistIdx);
            const hkQsTransform& firstJointMS = getBoneModelSpace(setup.m_firstJointIdx);
            hkQsTransform kInverse;
            setMulInverseMul(firstTwistOffset, kInverse, firstJointMS, firstTwistMS);
        }
        if (setup.m_secondJointTwistIdx >= 0) {
            const hkQsTransform& secondTwistMS = getBoneModelSpace(setup.m_secondJointTwistIdx);
            const hkQsTransform& secondJointMS = getBoneModelSpace(setup.m_secondJointIdx);
            hkQsTransform kInverse;
            setMulInverseMul(secondTwistOffset, kInverse, secondJointMS, secondTwistMS);
        }

        if (setup.m_enforceEndPosition.m_bool) {
            timer(s_szTwoJointsFixLengthTimer);
            const hkQsTransform& firstJointMS = getBoneModelSpace(setup.m_firstJointIdx);
            const hkQsTransform& secondJointMS = getBoneModelSpace(setup.m_secondJointIdx);
            const hkQsTransform& endBoneMS = getBoneModelSpace(setup.m_endBoneIdx);
            // Where the end bone's offset point is and where it has to go: with the end rotation enforced, the
            // target moves back by the offset turned into the target rotation.
            hkVector4 endTargetMS;
            hkVector4 endPosMS;
            if (!setup.m_enforceEndRotation.m_bool) {
                store(endTargetMS, load(setup.m_endTargetMS));
                endPosMS.setTransformedPos(endBoneMS, setup.m_endBoneOffsetLS);
            }
            else {
                const __m128 kOffset =
                    rotatedInverseDir(load(setup.m_endBoneRotationOffsetLS.m_vec), load(setup.m_endBoneOffsetLS));
                const __m128 kOffsetMS = rotatedDir(load(setup.m_endTargetRotationMS.m_vec), kOffset);
                store(endTargetMS, _mm_sub_ps(load(setup.m_endTargetMS), kOffsetMS));
                store(endPosMS, load(endBoneMS.m_translation));
            }
            const __m128 kFirstToSecond =
                _mm_sub_ps(load(secondJointMS.m_translation), load(firstJointMS.m_translation));
            const __m128 kSecondToEnd = _mm_sub_ps(load(endPosMS), load(secondJointMS.m_translation));
            const __m128 kFirstToTarget = _mm_sub_ps(load(endTargetMS), load(firstJointMS.m_translation));

            // The hinge axis in model space, from the second joint's normalized rotation.
            hkQuaternion kSecondRotation = getBoneModelSpace(setup.m_secondJointIdx).m_rotation;
            kSecondRotation.normalize();
            const __m128 kHingeAxis = rotatedDir(load(kSecondRotation.m_vec), load(setup.m_hingeAxisLS));
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
            const __m128 kFirstAcrossN = _mm_xor_ps(normalize3(kFirstAcross), loadQuad(rdata_10C4C00));
            const float fCurrentCosine = _mm_cvtss_f32(dot3(kFirstAcrossN, normalize3(kSecondAcross)));

            hkQsTransform& secondJointOut = poseInOut.accessBoneModelSpace(setup.m_secondJointIdx, hkaPose::PROPAGATE);
            const hkQuaternion kOldSecondRotation = secondJointOut.m_rotation;
            const __m128 kAxis = rotatedDir(load(kOldSecondRotation.m_vec), load(setup.m_hingeAxisLS));
            // EXE-INLINED none: hkVector4::fastNormalize3 (rsqrtss alone)
            hkVector4 hingeAxisMS;
            store(hingeAxisMS, _mm_mul_ps(splat(_mm_rsqrt_ss(dot3(kAxis, kAxis))), kAxis));
            const float fTargetAngle = hkMath::acos(fCosine);
            const float fAngleDiff = fTargetAngle - hkMath::acos(fCurrentCosine);
            const float fAngle = fAngleDiff * setup.m_firstJointIkGain;
            hkQuaternion kHingeRotation;
            kHingeRotation.setAxisAngle(hingeAxisMS, fAngle);
            secondJointOut.m_rotation.setMul(kHingeRotation, kOldSecondRotation);
            secondJointOut.m_rotation.normalize();

            // The first joint turns the end's direction towards the target's.
            timer(s_szTwoJointsFixDirectionTimer);
            const hkQsTransform& firstJointNowMS = getBoneModelSpace(setup.m_firstJointIdx);
            hkVector4 endNowMS;
            if (setup.m_enforceEndRotation.m_bool) {
                store(endNowMS, load(getBoneModelSpace(setup.m_endBoneIdx).m_translation));
            }
            else {
                endNowMS.setTransformedPos(getBoneModelSpace(setup.m_endBoneIdx), setup.m_endBoneOffsetLS);
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
            const hkQsTransform& endBoneMS = getBoneModelSpace(setup.m_endBoneIdx);
            hkQsTransform kNewEndBoneMS = endBoneMS;
            kNewEndBoneMS.m_rotation.setSlerp(endBoneMS.m_rotation, kEndRotationMS, setup.m_secondJointIkGain);
            setBoneModelSpace(setup.m_endBoneIdx, kNewEndBoneMS);
        }

        // The twist bones back at their offsets from the moved joints.
        if (setup.m_firstJointTwistIdx >= 0) {
            const hkQsTransform& firstJointMS = getBoneModelSpace(setup.m_firstJointIdx);
            // EXE-INLINED none: hkQsTransform::setMul
            hkQsTransform kTwistMS;
            store(
                kTwistMS.m_translation,
                _mm_add_ps(
                    rotatedDir(load(firstJointMS.m_rotation.m_vec), load(firstTwistOffset.m_translation)),
                    load(firstJointMS.m_translation)
                )
            );
            kTwistMS.m_rotation.setMul(firstJointMS.m_rotation, firstTwistOffset.m_rotation);
            store(kTwistMS.m_scale, _mm_mul_ps(load(firstJointMS.m_scale), load(firstTwistOffset.m_scale)));
            setBoneModelSpace(setup.m_firstJointTwistIdx, kTwistMS);
        }
        if (setup.m_secondJointTwistIdx >= 0) {
            const hkQsTransform& secondJointMS = getBoneModelSpace(setup.m_secondJointIdx);
            // EXE-INLINED none: hkQsTransform::setMul
            hkQsTransform kTwistMS;
            store(
                kTwistMS.m_translation,
                _mm_add_ps(
                    rotatedDir(load(secondJointMS.m_rotation.m_vec), load(secondTwistOffset.m_translation)),
                    load(secondJointMS.m_translation)
                )
            );
            kTwistMS.m_rotation.setMul(secondJointMS.m_rotation, secondTwistOffset.m_rotation);
            store(kTwistMS.m_scale, _mm_mul_ps(load(secondJointMS.m_scale), load(secondTwistOffset.m_scale)));
            setBoneModelSpace(setup.m_secondJointTwistIdx, kTwistMS);
        }

        timer(s_szTwoJointsEndListTimer);
    }
    return 1;
}
