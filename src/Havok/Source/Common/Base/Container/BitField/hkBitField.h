#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// A packed array of bits: m_words holds them 32 to a word, m_numBits is how many of them the field has.
class hkBitField {
public:
    int32_t fillUnusedBits(int32_t fillValue);
    void setSize(int32_t numBits, int32_t fillValue);

    // Bit index: 1 when it is set, else 0.
    int32_t get(int32_t index) const {
        return static_cast<int32_t>((m_words[index >> 5] >> (index & 0x1f)) & 1);
    }

    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    static int32_t countBits(const uint32_t* words, int32_t numWords);

    static const hkClass& staticClass();

    hkArray<uint32_t> m_words;
    int32_t m_numBits;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkBitFieldTypeInfo;
extern hkClass hkBitFieldClass;
