#include "BSMain/BSNoise1D.h"

#include "SDK/MSVC/stdlib.h"

float BSNoise1D::Noise(float afX) {
    if (BSNoise1D::bReady == 0.0) {
        int32_t i;
        for (i = 0; i < 256; ++i) {
            BSNoise1D::iPermutation[i] = i;
            BSNoise1D::fg1[i] = static_cast<double>(rt::rand() % 512 - 256) / 256.0;
        }
        while (--i) {
            int32_t v4 = BSNoise1D::iPermutation[i];
            int32_t v1 = rt::rand() % 256;
            BSNoise1D::iPermutation[i] = BSNoise1D::iPermutation[v1];
            BSNoise1D::iPermutation[v1] = v4;
        }
        for (int32_t j = 0; j < 258; ++j) {
            BSNoise1D::iPermutation[j + 256] = BSNoise1D::iPermutation[j];
            BSNoise1D::fg1[j + 256] = BSNoise1D::fg1[j];
        }
        BSNoise1D::bReady = 1.0f;
    }
    float t = afX + 4096.0;
    float rx0 = t - static_cast<double>(static_cast<int32_t>(t));
    float rx1 = rx0 - 1.0;
    float sx = BSNoise1D::s_curve(rx0);
    float u = rx0 * BSNoise1D::fg1[BSNoise1D::iPermutation[static_cast<uint8_t>(static_cast<int32_t>(t))]];
    float v = rx1 * BSNoise1D::fg1[BSNoise1D::iPermutation[static_cast<uint8_t>(static_cast<int32_t>(t) + 1)]];
    return BSNoise1D::lerp(sx, u, v);
}

// The polynomial is evaluated in double precision and only the result is rounded to float.
float BSNoise1D::s_curve(float afT) {
    double t = afT;
    float fResult = static_cast<float>(t * t * t * t * t * 6.0 - t * t * t * t * 15.0 + t * t * t * 10.0);
    return fResult;
}

float BSNoise1D::lerp(float afT, float afA, float afB) {
    double fA = afA;
    float fResult = static_cast<float>((afB - fA) * afT + fA);
    return fResult;
}
