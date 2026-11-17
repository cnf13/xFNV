#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

// A constraint that hangs each body's pivot on a rope running over a fixed world pivot per body. The exe's vtable has
// 16 slots; it overrides isValid (slot 7), getType (slot 8, returns CONSTRAINT_TYPE_PULLEY), getRuntimeInfo (slot 9)
// and getConstraintInfo (slot 15); the destructor is hkBaseObject's. Layout from the PC PDB.
class hkpPulleyConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xDEEF60's size, 8).
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

	// False when the rope length or the leverage is not positive (IDA names 0xDEEF90 the vector deleting
	// destructor).
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	// The atoms' info, m_atoms over 0x70 bytes.
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpPulleyConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPulleyConstraintDataAtomsTypeInfo;
