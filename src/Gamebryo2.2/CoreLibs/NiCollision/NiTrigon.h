#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A triangle: one corner and the two edges leaving it.
class NiTrigon {
public:
	// Stores the unit normal of the triangle in apNormal and returns it.
	NiPoint3* GetNormal(NiPoint3* apNormal);

	NiPoint3 m_kOrigin;
	NiPoint3 m_kEdge0;
	NiPoint3 m_kEdge1;
};
