#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The hkVector4 helpers (dot3, cross, normalize3, setRotatedDir) are written out with a fixed operation order: dot
// products sum z + (y + x); normalize3 is a reciprocal square root estimate with one Newton step, 0.5 * r * (3 - x * r
// * r), zero for a zero vector. Scalar steps are written packed: only lane 0 is read. The intermediates are double,
// rounded to float where the game stores them. The comparisons decide NaN cases as written.
hkBool* hkaLookAtIkSolver::solve(
    hkBool* apResult,
    const Setup& setup,
    const hkVector4& targetMS,
    float gain,
    hkQsTransform& neckModelSpaceInOut,
    const RangeLimits* range
) {
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
    // EXE-INLINED none: hkVector4::normalize3
    auto normalize3 = [&dot3](__m128 v) {
        const __m128 len2 = dot3(v, v);
        const __m128 r = _mm_rsqrt_ss(len2);
        const __m128 t = _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(len2, r), r));
        const __m128 f =
            _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_set_ss(0.0f)), _mm_mul_ps(_mm_mul_ps(_mm_set_ss(0.5f), r), t));
        return _mm_mul_ps(_mm_shuffle_ps(f, f, 0), v);
    };
    // EXE-INLINED none: hkVector4::setRotatedDir (quaternion): 2 * ((imag x dir) * w + ((w * w - 0.5) * dir +
    // (imag . dir) * quat)), the dot product z + (y + x), the -0.5 lanes hkQuadRealMinusHalf.
    auto rotatedDir = [&dot3, &cross](const hkQuaternion& q, const hkVector4& d) {
        const __m128 kQ = _mm_load_ps(&reinterpret_cast<const float*>(&q.m_vec.m_quad)[0]);
        const __m128 kD = _mm_load_ps(&reinterpret_cast<const float*>(&d.m_quad)[0]);
        const __m128 kDot = dot3(kQ, kD);
        const __m128 kW = _mm_shuffle_ps(kQ, kQ, 0xFF);
        const __m128 kScaled = _mm_mul_ps(
            _mm_add_ps(_mm_mul_ps(kW, kW), _mm_load_ps(&reinterpret_cast<const float*>(&hkQuadRealMinusHalf)[0])), kD
        );
        const __m128 kSum = _mm_add_ps(
            _mm_mul_ps(cross(kQ, kD), kW), _mm_add_ps(kScaled, _mm_mul_ps(_mm_shuffle_ps(kDot, kDot, 0), kQ))
        );
        return _mm_add_ps(kSum, kSum);
    };
    auto load = [](const hkVector4& v) { return _mm_load_ps(&reinterpret_cast<const float*>(&v.m_quad)[0]); };
    auto store = [](hkVector4& v, __m128 x) { _mm_store_ps(&reinterpret_cast<float*>(&v.m_quad)[0], x); };

    // The cone's half angle, or with range limits the largest of the four (a NaN limit is taken).
    float fLimitAngle = setup.m_limitAngle;
    if (range) {
        // EXE-INLINED none: fabs
        fLimitAngle = __builtin_fabsf(range->m_limitAngleLeft);
        const float fRight = __builtin_fabsf(range->m_limitAngleRight);
        if (!(fRight < fLimitAngle)) {
            fLimitAngle = fRight;
        }
        const float fUp = __builtin_fabsf(range->m_limitAngleUp);
        if (!(fUp < fLimitAngle)) {
            fLimitAngle = fUp;
        }
        const float fDown = __builtin_fabsf(range->m_limitAngleDown);
        if (!(fDown < fLimitAngle)) {
            fLimitAngle = fDown;
        }
    }

    // The unit direction from the neck to the target, and the distance.
    const __m128 kToTarget = _mm_sub_ps(load(targetMS), load(neckModelSpaceInOut.m_translation));
    const float fDistance = _mm_cvtss_f32(_mm_sqrt_ss(dot3(kToTarget, kToTarget)));
    hkVector4 kDirection;
    store(kDirection, normalize3(kToTarget));
    const float fCosToLimitAxis = _mm_cvtss_f32(dot3(load(setup.m_limitAxisMS), load(kDirection)));

    // The locals are deliberately shared, as the game does: the limit rotation's axis sits where the final rotation
    // goes later, and the limit rotation where the final rotation's axis goes, so the final setAxisAngle is handed an
    // object still holding the limit axis.
    hkQuaternion kRotation;
    hkQuaternion kLimitRotation;
    apResult->m_bool = 1;
    // Outside the cone: turn the limit axis by the half angle towards the target instead.
    if (std::cos(fLimitAngle) > fCosToLimitAxis) {
        hkVector4& kLimitAxis = kRotation.m_vec;
        store(kLimitAxis, normalize3(cross(load(setup.m_limitAxisMS), load(kDirection))));
        kLimitRotation.setAxisAngle(kLimitAxis, fLimitAngle);
        // EXE-INLINED none: hkVector4::setRotatedDir
        store(kDirection, rotatedDir(kLimitRotation, setup.m_limitAxisMS));
        apResult->m_bool = 0;
    }

    if (range) {
        // The basis (limit axis, up x limit axis, up), the direction in it as spherical coordinates, clamped to the
        // ranges, and back.
        hkMatrix3 kBasis;
        store(kBasis.m_col0, load(setup.m_limitAxisMS));
        store(kBasis.m_col1, cross(load(range->m_upAxisMS), load(setup.m_limitAxisMS)));
        store(kBasis.m_col2, load(range->m_upAxisMS));
        hkVector4 kUnclamped;
        store(kUnclamped, load(kDirection));
        hkMatrix3 kBasisTransposed;
        kBasisTransposed.setTranspose(kBasis);
        hkVector4 kSpherical;
        kSpherical.setMul3(kBasisTransposed, kDirection);
        sphericalFromCartesian(kSpherical);
        const __m128 kMax = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(range->m_limitAngleUp)),
            _mm_unpacklo_ps(_mm_set_ss(range->m_limitAngleLeft), _mm_set_ss(0.0f))
        );
        const __m128 kMin = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(range->m_limitAngleDown)),
            _mm_unpacklo_ps(_mm_set_ss(range->m_limitAngleRight), _mm_set_ss(0.0f))
        );
        hkVector4 kClamped;
        store(kClamped, _mm_max_ps(_mm_min_ps(load(kSpherical), kMax), kMin));
        cartesianFromSpherical(kClamped);
        kDirection.setMul3(kBasis, kClamped);
        // Clamped when x, y or z moved by more than 0.001.
        const __m128 kDiff = _mm_and_ps(
            _mm_sub_ps(load(kDirection), load(kUnclamped)),
            _mm_load_ps(&reinterpret_cast<const float*>(&hkVector4_setAbs4_absMask)[0])
        );
        if ((_mm_movemask_ps(_mm_cmple_ps(kDiff, _mm_set1_ps(0.001f))) & 7) != 7) {
            apResult->m_bool = 0;
        }
    }

    // The rotation taking the bone's forward axis onto the direction, scaled by gain.
    hkVector4 kFwdMS;
    // EXE-INLINED none: hkVector4::setRotatedDir
    store(kFwdMS, rotatedDir(neckModelSpaceInOut.m_rotation, setup.m_fwdLS));
    // The axis goes in the limit rotation's slot (see above).
    hkVector4& kAxis = kLimitRotation.m_vec;
    store(kAxis, normalize3(cross(load(kFwdMS), load(kDirection))));
    const float fCos = _mm_cvtss_f32(dot3(load(kFwdMS), load(kDirection)));
    float fAngle;
    if (!(__builtin_fabsf(fCos) >= 1.0f)) {
        fAngle = std::acos(fCos);
    }
    else {
        fAngle = fCos > 0.0f ? 0.0f : 3.1415927f;
    }
    // kRotation still holds the limit axis here when the limit block ran, as the game does.
    kRotation.setAxisAngle(kAxis, static_cast<float>(static_cast<double>(fAngle) * gain));

    // With the eye off the forward axis, turn back about the forward axis by the angle that puts the eye's line of
    // sight on the target.
    const __m128 kEyeLS = load(setup.m_eyePositionLS);
    if (_mm_cvtss_f32(_mm_sqrt_ss(dot3(kEyeLS, kEyeLS))) > 0.0f) {
        const __m128 kFwdLS = load(setup.m_fwdLS);
        const __m128 kSide = normalize3(cross(kFwdLS, kEyeLS));
        const __m128 kUp = normalize3(cross(kSide, kFwdLS));
        const float fEyeOffset = _mm_cvtss_f32(_mm_div_ps(dot3(kUp, kEyeLS), _mm_sqrt_ss(dot3(kUp, kUp))));
        const float fDistanceSq = static_cast<float>(static_cast<double>(fDistance) * fDistance);
        const float fOffsetSq = static_cast<float>(static_cast<double>(fEyeOffset) * fEyeOffset);
        const float fDiff =
            __builtin_fabsf(static_cast<float>(static_cast<double>(fDistanceSq) - static_cast<double>(fOffsetSq)));
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
        const float fSum = __builtin_fabsf(static_cast<float>((dQ * dQ) + dD));
        const float fRoot = std::sqrt(fSum);
        const float fHyp = static_cast<float>((static_cast<double>(fRoot) * 0.5) / fLeg);
        float fTwist = 0.0f;
        if (fHyp != 0.0f) {
            fTwist = hkMath::acos(static_cast<float>(static_cast<double>(fEyeOffset) / fHyp));
        }
        hkVector4 kEyeMS;
        // EXE-INLINED none: hkVector4::setRotatedDir
        store(kEyeMS, rotatedDir(neckModelSpaceInOut.m_rotation, setup.m_eyePositionLS));
        hkVector4 kTwistAxis;
        store(kTwistAxis, normalize3(cross(load(kFwdMS), load(kEyeMS))));
        hkQuaternion kTwist;
        kTwist.setAxisAngle(kTwistAxis, -fTwist);
        kRotation.mul(kTwist);
    }

    hkQuaternion kResult;
    kResult.setMul(kRotation, neckModelSpaceInOut.m_rotation);
    store(neckModelSpaceInOut.m_rotation.m_vec, load(kResult.m_vec));
    return apResult;
}

