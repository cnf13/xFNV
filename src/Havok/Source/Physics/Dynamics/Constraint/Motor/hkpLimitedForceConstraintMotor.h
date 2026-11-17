#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

// A motor whose force is clamped to [m_minForce, m_maxForce].
class hkpLimitedForceConstraintMotor : public hkpConstraintMotor {
public:
    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkpLimitedForceConstraintMotor() = default;
    hkpLimitedForceConstraintMotor(hkFinishLoadedObjectFlag f) : hkpConstraintMotor(f) {}
    ~hkpLimitedForceConstraintMotor() override;

    float m_minForce;
    float m_maxForce;
};

// The class's reflection record.
extern hkClass hkpLimitedForceConstraintMotorClass;
