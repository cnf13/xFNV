#include "BSCore/BSRandom.h"

#include "BSAudio/BSAudioManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSRandom BSRandom::Singleton{};

BSRandom::BSRandom() : rseed(1), rseed_sp(0), mti(625) {}

uint32_t BSRandom::UnsignedInt() {
    return UnsignedInt(0xFFFFFFFFu);
}

uint32_t BSRandom::UnsignedInt(uint32_t aiMax) {
    if (!aiMax) {
        return 0;
    }
    if (mti >= 624) {
        int32_t kk;
        if (mti == 625) {
            uint32_t TickCount = GetTickCount();
            Seed(TickCount);
        }
        for (kk = 0; kk < 227; ++kk) {
            uint32_t v6 = (mt[kk + 1] & 0x7FFFFFFF) | (mt[kk] & 0x80000000u);
            mt[kk] = mag01[v6 & 1] ^ (mt[kk + 397] ^ v6 >> 1);
        }
        while (kk < 623) {
            uint32_t v7 = (mt[kk + 1] & 0x7FFFFFFF) | (mt[kk] & 0x80000000u);
            mt[kk] = mag01[v7 & 1] ^ (mt[kk - 227] ^ v7 >> 1);
            ++kk;
        }
        uint32_t v8 = (mt[0] & 0x7FFFFFFF) | (mt[623] & 0x80000000u);
        mt[623] = mag01[v8 & 1] ^ (mt[396] ^ v8 >> 1);
        mti = 0;
    }
    uint32_t y = mt[mti++];
    if (aiMax == -1 || aiMax == 0x7FFF) {
        return aiMax & y;
    }

    return y % aiMax;
}

float BSRandom::FloatTwoPi() {
    return (float)(((double)UnsignedInt()) * 0x1.921fb60000000p-30);
}

float BSRandom::FloatNeg1To1() {
    return (float)((((double)UnsignedInt()) * 0x1.0000000000000p-31) - 1.0);
}

float BSRandom::Float0To1() {
    return (float)(((double)UnsignedInt()) * 0x1.0000000000000p-32);
}

void BSRandom::Seed(uint32_t aiSeed) {
    mt[0] = (int32_t)aiSeed;
    for (mti = (uint32_t)1; mti < 0x270u; ++mti) {
        mt[mti] = 69069 * mt[mti - 1];
    }
}

void BSRandom::Init(uint32_t aiSeed) {
    rseed_sp = 0;
    mti = (uint32_t)625;
    Seed(aiSeed);
}

void BSRandom::Kill() {}

bool BSRandom::Bool(float afPercentTrue) {
    return (bool)((uint8_t)(afPercentTrue >= Float0To1()));
}

BSRandom* BSRandom::GetSingleton() {
    if ((Singleton_guard & 1) == 0) {
        Singleton_guard |= 1u;
        BSRandom::Singleton.~BSRandom();
        ::new (&BSRandom::Singleton) BSRandom;
    }
    return &BSRandom::Singleton;
}
