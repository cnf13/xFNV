#include "Gamebryo2.2/CoreLibs/NiCollision/NiDistPntBox.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cmath>

void Case000(float* apPnt, const NiBox* apBox, float* apfSqrDistance) {
    if (-apBox->m_afExtent[0] > apPnt[0]) {
        float v5 = apBox->m_afExtent[0] + apPnt[0];
        *apfSqrDistance = v5 * v5 + *apfSqrDistance;
        apPnt[0] = -apBox->m_afExtent[0];
    }
    else if (apBox->m_afExtent[0] < static_cast<double>(apPnt[0])) {
        float v6 = apPnt[0] - apBox->m_afExtent[0];
        *apfSqrDistance = v6 * v6 + *apfSqrDistance;
        apPnt[0] = apBox->m_afExtent[0];
    }
    if (-apBox->m_afExtent[1] > apPnt[1]) {
        float v7 = apBox->m_afExtent[1] + apPnt[1];
        *apfSqrDistance = v7 * v7 + *apfSqrDistance;
        apPnt[1] = -apBox->m_afExtent[1];
    }
    else if (apBox->m_afExtent[1] < static_cast<double>(apPnt[1])) {
        float v8 = apPnt[1] - apBox->m_afExtent[1];
        *apfSqrDistance = v8 * v8 + *apfSqrDistance;
        apPnt[1] = apBox->m_afExtent[1];
    }
    if (-apBox->m_afExtent[2] > apPnt[2]) {
        float v9 = apBox->m_afExtent[2] + apPnt[2];
        *apfSqrDistance = v9 * v9 + *apfSqrDistance;
        apPnt[2] = -apBox->m_afExtent[2];
    }
    else if (apBox->m_afExtent[2] < static_cast<double>(apPnt[2])) {
        float v10 = apPnt[2] - apBox->m_afExtent[2];
        *apfSqrDistance = v10 * v10 + *apfSqrDistance;
        apPnt[2] = apBox->m_afExtent[2];
    }
}

float* CaseNoZeros(const NiBox* apBox, float* apfSqrDistance, float* apPnt, float* apDir, float* apfLParam) {
    float afPmE[3];
    afPmE[0] = apPnt[0] - apBox->m_afExtent[0];
    afPmE[1] = apPnt[1] - apBox->m_afExtent[1];
    afPmE[2] = apPnt[2] - apBox->m_afExtent[2];
    float fDyPx = apDir[1] * afPmE[0];
    double dDyPx = fDyPx;
    float fDxPy = apDir[0] * afPmE[1];
    if (!(fDxPy <= dDyPx)) {
        float fDzPy = afPmE[1] * apDir[2];
        double dDzPy = fDzPy;
        float fDyPz = apDir[1] * afPmE[2];
        if (fDyPz <= dDzPy) {
            return Face(2, apBox, 1, afPmE, apfLParam, apfSqrDistance, apPnt, apDir, 0);
        }
    }
    else {
        float fDzPx = afPmE[0] * apDir[2];
        double dDzPx = fDzPx;
        float fDxPz = apDir[0] * afPmE[2];
        if (fDxPz <= dDzPx) {
            return Face(1, apBox, 0, afPmE, apfLParam, apfSqrDistance, apPnt, apDir, 2);
        }
    }
    return Face(0, apBox, 2, afPmE, apfLParam, apfSqrDistance, apPnt, apDir, 1);
}

void Case0(
    int32_t ai1,
    int32_t ai0,
    const NiBox* apBox,
    int32_t ai2,
    float* apfLParam,
    float* apfSqrDistance,
    float* apPnt,
    float* apDir
) {
    float fPmE0 = apPnt[ai0] - apBox->m_afExtent[ai0];
    float fPmE1 = apPnt[ai1] - apBox->m_afExtent[ai1];
    float fProd0 = apDir[ai1] * fPmE0;
    float fProd1 = apDir[ai0] * fPmE1;
    if (fProd1 <= fProd0) {
        // The line meets the face apPnt[ai0] = extent[ai0].
        apPnt[ai0] = apBox->m_afExtent[ai0];
        float fPpE1 = apPnt[ai1] + apBox->m_afExtent[ai1];
        float fDelta = fProd0 - apDir[ai0] * fPpE1;
        if (fDelta >= 0.0f) {
            float fInvLSqr = 1.0f / (apDir[ai0] * apDir[ai0] + apDir[ai1] * apDir[ai1]);
            *apfSqrDistance = fDelta * fDelta * fInvLSqr + *apfSqrDistance;
            apPnt[ai1] = -apBox->m_afExtent[ai1];
            *apfLParam = -((apDir[ai0] * fPmE0 + apDir[ai1] * fPpE1) * fInvLSqr);
        }
        else {
            float fInv = 1.0f / apDir[ai0];
            apPnt[ai1] = apPnt[ai1] - fProd0 * fInv;
            *apfLParam = -fPmE0 * fInv;
        }
    }
    else {
        // The line meets the face apPnt[ai1] = extent[ai1].
        apPnt[ai1] = apBox->m_afExtent[ai1];
        float fPpE0 = apBox->m_afExtent[ai0] + apPnt[ai0];
        float fDelta = fProd1 - apDir[ai1] * fPpE0;
        if (fDelta >= 0.0f) {
            float fInvLSqr = 1.0f / (apDir[ai0] * apDir[ai0] + apDir[ai1] * apDir[ai1]);
            *apfSqrDistance = fDelta * fDelta * fInvLSqr + *apfSqrDistance;
            apPnt[ai0] = -apBox->m_afExtent[ai0];
            *apfLParam = -((apDir[ai0] * fPpE0 + apDir[ai1] * fPmE1) * fInvLSqr);
        }
        else {
            float fInv = 1.0f / apDir[ai1];
            apPnt[ai0] = apPnt[ai0] - fProd1 * fInv;
            *apfLParam = -fPmE1 * fInv;
        }
    }

    if (apPnt[ai2] < -apBox->m_afExtent[ai2]) {
        float fDelta = apPnt[ai2] + apBox->m_afExtent[ai2];
        *apfSqrDistance = fDelta * fDelta + *apfSqrDistance;
        apPnt[ai2] = -apBox->m_afExtent[ai2];
    }
    else if (apPnt[ai2] > apBox->m_afExtent[ai2]) {
        float fDelta = apPnt[ai2] - apBox->m_afExtent[ai2];
        *apfSqrDistance = fDelta * fDelta + *apfSqrDistance;
        apPnt[ai2] = apBox->m_afExtent[ai2];
    }
}

