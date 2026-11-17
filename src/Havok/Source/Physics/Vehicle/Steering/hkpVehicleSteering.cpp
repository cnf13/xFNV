#include "Havok/Source/Physics/Vehicle/Steering/hkpVehicleSteering.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleSteering (the PDB's hkpVehicleSteeringClass.obj; the SDK has no .cpp here to name
// the file, so it takes the header's stem): the class record, with no members of its own.
hkClass hkpVehicleSteeringClass("hkpVehicleSteering", &hkReferencedObjectClass, sizeof(hkpVehicleSteering), nullptr, 0,
	nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
