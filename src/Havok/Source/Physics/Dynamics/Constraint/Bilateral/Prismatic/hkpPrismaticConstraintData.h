#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

class hkpConstraintInstance;
class hkpConstraintMotor;

// A constraint that lets two bodies slide along one axis: the shaft, with its limits, motor and friction. The exe's
// vtable has 16 slots (the destructor, getClassType, calcContentStatistics and the constraint data interface); it
// overrides the destructor, isValid, getType, getRuntimeInfo and getConstraintInfo.
class hkpPrismaticConstraintData : public hkpConstraintData {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		SOLVER_RESULT_MOTOR = 0x0,
		SOLVER_RESULT_FRICTION = 0x1,
		SOLVER_RESULT_ANG_0 = 0x2,
		SOLVER_RESULT_ANG_1 = 0x3,
		SOLVER_RESULT_ANG_2 = 0x4,
		SOLVER_RESULT_LIN_0 = 0x5,
		SOLVER_RESULT_LIN_1 = 0x6,
		SOLVER_RESULT_LIMIT = 0x7,
		SOLVER_RESULT_MAX = 0x8,
	};

	// The runtime memory of one instance (getRuntimeInfo 0xD4F8A0's size, 72).
	struct Runtime {
		hkpSolverResults m_solverResults[8];
		uint8_t m_initialized;
		float m_previousTargetPosition;
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Values from the Xbox PDB.
		enum Axis : int32_t {
			AXIS_SHAFT = 0x0,
			AXIS_PERP_TO_SHAFT = 0x1,
		};

		Atoms();
		// The SDK's finish constructor: the loaded atoms keep their fields.
		Atoms(hkFinishLoadedObjectFlag) {}

		// Havok SDK (inline). The bytes from the first atom to the end of the last (hkGetByteOffsetInt(this,
		// &m_linLimit + 1)): sizeof(Atoms) on the 32-bit program; on x64 the struct has tail padding past the last
		// atom.
		const hkpConstraintAtom* getAtoms() const { return &m_transforms; }

		int32_t getSizeOfAllAtoms() const {
			return static_cast<int32_t>(reinterpret_cast<const char*>(&m_linLimit + 1) -
				reinterpret_cast<const char*>(this));
		}

		// The exe's out-of-line copy assignment (thiscall, ret 4, returns this, 0xD85AF0), emitted from
		// hkpConstraintDataCloningUtil.cpp: the transforms through their own operator=, then the other atoms.
		Atoms& operator=(const Atoms& other);

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTransformsConstraintAtom m_transforms;
		hkpLinMotorConstraintAtom m_motor;
		hkpLinFrictionConstraintAtom m_friction;
		hkpAngConstraintAtom m_ang;
		hkpLinConstraintAtom m_lin0;
		hkpLinConstraintAtom m_lin1;
		hkpLinLimitConstraintAtom m_linLimit;
	};

	// The exe's constructor run on apMemory, which it returns (thiscall, ret 0, 0xD4FE80).
	hkpPrismaticConstraintData();
	hkpPrismaticConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}
	~hkpPrismaticConstraintData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// Sets the motor of m_atoms.m_motor, taking a reference on apMotor and releasing the old one.
	void setMotor(hkpConstraintMotor* motor);
	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot,
		const hkVector4& axis);
	void setMotorActive(hkpConstraintInstance* instance, hkBool toBeEnabled);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& axisA,
		const hkVector4& axisB, const hkVector4& axisAPerp, const hkVector4& axisBPerp);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the SDK's HK_ALIGN16; the PDB has m_atoms at 0x10), so the 4 bytes
	// after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpPrismaticConstraintData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPrismaticConstraintDataAtomsTypeInfo;
