#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

float* MultiplyMatrixVector(float* a1, float* a2, float* a3);

// A dense matrix of T with compile-time dimensions, stored row by row (the FaceGen SDK's fixed-size coordinate
// matrices). The PDB's class also has operator[], operator*/-/+/-=/+=/-=/==, string, numRows, numCols, setZero,
// setOne, setIdentity and transpose; none of them is reconstructed yet.
template <class T, int32_t NR, int32_t NC> class FR2MatrixCTC {
public:
	// The PDB's data: the NR x NC elements, row by row.
	T data[NR][NC];
};

// The macro argument's comma needs the alias.
using FR2MatrixCTC_float_2_2 = FR2MatrixCTC<float, 2, 2>;