// The length is computed with packed adds in the order z + (y + x), then a square root; vectors shorter than the double
// 9.999999747378752e-05 become zero, and a NaN length goes on to the angles. z / length, the arc sine and the angle are
// rounded to float.
void hkaLookAtIkSolver::sphericalFromCartesian(hkVector4& cartesianInSphericalOut) {
    // EXE-INLINED none: hkVector4::length3
    const __m128 kSquares = _mm_mul_ps(cartesianInSphericalOut.m_quad, cartesianInSphericalOut.m_quad);
    const __m128 kSum = _mm_add_ps(
        _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ps(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
    );
    const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(kSum));
    if (fLength < 9.999999747378752e-05) {
        cartesianInSphericalOut.m_quad = _mm_setzero_ps();
        return;
    }
    const float fX = cartesianInSphericalOut.m_quad[0];
    const float fY = cartesianInSphericalOut.m_quad[1];
    const float fSinElevation = cartesianInSphericalOut.m_quad[2] / fLength;
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
    float v6 = sphericalInCartesianOut.m_quad[0];
    float v1 = sphericalInCartesianOut.m_quad[1];
    float v5 = sphericalInCartesianOut.m_quad[2];
    float v2 = std::cos((double)v5);
    float v3 = v2 * v6;
    float v7 = std::cos((double)v1);
    *reinterpret_cast<float*>(&v11) = v7 * v3;
    float v8 = std::sin((double)v1);
    *reinterpret_cast<float*>(&v4) = v8 * v3;
    float v9 = std::sin((double)v5);
    *reinterpret_cast<float*>(&v10) = v9 * v6;
    sphericalInCartesianOut.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v10))),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(v4)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
}

double hkMath::asin(float r) {
    double v1 = r;
    float a1a = std::fabs((double)r);
    if (!(a1a >= 1.0f)) {
        return static_cast<float>(std::asin(v1));
    }
    else {
        if (!(v1 > 0.0)) {
            return -1.5707964f;
        }
        else {
            return 1.5707964f;
        }
    }
}
