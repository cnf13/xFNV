#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A sphere the path smoother and the navmesh math test against: its centre and radius.
struct Sphere {
    // Locals of the class are left unbuilt.
    Sphere() = default;
    Sphere(const NiPoint3& aCenter, float afRadius);

    NiPoint3 Center;
    float fRadius;
};
