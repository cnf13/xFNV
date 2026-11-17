#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Reconstruction.hpp"

// The way a path turns around a vertex.
enum ANGLE_DIRECTION : int32_t {
	AD_NONE = -1,
	AD_CCW = 0,
	AD_CW = 1,
	AD_COUNT = 2,
};

// An arc on the unit circle from one angle to another, running in the direction Dir (IDA: eDirection; the PDB's
// Start, End and Dir).
struct AngleSinCosArc {
	// Locals the exe leaves unset until assigned.
	AngleSinCosArc() = default;
	// A copy of *a1.
	AngleSinCosArc(AngleSinCosArc* a1);
	// The arc from aStart to aEnd in direction aeDir.
	AngleSinCosArc(const AngleSinCos& aStart, const AngleSinCos& aEnd, int32_t aeDir);

	bool WithinRange(const AngleSinCos& aAngle) const;
	// Inferred type: return: double: used as one at 0x6a030c; a1: float: used as one at 0x6a0831
	double GetLength(float a1);

	AngleSinCos Start;
	AngleSinCos End;
	ANGLE_DIRECTION Dir;
};
