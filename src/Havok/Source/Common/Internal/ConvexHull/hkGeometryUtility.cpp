#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"

#include "Havok/Source/Common/Base/Algorithm/PseudoRandom/hkPseudoRandomGenerator.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The eigenvectors of the symmetric 3x3 matrix apMatrix (three columns), in place: the matrix is scaled by its longest
// column, reduced to tridiagonal form by one rotation that zeroes column 2's x (the identity when it is zero already),
// then diagonalised by the implicit QL iteration (Numerical Recipes' tqli), each rotation applied to the columns. A
// value under 1e-06 counts as zero; each eigenvalue gets aiMaxIterations sweeps. Writes 1 to *apResult when every
// eigenvalue converged, else 0, and returns apResult. Every intermediate is kept as a float.
uint8_t* diagonalizeSymmetricMatrix3(uint8_t* apResult, __m128* apMatrix, int32_t aiMaxIterations) {
    float afOff[3];
    float afDiag[3];
    float fMax = 0.0f;
    for (int32_t i = 0; i < 3; ++i) {
        const __m128 kSq = _mm_mul_ps(apMatrix[i], apMatrix[i]);
        const float fLength = _mm_cvtss_f32(
            _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)))
        );
        if (fLength > fMax) {
            fMax = fLength;
        }
    }
    if (0.0 != fMax) {
        for (int32_t k = 0; k < 3; ++k) {
            XFNV_M128_F32(apMatrix[0], k) = XFNV_M128_F32(apMatrix[0], k) / fMax;
            XFNV_M128_F32(apMatrix[1], k) = XFNV_M128_F32(apMatrix[1], k) / fMax;
            XFNV_M128_F32(apMatrix[2], k) = XFNV_M128_F32(apMatrix[2], k) / fMax;
        }
    }
    afDiag[0] = XFNV_M128_F32(apMatrix[0], 0);
    if (XFNV_M128_F32(apMatrix[2], 0) == 0.0f) {
        afDiag[1] = XFNV_M128_F32(apMatrix[1], 1);
        afDiag[2] = XFNV_M128_F32(apMatrix[2], 2);
        afOff[0] = XFNV_M128_F32(apMatrix[1], 0);
        afOff[1] = XFNV_M128_F32(apMatrix[2], 1);
        apMatrix[0] = _mm_setzero_ps();
        apMatrix[1] = _mm_setzero_ps();
        apMatrix[2] = _mm_setzero_ps();
        afOff[2] = 0.0f;
        XFNV_M128_F32(apMatrix[0], 0) = 1.0f;
        XFNV_M128_F32(apMatrix[1], 1) = 1.0f;
        XFNV_M128_F32(apMatrix[2], 2) = 1.0f;
    }
    else {
        afOff[2] = 0.0f;
        const float fLength2 = XFNV_M128_F32(apMatrix[2], 0) * XFNV_M128_F32(apMatrix[2], 0) +
            XFNV_M128_F32(apMatrix[1], 0) * XFNV_M128_F32(apMatrix[1], 0);
        const float fLength = std::sqrt(fLength2);
        const float fCos = XFNV_M128_F32(apMatrix[1], 0) / fLength;
        const float fSin = XFNV_M128_F32(apMatrix[2], 0) / fLength;
        const float fQ = (XFNV_M128_F32(apMatrix[2], 2) - XFNV_M128_F32(apMatrix[1], 1)) * fSin +
            (fCos + fCos) * XFNV_M128_F32(apMatrix[2], 1);
        const double dQSin = static_cast<double>(fQ) * fSin;
        afDiag[1] = XFNV_M128_F32(apMatrix[1], 1) + dQSin;
        afDiag[2] = XFNV_M128_F32(apMatrix[2], 2) - dQSin;
        afOff[0] = fLength;
        afOff[1] = XFNV_M128_F32(apMatrix[2], 1) - static_cast<double>(fQ) * fCos;
        XFNV_M128_F32(apMatrix[0], 0) = 1.0f;
        XFNV_M128_F32(apMatrix[1], 0) = 0.0f;
        XFNV_M128_F32(apMatrix[2], 0) = 0.0f;
        XFNV_M128_F32(apMatrix[0], 1) = 0.0f;
        XFNV_M128_F32(apMatrix[0], 2) = 0.0f;
        XFNV_M128_F32(apMatrix[1], 1) = fCos;
        XFNV_M128_F32(apMatrix[2], 1) = fSin;
        XFNV_M128_F32(apMatrix[1], 2) = fSin;
        XFNV_M128_F32(apMatrix[2], 2) = -fCos;
    }
    int32_t iLow = 0;
    for (int32_t iRemaining = 2; iRemaining > -1; --iRemaining, ++iLow) {
        int32_t iIteration = 0;
        if (aiMaxIterations > 0) {
            do {
                int32_t m = iLow;
                while (m <= 1) {
                    const float fAbs = std::fabs(afOff[m]);
                    if (fAbs < 1e-06f) {
                        break;
                    }
                    ++m;
                }
                if (m == iLow) {
                    break;
                }
                float fG = (afDiag[iLow + 1] - afDiag[iLow]) / (afOff[iLow] + afOff[iLow]);
                const float fR2 = fG * fG + 1.0;
                const float fR = std::sqrt(fR2);
                fG = (afDiag[m] - afDiag[iLow]) + afOff[iLow] / (fG < 0.0f ? fG - fR : fG + fR);
                float fSin = 1.0f;
                float fCos = 1.0f;
                float fP = 0.0f;
                for (int32_t i = m - 1; i >= iLow; --i) {
                    const float fF = fSin * afOff[i];
                    const float fB = fCos * afOff[i];
                    const float fAbsF = std::fabs(fF);
                    const float fAbsG = std::fabs(fG);
                    if (fAbsG < fAbsF) {
                        const float fT = -fG / fF;
                        const float fT2 = fT * fT + 1.0;
                        const float fRoot = std::sqrt(fT2);
                        const float fU = -1.0 / fRoot;
                        fCos = fT * fU;
                        fSin = -fU;
                        afOff[i + 1] = fF / fSin;
                    }
                    else {
                        const float fT = -fF / fG;
                        const float fT2 = fT * fT + 1.0;
                        const float fRoot = std::sqrt(fT2);
                        const float fW = -1.0 / fRoot;
                        fSin = fT * fW;
                        fCos = -fW;
                        afOff[i + 1] = fG / fCos;
                    }
                    fG = afDiag[i + 1] - fP;
                    const float fRot = (afDiag[i] - fG) * fSin + (fB + fB) * fCos;
                    fP = fSin * fRot;
                    afDiag[i + 1] = fG + fP;
                    fG = fRot * fCos - fB;
                    for (int32_t k = 0; k < 3; ++k) {
                        const float fZ = XFNV_M128_F32(apMatrix[i + 1], k);
                        XFNV_M128_F32(apMatrix[i + 1], k) = fCos * fZ + XFNV_M128_F32(apMatrix[i], k) * fSin;
                        XFNV_M128_F32(apMatrix[i], k) = XFNV_M128_F32(apMatrix[i], k) * fCos - fZ * fSin;
                    }
                }
                afDiag[iLow] = afDiag[iLow] - fP;
                afOff[iLow] = fG;
                afOff[m] = 0.0f;
                ++iIteration;
            }
            while (iIteration < aiMaxIterations);
        }
        if (iIteration == aiMaxIterations) {
            *apResult = 0;
            return apResult;
        }
    }
    *apResult = 1;
    return apResult;
}

