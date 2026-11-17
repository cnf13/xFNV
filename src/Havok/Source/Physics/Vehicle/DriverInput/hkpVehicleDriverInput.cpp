#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleDriverInputStatus and hkpVehicleDriverInput (the PDB's
// hkpVehicleDriverInputClass.obj; the SDK has no .cpp here to name the file, so it takes the header's stem): the two
// class records, with no members of their own.
hkClass hkpVehicleDriverInputStatusClass("hkpVehicleDriverInputStatus", &hkReferencedObjectClass,
	sizeof(hkpVehicleDriverInputStatus), nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
hkClass hkpVehicleDriverInputClass("hkpVehicleDriverInput", &hkReferencedObjectClass, sizeof(hkpVehicleDriverInput),
	nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);

// The exe has no copy (no vtable of the class): the derived statuses' destructors inline it, with nothing to free.
hkpVehicleDriverInputStatus::~hkpVehicleDriverInputStatus() {}
