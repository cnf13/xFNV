#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A sphere the path smoother and the navmesh math test against: its centre and radius (PDB names; the PDB places it
// in PathSmootherPOV.cpp).
struct Sphere {
	// The exe's locals of the class are left unbuilt.
	Sphere() = default;
	Sphere(const NiPoint3& a1, float a2);

	NiPoint3 Center;
	float fRadius;
};
