#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

// A constraint that keeps a pivot of each body a fixed distance apart. The exe's vtable has 16 slots; it overrides
// isValid, getType, getRuntimeInfo and getConstraintInfo (slots 3 to 6 are hkpConstraintData's); the destructor is
// hkBaseObject's.
class hkpStiffSpringConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xD4F090's size, 8).
	struct Runtime {
		hkpSolverResults m_solverResults[1];
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTranslationsConstraintAtom m_pivots;
		hkpStiffSpringConstraintAtom m_spring;
	};

	hkpStiffSpringConstraintData();
	// The SDK's finish constructor: the loaded atoms keep their fields.
	hkpStiffSpringConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {}

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpStiffSpringConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpStiffSpringConstraintDataAtomsTypeInfo;
