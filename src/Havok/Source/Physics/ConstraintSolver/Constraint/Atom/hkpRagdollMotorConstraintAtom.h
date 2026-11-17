#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

class hkpConstraintMotor;

// Constraint atom that drives a ragdoll joint's three angles (twist, cone, plane) towards a target orientation with one
// motor each.
class hkpRagdollMotorConstraintAtom : public hkpConstraintAtom {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkBool m_isEnabled;
    uint8_t padding03;
    int16_t m_initializedOffset;
    int16_t m_previousTargetAnglesOffset;
    // m_target_bRca starts on a 16-byte boundary. hkMatrix3's hkVector4 columns give it that alignment, so the 8 bytes
    // after m_previousTargetAnglesOffset are the compiler's padding.
    hkMatrix3 m_target_bRca;
    hkpConstraintMotor* m_motors[3];
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpRagdollMotorConstraintAtomTypeInfo;