uint8_t* calcEigenVectors(uint8_t* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5) {
    diagonalizeSymmetricMatrix3(a1, a2, 30);
    if (*a1) {
        *a3 = *a2;
        *a4 = a2[1];
        __m128 v14 = a2[2];
        *a5 = v14;
        __m128 v15 = _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(*a3, *a3, 201), _mm_shuffle_ps(*a4, *a4, 210)),
                _mm_mul_ps(_mm_shuffle_ps(*a3, *a3, 210), _mm_shuffle_ps(*a4, *a4, 201))
            ),
            v14
        );
        if ((XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 85), 0) + XFNV_M128_F32(v15, 0))) < 0.0f) {
            *a5 = _mm_xor_ps(v14, hkQuadRealNegateMask);
        }
        return a1;
    }

    __m128 v5 = _mm_undefined_ps();
    __m128 v6 = _mm_undefined_ps();
    __m128 v7 = _mm_undefined_ps();
    __m128 v8 = _mm_undefined_ps();
    XFNV_M128_F32(v7, 0) = 0.0f;
    XFNV_M128_F32(v8, 0) = 0.0f;
    XFNV_M128_F32(v5, 0) = 1.0f;
    XFNV_M128_F32(v6, 0) = 0.0f;
    __m128 v9 = _mm_unpacklo_ps(v6, v8);
    __m128 v10 = _mm_unpacklo_ps(_mm_unpacklo_ps(v5, v7), v9);
    *a3 = v10;
    XFNV_M128_F32(v10, 0) = 0.0f;
    XFNV_M128_F32(v9, 0) = 1.0f;
    __m128 v12 = _mm_unpacklo_ps(v9, v8);
    __m128 v13 = _mm_unpacklo_ps(_mm_unpacklo_ps(v10, v7), v12);
    *a4 = v13;
    XFNV_M128_F32(v13, 0) = 0.0f;
    XFNV_M128_F32(v12, 0) = 0.0f;
    *a5 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v13, _mm_set_ss(std::bit_cast<float>(0x3F800000u))),
        _mm_unpacklo_ps(v12, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    return a1;
}

// Whether the largest of |x|, |y| and |z| of apColumn is within afTolerance of 1. Returns 1 or 0.
int32_t isAxisAligned(float afTolerance, const hkVector4* apColumn) {
    const float fZ = std::fabs((*apColumn)(2));
    const float fY = std::fabs((*apColumn)(1));
    const float fX = std::fabs((*apColumn)(0));
    float fMax = fX;
    if (!(fY < fX)) {
        fMax = fY;
    }
    if (fMax < fZ) {
        fMax = fZ;
    }
    const float fOffset = fMax - 1.0;
    const float fDistance = std::fabs(fOffset);
    return afTolerance > fDistance ? 1 : 0;
}

// The gradient of the box volume measured over x, y and z of the two directions apDirections, by forward differences of
// step afStep on a copy of them, negated into apGradient[0] and [1]; scaled to length 1 when its squared length is
// above 1. Each probed component is put back from apDirections.
void calcVolumeGradient(
    const float* apVertices,
    int32_t aiNumVertices,
    int32_t aiStriding,
    float afStep,
    hkVector4* apGradient,
    const hkVector4* apDirections
) {
    hkVector4 kProbe[2];
    kProbe[0].m_quad = apDirections[0].m_quad;
    kProbe[1].m_quad = apDirections[1].m_quad;
    const float fBase = calcObbVolume(aiStriding, apVertices, aiNumVertices, apDirections);
    const float fInvStep = 1.0 / afStep;
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 3; ++j) {
            kProbe[i](j) = afStep + kProbe[i](j);
            const float fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
            apGradient[i](j) = (fVolume - fBase) * fInvStep;
            kProbe[i](j) = apDirections[i](j);
        }
        apGradient[i].m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), apGradient[i].m_quad);
    }
    const __m128 kGradient0 = apGradient[0].m_quad;
    const __m128 kSq0 = _mm_mul_ps(kGradient0, kGradient0);
    const __m128 kSq1 = _mm_mul_ps(apGradient[1].m_quad, apGradient[1].m_quad);
    const __m128 kLength0 =
        _mm_add_ss(_mm_shuffle_ps(kSq0, kSq0, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq0, kSq0, 0x55), kSq0));
    const __m128 kLength1 =
        _mm_add_ss(_mm_shuffle_ps(kSq1, kSq1, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq1, kSq1, 0x55), kSq1));
    const float fLengthSquared = _mm_cvtss_f32(_mm_add_ss(kLength0, kLength1));
    if (fLengthSquared > 1.0) {
        const float fLength = std::sqrt(fLengthSquared);
        const float fInvLength = 1.0 / fLength;
        apGradient[0].m_quad = _mm_mul_ps(_mm_set1_ps(fInvLength), kGradient0);
        apGradient[1].m_quad = _mm_mul_ps(_mm_set1_ps(fInvLength), apGradient[1].m_quad);
    }
}

// The extents of the vertices along the three axes apAxes: aiNumVertices / 3 runs of three vertices (the remainder is
// skipped), each vertex aiStriding bytes after the last. Writes half the extents to apHalfExtents and the box's centre,
// in world space, to apCenter, and returns apCenter.
hkVector4* calcObbExtents(
    const float* apVertices,
    int32_t aiNumVertices,
    int32_t aiStriding,
    hkVector4* apHalfExtents,
    hkVector4* apCenter,
    const hkVector4* apAxes
) {
    hkVector4 kMin;
    hkVector4 kMax;
    kMin.m_quad = _mm_set1_ps(1.3611281e+38f);
    kMax.m_quad = _mm_set1_ps(-1.3611281e+38f);
    const float* pVertex = apVertices;
    for (int32_t iRun = aiNumVertices / 3; iRun > 0; --iRun) {
        for (int32_t k = 0; k < 3; ++k) {
            const __m128 kVertex = _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_load_ss(&pVertex[0]), _mm_load_ss(&pVertex[2])),
                _mm_unpacklo_ps(_mm_load_ss(&pVertex[1]), _mm_setzero_ps())
            );
            // The vertices are raw strided memory (the SDK's hkStridedVertices layout): aiStriding bytes apart.
            pVertex = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVertex) + aiStriding);
            for (int32_t j = 0; j < 3; ++j) {
                const __m128 kProduct = _mm_mul_ps(apAxes[j].m_quad, kVertex);
                const float fDistance = _mm_cvtss_f32(_mm_add_ss(
                    _mm_shuffle_ps(kProduct, kProduct, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)
                ));
                if (kMin(j) > fDistance) {
                    kMin(j) = fDistance;
                }
                if (kMax(j) < fDistance) {
                    kMax(j) = fDistance;
                }
            }
        }
    }
    const __m128 kExtents = _mm_sub_ps(kMax.m_quad, kMin.m_quad);
    apHalfExtents->m_quad = kExtents;
    hkVector4 kMid;
    kMid.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(kMax.m_quad, kMin.m_quad));
    apHalfExtents->m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), kExtents);
    __m128 kCenter = _mm_mul_ps(_mm_shuffle_ps(kMid.m_quad, kMid.m_quad, 0), apAxes[0].m_quad);
    apCenter->m_quad = kCenter;
    kCenter = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(kMid(1)), apAxes[1].m_quad), kCenter);
    apCenter->m_quad = kCenter;
    kCenter = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(kMid(2)), apAxes[2].m_quad), kCenter);
    apCenter->m_quad = kCenter;
    return apCenter;
}

