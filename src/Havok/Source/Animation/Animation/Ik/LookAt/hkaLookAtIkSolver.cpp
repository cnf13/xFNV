#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The intermediates are double, rounded to float where the game stores them. The comparisons decide NaN cases as
// written.
hkBool hkaLookAtIkSolver::solve(
    const Setup& setup,
    const hkVector4& targetMS,
    float gain,
    hkQsTransform& neckModelSpaceInOut,
    const RangeLimits* range
) {
    auto load = [](const hkVector4& v) { return _mm_load_ps(&reinterpret_cast<const float*>(&v.m_quad)[0]); };
    auto store = [](hkVector4& v, __m128 x) { _mm_store_ps(&reinterpret_cast<float*>(&v.m_quad)[0], x); };

    // The cone's half angle, or with range limits the largest of the four (a NaN limit is taken).
    float fLimitAngle = setup.m_limitAngle;
    if (range) {
        fLimitAngle = hkMath::fabs(range->m_limitAngleLeft);
        const float fRight = hkMath::fabs(range->m_limitAngleRight);
        if (!(fRight < fLimitAngle)) {
            fLimitAngle = fRight;
        }
        const float fUp = hkMath::fabs(range->m_limitAngleUp);
        if (!(fUp < fLimitAngle)) {
            fLimitAngle = fUp;
        }
        const float fDown = hkMath::fabs(range->m_limitAngleDown);
        if (!(fDown < fLimitAngle)) {
            fLimitAngle = fDown;
        }
    }

    // The unit direction from the neck to the target, and the distance.
    hkVector4 kToTarget;
    store(kToTarget, _mm_sub_ps(load(targetMS), load(neckModelSpaceInOut.m_translation)));
    const float fDistance = _mm_cvtss_f32(_mm_sqrt_ss(kToTarget.dot3(kToTarget).getQuad()));
    hkVector4 kDirection = kToTarget;
    kDirection.normalize3();
    const float fCosToLimitAxis = setup.m_limitAxisMS.dot3(kDirection);

    // The locals are deliberately shared: the limit rotation's axis sits where the final rotation
    // goes later, and the limit rotation where the final rotation's axis goes, so the final setAxisAngle is handed an
    // object still holding the limit axis.
    hkQuaternion kRotation;
    hkQuaternion kLimitRotation;
    hkBool result;
    result.m_bool = 1;
    // Outside the cone: turn the limit axis by the half angle towards the target instead.
    if (std::cos(fLimitAngle) > fCosToLimitAxis) {
        hkVector4& kLimitAxis = kRotation.m_vec;
        kLimitAxis.setCross(setup.m_limitAxisMS, kDirection);
        kLimitAxis.normalize3();
        kLimitRotation.setAxisAngle(kLimitAxis, fLimitAngle);
        kDirection.setRotatedDir(kLimitRotation, setup.m_limitAxisMS);
        result.m_bool = 0;
    }

    if (range) {
        // The basis (limit axis, up x limit axis, up), the direction in it as spherical coordinates, clamped to the
        // ranges, and back.
        hkMatrix3 kBasis;
        store(kBasis.m_col0, load(setup.m_limitAxisMS));
        kBasis.m_col1.setCross(range->m_upAxisMS, setup.m_limitAxisMS);
        store(kBasis.m_col2, load(range->m_upAxisMS));
        hkVector4 kUnclamped;
        store(kUnclamped, load(kDirection));
        hkMatrix3 kBasisTransposed;
        kBasisTransposed.setTranspose(kBasis);
        hkVector4 kSpherical;
        kSpherical.setMul3(kBasisTransposed, kDirection);
        sphericalFromCartesian(kSpherical);
        const __m128 kMax = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(HK_REAL_MAX), _mm_set_ss(range->m_limitAngleUp)),
            _mm_unpacklo_ps(_mm_set_ss(range->m_limitAngleLeft), _mm_set_ss(0.0f))
        );
        const __m128 kMin = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(-HK_REAL_MAX), _mm_set_ss(range->m_limitAngleDown)),
            _mm_unpacklo_ps(_mm_set_ss(range->m_limitAngleRight), _mm_set_ss(0.0f))
        );
        hkVector4 kClamped;
        store(kClamped, _mm_max_ps(_mm_min_ps(load(kSpherical), kMax), kMin));
        cartesianFromSpherical(kClamped);
        kDirection.setMul3(kBasis, kClamped);
        // Clamped when x, y or z moved by more than 0.001.
        const __m128 kDiff = _mm_and_ps(
            _mm_sub_ps(load(kDirection), load(kUnclamped)),
            _mm_load_ps(&reinterpret_cast<const float*>(&hkVector4::absMask)[0])
        );
        if ((_mm_movemask_ps(_mm_cmple_ps(kDiff, _mm_set1_ps(0.001f))) & 7) != 7) {
            result.m_bool = 0;
        }
    }

    // The rotation taking the bone's forward axis onto the direction, scaled by gain.
    hkVector4 kFwdMS;
    kFwdMS.setRotatedDir(neckModelSpaceInOut.m_rotation, setup.m_fwdLS);
    // The axis goes in the limit rotation's slot (see above).
    hkVector4& kAxis = kLimitRotation.m_vec;
    kAxis.setCross(kFwdMS, kDirection);
    kAxis.normalize3();
    const float fCos = kFwdMS.dot3(kDirection);
    float fAngle;
    if (!(std::fabsf(fCos) >= 1.0f)) {
        fAngle = std::acos(fCos);
    }
    else {
        fAngle = fCos > 0.0f ? 0.0f : 3.1415927f;
    }
    // kRotation still holds the limit axis here when the limit block ran.
    kRotation.setAxisAngle(kAxis, static_cast<float>(static_cast<double>(fAngle) * gain));

    // With the eye off the forward axis, turn back about the forward axis by the angle that puts the eye's line of
    // sight on the target.
    const hkVector4& kEyeLS = setup.m_eyePositionLS;
    if (_mm_cvtss_f32(_mm_sqrt_ss(kEyeLS.dot3(kEyeLS).getQuad())) > 0.0f) {
        const hkVector4& kFwdLS = setup.m_fwdLS;
        hkVector4 kSide;
        kSide.setCross(kFwdLS, kEyeLS);
        kSide.normalize3();
        hkVector4 kUp;
        kUp.setCross(kSide, kFwdLS);
        kUp.normalize3();
        const float fEyeOffset = _mm_cvtss_f32(
            _mm_div_ps(kUp.dot3(kEyeLS).getQuad(), _mm_sqrt_ss(kUp.dot3(kUp).getQuad()))
        );
        const float fDistanceSq = static_cast<float>(static_cast<double>(fDistance) * fDistance);
        const float fOffsetSq = static_cast<float>(static_cast<double>(fEyeOffset) * fEyeOffset);
        const float fDiff = std::fabsf(
            static_cast<float>(static_cast<double>(fDistanceSq) - static_cast<double>(fOffsetSq))
        );
        float fLeg = std::sqrt(fDiff);
        const float fLegSq = static_cast<float>(static_cast<double>(fLeg) * fLeg);
        const double dS = fDistanceSq;
        const double dQ = fOffsetSq;
        const double dR = fLegSq;
        const double dTwoS = dS * 2.0;
        const double dA = ((2.0 * dR) * dQ) - (dR * dTwoS);
        const double dB = (dS * dS) + dA;
        const double dC = (dTwoS * dQ) + dB;
        const double dD = dC + (dR * dR);
        const float fSum = std::fabsf(static_cast<float>((dQ * dQ) + dD));
        const float fRoot = std::sqrt(fSum);
        const float fHyp = static_cast<float>((static_cast<double>(fRoot) * 0.5) / fLeg);
        float fTwist = 0.0f;
        if (fHyp != 0.0f) {
            fTwist = hkMath::acos(static_cast<float>(static_cast<double>(fEyeOffset) / fHyp));
        }
        hkVector4 kEyeMS;
        kEyeMS.setRotatedDir(neckModelSpaceInOut.m_rotation, setup.m_eyePositionLS);
        hkVector4 kTwistAxis;
        kTwistAxis.setCross(kFwdMS, kEyeMS);
        kTwistAxis.normalize3();
        hkQuaternion kTwist;
        kTwist.setAxisAngle(kTwistAxis, -fTwist);
        kRotation.mul(kTwist);
    }

    hkQuaternion kResult;
    kResult.setMul(kRotation, neckModelSpaceInOut.m_rotation);
    store(neckModelSpaceInOut.m_rotation.m_vec, load(kResult.m_vec));
    return result;
}

