#pragma once

#include <cstdint>

class hkpGenericConstraintDataParameters;

// A callback the constraint kit runs while a generic constraint builds its Jacobians
// (hkpConstraintConstructionKit::addConstraintModifierCallback stores it in the scheme's m_modifiers). The interface
// has empty inline bodies.
class hkpConstraintModifier {
public:
    virtual ~hkpConstraintModifier() {}

    virtual void modify(hkpGenericConstraintDataParameters& vars, int32_t userData) {}
};
