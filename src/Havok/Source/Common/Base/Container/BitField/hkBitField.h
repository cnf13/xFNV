#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A packed array of bits: m_words holds them 32 to a word, m_numBits is how many of them the field has. Only the
// layout is declared; the exe's methods (get/set/assign/setSize and the rest) are not reconstructed yet.
class hkBitField {
public:
	int32_t fillUnusedBits(int32_t a1);
	void setSize(int32_t numBits, int32_t fillValue);

	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static int32_t countBits(const uint32_t* words, int32_t numWords);

	hkArray<uint32_t> m_words;
	int32_t m_numBits;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkBitFieldTypeInfo;
