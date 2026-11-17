#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"

// A triangle the path search fans out of, and whether it is the one that holds the inradius.
struct NavmeshTriFan {
    // The user-declared copy constructor would suppress the implicit default one, which locals and arrays use.
    NavmeshTriFan() = default;
    NavmeshTriFan(const NavmeshTriFan& arOther);

    // Copy assignment: copies hTri, then bInradius element by element; returns apThis. BSSimpleArray<NavmeshTriFan,
    // 1024> calls it. Not declared as operator=, because a user-declared one would change every copy of a
    // NavmeshTriFan.
    static NavmeshTriFan* Assign(NavmeshTriFan* apThis, const NavmeshTriFan& arOther);

    FastNavMeshTriHandle hTri;
    bool bInradius[3];
};