// Vectors shorter than the double 9.999999747378752e-05 become zero, and a NaN length goes on to the angles. z /
// length, the arc sine and the angle are rounded to float.
void hkaLookAtIkSolver::sphericalFromCartesian(hkVector4& cartesianInSphericalOut) {
    const float fLength = cartesianInSphericalOut.length3();
    if (fLength < 9.999999747378752e-05) {
        cartesianInSphericalOut.m_quad = _mm_setzero_ps();
        return;
    }
    const float fX = XFNV_M128_F32(cartesianInSphericalOut.m_quad, 0);
    const float fY = XFNV_M128_F32(cartesianInSphericalOut.m_quad, 1);
    const float fSinElevation = XFNV_M128_F32(cartesianInSphericalOut.m_quad, 2) / fLength;
    const float fElevation = hkMath::asin(fSinElevation);
    const float fAzimuth = lookAtAtan2(fY, fX);
    cartesianInSphericalOut.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(fLength), _mm_set_ss(fElevation)),
        _mm_unpacklo_ps(_mm_set_ss(fAzimuth), _mm_set_ss(zero))
    );
}

double hkaLookAtIkSolver::lookAtAtan2(float y, float x) {
    return hkMath_atan2fApproximation(y, x);
}

void hkaLookAtIkSolver::cartesianFromSpherical(hkVector4& sphericalInCartesianOut) {
    uint32_t v4;
    uint32_t v10;
    uint32_t v11;
    float v6 = XFNV_M128_F32(sphericalInCartesianOut.m_quad, 0);
    float v1 = XFNV_M128_F32(sphericalInCartesianOut.m_quad, 1);
    float v5 = XFNV_M128_F32(sphericalInCartesianOut.m_quad, 2);
    float v2 = std::cos(static_cast<double>(v5));
    float v3 = v2 * v6;
    float v7 = std::cos(static_cast<double>(v1));
    *reinterpret_cast<float*>(&v11) = v7 * v3;
    float v8 = std::sin(static_cast<double>(v1));
    *reinterpret_cast<float*>(&v4) = v8 * v3;
    float v9 = std::sin(static_cast<double>(v5));
    *reinterpret_cast<float*>(&v10) = v9 * v6;
    sphericalInCartesianOut.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v10))),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(v4)),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
}

double hkMath::asin(float r) {
    double v1 = r;
    float absR = std::fabs(static_cast<double>(r));
    if (!(absR >= 1.0f)) {
        return static_cast<float>(std::asin(v1));
    }

    if (!(v1 > 0.0)) {
        return -1.5707964f;
    }

    return 1.5707964f;
}
