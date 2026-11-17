#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Reconstruction.hpp"

class hkTransform;
struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpJacobianSchema;
struct hkpSetLocalTranslationsConstraintAtom;
struct hkpSimpleContactConstraintAtom;
class hkpSimpleContactConstraintData;
class hkpVelocityAccumulator;
// The world's buffer of constraints over their violation limit; src only passes its address on.
class hkpViolatedConstraintArray;

// The PDB's struct (PDB source hkpWorld.cpp); src's forward declarations say class, so the definition does too.
// The world allocates it (0x204 bytes) and its inlined constructor zeroes the count.
class hkpViolatedConstraintArray {
public:
	uint32_t m_nextFreeElement;
	hkpConstraintInstance* m_constraints[0x80];
};

// The step's times and solver factors a constraint's Jacobian builder reads. Offsets from the PDB; the PC layout is
// the same (all 4-byte members).
class hkpConstraintQueryStepInfo {
public:
	float m_substepDeltaTime;
	float m_microStepDeltaTime;
	float m_substepInvDeltaTime;
	float m_frameDeltaTime;
	float m_frameInvDeltaTime;
	float m_invNumSteps;
	float m_invNumStepsTimesMicroSteps;
	float m_maxConstraintViolationSqrd;
	float m_rhsFactor;
	float m_virtMassFactor;
	float m_frictionRhsFactor;
	uint8_t padding2C[4];
};

// The input of a constraint's Jacobian builder: the step info, the two bodies and the constraint. Its code stays with
// the exe. hkpSimulation::integrateInternal keeps one in a 16-aligned 0x80-byte stack slot (its frame bounds the
// size; the PDB's last member is at 0x74, and the class's 16-byte alignment pads it to 0x80).
class alignas(16) hkpConstraintQueryIn : public hkpConstraintQueryStepInfo {
public:
	// Fills the step info from the solver info and the step (the SDK's inline body). The exe's out-of-line copy
	// (thiscall, ret 0xC, 0xCFA6E0) is registered in hkpSimulation.cpp.
	void set(const hkpSolverInfo& si, const hkStepInfo& stepInfo,
		hkpViolatedConstraintArray* violatedConstraintsBuffer) {
		m_substepDeltaTime = si.m_deltaTime;
		m_microStepDeltaTime = si.m_deltaTime * si.m_invNumMicroSteps;
		m_substepInvDeltaTime = si.m_invDeltaTime;
		m_invNumSteps = si.m_invNumSteps;
		m_invNumStepsTimesMicroSteps = si.m_invNumSteps * si.m_invNumMicroSteps;
		m_tau = si.m_tau;
		m_damping = si.m_damping;

		m_violatedConstraints = violatedConstraintsBuffer;

		m_rhsFactor = si.m_tauDivDamp * si.m_invDeltaTime;
		m_virtMassFactor = si.m_damping;
		m_frictionRhsFactor = si.m_frictionTauDivDamp * si.m_invDeltaTime;

		m_frameDeltaTime = stepInfo.m_deltaTime;
		m_frameInvDeltaTime = stepInfo.m_invDeltaTime;
		// The SDK's SSE hkSimdReal(const float&) written out: the float in lane 0, the others cleared (movss).
		m_maxConstraintViolationSqrd.m_real = _mm_load_ss(&si.m_maxConstraintViolationSqrd);
	}

	hkpVelocityAccumulator* m_bodyA;
	hkpVelocityAccumulator* m_bodyB;
	const hkTransform* m_transformA;
	const hkTransform* m_transformB;
	float m_tau;
	float m_damping;
	// The PDB's hkSimdReal: 16 bytes on a 16-byte boundary (the 8 bytes after m_damping are its alignment), stored by
	// the exe with a 16-byte copy (0xCFA6E0).
	hkSimdReal m_maxConstraintViolationSqrd;
	hkpConstraintInstance* m_constraintInstance;
	hkPadSpu<void*> m_constraintRuntime;
	hkpViolatedConstraintArray* m_violatedConstraints;
	hkPadSpu<uint32_t> m_accumulatorAIndex;
	hkPadSpu<uint32_t> m_accumulatorBIndex;
	hkPadSpu<void*> m_constraintRuntimeInMainMemory;
};

// Where a constraint's Jacobian builder writes the schemas it builds: the next schema's slot. The build-jacobians
// job keeps it as a local and passes its address to the builders (the PDB's struct; the SDK's header declares it
// beside hkpConstraintQueryIn).
class hkpConstraintQueryOut {
public:
	hkpJacobianSchema* m_jacobianSchemas;
};

// Havok SDK: builds the Jacobian schemas of a constraint's atoms that are not a contact: walks the atoms from in to
// in + atomsEnd bytes and writes their schemas through angularLimitInfoBuffer (the PDB's parameter names; cdecl,
// 0xD6F460). The body is in Atom/hkpBuildJacobianFromAtoms.cpp.
void hkSolverBuildJacobianFromAtomsNotContact(const hkpConstraintAtom* atoms, int32_t sizeOfAllAtoms,
	const hkpConstraintQueryIn& tmpIn, hkpConstraintQueryOut& angularLimitInfoBuffer);
// Fires the contact point callbacks of a contact constraint's atom for the step aiStep.
// Defined in hkpSimpleContactConstraintData.cpp.
void hkSimpleContactConstraintData_fireCallbacks(hkpSimpleContactConstraintData* constraintData,
	const hkpConstraintQueryIn* in, hkpSimpleContactConstraintAtom* atom, hkpContactPointEvent::Type type);
void setLocalTranslationsToWorld(const hkpConstraintQueryIn* apIn, const hkpSetLocalTranslationsConstraintAtom* apAtom,
	hkTransform* apOutA, hkTransform* apOutB);
