#pragma once

#include <stdint.h>

template <class T>
class FR2Vect3TC;

// The 12-byte FR2Vect3TC<int>: a .tri triangle's three vertex indices (FffTriFileDataS::triList and txtTriList).
template <>
class FR2Vect3TC<int> {
public:
    int32_t x1;
    int32_t x2;
    int32_t x3;
};

// The 6-byte FR2Vect3TC<short>: the diff morphs' vertex deltas and FffModeGS's vertex offsets.
template <>
class FR2Vect3TC<int16_t> {
public:
    int16_t x1;
    int16_t x2;
    int16_t x3;
};

// The 12-byte FR2Vect3TC<float>: the .tri reader's vertices. The int instance has the same shape.
template <>
class FR2Vect3TC<float> {
public:
    // The element-wise sum with v1, built in apResult.
    FR2Vect3TC* Add(FR2Vect3TC* apResult, const FR2Vect3TC& v1) const;
    // Multiplies each element by val and builds the result in apResult.
    FR2Vect3TC* MulScalar(FR2Vect3TC* apResult, float val) const;

    float x1;
    float x2;
    float x3;
};