// The volume measure of the box around the vertices whose axes are built from apDirections: the first direction
// normalised, the second made orthogonal to the first (unnormalised) one and normalised, and their cross product
// normalised (each with one Newton step on the reciprocal square root, 0 for a zero vector). Returns the product of the
// half extents, rounded to a float.
float calcObbVolume(int32_t aiStriding, const float* apVertices, int32_t aiNumVertices, const hkVector4* apDirections) {
    // kHalfExtents starts out zeroed, so the callee that fills in the half extents finds it zeroed, as the game does.
    hkVector4 kHalfExtents;
    kHalfExtents.m_quad = _mm_setzero_ps();
    hkVector4 kCenter;
    hkVector4 kAxes[3];
    const __m128 kZero = kHalfExtents.m_quad;
    const __m128 kHalf = _mm_set_ss(0.5f);
    const __m128 kThree = _mm_set_ss(3.0f);
    const __m128 kU = apDirections[0].m_quad;
    __m128 kV = apDirections[1].m_quad;
    const __m128 kUSq = _mm_mul_ps(kU, kU);
    const __m128 kULength2 =
        _mm_add_ss(_mm_shuffle_ps(kUSq, kUSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kUSq, kUSq, 0x55), kUSq));
    const __m128 kUEstimate = _mm_rsqrt_ss(kULength2);
    const __m128 kUInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kULength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kUEstimate), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kULength2, kUEstimate), kUEstimate))
        )
    );
    kAxes[0].m_quad = _mm_mul_ps(_mm_shuffle_ps(kUInv, kUInv, 0), kU);

    const __m128 kUV = _mm_mul_ps(kV, kU);
    const float fDot =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kUV, kUV, 0xAA), _mm_add_ss(_mm_shuffle_ps(kUV, kUV, 0x55), kUV)));
    kV = _mm_sub_ps(kV, _mm_mul_ps(_mm_set1_ps(fDot), kU));
    const __m128 kVSq = _mm_mul_ps(kV, kV);
    const __m128 kVLength2 =
        _mm_add_ss(_mm_shuffle_ps(kVSq, kVSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kVSq, kVSq, 0x55), kVSq));
    const __m128 kVEstimate = _mm_rsqrt_ss(kVLength2);
    const __m128 kVInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kVLength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kVEstimate), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kVLength2, kVEstimate), kVEstimate))
        )
    );
    kAxes[1].m_quad = _mm_mul_ps(_mm_shuffle_ps(kVInv, kVInv, 0), kV);

    // The cross product of the two directions as given.
    const __m128 kSecond = apDirections[1].m_quad;
    const __m128 kW = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kSecond, kSecond, 0xD2), _mm_shuffle_ps(kU, kU, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kSecond, kSecond, 0xC9), _mm_shuffle_ps(kU, kU, 0xD2))
    );
    const __m128 kWSq = _mm_mul_ps(kW, kW);
    const __m128 kWLength2 =
        _mm_add_ss(_mm_shuffle_ps(kWSq, kWSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kWSq, kWSq, 0x55), kWSq));
    const __m128 kWEstimate = _mm_rsqrt_ss(kWLength2);
    const __m128 kWInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kWLength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kWEstimate), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kWLength2, kWEstimate), kWEstimate))
        )
    );
    kAxes[2].m_quad = _mm_mul_ps(_mm_shuffle_ps(kWInv, kWInv, 0), kW);

    calcObbExtents(apVertices, aiNumVertices, aiStriding, &kHalfExtents, &kCenter, kAxes);
    const float fVolume = kHalfExtents(1) * kHalfExtents(0) * kHalfExtents(2);
    return fVolume;
}

// A line search for the step along apDirection (two vectors, the gradient gives) from apStart that lowers the volume
// measure the most: from afStep it grows the step by 1.2 while the volume drops, or halves it until the volume is below
// both the start's and the first probe's, then narrows the bracket by golden-section search over aiMaxIterations steps.
// Returns the step, or -1 when no bracket is found. Each probe is built in a local.
float lineSearchMinVolume(
    const float* apVertices,
    int32_t aiNumVertices,
    float afStep,
    const hkVector4* apDirection,
    int32_t aiMaxIterations,
    int32_t aiStriding,
    const hkVector4* apStart
) {
    hkVector4 kProbe[3];
    float fBest = 0.0f;
    float fOther = 0.0f;
    float fLowBound = 0.0f;
    float fLow;
    float fHigh;
    const float fStart = calcObbVolume(aiStriding, apVertices, aiNumVertices, apStart);
    stepAndOrthonormalizeAxes(kProbe, apStart, apDirection, afStep);
    const float fFirst = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
    if (fStart > fFirst) {
        fHigh = afStep;
        fLow = afStep;
        fBest = fFirst;
        for (int32_t i = 0; i < aiMaxIterations; ++i) {
            fHigh = fHigh * 1.2000000476837158;
            stepAndOrthonormalizeAxes(kProbe, apStart, apDirection, fHigh);
            const float fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
            if (fVolume > fBest) {
                fOther = fVolume;
                break;
            }
            fLow = fHigh;
            fBest = fVolume;
            if (i == aiMaxIterations - 1) {
                return fLow;
            }
        }
    }
    else {
        fHigh = afStep;
        fLow = afStep;
        fOther = fFirst;
        for (int32_t i = 0; i < aiMaxIterations; ++i) {
            fLow = (fLow + 0.0) * 0.5;
            stepAndOrthonormalizeAxes(kProbe, apStart, apDirection, fLow);
            fBest = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
            if (fStart > fBest && fFirst > fBest) {
                break;
            }
            if (i == aiMaxIterations - 1) {
                return -1.0f;
            }
        }
    }
    if (fLow < 0.0 || fHigh < fLow || fStart < fBest || fOther < fBest) {
        return -1.0f;
    }
    const float fSqrt5 = std::sqrt(5.0);
    const float fGolden = (3.0 - fSqrt5) * 0.5;
    for (int32_t i = aiMaxIterations; i > 0; --i) {
        const float fLeft = fLow - fLowBound;
        const float fRight = fHigh - fLow;
        if (fRight < fLeft) {
            const float fProbe = fLow - fGolden * fLeft;
            stepAndOrthonormalizeAxes(kProbe, apStart, apDirection, fProbe);
            const float fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
            if (fBest > fVolume) {
                fHigh = fLow;
                fLow = fProbe;
                fBest = fVolume;
            }
            else {
                fLowBound = fProbe;
            }
        }
        else {
            const float fProbe = fGolden * fRight + fLow;
            stepAndOrthonormalizeAxes(kProbe, apStart, apDirection, fProbe);
            const float fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, kProbe);
            if (fBest > fVolume) {
                fLowBound = fLow;
                fLow = fProbe;
                fBest = fVolume;
            }
            else {
                fHigh = fProbe;
            }
        }
    }
    return fLow;
}

// The sphere through two points, grown by afRadiusEpsilon: the centre halfway between them, the radius half their
// distance. Returns apResult.
hkSphere* calcSphereFrom2Points(const hkVector4* apA, const hkVector4* apB, float afRadiusEpsilon, hkSphere* apResult) {
    const __m128 kA = apA->m_quad;
    const __m128 kHalf = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_sub_ps(apB->m_quad, kA));
    const __m128 kSq = _mm_mul_ps(kHalf, kHalf);
    const float fLength = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)))
    );
    apResult->m_pos.m_quad = _mm_add_ps(kA, kHalf);
    apResult->m_pos(3) = fLength + afRadiusEpsilon;
    return apResult;
}