float* Face(
    int32_t ai1,
    const NiBox* apBox,
    int32_t ai0,
    float* apPmE,
    float* apfLParam,
    float* apfSqrDistance,
    float* apPnt,
    float* apDir,
    int32_t ai2
) {
    float afPpE[3];
    afPpE[ai1] = apPnt[ai1] + apBox->m_afExtent[ai1];
    afPpE[ai2] = apPnt[ai2] + apBox->m_afExtent[ai2];
    double v10 = apDir[ai0] * afPpE[ai1];
    double v11 = apDir[ai1] * apPmE[ai0];
    double v14 = apDir[ai0] * afPpE[ai2];
    double v16 = apPmE[ai0] * apDir[ai2];
    if (v10 >= v11) {
        if (v14 >= v16) {
            apPnt[ai0] = apBox->m_afExtent[ai0];
            float fInv = 1.0 / apDir[ai0];
            apPnt[ai1] = apPnt[ai1] - apDir[ai1] * apPmE[ai0] * fInv;
            apPnt[ai2] = apPnt[ai2] - apPmE[ai0] * apDir[ai2] * fInv;
            *apfLParam = fInv * -apPmE[ai0];
            return apPmE;
        }
        else {
            float fLSqr = apDir[ai0] * apDir[ai0] + apDir[ai2] * apDir[ai2];
            double v20 = apDir[ai0] * apPmE[ai0];
            double v21 = apDir[ai2] * afPpE[ai2];
            double v22 = fLSqr;
            float fTmp = afPpE[ai1] * fLSqr - (v21 + v20) * apDir[ai1];
            if (fTmp <= (v22 + v22) * apBox->m_afExtent[ai1]) {
                float fT = fTmp / v22;
                double v23 = fT;
                float fDiff = afPpE[ai1] - fT;
                double v24 = v20 + apDir[ai1] * fDiff;
                double v25 = fDiff;
                float fDelta = v21 + v24;
                double v27 = -fDelta;
                double v28 = v22 + apDir[ai1] * apDir[ai1];
                double v29 = fDelta;
                float fLSqr012 = v28;
                float fParam = v27 / fLSqr012;
                *apfSqrDistance = v29 * fParam + (afPpE[ai2] * afPpE[ai2] + (apPmE[ai0] * apPmE[ai0] + v25 * v25)) +
                    *apfSqrDistance;
                *apfLParam = fParam;
                apPnt[ai0] = apBox->m_afExtent[ai0];
                apPnt[ai1] = v23 - apBox->m_afExtent[ai1];
                apPnt[ai2] = -apBox->m_afExtent[ai2];
            }
            else {
                float fDelta = v21 + (v20 + apDir[ai1] * apPmE[ai1]);
                double v31 = fDelta;
                float fLSqr012 = v22 + apDir[ai1] * apDir[ai1];
                float fParam = -fDelta / fLSqr012;
                *apfSqrDistance = v31 * fParam +
                    (apPmE[ai0] * apPmE[ai0] + apPmE[ai1] * apPmE[ai1] + afPpE[ai2] * afPpE[ai2]) + *apfSqrDistance;
                *apfLParam = fParam;
                apPnt[ai0] = apBox->m_afExtent[ai0];
                apPnt[ai1] = apBox->m_afExtent[ai1];
                apPnt[ai2] = -apBox->m_afExtent[ai2];
            }
            return apfLParam;
        }
    }
    else {
        double v32 = apDir[ai0];
        if (v14 >= v16) {
            float fLSqr = v32 * v32 + apDir[ai1] * apDir[ai1];
            double v33 = apDir[ai0] * apPmE[ai0] + apDir[ai1] * afPpE[ai1];
            double v34 = fLSqr;
            float fTmp = fLSqr * afPpE[ai2] - apDir[ai2] * v33;
            if (fTmp <= (v34 + v34) * apBox->m_afExtent[ai2]) {
                float fT = fTmp / v34;
                double v36 = fT;
                float fDiff = afPpE[ai2] - fT;
                double v37 = v33 + apDir[ai2] * fDiff;
                double v38 = fDiff;
                float fDelta = v37;
                double v39 = -fDelta;
                double v40 = v34 + apDir[ai2] * apDir[ai2];
                double v41 = fDelta;
                float fLSqr012 = v40;
                float fParam = v39 / fLSqr012;
                *apfSqrDistance = v41 * fParam + (v38 * v38 + (apPmE[ai0] * apPmE[ai0] + afPpE[ai1] * afPpE[ai1])) +
                    *apfSqrDistance;
                *apfLParam = fParam;
                apPnt[ai0] = apBox->m_afExtent[ai0];
                apPnt[ai1] = -apBox->m_afExtent[ai1];
                apPnt[ai2] = v36 - apBox->m_afExtent[ai2];
            }
            else {
                float fDelta = v33 + apDir[ai2] * apPmE[ai2];
                double v44 = fDelta;
                float fLSqr012 = v34 + apDir[ai2] * apDir[ai2];
                float fParam = -fDelta / fLSqr012;
                *apfSqrDistance = v44 * fParam +
                    (apPmE[ai2] * apPmE[ai2] + (afPpE[ai1] * afPpE[ai1] + apPmE[ai0] * apPmE[ai0])) + *apfSqrDistance;
                *apfLParam = fParam;
                apPnt[ai0] = apBox->m_afExtent[ai0];
                apPnt[ai1] = -apBox->m_afExtent[ai1];
                apPnt[ai2] = apBox->m_afExtent[ai2];
            }
            return apfLParam;
        }
        else {
            double v45 = v32 * v32;
            double v46 = apDir[ai2] * apDir[ai2];
            float fLSqr02 = v46 + v45;
            double v47 = apDir[ai0] * apPmE[ai0];
            double v48 = apDir[ai2] * afPpE[ai2];
            double v49 = fLSqr02;
            float fTmp1 = afPpE[ai1] * fLSqr02 - (v48 + v47) * apDir[ai1];
            double v50 = fTmp1;
            if (fTmp1 >= 0.0f) {
                if (v50 <= (v49 + v49) * apBox->m_afExtent[ai1]) {
                    float fT = v50 / v49;
                    double v53 = fT;
                    float fDiff = afPpE[ai1] - fT;
                    double v54 = v47 + apDir[ai1] * fDiff;
                    double v55 = fDiff;
                    double v56 = v48 + v54;
                    float fDelta = v56;
                    double v58 = -fDelta;
                    double v59 = v49 + apDir[ai1] * apDir[ai1];
                    double v60 = fDelta;
                    float fLSqr012 = v59;
                    float fParam = v58 / fLSqr012;
                    *apfSqrDistance = v60 * fParam + (afPpE[ai2] * afPpE[ai2] + (apPmE[ai0] * apPmE[ai0] + v55 * v55)) +
                        *apfSqrDistance;
                    *apfLParam = fParam;
                    apPnt[ai0] = apBox->m_afExtent[ai0];
                    apPnt[ai1] = v53 - apBox->m_afExtent[ai1];
                    apPnt[ai2] = -apBox->m_afExtent[ai2];
                }
                else {
                    float fDelta = v48 + (v47 + apDir[ai1] * apPmE[ai1]);
                    double v62 = fDelta;
                    float fLSqr012 = v49 + apDir[ai1] * apDir[ai1];
                    float fParam = -fDelta / fLSqr012;
                    *apfSqrDistance = v62 * fParam +
                        (apPmE[ai0] * apPmE[ai0] + apPmE[ai1] * apPmE[ai1] + afPpE[ai2] * afPpE[ai2]) + *apfSqrDistance;
                    *apfLParam = fParam;
                    apPnt[ai0] = apBox->m_afExtent[ai0];
                    apPnt[ai1] = apBox->m_afExtent[ai1];
                    apPnt[ai2] = -apBox->m_afExtent[ai2];
                }
                return apfLParam;
            }

            float fLSqr01 = v45 + apDir[ai1] * apDir[ai1];
            double v63 = v47 + apDir[ai1] * afPpE[ai1];
            double v64 = fLSqr01;
            float fTmp2 = fLSqr01 * afPpE[ai2] - apDir[ai2] * v63;
            double v65 = fTmp2;
            if (fTmp2 >= 0.0f) {
                if (v65 <= (v64 + v64) * apBox->m_afExtent[ai2]) {
                    float fT = v65 / v64;
                    double v68 = fT;
                    float fDiff = afPpE[ai2] - fT;
                    double v69 = v63 + apDir[ai2] * fDiff;
                    double v70 = fDiff;
                    float fDelta = v69;
                    double v71 = -fDelta;
                    double v72 = v46 + v64;
                    double v73 = fDelta;
                    float fLSqr012 = v72;
                    float fParam = v71 / fLSqr012;
                    *apfSqrDistance = v73 * fParam + (v70 * v70 + (apPmE[ai0] * apPmE[ai0] + afPpE[ai1] * afPpE[ai1])) +
                        *apfSqrDistance;
                    *apfLParam = fParam;
                    apPnt[ai0] = apBox->m_afExtent[ai0];
                    apPnt[ai1] = -apBox->m_afExtent[ai1];
                    apPnt[ai2] = v68 - apBox->m_afExtent[ai2];
                }
                else {
                    float fEdgeDelta = v63 + apDir[ai2] * apPmE[ai2];
                    float fEdgeLSqr = v46 + v64;
                    double v77 = fEdgeDelta;
                    float fEdgeParam = -fEdgeDelta / fEdgeLSqr;
                    *apfSqrDistance = v77 * fEdgeParam +
                        (apPmE[ai2] * apPmE[ai2] + (afPpE[ai1] * afPpE[ai1] + apPmE[ai0] * apPmE[ai0])) +
                        *apfSqrDistance;
                    *apfLParam = fEdgeParam;
                    apPnt[ai0] = apBox->m_afExtent[ai0];
                    apPnt[ai1] = -apBox->m_afExtent[ai1];
                    apPnt[ai2] = apBox->m_afExtent[ai2];
                }
                return apfLParam;
            }

            float fCornerDelta = v63 + v48;
            double v78 = -fCornerDelta;
            double v79 = v46 + v64;
            double v80 = fCornerDelta;
            float fCornerLSqr = v79;
            float fCornerParam = v78 / fCornerLSqr;
            *apfSqrDistance = v80 * fCornerParam +
                (afPpE[ai2] * afPpE[ai2] + (afPpE[ai1] * afPpE[ai1] + apPmE[ai0] * apPmE[ai0])) + *apfSqrDistance;
            *apfLParam = fCornerParam;
            apPnt[ai0] = apBox->m_afExtent[ai0];
            apPnt[ai1] = -apBox->m_afExtent[ai1];
            apPnt[ai2] = -apBox->m_afExtent[ai2];
            return apfLParam;
        }
    }
}

// Wild Magic's Face(i0, i1, i2, ...) tail for a direction with no component along i0: moves the point onto the box face
// i0, giving the line parameter, then clamps components i1 and i2 to the box, adding the squared distance of each
// clamp. Each clamp's difference is rounded through a float local.
void CaseTwoZeros(
    float* afPnt,
    const NiBox* pkBox,
    int32_t i0,
    const float* afDir,
    float* pfLParam,
    float* pfSqrDistance,
    int32_t i2,
    int32_t i1
) {
    *pfLParam = (pkBox->m_afExtent[i0] - afPnt[i0]) / afDir[i0];
    afPnt[i0] = pkBox->m_afExtent[i0];

    if (-pkBox->m_afExtent[i1] > afPnt[i1]) {
        float fDelta = afPnt[i1] + pkBox->m_afExtent[i1];
        *pfSqrDistance += fDelta * fDelta;
        afPnt[i1] = -pkBox->m_afExtent[i1];
    }
    else if (pkBox->m_afExtent[i1] < afPnt[i1]) {
        float fDelta = afPnt[i1] - pkBox->m_afExtent[i1];
        *pfSqrDistance += fDelta * fDelta;
        afPnt[i1] = pkBox->m_afExtent[i1];
    }

    if (-pkBox->m_afExtent[i2] > afPnt[i2]) {
        float fDelta = pkBox->m_afExtent[i2] + afPnt[i2];
        *pfSqrDistance += fDelta * fDelta;
        afPnt[i2] = -pkBox->m_afExtent[i2];
    }
    else if (pkBox->m_afExtent[i2] < afPnt[i2]) {
        float fDelta = afPnt[i2] - pkBox->m_afExtent[i2];
        *pfSqrDistance += fDelta * fDelta;
        afPnt[i2] = pkBox->m_afExtent[i2];
    }
}

