#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

class hkTransform;

// A constraint that holds a pivot of each body together. It overrides the impulse and notify accessors, isValid,
// getType, getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's.
class hkpBallAndSocketConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance.
	struct Runtime {
		hkpSolverResults m_solverResults[3];
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTranslationsConstraintAtom m_pivots;
		hkpBallSocketConstraintAtom m_ballSocket;
	};

	hkpBallAndSocketConstraintData();
	hkpBallAndSocketConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {}

	void setMaxLinearImpulse(float maxImpulse) override;
	float getMaxLinearImpulse() const override;
	void setBodyToNotify(int32_t bodyIdx) override;
	uint8_t getNotifiedBodyIndex() const override;
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary, so the bytes after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpBallAndSocketConstraintDataAtomsTypeInfo;
