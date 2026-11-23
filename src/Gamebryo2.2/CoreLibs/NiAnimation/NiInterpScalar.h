#pragma once

// The scalar interpolation formulas the float keys use.
class NiInterpScalar {
public:
    // Interpolates from fP at time 0 to fQ at time 1.
    static float Linear(float fTime, float fP, float fQ);
    static float Bezier(float fTime, float fP, float fDP, float fQ, float fDQ);
    static float* AdjustBezier(
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
    );
};
