#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleEngine (the PDB's hkpVehicleEngineClass.obj; the SDK has no .cpp here to name the
// file, so it takes the header's stem): the class record, with no members of its own.
hkClass hkpVehicleEngineClass("hkpVehicleEngine", &hkReferencedObjectClass, sizeof(hkpVehicleEngine), nullptr, 0,
	nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
