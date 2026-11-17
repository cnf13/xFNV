#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

// A constraint that holds a wheel on its suspension and steering axes. The exe's vtable has 16 slots; it overrides
// isValid, getType, getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's.
class hkpWheelConstraintData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xD4E580's size, 48).
	struct Runtime {
		hkpSolverResults m_solverResults[6];
	};

	// The atoms the constraint is made of (Havok SDK).
	struct Atoms {
		// Values from the Havok SDK: the suspension base's and the steering base's axes share the indices.
		enum Axis : int32_t {
			AXIS_SUSPENSION = 0x0,
			AXIS_PERP_SUSPENSION = 0x1,
			AXIS_AXLE = 0x0,
			AXIS_STEERING = 0x1,
		};

		// The atoms' constructor (PDB hkpWheelConstraintData::Atoms::Atoms; thiscall, ret 0, 0xD4F000).
		Atoms();
		// The SDK's finish constructor: the loaded atoms keep their fields.
		Atoms(hkFinishLoadedObjectFlag) {}

		// Havok SDK (inline); declared only.
		const hkpConstraintAtom* getAtoms() const;
		int32_t getSizeOfAllAtoms() const;

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTransformsConstraintAtom m_suspensionBase;
		hkpLinLimitConstraintAtom m_lin0Limit;
		hkpLinSoftConstraintAtom m_lin0Soft;
		hkpLinConstraintAtom m_lin1;
		hkpLinConstraintAtom m_lin2;
		hkpSetLocalRotationsConstraintAtom m_steeringBase;
		hkp2dAngConstraintAtom m_2dAng;
	};

	hkpWheelConstraintData();
	hkpWheelConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Turns the wheel to afAngle radians about the steering axis: rebuilds the first and third rows of the steering
	// base rotation in body B's space from the axle and steering axis it was built with, leaving the second row.
	void setSteeringAngle(float angle);
	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot,
		const hkVector4& axleTmp, const hkVector4& suspensionAxis, const hkVector4& steeringAxisTmp);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& axleA,
		const hkVector4& axleB, const hkVector4& suspensionAxisB, const hkVector4& steeringAxisB);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
	hkVector4 m_initialAxleInB;
	hkVector4 m_initialSteeringAxisInB;
};

inline constexpr fnvd::Vtable hkpWheelConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpWheelConstraintDataAtomsTypeInfo;