// The circumsphere of three points, grown by afRadiusEpsilon: with the edges AB and AC and the normal N = AB x AC, the
// centre is A + ((N x AB) |AC|^2 + (AC x N) |AB|^2) / (2 |N|^2). Returns apResult.
hkSphere* calcSphereFrom3Points(
    const hkVector4* apA,
    const hkVector4* apB,
    const hkVector4* apC,
    float afRadiusEpsilon,
    hkSphere* apResult
) {
    const __m128 kA = apA->m_quad;
    const __m128 kAB = _mm_sub_ps(apB->m_quad, kA);
    const __m128 kAC = _mm_sub_ps(apC->m_quad, kA);
    const __m128 kAByzx = _mm_shuffle_ps(kAB, kAB, 0xC9);
    const __m128 kABzxy = _mm_shuffle_ps(kAB, kAB, 0xD2);
    const __m128 kACyzx = _mm_shuffle_ps(kAC, kAC, 0xC9);
    const __m128 kACzxy = _mm_shuffle_ps(kAC, kAC, 0xD2);
    const __m128 kNormal = _mm_sub_ps(_mm_mul_ps(kAByzx, kACzxy), _mm_mul_ps(kABzxy, kACyzx));
    const __m128 kNormalSq = _mm_mul_ps(kNormal, kNormal);
    const float fNormalLength2 = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(kNormalSq, kNormalSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kNormalSq, kNormalSq, 0x55), kNormalSq)
    ));
    const float fScale = 0.5 / fNormalLength2;
    const __m128 kACSq = _mm_mul_ps(kAC, kAC);
    const __m128 kABSq = _mm_mul_ps(kAB, kAB);
    const __m128 kACLength2 =
        _mm_add_ss(_mm_shuffle_ps(kACSq, kACSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kACSq, kACSq, 0x55), kACSq));
    const __m128 kABLength2 =
        _mm_add_ss(_mm_shuffle_ps(kABSq, kABSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kABSq, kABSq, 0x55), kABSq));
    const __m128 kNyzx = _mm_shuffle_ps(kNormal, kNormal, 0xC9);
    const __m128 kNzxy = _mm_shuffle_ps(kNormal, kNormal, 0xD2);
    __m128 kOffset = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(kNyzx, kABzxy), _mm_mul_ps(kNzxy, kAByzx)), _mm_shuffle_ps(kACLength2, kACLength2, 0)
    );
    kOffset = _mm_add_ps(kOffset, _mm_setzero_ps());
    kOffset = _mm_add_ps(
        kOffset,
        _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(kNzxy, kACyzx), _mm_mul_ps(kNyzx, kACzxy)), _mm_shuffle_ps(kABLength2, kABLength2, 0)
        )
    );
    kOffset = _mm_mul_ps(kOffset, _mm_set1_ps(fScale));
    const __m128 kOffsetSq = _mm_mul_ps(kOffset, kOffset);
    const float fRadius = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
        _mm_shuffle_ps(kOffsetSq, kOffsetSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kOffsetSq, kOffsetSq, 0x55), kOffsetSq)
    )));
    apResult->m_pos.m_quad = _mm_add_ps(kA, kOffset);
    apResult->m_pos(3) = fRadius + afRadiusEpsilon;
    return apResult;
}

// The circumsphere of four points, grown by afRadiusEpsilon: with the edges AB, AC and AD, the centre is A + ((AB x AC)
// |AD|^2 + (AD x AB) |AC|^2 + (AC x AD) |AB|^2) / (2 det), the determinant of the edges rounded to a float, as the
// division. Returns apResult.
hkSphere* calcSphereFrom4Points(
    const hkVector4* apA,
    const hkVector4* apB,
    const hkVector4* apC,
    const hkVector4* apD,
    float afRadiusEpsilon,
    hkSphere* apResult
) {
    const __m128 kA = apA->m_quad;
    const __m128 kAB = _mm_sub_ps(apB->m_quad, kA);
    const __m128 kAC = _mm_sub_ps(apC->m_quad, kA);
    const __m128 kAD = _mm_sub_ps(apD->m_quad, kA);
    const float fDeterminant =
        ((XFNV_M128_F32(kAD, 2) * XFNV_M128_F32(kAC, 1) - XFNV_M128_F32(kAD, 1) * XFNV_M128_F32(kAC, 2)) *
             XFNV_M128_F32(kAB, 0) -
         (XFNV_M128_F32(kAD, 2) * XFNV_M128_F32(kAC, 0) - XFNV_M128_F32(kAC, 2) * XFNV_M128_F32(kAD, 0)) *
             XFNV_M128_F32(kAB, 1)) +
        (XFNV_M128_F32(kAD, 1) * XFNV_M128_F32(kAC, 0) - XFNV_M128_F32(kAC, 1) * XFNV_M128_F32(kAD, 0)) *
            XFNV_M128_F32(kAB, 2);
    const float fScale = 0.5 / fDeterminant;
    const __m128 kADSq = _mm_mul_ps(kAD, kAD);
    const __m128 kACSq = _mm_mul_ps(kAC, kAC);
    const __m128 kABSq = _mm_mul_ps(kAB, kAB);
    const __m128 kADLength2 =
        _mm_add_ss(_mm_shuffle_ps(kADSq, kADSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kADSq, kADSq, 0x55), kADSq));
    const __m128 kACLength2 =
        _mm_add_ss(_mm_shuffle_ps(kACSq, kACSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kACSq, kACSq, 0x55), kACSq));
    const __m128 kABLength2 =
        _mm_add_ss(_mm_shuffle_ps(kABSq, kABSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kABSq, kABSq, 0x55), kABSq));
    const __m128 kAByzx = _mm_shuffle_ps(kAB, kAB, 0xC9);
    const __m128 kABzxy = _mm_shuffle_ps(kAB, kAB, 0xD2);
    const __m128 kACyzx = _mm_shuffle_ps(kAC, kAC, 0xC9);
    const __m128 kACzxy = _mm_shuffle_ps(kAC, kAC, 0xD2);
    const __m128 kADyzx = _mm_shuffle_ps(kAD, kAD, 0xC9);
    const __m128 kADzxy = _mm_shuffle_ps(kAD, kAD, 0xD2);
    __m128 kOffset = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(kAByzx, kACzxy), _mm_mul_ps(kABzxy, kACyzx)), _mm_shuffle_ps(kADLength2, kADLength2, 0)
    );
    kOffset = _mm_add_ps(kOffset, _mm_setzero_ps());
    kOffset = _mm_add_ps(
        kOffset,
        _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(kADyzx, kABzxy), _mm_mul_ps(kADzxy, kAByzx)),
            _mm_shuffle_ps(kACLength2, kACLength2, 0)
        )
    );
    kOffset = _mm_add_ps(
        kOffset,
        _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(kADzxy, kACyzx), _mm_mul_ps(kADyzx, kACzxy)),
            _mm_shuffle_ps(kABLength2, kABLength2, 0)
        )
    );
    kOffset = _mm_mul_ps(kOffset, _mm_set1_ps(fScale));
    const __m128 kOffsetSq = _mm_mul_ps(kOffset, kOffset);
    const float fRadius = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
        _mm_shuffle_ps(kOffsetSq, kOffsetSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kOffsetSq, kOffsetSq, 0x55), kOffsetSq)
    )));
    apResult->m_pos.m_quad = _mm_add_ps(kA, kOffset);
    apResult->m_pos(3) = fRadius + afRadiusEpsilon;
    return apResult;
}

int32_t createVerticesFromPlaneEquations(
    const hkArray<hkVector4, hkContainerHeapAllocator>& points,
    hkArray<hkVector4, hkContainerHeapAllocator>& hullVertices
) {
    return hkGeometryUtility::calcConvexHullVertices(points, hullVertices);
}

bool isApproxEqual(const float* apA, float afTolerance, const float* apB) {
    float v4 = *apB - *apA;
    float v5 = std::fabs((double)v4);
    return afTolerance > static_cast<double>(v5);
}

