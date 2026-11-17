#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// Conversions from bounds to bounding volumes; the results live in static volumes that the next call overwrites.
class NiCollisionConvert {
public:
	// Fills ms_kSphereABV with the sphere and returns it.
	static NiBoundingVolume* ConvertToSphereBV_Fast(float fRadius, const NiPoint3& kCenter);
	static NiBoundingVolume* ConvertToSphereBV_Safe(float fRadius, const NiPoint3& kCenter);
	// usQuantity: PDB unsigned short. The exe passes the caller's whole dword on; bool would
	// be zero-extended.
	static NiBoundingVolume* ConvertToBoxBV(int32_t usQuantity, const NiPoint3* pkVertex);

	// The sphere ConvertToSphereBV_Fast fills (its destructor is registered with atexit).
	static NiSphereBV ms_kSphereABV;
};
