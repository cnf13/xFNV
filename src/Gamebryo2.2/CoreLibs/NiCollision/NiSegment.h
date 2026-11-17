#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// a line segment from an origin along a direction whose length is the segment length
class NiSegment {
public:
    // The point at parameter fS along the segment: origin + fS * direction.
    NiPoint3 GetPoint(float fS) const;
    int32_t IsEqual(NiSegment* a2);

    NiPoint3 m_kOrigin;
    NiPoint3 m_kDirection;
};