// apOut[i] = apStart[i] + afStep * apDirection[i] for the first two vectors (apOut may be apStart), then the first
// normalised, the second made orthogonal to it and normalised, and the third their cross product, normalised.
void stepAndOrthonormalizeAxes(hkVector4* apOut, const hkVector4* apStart, const hkVector4* apDirection, float afStep) {
    for (int32_t i = 0; i < 2; ++i) {
        apOut[i].m_quad = apStart[i].m_quad;
        apOut[i].m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(afStep), apDirection[i].m_quad), apOut[i].m_quad);
    }
    const __m128 kZero = _mm_setzero_ps();
    const __m128 kHalf = _mm_set_ss(0.5f);
    const __m128 kThree = _mm_set_ss(3.0f);

    const __m128 kFirst = apOut[0].m_quad;
    const __m128 kFirstSq = _mm_mul_ps(kFirst, kFirst);
    const __m128 kFirstLength2 = _mm_add_ss(
        _mm_shuffle_ps(kFirstSq, kFirstSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kFirstSq, kFirstSq, 0x55), kFirstSq)
    );
    const __m128 kFirstEstimate = _mm_rsqrt_ss(kFirstLength2);
    const __m128 kFirstInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kFirstLength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kFirstEstimate),
            _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kFirstLength2, kFirstEstimate), kFirstEstimate))
        )
    );
    const __m128 kAxis0 = _mm_mul_ps(_mm_shuffle_ps(kFirstInv, kFirstInv, 0), apOut[0].m_quad);
    const __m128 kProduct = _mm_mul_ps(kAxis0, apOut[1].m_quad);
    apOut[0].m_quad = kAxis0;
    const float fDot = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(kProduct, kProduct, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)
    ));
    apOut[1].m_quad = _mm_sub_ps(apOut[1].m_quad, _mm_mul_ps(_mm_set1_ps(fDot), kAxis0));

    const __m128 kSecond = apOut[1].m_quad;
    const __m128 kSecondSq = _mm_mul_ps(kSecond, kSecond);
    const __m128 kSecondLength2 = _mm_add_ss(
        _mm_shuffle_ps(kSecondSq, kSecondSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSecondSq, kSecondSq, 0x55), kSecondSq)
    );
    const __m128 kSecondEstimate = _mm_rsqrt_ss(kSecondLength2);
    const __m128 kSecondInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kSecondLength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kSecondEstimate),
            _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kSecondLength2, kSecondEstimate), kSecondEstimate))
        )
    );
    apOut[1].m_quad = _mm_mul_ps(_mm_shuffle_ps(kSecondInv, kSecondInv, 0), kSecond);

    const __m128 kAxis1 = apOut[1].m_quad;
    const __m128 kAxis0Again = apOut[0].m_quad;
    apOut[2].m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kAxis1, kAxis1, 0xD2), _mm_shuffle_ps(kAxis0Again, kAxis0Again, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kAxis1, kAxis1, 0xC9), _mm_shuffle_ps(kAxis0Again, kAxis0Again, 0xD2))
    );
    const __m128 kThird = apOut[2].m_quad;
    const __m128 kThirdSq = _mm_mul_ps(kThird, kThird);
    const __m128 kThirdLength2 = _mm_add_ss(
        _mm_shuffle_ps(kThirdSq, kThirdSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kThirdSq, kThirdSq, 0x55), kThirdSq)
    );
    const __m128 kThirdEstimate = _mm_rsqrt_ss(kThirdLength2);
    const __m128 kThirdInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kThirdLength2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kThirdEstimate),
            _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kThirdLength2, kThirdEstimate), kThirdEstimate))
        )
    );
    apOut[2].m_quad = _mm_mul_ps(_mm_shuffle_ps(kThirdInv, kThirdInv, 0), kThird);
}

// Minimises the volume measure over the box axes: apOut starts as apStart (three vectors) and moves along the gradient
// by the step the line search finds, up to aiMaxIterations times. Stops when the search fails, when the volume rises
// (apOut is put back and the previous volume returned) or when the step falls below 1.1920929e-06. Returns the last
// volume.
float optimizeOrientedBoxAxes(
    int32_t aiStriding,
    const float* apVertices,
    int32_t aiNumVertices,
    const hkVector4* apStart,
    int32_t aiMaxIterations,
    hkVector4* apOut
) {
    float fStep = 0.001f;
    float fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, apStart);
    apOut[0].m_quad = apStart[0].m_quad;
    apOut[1].m_quad = apStart[1].m_quad;
    apOut[2].m_quad = apStart[2].m_quad;
    for (int32_t i = 0; i < aiMaxIterations; ++i) {
        hkVector4 kGradient[2];
        hkVector4 kSaved[3];
        kSaved[0].m_quad = apOut[0].m_quad;
        kSaved[1].m_quad = apOut[1].m_quad;
        kSaved[2].m_quad = apOut[2].m_quad;
        calcVolumeGradient(apVertices, aiNumVertices, aiStriding, 0.0001f, kGradient, apOut);
        fStep = lineSearchMinVolume(apVertices, aiNumVertices, fStep, kGradient, aiMaxIterations, aiStriding, apOut);
        if (fStep < 0.0) {
            return fVolume;
        }
        stepAndOrthonormalizeAxes(apOut, apOut, kGradient, fStep);
        const float fPrevious = fVolume;
        fVolume = calcObbVolume(aiStriding, apVertices, aiNumVertices, apOut);
        if (fVolume > fPrevious) {
            apOut[0].m_quad = kSaved[0].m_quad;
            apOut[1].m_quad = kSaved[1].m_quad;
            apOut[2].m_quad = kSaved[2].m_quad;
            return fPrevious;
        }
        if (fStep < 1.1920929e-06f) {
            return fVolume;
        }
    }
    return fVolume;
}

