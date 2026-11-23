#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// The class record of hkpConstraintChainData, as the SDK's generated file has it (the SDK has no .cpp of the class:
// this file holds its reflection data). An abstract constraint data: parent hkpConstraintData, no members of its own.
hkClass hkpConstraintChainDataClass(
    "hkpConstraintChainData",
    &hkpConstraintDataClass,
    12,
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
