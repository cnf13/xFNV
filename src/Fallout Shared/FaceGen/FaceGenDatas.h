#pragma once

#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"

// The four coordinate matrices FaceGen builds a head from.
class FaceGenDatas {
public:
	// BipedAnim casts the struct to FR2MatrixVTC<float>[2].
	FR2MatrixVTC<float> kFaceGens[2][2];
};
