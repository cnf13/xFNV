#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkpVehicleEngine: the class record, with no members of its own.
hkClass hkpVehicleEngineClass(
    "hkpVehicleEngine",
    &hkReferencedObjectClass,
    sizeof(hkpVehicleEngine),
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
