#pragma once

#include <cstdint>

// One-dimensional smooth noise.
class BSNoise1D {
public:
    using fg1_t = float[514];
    using iPermutation_t = int32_t[514];

    // The noise at afX.
    static float Noise(float afX);
    // Perlin's quintic fade curve 6t^5 - 15t^4 + 10t^3, rounded to float.
    static float s_curve(float afT);
    // afA + afT * (afB - afA), rounded to float.
    static float lerp(float afT, float afA, float afB);

    static inline float bReady;
    static inline fg1_t fg1{};
    static inline iPermutation_t iPermutation{};
};
