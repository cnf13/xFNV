#pragma once

#include "Reconstruction.hpp"

// The timing of a screen splatter: how old it is, how long it lasts, its alpha and how many splatters it makes.
struct BSSplatterData {
	float fAge;
	float fDuration;
	float fAlpha;
	uint32_t uiCount;
};
