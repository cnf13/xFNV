#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A parallelogram: one corner and the two edges leaving it; the points are origin + s * edge0 + t * edge1 with s and
// t in [0, 1].
class NiParallelogram {
public:
	NiPoint3 m_kOrigin;
	NiPoint3 m_kEdge0;
	NiPoint3 m_kEdge1;
};
