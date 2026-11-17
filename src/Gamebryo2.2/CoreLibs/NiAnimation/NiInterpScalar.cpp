#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"

#include <cstdint>

// The sum is kept in higher precision and rounded to float once.
float NiInterpScalar::Linear(float fTime, float fP, float fQ) {
    return (float)((fTime * fQ) + ((1.0 - fTime) * fP));
}

float NiInterpScalar::Bezier(float a1, float a2, float a3, float a4, float a5) {
    double v5 = a4 - a2;
    float a4a = a3 + a5 - v5 * 2.0;
    double v6 = a4a * a1;
    float a4b = v5 * 3.0 - (a5 + 2.0 * a3);
    return a1 * (a3 + (v6 + a4b) * a1) + a2;
}

float* NiInterpScalar::AdjustBezier(
    float a1,
    float a2,
    float* a3,
    float a4,
    float a5,
    float* a6,
    float a7,
    float a8,
    float* a9,
    float* a10
) {
    float v15 = a4 - a1;
    float v16 = a5 - a2;
    float v19 = a7 - a2;
    float v22 = a5 - a7;
    double v11 = (double)v19;
    double v12 = (double)(v19 / v16);
    double v13 = (double)v16;
    float v20 = (float)v12;
    float v17 = (float)(((*a3) - (v15 * 2.0)) + (*a6));
    double v14 = (v17 * 3.0) * v20;
    float v18 = (float)(((v15 * 3.0) - ((*a3) * 2.0)) - (*a6));
    float v21 = (float)(((((2.0 * v18) + v14) * v20) + (*a3)) / v13);
    *a9 = v21;
    *a10 = (v21 * v22);
    *a9 = (float)(v11 * (*a9));
    *a3 = (float)(v12 * (*a3));
    *a6 = (float)((v22 / v13) * (*a6));
    return a3;
}
