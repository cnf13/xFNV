#pragma once

#include "SDK/Windows/winbase.h"

#include <cmath>
#include <cstdint>

// The larger of two ints: aiB unless aiA is above it. The compare is signed.
inline int32_t BSMAX(int32_t aiA, int32_t aiB) {
    if (aiA <= aiB) {
        return aiB;
    }

    return aiA;
}

// Compare-and-swap: stores aiValue in *apAddress when it equals aiCompare and returns the old value.
inline int32_t BSAtomicCompareExchange(volatile int32_t* apAddress, int32_t aiCompare, int32_t aiValue) {
    return InterlockedCompareExchange32(apAddress, aiValue, aiCompare);
}

// Overload for a volatile unsigned word, used for BSSpinLock's owner.
// InterlockedCompareExchange takes a LONG, so the cast reads the same unsigned word as signed.
inline uint32_t BSAtomicCompareExchange(volatile uint32_t* apAddress, uint32_t aiCompare, uint32_t aiValue) {
    return static_cast<uint32_t>(InterlockedCompareExchange32(
        reinterpret_cast<volatile int32_t*>(apAddress),
        static_cast<int32_t>(aiValue),
        static_cast<int32_t>(aiCompare)
    ));
}

// The overload for a node pointer word: CompareAndSwapEx's
// pointer-width form passes the lock-free queues' and maps' links to the same function.
inline void* BSAtomicCompareExchange(void* volatile* apAddress, void* apCompare, void* apValue) {
    return InterlockedCompareExchangePtr(apAddress, apValue, apCompare);
}

inline uint32_t BSMAX(uint32_t iA, uint16_t iB) {
    if (iA <= iB) {
        return iB;
    }

    return iA;
}

inline char BSMIN(char iA, char iB) {
    if (iA >= iB) {
        return iB;
    }

    return iA;
}

// The larger of two floats: afB unless it is below afA, so afB is returned for a tie and for a NaN.
inline float BSMAX(float afA, float afB) {
    if (afB < afA) {
        return afA;
    }
    return afB;
}

inline double BSMAX_0(double adA, float afB) {
    if (afB < adA) {
        return adA;
    }

    return afB;
}

// The smaller of two floats: afB only when it is below afA (a tie returns afA and a NaN returns afB).
inline float BSMIN(float afA, float afB) {
    if (!(afB >= afA)) {
        return afB;
    }
    return afA;
}

// The larger of two unsigned ints.
inline uint32_t BSMAX(uint32_t auiA, uint32_t auiB) {
    if (auiA <= auiB) {
        return auiB;
    }
    return auiA;
}

inline int32_t BSMIN(int32_t aiA, int32_t aiB) {
    if (aiA >= aiB) {
        return aiB;
    }
    return aiA;
}

inline uint32_t BSMIN(uint32_t auiA, uint32_t auiB) {
    if (auiA >= auiB) {
        return auiB;
    }
    return auiA;
}

// Wraps aVal into [aMin, aMax].
inline void BSWrap(float& aVal, float aMin, float aMax) {
    if (!(aMax < static_cast<double>(aMin))) {
        if (!(aMax < static_cast<double>(aVal))) {
            if (aMin > static_cast<double>(aVal)) {
                float v4 = aMax - aMin;
                float v3 = aVal - aMin;
                aVal = std::fmodf(v3, v4) + aMax;
            }
        }
        else {
            float v6 = aMax - aMin;
            float v5 = aVal - aMin;
            aVal = std::fmodf(v5, v6) + aMin;
        }
    }
}

inline uint32_t BSMIN(uint16_t a, uint32_t b) {
    if (a < b) {
        return a;
    }

    return b;
}

inline int32_t BSAbs(int32_t alValue) {
    return std::abs(alValue);
}

// afVal rounded to the nearest multiple of afNearest.
inline float BSRoundToNearest(float afVal, float afNearest) {
    float fQuotient = afVal / afNearest;
    return static_cast<float>(
        static_cast<double>(
            !(fQuotient - static_cast<double>(static_cast<int32_t>(fQuotient)) < 0.5) + static_cast<int32_t>(fQuotient)
        ) *
        afNearest
    );
}

// Clamps aVal into [aMin, aMax]: aMax when aMax < aVal, else aMin when aVal < aMin (the float instantiation).
inline void BSClamp(float& aVal, float aMin, float aMax) {
    if (aMax < static_cast<double>(aVal)) {
        aVal = aMax;
    }
    else if (aMin > static_cast<double>(aVal)) {
        aVal = aMin;
    }
}

template <class T>
void BSClamp(T& aVal, T aMin, T aMax) {
    if (aVal > aMax) {
        aVal = aMax;
    }
    else if (aVal < aMin) {
        aVal = aMin;
    }
}

inline uint8_t BSMIN(uint8_t acA, uint8_t acB) {
    if (acA >= acB) {
        return acB;
    }

    return acA;
}
