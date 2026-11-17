#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix6.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Reconstruction.hpp"

class hkpJacobianSchema;

// The PDB's out-of-line wrappers of the schema's static accessors (hkpJacobianChainSchemas.cpp), which the chain data
// calls from outside the solver library. The exe's four tiny functions 0xDD13B0..0xDD13E0 match that file's four
// wrappers in order (stiff-spring and ball-socket getJacobians at +0x10, then these two); names.py names no address,
// so the names rest on that match. Types from the PDB's statics getLinearJacobians(hkpJacobianSchema*) and
// getAngularJacobians(hkpJacobianSchema*, int).
// The linear Jacobians, three per constraint, right after the 0x20-byte head (cdecl, 0xDD13D0).
hkp1Lin2AngJacobian* hkJacobianPoweredChainSchema_getLinearJacobians(hkpJacobianSchema* schema);
// The angular Jacobians, after the aiNumConstraints linear triples.
hkp2AngJacobian* hkJacobianPoweredChainSchema_getAngularJacobians(hkpJacobianSchema* schema, int32_t numConstraints);
int32_t hkJacobianStiffSpringChainSchema_getJacobians(int32_t a1);
int32_t hkJacobianBallSocketChainSchema_getJacobians(int32_t a1);

// The head of a powered chain's schema: the chain's size in bytes, where the motors' status flags go, the motors'
// hysteresis and the chain's tau and damping. The Jacobians, buffers, accumulator offsets and motor states follow it
// in the stream (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobianPoweredChainSchema {
public:
	// The PDB's inline accessors (hkPoweredChainBuildJacobian 0xDF21C0 and hkSolverExport::exportImpulsesAndRhs
	// 0xDEF570 inline them), and const views of the last two for the exporter. After the head come three linear and
	// three angular Jacobians per constraint, the matrix buffer (one block triple per constraint), the velocity and
	// temp buffers (2 * numConstraints + 1 hkVector8), the accumulator offsets (one per body) and the child
	// constraints' motor states. The schema stream is raw memory the exe lays out by byte offsets, hence the casts.
	const hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() const {
		return const_cast<hkpJacobianPoweredChainSchema*>(this)->getAccumulatorOffsetsBase();
	}

	const hkp3dAngularMotorSolverInfo* getChildConstraintStatusBase() const {
		return const_cast<hkpJacobianPoweredChainSchema*>(this)->getChildConstraintStatusBase();
	}

	hkpConstraintChainMatrix6Triple* getMatrixBuffer() {
		return reinterpret_cast<
			hkpConstraintChainMatrix6Triple*>(getAngularJacobians(reinterpret_cast<hkpJacobianSchema*>(this),
												  m_numConstraints) +
			3 * m_numConstraints);
	}

	hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() {
		return reinterpret_cast<hkpVelocityAccumulatorOffset*>(reinterpret_cast<hkVector8*>(getMatrixBuffer() +
																   m_numConstraints) +
			2 * m_numConstraints + 1);
	}

	hkp3dAngularMotorSolverInfo* getChildConstraintStatusBase() {
		return reinterpret_cast<hkp3dAngularMotorSolverInfo*>(getAccumulatorOffsetsBase() + m_numConstraints + 1);
	}

	static hkp1Lin2AngJacobian* getLinearJacobians(hkpJacobianSchema* schema) {
		return reinterpret_cast<hkp1Lin2AngJacobian*>(reinterpret_cast<char*>(schema) +
			sizeof(hkpJacobianPoweredChainSchema));
	}

	static hkp2AngJacobian* getAngularJacobians(hkpJacobianSchema* schema, int32_t numConstraints) {
		return reinterpret_cast<hkp2AngJacobian*>(getLinearJacobians(schema) + 3 * numConstraints);
	}

	// Not named in the PDB or IDA; name chosen here. The size the exe stores in m_size for auNumConstraints
	// constraints, up to the next multiple of 16 (0xDF2213: 0x534 bytes per constraint, 0x44 more).
	static uint32_t getSize(uint16_t auNumConstraints) {
		const uint32_t uiBytes = static_cast<uint32_t>(sizeof(hkpJacobianPoweredChainSchema) +
			auNumConstraints *
				(3 * sizeof(hkp1Lin2AngJacobian) + 3 * sizeof(hkp2AngJacobian) +
					sizeof(hkpConstraintChainMatrix6Triple) + 2 * sizeof(hkVector8) +
					sizeof(hkpVelocityAccumulatorOffset) + sizeof(hkp3dAngularMotorSolverInfo)) +
			sizeof(hkVector8) + sizeof(hkpVelocityAccumulatorOffset));
		return (uiBytes + 15) & ~15u;
	}

	uint32_t m_typeBuffer;
	uint16_t m_numConstraints;

	// The PDB's anonymous union: the status pointer padded to 8 bytes.
	union {
		uint8_t* m_statusFlagsOutput;
		uint64_t m_padding1;
	};

	uint32_t m_size;
	float m_maxTorqueHysterisys;
	float m_tau;
	float m_damping;
};
