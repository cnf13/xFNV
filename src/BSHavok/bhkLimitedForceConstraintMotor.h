#pragma once

#include "Reconstruction.hpp"

class NiStream;
class hkpLimitedForceConstraintMotor;

// The load and save every Havok motor shares for its force limits: a tag class with static helpers and no data of
// its own (the PDB's BSHavok\bhkLimitedForceConstraintMotor.cpp).
class bhkLimitedForceConstraintMotor {
public:
	// Reads the motor's minimum, then its maximum force, each one 4-byte component of the stream (cdecl, ret 0,
	// 0xD86100). The exe leaves the last read's count in eax; none of its three callers (0xCF41CC, 0xCF440B,
	// 0xCF460B) reads it.
	static void LoadBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor);
	// Writes the motor's minimum, then its maximum force, each one 4-byte component of the stream (cdecl, ret 0,
	// 0xD86160). The exe leaves the last write's count in eax; none of its three callers (0xCF426C, 0xCF449B,
	// 0xCF467B) reads it.
	static void SaveBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor);
};
