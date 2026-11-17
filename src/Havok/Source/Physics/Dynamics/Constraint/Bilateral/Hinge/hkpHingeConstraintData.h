#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

// A constraint that lets two bodies turn freely about one axle. The exe's vtable has 16 slots; it overrides slots 3
// to 9 (the impulse and notify accessors, isValid, getType, getRuntimeInfo) and getConstraintInfo; the destructor is
// hkBaseObject's.
class hkpHingeConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xCBE1E0's size, 40).
	struct Runtime {
		hkpSolverResults m_solverResults[5];
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Values from the Havok SDK.
		enum Axis : int32_t {
			AXIS_AXLE = 0x0,
		};

		Atoms();
		// The SDK's finish constructor: the loaded atoms keep their fields.
		Atoms(hkFinishLoadedObjectFlag) {}

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTransformsConstraintAtom m_transforms;
		hkp2dAngConstraintAtom m_2dAng;
		hkpBallSocketConstraintAtom m_ballSocket;
	};

	hkpHingeConstraintData();
	hkpHingeConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}

	void setMaxLinearImpulse(float maxImpulse) override;
	float getMaxLinearImpulse() const override;
	void setBodyToNotify(int32_t bodyIdx) override;
	uint8_t getNotifiedBodyIndex() const override;
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot,
		const hkVector4& axis);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& axisA,
		const hkVector4& axisB);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpHingeConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpHingeConstraintDataAtomsTypeInfo;
extern hkClass hkpHingeConstraintDataAtomsClass;
extern hkClass hkpHingeConstraintDataClass;
extern hkTypeInfo hkpHingeConstraintDataTypeInfo;