// The area-weighted covariance of the triangles of numVertices strided vertices (three per triangle; a file-local
// helper of hkGeometryUtility::calcObb): each triangle's centroid goes to a scratch array, its area / 12 * (9 c c^T +
// p0 p0^T + p1 p1^T + p2 p2^T) into covarianceOut's columns, and the area-weighted mean centroid into centroidOut, the
// mean's outer product times the total area taken off. A triangle of no area adds its three outer products unweighted;
// no area at all gives the identity and a zero centroid. With jitter, the first point of each edge pair moves by 0.01
// times a pseudo-random vector (seeded with numVertices) before the cross product. The 9.0, 0.5 and 12.0 are doubles;
// the area, the total area and 1 / total are stored as floats.
void calcCovarianceOfTriangles(
    const float* vertices,
    int32_t numVertices,
    int32_t striding,
    hkMatrix3& covarianceOut,
    hkVector4& centroidOut,
    bool jitter
) {
    float totalArea = 0.0f;
    hkVector4 zero;
    zero.setZero4();
    hkVector4 weightedSum;
    weightedSum.setZero4();
    hkArray<hkVector4> centroids;
    const int32_t numTriangles = numVertices / 3;
    hkVector4* pCentroid = centroids.expandBy(numTriangles);
    covarianceOut.m_col0.m_quad = zero.m_quad;
    covarianceOut.m_col1.m_quad = zero.m_quad;
    covarianceOut.m_col2.m_quad = zero.m_quad;
    bool identity = true;
    if (numTriangles > 0) {
        for (int32_t remaining = numTriangles; remaining != 0; --remaining, ++pCentroid) {
            hkVector4 p0;
            p0.set(vertices[0], vertices[1], vertices[2], 0.0f);
            // The vertices are untyped strided memory: three floats every striding bytes.
            vertices = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertices) + striding);
            hkVector4 p1;
            p1.set(vertices[0], vertices[1], vertices[2], 0.0f);
            vertices = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertices) + striding);
            hkVector4 p2;
            p2.set(vertices[0], vertices[1], vertices[2], 0.0f);
            vertices = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertices) + striding);
            hkVector4 edge0;
            if (jitter) {
                // EXE-INLINED none: hkPseudoRandomGenerator::hkPseudoRandomGenerator
                hkPseudoRandomGenerator random;
                random.m_seed = static_cast<uint32_t>(numVertices);
                random.m_current = static_cast<uint32_t>(numVertices);
                hkVector4 offset;
                random.getRandomVector01(offset);
                edge0.m_quad =
                    _mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.01f), offset.m_quad), p0.m_quad), p1.m_quad);
            }
            else {
                edge0.setSub4(p0, p1);
            }
            hkVector4 edge1;
            edge1.setSub4(p0, p2);
            hkVector4 normal;
            normal.setCross(edge0, edge1);
            const float length = normal.length3();
            const float area = length * 0.5;
            hkVector4 sum;
            sum.setAdd4(p1, p0);
            if (area > 0.0f) {
                totalArea = area + totalArea;
                hkVector4 centroid;
                centroid.m_quad = _mm_mul_ps(_mm_add_ps(sum.m_quad, p2.m_quad), _mm_set1_ps(0.33333334f));
                pCentroid->m_quad = centroid.m_quad;
                weightedSum.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(area), centroid.m_quad), weightedSum.m_quad);
                for (int32_t i = 0; i < 3; ++i) {
                    const hkVector4& c = *pCentroid;
                    covarianceOut.m_col0(i) =
                        (c(i) * 9.0 * c(0) + p0(0) * p0(i) + p1(0) * p1(i) + p2(i) * p2(0)) * area +
                        covarianceOut.m_col0(i);
                    covarianceOut.m_col1(i) =
                        (c(i) * 9.0 * c(1) + p0(1) * p0(i) + p1(1) * p1(i) + p2(i) * p2(1)) * area +
                        covarianceOut.m_col1(i);
                    covarianceOut.m_col2(i) =
                        (c(i) * 9.0 * c(2) + p0(2) * p0(i) + p1(2) * p1(i) + p2(i) * p2(2)) * area +
                        covarianceOut.m_col2(i);
                }
            }
            else {
                pCentroid->m_quad = sum.m_quad;
                pCentroid->m_quad = _mm_add_ps(p2.m_quad, pCentroid->m_quad);
                pCentroid->m_quad = _mm_mul_ps(_mm_set1_ps(0.33333334f), pCentroid->m_quad);
                for (int32_t i = 0; i < 3; ++i) {
                    covarianceOut.m_col0(i) = p1(i) * p1(0) + p0(0) * p0(i) + p2(i) * p2(0) + covarianceOut.m_col0(i);
                    covarianceOut.m_col1(i) = p1(i) * p1(1) + p0(1) * p0(i) + p2(i) * p2(1) + covarianceOut.m_col1(i);
                    covarianceOut.m_col2(i) = p1(i) * p1(2) + p0(2) * p0(i) + p2(i) * p2(2) + covarianceOut.m_col2(i);
                }
            }
        }
        // The test takes the weighted path for a positive or unordered total.
        identity = totalArea <= 0.0f;
    }
    if (identity) {
        centroidOut.m_quad = zero.m_quad;
        covarianceOut.m_col0.m_quad = zero.m_quad;
        covarianceOut.m_col1.m_quad = zero.m_quad;
        covarianceOut.m_col2.m_quad = zero.m_quad;
        covarianceOut.m_col0(0) = 1.0f;
        covarianceOut.m_col1(1) = 1.0f;
        covarianceOut.m_col2(2) = 1.0f;
    }
    else {
        const float inverseArea = 1.0 / totalArea;
        weightedSum.m_quad = _mm_mul_ps(_mm_set1_ps(inverseArea), weightedSum.m_quad);
        for (int32_t i = 0; i < 3; ++i) {
            covarianceOut.m_col0(i) = covarianceOut.m_col0(i) / 12.0 - weightedSum(0) * weightedSum(i) * totalArea;
            covarianceOut.m_col1(i) = covarianceOut.m_col1(i) / 12.0 - weightedSum(1) * weightedSum(i) * totalArea;
            covarianceOut.m_col2(i) = covarianceOut.m_col2(i) / 12.0 - weightedSum(2) * weightedSum(i) * totalArea;
        }
        centroidOut.m_quad = weightedSum.m_quad;
    }
}

// Whether the vertices are the 8 corners of a box (the file-local helper of hkGeometryUtility::calcObb): the vertices
// within tolerance of the surface of their minimum bounding sphere must be exactly 8. The two shortest edges from the
// first corner (nearly orthogonal: their dot at most 1e-5) and their cross product give a rotation; the corners,
// rotated by its inverse, must each lie within 0.001 of their bounding box's faces on every axis. halfExtentsOut and
// transformOut get that box; when each column of the inverted rotation is within tolerance of an axis, the box is
// turned axis-aligned (half extents rotated and made positive, an identity rotation). Last, every vertex must lie
// within 1.001 times the half extents in the box's frame.
bool getIsBoxGeometry(
    const hkArray<hkVector4>& vertices,
    hkVector4& halfExtentsOut,
    hkTransform& transformOut,
    float tolerance
) {
    hkStridedVertices stridedVerts;
    stridedVerts.m_vertices = &(*vertices.m_data)(0);
    stridedVerts.m_numVertices = vertices.m_size;
    stridedVerts.m_striding = static_cast<int32_t>(sizeof(hkVector4));
    const hkSphere sphere = hkGeometryUtility::createMinBoundingSphere(stridedVerts, 0.00001f);
    hkArray<hkVector4> onSphere;
    hkArray<hkVector4> corners(8);
    const int32_t numVertices = vertices.m_size;
    for (int32_t i = 0; i < numVertices; ++i) {
        const hkVector4& vertex = vertices.m_data[i];
        hkVector4 offset;
        offset.setSub4(vertex, sphere.m_pos);
        const float distance = offset.length3();
        float error = distance - sphere.m_pos(3);
        error = std::fabs(error);
        if (tolerance > error) {
            onSphere.pushBack(vertex);
        }
    }
    bool isBox = false;
    if (onSphere.m_size == 8) {
        corners[0].m_quad = onSphere.m_data[0].m_quad;
        corners[1].m_quad = onSphere.m_data[1].m_quad;
        corners[2].m_quad = onSphere.m_data[2].m_quad;
        corners[3].m_quad = onSphere.m_data[3].m_quad;
        corners[4].m_quad = onSphere.m_data[4].m_quad;
        corners[5].m_quad = onSphere.m_data[5].m_quad;
        corners[6].m_quad = onSphere.m_data[6].m_quad;
        corners[7].m_quad = onSphere.m_data[7].m_quad;
        // The two shortest edges from the first corner.
        hkVector4 edge0;
        edge0.setSub4(corners[0], corners[1]);
        hkVector4 edge1;
        edge1.setSub4(corners[0], corners[2]);
        for (int32_t k = 3; k < 8; ++k) {
            hkVector4 edge;
            edge.setSub4(corners[0], corners[k]);
            const float edgeLength2 = edge.lengthSquared3();
            const float edge0Length2 = edge0.lengthSquared3();
            if (edge0Length2 > edgeLength2) {
                const float shortestLength2 = edge0.lengthSquared3();
                const float edge1Length2 = edge1.lengthSquared3();
                if (edge1Length2 > shortestLength2) {
                    edge1.m_quad = edge0.m_quad;
                }
                edge0.m_quad = edge.m_quad;
            }
            else {
                const float newLength2 = edge.lengthSquared3();
                const float edge1Length2 = edge1.lengthSquared3();
                if (edge1Length2 > newLength2) {
                    edge1.m_quad = edge.m_quad;
                }
            }
        }
        const float edgeDot = edge1.dot3(edge0);
        if (0.00001f < edgeDot) {
            goto done;
        }
        {
            hkVector4 edge2;
            edge2.setCross(edge0, edge1);
            edge0.normalize3();
            edge1.normalize3();
            edge2.normalize3();
            hkRotation rotation;
            rotation.m_col0.m_quad = edge0.m_quad;
            rotation.m_col1.m_quad = edge1.m_quad;
            rotation.m_col2.m_quad = edge2.m_quad;
            rotation.invert(0.01f);
            for (int32_t k = 0; k < 8; ++k) {
                corners[k].setRotatedDir(rotation, corners[k]);
            }
            float minX = corners[0](0);
            float maxX = corners[0](0);
            float minY = corners[0](1);
            float maxY = corners[0](1);
            float minZ = corners[0](2);
            float maxZ = corners[0](2);
            for (int32_t k = 1; k < 8; ++k) {
                if (corners[k](0) < minX) {
                    minX = corners[k](0);
                }
                else if (corners[k](0) > maxX) {
                    maxX = corners[k](0);
                }
                if (corners[k](1) < minY) {
                    minY = corners[k](1);
                }
                else if (corners[k](1) > maxY) {
                    maxY = corners[k](1);
                }
                if (corners[k](2) < minZ) {
                    minZ = corners[k](2);
                }
                else if (corners[k](2) > maxZ) {
                    maxZ = corners[k](2);
                }
            }
            for (int32_t k = 0; k < 8; ++k) {
                if (!isApproxEqual(&minX, 0.001f, &corners[k](0)) && !isApproxEqual(&maxX, 0.001f, &corners[k](0))) {
                    goto done;
                }
                if (!isApproxEqual(&minY, 0.001f, &corners[k](1)) && !isApproxEqual(&maxY, 0.001f, &corners[k](1))) {
                    goto done;
                }
                if (!isApproxEqual(&minZ, 0.001f, &corners[k](2)) && !isApproxEqual(&maxZ, 0.001f, &corners[k](2))) {
                    goto done;
                }
            }
            halfExtentsOut(0) = maxX - minX;
            halfExtentsOut(1) = maxY - minY;
            halfExtentsOut(2) = maxZ - minZ;
            halfExtentsOut.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), halfExtentsOut.m_quad);
            const float centerX = (minX + maxX) * 0.5;
            const float centerY = (minY + maxY) * 0.5;
            const float centerZ = (minZ + maxZ) * 0.5;
            hkVector4 center;
            center.set(centerX, centerY, centerZ, 0.0f);
            hkTransform boxFrame;
            boxFrame.m_rotation.m_col0.m_quad = rotation.m_col0.m_quad;
            boxFrame.m_rotation.m_col1.m_quad = rotation.m_col1.m_quad;
            boxFrame.m_rotation.m_col2.m_quad = rotation.m_col2.m_quad;
            boxFrame.m_translation.m_quad = _mm_mul_ps(center.m_quad, _mm_set1_ps(-1.0f));
            transformOut.setInverse(boxFrame);
            if (isAxisAligned(tolerance, &rotation.m_col0) && isAxisAligned(tolerance, &rotation.m_col1) &&
                isAxisAligned(tolerance, &rotation.m_col2)) {
                halfExtentsOut.setRotatedDir(transformOut.m_rotation, halfExtentsOut);
                halfExtentsOut(3) = 0.0f;
                halfExtentsOut.setAbs4(halfExtentsOut);
                rotation.m_col0.setZero4();
                rotation.m_col0(0) = 1.0f;
                rotation.m_col1.setZero4();
                rotation.m_col1(1) = 1.0f;
                rotation.m_col2.setZero4();
                rotation.m_col2(2) = 1.0f;
                transformOut.m_rotation.m_col0.m_quad = rotation.m_col0.m_quad;
                transformOut.m_rotation.m_col1.m_quad = rotation.m_col1.m_quad;
                transformOut.m_rotation.m_col2.m_quad = rotation.m_col2.m_quad;
            }
            boxFrame.setInverse(transformOut);
            for (int32_t i = 0; i < vertices.m_size; ++i) {
                hkVector4 local;
                local.setTransformedPos(boxFrame, vertices.m_data[i]);
                for (int32_t j = 0; j < 3; ++j) {
                    const float distance = std::fabs(local(j));
                    // The 1.001 is the double value of 1.001f widened.
                    if (halfExtentsOut(j) * 1.0010000467300415 < distance) {
                        goto done;
                    }
                }
            }
            isBox = true;
        }
    }
