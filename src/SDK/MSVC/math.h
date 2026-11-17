#pragma once

#include "Reconstruction.hpp"
#include <cmath>
#include <cstdint>

// The one VC9 <math.h> body the exe compiled out of line that the standard library does not have: the math
// functions themselves are std:: (<cmath>).

// VC9's _Pow_int for float, which its float overload pow(float, int)
// called: afX raised to aiY by repeated squaring, the reciprocal for a negative aiY.
// Inferred type: a1: float from 0x685d2e (parameter 0 of pow(float, int)); a2: int32_t: signed uses from 0x688f1a
double _Pow_int(float a1, int32_t a2);
