#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleAerodynamics: the class record, with no members of its own.
hkClass hkpVehicleAerodynamicsClass(
    "hkpVehicleAerodynamics",
    &hkReferencedObjectClass,
    sizeof(hkpVehicleAerodynamics),
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

hkpVehicleAerodynamics::~hkpVehicleAerodynamics() {}
