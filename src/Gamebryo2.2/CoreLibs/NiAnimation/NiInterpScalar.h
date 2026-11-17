#pragma once

// The scalar interpolation formulas the float keys use.
class NiInterpScalar {
public:
	// Interpolates from fP at time 0 to fQ at time 1.
	static float Linear(float fTime, float fP, float fQ);
	static float Bezier(float a1, float a2, float a3, float a4, float a5);
	static float* AdjustBezier(float a1, float a2, float* a3, float a4, float a5, float* a6, float a7, float a8,
		float* a9, float* a10);
};
