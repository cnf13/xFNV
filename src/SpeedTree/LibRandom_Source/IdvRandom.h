#pragma once

#include "SpeedTree/Random.h"

// SpeedTree's random number generator: an empty object over a shared generator state (seeded once by the
// constructor through Reseed).
class CIdvRandom {
public:
    CIdvRandom();
    ~CIdvRandom();

    // A uniform value between fMin and fMax.
    float GetUniform(float fMin, float fMax);
    // Seeds the shared generator; -1 picks a seed.
    void Reseed(int32_t lSeed);
};

// CIdvRandom's shared generators: the uniform and normal distributions and whether they have been seeded.
struct SIdvRandomImpl {
    static Uniform m_cUniform;
    static Normal m_cNormal;
    static inline bool m_bInit;
};
