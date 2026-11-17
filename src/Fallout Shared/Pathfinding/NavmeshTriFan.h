#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Reconstruction.hpp"

// A triangle the path search fans out of, and whether it is the one that holds the inradius.
struct NavmeshTriFan {
	// The user-declared copy constructor would suppress the implicit default one, which src's locals and arrays
	// use.
	NavmeshTriFan() = default;
	NavmeshTriFan(const NavmeshTriFan& arOther);

	// The PDB's compiler-generated operator=(__that) (Pathfind.cpp), out of line: hTri through
	// the copy constructor 0x69A690 (FastNavMeshVertHandle's, the same layout), then bInradius element by element;
	// returns apThis (thiscall, ret 4, 0x6DBBB0). BSSimpleArray<NavmeshTriFan, 1024>'s code calls it. Not
	// operator=: a user-declared one would change every copy of a NavmeshTriFan in src.
	// Inferred type: arOther: NavmeshTriFan* from 0x6db52b (parameter 0 of BSSimpleArray<NavmeshTriFan, 1024>::Add),
	// its members used at 0x6dbbbc
	static NavmeshTriFan* Assign(NavmeshTriFan* apThis, const NavmeshTriFan& arOther);

	FastNavMeshTriHandle hTri;
	bool bInradius[3];
};
