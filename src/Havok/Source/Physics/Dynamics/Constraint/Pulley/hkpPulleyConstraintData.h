#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// A constraint that hangs each body's pivot on a rope running over a fixed world pivot per body. It overrides isValid,
// getType (returns CONSTRAINT_TYPE_PULLEY), getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's.
class hkpPulleyConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance, as reported by getRuntimeInfo.
	struct Runtime {
		hkpSolverResults m_solverResults[1];
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTranslationsConstraintAtom m_translations;
		hkpPulleyConstraintAtom m_pulley;
	};

	hkpPulleyConstraintData();
	// The SDK's finish constructor: the loaded atoms keep their fields.
	hkpPulleyConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {}

	// False when the rope length or the leverage is not positive.
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	// The atoms' info, m_atoms over 0x70 bytes.
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary, so the 4 bytes after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPulleyConstraintDataAtomsTypeInfo;
