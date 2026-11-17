#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A navmesh vertex (PDB names).
struct NavMeshVertex {
	NavMeshVertex();

	// Byte swaps the three coordinates.
	void Endian();

	NiPoint3 Location;
};
