#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkVector4;

// Havok's linear congruential generator (the PDB's members; the SDK's header-only class): m_current steps as
// 1664525 * m_current + 1013904223, and m_seed keeps the value it started from.
class hkPseudoRandomGenerator {
public:
	// Four getRandReal01 draws (each m_current / 2^32) into vecOut's x, y, z and w; the exe
	// keeps an out-of-line copy in hkGeometryUtility.cpp (thiscall, ret 4, 0xD5D660; called from 0xD5C1A0 and
	// 0xDEDBB0).
	void getRandomVector01(hkVector4& vecOut);
	// Four draws into vecOut's x, y, z and w, each 2 * getRandReal01() - 1; the exe keeps an
	// out-of-line copy in hkgpConvexHull.cpp (the PDB's file; thiscall, ret 4, 0xD7A4B0; called from 0xD76FA0).
	void getRandomVector11(hkVector4& vecOut);

	uint32_t m_seed;
	uint32_t m_current;
};