done:
    return isBox;
}

void hkGeometryUtility::expandPlanes(hkArray<hkVector4>& planeEquations, float byDistance) {
    for (hkVector4& plane : planeEquations) {
        XFNV_M128_F32(plane.m_quad, 3) -= byDistance;
    }
}

void hkGeometryUtility::createConvexGeometry(const hkStridedVertices& vertices, hkGeometry& geometry) {
    hkgpConvexHull apMemory;
    hkgpConvexHull::BuildConfig arg4;
    arg4.m_allowLowerDimensions.m_bool = 1;
    if (apMemory.build(vertices, arg4) != -1) {
        apMemory.generateGeometry(hkgpConvexHull::SOURCE_VERTICES, geometry);
    }
}

int32_t hkGeometryUtility::calcConvexHullVertices(
    const hkArray<hkVector4, hkContainerHeapAllocator>& points,
    hkArray<hkVector4, hkContainerHeapAllocator>& hullVertices
) {
    if (points.m_size >= 4) {
        hkgpConvexHull apThis;
        int32_t m_size = points.m_size;
        hkVector4* m_data = points.m_data;
        if (apThis.buildFromPlanes(m_data, m_size, hkgpConvexHull::BuildConfig()) != -1) {
            apThis.fetchPositions(hkgpConvexHull::INTERNAL_VERTICES, hullVertices);
            return 0;
        }
    }
    return 1;
}

// Copies x, y and z of each vertex into a packed hkVector4 buffer (w stays as allocated), stepping m_striding / 4
// floats (signed division) and reloading m_vertices and m_numVertices each pass.
hkSphere hkGeometryUtility::createMinBoundingSphere(const hkStridedVertices& stridedVertsIn, float radiusEpsilon) {
    int32_t iNumBytes = stridedVertsIn.m_numVertices << 4;
    hkVector4* pPoints =
        static_cast<hkVector4*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, iNumBytes));
    int32_t iStride = stridedVertsIn.m_striding / 4;
    int32_t iOffset = 0;
    for (int32_t i = 0; i < stridedVertsIn.m_numVertices; ++i) {
        XFNV_M128_F32(pPoints[i].m_quad, 0) = stridedVertsIn.m_vertices[iOffset];
        XFNV_M128_F32(pPoints[i].m_quad, 1) = stridedVertsIn.m_vertices[iOffset + 1];
        XFNV_M128_F32(pPoints[i].m_quad, 2) = stridedVertsIn.m_vertices[iOffset + 2];
        iOffset += iStride;
    }
    hkSphere kSphere;
    calcMinSphereFromPoints(&kSphere, pPoints, stridedVertsIn.m_numVertices, 0, radiusEpsilon);
    hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, pPoints);
    return kSphere;
}

// The smallest sphere around aiNumPoints packed points with the aiNumBoundary points before apPoints on its boundary,
// grown by afRadiusEpsilon, into apResult (Welzl's move-to-front recursion). Up to three boundary points give the
// starting sphere (an empty one of radius -1, the point itself, or the helpers' circumsphere); four or more (or a
// negative count) give the sphere through the last four, returned as is. A point outside the current sphere is moved to
// the front, one slot at a time, and the sphere is rebuilt with it on the boundary.
hkSphere* hkGeometryUtility::calcMinSphereFromPoints(
    hkSphere* apResult,
    hkVector4* apPoints,
    int32_t aiNumPoints,
    int32_t aiNumBoundary,
    float afRadiusEpsilon
) {
    hkSphere kSphere;
    switch (aiNumBoundary) {
        case 0:
            kSphere.m_pos.m_quad = _mm_setzero_ps();
            kSphere.m_pos(3) = -1.0f;
            apResult->m_pos.m_quad = kSphere.m_pos.m_quad;
            break;
        case 1:
            kSphere.m_pos.m_quad = apPoints[-1].m_quad;
            kSphere.m_pos(3) = afRadiusEpsilon;
            apResult->m_pos.m_quad = kSphere.m_pos.m_quad;
            break;
        case 2:
            apResult->m_pos.m_quad =
                calcSphereFrom2Points(&apPoints[-1], &apPoints[-2], afRadiusEpsilon, &kSphere)->m_pos.m_quad;
            break;
        case 3:
            apResult->m_pos.m_quad =
                calcSphereFrom3Points(&apPoints[-1], &apPoints[-2], &apPoints[-3], afRadiusEpsilon, &kSphere)
                    ->m_pos.m_quad;
            break;
        default:
            apResult->m_pos.m_quad =
                calcSphereFrom4Points(
                    &apPoints[-1], &apPoints[-2], &apPoints[-3], &apPoints[-4], afRadiusEpsilon, &kSphere
                )
                    ->m_pos.m_quad;
            return apResult;
    }
    for (int32_t i = 0; i < aiNumPoints; ++i) {
        hkVector4* pPoint = &apPoints[i];
        const __m128 kDelta = _mm_sub_ps(pPoint->m_quad, apResult->m_pos.m_quad);
        const __m128 kSq = _mm_mul_ps(kDelta, kDelta);
        const float fDistance = _mm_cvtss_f32(
            _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)))
        );
        if (apResult->m_pos(3) < fDistance) {
            for (int32_t j = i; j > 0; --j) {
                hkVector4 kSwap;
                kSwap.m_quad = apPoints[j].m_quad;
                apPoints[j].m_quad = apPoints[j - 1].m_quad;
                apPoints[j - 1].m_quad = kSwap.m_quad;
            }
            hkSphere kInner;
            apResult->m_pos.m_quad =
                calcMinSphereFromPoints(&kInner, apPoints + 1, i, aiNumBoundary + 1, afRadiusEpsilon)->m_pos.m_quad;
        }
    }
    return apResult;
}

