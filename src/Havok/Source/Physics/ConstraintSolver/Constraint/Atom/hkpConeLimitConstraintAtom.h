#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that limits the angle between a twist axis in body A and a reference axis in body B to a cone.
class hkpConeLimitConstraintAtom : public hkpConstraintAtom {
public:
    enum MeasurementMode : int32_t {
        ZERO_WHEN_VECTORS_ALIGNED = 0x0,
        ZERO_WHEN_VECTORS_PERPENDICULAR = 0x1,
    };

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_isEnabled;
    uint8_t m_twistAxisInA;
    uint8_t m_refAxisInB;
    hkEnum<MeasurementMode, uint8_t> m_angleMeasurementMode;
    uint8_t m_memOffsetToAngleOffset;
    uint8_t padding07;
    float m_minAngle;
    float m_maxAngle;
    float m_angularLimitsTauFactor;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpConeLimitConstraintAtomTypeInfo;
