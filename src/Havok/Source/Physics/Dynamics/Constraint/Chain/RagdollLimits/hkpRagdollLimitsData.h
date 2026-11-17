#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Reconstruction.hpp"

class hkTransform;

// The angular limits of a ragdoll joint, used as a link of a constraint chain. The exe's vtable has 16 slots; it
// overrides isValid, getType, getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's.
class hkpRagdollLimitsData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xD4F150's size, 28).
	struct Runtime {
		hkpSolverResults m_solverResults[3];
		float m_coneAngleOffset;
	};

	// The atoms: the local frames of both bodies, the twist limit, the cone limit and the two plane limits as a
	// cone. Offsets from the PDB; hkpRagdollLimitsDrawer::drawConstraint (0xDE59F0) reads them.
	struct Atoms {
		// Values from the Havok SDK.
		enum Axis : int32_t {
			AXIS_TWIST = 0x0,
			AXIS_PLANES = 0x1,
			AXIS_CROSS_PRODUCT = 0x2,
		};

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalRotationsConstraintAtom m_rotations;
		hkpTwistLimitConstraintAtom m_twistLimit;
		hkpConeLimitConstraintAtom m_coneLimit;
		hkpConeLimitConstraintAtom m_planesLimit;
	};

	// Havok SDK signature (hkpDynamics.lib, tied by reloc).
	hkpRagdollLimitsData();

	// The SDK's finish constructor: while finishing, turns cone limit stabilization back on when the loaded cone
	// limit has it.
	hkpRagdollLimitsData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {
		if (aFlag.m_finishing && m_atoms.m_coneLimit.m_memOffsetToAngleOffset) {
			setConeLimitStabilization(1);
		}
	}

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	char setConeLimitStabilization(char a1);
	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform,
		const hkVector4& twistAxisW, const hkVector4& planeAxisW);
	void setInBodySpace(const hkVector4& planeAxisA, const hkVector4& planeAxisB, const hkVector4& twistAxisA,
		const hkVector4& twistAxisB);
	void getConstraintFrameA(hkMatrix3& constraintFrameA);
	void getConstraintFrameB(hkMatrix3& constraintFrameB);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpRagdollLimitsData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpRagdollLimitsDataAtomsTypeInfo;
