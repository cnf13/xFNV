#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionConvert.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionData.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiHalfSpaceBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiUnionBV.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiStream;

// The static data manager of the collision library: it sets up and tears down the bounding volume function tables.
// Everything in it is static.
class NiCollisionSDM {
public:
	NiCollisionSDM();

	static void Init();
	static void Shutdown();

	static inline bool ms_bInitialized;
	static inline int32_t siCounter;
};