float NiSqrDistance::Compute(const NiPoint3& kPnt, const NiSegment& kSeg, float& fSegParam) {
    double v7;
    float v10;
    float v12;
    float v14;
    float v15;
    float v16;
    float v17;
    float v19;
    float v20;
    float v9 = kPnt.x - kSeg.m_kOrigin.x;
    float v11 = kPnt.y - kSeg.m_kOrigin.y;
    float v13 = kPnt.z - kSeg.m_kOrigin.z;
    double v3 = v11;
    double v4 = v9;
    double v5 = v13;
    float v18 = (kSeg.m_kDirection.z * v13) + ((kSeg.m_kDirection.y * v11) + (v9 * kSeg.m_kDirection.x));
    double v6 = v18;
    fSegParam = v18;
    if (!(v18 <= 0.0)) {
        v19 = ((kSeg.m_kDirection.y * kSeg.m_kDirection.y) + (kSeg.m_kDirection.x * kSeg.m_kDirection.x)) +
            (kSeg.m_kDirection.z * kSeg.m_kDirection.z);
        if (!(v19 <= v6)) {
            v20 = static_cast<float>(v6 / v19);
            fSegParam = v20;
            v15 = kSeg.m_kDirection.x * v20;
            v16 = kSeg.m_kDirection.y * v20;
            v17 = v20 * kSeg.m_kDirection.z;
            v10 = static_cast<float>(v4 - v15);
            v12 = static_cast<float>(v3 - v16);
            v7 = v5 - v17;
        }
        else {
            fSegParam = 1.0f;
            v10 = static_cast<float>(v4 - kSeg.m_kDirection.x);
            v12 = static_cast<float>(v3 - kSeg.m_kDirection.y);
            v7 = v5 - kSeg.m_kDirection.z;
        }
        v14 = static_cast<float>(v7);
        v5 = v14;
        v3 = v12;
        v4 = v10;
    }
    else {
        fSegParam = 0.0f;
    }
    return static_cast<float>((v5 * v5) + ((v3 * v3) + (v4 * v4)));
}

