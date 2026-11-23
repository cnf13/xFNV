#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCos.h"

// The way a path turns around a vertex.
enum ANGLE_DIRECTION : int32_t {
    AD_NONE = -1,
    AD_CCW = 0,
    AD_CW = 1,
    AD_COUNT = 2,
};

// An arc on the unit circle from one angle to another, running in the direction Dir.
struct AngleSinCosArc {
    // Defaulted so that locals are left unset until assigned.
    AngleSinCosArc() = default;
    // A copy of *apArc.
    AngleSinCosArc(AngleSinCosArc* apArc);
    // The arc from aStart to aEnd in direction aeDir.
    AngleSinCosArc(const AngleSinCos& aStart, const AngleSinCos& aEnd, int32_t aeDir);

    bool WithinRange(const AngleSinCos& aAngle) const;
    double GetLength(float afRadius);

    AngleSinCos Start;
    AngleSinCos End;
    ANGLE_DIRECTION Dir;
};
