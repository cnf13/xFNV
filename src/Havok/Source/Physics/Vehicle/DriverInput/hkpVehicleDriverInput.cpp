#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleDriverInputStatus and hkpVehicleDriverInput: the two class records, with no members
// of their own.
hkClass hkpVehicleDriverInputStatusClass(
    "hkpVehicleDriverInputStatus",
    &hkReferencedObjectClass,
    sizeof(hkpVehicleDriverInputStatus),
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpVehicleDriverInputClass(
    "hkpVehicleDriverInput",
    &hkReferencedObjectClass,
    sizeof(hkpVehicleDriverInput),
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);

hkpVehicleDriverInputStatus::~hkpVehicleDriverInputStatus() {}
