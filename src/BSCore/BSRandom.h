#pragma once

#include <cstdint>

// The game's Mersenne Twister random number generator (624 words of state; rseed_sp is the second seed).
class BSRandom {
public:
    BSRandom();

    // A random 32-bit value: UnsignedInt(0xFFFFFFFF).
    uint32_t UnsignedInt();
    // A random 32-bit value below aiMax.
    uint32_t UnsignedInt(uint32_t aiMax);
    // A random value in [auiMin, auiMax): auiMin plus UnsignedInt(auiMax - auiMin).
    uint32_t UnsignedInt(uint32_t auiMin, uint32_t auiMax);
    // A random float in [afMin, afMax): the 32-bit value over 2^32, scaled in double and rounded to float.
    float Float(float afMin, float afMax);
    // A random angle in [-pi, pi), rounded to float.
    float FloatNegPiToPi();
    float FloatTwoPi();
    float FloatNeg1To1();
    float Float0To1();
    void Seed(uint32_t aiSeed);
    void Init(uint32_t aiSeed);
    void Kill();
    bool Bool(float afPercentTrue);

    // The generator, created on first use.
    static BSRandom* GetSingleton();

    uint32_t rseed;
    int32_t rseed_sp;
    int32_t mt[624];
    uint32_t mti;

    static BSRandom Singleton;
};

inline uint32_t BSRandom::UnsignedInt(uint32_t auiMin, uint32_t auiMax) {
    return auiMin + UnsignedInt(auiMax - auiMin);
}

// Function static of BSRandom::UnsignedInt(uint32_t) (`unsigned long mag01[2]`, 0x0 and MATRIX_A), indexed by the low
// bit.
using mag01_t = uint32_t[2];
inline mag01_t mag01{0x0u, 0x9908B0DFu};

// The BSRandom singleton's UnsignedInt(aiMax): a random number below aiMax.
inline uint32_t RandomUnsignedInt(uint32_t aiMax) {
    return BSRandom::GetSingleton()->UnsignedInt(aiMax);
}

// The range is the double difference of the bounds, and the sum is rounded to a float local before it is returned.
inline float BSRandom::Float(float afMin, float afMax) {
    uint32_t uiValue = UnsignedInt();
    float fResult = static_cast<float>(uiValue * (static_cast<double>(afMax) - afMin) / 4294967296.0 + afMin);
    return fResult;
}

inline float RandomFloat(float afMin, float afMax) {
    return BSRandom::GetSingleton()->Float(afMin, afMax);
}

// A random float in [0, 1] from the BSRandom singleton.
inline float RandomFloat0To1() {
    return BSRandom::GetSingleton()->Float0To1();
}

// A random angle in [0, 2*pi) from the BSRandom singleton.
inline float RandomFloatTwoPi() {
    return BSRandom::GetSingleton()->FloatTwoPi();
}

// A random integer in [aiMin, aiMax].
inline int32_t RandomInt(int32_t aiMin, int32_t aiMax) {
    return static_cast<int32_t>(
        BSRandom::GetSingleton()->UnsignedInt(static_cast<uint32_t>(aiMin), static_cast<uint32_t>(aiMax))
    );
}

inline float RandomFloatNegPiToPi() {
    return BSRandom::GetSingleton()->FloatNegPiToPi();
}

// The double constants are the float two pi over 2^32 and the float pi; the result is rounded to a float local.
inline float BSRandom::FloatNegPiToPi() {
    uint32_t uiValue = UnsignedInt();
    float fResult = static_cast<float>(uiValue * 1.462918119976564e-09 - 3.1415927410125732);
    return fResult;
}

inline float RandomFloatNeg1To1() {
    return BSRandom::GetSingleton()->FloatNeg1To1();
}

// BSRandom's Bool with afPercentTrue.
inline bool RandomBool(float afPercentTrue) {
    return BSRandom::GetSingleton()->Bool(afPercentTrue);
}

// The next unsigned random number of the BSRandom singleton.
// The next unsigned random number of the BSRandom singleton.
inline uint32_t Random() {
    return BSRandom::GetSingleton()->UnsignedInt();
}
