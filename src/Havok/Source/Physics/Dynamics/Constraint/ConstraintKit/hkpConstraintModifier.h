#pragma once

#include "Reconstruction.hpp"

class hkpGenericConstraintDataParameters;

// A callback the constraint kit runs while a generic constraint builds its Jacobians
// (hkpConstraintConstructionKit::addConstraintModifierCallback, 0xD52BE0, stores it in the scheme's m_modifiers). The
// interface of the Havok SDK, with its empty inline bodies; the exe has no vtable of the class itself.
class hkpConstraintModifier {
public:
	virtual ~hkpConstraintModifier() {}

	virtual void modify(hkpGenericConstraintDataParameters& vars, int32_t userData) {}
};
