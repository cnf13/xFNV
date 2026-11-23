#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"

#include <cstdint>

// The sum is kept in higher precision and rounded to float once.
float NiInterpScalar::Linear(float fTime, float fP, float fQ) {
    return static_cast<float>((fTime * fQ) + ((1.0 - fTime) * fP));
}

float NiInterpScalar::Bezier(float fTime, float fP, float fDP, float fQ, float fDQ) {
    double v5 = fQ - fP;
    float fB = fDP + fDQ - v5 * 2.0;
    double v6 = fB * fTime;
    float fA = v5 * 3.0 - (fDQ + 2.0 * fDP);
    return fTime * (fDP + (v6 + fA) * fTime) + fP;
}

float* NiInterpScalar::AdjustBezier(
    float fLastValue,
    float fLastTime,
    float* pfLastOut,
    float fNextValue,
    float fNextTime,
    float* pfNextIn,
    float fNewTime,
    float fNewValue,
    float* pfNewIn,
    float* pfNewOut
) {
    float v15 = fNextValue - fLastValue;
    float v16 = fNextTime - fLastTime;
    float v19 = fNewTime - fLastTime;
    float v22 = fNextTime - fNewTime;
    double v11 = v19;
    double v12 = v19 / v16;
    double v13 = v16;
    float v20 = static_cast<float>(v12);
    float v17 = static_cast<float>(((*pfLastOut) - (v15 * 2.0)) + (*pfNextIn));
    double v14 = (v17 * 3.0) * v20;
    float v18 = static_cast<float>(((v15 * 3.0) - ((*pfLastOut) * 2.0)) - (*pfNextIn));
    float v21 = static_cast<float>(((((2.0 * v18) + v14) * v20) + (*pfLastOut)) / v13);
    *pfNewIn = v21;
    *pfNewOut = (v21 * v22);
    *pfNewIn = static_cast<float>(v11 * (*pfNewIn));
    *pfLastOut = static_cast<float>(v12 * (*pfLastOut));
    *pfNextIn = static_cast<float>((v22 / v13) * (*pfNextIn));
    return pfLastOut;
}
