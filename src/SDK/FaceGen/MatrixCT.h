#pragma once

#include <stdint.h>

// The 2 x 2 matrix apfMatrix (row by row) times the 2-vector apfVector, into apfResult; returns apfResult.
float* MultiplyMatrixVector(float* apfResult, float* apfMatrix, float* apfVector);

// A dense matrix of T with compile-time dimensions, stored row by row (the FaceGen SDK's fixed-size coordinate
// matrices). Only the data is declared; operator[], operator*/-/+/-=/+=/-=/==, string, numRows, numCols, setZero,
// setOne, setIdentity and transpose are not declared.
template <class T, int32_t NR, int32_t NC>
class FR2MatrixCTC {
public:
    // The NR x NC elements, row by row.
    T data[NR][NC];
};

// The macro argument's comma needs the alias.
using FR2MatrixCTC_float_2_2 = FR2MatrixCTC<float, 2, 2>;
