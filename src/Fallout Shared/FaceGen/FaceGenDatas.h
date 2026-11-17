#pragma once

#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Reconstruction.hpp"

// The four coordinate matrices FaceGen builds a head from. IDA name; not in the PDB.
class FaceGenDatas {
public:
	// IDA name; not in the PDB. FR2MatrixVTC<float> is 0x20 bytes and the exe reads each element's nrows at +0x18
	// and ncols at +0x1C (FanGenCtlsGrpC::geneticRandom, 0xAAD6B0); BipedAnim casts the struct to
	// FR2MatrixVTC<float>[2].
	FR2MatrixVTC<float> kFaceGens[2][2];
};