void hkGeometryUtility::createConvexGeometry(
    const hkStridedVertices& stridedVertsIn,
    hkGeometry& geometryOut,
    hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsOut
) {
    hkgpConvexHull apMemory;
    hkgpConvexHull::BuildConfig arg4;
    arg4.m_allowLowerDimensions.m_bool = 1;
    if (apMemory.build(stridedVertsIn, arg4) != -1) {
        apMemory.generateGeometry(hkgpConvexHull::SOURCE_VERTICES, geometryOut);
        apMemory.buildIndices(0.9999f, false);
        apMemory.appendVertices(planeEquationsOut);
    }
}

// Fills geometryOut with the convex hull of the vertices of the planes' intersection, or empties it (its triangles'
// size first) when there are none. The size compare is signed.
void hkGeometryUtility::createGeometryFromPlaneEquations(
    const hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsIn,
    hkGeometry& geometryOut
) {
    hkArray<hkVector4> hullVertices;
    calcConvexHullVertices(planeEquationsIn, hullVertices);
    if (hullVertices.m_size > 0) {
        hkStridedVertices vertices;
        vertices.m_numVertices = hullVertices.m_size;
        vertices.m_vertices = &hullVertices.m_data[0](0);
        vertices.m_striding = sizeof(hkVector4);
        createConvexGeometry(vertices, geometryOut);
    }
    else {
        geometryOut.m_triangles.m_size = 0;
        geometryOut.m_vertices.m_size = 0;
    }
}

// The convex hull of the vertices is built, its triangles' corners copied to a stack-allocated array, and a box whose
// corners the hull's vertices are is taken as it is. Otherwise two candidate boxes are fitted, one along the world axes
// and one along the eigenvectors of the area-weighted covariance; in HK_GEOM_OBB_MODE_BETTER_FIT_BUT_SLOWER both are
// refined (20 iterations), and the one of smaller volume wins (the covariance box on a tie or a NaN).
void hkGeometryUtility::calcObb(
    const hkStridedVertices& stridedVertsIn,
    hkVector4& halfExtents,
    hkTransform& transform,
    hkGeomObbMode mode
) {
    hkGeometry geometry;
    hkGeometryUtility::createConvexGeometry(stridedVertsIn, geometry);
    hkLocalArray<hkVector4> corners(3 * geometry.m_triangles.m_size);
    corners.setSize(3 * geometry.m_triangles.m_size);
    for (int32_t i = 0; i < geometry.m_triangles.m_size; ++i) {
        corners.m_data[3 * i].m_quad = geometry.m_vertices.m_data[geometry.m_triangles.m_data[i].m_a].m_quad;
        corners.m_data[3 * i + 1].m_quad = geometry.m_vertices.m_data[geometry.m_triangles.m_data[i].m_b].m_quad;
        corners.m_data[3 * i + 2].m_quad = geometry.m_vertices.m_data[geometry.m_triangles.m_data[i].m_c].m_quad;
    }
    if (!getIsBoxGeometry(geometry.m_vertices, halfExtents, transform, 0.01f)) {
        const int32_t kStriding = static_cast<int32_t>(sizeof(hkVector4));
        const int32_t numCorners = corners.m_size;
        const float* pCorners = &(*corners.m_data)(0);
        hkVector4 axesA[3];
        axesA[0].set(1.0f, 0.0f, 0.0f, 0.0f);
        axesA[1].set(0.0f, 1.0f, 0.0f, 0.0f);
        axesA[2].set(0.0f, 0.0f, 1.0f, 0.0f);
        hkVector4 halfA;
        hkVector4 centerA;
        calcObbExtents(pCorners, numCorners, kStriding, &halfA, &centerA, axesA);
        float volumeA = halfA(1) * halfA(0) * halfA(2);
        hkMatrix3 covariance;
        hkVector4 centerB;
        calcCovarianceOfTriangles(pCorners, numCorners, kStriding, covariance, centerB, false);
        hkVector4 axesB[3];
        uint8_t converged;
        calcEigenVectors(&converged, &covariance.m_col0.m_quad, &axesB[0].m_quad, &axesB[1].m_quad, &axesB[2].m_quad);
        hkVector4 halfB;
        calcObbExtents(pCorners, numCorners, kStriding, &halfB, &centerB, axesB);
        float volumeB = halfB(1) * halfB(0) * halfB(2);
        if (mode == HK_GEOM_OBB_MODE_BETTER_FIT_BUT_SLOWER) {
            volumeA = optimizeOrientedBoxAxes(kStriding, pCorners, numCorners, axesA, 20, axesA);
            calcObbExtents(pCorners, numCorners, kStriding, &halfA, &centerA, axesA);
            volumeB = optimizeOrientedBoxAxes(kStriding, pCorners, numCorners, axesB, 20, axesB);
            calcObbExtents(pCorners, numCorners, kStriding, &halfB, &centerB, axesB);
        }
        const hkVector4* pAxes;
        const hkVector4* pCenter;
        if (volumeB > volumeA) {
            halfExtents.m_quad = halfA.m_quad;
            pCenter = &centerA;
            pAxes = axesA;
        }
        else {
            halfExtents.m_quad = halfB.m_quad;
            pCenter = &centerB;
            pAxes = axesB;
        }
        transform.m_rotation.m_col0.m_quad = pAxes[0].m_quad;
        transform.m_rotation.m_col1.m_quad = pAxes[1].m_quad;
        transform.m_rotation.m_col2.m_quad = pAxes[2].m_quad;
        transform.m_translation.m_quad = pCenter->m_quad;
    }
}

void hkPseudoRandomGenerator::getRandomVector01(hkVector4& vecOut) {
    uint32_t v3 = 0x19660D * m_current + 0x3C6EF35F;
    m_current = v3;
    XFNV_M128_F32(vecOut.m_quad, 0) = static_cast<double>(v3) * 2.3283064365386963e-10;
    uint32_t vecOuta = 0x19660D * m_current + 0x3C6EF35F;
    m_current = vecOuta;
    XFNV_M128_F32(vecOut.m_quad, 1) = static_cast<double>(vecOuta) * 2.3283064365386963e-10;
    uint32_t vecOutb = 0x19660D * m_current + 0x3C6EF35F;
    m_current = vecOutb;
    XFNV_M128_F32(vecOut.m_quad, 2) = static_cast<double>(vecOutb) * 2.3283064365386963e-10;
    uint32_t vecOutc = 0x19660D * m_current + 0x3C6EF35F;
    m_current = vecOutc;
    XFNV_M128_F32(vecOut.m_quad, 3) = 2.3283064365386963e-10 * static_cast<double>(vecOutc);
}
