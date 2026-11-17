#pragma once

#include <stdint.h>

class hkVector4;

// Havok's linear congruential generator: m_current steps as 1664525 * m_current + 1013904223, and m_seed keeps the
// value it started from.
class hkPseudoRandomGenerator {
public:
    // Four getRandReal01 draws (each m_current / 2^32) into vecOut's x, y, z and w.
    void getRandomVector01(hkVector4& vecOut);
    // Four draws into vecOut's x, y, z and w, each 2 * getRandReal01() - 1.
    void getRandomVector11(hkVector4& vecOut);

    uint32_t m_seed;
    uint32_t m_current;
};
