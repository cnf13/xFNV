#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpSurfaceConstraintInfo.h"
#include "Reconstruction.hpp"

// What one solver run did with one surface constraint: whether it was used, how long it held the character and how
// far the character had to be pushed off it.
class hkpSurfaceConstraintInteraction {
public:
	// Values from the Xbox PDB.
	enum hkpStatus : int32_t {
		HK_STATUS_OK = 0x0,
		HK_STATUS_3D_FAILURE = 0x1,
		HK_STATUS_2D_FAILURE = 0x2,
	};

	uint8_t m_touched;
	uint8_t m_stopped;
	uint8_t padding02[2];
	float m_surfaceTime;
	float m_penaltyDistance;
	hkpStatus m_status;
};

// What a character proxy solve is given: where the character is and moves, the highest velocity a surface may carry
// it at, the up axis, the time to cover and the surfaces that may stop it.
struct hkpSimplexSolverInput {
	hkVector4 m_position;
	hkVector4 m_velocity;
	hkVector4 m_maxSurfaceVelocity;
	hkVector4 m_upVector;
	float m_deltaTime;
	float m_minDeltaTime;
	hkpSurfaceConstraintInfo* m_constraints;
	int32_t m_numConstraints;
};

// What the solve leaves: the position and velocity it reached, the time it took and one interaction per constraint.
struct hkpSimplexSolverOutput {
	hkVector4 m_position;
	hkVector4 m_velocity;
	float m_deltaTime;
	hkpSurfaceConstraintInteraction* m_planeInteractions;
};

// The exe's record is 0x28 bytes: its hkVector4 does not align the struct. src's hkVector4 is 16-byte aligned, so
// sizeof here rounds up to 0x30. Nothing in src allocates the record (the exe's proxies own it), so only the member
// offsets matter.

// The solver's working set: the support planes it has chosen so far (four at most) and the query it answers. The exe
// keeps it in hkSimplexSolverSolve's frame, uninitialised, and passes its address in eax to the solver proper.
class hkpSimplexSolver {
public:
	// One chosen surface: its index in the input, the constraint and the interaction record.
	struct SupportPlane {
		int32_t m_index;
		const hkpSurfaceConstraintInfo* m_constraint;
		hkpSurfaceConstraintInteraction* m_interaction;
	};

	// The solver proper: switches on the number of support planes and moves the output velocity on past them
	// (this in eax, ret 0, 0xD23F50).
	void solve();

	SupportPlane m_supportPlanes[4];
	int32_t m_numSupportPlanes;
	float m_currentTime;
	const hkpSimplexSolverInput* m_input;
	hkpSimplexSolverOutput* m_output;
};

// Runs one character proxy solve: finds the surface the character reaches first, advances it there and repeats until
// the time is used up.
void hkSimplexSolverSolve(const hkpSimplexSolverInput& input, hkpSimplexSolverOutput& output);
// Inferred type: a1: hkpSimplexSolver* from 0xd24090 (this of hkpSimplexSolver::solve), its members used at 0xd2338e
int32_t sortSupportPlanes(hkpSimplexSolver* a1);
// Inferred type: r_eax: hkpSurfaceConstraintInfo* from 0xd242ea (hkpSimplexSolver::SupportPlane::m_constraint (this of
// hkpSimplexSolver::solve)), its members used at 0xd23489
bool* hkSimplexSolver_isOutsidePlane(bool* a1, const __m128* a2, const hkpSurfaceConstraintInfo* r_eax);
// Inferred type: a2: hkpSimplexSolver* from 0xd242ba (this of hkpSimplexSolver::solve), its members used at 0xd23504;
// r_eax: hkVector4* from 0xd242ba (&hkpSimplexSolverInput+0x10 (hkpSimplexSolver::m_input (this of
// hkpSimplexSolver::solve))), its members used at 0xd234f7; r_esi: hkVector4* from 0xd242ba
// (&hkpSimplexSolverOutput+0x10 (hkpSimplexSolver::m_output (this of hkpSimplexSolver::solve))), its members used at
// 0xd234e0
// a1: one of the solver's input constraints: its plane (+0), velocity (+0x10) and frictions (+0x20..+0x2C) are read.
void solveOneConstraint(const hkpSurfaceConstraintInfo* a1, hkpSimplexSolver* a2, const hkVector4* r_eax,
	hkVector4* r_esi);
// Inferred type: a1: hkpSimplexSolver* from 0xd2404f (this of hkpSimplexSolver::solve), its members used at 0xd23de3;
// a5: hkVector4* from 0xd2404f (&hkpSimplexSolverInput+0x10 (hkpSimplexSolver::m_input (this of
// hkpSimplexSolver::solve))); a6: hkVector4* from 0xd2404f (&hkpSimplexSolverOutput+0x10 (hkpSimplexSolver::m_output
// (this of hkpSimplexSolver::solve))), its members used at 0xd23ca0; r_eax: hkpSurfaceConstraintInfo* from 0xd2404f
// (hkpSimplexSolver::SupportPlane::m_constraint (this of hkpSimplexSolver::solve)), its members used at 0xd23cf6
// a2, a3: the other two support-plane constraints, as r_eax (their planes and velocities read, a3's index taken from
// its offset in m_input->m_constraints; the callers pass m_supportPlanes[i].m_constraint).
void solve3d(hkpSimplexSolver* a1, const hkpSurfaceConstraintInfo* a2, const hkpSurfaceConstraintInfo* a3, int32_t a4,
	const hkVector4* a5, hkVector4* a6, const hkpSurfaceConstraintInfo* r_eax);
// Inferred type: a3: hkVector4* from 0xd24337 (&hkpSimplexSolverInput+0x10 (hkpSimplexSolver::m_input (this of
// hkpSimplexSolver::solve))), its members used at 0xd23ae5; a4: hkVector4* from 0xd24337 (&hkpSimplexSolverOutput+0x10
// (hkpSimplexSolver::m_output (this of hkpSimplexSolver::solve))), its members used at 0xd23850; r_edi:
// hkpSimplexSolver* from 0xd24337 (this of hkpSimplexSolver::solve), its members used at 0xd23a35
// a1, a2: two of the solver's input constraints: their planes are read at +0, their velocities at +0x10, and their
// indices in r_edi->m_input->m_constraints come from (a - m_constraints) >> 6 (0x40-byte hkpSurfaceConstraintInfo).
void solveTwoConstraints(const hkpSurfaceConstraintInfo* a1, const hkpSurfaceConstraintInfo* a2, const hkVector4* a3,
	hkVector4* a4, hkpSimplexSolver* r_edi);
void sub_D24650(int32_t a1);
