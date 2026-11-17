#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A line segment between two points (the navmesh math's, not BSSegmentedTriShape::Segment).
struct Segment {
    // The default constructor leaves the members unbuilt.
    Segment() = default;
    Segment(const NiPoint3& aSegment, const NiPoint3& a2);

    void Offset(const NiPoint3& aOffset);
    NiPoint3* Delta(NiPoint3* apResult);
    // Copies aSegment's two points; returns this.
    Segment& operator=(const Segment& aSegment);

    NiPoint3 Start;
    NiPoint3 End;
};
