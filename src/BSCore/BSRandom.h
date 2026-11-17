#pragma once

#include "Reconstruction.hpp"

// The game's Mersenne Twister random number generator (624 words of state; rseed_sp is the PDB's second seed).
class BSRandom {
public:
	BSRandom();

	// A random 32-bit value: UnsignedInt(0xFFFFFFFF).
	uint32_t UnsignedInt();
	// A random 32-bit value below aiMax.
	uint32_t UnsignedInt(uint32_t aiMax);
	// A random value in [auiMin, auiMax): auiMin plus UnsignedInt(auiMax - auiMin).
	uint32_t UnsignedInt(uint32_t auiMin, uint32_t auiMax);
	// A random float in [afMin, afMax): the 32-bit value over 2^32, scaled in double and rounded to float (thiscall,
	// ret 8, ST0, 0x476B90).
	float Float(float afMin, float afMax);
	// A random angle in [-pi, pi), rounded to float (thiscall, ST0, 0x6D1620).
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

// The PDB's function static of BSRandom::UnsignedInt(uint32_t) (`unsigned long mag01[2]`, 0x0 and MATRIX_A); the
// exe indexes it by the low bit (0xAA52D1). The type descriptor of IMemoryHeap follows it at 0x11AC3B8.
using mag01_t = uint32_t[2];
inline mag01_t mag01{0x0u, 0x9908B0DFu};
