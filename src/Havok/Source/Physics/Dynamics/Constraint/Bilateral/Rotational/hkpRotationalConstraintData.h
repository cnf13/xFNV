#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// A constraint that holds the relative orientation of two bodies and leaves their positions free. It overrides isValid,
// getType, getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's and the others are
// hkpConstraintData's.
class hkpRotationalConstraintData : public hkpConstraintData {
public:
	// The solver results' indices (Havok SDK).
	enum {
		SOLVER_RESULT_ANG_0 = 0,
		SOLVER_RESULT_ANG_1 = 1,
		SOLVER_RESULT_ANG_2 = 2,
		SOLVER_RESULT_MAX = 3,
	};

	// The runtime memory of one instance.
	struct Runtime {
		hkpSolverResults m_solverResults[3];
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Havok SDK (inline); declared only.
		const hkpConstraintAtom* getAtoms() const;
		int32_t getSizeOfAllAtoms() const;

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalRotationsConstraintAtom m_rotations;
		hkpAngConstraintAtom m_ang;
	};

	hkpRotationalConstraintData();
	// The SDK's finish constructor: the loaded atoms keep their fields.
	hkpRotationalConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {}

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Declared only.
	void setInBodySpace(const hkQuaternion& aTb);
	void setInWorldSpace(const hkQuaternion& bodyARotation, const hkQuaternion& bodyBRotation);
	const Runtime* getRuntime(void* runtime) { return reinterpret_cast<Runtime*>(runtime); }

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary, so the 4 bytes after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpRotationalConstraintDataAtomsTypeInfo;
