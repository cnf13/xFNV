#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleAerodynamics (the PDB's hkpVehicleAerodynamicsClass.obj; the SDK has no .cpp here
// to name the file, so it takes the header's stem): the class record, with no members of its own.
hkClass hkpVehicleAerodynamicsClass("hkpVehicleAerodynamics", &hkReferencedObjectClass, sizeof(hkpVehicleAerodynamics),
	nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);

// The exe has no copy (no vtable of the abstract class): its subclasses' destructors inline it, with nothing to free.
hkpVehicleAerodynamics::~hkpVehicleAerodynamics() {}
