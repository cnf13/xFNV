#pragma once

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

// A constraint that lets two bodies turn about one axle within limits, with a motor and friction. The exe's vtable
// has 16 slots (the destructor, getClassType, calcContentStatistics and the constraint data interface); it
// overrides the destructor, slots 3 to 6, isValid, getType, getRuntimeInfo and getConstraintInfo.
class hkpLimitedHingeConstraintData : public hkpConstraintData {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		SOLVER_RESULT_MOTOR = 0x0,
		SOLVER_RESULT_FRICTION = 0x1,
		SOLVER_RESULT_LIMIT = 0x2,
		SOLVER_RESULT_ANG_0 = 0x3,
		SOLVER_RESULT_ANG_1 = 0x4,
		SOLVER_RESULT_LIN_0 = 0x5,
		SOLVER_RESULT_LIN_1 = 0x6,
		SOLVER_RESULT_LIN_2 = 0x7,
		SOLVER_RESULT_MAX = 0x8,
	};

	// The runtime memory of one instance (getRuntimeInfo 0xCB8B10's size, 80). The solver results start on a
	// 16-byte boundary (the SDK's HK_ALIGN16), which pads the struct to 0x50.
	struct Runtime {
		alignas(16) hkpSolverResults m_solverResults[8];
		uint8_t m_initialized;
		float m_previousTargetAngle;
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Values from the Xbox PDB.
		enum Axis : int32_t {
			AXIS_AXLE = 0x0,
			AXIS_PERP_TO_AXLE_1 = 0x1,
			AXIS_PERP_TO_AXLE_2 = 0x2,
		};

		Atoms();
		// The SDK's finish constructor: the loaded atoms keep their values (the atoms' own finish constructors do
		// nothing).
		Atoms(hkFinishLoadedObjectFlag) {}

		// Havok SDK (inline). The bytes from the first atom to the end of the last (hkGetByteOffsetInt(this,
		// &m_ballSocket + 1)): 204 on the 32-bit program, which the exe has as a literal; more on x64, where the motor
		// atom holds a pointer.
		const hkpConstraintAtom* getAtoms() const { return &m_transforms; }

		int32_t getSizeOfAllAtoms() const {
			return static_cast<int32_t>(reinterpret_cast<const char*>(&m_ballSocket + 1) -
				reinterpret_cast<const char*>(this));
		}

		// The exe's out-of-line copy assignment (thiscall, ret 4, returns this, 0xD85A20), emitted from
		// hkpConstraintDataCloningUtil.cpp: the transforms through their own operator=, then the other atoms.
		Atoms& operator=(const Atoms& other);

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTransformsConstraintAtom m_transforms;
		hkpAngMotorConstraintAtom m_angMotor;
		hkpAngFrictionConstraintAtom m_angFriction;
		hkpAngLimitConstraintAtom m_angLimit;
		hkp2dAngConstraintAtom m_2dAng;
		hkpBallSocketConstraintAtom m_ballSocket;
	};

	hkpLimitedHingeConstraintData();
	hkpLimitedHingeConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}
	~hkpLimitedHingeConstraintData() override;

	void setMaxLinearImpulse(float maxImpulse) override;
	float getMaxLinearImpulse() const override;
	void setBodyToNotify(int32_t bodyIdx) override;
	uint8_t getNotifiedBodyIndex() const override;
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Inferred type: a1: hkpConstraintMotor* from 0xcaa7d9 (result of hkConstraintCinfo::LoadMotor)
	void setMotor(hkpConstraintMotor* a1);
	// Turns the constraint's motor on or off.
	void setMotorActive(hkpConstraintInstance* instance, hkBool toBeEnabled);

	// Whether the angular motor is enabled (the SDK's inline; the exe's out-of-line copy 0xCDEAC0 is registered in
	// bhkRagdollController.cpp).
	hkBool* isMotorActive(hkBool* apResult) const {
		// hkBool's copy: the stored byte as it is.
		apResult->m_bool = m_atoms.m_angMotor.m_isEnabled.m_bool;
		return apResult;
	}

	// The SDK's signature (hkpLimitedHingeConstraintData.h); the exe passes four pointers, ret 0x10.
	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot,
		const hkVector4& axisIn);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& axisA,
		const hkVector4& axisB, const hkVector4& axisAPerp, const hkVector4& axisBPerp);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpLimitedHingeConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLimitedHingeConstraintDataAtomsTypeInfo;
extern hkClass hkpLimitedHingeConstraintDataAtomsClass;
extern hkClass hkpLimitedHingeConstraintDataClass;
extern hkTypeInfo hkpLimitedHingeConstraintDataTypeInfo;