float NiSqrDistance::Compute(const NiSegment& kSeg0, const NiSegment& kSeg1, float& fSeg0Param, float& fSeg1Param) {
    float v36 = kSeg0.m_kOrigin.x - kSeg1.m_kOrigin.x;
    float v38 = kSeg0.m_kOrigin.y - kSeg1.m_kOrigin.y;
    float v40 = kSeg0.m_kOrigin.z - kSeg1.m_kOrigin.z;
    float v34 = kSeg0.m_kDirection.y * kSeg0.m_kDirection.y + kSeg0.m_kDirection.x * kSeg0.m_kDirection.x +
        kSeg0.m_kDirection.z * kSeg0.m_kDirection.z;
    float kSeg0b = kSeg1.m_kDirection.y * kSeg0.m_kDirection.y + kSeg1.m_kDirection.x * kSeg0.m_kDirection.x +
        kSeg1.m_kDirection.z * kSeg0.m_kDirection.z;
    float kSeg1p = -kSeg0b;
    float v33 = kSeg1.m_kDirection.y * kSeg1.m_kDirection.y + kSeg1.m_kDirection.x * kSeg1.m_kDirection.x +
        kSeg1.m_kDirection.z * kSeg1.m_kDirection.z;
    float v23 = kSeg0.m_kDirection.z * v40 + (kSeg0.m_kDirection.x * v36 + kSeg0.m_kDirection.y * v38);
    float kSeg0c = kSeg1.m_kDirection.y * v38 + kSeg1.m_kDirection.x * v36 + kSeg1.m_kDirection.z * v40;
    float v31 = -kSeg0c;
    float kSeg0a = v40 * v40 + (v38 * v38 + v36 * v36);
    double v6 = kSeg1p;
    double v7 = v33;
    double v8 = v34;
    float kSeg1q = v33 * v34 - kSeg1p * kSeg1p;
    float kSeg1a = std::fabs(static_cast<double>(kSeg1q));
    float v37 = kSeg0.m_kDirection.y * kSeg1.m_kDirection.z - kSeg0.m_kDirection.z * kSeg1.m_kDirection.y;
    float v39 = kSeg1.m_kDirection.x * kSeg0.m_kDirection.z - kSeg0.m_kDirection.x * kSeg1.m_kDirection.z;
    float v41 = kSeg0.m_kDirection.x * kSeg1.m_kDirection.y - kSeg1.m_kDirection.x * kSeg0.m_kDirection.y;
    float v35 = v39 * v39 + v37 * v37 + v41 * v41;
    if (!(v35 > 1e-06f)) {
        double v19 = v23;
        if (!(v6 > 0.0)) {
            float kSeg1o = -v19;
            if (kSeg1o >= v8) {
                fSeg0Param = 1.0;
                fSeg1Param = 0.0;
                float kSeg0s = v8 + (v19 + v19) + kSeg0a;
                return std::fabs(static_cast<double>(kSeg0s));
            }
            double v21 = v23;
            if (v23 <= 0.0) {
                fSeg0Param = kSeg1o / v8;
                fSeg1Param = 0.0;
                float kSeg0u = v21 * fSeg0Param + kSeg0a;
                return std::fabs(static_cast<double>(kSeg0u));
            }
            fSeg0Param = 0.0;
            if (!(-v6 <= v21)) {
                float kSeg1bh = kSeg1o / v6;
                fSeg1Param = kSeg1bh;
                kSeg0a = (v7 * kSeg1bh + (v31 + v31)) * kSeg1bh + kSeg0a;
            }
            else {
                fSeg1Param = 1.0;
                kSeg0a = v7 + (v31 + v31) + kSeg0a;
            }
        }
        else {
            if (v19 >= 0.0) {
                fSeg0Param = 0.0;
                fSeg1Param = 0.0;
                return std::fabs(static_cast<double>(kSeg0a));
            }
            float kSeg1m = -v19;
            if (kSeg1m <= v8) {
                fSeg0Param = kSeg1m / v8;
                fSeg1Param = 0.0;
                float kSeg0q = v19 * fSeg0Param + kSeg0a;
                return std::fabs(static_cast<double>(kSeg0q));
            }
            double v20 = v23;
            fSeg0Param = 1.0;
            float v30 = v23 + v8;
            float kSeg1n = -v30;
            if (!(kSeg1n >= v6)) {
                float kSeg1bg = kSeg1n / v6;
                fSeg1Param = kSeg1bg;
                kSeg0a = (v7 * kSeg1bg + (v6 + v31) * 2.0) * kSeg1bg + (v8 + 2.0 * v20 + kSeg0a);
            }
            else {
                fSeg1Param = 1.0;
                kSeg0a = v6 + v20 + v31 + (v6 + v20 + v31) + (v8 + v7 + kSeg0a);
            }
        }
        return std::fabs(static_cast<double>(kSeg0a));
    }
    double v9 = v31;
    double v10 = v23;
    fSeg0Param = v31 * v6 - v23 * v7;
    float v32 = v23 * v6 - v31 * v8;
    double v11 = v32;
    fSeg1Param = v32;
    if (!(fSeg0Param >= 0.0)) {
        if (!(v11 >= 0.0)) {
            if (v10 < 0.0) {
                fSeg1Param = 0.0;
                float kSeg1k = -v10;
                if (!(kSeg1k >= v8)) {
                    float kSeg1be = kSeg1k / v8;
                    fSeg0Param = kSeg1be;
                    kSeg0a = v10 * kSeg1be + kSeg0a;
                }
                else {
                    fSeg0Param = 1.0;
                    kSeg0a = v8 + (v10 + v10) + kSeg0a;
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
            fSeg0Param = 0.0;
            if (!(v9 >= 0.0)) {
                float kSeg1l = -v9;
                if (!(kSeg1l >= v7)) {
                    float kSeg1bf = kSeg1l / v7;
                    fSeg1Param = kSeg1bf;
                    kSeg0a = v9 * kSeg1bf + kSeg0a;
                }
                else {
                    fSeg1Param = 1.0;
                    kSeg0a = v7 + (v9 + v9) + kSeg0a;
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
        }
        else {
            if (!(kSeg1a >= v11)) {
                float v29 = v6 + v10;
                if (!(v29 < 0.0)) {
                    fSeg0Param = 0.0;
                    float kSeg1bc = v9 + v7;
                    if (kSeg1bc <= 0.0) {
                        fSeg1Param = 1.0;
                        float kSeg0n = v7 + (v9 + v9) + kSeg0a;
                        return std::fabs(static_cast<double>(kSeg0n));
                    }
                    if (!(v9 >= 0.0)) {
                        float kSeg1bd = -v9 / v7;
                        fSeg1Param = kSeg1bd;
                        kSeg0a = v9 * kSeg1bd + kSeg0a;
                    }
                    else {
                        fSeg1Param = 0.0;
                    }
                }
                else {
                    double v18 = v29;
                    fSeg1Param = 1.0;
                    float kSeg1j = -v29;
                    if (!(kSeg1j >= v8)) {
                        float kSeg1bb = kSeg1j / v8;
                        fSeg0Param = kSeg1bb;
                        kSeg0a = v18 * kSeg1bb + v7 + (v9 + v9) + kSeg0a;
                    }
                    else {
                        fSeg0Param = 1.0;
                        kSeg0a = v8 + v7 + kSeg0a + (v18 + v9 + (v18 + v9));
                    }
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
            fSeg0Param = 0.0;
            if (!(v9 >= 0.0)) {
                float kSeg1i = -v9;
                if (!(kSeg1i >= v7)) {
                    float kSeg1ba = kSeg1i / v7;
                    fSeg1Param = kSeg1ba;
                    kSeg0a = v9 * kSeg1ba + kSeg0a;
                }
                else {
                    fSeg1Param = 1.0;
                    kSeg0a = v7 + (v9 + v9) + kSeg0a;
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
        }
        fSeg1Param = 0.0;
        return std::fabs(static_cast<double>(kSeg0a));
    }
    double v12 = v32;
    if (!(kSeg1a >= static_cast<double>(fSeg0Param))) {
        if (!(v32 >= 0.0)) {
            float kSeg1g = -v10;
            if (kSeg1g < v8) {
                fSeg1Param = 0.0;
                if (!(v10 >= 0.0)) {
                    float kSeg1y = kSeg1g / v8;
                    fSeg0Param = kSeg1y;
                    kSeg0a = v10 * kSeg1y + kSeg0a;
                }
                else {
                    fSeg0Param = 0.0;
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
            fSeg0Param = 1.0;
            float v28 = v6 + v9;
            if (!(v28 >= 0.0)) {
                double v17 = v28;
                float kSeg1h = -v28;
                if (!(kSeg1h >= v7)) {
                    float kSeg1z = kSeg1h / v7;
                    fSeg1Param = kSeg1z;
                    kSeg0a = v8 + v17 * kSeg1z + (v10 + v10) + kSeg0a;
                }
                else {
                    fSeg1Param = 1.0;
                    kSeg0a = v8 + v7 + kSeg0a + (v17 + v10 + (v17 + v10));
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
        }
        else {
            if (!(kSeg1a >= v12)) {
                float v26 = v10 + v6;
                float kSeg1e = -v26;
                if (!(kSeg1e <= v8)) {
                    fSeg0Param = 1.0;
                    float v27 = v6 + v9;
                    double v16 = v27;
                    float kSeg1f = -v27;
                    if (kSeg1f >= v7) {
                        fSeg1Param = 1.0;
                        float kSeg0k = v8 + v7 + kSeg0a + (v16 + v10 + (v16 + v10));
                        return std::fabs(static_cast<double>(kSeg0k));
                    }
                    if (!(v16 > 0.0)) {
                        float kSeg1x = kSeg1f / v7;
                        fSeg1Param = kSeg1x;
                        kSeg0a = v8 + v16 * kSeg1x + (v10 + v10) + kSeg0a;
                    }
                    else {
                        fSeg1Param = 0.0;
                        kSeg0a = v8 + (v10 + v10) + kSeg0a;
                    }
                }
                else {
                    fSeg1Param = 1.0;
                    if (!(v26 >= 0.0)) {
                        float kSeg1w = kSeg1e / v8;
                        fSeg0Param = kSeg1w;
                        kSeg0a = v26 * kSeg1w + v7 + (v9 + v9) + kSeg0a;
                    }
                    else {
                        fSeg0Param = 0.0;
                        kSeg0a = v7 + (v9 + v9) + kSeg0a;
                    }
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
            fSeg0Param = 1.0;
            float v25 = v6 + v9;
            if (!(v25 >= 0.0)) {
                double v15 = v25;
                float kSeg1d = -v25;
                if (!(kSeg1d >= v7)) {
                    float kSeg1v = kSeg1d / v7;
                    fSeg1Param = kSeg1v;
                    kSeg0a = v8 + v15 * kSeg1v + (v10 + v10) + kSeg0a;
                }
                else {
                    fSeg1Param = 1.0;
                    kSeg0a = v8 + v7 + kSeg0a + (v15 + v10 + (v15 + v10));
                }
                return std::fabs(static_cast<double>(kSeg0a));
            }
        }
        fSeg1Param = 0.0;
        float kSeg0i = v8 + (v10 + v10) + kSeg0a;
        return std::fabs(static_cast<double>(kSeg0i));
    }
    if (!(v32 >= 0.0)) {
        fSeg1Param = 0.0;
        if (v10 >= 0.0) {
            fSeg0Param = 0.0;
            return std::fabs(static_cast<double>(kSeg0a));
        }
        float kSeg1c = -v10;
        if (!(kSeg1c >= v8)) {
            float kSeg1u = kSeg1c / v8;
            fSeg0Param = kSeg1u;
            kSeg0a = v10 * kSeg1u + kSeg0a;
        }
        else {
            fSeg0Param = 1.0;
            kSeg0a = v8 + (v10 + v10) + kSeg0a;
        }
        return std::fabs(static_cast<double>(kSeg0a));
    }
    if (!(kSeg1a >= v12)) {
        fSeg1Param = 1.0;
        float v24 = v6 + v10;
        if (v24 >= 0.0) {
            fSeg0Param = 0.0;
            float kSeg0f = v7 + (v9 + v9) + kSeg0a;
            return std::fabs(static_cast<double>(kSeg0f));
        }
        double v14 = v24;
        float kSeg1b = -v24;
        if (!(kSeg1b >= v8)) {
            float kSeg1t = kSeg1b / v8;
            fSeg0Param = kSeg1t;
            kSeg0a = v14 * kSeg1t + v7 + (v9 + v9) + kSeg0a;
        }
        else {
            fSeg0Param = 1.0;
            kSeg0a = v8 + v7 + kSeg0a + (v14 + v9 + (v14 + v9));
        }
        return std::fabs(static_cast<double>(kSeg0a));
    }
    float kSeg1r = 1.0 / kSeg1a;
    fSeg0Param = fSeg0Param * kSeg1r;
    float kSeg1s = kSeg1r * fSeg1Param;
    fSeg1Param = kSeg1s;
    float kSeg0d = (v6 * fSeg1Param + v8 * fSeg0Param + 2.0 * v10) * fSeg0Param +
        (v9 * 2.0 + (v7 * kSeg1s + fSeg0Param * v6)) * kSeg1s + kSeg0a;
    return std::fabs(static_cast<double>(kSeg0d));
}

float NiSqrDistance::Compute(
    const NiSegment& kSeg,
    const NiTrigon& kTri,
    float& fSegParm,
    float& fTriParm0,
    float& fTriParm1
) {
    NiPoint3 kPoint_2;
    NiSegment kTriSeg;
    NiPoint3 kPoint;
    float fMin;
    float fMin0;
    const NiPoint3* p_m_kEdge0 = &kTri.m_kEdge0;
    const NiPoint3* p_m_kEdge1 = &kTri.m_kEdge1;
    kTriSeg.m_kOrigin.x = kTri.m_kOrigin.x - kSeg.m_kOrigin.x;
    kTriSeg.m_kOrigin.y = kTri.m_kOrigin.y - kSeg.m_kOrigin.y;
    kTriSeg.m_kOrigin.z = kTri.m_kOrigin.z - kSeg.m_kOrigin.z;
    float v105 = kSeg.m_kDirection.y * kSeg.m_kDirection.y + kSeg.m_kDirection.x * kSeg.m_kDirection.x +
        kSeg.m_kDirection.z * kSeg.m_kDirection.z;
    float kTrid = kTri.m_kEdge0.y * kSeg.m_kDirection.y + kTri.m_kEdge0.x * kSeg.m_kDirection.x +
        kTri.m_kEdge0.z * kSeg.m_kDirection.z;
    float v104 = -kTrid;
    float kTrie = p_m_kEdge1->y * kSeg.m_kDirection.y + p_m_kEdge1->x * kSeg.m_kDirection.x +
        p_m_kEdge1->z * kSeg.m_kDirection.z;
    float fTmpTriParm0 = -kTrie;
    float fTmpTriParm1 = p_m_kEdge0->y * p_m_kEdge0->y + p_m_kEdge0->x * p_m_kEdge0->x + p_m_kEdge0->z * p_m_kEdge0->z;
    float v102 = p_m_kEdge1->y * p_m_kEdge0->y + p_m_kEdge0->x * p_m_kEdge1->x + p_m_kEdge1->z * p_m_kEdge0->z;
    float fTmpSegParm = p_m_kEdge1->y * p_m_kEdge1->y + p_m_kEdge1->x * p_m_kEdge1->x + p_m_kEdge1->z * p_m_kEdge1->z;
    kPoint.x = p_m_kEdge1->z * p_m_kEdge0->y - p_m_kEdge1->y * p_m_kEdge0->z;
    kPoint.y = p_m_kEdge1->x * p_m_kEdge0->z - p_m_kEdge1->z * p_m_kEdge0->x;
    kPoint.z = p_m_kEdge0->x * p_m_kEdge1->y - p_m_kEdge1->x * p_m_kEdge0->y;
    // The three direction components are held as doubles for the length below.
    double y = kSeg.m_kDirection.y;
    double x = kSeg.m_kDirection.x;
    double z = kSeg.m_kDirection.z;
    float kTrif = kPoint.x * kSeg.m_kDirection.x + kPoint.y * kSeg.m_kDirection.y + kPoint.z * kSeg.m_kDirection.z;
    float kTrig = std::fabs(static_cast<double>(kTrif));
    float v103 = kTrig;
    float kTrih = kPoint.z * kPoint.z + (kPoint.y * kPoint.y + kPoint.x * kPoint.x);
    float kTrii = std::sqrt(static_cast<double>(kTrih));
    float kTrij = x * x + y * y + z * z;
    float kTrik = std::sqrt(static_cast<double>(kTrij));
    float kTril = kTrik * kTrii * 9.999999747378752e-06;
    if (!(v103 > static_cast<double>(kTril))) {
        kTriSeg.m_kOrigin = kTri.m_kOrigin;
        kTriSeg.m_kDirection = *p_m_kEdge0;
        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
        fTriParm1 = 0.0f;
        kTriSeg.m_kDirection = *p_m_kEdge1;
        float kPntp = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
        fTmpTriParm0 = 0.0f;
        if (fMin > static_cast<double>(kPntp)) {
            fMin = kPntp;
            fSegParm = fTmpSegParm;
            fTriParm0 = 0.0f;
            fTriParm1 = fTmpTriParm1;
        }
        double v93 = p_m_kEdge0->y;
        double v94 = p_m_kEdge0->z;
        kPoint.x = kTriSeg.m_kOrigin.x + p_m_kEdge0->x;
        kTriSeg.m_kOrigin.x = kPoint.x;
        kPoint.y = v93 + kTriSeg.m_kOrigin.y;
        kTriSeg.m_kOrigin.y = kPoint.y;
        kPoint.z = v94 + kTriSeg.m_kOrigin.z;
        kTriSeg.m_kOrigin.z = kPoint.z;
        kPoint.x = kTriSeg.m_kDirection.x - p_m_kEdge0->x;
        kTriSeg.m_kDirection.x = kPoint.x;
        kPoint.y = kTriSeg.m_kDirection.y - p_m_kEdge0->y;
        kTriSeg.m_kDirection.y = kPoint.y;
        kPoint.z = kTriSeg.m_kDirection.z - p_m_kEdge0->z;
        kTriSeg.m_kDirection.z = kPoint.z;
        float kPntq = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
        double v95 = fTmpTriParm1;
        fTmpTriParm0 = 1.0 - fTmpTriParm1;
        if (fMin > static_cast<double>(kPntq)) {
            fMin = kPntq;
            fSegParm = fTmpSegParm;
            fTriParm0 = fTmpTriParm0;
            fTriParm1 = v95;
        }
        float kPntr = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
        fTmpSegParm = 0.0f;
        if (fMin > static_cast<double>(kPntr)) {
            fMin = kPntr;
            fSegParm = 0.0f;
            fTriParm0 = fTmpTriParm0;
            fTriParm1 = fTmpTriParm1;
        }
        double v96 = kSeg.m_kDirection.y;
        double v97 = kSeg.m_kDirection.z;
        kPoint.x = kSeg.m_kOrigin.x + kSeg.m_kDirection.x;
        kPoint_2.x = kPoint.x;
        kPoint.y = v96 + kSeg.m_kOrigin.y;
        kPoint_2.y = kPoint.y;
        kPoint.z = v97 + kSeg.m_kOrigin.z;
        kPoint_2.z = kPoint.z;
        float kPnts = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
        if (fMin > static_cast<double>(kPnts)) {
            fMin = kPnts;
            fSegParm = 1.0f;
            fTriParm0 = fTmpTriParm0;
            fTriParm1 = fTmpTriParm1;
        }
    }
    else {
        double v9 = fTmpSegParm;
        double v10 = fTmpTriParm1;
        fTmpTriParm1 = fTmpSegParm * fTmpTriParm1 - v102 * v102;
        double v11 = fTmpTriParm0;
        double v12 = v104;
        float kPntu = v102 * fTmpTriParm0 - fTmpSegParm * v104;
        fTmpSegParm = v102 * v104 - v10 * fTmpTriParm0;
        double v13 = v105;
        v103 = fTmpTriParm0 * v104 - v102 * v105;
        float kTrim = kPntu * v104 + fTmpTriParm1 * v105 + fTmpSegParm * fTmpTriParm0;
        float kTrin = 1.0 / kTrim;
        v105 = kSeg.m_kDirection.y * kTriSeg.m_kOrigin.y + kSeg.m_kDirection.x * kTriSeg.m_kOrigin.x +
            kSeg.m_kDirection.z * kTriSeg.m_kOrigin.z;
        v102 = -(-v105) * kTrin;
        v105 = p_m_kEdge0->y * kTriSeg.m_kOrigin.y + p_m_kEdge0->x * kTriSeg.m_kOrigin.x +
            p_m_kEdge0->z * kTriSeg.m_kOrigin.z;
        fTmpTriParm0 = -v105 * kTrin;
        v105 = p_m_kEdge1->y * kTriSeg.m_kOrigin.y + p_m_kEdge1->x * kTriSeg.m_kOrigin.x +
            p_m_kEdge1->z * kTriSeg.m_kOrigin.z;
        float kTrio = -v105 * kTrin;
        fSegParm = fTmpTriParm0 * kPntu + v102 * fTmpTriParm1 + kTrio * fTmpSegParm;
        v105 = v9 * v13 - v11 * v11;
        double v15 = fTmpTriParm0;
        double v16 = v102;
        fTriParm0 = v105 * fTmpTriParm0 + v102 * kPntu + kTrio * v103;
        float kPntv = v13 * v10 - v12 * v12;
        float kTrip = v16 * fTmpSegParm + v15 * v103 + kPntv * kTrio;
        double v17 = kTrip;
        fTriParm1 = kTrip;
        if (fSegParm < 0.0f) {
            if (fTriParm0 + v17 <= 1.0) {
                if (fTriParm0 < 0.0f) {
                    if (v17 < 0.0) {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge0;
                        fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm0);
                        fTmpTriParm1 = 0.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                        fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                        fTmpSegParm = 0.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                    }
                    else {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                        fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                        fTmpSegParm = 0.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                    fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 0.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else {
                    fMin = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTriParm0, fTriParm1);
                    fSegParm = 0.0f;
                }
            }
            else {
                if (fTriParm0 < 0.0f) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge1;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                    fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 0.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                    fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 0.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 1.0f - fTriParm1;
                    fMin0 = NiSqrDistance::Compute(kSeg.m_kOrigin, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 0.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
            }
        }
        else if (fSegParm <= 1.0f) {
            if (fTriParm0 + v17 <= 1.0) {
                if (fTriParm0 < 0.0f) {
                    if (v17 < 0.0) {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge0;
                        fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm0);
                        fTmpTriParm1 = 0.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                    }
                    else {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                }
                else {
                    fMin = 0.0f;
                }
            }
            else {
                if (fTriParm0 < 0.0f) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge1;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 1.0f - fTriParm1;
                }
            }
        }
        else {
            if (fTriParm0 + v17 <= 1.0) {
                if (fTriParm0 < 0.0f) {
                    if (v17 < 0.0) {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge0;
                        fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm0);
                        fTmpTriParm1 = 0.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                        kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                        fTmpSegParm = 1.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                    }
                    else {
                        kTriSeg.m_kOrigin = kTri.m_kOrigin;
                        kTriSeg.m_kDirection = kTri.m_kEdge1;
                        fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                        fTriParm0 = 0.0f;
                        kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                        fTmpSegParm = 1.0f;
                        if (fMin0 < fMin) {
                            fMin = fMin0;
                            fSegParm = fTmpSegParm;
                            fTriParm0 = fTmpTriParm0;
                            fTriParm1 = fTmpTriParm1;
                        }
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                    kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                    fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 1.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else {
                    kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                    fMin = NiSqrDistance::Compute(kPoint_2, kTri, fTriParm0, fTriParm1);
                    fSegParm = 1.0f;
                }
            }
            else {
                if (fTriParm0 < 0.0f) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge1;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                    kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                    fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 1.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else if (v17 < 0.0) {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin;
                    kTriSeg.m_kDirection = kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm0);
                    fTriParm1 = 0.0f;
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin0 = NiSqrDistance::Compute(kSeg, kTriSeg, fTmpSegParm, fTmpTriParm1);
                    fTmpTriParm0 = 1.0f - fTmpTriParm1;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                    kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                    fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 1.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
                else {
                    kTriSeg.m_kOrigin = kTri.m_kOrigin + kTri.m_kEdge0;
                    kTriSeg.m_kDirection = kTri.m_kEdge1 - kTri.m_kEdge0;
                    fMin = NiSqrDistance::Compute(kSeg, kTriSeg, fSegParm, fTriParm1);
                    fTriParm0 = 1.0f - fTriParm1;
                    kPoint_2 = kSeg.m_kOrigin + kSeg.m_kDirection;
                    fMin0 = NiSqrDistance::Compute(kPoint_2, kTri, fTmpTriParm0, fTmpTriParm1);
                    fTmpSegParm = 1.0f;
                    if (fMin0 < fMin) {
                        fMin = fMin0;
                        fSegParm = fTmpSegParm;
                        fTriParm0 = fTmpTriParm0;
                        fTriParm1 = fTmpTriParm1;
                    }
                }
            }
        }
    }
    return fMin;
}

float NiSqrDistance::Compute(
    const NiPoint3& kPoint,
    const NiBox& kBox,
    float& fBoxParam0,
    float& fBoxParam1,
    float& fBoxParam2
) {
    float v22 = 0.0;
    float v23 = kPoint.x - kBox.m_kCenter.x;
    float v26 = kPoint.y - kBox.m_kCenter.y;
    float v29 = kPoint.z - kBox.m_kCenter.z;
    double v5 = v26;
    double v6 = v23;
    double v7 = v29;
    float v24 = kBox.m_akAxis[0u].z * v29 + (kBox.m_akAxis[0u].x * v23 + kBox.m_akAxis[0u].y * v26);
    double v8 = v24;
    if (-kBox.m_afExtent[0u] > v24) {
        float kPointa = v8 + kBox.m_afExtent[0u];
        v22 = kPointa * kPointa + 0.0;
        v8 = -kBox.m_afExtent[0u];
    }
    else if (kBox.m_afExtent[0u] < v8) {
        float kPointb = v8 - kBox.m_afExtent[0u];
        v22 = kPointb * kPointb + 0.0;
        v8 = kBox.m_afExtent[0u];
    }
    float v27 = kBox.m_akAxis[1].y * v5 + kBox.m_akAxis[1].x * v6 + kBox.m_akAxis[1].z * v7;
    double v10 = v27;
    if (-kBox.m_afExtent[1] > v27) {
        float kPointc = v10 + kBox.m_afExtent[1];
        v22 = kPointc * kPointc + v22;
        v10 = -kBox.m_afExtent[1];
    }
    else if (kBox.m_afExtent[1] < v10) {
        float kPointd = v10 - kBox.m_afExtent[1];
        v22 = kPointd * kPointd + v22;
        v10 = kBox.m_afExtent[1];
    }
    double v13 = v5 * kBox.m_akAxis[2].y;
    double v14 = v10;
    double v16 = v6 * kBox.m_akAxis[2].x + v13;
    double v17 = v8;
    float v30 = v7 * kBox.m_akAxis[2].z + v16;
    double v18 = v30;
    if (-kBox.m_afExtent[2] > v30) {
        float kPointe = v18 + kBox.m_afExtent[2];
        v22 = kPointe * kPointe + v22;
        v18 = -kBox.m_afExtent[2];
    }
    else if (kBox.m_afExtent[2] < v18) {
        float kPointf = v18 - kBox.m_afExtent[2];
        v22 = kPointf * kPointf + v22;
        v18 = kBox.m_afExtent[2];
    }
    fBoxParam0 = v17;
    fBoxParam1 = v14;
    fBoxParam2 = v18;
    return v22;
}

float NiSqrDistance::Compute(
    const NiSegment& kSeg,
    const NiBox& kBox,
    float& fSegParam,
    float& fBoxParam0,
    float& fBoxParam1,
    float& fBoxParam2
) {
    NiPoint3 kDiff;
    kDiff.x = kSeg.m_kOrigin.x - kBox.m_kCenter.x;
    kDiff.y = kSeg.m_kOrigin.y - kBox.m_kCenter.y;
    kDiff.z = kSeg.m_kOrigin.z - kBox.m_kCenter.z;
    float afPnt[3];
    afPnt[0] = kBox.m_akAxis[0].z * kDiff.z + (kBox.m_akAxis[0].x * kDiff.x + kBox.m_akAxis[0].y * kDiff.y);
    afPnt[1] = kBox.m_akAxis[1].y * kDiff.y + kBox.m_akAxis[1].x * kDiff.x + kBox.m_akAxis[1].z * kDiff.z;
    afPnt[2] = kDiff.x * kBox.m_akAxis[2].x + kDiff.y * kBox.m_akAxis[2].y + kDiff.z * kBox.m_akAxis[2].z;
    float afDir[3];
    afDir[0] = kBox.m_akAxis[0].y * kSeg.m_kDirection.y + kBox.m_akAxis[0].x * kSeg.m_kDirection.x +
        kBox.m_akAxis[0].z * kSeg.m_kDirection.z;
    afDir[1] = kBox.m_akAxis[1].y * kSeg.m_kDirection.y + kBox.m_akAxis[1].x * kSeg.m_kDirection.x +
        kBox.m_akAxis[1].z * kSeg.m_kDirection.z;
    afDir[2] = kBox.m_akAxis[2].y * kSeg.m_kDirection.y + kBox.m_akAxis[2].x * kSeg.m_kDirection.x +
        kBox.m_akAxis[2].z * kSeg.m_kDirection.z;

    // Reflect the coordinate system so that the direction is non-negative along each axis.
    bool abReflect[3];
    for (int32_t i = 0; i < 3; i++) {
        if (!(afDir[i] < 0.0f)) {
            abReflect[i] = false;
        }
        else {
            abReflect[i] = true;
            afPnt[i] = -afPnt[i];
            afDir[i] = -afDir[i];
        }
    }

    float fSqrDistance = 0.0f;
    fSegParam = 0.0f;
    if (afDir[0] > 0.0f) {
        if (afDir[1] > 0.0f) {
            if (afDir[2] > 0.0f) {
                CaseNoZeros(&kBox, &fSqrDistance, afPnt, afDir, &fSegParam);
            }
            else {
                Case0(1, 0, &kBox, 2, &fSegParam, &fSqrDistance, afPnt, afDir);
            }
        }
        else {
            if (afDir[2] > 0.0f) {
                Case0(2, 0, &kBox, 1, &fSegParam, &fSqrDistance, afPnt, afDir);
            }
            else {
                CaseTwoZeros(afPnt, &kBox, 0, afDir, &fSegParam, &fSqrDistance, 2, 1);
            }
        }
    }
    else {
        if (afDir[1] > 0.0f) {
            if (afDir[2] > 0.0f) {
                Case0(2, 1, &kBox, 0, &fSegParam, &fSqrDistance, afPnt, afDir);
            }
            else {
                CaseTwoZeros(afPnt, &kBox, 1, afDir, &fSegParam, &fSqrDistance, 2, 0);
            }
        }
        else {
            if (afDir[2] > 0.0f) {
                CaseTwoZeros(afPnt, &kBox, 2, afDir, &fSegParam, &fSqrDistance, 1, 0);
            }
            else {
                Case000(afPnt, &kBox, &fSqrDistance);
            }
        }
    }

    // Compares with a double 0.0 and a float 1.0.
    if (!(fSegParam >= 0.0)) {
        fSegParam = 0.0f;
        return NiSqrDistance::Compute(kSeg.m_kOrigin, kBox, fBoxParam0, fBoxParam1, fBoxParam2);
    }
    if (!(fSegParam <= 1.0f)) {
        fSegParam = 1.0f;
        NiPoint3 kEnd;
        kEnd.x = kSeg.m_kOrigin.x + kSeg.m_kDirection.x;
        kEnd.y = kSeg.m_kDirection.y + kSeg.m_kOrigin.y;
        kEnd.z = kSeg.m_kDirection.z + kSeg.m_kOrigin.z;
        return NiSqrDistance::Compute(kEnd, kBox, fBoxParam0, fBoxParam1, fBoxParam2);
    }

    // Undo the reflections.
    if (abReflect[0]) {
        afPnt[0] = -afPnt[0];
    }
    if (abReflect[1]) {
        afPnt[1] = -afPnt[1];
    }
    if (abReflect[2]) {
        afPnt[2] = -afPnt[2];
    }
    fBoxParam0 = afPnt[0];
    fBoxParam1 = afPnt[1];
    fBoxParam2 = afPnt[2];
    return fSqrDistance;
}

float NiSqrDistance::Compute(const NiPoint3& kPnt, const NiTrigon& kTri, float& fTriParam0, float& fTriParam1) {
    float v27 = kTri.m_kOrigin.x - kPnt.x;
    float v28 = kTri.m_kOrigin.y - kPnt.y;
    float v29 = kTri.m_kOrigin.z - kPnt.z;
    float v24 = kTri.m_kEdge0.y * kTri.m_kEdge0.y + kTri.m_kEdge0.x * kTri.m_kEdge0.x +
        kTri.m_kEdge0.z * kTri.m_kEdge0.z;
    float kPntk = kTri.m_kEdge1.y * kTri.m_kEdge0.y + kTri.m_kEdge1.x * kTri.m_kEdge0.x +
        kTri.m_kEdge1.z * kTri.m_kEdge0.z;
    float v23 = kTri.m_kEdge1.y * kTri.m_kEdge1.y + kTri.m_kEdge1.x * kTri.m_kEdge1.x +
        kTri.m_kEdge1.z * kTri.m_kEdge1.z;
    float v26 = kTri.m_kEdge0.z * v29 + (kTri.m_kEdge0.x * v27 + kTri.m_kEdge0.y * v28);
    float v25 = kTri.m_kEdge1.y * v28 + kTri.m_kEdge1.x * v27 + kTri.m_kEdge1.z * v29;
    float kTria = v29 * v29 + (v28 * v28 + v27 * v27);
    double v4 = kPntk;
    double v5 = v23;
    double v7 = v24;
    float kPntl = v23 * v24 - kPntk * kPntk;
    float kPnta = std::fabs(static_cast<double>(kPntl));
    double v8 = v25;
    double v9 = v26;
    fTriParam0 = v25 * v4 - v26 * v23;
    float fTriParam0d = v26 * v4 - v25 * v24;
    double v10 = fTriParam0d;
    fTriParam1 = fTriParam0d;
    double v11 = fTriParam0d + fTriParam0;
    // A single compare of fDet with s + t decides the region: below is s + t > fDet.
    bool v13 = !(kPnta >= v11);
    bool v17 = !(fTriParam0 <= 0.0f);
    bool v18 = 0.0f == fTriParam0;
    if (v13) {
        if (v17 || v18) {
            if (!(v10 < 0.0)) {
                float fTriParam0c = v8 + v5 - (v9 + v4);
                if (!(fTriParam0c <= 0.0f)) {
                    float kPntj = v7 - v4 * 2.0 + v5;
                    if (!(kPntj <= static_cast<double>(fTriParam0c))) {
                        float kPntv = fTriParam0c / kPntj;
                        fTriParam0 = kPntv;
                        fTriParam1 = 1.0 - kPntv;
                        kTria = 2.0 * v8 + v5 + kTria - fTriParam0c * fTriParam0;
                    }
                    else {
                        fTriParam0 = 1.0f;
                        fTriParam1 = 0.0f;
                        kTria = 2.0 * v9 + v7 + kTria;
                    }
                    return std::fabs(static_cast<double>(kTria));
                }
                fTriParam0 = 0.0f;
                fTriParam1 = 1.0f;
                float kTrij = v5 + (v8 + v8) + kTria;
                return std::fabs(static_cast<double>(kTrij));
            }

            float kPnth = v8 + v4;
            float fTriParam0f = v9 + v7;
            double v21 = fTriParam0f;
            if (kPnth < static_cast<double>(fTriParam0f)) {
                float fTriParam0b = v21 - kPnth;
                float kPnti = v7 - v4 * 2.0 + v5;
                if (!(kPnti <= static_cast<double>(fTriParam0b))) {
                    float kPntt = fTriParam0b / kPnti;
                    fTriParam1 = kPntt;
                    fTriParam0 = 1.0 - kPntt;
                    kTria = v7 + 2.0 * v9 + kTria - fTriParam0b * fTriParam1;
                }
                else {
                    fTriParam1 = 1.0f;
                    fTriParam0 = 0.0f;
                    kTria = 2.0 * v8 + v5 + kTria;
                }
                return std::fabs(static_cast<double>(kTria));
            }
            fTriParam1 = 0.0f;
            if (!(v21 <= 0.0)) {
                if (!(v9 >= 0.0)) {
                    float kPntu = -v9 / v7;
                    fTriParam0 = kPntu;
                    kTria = v9 * kPntu + kTria;
                }
                else {
                    fTriParam0 = 0.0f;
                }
                return std::fabs(static_cast<double>(kTria));
            }
            fTriParam0 = 1.0f;
            float kTrih = v7 + (v9 + v9) + kTria;
            return std::fabs(static_cast<double>(kTrih));
        }

        float kPntf = v9 + v4;
        float fTriParam0e = v8 + v5;
        double v20 = fTriParam0e;
        if (kPntf < static_cast<double>(fTriParam0e)) {
            float fTriParam0a = v20 - kPntf;
            float kPntg = v7 - v4 * 2.0 + v5;
            if (!(kPntg <= static_cast<double>(fTriParam0a))) {
                float kPntr = fTriParam0a / kPntg;
                fTriParam0 = kPntr;
                fTriParam1 = 1.0 - kPntr;
                kTria = 2.0 * v8 + v5 + kTria - fTriParam0a * fTriParam0;
            }
            else {
                fTriParam0 = 1.0f;
                fTriParam1 = 0.0f;
                kTria = 2.0 * v9 + v7 + kTria;
            }
            return std::fabs(static_cast<double>(kTria));
        }
        fTriParam0 = 0.0f;
        if (!(v20 <= 0.0)) {
            if (!(v8 >= 0.0)) {
                float kPnts = -v8 / v23;
                fTriParam1 = kPnts;
                kTria = v8 * kPnts + kTria;
            }
            else {
                fTriParam1 = 0.0f;
            }
            return std::fabs(static_cast<double>(kTria));
        }
        fTriParam1 = 1.0f;
        float kTrif = v5 + (v8 + v8) + kTria;
        return std::fabs(static_cast<double>(kTrif));
    }

    if (v17 || v18) {
        if (!(v10 < 0.0)) {
            float fInvDet = 1.0 / kPnta;
            fTriParam0 = fTriParam0 * fInvDet;
            float kPntq = fInvDet * fTriParam1;
            fTriParam1 = kPntq;
            float kTrid = (v4 * fTriParam1 + v7 * fTriParam0 + 2.0 * v9) * fTriParam0 +
                (v8 * 2.0 + (v5 * kPntq + fTriParam0 * v4)) * kPntq + kTria;
            return std::fabs(static_cast<double>(kTrid));
        }

        fTriParam1 = 0.0f;
        if (!(v9 >= 0.0)) {
            float kPnte = -v9;
            if (!(kPnte >= v7)) {
                float kPntp = kPnte / v7;
                fTriParam0 = kPntp;
                kTria = v9 * kPntp + kTria;
            }
            else {
                fTriParam0 = 1.0f;
                kTria = v7 + (v9 + v9) + kTria;
            }
            return std::fabs(static_cast<double>(kTria));
        }
        fTriParam0 = 0.0f;
        return std::fabs(static_cast<double>(kTria));
    }

    if (!(v10 < 0.0)) {
        fTriParam0 = 0.0f;
        if (!(v8 >= 0.0)) {
            float kPntd = -v8;
            if (!(kPntd >= v5)) {
                float kPnto = kPntd / v5;
                fTriParam1 = kPnto;
                kTria = v8 * kPnto + kTria;
            }
            else {
                fTriParam1 = 1.0f;
                kTria = v5 + (v8 + v8) + kTria;
            }
            return std::fabs(static_cast<double>(kTria));
        }
    }
    else {
        if (v9 < 0.0) {
            fTriParam1 = 0.0f;
            float kPntb = -v9;
            if (!(kPntb >= v7)) {
                float kPntm = kPntb / v7;
                fTriParam0 = kPntm;
                kTria = v9 * kPntm + kTria;
            }
            else {
                fTriParam0 = 1.0f;
                kTria = v7 + (v9 + v9) + kTria;
            }
            return std::fabs(static_cast<double>(kTria));
        }
        fTriParam0 = 0.0f;
        if (!(v8 >= 0.0)) {
            float kPntc = -v8;
            if (!(kPntc >= v5)) {
                float kPntn = kPntc / v5;
                fTriParam1 = kPntn;
                kTria = v8 * kPntn + kTria;
            }
            else {
                fTriParam1 = 1.0f;
                kTria = v5 + (v8 + v8) + kTria;
            }
            return std::fabs(static_cast<double>(kTria));
        }
    }
    fTriParam1 = 0.0f;
    return std::fabs(static_cast<double>(kTria));
}

float NiSqrDistance::Compute(const NiPoint3& kPnt, const NiParallelogram& kPgm, float& fPgmParam0, float& fPgmParam1) {
    NiSegment kSeg;
    kSeg.m_kOrigin.x = kPgm.m_kOrigin.x - kPnt.x;
    kSeg.m_kOrigin.y = kPgm.m_kOrigin.y - kPnt.y;
    kSeg.m_kOrigin.z = kPgm.m_kOrigin.z - kPnt.z;
    float v31 = kPgm.m_kEdge0.y * kPgm.m_kEdge0.y + kPgm.m_kEdge0.x * kPgm.m_kEdge0.x +
        kPgm.m_kEdge0.z * kPgm.m_kEdge0.z;
    float kPntn = kPgm.m_kEdge1.y * kPgm.m_kEdge0.y + kPgm.m_kEdge1.x * kPgm.m_kEdge0.x +
        kPgm.m_kEdge1.z * kPgm.m_kEdge0.z;
    float v23 = kPgm.m_kEdge1.y * kPgm.m_kEdge1.y + kPgm.m_kEdge1.x * kPgm.m_kEdge1.x +
        kPgm.m_kEdge1.z * kPgm.m_kEdge1.z;
    double x = kSeg.m_kOrigin.x;
    kSeg.m_kOrigin.x = kPgm.m_kEdge0.z * kSeg.m_kOrigin.z +
        (kPgm.m_kEdge0.x * kSeg.m_kOrigin.x + kPgm.m_kEdge0.y * kSeg.m_kOrigin.y);
    float v32 = kPgm.m_kEdge1.y * kSeg.m_kOrigin.y + kPgm.m_kEdge1.x * x + kPgm.m_kEdge1.z * kSeg.m_kOrigin.z;
    float kPgma = kSeg.m_kOrigin.z * kSeg.m_kOrigin.z + (kSeg.m_kOrigin.y * kSeg.m_kOrigin.y + x * x);
    double v7 = kPntn;
    double v8 = v23;
    double v9 = v31;
    float kPnto = v23 * v31 - kPntn * kPntn;
    float kPnta = std::fabs(static_cast<double>(kPnto));
    if (!(kPnta >= 1e-05f)) {
        kSeg.m_kOrigin = kPgm.m_kOrigin;
        if (!(v9 >= v8)) {
            kSeg.m_kDirection.x = kPgm.m_kEdge1.x;
            float y = kPgm.m_kEdge1.y;
            float z = kPgm.m_kEdge1.z;
            kSeg.m_kDirection.y = y;
            kSeg.m_kDirection.z = z;
            fPgmParam0 = 0.0f;
            return NiSqrDistance::Compute(kPnt, kSeg, fPgmParam1);
        }

        kSeg.m_kDirection.x = kPgm.m_kEdge0.x;
        float v18 = kPgm.m_kEdge0.y;
        float v19 = kPgm.m_kEdge0.z;
        kSeg.m_kDirection.y = v18;
        kSeg.m_kDirection.z = v19;
        fPgmParam1 = 0.0f;
        return NiSqrDistance::Compute(kPnt, kSeg, fPgmParam0);
    }

    double v10 = v32;
    double v11 = kSeg.m_kOrigin.x;
    fPgmParam0 = v32 * v7 - kSeg.m_kOrigin.x * v8;
    float v24 = v11 * v7 - v32 * v9;
    double v12 = v24;
    fPgmParam1 = v24;
    if (!(fPgmParam0 < 0.0f)) {
        double v16 = v24;
        if (!(kPnta >= static_cast<double>(fPgmParam0))) {
            if (!(v24 < 0.0f)) {
                if (!(kPnta >= v16)) {
                    // d + b is kept as a double and converted to float below.
                    double dDPlusB = v11 + v7;
                    float v29 = dDPlusB;
                    float kPntl = -v29;
                    if (kPntl < v9) {
                        fPgmParam1 = 1.0f;
                        if (!(v29 >= 0.0f)) {
                            float kPntbb = kPntl / v9;
                            fPgmParam0 = kPntbb;
                            kPgma = v29 * kPntbb + v8 + (v10 + v10) + kPgma;
                        }
                        else {
                            fPgmParam0 = 0.0f;
                            kPgma = v8 + (v10 + v10) + kPgma;
                        }
                        return std::fabs(static_cast<double>(kPgma));
                    }
                    fPgmParam0 = 1.0f;
                    float v30 = v7 + v10;
                    if (!(v30 >= 0.0f)) {
                        float kPntm = -v30;
                        if (!(kPntm >= v8)) {
                            float kPntbc = kPntm / v8;
                            fPgmParam1 = kPntbc;
                            kPgma = v9 + v30 * kPntbc + (v11 + v11) + kPgma;
                        }
                        else {
                            fPgmParam1 = 1.0f;
                            kPgma = v9 + v8 + kPgma + (v10 + dDPlusB + (v10 + dDPlusB));
                        }
                        return std::fabs(static_cast<double>(kPgma));
                    }
                }
                else {
                    fPgmParam0 = 1.0f;
                    float v28 = v10 + v7;
                    if (!(v28 >= 0.0f)) {
                        float kPntk = -v28;
                        if (!(kPntk >= v8)) {
                            float kPntba = kPntk / v8;
                            fPgmParam1 = kPntba;
                            kPgma = v9 + v28 * kPntba + (v11 + v11) + kPgma;
                        }
                        else {
                            fPgmParam1 = 1.0f;
                            kPgma = v7 + v11 + v10 + (v7 + v11 + v10) + (v9 + v8 + kPgma);
                        }
                        return std::fabs(static_cast<double>(kPgma));
                    }
                }
            }
            else {
                float kPnti = -v11;
                if (kPnti < v9) {
                    fPgmParam1 = 0.0f;
                    if (!(v11 >= 0.0)) {
                        float kPnty = kPnti / v9;
                        fPgmParam0 = kPnty;
                        kPgma = v11 * kPnty + kPgma;
                    }
                    else {
                        fPgmParam0 = 0.0f;
                    }
                    return std::fabs(static_cast<double>(kPgma));
                }
                fPgmParam0 = 1.0f;
                float v27 = v10 + v7;
                if (!(v27 >= 0.0f)) {
                    float kPntj = -v27;
                    if (!(kPntj >= v8)) {
                        float kPntz = kPntj / v8;
                        fPgmParam1 = kPntz;
                        kPgma = v9 + v27 * kPntz + (v11 + v11) + kPgma;
                    }
                    else {
                        fPgmParam1 = 1.0f;
                        kPgma = v7 + v11 + v10 + (v7 + v11 + v10) + (v9 + v8 + kPgma);
                    }
                    return std::fabs(static_cast<double>(kPgma));
                }
            }
            fPgmParam1 = 0.0f;
            float kPgmh = v9 + (v11 + v11) + kPgma;
            return std::fabs(static_cast<double>(kPgmh));
        }

        if (!(v24 < 0.0f)) {
            if (!(kPnta >= v16)) {
                fPgmParam1 = 1.0f;
                double v17 = v7 + v11;
                float v26 = v17;
                if (!(v26 >= 0.0f)) {
                    float kPnth = -v26;
                    if (!(kPnth >= v9)) {
                        float kPntx = kPnth / v9;
                        fPgmParam0 = kPntx;
                        kPgma = v26 * kPntx + v8 + (v10 + v10) + kPgma;
                    }
                    else {
                        fPgmParam0 = 1.0f;
                        kPgma = v9 + v8 + kPgma + (v17 + v10 + (v17 + v10));
                    }
                    return std::fabs(static_cast<double>(kPgma));
                }
                fPgmParam0 = 0.0f;
                float kPgmf = v8 + (v10 + v10) + kPgma;
                return std::fabs(static_cast<double>(kPgmf));
            }

            float kPntv = 1.0 / kPnta;
            fPgmParam0 = fPgmParam0 * kPntv;
            float kPntw = kPntv * fPgmParam1;
            fPgmParam1 = kPntw;
            float kPgmd = (v7 * fPgmParam1 + v9 * fPgmParam0 + 2.0 * v11) * fPgmParam0 +
                (v10 * 2.0 + (v8 * kPntw + fPgmParam0 * v7)) * kPntw + kPgma;
            return std::fabs(static_cast<double>(kPgmd));
        }

        fPgmParam1 = 0.0f;
        if (!(v11 >= 0.0)) {
            float kPntg = -v11;
            if (!(kPntg >= v9)) {
                float kPntu = kPntg / v9;
                fPgmParam0 = kPntu;
                kPgma = v11 * kPntu + kPgma;
            }
            else {
                fPgmParam0 = 1.0f;
                kPgma = v9 + (v11 + v11) + kPgma;
            }
            return std::fabs(static_cast<double>(kPgma));
        }
        fPgmParam0 = 0.0f;
        return std::fabs(static_cast<double>(kPgma));
    }

    if (!(v12 < 0.0)) {
        if (!(kPnta >= v12)) {
            double v13 = v7 + v11;
            float v25 = v13;
            double v14 = v25;
            if (v25 < 0.0f) {
                fPgmParam1 = 1.0f;
                float kPnte = -v14;
                if (!(kPnte >= v9)) {
                    float kPnts = kPnte / v9;
                    fPgmParam0 = kPnts;
                    kPgma = v14 * kPnts + v8 + (v10 + v10) + kPgma;
                }
                else {
                    fPgmParam0 = 1.0f;
                    kPgma = v9 + v8 + kPgma + (v13 + v10 + (v13 + v10));
                }
                return std::fabs(static_cast<double>(kPgma));
            }
            fPgmParam0 = 0.0f;
            if (!(v10 >= 0.0)) {
                float kPntf = -v10;
                if (!(kPntf >= v8)) {
                    float kPntt = kPntf / v8;
                    fPgmParam1 = kPntt;
                    kPgma = v10 * kPntt + kPgma;
                }
                else {
                    fPgmParam1 = 1.0f;
                    kPgma = v8 + (v10 + v10) + kPgma;
                }
                return std::fabs(static_cast<double>(kPgma));
            }
        }
        else {
            fPgmParam0 = 0.0f;
            if (!(v10 >= 0.0)) {
                float kPntd = -v10;
                if (!(kPntd >= v8)) {
                    float kPntr = kPntd / v8;
                    fPgmParam1 = kPntr;
                    kPgma = v10 * kPntr + kPgma;
                }
                else {
                    fPgmParam1 = 1.0f;
                    kPgma = v8 + (v10 + v10) + kPgma;
                }
                return std::fabs(static_cast<double>(kPgma));
            }
        }
    }
    else {
        if (v11 < 0.0) {
            fPgmParam1 = 0.0f;
            float kPntb = -v11;
            if (!(kPntb >= v9)) {
                float kPntp = kPntb / v9;
                fPgmParam0 = kPntp;
                kPgma = v11 * kPntp + kPgma;
            }
            else {
                fPgmParam0 = 1.0f;
                kPgma = v9 + (v11 + v11) + kPgma;
            }
            return std::fabs(static_cast<double>(kPgma));
        }
        fPgmParam0 = 0.0f;
        if (!(v10 >= 0.0)) {
            float kPntc = -v10;
            if (!(kPntc >= v8)) {
                float kPntq = kPntc / v8;
                fPgmParam1 = kPntq;
                kPgma = v10 * kPntq + kPgma;
            }
            else {
                fPgmParam1 = 1.0f;
                kPgma = v8 + (v10 + v10) + kPgma;
            }
            return std::fabs(static_cast<double>(kPgma));
        }
    }
    fPgmParam1 = 0.0f;
    return std::fabs(static_cast<double>(kPgma));
}
