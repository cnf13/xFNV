#pragma once

#include "Reconstruction.hpp"

// The scalar interpolation formulas the float keys use.
class NiInterpScalar {
public:
	// fP at time 0 to fQ at time 1 (cdecl, ST0).
	static float Linear(float fTime, float fP, float fQ);
	// Inferred type: a1: float: used as one at 0xa420a4; a2: float: used as one at 0xa42078; a3: float: used as one at
	// 0xa42080; a4: float: used as one at 0xa42070; a5: float: used as one at 0xa42086
	static float Bezier(float a1, float a2, float a3, float a4, float a5);
	// Inferred type: a1: float: used as one at 0xa420e9; a2: float: used as one at 0xa42108; a4: float: used as one at
	// 0xa420e1; a5: float: used as one at 0xa42102; a7: float: used as one at 0xa42116. a8: float: NiBezPosKey::Insert
	// (0xA282A0) passes the new key's position component there with fstp.
	static float* AdjustBezier(float a1, float a2, float* a3, float a4, float a5, float* a6, float a7, float a8,
		float* a9, float* a10);
};
