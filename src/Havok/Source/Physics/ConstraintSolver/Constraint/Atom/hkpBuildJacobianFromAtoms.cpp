#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// This file's constant tables. The byte offsets of the three columns of a matrix, cyclic: entry i, i + 1 and i + 2 are
// the offsets of axis i and of the two axes after it ({0x00, 0x10, 0x20, 0x00, 0x10, 0x20, 0x00, 0x00}).
using AxisOffsetTable = uint8_t[8];
static AxisOffsetTable rdata_10D33C0{0, 16, 32, 0, 16, 32, 0, 16};
// The cone limit's angle offset and sign per measurement mode: {0.0, pi / 2} and {1.0, -1.0}.
using ConeModeTable = float[2];
static ConeModeTable rdata_10D33D8{0.0f, 1.5707964f};

// The SDK's name: buildJacobianFromConeLimitAtom's local `static const hkReal flipSignOrNot[2]` (release
// hkpConstraintSolver.lib, hkpBuildJacobianFromAtoms.obj), the function inlined here: the angle's sign per mode.
static const ConeModeTable buildJacobianFromConeLimitAtom_flipSignOrNot{1.0f, -1.0f};

double hkMath_atan2fApproximation(float afY, float afX) {
	double v6;
	double v2 = afY;
	float afYb = std::fabs((double)afY);
	float afYc = std::fabs((double)afX);
	double v3 = afYb;
	double v4 = afYc;
	if (!(afYc >= static_cast<double>(afYb))) {
		float afYg = v3 + 1.1920928955078125e-07;
		float afYh = v4 / afYg;
		double v7 = afYh;
		float afYi = afYh - afYh * 0.12107899785041809 * afYh;
		float afYj = afYi - v7 * (v7 * 0.0935228168964386 * v7);
		v6 = 1.5707963705062866 - afYj;
	} else {
		float afYd = v4 + 1.1920928955078125e-07;
		float afYe = v3 / afYd;
		double v5 = afYe;
		float afYf = afYe - afYe * 0.12107899785041809 * afYe;
		v6 = afYf - v5 * (v5 * 0.0935228168964386 * v5);
	}
	float afYa = v6;
	if (afX < 0.0) {
		afYa = 3.1415927410125732 - afYa;
	}
	// The exe negates only when y is below zero (fcompp; test ah, 0x41; jne skips on above, equal and unordered), so
	// a NaN y keeps the result's sign.
	double result = afYa;
	if (v2 < 0.0) {
		return static_cast<float>(-result);
	}
	return result;
}

// The stiff spring: a linear constraint along the pivots' separation (the x axis when they coincide), its
// right-hand side the atom's length less the distance.
void buildJacobianFromStiffSpringAtom(const hkpStiffSpringConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out) {
	hkp1dLinearBilateralConstraintInfo info;
	info.m_pivotA = baseA.m_translation;
	info.m_pivotB = baseB.m_translation;
	__m128 separation = _mm_sub_ps(info.m_pivotA.m_quad, info.m_pivotB.m_quad);
	__m128 squares = _mm_mul_ps(separation, separation);
	__m128 sum = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares);
	sum = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), sum);
	float fDistance = _mm_cvtss_f32(_mm_sqrt_ss(sum));
	// HK_REAL_EPSILON (fld dword 0x104DA4C); a NaN distance takes the normalising branch.
	if (!(fDistance <= 1.1920929e-07f)) {
		float fInvDistance = 1.0f / fDistance;
		info.m_constrainedDofW.m_quad =
			_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fInvDistance), _mm_set_ss(fInvDistance), 0), separation);
	} else {
		info.m_constrainedDofW = hkTransformIdentity.m_rotation.m_col0;
		fDistance = 0.0f;
	}
	hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs(info, in, out, atom.m_length - fDistance);
}

// The linear constraint along base B's axis m_axisIndex.
void buildJacobianFromLinAtom(const hkpLinConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out) {
	hkp1dLinearBilateralConstraintInfo info;
	info.m_pivotA = baseA.m_translation;
	info.m_pivotB = baseB.m_translation;
	info.m_constrainedDofW = baseB.m_rotation.getColumn(atom.m_axisIndex);
	hk1dLinearBilateralConstraintBuildJacobian(info, in, out);
}

// The ball-and-socket's stabilised right-hand side: the pivots' separation (arPosB - arPosA, stored to arRhsOut
// first) plus the difference of the two bodies' angular velocities (world space) crossed with each pivot's offset
// from its body's centre (m_scratch0), scaled by afFactor / afDenominator.
void _hkBallSocketConstraintComputeStabilizedRhs(const hkpVelocityAccumulator* accuA,
	const hkpVelocityAccumulator* accuB, const hkVector4& posA, const hkVector4& posB, float rhsFactor,
	float stabilizationFactor, hkVector4& errorOut) {
	float fScale = 1.0f / rhsFactor * stabilizationFactor;
	__m128 separation = _mm_sub_ps(posB.m_quad, posA.m_quad);
	errorOut.m_quad = separation;
	__m128 relA = _mm_sub_ps(posA.m_quad, accuA->m_scratch0.m_quad);
	__m128 relB = _mm_sub_ps(posB.m_quad, accuB->m_scratch0.m_quad);
	hkVector4 angVelA;
	angVelA.setRotatedInverseDir(accuA->m_scratch123, accuA->m_angularVel);
	hkVector4 angVelB;
	angVelB.setRotatedInverseDir(accuB->m_scratch123, accuB->m_angularVel);
	__m128 scale = _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0);
	__m128 crossB =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angVelB.m_quad, angVelB.m_quad, 0xD2), _mm_shuffle_ps(relB, relB, 0xC9)),
			_mm_mul_ps(_mm_shuffle_ps(angVelB.m_quad, angVelB.m_quad, 0xC9), _mm_shuffle_ps(relB, relB, 0xD2)));
	__m128 crossA =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angVelA.m_quad, angVelA.m_quad, 0xD2), _mm_shuffle_ps(relA, relA, 0xC9)),
			_mm_mul_ps(_mm_shuffle_ps(angVelA.m_quad, angVelA.m_quad, 0xC9), _mm_shuffle_ps(relA, relA, 0xD2)));
	errorOut.m_quad = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(crossB, scale), separation), _mm_mul_ps(crossA, scale));
}

// The SDK's file-local _hkBallSocketConstraintBuildJacobian: three 1D bilateral schemas along the identity axes.
void _hkBallSocketConstraintBuildJacobian(const hkpBallSocketConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn,
	const hkTransform& arTransformA, const hkTransform& arTransformB, hkpConstraintQueryOut& arOut) {
	int32_t v36;
	uint32_t v38;
	hkVector4 v46;
	hkVector4 v49;
	float m_maxImpulse = apAtom->m_maxImpulse;
	hkpVelocityAccumulator* m_bodyA = apIn->m_bodyA;
	__m128 m_quad = m_bodyA->m_scratch0.m_quad;
	hkpJacobianSchema* m_jacobianSchemas = arOut.m_jacobianSchemas;
	hkpVelocityAccumulator* m_bodyB = apIn->m_bodyB;
	*reinterpret_cast<float*>(&v38) =
		hkUFloat8_intToReal[apAtom->m_stabilizationFactor.m_value] * (1.0 / apIn->m_rhsFactor);
	__m128 v51 = _mm_set_ss(std::bit_cast<float>(v38));
	__m128 m_quad_1 = arTransformA.m_translation.m_quad;
	__m128 v8 = _mm_sub_ps(m_quad_1, m_quad);
	__m128 m_quad_2 = m_bodyB->m_scratch0.m_quad;
	__m128 v45 = v8;
	__m128 m_quad_3 = arTransformB.m_translation.m_quad;
	__m128 v48 = _mm_sub_ps(m_quad_3, m_quad_2);
	v49.setRotatedInverseDir(m_bodyA->m_scratch123, m_bodyA->m_angularVel);
	v46.setRotatedInverseDir(m_bodyB->m_scratch123, m_bodyB->m_angularVel);
	hkpViolatedConstraintArray* m_violatedConstraints = apIn->m_violatedConstraints;
	__m128 v11 = _mm_shuffle_ps(v51, v51, 0);
	__m128 v12 = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v46.m_quad, v46.m_quad, 210),
																 _mm_shuffle_ps(v48, v48, 201)),
													  _mm_mul_ps(_mm_shuffle_ps(v46.m_quad, v46.m_quad, 201),
														  _mm_shuffle_ps(v48, v48, 210))),
										   v11),
								_mm_sub_ps(m_quad_3, m_quad_1)),
		_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v49.m_quad, v49.m_quad, 210), _mm_shuffle_ps(v45, v45, 201)),
					   _mm_mul_ps(_mm_shuffle_ps(v49.m_quad, v49.m_quad, 201), _mm_shuffle_ps(v45, v45, 210))),
			v11));
	v45 = v12;
	if (m_violatedConstraints) {
		hkSimdReal v16;
		__m128 v13 = _mm_mul_ps(v12, v12);
		float v14 = _mm_shuffle_ps(v13, v13, 85)[0] + v13[0];
		float v15 = _mm_shuffle_ps(v13, v13, 170)[0];
		v16.m_real = apIn->hkpConstraintQueryIn::m_maxConstraintViolationSqrd.m_real;
		v16.m_real[0] = v16.m_real[0] - static_cast<float>(v15 + v14);
		if ((_mm_movemask_ps(v16.m_real) & 1) != 0) {
			// EXE-INLINED none: hkCriticalSection::atomicExchangeAdd
			// EXE-CAST: hkUint32 as LONG (the SDK's atomicExchangeAdd casts the counter for InterlockedExchangeAdd)
			uint32_t n0x80 =
				InterlockedExchangeAdd(reinterpret_cast<volatile int32_t*>(&m_violatedConstraints->m_nextFreeElement),
					1);
			if (n0x80 < 128) {
				apIn->m_violatedConstraints->m_constraints[n0x80] = apIn->m_constraintInstance;
			}
		}
	}
	// The three axes are the columns of hkTransformIdentity's rotation.
	const hkVector4* pAxis = &hkTransformIdentity.m_rotation.m_col0;
	int32_t v39 = 0;
	do {
		float m_virtMassFactor = apIn->m_virtMassFactor;
		double m_rhsFactor = apIn->m_rhsFactor;
		m_jacobianSchemas->m_quad = pAxis->m_quad;
		float m_rhsFactor_1 = m_rhsFactor;
		__m128 v20 = _mm_sub_ps(arTransformA.m_translation.m_quad, m_bodyA->m_scratch0.m_quad);
		__m128 v21 = _mm_sub_ps(arTransformB.m_translation.m_quad, m_bodyB->m_scratch0.m_quad);
		__m128 m_quad_4 = pAxis->m_quad;
		__m128 v23 = _mm_shuffle_ps(m_quad_4, m_quad_4, 201);
		__m128 v24 = _mm_shuffle_ps(m_quad_4, m_quad_4, 210);
		__m128 v25 =
			_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v20, v20, 201), v24), _mm_mul_ps(_mm_shuffle_ps(v20, v20, 210), v23));
		__m128 v26 =
			_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v21, v21, 210), v23), _mm_mul_ps(_mm_shuffle_ps(v21, v21, 201), v24));
		m_jacobianSchemas[1].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v25, v25, 85), m_bodyA->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(v25, v25, 0), m_bodyA->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(v25, v25, 170), m_bodyA->m_scratch123.m_col2.m_quad));
		__m128 v27 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v26, v26, 85), m_bodyB->m_scratch123.m_col1.m_quad),
			_mm_mul_ps(_mm_shuffle_ps(v26, v26, 0), m_bodyB->m_scratch123.m_col0.m_quad));
		__m128 v28 = _mm_mul_ps(_mm_shuffle_ps(v26, v26, 170), m_bodyB->m_scratch123.m_col2.m_quad);
		__m128 v29 = m_jacobianSchemas[1].m_quad;
		__m128 v30 = _mm_add_ps(v27, v28);
		m_jacobianSchemas[2].m_quad = v30;
		__m128 m_quad_5 = m_bodyB->m_invMasses.m_quad;
		__m128 m_quad_6 = m_bodyA->m_invMasses.m_quad;
		__m128 v33 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v30, v30), m_quad_5), _mm_mul_ps(_mm_mul_ps(v29, v29), m_quad_6));
		__m128 v34 = _mm_shuffle_ps(v33, _mm_unpackhi_ps(v33, _mm_add_ps(m_quad_6, m_quad_5)), 196);
		__m128 v35 = _mm_add_ps(_mm_shuffle_ps(v34, v34, 78), v34);
		float v43 = static_cast<float>(_mm_shuffle_ps(v35, v35, 177)[0] + v35[0]) + 1.1920928955078125e-07;
		m_jacobianSchemas[2].m_quad[3] = v43;
		m_jacobianSchemas[1].m_quad[3] = m_virtMassFactor / v43;
		v36 = v39;
		__m128 v37 =
			_mm_mul_ps(_mm_sub_ps(arTransformB.m_translation.m_quad, arTransformA.m_translation.m_quad), pAxis->m_quad);
		m_jacobianSchemas->m_quad[3] = static_cast<float>(_mm_shuffle_ps(v37, v37, 170)[0] +
										   static_cast<float>(_mm_shuffle_ps(v37, v37, 85)[0] + v37[0])) *
			m_rhsFactor_1;
		m_jacobianSchemas->m_quad[3] = v45[v39] * apIn->m_rhsFactor;
		m_jacobianSchemas->m_quad[0] = m_jacobianSchemas->m_quad[0] + 1.1920928955078125e-07;
		if (m_maxImpulse == 3.402820018375656e+38) {
			m_jacobianSchemas->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL;
		} else {
			m_jacobianSchemas->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT;
			m_jacobianSchemas[2].m_quad[3] = m_maxImpulse;
		}
		m_jacobianSchemas += 3;
		++pAxis;
		++v39;
	} while (v36 + 1 < 3);
	arOut.m_jacobianSchemas = m_jacobianSchemas;
}

// A header schema (type 1) for a contact constraint's solver results. The one caller passes
// the contact points after the reserved ones (hkContactPoint*); the header's field holds them as solver results.
void buildJacobianHeader(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut, void* apSolverResults,
	int32_t aiStriding) {
	hkpJacobianSchema* pSchema = arOut.m_jacobianSchemas;
	uint32_t bodyBIndex = arIn.m_accumulatorBIndex.m_storage;
	uint32_t bodyAIndex = arIn.m_accumulatorAIndex.m_storage;
	hkpConstraintInstance* pConstraintInstance = arIn.m_constraintInstance;
	pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
	pSchema->getHeader().m_bodyBIndex = static_cast<uint16_t>(bodyBIndex);
	pSchema->getHeader().m_constraintInstance = pConstraintInstance;
	pSchema->getHeader().m_bodyAIndex = static_cast<uint16_t>(bodyAIndex);
	pSchema->getHeader().m_solverResultInMainMemory = static_cast<hkpSolverResults*>(apSolverResults);
	pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
	pSchema->getHeader().m_solverResultStriding = static_cast<uint8_t>(aiStriding);
	arOut.m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
}

// A shift schema (type 3) at arOut's cursor: the solver skips
// auiNumToSkip results.
void buildShiftSolverResults(hkpConstraintQueryOut& arOut, uint8_t auiNumToSkip) {
	hkpJacobianSchema* pShift = arOut.m_jacobianSchemas;
	pShift->m_shiftSolverResults.m_numSolverResultsToSkip = auiNumToSkip;
	pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
	pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
	arOut.m_jacobianSchemas = pShift + 1;
}

// Not named in the PDB or IDA; name chosen here. The ignore modifier's step: the output's cursor back over the header
// schema (usercall: arOut in eax, 0xD6C1A0; the exe has a second copy at 0xD6C590).
void removeJacobianHeader(hkpConstraintQueryOut& arOut) {
	arOut.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
}

// Not named in the PDB or IDA; name chosen here. The exe's second copy of removeJacobianHeader (0xD6C1A0): the output's cursor
// back over the header schema (usercall: arOut in eax, 0xD6C590).
void removeJacobianHeaderSchema(hkpConstraintQueryOut& arOut) {
	arOut.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
}

// Not named in the PDB or IDA; name chosen here. The bridge atom: the output's cursor back over the header schema,
// then the constraint data's own builder (usercall: the atom in ecx, arIn on the stack, arOut in eax, 0xD6C1B0).
void buildBridgeJacobian(const hkpBridgeConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	hkpConstraintQueryOut& arOut) {
	arOut.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	arAtom.m_buildJacobianFunc(arAtom.m_constraintData, arIn, arOut);
}

// Not named in the PDB or IDA; name chosen here. arLocal's four columns (rotation and translation) taken by
// arWorld's rotation into arOut, then arWorld's translation added to the last (usercall: arOut on the stack, arLocal
// in edi, arWorld in esi, 0xD6C1D0).
void setMulTransform(hkTransform& arOut, const hkTransform& arLocal, const hkTransform& arWorld) {
	const __m128 c0 = arWorld.m_rotation.m_col0.m_quad;
	const __m128 c1 = arWorld.m_rotation.m_col1.m_quad;
	const __m128 c2 = arWorld.m_rotation.m_col2.m_quad;
	hkVector4* pOut = &arOut.m_rotation.m_col0;
	const hkVector4* pLocal = &arLocal.m_rotation.m_col0;
	for (int32_t i = 3; i >= 0; --i) {
		const __m128 v = pLocal[i].m_quad;
		pOut[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
										_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
			_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2));
	}
	arOut.m_translation.m_quad = _mm_add_ps(arWorld.m_translation.m_quad, arOut.m_translation.m_quad);
}

// Not named in the PDB or IDA; name chosen here. The set-local-transforms atom: the query's m_transformA/B times the
// atom's m_transformA/B into apOutA and apOutB (usercall: apOutA in ecx, the rest on the stack, 0xD6C240).
void calcWorldTransforms(hkTransform* apOutA, const hkpSetLocalTransformsConstraintAtom* apAtom,
	const hkpConstraintQueryIn* apIn, hkTransform* apOutB) {
	const hkTransform* pTransform = apIn->m_transformA;
	__m128 c0 = pTransform->m_rotation.m_col0.m_quad;
	__m128 c1 = pTransform->m_rotation.m_col1.m_quad;
	__m128 c2 = pTransform->m_rotation.m_col2.m_quad;
	hkVector4* pOut = &apOutA->m_rotation.m_col0;
	const hkVector4* pLocal = &apAtom->m_transformA.m_rotation.m_col0;
	for (int32_t i = 3; i >= 0; --i) {
		const __m128 v = pLocal[i].m_quad;
		pOut[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
										_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
			_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2));
	}
	apOutA->m_translation.m_quad = _mm_add_ps(pTransform->m_translation.m_quad, apOutA->m_translation.m_quad);
	pTransform = apIn->m_transformB;
	c0 = pTransform->m_rotation.m_col0.m_quad;
	c1 = pTransform->m_rotation.m_col1.m_quad;
	c2 = pTransform->m_rotation.m_col2.m_quad;
	pOut = &apOutB->m_rotation.m_col0;
	pLocal = &apAtom->m_transformB.m_rotation.m_col0;
	for (int32_t i = 3; i >= 0; --i) {
		const __m128 v = pLocal[i].m_quad;
		pOut[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
										_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
			_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2));
	}
	apOutB->m_translation.m_quad = _mm_add_ps(pTransform->m_translation.m_quad, apOutB->m_translation.m_quad);
}

// Not named in the PDB or IDA; name chosen here. The set-local-rotations atom: the query's m_transformA/B rotations
// times the atom's m_rotationA/B into apOutA and apOutB. The asm wins over the SDK: each loop runs four columns, so
// it also overwrites each output's translation with the rotation of the 16 bytes after the atom's rotation, as
// hkSolverBuildJacobianFromAtomsNotContact's inline copy does (usercall: apOutA in edx, apOutB on the stack, the atom
// in edi, apIn in esi, 0xD6C3C0).
void setLocalRotationsToWorld(hkTransform* apOutA, hkTransform* apOutB,
	const hkpSetLocalRotationsConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn) {
	const hkTransform* pTransform = apIn->m_transformA;
	__m128 c0 = pTransform->m_rotation.m_col0.m_quad;
	__m128 c1 = pTransform->m_rotation.m_col1.m_quad;
	__m128 c2 = pTransform->m_rotation.m_col2.m_quad;
	hkVector4* pOut = &apOutA->m_rotation.m_col0;
	const hkVector4* pLocal = &apAtom->m_rotationA.m_col0;
	for (int32_t i = 3; i >= 0; --i) {
		const __m128 v = pLocal[i].m_quad;
		pOut[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
										_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
			_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2));
	}
	pTransform = apIn->m_transformB;
	c0 = pTransform->m_rotation.m_col0.m_quad;
	c1 = pTransform->m_rotation.m_col1.m_quad;
	c2 = pTransform->m_rotation.m_col2.m_quad;
	pOut = &apOutB->m_rotation.m_col0;
	pLocal = &apAtom->m_rotationB.m_col0;
	for (int32_t i = 3; i >= 0; --i) {
		const __m128 v = pLocal[i].m_quad;
		pOut[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
										_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
			_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2));
	}
}

// The PDB's file-local buildJacobianFrom2dAngAtom: two angular rows about the axes of base B after the free axis
// (usercall: arBaseA in ecx, arIn and arOut on the stack, the atom in eax, arBaseB in esi, 0xD6C470).
void buildJacobianFrom2dAngAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& baseA,
	const hkp2dAngConstraintAtom& baseB, const hkTransform& out) {
	uint32_t uiAxis = baseB.m_freeRotationAxis;
	// The exe adds its axis table's byte offsets to the frame's address: raw bytes.
	const uint8_t* pBytesB = reinterpret_cast<const uint8_t*>(&out);
	const hkVector4* pOther1 = reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[uiAxis + 1]);
	const hkVector4* pOther2 = reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[uiAxis + 2]);
	hkp1dAngularBilateralConstraintInfo kInfo;
	kInfo.m_zeroErrorAxisAinW = (&atom.m_rotation.m_col0)[uiAxis];
	kInfo.m_constrainedDofW = *pOther1;
	kInfo.m_perpZeroErrorAxisBinW = *pOther2;
	hk1dAngularBilateralConstraintBuildJacobian(kInfo, in, baseA);
	kInfo.m_constrainedDofW = *pOther2;
	kInfo.m_perpZeroErrorAxisBinW.m_quad = _mm_xor_ps(hkQuadRealNegateMask, pOther1->m_quad);
	hk1dAngularBilateralConstraintBuildJacobian(kInfo, in, baseA);
}

// The PDB's file-local buildJacobianFromAngAtom: one angular row per constrained axis, the atom's members read again
// on every pass (usercall: arIn, arBaseA, arBaseB and arOut on the stack, the atom in edi, 0xD6C500).
void buildJacobianFromAngAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpAngConstraintAtom& out) {
	// The exe adds its axis table's byte offsets to the frames' addresses: raw bytes.
	const uint8_t* pBytesA = reinterpret_cast<const uint8_t*>(&in);
	const uint8_t* pBytesB = reinterpret_cast<const uint8_t*>(&baseA);
	for (int32_t i = out.m_firstConstrainedAxis; i < out.m_firstConstrainedAxis + out.m_numConstrainedAxes; ++i) {
		hkp1dAngularBilateralConstraintInfo kInfo;
		kInfo.m_perpZeroErrorAxisBinW = *reinterpret_cast<const hkVector4*>(pBytesA + rdata_10D33C0[i]);
		kInfo.m_zeroErrorAxisAinW = *reinterpret_cast<const hkVector4*>(pBytesA + rdata_10D33C0[i + 1]);
		kInfo.m_constrainedDofW = *reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[i + 2]);
		hk1dAngularBilateralConstraintBuildJacobian(kInfo, atom, baseB);
	}
}

// Not named in the PDB or IDA; name chosen here. The atom under a chain of modifiers: each modifier's child until one
// that is no modifier (usercall: the first modifier in eax, the result in eax, 0xD6C580).
hkpConstraintAtom* findInnermostChildAtom(const hkpModifierConstraintAtom* apModifier) {
	hkpConstraintAtom* pAtom = apModifier->m_child;
	while (pAtom->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
		pAtom = static_cast<hkpModifierConstraintAtom*>(pAtom)->m_child;
	}
	return pAtom;
}

// Not named in the PDB or IDA; name chosen here. The exe's register-argument copy of
// hk1dAngularVelocityMotorBeginJacobian (0xD6CD70), the same code (usercall: arIn in ecx, arJac in edx, arStatusOut on
// the stack, arDir in esi, 0xD6CC80). Returns &arStatusOut, the eax the exe leaves.
hkp1dBilateralConstraintStatus* build2AngJacobian(const hkpConstraintQueryIn& arIn, hkp2AngJacobian& arJac,
	hkp1dBilateralConstraintStatus& arStatusOut, const hkVector4& arDir) {
	const hkpVelocityAccumulator* pBodyB = arIn.m_bodyB;
	const hkpVelocityAccumulator* pBodyA = arIn.m_bodyA;
	const __m128 kDir = arDir.m_quad;
	const __m128 kNegDir = _mm_xor_ps(kDir, hkQuadRealNegateMask);
	const hkRotation& rotA = pBodyA->m_scratch123;
	const hkRotation& rotB = pBodyB->m_scratch123;
	arJac.m_angular[0].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55), rotA.m_col1.m_quad),
											   _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00), rotA.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA), rotA.m_col2.m_quad));
	const __m128 kAng1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55), rotB.m_col1.m_quad),
										_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00), rotB.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA), rotB.m_col2.m_quad));
	arJac.m_angular[1].m_quad = kAng1;
	const __m128 kAng0 = arJac.m_angular[0].m_quad;
	const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAng0, kAng0), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kAng1, kAng1), pBodyB->m_invMasses.m_quad));
	const __m128 kSum =
		_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA), _mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass));
	// The exe rounds the sum plus epsilon and the quotient through float stack slots (fstp/fld).
	float fDiag = _mm_cvtss_f32(kSum) + 1.1920928955078125e-07;
	float fVirtualMass = 1.0f / fDiag;
	const float fStored = fVirtualMass;
	arJac.m_angular[0].m_quad[3] = fStored;
	arStatusOut.m_virtualMass = fStored;
	return &arStatusOut;
}

// The PDB's file-local buildJacobianFromLinSoftAtom: a soft linear row along base B's axis m_axisIndex with the atom's
// tau and damping (usercall: arBaseB in ecx, arIn, arBaseA and arOut on the stack, the atom in eax, 0xD6D220).
void buildJacobianFromLinSoftAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinSoftConstraintAtom& out) {
	hkp1dLinearBilateralUserTauConstraintInfo kInfo;
	kInfo.m_tau = out.m_tau;
	kInfo.m_pivotA = baseA.m_translation;
	kInfo.m_pivotB = atom.m_translation;
	kInfo.m_constrainedDofW = (&atom.m_rotation.m_col0)[out.m_axisIndex];
	kInfo.m_damping = out.m_damping;
	hk1dLinearBilateralConstraintUserTauBuildJacobian(kInfo, in, baseB);
}

// The PDB's file-local buildJacobianFromLinLimitAtom: a limited linear row along base B's axis m_axisIndex
// (usercall: arBaseB in ecx, arIn, arBaseA and arOut on the stack, the atom in eax, 0xD6D280).
void buildJacobianFromLinLimitAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinLimitConstraintAtom& out) {
	hkp1dLinearLimitInfo kInfo;
	kInfo.m_min = out.m_min;
	kInfo.m_pivotA = baseA.m_translation;
	kInfo.m_max = out.m_max;
	kInfo.m_pivotB = atom.m_translation;
	kInfo.m_constrainedDofW = (&atom.m_rotation.m_col0)[out.m_axisIndex];
	hk1dLinearLimitBuildJacobian(kInfo, in, baseB);
}

// The PDB's file-local buildJacobianFromAngLimitAtom, its first half: the angular limit's info (axis, bounds, tau and
// the current angle about base A's axis m_limitAxis, moved by 2 pi into (reference - pi, reference + pi] with each
// step stored) into arInfo. Returns 1 when the limit is disabled (arInfo untouched), else 0 (usercall: the atom in
// ecx, arIn, arBaseA and arBaseB on the stack, arInfo in esi, 0xD6D2E0).
int32_t buildJacobianFromAngLimitAtom(const hkpAngLimitConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	const hkTransform& arBaseA, const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo) {
	if (!arAtom.m_isEnabled) {
		return 1;
	}
	uint32_t uiAxis = arAtom.m_limitAxis;
	// The exe adds its axis table's byte offsets to the frames' addresses: raw bytes.
	const uint8_t* pBytesA = reinterpret_cast<const uint8_t*>(&arBaseA);
	const uint8_t* pBytesB = reinterpret_cast<const uint8_t*>(&arBaseB);
	arInfo.m_tau.m_storage = arIn.m_tau * arAtom.m_angularLimitsTauFactor;
	arInfo.m_constrainedDofW = (&arBaseA.m_rotation.m_col0)[uiAxis];
	const __m128 kA1 = _mm_load_ps(reinterpret_cast<const float*>(pBytesA + rdata_10D33C0[uiAxis + 1]));
	arInfo.m_min.m_storage = arAtom.m_minAngle;
	const __m128 kB2 = _mm_load_ps(reinterpret_cast<const float*>(pBytesB + rdata_10D33C0[uiAxis + 2]));
	const __m128 kB = (&arBaseB.m_rotation.m_col0)[uiAxis].m_quad;
	arInfo.m_max.m_storage = arAtom.m_maxAngle;
	const __m128 kCosV = _mm_mul_ps(kB2, kA1);
	const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
	const __m128 kSinV = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kB2, kB2, 0xC9)),
										_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kB2, kB2, 0xD2))),
		kA1);
	const float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
	const float fAngle = hkMath_atan2fApproximation(fCos, fSin);
	arInfo.m_computedAngle.m_storage = fAngle;
	const float fReference =
		static_cast<hkpSolverResults*>(arIn.m_constraintRuntime.m_storage)->m_internalSolverData * -1.0;
	if (fReference - fAngle > 3.1415927410125732) {
		do {
			arInfo.m_computedAngle.m_storage = arInfo.m_computedAngle.m_storage + 6.2831854820251465;
		} while (fReference - arInfo.m_computedAngle.m_storage > 3.1415927410125732);
	}
	if (arInfo.m_computedAngle.m_storage - fReference > 3.1415927410125732) {
		do {
			arInfo.m_computedAngle.m_storage = arInfo.m_computedAngle.m_storage - 6.2831854820251465;
		} while (arInfo.m_computedAngle.m_storage - fReference > 3.1415927410125732);
	}
	return 0;
}

// Not named in the PDB or IDA; name chosen here. The twist limit's info into arInfo: the bounds, the angle of the
// reference axes about the bisector of the two bodies' twist axes (arInfo's axis), and tau scaled by the square of
// half the bisector's length. Returns 0 (usercall: arBaseA in edx, arIn and arBaseB on the stack, arInfo in edi, the
// atom in esi, 0xD6D450).
int32_t calcTwistLimitInfo(const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn, const hkTransform& arBaseB,
	hkp1dAngularLimitInfo& arInfo, const hkpTwistLimitConstraintAtom& arAtom) {
	const hkVector4* pColsA = &arBaseA.m_rotation.m_col0;
	const hkVector4* pColsB = &arBaseB.m_rotation.m_col0;
	arInfo.m_constrainedDofW = pColsA[arAtom.m_twistAxis];
	arInfo.m_min.m_storage = arAtom.m_minAngle;
	arInfo.m_max.m_storage = arAtom.m_maxAngle;
	uint32_t uiTwist = arAtom.m_twistAxis;
	uint32_t uiRef = arAtom.m_refAxis;
	__m128 kAxis = pColsB[uiTwist].m_quad;
	const __m128 kSum = _mm_add_ps(pColsA[uiTwist].m_quad, kAxis);
	const __m128 kSq = _mm_mul_ps(kSum, kSum);
	const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
	float fHalfLength;
	// fcom / test ah, 0x41 / jne: normalises only when the length is above 1e-16 (the double at 0x10D33D0).
	if (fLength > 1.0000000168623835e-16) {
		fHalfLength = fLength * 0.5;
		float fInvLength = 1.0 / fLength;
		kAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
	} else {
		fHalfLength = 0.0f;
	}
	const __m128 kRefB = pColsB[uiRef].m_quad;
	const __m128 kAxisD2 = _mm_shuffle_ps(kAxis, kAxis, 0xD2);
	const __m128 kAxisC9 = _mm_shuffle_ps(kAxis, kAxis, 0xC9);
	const __m128 kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
		_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
	const __m128 kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
		_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
	const __m128 kRefA = pColsA[uiRef].m_quad;
	const __m128 kCosV = _mm_mul_ps(kRefA, kPerp);
	const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
	const __m128 kSinV = _mm_mul_ps(kRefA, kPerp2);
	const float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
	arInfo.m_computedAngle.m_storage = hkMath_atan2fApproximation(fCos, fSin);
	arInfo.m_constrainedDofW.m_quad = kAxis;
	arInfo.m_tau.m_storage = arIn.m_tau * arAtom.m_angularLimitsTauFactor * fHalfLength * fHalfLength;
	return 0;
}

// The PDB's file-local buildJacobianFromAngFrictionAtom: angular friction about base A's axes from
// m_firstFrictionAxis, else a shift schema over its solver results (usercall: the atom in ecx, arOut in edx, arBaseA
// on the stack, arIn in esi, 0xD6D990).
void buildJacobianFromAngFrictionAtom(const hkpAngFrictionConstraintAtom& arAtom, hkpConstraintQueryOut& arOut,
	const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn) {
	// fcomp / test ah, 0x44 / jnp: no friction when the maximum torque is exactly 0.
	if (arAtom.m_isEnabled && 0.0f != arAtom.m_maxFrictionTorque) {
		hkp1dAngularFrictionInfo kInfo;
		kInfo.m_maxFrictionTorque = arAtom.m_maxFrictionTorque;
		kInfo.m_constrainedDofW = &(&arBaseA.m_rotation.m_col0)[arAtom.m_firstFrictionAxis];
		kInfo.m_numFriction.m_storage = arAtom.m_numFrictionAxes;
		kInfo.m_lastSolverResults = static_cast<hkpSolverResults*>(arIn.m_constraintRuntime.m_storage);
		hk1dAngularFrictionBuildJacobian(kInfo, arIn, arOut);
	} else {
		hkpJacobianSchema* pShift = arOut.m_jacobianSchemas;
		pShift->m_shiftSolverResults.m_numSolverResultsToSkip = arAtom.m_numFrictionAxes;
		pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
		pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
		arOut.m_jacobianSchemas = pShift + 1;
	}
}

// The PDB's file-local buildJacobianFromLinFrictionAtom: linear friction along base B's axis m_frictionAxis at base
// A's pivot, else a shift schema over its solver result (usercall: the atom in ecx, arIn in edx, arBaseA on the
// stack, arBaseB in edi, arOut in esi, 0xD6E280).
void buildJacobianFromLinFrictionAtom(const hkpLinFrictionConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out) {
	// fcomp / test ah, 5 / jp: no friction when 0 >= the maximum force or it is NaN.
	if (atom.m_isEnabled && 0.0f < atom.m_maxFrictionForce) {
		hkp1dLinearFrictionInfo kInfo;
		kInfo.m_maxFrictionForce = atom.m_maxFrictionForce;
		kInfo.m_pivot = baseA.m_translation;
		kInfo.m_constrainedDofW = (&baseB.m_rotation.m_col0)[atom.m_frictionAxis];
		kInfo.m_lastSolverResults = static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage);
		hk1dLinearFrictionBuildJacobian(kInfo, in, out);
	} else {
		hkpJacobianSchema* pShift = out.m_jacobianSchemas;
		pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
		pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
		pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
		out.m_jacobianSchemas = pShift + 1;
	}
}

// Not named in the PDB or IDA; name chosen here. The header schema (type 1) at apOut's cursor, as buildJacobianHeader
// (0xD6EA30) writes it in another order: the query's accumulator indices and constraint instance, apSolverResults and
// auiStriding (usercall: apIn in ecx, apOut in edx, the rest on the stack, 0xD6F270).
void buildJacobianHeaderPtr(const hkpConstraintQueryIn* apIn, hkpConstraintQueryOut* apOut, void* apSolverResults,
	uint8_t auiStriding) {
	hkpJacobianSchema* pSchema = apOut->m_jacobianSchemas;
	uint32_t bodyBIndex = apIn->m_accumulatorBIndex.m_storage;
	uint32_t bodyAIndex = apIn->m_accumulatorAIndex.m_storage;
	hkpConstraintInstance* pConstraintInstance = apIn->m_constraintInstance;
	pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
	pSchema->getHeader().m_bodyBIndex = static_cast<uint16_t>(bodyBIndex);
	pSchema->getHeader().m_constraintInstance = pConstraintInstance;
	pSchema->getHeader().m_bodyAIndex = static_cast<uint16_t>(bodyAIndex);
	pSchema->getHeader().m_solverResultInMainMemory = static_cast<hkpSolverResults*>(apSolverResults);
	pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
	pSchema->getHeader().m_solverResultStriding = auiStriding;
	apOut->m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
}

// The PDB's file-local buildJacobianFromConeLimitAtom, the limit info only: the angle between base A's twist axis and
// base B's reference axis (offset and signed per the atom's measurement mode) about their cross product, into arInfo.
// Returns 1 when the axes are parallel (the squared sine below epsilon; arInfo untouched), else 0 (usercall: arIn,
// arBaseA and arBaseB on the stack, arInfo in edi, the atom in esi, 0xD6D5D0).
int32_t buildJacobianFromConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom) {
	const __m128 kTwistA = (&arBaseA.m_rotation.m_col0)[arAtom.m_twistAxisInA].m_quad;
	const __m128 kRefB = (&arBaseB.m_rotation.m_col0)[arAtom.m_refAxisInB].m_quad;
	const __m128 kCross =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), _mm_shuffle_ps(kTwistA, kTwistA, 0xC9)),
			_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), _mm_shuffle_ps(kTwistA, kTwistA, 0xD2)));
	const __m128 kSq = _mm_mul_ps(kCross, kCross);
	const float fSinSq =
		_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)));
	// fcom / test ah, 5 / jp: no limit when the squared sine is below epsilon (NaN builds the limit).
	if (fSinSq < 1.1920929e-07f) {
		return 1;
	}
	uint32_t uiMode = arAtom.m_angleMeasurementMode.m_storage;
	const __m128 kCosV = _mm_mul_ps(kRefB, kTwistA);
	const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
	const float fSin = std::sqrt(static_cast<double>(fSinSq));
	const float fAngle = hkMath_atan2fApproximation(fSin, fCos) * buildJacobianFromConeLimitAtom_flipSignOrNot[uiMode] +
		rdata_10D33D8[uiMode];
	// The exe reads the sign at 0x10D33E4 - 4 * mode: the other entry of the {1, -1} table.
	const float fSign = buildJacobianFromConeLimitAtom_flipSignOrNot[1 - arAtom.m_angleMeasurementMode.m_storage];
	arInfo.m_tau.m_storage = arIn.m_tau * arAtom.m_angularLimitsTauFactor;
	const float fInvSin = 1.0 / fSin;
	arInfo.m_computedAngle.m_storage = fAngle;
	arInfo.m_constrainedDofW.m_quad = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(fInvSin), kCross), _mm_set1_ps(fSign));
	arInfo.m_min.m_storage = arAtom.m_minAngle;
	arInfo.m_max.m_storage = arAtom.m_maxAngle;
	return 0;
}

// The PDB's file-local buildJacobianFromStabilizedConeLimitAtom, the limit info only: the cone limit's info as
// buildJacobianFromConeLimitAtom (0xD6D5D0) builds it; then, when the atom keeps an angle offset in the runtime
// (m_memOffsetToAngleOffset above 1), the offset is moved towards the angle by the larger of half a frame's time and
// 5% of the offset, and the info's angle is the remaining error. Returns 1 when the axes are parallel, else 0
// (usercall: arIn, arBaseA and arBaseB on the stack, arInfo in edi, the atom in esi, 0xD6D720).
int32_t buildJacobianFromStabilizedConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom) {
	const __m128 kTwistA = (&arBaseA.m_rotation.m_col0)[arAtom.m_twistAxisInA].m_quad;
	const __m128 kRefB = (&arBaseB.m_rotation.m_col0)[arAtom.m_refAxisInB].m_quad;
	const __m128 kCross =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), _mm_shuffle_ps(kTwistA, kTwistA, 0xC9)),
			_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), _mm_shuffle_ps(kTwistA, kTwistA, 0xD2)));
	const __m128 kSq = _mm_mul_ps(kCross, kCross);
	const float fSinSq =
		_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)));
	// fcom / test ah, 5 / jp: no limit when the squared sine is below epsilon (NaN builds the limit).
	if (fSinSq < 1.1920929e-07f) {
		return 1;
	}
	uint32_t uiMode = arAtom.m_angleMeasurementMode.m_storage;
	const __m128 kCosV = _mm_mul_ps(kRefB, kTwistA);
	const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
	const float fSin = std::sqrt(static_cast<double>(fSinSq));
	const float fAngle = hkMath_atan2fApproximation(fSin, fCos) * buildJacobianFromConeLimitAtom_flipSignOrNot[uiMode] +
		rdata_10D33D8[uiMode];
	// The exe reads the sign at 0x10D33E4 - 4 * mode: the other entry of the {1, -1} table.
	const float fSign = buildJacobianFromConeLimitAtom_flipSignOrNot[1 - arAtom.m_angleMeasurementMode.m_storage];
	arInfo.m_tau.m_storage = arIn.m_tau * arAtom.m_angularLimitsTauFactor;
	const float fInvSin = 1.0 / fSin;
	arInfo.m_computedAngle.m_storage = fAngle;
	arInfo.m_constrainedDofW.m_quad = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(fInvSin), kCross), _mm_set1_ps(fSign));
	arInfo.m_min.m_storage = arAtom.m_minAngle;
	arInfo.m_max.m_storage = arAtom.m_maxAngle;
	uint8_t uiOffset = arAtom.m_memOffsetToAngleOffset;
	if (uiOffset > 1) {
		// The angle offset sits at a byte offset into the constraint runtime the atom gives: raw bytes.
		uint8_t* pRuntime = static_cast<uint8_t*>(arIn.m_constraintRuntime.m_storage);
		float* pAngleOffset = reinterpret_cast<float*>(pRuntime + uiOffset);
		float fOffset = *pAngleOffset;
		float fReference = -static_cast<hkpSolverResults*>(arIn.m_constraintRuntime.m_storage)->m_internalSolverData;
		float fError = fAngle - fOffset;
		float fRemaining = fReference - fError;
		float fHalfStep = arIn.m_frameDeltaTime * 0.5;
		float fScaledOffset = fOffset * 0.05000000074505806;
		float fStep = fHalfStep;
		// fcom / test ah, 5 / jp: the larger of the two; NaN keeps the half step.
		if (fHalfStep < fScaledOffset) {
			fStep = fScaledOffset;
		}
		float fNewOffset = fOffset - fStep;
		float fNewAngle = fError + fStep;
		float fMargin = 0.5 * (fHalfStep + (fStep + fStep));
		// fucompp / test ah, 0x44 / jnp: skipped only when the reference is exactly 0.
		if (fMargin < -fRemaining && 0.0f != fReference) {
			fNewOffset = fNewOffset - fRemaining;
			fNewAngle = fRemaining + fNewAngle;
		}
		// fcom / test ah, 5 / jnp: clamped up to epsilon unless above it (NaN is clamped).
		if (!(1.1920929e-07f < fNewOffset)) {
			fNewOffset = 1.1920929e-07f;
		}
		*pAngleOffset = fNewOffset;
		arInfo.m_computedAngle.m_storage = fNewAngle;
	}
	return 0;
}

// The PDB's file-local buildJacobianFromAngMotorAtom: the angle about base A's motor axis (moved by 2 pi towards the
// corresponding limit's reference when there is one), the motor's angular rows and the motor schema from
// hkCalcMotorData; a shift schema when the motor is missing or disabled (usercall: arBaseA, arBaseB and arOut on the
// stack, the atom in edi, arIn in esi, 0xD6DA00).
void buildJacobianFromAngMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpAngMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out) {
	// The motor's state sits at byte offsets into the constraint runtime the atom gives: raw bytes.
	uint8_t* pRuntime = static_cast<uint8_t*>(out.m_constraintRuntime.m_storage);
	uint8_t* pInitialized = pRuntime + baseB.m_initializedOffset;
	float* pPrevious = reinterpret_cast<float*>(pRuntime + baseB.m_previousTargetAngleOffset);
	hkpConstraintMotor* pMotor = baseB.m_motor;
	if (pMotor != nullptr && baseB.m_isEnabled.m_bool) {
		uint32_t uiAxis = baseB.m_motorAxis;
		// The exe adds its axis table's byte offsets to the frames' addresses: raw bytes.
		const uint8_t* pBytesA = reinterpret_cast<const uint8_t*>(&atom);
		const uint8_t* pBytesB = reinterpret_cast<const uint8_t*>(&in);
		const __m128 kA1 = _mm_load_ps(reinterpret_cast<const float*>(pBytesA + rdata_10D33C0[uiAxis + 1]));
		const __m128 kB2 = _mm_load_ps(reinterpret_cast<const float*>(pBytesB + rdata_10D33C0[uiAxis + 2]));
		const __m128 kCosV = _mm_mul_ps(kA1, kB2);
		const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
		const __m128 kB = (&in.m_rotation.m_col0)[uiAxis].m_quad;
		const __m128 kSinV =
			_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kB2, kB2, 0xC9)),
						   _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kB2, kB2, 0xD2))),
				kA1);
		const float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
		float fAngle = hkMath_atan2fApproximation(fCos, fSin);
		int16_t iLimitOffset = baseB.m_correspondingAngLimitSolverResultOffset;
		if (iLimitOffset != 0) {
			float fReference =
				reinterpret_cast<hkpSolverResults*>(static_cast<uint8_t*>(out.m_constraintRuntime.m_storage) +
					iLimitOffset)
					->m_internalSolverData *
				-1.0;
			// The angle is moved by 2 pi into (reference - pi, reference + pi]; each step is stored as a float.
			if (fReference - fAngle > 3.1415927410125732) {
				do {
					fAngle = fAngle + 6.2831854820251465;
				} while (fReference - fAngle > 3.1415927410125732);
			}
			if (fAngle - fReference > 3.1415927410125732) {
				do {
					fAngle = fAngle - 6.2831854820251465;
				} while (fAngle - fReference > 3.1415927410125732);
			}
		}
		if (*pInitialized == 0) {
			*pPrevious = fAngle;
		}
		hkpJacobianSchema* pJac = baseA.m_jacobianSchemas;
		// The motor input's slot is 16-aligned in the exe's realigned frame; the output's is at 8 mod 16.
		alignas(16) hkpConstraintMotorInput kMotorInput;
		hkpConstraintMotorOutput kMotorOutput;
		kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
		kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
		const __m128 kDir = (&atom.m_rotation.m_col0)[baseB.m_motorAxis].m_quad;
		const __m128 kNegDir = _mm_xor_ps(kDir, hkQuadRealNegateMask);
		const hkRotation& kRotA = out.m_bodyA->m_scratch123;
		const hkRotation& kRotB = out.m_bodyB->m_scratch123;
		pJac[0].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55), kRotA.m_col1.m_quad),
										_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00), kRotA.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA), kRotA.m_col2.m_quad));
		const __m128 kJacB =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55), kRotB.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00), kRotB.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA), kRotB.m_col2.m_quad));
		const __m128 kJacA = pJac[0].m_quad;
		pJac[1].m_quad = kJacB;
		const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacB, kJacB), out.m_bodyB->m_invMasses.m_quad),
			_mm_mul_ps(_mm_mul_ps(kJacA, kJacA), out.m_bodyA->m_invMasses.m_quad));
		const float fInvMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
		const float fInvMassEps = fInvMass + 1.1920928955078125e-07;
		const float fVirtualMass = 1.0 / fInvMassEps;
		pJac[0].m_quad[3] = fVirtualMass;
		kMotorInput.m_virtualMass = fVirtualMass;
		kMotorInput.m_stepInfo = &out;
		kMotorInput.m_lastResults = *static_cast<hkpSolverResults*>(out.m_constraintRuntime.m_storage);
		kMotorInput.m_deltaTarget = baseB.m_targetAngle - *pPrevious;
		kMotorInput.m_positionError = *pPrevious - fAngle;
		hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
		pJac[1].m_quad[3] = out.m_substepInvDeltaTime * kMotorOutput.m_targetPosition;
		hkpJacobianSchema* pSchema = baseA.m_jacobianSchemas;
		pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
		pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR;
		pSchema[2].m_quad[0] = kMotorOutput.m_maxForce * out.m_microStepDeltaTime;
		baseA.m_jacobianSchemas = pSchema + 4;
		pSchema[2].m_quad[1] = kMotorOutput.m_minForce * out.m_microStepDeltaTime;
		pSchema[2].m_quad[2] = kMotorOutput.m_targetVelocity;
		pSchema[2].m_quad[3] = kMotorOutput.m_tau;
		pSchema[3].m_quad[0] = kMotorOutput.m_damping;
		// fucompp / test ah, 0x44 / jp: a target that differs (or is NaN) is stored.
		if (baseB.m_targetAngle != *pPrevious || *pInitialized == 0) {
			*pPrevious = baseB.m_targetAngle;
			*pInitialized = 1;
		}
	} else {
		hkpJacobianSchema* pShift = baseA.m_jacobianSchemas;
		pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
		pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
		pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
		baseA.m_jacobianSchemas = pShift + 1;
	}
}

// hkSolverBuildJacobianFromAtomsNotContact's TYPE_RAGDOLL_MOTOR case (inlined there) out of line: per motor the
// current angle about the bisector of the bodies' axes and the target angle about the target frame's, the previous
// target kept in the runtime, one angular motor schema from hkCalcMotorData's output; a shift schema per missing
// motor, one over all three when the atom is disabled. The angles and the motor values are x87 float copies.
void buildJacobianFromRagdollMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpRagdollMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out) {
	// The runtime is raw bytes the atom addresses by byte offsets.
	uint8_t* pRuntime = static_cast<uint8_t*>(out.m_constraintRuntime.m_storage);
	uint8_t* pInitialized = pRuntime + baseB.m_initializedOffset;
	float* pPrevious = reinterpret_cast<float*>(pRuntime + baseB.m_previousTargetAnglesOffset);
	if (!baseB.m_isEnabled.m_bool) {
		hkpJacobianSchema* pShift = baseA.m_jacobianSchemas;
		pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 3;
		pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
		pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
		baseA.m_jacobianSchemas = pShift + 1;
		return;
	}
	hkMatrix3 kTarget;
	hkMatrix3_setMulMat3Mat3(&kTarget, &out.m_transformB->m_rotation, &baseB.m_target_bRca);
	hkpConstraintMotorInput kMotorInput;
	hkpConstraintMotorOutput kMotorOutput;
	kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
	kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
	kMotorInput.m_stepInfo = &out;
	// The exe's two index triples on its stack: the axis each motor turns about, and its reference axis.
	const int32_t aiAxis[3] = {0, 1, 2};
	const int32_t aiRef[3] = {1, 0, 0};
	const hkVector4* pColsA = &atom.m_rotation.m_col0;
	const hkVector4* pColsB = &in.m_rotation.m_col0;
	const hkVector4* pTargetCols = &kTarget.m_col0;
	for (int32_t i = 0; i < 3; ++i) {
		hkpConstraintMotor* pMotor = baseB.m_motors[i];
		if (pMotor == nullptr) {
			hkpJacobianSchema* pShift = baseA.m_jacobianSchemas;
			pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
			pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
			pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
			baseA.m_jacobianSchemas = pShift + 1;
			continue;
		}
		const int32_t iAxis = aiAxis[i];
		const int32_t iRef = aiRef[i];
		// The current angle: about the bisector of the two bodies' axes, from the reference axes.
		__m128 kAxis = pColsB[iAxis].m_quad;
		__m128 kSum = _mm_add_ps(pColsA[iAxis].m_quad, kAxis);
		__m128 kSq = _mm_mul_ps(kSum, kSum);
		float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
		if (fLength > 1.000000016862384e-16) {
			const float fInvLength = 1.0 / fLength;
			kAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
		}
		__m128 kRefB = pColsB[iRef].m_quad;
		__m128 kAxisD2 = _mm_shuffle_ps(kAxis, kAxis, 0xD2);
		__m128 kAxisC9 = _mm_shuffle_ps(kAxis, kAxis, 0xC9);
		__m128 kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
			_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
		__m128 kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
			_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
		const __m128 kRefA = pColsA[iRef].m_quad;
		__m128 kCosV = _mm_mul_ps(kPerp, kRefA);
		float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
		__m128 kSinV = _mm_mul_ps(kPerp2, kRefA);
		float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
		const float fCurrent = hkMath_atan2fApproximation(fCos, fSin);
		// The target angle: the same with the target frame in place of the first body's.
		__m128 kTargetAxis = pColsB[iAxis].m_quad;
		kSum = _mm_add_ps(pTargetCols[iAxis].m_quad, kTargetAxis);
		kSq = _mm_mul_ps(kSum, kSum);
		fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
		if (fLength > 1.000000016862384e-16) {
			const float fInvLength = 1.0 / fLength;
			kTargetAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
		}
		kAxisD2 = _mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xD2);
		kAxisC9 = _mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xC9);
		kRefB = pColsB[iRef].m_quad;
		kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
			_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
		kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
			_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
		const __m128 kRefT = pTargetCols[iRef].m_quad;
		kCosV = _mm_mul_ps(kPerp, kRefT);
		fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
		kSinV = _mm_mul_ps(kPerp2, kRefT);
		fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
		const float fTarget = hkMath_atan2fApproximation(fCos, fSin);
		if (pInitialized[i] == 0) {
			pPrevious[i] = fCurrent;
			pInitialized[i] = 1;
		}
		const __m128 kNegDir = _mm_xor_ps(kTargetAxis, hkQuadRealNegateMask);
		const hkRotation& kRotA = out.m_bodyA->m_scratch123;
		const hkRotation& kRotB = out.m_bodyB->m_scratch123;
		hkpJacobianSchema* pJac = baseA.m_jacobianSchemas;
		pJac[0].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0x55), kRotA.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0x00), kRotA.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xAA), kRotA.m_col2.m_quad));
		const __m128 kJacB =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55), kRotB.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00), kRotB.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA), kRotB.m_col2.m_quad));
		pJac[1].m_quad = kJacB;
		const __m128 kJacA = pJac[0].m_quad;
		const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacB, kJacB), out.m_bodyB->m_invMasses.m_quad),
			_mm_mul_ps(_mm_mul_ps(kJacA, kJacA), out.m_bodyA->m_invMasses.m_quad));
		const float fInvMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
		const float fInvMassEps = fInvMass + 1.1920928955078125e-07;
		const float fVirtualMass = 1.0 / fInvMassEps;
		pJac[0].m_quad[3] = fVirtualMass;
		kMotorInput.m_virtualMass = fVirtualMass;
		kMotorInput.m_lastResults = static_cast<hkpSolverResults*>(out.m_constraintRuntime.m_storage)[i];
		kMotorInput.m_deltaTarget = fTarget - pPrevious[i];
		kMotorInput.m_positionError = pPrevious[i] - fCurrent;
		hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
		pJac[1].m_quad[3] = out.m_substepInvDeltaTime * kMotorOutput.m_targetPosition;
		hkpJacobianSchema* pSchema = baseA.m_jacobianSchemas;
		pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
		pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR;
		pSchema[2].m_quad[0] = kMotorOutput.m_maxForce * out.m_microStepDeltaTime;
		baseA.m_jacobianSchemas = pSchema + 4;
		pSchema[2].m_quad[1] = kMotorOutput.m_minForce * out.m_microStepDeltaTime;
		pSchema[2].m_quad[2] = kMotorOutput.m_targetVelocity;
		pSchema[2].m_quad[3] = kMotorOutput.m_tau;
		pSchema[3].m_quad[0] = kMotorOutput.m_damping;
		// fucomp / test ah, 0x44 / jnp: the target is stored unless it equals the previous one.
		if (fTarget != pPrevious[i]) {
			pPrevious[i] = fTarget;
		}
	}
}

// The PDB's file-local buildJacobianFromMassChangerModifier: the child atoms built with both bodies' inverse masses
// scaled by the atom's factors (a set-mass schema before them and one restoring the bodies' own after); the query's
// transforms are not taken (usercall: arIn and arOut on the stack, the atom in eax, 0xD6ECB0). The exe passes the
// hkBool for the contact builder by storing its byte into the stack dword holding the child size and pushing the
// whole dword.
void buildJacobianFromMassChangerModifier(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMassChangerModifierConstraintAtom& arAtom) {
	hkpConstraintQueryIn kModifierIn = arIn;
	hkVector4 kFactorA = arAtom.m_factorA;
	hkpConstraintAtom* pChild = arAtom.m_child;
	hkVector4 kFactorB = arAtom.m_factorB;
	int32_t iChildSize = arAtom.m_childSize;
	hkpVelocityAccumulator kAccA;
	hkpVelocityAccumulator kAccB;
	kAccA.copyVelocityAndScratch(*arIn.m_bodyA);
	kAccB.copyVelocityAndScratch(*arIn.m_bodyB);
	kAccA.m_invMasses.m_quad = _mm_mul_ps(kAccA.m_invMasses.m_quad, kFactorA.m_quad);
	kModifierIn.m_bodyA = &kAccA;
	kAccB.m_invMasses.m_quad = _mm_mul_ps(kAccB.m_invMasses.m_quad, kFactorB.m_quad);
	kModifierIn.m_bodyB = &kAccB;
	hkSetInvMassBuildJacobian(kAccA.m_invMasses, kAccB.m_invMasses, arOut);
	if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
		hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild), kModifierIn,
			hkBool{1}, arOut);
	} else {
		hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, arOut);
	}
	hkSetInvMassBuildJacobian(arIn.m_bodyA->m_invMasses, arIn.m_bodyB->m_invMasses, arOut);
}

// The PDB's file-local buildJacobianFromLinMotorAtom: a linear velocity motor along base B's motor axis at base A's
// pivot, the motor's position error the stored previous target less the pivots' separation along the axis; a shift
// schema when the motor is disabled or missing (usercall: arIn, arBaseA, arBaseB and arOut on the stack, the atom in
// esi, 0xD6F300).
void buildJacobianFromLinMotorAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinMotorConstraintAtom& out) {
	// The motor's state sits at byte offsets into the constraint runtime the atom gives: raw bytes.
	uint8_t* pRuntime = static_cast<uint8_t*>(atom.m_constraintRuntime.m_storage);
	uint8_t* pInitialized = pRuntime + out.m_initializedOffset;
	float* pPrevious = reinterpret_cast<float*>(pRuntime + out.m_previousTargetPositionOffset);
	hkpConstraintMotor* pMotor;
	if (out.m_isEnabled.m_bool && (pMotor = out.m_motor) != nullptr) {
		if (*pInitialized == 0) {
			*pPrevious = out.m_targetPosition;
		}
		hkpJacobianSchema* pJac = baseB.m_jacobianSchemas;
		// The motor input's slot is 16-aligned in the exe's realigned frame; the output's is at 8 mod 16.
		alignas(16) hkpConstraintMotorInput kMotorInput;
		hkpConstraintMotorOutput kMotorOutput;
		kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
		kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
		hk1dLinearVelocityMotorBeginJacobian((&baseA.m_rotation.m_col0)[out.m_motorAxis], in.m_translation, atom, pJac,
			&kMotorInput);
		kMotorInput.m_stepInfo = &atom;
		kMotorInput.m_lastResults = *static_cast<hkpSolverResults*>(atom.m_constraintRuntime.m_storage);
		kMotorInput.m_deltaTarget = out.m_targetPosition - *pPrevious;
		const __m128 kDot = _mm_mul_ps(_mm_sub_ps(in.m_translation.m_quad, baseA.m_translation.m_quad),
			(&baseA.m_rotation.m_col0)[out.m_motorAxis].m_quad);
		const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0x55), kDot)));
		kMotorInput.m_positionError = *pPrevious - fDistance;
		hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
		hk1dLinearVelocityMotorCommitJacobian(&kMotorOutput, atom, pJac, baseB);
		// fucompp / test ah, 0x44 / jp: a target that differs (or is NaN) is stored.
		if (out.m_targetPosition != *pPrevious || *pInitialized == 0) {
			*pPrevious = out.m_targetPosition;
			*pInitialized = 1;
		}
	} else {
		hkpJacobianSchema* pShift = baseB.m_jacobianSchemas;
		pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
		pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
		pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
		baseB.m_jacobianSchemas = pShift + 1;
	}
}

// Not named in the PDB or IDA; name chosen here. An out-of-line copy of the pulley case of
// hkSolverBuildJacobianFromAtomsNotContact (the PDB's file-local buildJacobianFromPulleyAtom has two such copies, this
// one and 0xD6E6B0): the 1D pulley schema (type 11) from the pivots, the fixed pivots, the rope length and the
// leverage; the atom cursor is not advanced (usercall: arIn and arOut on the stack, apData in edi, 0xD6E300).
void buildJacobianPulleyFromData(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkVector4* apData) {
	hkpJacobianSchema* pJac = arOut.m_jacobianSchemas;
	const __m128 kPivotA = apData[0].m_quad;
	const __m128 kPivotB = apData[1].m_quad;
	const __m128 kFixedA = apData[2].m_quad;
	const __m128 kFixedB = apData[3].m_quad;
	__m128 kToA = _mm_sub_ps(kFixedA, kPivotA);
	__m128 kToB = _mm_sub_ps(kPivotB, kFixedB);
	__m128 kSq = _mm_mul_ps(kToA, kToA);
	__m128 kLengthSq = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
	const __m128 kIdentity = hkTransformIdentity.m_rotation.m_col0.m_quad;
	hkpVelocityAccumulator* pBodyA = arIn.m_bodyA;
	hkpVelocityAccumulator* pBodyB = arIn.m_bodyB;
	__m128 kDirA;
	// fcom / test ah, 5 / jp: the identity's first column unless epsilon is below the length.
	if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
		// The SDK's normalize3 with the rsqrtss estimate and one Newton step (the exe's 0.0f at 0x1011D78, 3.0f at
		// 0x1017718 and 0.5f at 0x1016248); a zero length gives zero.
		__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
		__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
		__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
		__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
		kDirA = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToA);
	} else {
		kDirA = kIdentity;
	}
	kSq = _mm_mul_ps(kToB, kToB);
	kLengthSq = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
	__m128 kDirB;
	if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
		__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
		__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
		__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
		__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
		kDirB = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToB);
	} else {
		kDirB = _mm_xor_ps(kIdentity, hkQuadRealNegateMask);
	}
	float fRopeLength = apData[4].m_quad[0];
	float fLeverage = apData[4].m_quad[1];
	__m128 kJacLinB = _mm_mul_ps(_mm_set1_ps(fLeverage), kDirB);
	float fInvDeltaTime = arIn.m_substepInvDeltaTime;
	float fVirtMassFactor = arIn.m_virtMassFactor;
	pJac[1].m_quad = kJacLinB;
	pJac[0].m_quad = kDirA;
	__m128 kArmA = _mm_sub_ps(kPivotA, pBodyA->m_scratch0.m_quad);
	__m128 kArmB = _mm_sub_ps(kPivotB, pBodyB->m_scratch0.m_quad);
	__m128 kAngA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xC9), _mm_shuffle_ps(kDirA, kDirA, 0xD2)),
		_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xD2), _mm_shuffle_ps(kDirA, kDirA, 0xC9)));
	__m128 kAngB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xD2), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xC9)),
		_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xC9), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xD2)));
	const hkRotation& kRotA = pBodyA->m_scratch123;
	const hkRotation& kRotB = pBodyB->m_scratch123;
	pJac[2].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x55), kRotA.m_col1.m_quad),
									_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x00), kRotA.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0xAA), kRotA.m_col2.m_quad));
	__m128 kJacAngB = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x55), kRotB.m_col1.m_quad),
									 _mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x00), kRotB.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0xAA), kRotB.m_col2.m_quad));
	__m128 kJacAngA = pJac[2].m_quad;
	pJac[3].m_quad = kJacAngB;
	__m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacAngA, kJacAngA), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kJacAngB, kJacAngB), pBodyB->m_invMasses.m_quad));
	float fAngularMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
	float fLeverageSq = fLeverage * fLeverage;
	float fLinearMass =
		fLeverageSq * pBodyB->m_invMasses.m_quad[3] + pBodyA->m_invMasses.m_quad[3] + 1.1920928955078125e-07;
	__m128 kSegA = _mm_sub_ps(kFixedA, kPivotA);
	kSq = _mm_mul_ps(kSegA, kSegA);
	float fLengthA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
	float fInvMass = fLinearMass + fAngularMass;
	__m128 kSegB = _mm_sub_ps(kPivotB, kFixedB);
	kSq = _mm_mul_ps(kSegB, kSegB);
	float fLengthB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
	pJac[2].m_quad[3] = fVirtMassFactor / fInvMass;
	float fRope = fLeverage * fLengthB + fLengthA;
	float fError = fRope - fRopeLength;
	pJac[0].m_quad[3] = fError * fInvDeltaTime;
	hkpJacobianSchema* pSchema = arOut.m_jacobianSchemas;
	arOut.m_jacobianSchemas = pSchema + 4;
	pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
	pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY;
}

// Not named in the PDB or IDA; name chosen here. The other out-of-line copy of the pulley case of
// hkSolverBuildJacobianFromAtomsNotContact (see 0xD6E300), with the pivots from the bodies' world transforms and the
// rest from the atom (usercall: arLocalToWorldA in ecx, arLocalToWorldB and arOut on the stack, the atom in eax, arIn
// in edi, 0xD6E6B0).
void buildJacobianPulley(const hkTransform& arLocalToWorldA, const hkTransform& arLocalToWorldB,
	hkpConstraintQueryOut& arOut, const hkpPulleyConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn) {
	float fRopeLength = arAtom.m_ropeLength;
	const __m128 kPivotB = arLocalToWorldB.m_translation.m_quad;
	const __m128 kFixedB = arAtom.m_fixedPivotBinWorld.m_quad;
	float fLeverage = arAtom.m_leverageOnBodyB;
	const __m128 kPivotA = arLocalToWorldA.m_translation.m_quad;
	const __m128 kFixedA = arAtom.m_fixedPivotAinWorld.m_quad;
	hkpVelocityAccumulator* pBodyA = arIn.m_bodyA;
	__m128 kToA = _mm_sub_ps(kFixedA, kPivotA);
	__m128 kToB = _mm_sub_ps(kPivotB, kFixedB);
	__m128 kSq = _mm_mul_ps(kToA, kToA);
	__m128 kLengthSq = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
	hkpJacobianSchema* pJac = arOut.m_jacobianSchemas;
	hkpVelocityAccumulator* pBodyB = arIn.m_bodyB;
	__m128 kDirA;
	// fcom / test ah, 5 / jp: the identity's first column unless epsilon is below the length.
	if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
		// The SDK's normalize3 with the rsqrtss estimate and one Newton step (the exe's 0.0f at 0x1011D78, 3.0f at
		// 0x1017718 and 0.5f at 0x1016248); a zero length gives zero.
		__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
		__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
		__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
		__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
		kDirA = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToA);
	} else {
		kDirA = hkTransformIdentity.m_rotation.m_col0.m_quad;
	}
	kSq = _mm_mul_ps(kToB, kToB);
	kLengthSq = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
	__m128 kDirB;
	if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
		__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
		__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
		__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
		__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
		kDirB = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToB);
	} else {
		kDirB = _mm_xor_ps(hkTransformIdentity.m_rotation.m_col0.m_quad, hkQuadRealNegateMask);
	}
	__m128 kJacLinB = _mm_mul_ps(_mm_set1_ps(fLeverage), kDirB);
	float fInvDeltaTime = arIn.m_substepInvDeltaTime;
	float fVirtMassFactor = arIn.m_virtMassFactor;
	pJac[1].m_quad = kJacLinB;
	pJac[0].m_quad = kDirA;
	__m128 kArmA = _mm_sub_ps(kPivotA, pBodyA->m_scratch0.m_quad);
	__m128 kArmB = _mm_sub_ps(kPivotB, pBodyB->m_scratch0.m_quad);
	__m128 kAngA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xC9), _mm_shuffle_ps(kDirA, kDirA, 0xD2)),
		_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xD2), _mm_shuffle_ps(kDirA, kDirA, 0xC9)));
	__m128 kAngB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xD2), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xC9)),
		_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xC9), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xD2)));
	const hkRotation& kRotA = pBodyA->m_scratch123;
	const hkRotation& kRotB = pBodyB->m_scratch123;
	pJac[2].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x55), kRotA.m_col1.m_quad),
									_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x00), kRotA.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0xAA), kRotA.m_col2.m_quad));
	__m128 kJacAngB = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x55), kRotB.m_col1.m_quad),
									 _mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x00), kRotB.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0xAA), kRotB.m_col2.m_quad));
	__m128 kJacAngA = pJac[2].m_quad;
	pJac[3].m_quad = kJacAngB;
	__m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacAngA, kJacAngA), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kJacAngB, kJacAngB), pBodyB->m_invMasses.m_quad));
	float fAngularMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
	float fLeverageSq = fLeverage * fLeverage;
	float fLinearMass =
		fLeverageSq * pBodyB->m_invMasses.m_quad[3] + pBodyA->m_invMasses.m_quad[3] + 1.1920928955078125e-07;
	// The exe takes both lengths from the pivot-to-fixed vectors computed at the start.
	kSq = _mm_mul_ps(kToA, kToA);
	float fLengthA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
	float fInvMass = fLinearMass + fAngularMass;
	kSq = _mm_mul_ps(kToB, kToB);
	float fLengthB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
	pJac[2].m_quad[3] = fVirtMassFactor / fInvMass;
	float fRope = fLeverage * fLengthB + fLengthA;
	float fError = fRope - fRopeLength;
	pJac[0].m_quad[3] = fError * fInvDeltaTime;
	hkpJacobianSchema* pSchema = arOut.m_jacobianSchemas;
	arOut.m_jacobianSchemas = pSchema + 4;
	pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
	pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY;
}

// The PDB's file-local buildJacobianFromSoftContactModifier: the output's cursor back over the header schema, the
// child built with the query's virtual mass factor, tau and damping scaled by the atom's tau, after each contact
// point's allowed penetration is lowered to the soft contact's speed limit over one substep; the query's transforms
// are not taken (usercall: arIn in edx, the atom and arOut on the stack, 0xD6EA70). The exe passes the hkBool for the
// contact builder by storing its byte into a stack dword of its own frame and pushing the whole dword.
void buildJacobianFromSoftContactModifier(const hkpConstraintQueryIn& arIn,
	const hkpSoftContactModifierConstraintAtom& arAtom, hkpConstraintQueryOut& arOut) {
	arOut.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	float fTau = arAtom.m_tau;
	int32_t iChildSize = arAtom.m_childSize;
	hkpConstraintQueryIn kModifierIn = arIn;
	kModifierIn.m_virtMassFactor = arIn.m_virtMassFactor * fTau;
	kModifierIn.m_tau = arIn.m_tau * fTau;
	kModifierIn.m_damping = fTau * arIn.m_damping;
	// The contact atom under the modifier chain.
	hkpConstraintAtom* pInner = arAtom.m_child;
	while (pInner->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
		pInner = static_cast<hkpModifierConstraintAtom*>(pInner)->m_child;
	}
	hkpSimpleContactConstraintAtom* pContact = static_cast<hkpSimpleContactConstraintAtom*>(pInner);
	int32_t iNumPoints = pContact->m_numContactPoints;
	// The contact points and their strided properties follow the contact atom in its byte stream: raw bytes.
	hkContactPoint* pPoints = reinterpret_cast<hkContactPoint*>(pContact + 1);
	uint8_t* pProperties = reinterpret_cast<uint8_t*>(pPoints + pContact->m_numReservedContactPoints);
	uint32_t uiStriding = pContact->m_contactPointPropertiesStriding;
	for (int32_t i = 0; i < iNumPoints; ++i) {
		float fDistance = pPoints[i].m_separatingNormal.m_quad[3];
		float fAllowed = arAtom.m_maxAcceleration * arIn.m_microStepDeltaTime * arIn.m_substepDeltaTime / arAtom.m_tau;
		hkpContactPointProperties* pProperty = reinterpret_cast<hkpContactPointProperties*>(pProperties);
		// fcomp / test ah, 0x41 / jne: stored only when the old value is above the new one.
		if (pProperty->m_internalDataA > fAllowed + fDistance) {
			pProperty->m_internalDataA = fAllowed + fDistance;
		}
		pProperties += uiStriding;
	}
	hkpConstraintAtom* pChild = arAtom.m_child;
	if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
		hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild), kModifierIn,
			hkBool{1}, arOut);
	} else {
		hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, arOut);
	}
}

// The PDB's file-local buildJacobianFromCenterOfMassChangerModifier: the child built with both bodies' centres moved
// by the atom's displacements (rotated into world space), their linear velocities corrected for the move and
// center-of-mass schemas around it; the query's transforms are not taken (cdecl, 0xD6EDF0). The exe passes the
// hkBool for the contact builder by storing its byte into the stack dword holding the child size and pushing the
// whole dword.
void buildJacobianFromCenterOfMassChangerModifier(const hkpCenterOfMassChangerModifierConstraintAtom& arAtom,
	const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut) {
	hkpConstraintQueryIn kModifierIn = arIn;
	int32_t iChildSize = arAtom.m_childSize;
	hkpConstraintAtom* pChild = arAtom.m_child;
	hkVector4 kDisplacementA;
	hkVector4 kDisplacementB;
	kDisplacementA.setRotatedDir(arIn.m_transformA->m_rotation, arAtom.m_displacementA);
	kDisplacementB.setRotatedDir(arIn.m_transformB->m_rotation, arAtom.m_displacementB);
	hkpVelocityAccumulator kAccA;
	hkpVelocityAccumulator kAccB;
	kAccA.copyVelocityAndScratch(*arIn.m_bodyA);
	kAccB.copyVelocityAndScratch(*arIn.m_bodyB);
	kAccA.m_scratch0.m_quad = _mm_add_ps(kAccA.m_scratch0.m_quad, kDisplacementA.m_quad);
	kAccB.m_scratch0.m_quad = _mm_add_ps(kAccB.m_scratch0.m_quad, kDisplacementB.m_quad);
	kModifierIn.m_bodyB = &kAccB;
	kModifierIn.m_bodyA = &kAccA;
	hkMatrix3 kSkewA;
	hkMatrix3 kSkewB;
	kSkewA.setCrossSkewSymmetric(kDisplacementA);
	kSkewB.setCrossSkewSymmetric(kDisplacementB);
	hkMatrix3 kCopy;
	kCopy.m_col0 = kSkewA.m_col0;
	kCopy.m_col1 = kSkewA.m_col1;
	kCopy.m_col2 = kSkewA.m_col2;
	kSkewA.setMulInverse(kCopy, kAccA.m_scratch123);
	kCopy.m_col0 = kSkewB.m_col0;
	kCopy.m_col1 = kSkewB.m_col1;
	kCopy.m_col2 = kSkewB.m_col2;
	kSkewB.setMulInverse(kCopy, kAccB.m_scratch123);
	// The negated matrices: copies scaled by -1.0f (the exe's float at 0x1012054, loaded with movss into one slot).
	hkSimdReal kMinusOne;
	hkMatrix3 kNegSkewA;
	kNegSkewA.m_col0 = kSkewA.m_col0;
	kNegSkewA.m_col1 = kSkewA.m_col1;
	kNegSkewA.m_col2 = kSkewA.m_col2;
	kMinusOne.m_real = _mm_set_ss(-1.0f);
	kNegSkewA.mul(kMinusOne);
	hkMatrix3 kNegSkewB;
	kNegSkewB.m_col0 = kSkewB.m_col0;
	kNegSkewB.m_col1 = kSkewB.m_col1;
	kNegSkewB.m_col2 = kSkewB.m_col2;
	kMinusOne.m_real = _mm_set_ss(-1.0f);
	kNegSkewB.mul(kMinusOne);
	hkVector4 kLinearA;
	hkVector4 kLinearB;
	kLinearA.setMul3(kNegSkewA, kAccA.m_angularVel);
	kLinearB.setMul3(kNegSkewB, kAccB.m_angularVel);
	kAccA.m_linearVel.m_quad = _mm_add_ps(kAccA.m_linearVel.m_quad, kLinearA.m_quad);
	kAccB.m_linearVel.m_quad = _mm_add_ps(kAccB.m_linearVel.m_quad, kLinearB.m_quad);
	hkSetCenterOfMassBuildJacobian(kNegSkewA, kNegSkewB, arOut);
	if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
		hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild), kModifierIn,
			hkBool{1}, arOut);
	} else {
		hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, arOut);
	}
	hkSetCenterOfMassBuildJacobian(kSkewA, kSkewB, arOut);
}

// The PDB's file-local buildJacobianFromMovingSurfaceModifier: a header schema for the contact's solver results (the
// output's cursor stepped back over the caller's), then the contact built with the moving body's velocity (a copy
// of its accumulator the query is pointed at) raised by the surface velocity's tangential part, and that part taken
// off again after it; the query's transforms are not taken (usercall: arIn and arOut on the stack, the atom in eax,
// 0xD6F0C0). This copy points the caller's query (arIn) at its accumulator copy. The exe passes the hkBool for the
// contact builder by storing its byte into a stack dword of its own frame and pushing the whole dword.
void buildJacobianFromMovingSurfaceModifier(hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMovingSurfaceModifierConstraintAtom& arAtom) {
	hkVector4 kVelocity = arAtom.m_velocity;
	hkpConstraintQueryIn kModifierIn = arIn;
	hkpSimpleContactConstraintAtom* pContact = static_cast<hkpSimpleContactConstraintAtom*>(arAtom.m_child);
	arOut.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	// EXE-INLINED 0xD6EA30
	hkpJacobianSchema* pHeader = arOut.m_jacobianSchemas;
	uint8_t uiStriding = pContact->m_contactPointPropertiesStriding;
	// The contact points and their properties follow the contact atom in its byte stream: raw bytes.
	hkContactPoint* pPoints = reinterpret_cast<hkContactPoint*>(pContact + 1);
	void* pSolverResults = pPoints + pContact->m_numReservedContactPoints;
	uint32_t uiBodyBIndex = arIn.m_accumulatorBIndex.m_storage;
	uint32_t uiBodyAIndex = arIn.m_accumulatorAIndex.m_storage;
	hkpConstraintInstance* pConstraintInstance = arIn.m_constraintInstance;
	pHeader->m_quad[0] = pHeader->m_quad[0] + 1.1920928955078125e-07;
	pHeader->getHeader().m_bodyAIndex = static_cast<uint16_t>(uiBodyAIndex);
	pHeader->getHeader().m_bodyBIndex = static_cast<uint16_t>(uiBodyBIndex);
	pHeader->getHeader().m_solverResultInMainMemory = static_cast<hkpSolverResults*>(pSolverResults);
	pHeader->getHeader().m_solverResultStriding = uiStriding;
	pHeader->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
	pHeader->getHeader().m_constraintInstance = pConstraintInstance;
	arOut.m_jacobianSchemas = pHeader + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	// The exe keeps this flag as a byte: whether the moving body is B (body A is the fixed one).
	uint8_t bIsBodyB = arIn.m_accumulatorAIndex.m_storage == 0;
	hkpVelocityAccumulator kAcc;
	__m128 kV;
	if (!bIsBodyB) {
		kAcc = *arIn.m_bodyA;
		kV = kVelocity.m_quad;
		arIn.m_bodyA = &kAcc;
	} else {
		kAcc = *arIn.m_bodyB;
		kV = kVelocity.m_quad;
		arIn.m_bodyB = &kAcc;
		kV = _mm_xor_ps(kV, rdata_10C4C00);
	}
	hkVector4 kNormal = pPoints[0].m_separatingNormal;
	const __m128 kN = kNormal.m_quad;
	const __m128 kDotV = _mm_mul_ps(kN, kV);
	const __m128 kDot =
		_mm_add_ss(_mm_shuffle_ps(kDotV, kDotV, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDotV, kDotV, 0x55), kDotV));
	int32_t iIsBodyB = bIsBodyB != 0;
	hkVector4 kTangential;
	kTangential.m_quad =
		_mm_xor_ps(_mm_sub_ps(kV, _mm_mul_ps(_mm_shuffle_ps(kDot, kDot, 0), kN)), hkQuadRealNegateMask);
	hkAddVelocityBuildJacobian(kTangential, iIsBodyB, arOut);
	kAcc.m_linearVel.m_quad = _mm_add_ps(kAcc.m_linearVel.m_quad, kTangential.m_quad);
	hkSimpleContactConstraintDataBuildJacobian(pContact, kModifierIn, hkBool{0}, arOut);
	kTangential.m_quad = _mm_xor_ps(hkQuadRealNegateMask, kTangential.m_quad);
	hkAddVelocityBuildJacobian(kTangential, iIsBodyB, arOut);
}

// Each row: the axis as the linear part, the pivots' lever arms crossed with it and taken into each body's inertia
// space (m_scratch123) as the angular parts, the inverse of the effective mass (plus FLT_EPSILON) in the second
// angular part's w, the virtual mass factor over it in the first's, and the pivots' separation along the axis times
// the rhs factor in the linear part's w. The exe reloads both step factors on every row.
void hkBallSocketConstraintBuildJacobian_noSchema(const hkVector4& posA, const hkVector4& posB,
	const hkpConstraintQueryIn& in, hkp1Lin2AngJacobian* jac) {
	const hkpVelocityAccumulator* pBodyA = in.m_bodyA;
	const hkpVelocityAccumulator* pBodyB = in.m_bodyB;
	for (int32_t i = 0; i < 3; ++i) {
		const float fVirtMassFactor = in.m_virtMassFactor;
		const float fRhsFactor = in.m_rhsFactor;
		hkp1Lin2AngJacobian& rJac = jac[i];
		const __m128 kDir = hkTransformIdentity.m_rotation.getColumn(i).m_quad;
		rJac.m_linear0.m_quad = kDir;
		const __m128 kArmA = _mm_sub_ps(posA.m_quad, pBodyA->m_scratch0.m_quad);
		const __m128 kArmB = _mm_sub_ps(posB.m_quad, pBodyB->m_scratch0.m_quad);
		const __m128 kDirYzx = _mm_shuffle_ps(kDir, kDir, 201);
		const __m128 kDirZxy = _mm_shuffle_ps(kDir, kDir, 210);
		// cross(armA, dir) and cross(dir, armB).
		const __m128 kCrossA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 201), kDirZxy),
			_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 210), kDirYzx));
		const __m128 kCrossB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 210), kDirYzx),
			_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 201), kDirZxy));
		const __m128 kAngA =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0), pBodyA->m_scratch123.m_col0.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 85), pBodyA->m_scratch123.m_col1.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 170), pBodyA->m_scratch123.m_col2.m_quad));
		rJac.m_angular[0].m_quad = kAngA;
		const __m128 kAngB =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 85), pBodyB->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0), pBodyB->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 170), pBodyB->m_scratch123.m_col2.m_quad));
		rJac.m_angular[1].m_quad = kAngB;
		const __m128 kInvMassesA = pBodyA->m_invMasses.m_quad;
		const __m128 kInvMassesB = pBodyB->m_invMasses.m_quad;
		const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAngB, kAngB), kInvMassesB),
			_mm_mul_ps(_mm_mul_ps(kAngA, kAngA), kInvMassesA));
		// The three angular lanes and the sum of the inverse masses' w lanes, added across.
		const __m128 kTerms = _mm_shuffle_ps(kMass, _mm_unpackhi_ps(kMass, _mm_add_ps(kInvMassesA, kInvMassesB)), 196);
		const __m128 kPairs = _mm_add_ps(_mm_shuffle_ps(kTerms, kTerms, 78), kTerms);
		const float fDiag =
			static_cast<float>(_mm_shuffle_ps(kPairs, kPairs, 177)[0] + kPairs[0]) + 1.1920928955078125e-07;
		rJac.m_angular[1].m_quad[3] = fDiag;
		rJac.m_angular[0].m_quad[3] = fVirtMassFactor / fDiag;
		const __m128 kSeparation = _mm_mul_ps(_mm_sub_ps(posB.m_quad, posA.m_quad), kDir);
		rJac.m_linear0.m_quad[3] =
			static_cast<float>(kSeparation[2] + static_cast<float>(kSeparation[1] + kSeparation[0])) * fRhsFactor;
	}
}

// The SDK's: the motor's axis rotated into each body's frame (body B's negated), and the virtual mass from the
// bodies' inverse inertias along those rows. The exe sums the lanes in SSE and adds the double epsilon on the x87.
hkp1dBilateralConstraintStatus* hk1dAngularVelocityMotorBeginJacobian(const hkVector4& directionOfConstraint,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkp1dBilateralConstraintStatus& statusOut) {
	const hkpVelocityAccumulator* pBodyB = in.m_bodyB;
	const hkpVelocityAccumulator* pBodyA = in.m_bodyA;
	const __m128 kDir = directionOfConstraint.m_quad;
	const __m128 kNegDir = _mm_xor_ps(kDir, hkQuadRealNegateMask);
	const hkRotation& rotA = pBodyA->m_scratch123;
	const hkRotation& rotB = pBodyB->m_scratch123;
	jac.m_angular[0].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55), rotA.m_col1.m_quad),
											 _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00), rotA.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA), rotA.m_col2.m_quad));
	const __m128 kAng1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55), rotB.m_col1.m_quad),
										_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00), rotB.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA), rotB.m_col2.m_quad));
	jac.m_angular[1].m_quad = kAng1;
	const __m128 kAng0 = jac.m_angular[0].m_quad;
	const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAng0, kAng0), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kAng1, kAng1), pBodyB->m_invMasses.m_quad));
	// z + (y + x): the exe's addss, written as _ps operations on lane 0.
	const __m128 kSum =
		_mm_add_ps(_mm_shuffle_ps(kMass, kMass, 0xAA), _mm_add_ps(_mm_shuffle_ps(kMass, kMass, 0x55), kMass));
	float fDiag = _mm_cvtss_f32(kSum) + 1.1920928955078125e-07;
	float fVirtualMass = 1.0f / fDiag;
	const float fStored = fVirtualMass;
	jac.m_angular[0].m_quad[3] = fStored;
	statusOut.m_virtualMass = fStored;
	return &statusOut;
}

// The SDK's: the limit's axis rotated into each body's frame (body B's negated), the virtual mass tau / (the effective
// inverse mass + epsilon), the angle's right-hand side and the bounds scaled by -1/dt (the bounds by tau too). The
// exe sums the lanes in SSE and does the rest on the x87.
void hk1dAngularLimitBuildJacobian(const hkp1dAngularLimitInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out) {
	const __m128 kDir = info.m_constrainedDofW.m_quad;
	const float fTau = info.m_tau.m_storage;
	const __m128 kNegDir = _mm_xor_ps(kDir, hkQuadRealNegateMask);
	const hkpVelocityAccumulator* pBodyB = in.m_bodyB;
	const hkpVelocityAccumulator* pBodyA = in.m_bodyA;
	const hkRotation& rotA = pBodyA->m_scratch123;
	const hkRotation& rotB = pBodyB->m_scratch123;
	hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
	pSchema[0].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55), rotA.m_col1.m_quad),
									   _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00), rotA.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA), rotA.m_col2.m_quad));
	const __m128 kAng1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55), rotB.m_col1.m_quad),
										_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00), rotB.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA), rotB.m_col2.m_quad));
	pSchema[1].m_quad = kAng1;
	const __m128 kAng0 = pSchema[0].m_quad;
	const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAng0, kAng0), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kAng1, kAng1), pBodyB->m_invMasses.m_quad));
	// z + (y + x): the exe's addss, written as _ps operations on lane 0.
	const __m128 kSum =
		_mm_add_ps(_mm_shuffle_ps(kMass, kMass, 0xAA), _mm_add_ps(_mm_shuffle_ps(kMass, kMass, 0x55), kMass));
	// The exe rounds the sum plus epsilon through a float stack slot (fstp/fld) before dividing tau by it.
	float fDiag = _mm_cvtss_f32(kSum) + 1.1920928955078125e-07;
	pSchema[0].m_quad[3] = fTau / fDiag;
	const float fNegInvDt = -in.m_substepInvDeltaTime;
	const float fMin = info.m_min.m_storage;
	const float fMax = info.m_max.m_storage;
	pSchema[1].m_quad[3] = info.m_computedAngle.m_storage * fNegInvDt;
	hkpJacobianSchema* pLimit = out.m_jacobianSchemas;
	out.m_jacobianSchemas = pLimit + 3;
	pLimit[2].m_quad[0] = fMin * fNegInvDt * fTau;
	pLimit[2].m_quad[1] = fNegInvDt * fMax * fTau;
	pLimit[2].m_quad[2] = fTau;
	pLimit[0].m_quad[0] = pLimit[0].m_quad[0] + 1.1920928955078125e-07;
	pLimit[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS;
}

// The SDK's: the target's right-hand side into arJac, then the angular motor schema from the motor's output.
hkpJacobianSchema* hk1dAngularVelocityMotorCommitJacobian(hkp1dConstraintMotorInfo& info,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkpConstraintQueryOut& out) {
	jac.m_angular[1].m_quad[3] = in.m_substepInvDeltaTime * info.m_targetPosition;
	hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
	pSchema[0].m_quad[0] = pSchema[0].m_quad[0] + 1.1920928955078125e-07;
	pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR;
	out.m_jacobianSchemas = pSchema + 4;
	pSchema[2].m_quad[0] = info.m_maxForce * in.m_microStepDeltaTime;
	pSchema[2].m_quad[1] = info.m_minForce * in.m_microStepDeltaTime;
	// The exe copies these three with fld/fstp (0xD6CEB2-0xD6CEC1), which quiets a signalling NaN.
	pSchema[2].m_quad[2] = info.m_targetVelocity;
	pSchema[2].m_quad[3] = info.m_tau;
	pSchema[3].m_quad[0] = info.m_damping;
	return pSchema + 4;
}

// The SDK's: the header schema, as buildJacobianHeader writes it, with the arguments on the stack.
hkpJacobianSchema* hkBeginConstraints(const hkpConstraintQueryIn* in, hkpConstraintQueryOut* out, hkpSolverResults* sr,
	uint8_t solverResultStriding) {
	hkpJacobianSchema* pSchema = out->m_jacobianSchemas;
	pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
	pSchema->getHeader().m_bodyBIndex = static_cast<uint16_t>(in->m_accumulatorBIndex.m_storage);
	pSchema->getHeader().m_constraintInstance = in->m_constraintInstance;
	pSchema->getHeader().m_bodyAIndex = static_cast<uint16_t>(in->m_accumulatorAIndex.m_storage);
	pSchema->getHeader().m_solverResultInMainMemory = sr;
	pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
	pSchema->getHeader().m_solverResultStriding = solverResultStriding;
	out->m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	return pSchema + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
}

void buildViscousSurfaceJacobian(const hkpViscousSurfaceModifierConstraintAtom& atom, hkpConstraintQueryOut& out,
	const hkpConstraintQueryIn& r_eax) {
	// The exe aligns the stack to 16 for this copy (0xD6C5A3).
	alignas(16) hkpConstraintQueryIn avgNormal;
	--out.m_jacobianSchemas;
	avgNormal = r_eax;
	int32_t m_childSize = atom.m_childSize;
	hkpConstraintAtom* pInner = atom.m_child;
	while (pInner->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
		pInner = static_cast<hkpModifierConstraintAtom*>(pInner)->m_child;
	}
	clearContactInfo(&static_cast<hkpSimpleContactConstraintAtom*>(pInner)->m_info);
	hkpSimpleContactConstraintAtom* v5 = static_cast<hkpSimpleContactConstraintAtom*>(atom.m_child);
	if (v5->m_type.m_storage == 22) {
		hkSimpleContactConstraintDataBuildJacobian(v5, avgNormal, static_cast<hkBool>(1), out);
	} else {
		hkSolverBuildJacobianFromAtomsNotContact(v5, m_childSize, avgNormal, out);
	}
}

// _hkBallSocketConstraintBuildJacobian's code (0xD6CED0): the pivots' error advanced by each body's angular velocity
// (taken into its inertia space, out of line 0x4B4AB0) times the stabilization factor over the rhs factor, the
// constraint recorded as violated when that error's square exceeds the limit, then one bilateral row per world axis.
// The step factors are x87 float copies (0xD6C7B1, 0xD6C7BE); the row's rhs is stored twice as the exe does
// (0xD6C915, 0xD6C91F).
void buildBallSocketJacobian(const hkVector4& posA, const hkVector4& posB, hkpConstraintQueryOut& out, float maxImpulse,
	float stabilizationFactor, const hkpConstraintQueryIn& in) {
	const float fFactor = 1.0 / in.m_rhsFactor * stabilizationFactor;
	hkpVelocityAccumulator* pBodyA = in.m_bodyA;
	hkpJacobianSchema* pSchemas = out.m_jacobianSchemas;
	hkpVelocityAccumulator* pBodyB = in.m_bodyB;
	const __m128 kFactor = _mm_set_ss(fFactor);
	const __m128 kPosA = posA.m_quad;
	const __m128 kArmA = _mm_sub_ps(kPosA, pBodyA->m_scratch0.m_quad);
	const __m128 kPosB = posB.m_quad;
	const __m128 kArmB = _mm_sub_ps(kPosB, pBodyB->m_scratch0.m_quad);
	hkVector4 angVelA;
	angVelA.setRotatedInverseDir(pBodyA->m_scratch123, pBodyA->m_angularVel);
	hkVector4 angVelB;
	angVelB.setRotatedInverseDir(pBodyB->m_scratch123, pBodyB->m_angularVel);
	hkpViolatedConstraintArray* pViolated = in.m_violatedConstraints;
	const __m128 kFactor4 = _mm_shuffle_ps(kFactor, kFactor, 0);
	// The error: (posB - posA) + cross(angVelB, armB) * f - cross(angVelA, armA) * f.
	const __m128 kVelB =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angVelB.m_quad, angVelB.m_quad, 210), _mm_shuffle_ps(kArmB, kArmB, 201)),
			_mm_mul_ps(_mm_shuffle_ps(angVelB.m_quad, angVelB.m_quad, 201), _mm_shuffle_ps(kArmB, kArmB, 210)));
	const __m128 kVelA =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angVelA.m_quad, angVelA.m_quad, 210), _mm_shuffle_ps(kArmA, kArmA, 201)),
			_mm_mul_ps(_mm_shuffle_ps(angVelA.m_quad, angVelA.m_quad, 201), _mm_shuffle_ps(kArmA, kArmA, 210)));
	hkVector4 error;
	error.m_quad =
		_mm_sub_ps(_mm_add_ps(_mm_mul_ps(kVelB, kFactor4), _mm_sub_ps(kPosB, kPosA)), _mm_mul_ps(kVelA, kFactor4));
	if (pViolated) {
		const __m128 kSquares = _mm_mul_ps(error.m_quad, error.m_quad);
		const __m128 kLenSq = _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 170),
			_mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 85), kSquares));
		const __m128 kSlack = _mm_sub_ss(in.hkpConstraintQueryIn::m_maxConstraintViolationSqrd.m_real, kLenSq);
		if ((_mm_movemask_ps(kSlack) & 1) != 0) {
			// EXE-INLINED none: hkCriticalSection::atomicExchangeAdd
			// EXE-CAST: hkUint32 as LONG (the SDK's atomicExchangeAdd casts the counter for InterlockedExchangeAdd)
			const uint32_t uiIndex =
				InterlockedExchangeAdd(reinterpret_cast<volatile int32_t*>(&pViolated->m_nextFreeElement), 1);
			if (uiIndex < 128) {
				in.m_violatedConstraints->m_constraints[uiIndex] = in.m_constraintInstance;
			}
		}
	}

	// The three axes are the columns of hkTransformIdentity's rotation.
	const hkVector4* pAxis = &hkTransformIdentity.m_rotation.m_col0;
	for (int32_t i = 0; i < 3; ++i, ++pAxis, pSchemas += 3) {
		const float fVirtMassFactor = in.m_virtMassFactor;
		const float fRhsFactor = in.m_rhsFactor;
		pSchemas[0].m_quad = pAxis->m_quad;
		const __m128 kRowArmA = _mm_sub_ps(posA.m_quad, pBodyA->m_scratch0.m_quad);
		const __m128 kRowArmB = _mm_sub_ps(posB.m_quad, pBodyB->m_scratch0.m_quad);
		const __m128 kDir = pAxis->m_quad;
		const __m128 kDirYzx = _mm_shuffle_ps(kDir, kDir, 201);
		const __m128 kDirZxy = _mm_shuffle_ps(kDir, kDir, 210);
		// cross(armA, dir) and cross(dir, armB).
		const __m128 kCrossA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRowArmA, kRowArmA, 201), kDirZxy),
			_mm_mul_ps(_mm_shuffle_ps(kRowArmA, kRowArmA, 210), kDirYzx));
		const __m128 kCrossB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRowArmB, kRowArmB, 210), kDirYzx),
			_mm_mul_ps(_mm_shuffle_ps(kRowArmB, kRowArmB, 201), kDirZxy));
		pSchemas[1].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 85), pBodyA->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0), pBodyA->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 170), pBodyA->m_scratch123.m_col2.m_quad));
		const __m128 kAngB =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0), pBodyB->m_scratch123.m_col0.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 85), pBodyB->m_scratch123.m_col1.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 170), pBodyB->m_scratch123.m_col2.m_quad));
		pSchemas[2].m_quad = kAngB;
		const __m128 kAngA = pSchemas[1].m_quad;
		const __m128 kInvMassesB = pBodyB->m_invMasses.m_quad;
		const __m128 kInvMassesA = pBodyA->m_invMasses.m_quad;
		const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAngB, kAngB), kInvMassesB),
			_mm_mul_ps(_mm_mul_ps(kAngA, kAngA), kInvMassesA));
		// The three angular lanes and the sum of the inverse masses' w lanes, added across.
		const __m128 kTerms = _mm_shuffle_ps(kMass, _mm_unpackhi_ps(kMass, _mm_add_ps(kInvMassesA, kInvMassesB)), 196);
		const __m128 kPairs = _mm_add_ps(_mm_shuffle_ps(kTerms, kTerms, 78), kTerms);
		const float fSum = _mm_add_ps(_mm_shuffle_ps(kPairs, kPairs, 177), kPairs)[0];
		const float fDiag = fSum + 1.1920928955078125e-07;
		pSchemas[2].m_quad[3] = fDiag;
		pSchemas[1].m_quad[3] = fVirtMassFactor / fDiag;
		const __m128 kSeparation = _mm_mul_ps(_mm_sub_ps(posB.m_quad, posA.m_quad), pAxis->m_quad);
		const float fSeparation = _mm_add_ss(_mm_shuffle_ps(kSeparation, kSeparation, 170),
			_mm_add_ss(_mm_shuffle_ps(kSeparation, kSeparation, 85), kSeparation))[0];
		pSchemas[0].m_quad[3] = fSeparation * fRhsFactor;
		pSchemas[0].m_quad[3] = error(i) * in.m_rhsFactor;
		pSchemas[0].m_quad[0] = pSchemas[0].m_quad[0] + 1.1920928955078125e-07;
		if (maxImpulse == 3.402820018375656e+38) {
			pSchemas[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL;
		} else {
			pSchemas[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT;
			pSchemas[2].m_quad[3] = maxImpulse;
		}
	}
	out.m_jacobianSchemas = pSchemas;
}

// The exe's code, read from its asm and Hex-Rays (the SDK ships no source for it). The SSE steps are written as the
// exe's instructions (the horizontal sums are two addss, the lengths sqrtss); every value the x87 code stores as a
// float is a float local, and its constants keep their size (the qword HK_REAL_EPSILON 1.1920928955078125e-07, the
// dword 1.1920929e-07f). Each schema the function writes starts by adding HK_REAL_EPSILON to the float the
// schema's first lane holds and storing the type byte over it, as the exe does. The bool the exe passes
// hkSimpleContactConstraintDataBuildJacobian is an hkBool by value: the exe stores its byte into a stack dword before
// pushing the whole dword, and the callee reads the byte only.
// The exe's x87 register cache of its constants (1.0, 0.0, the float and the double epsilon) is not state.
// The reinterpret_casts below are on memory the exe treats as raw bytes: the atom stream (and the contact points and
// strided properties after a contact atom), the constraint runtime (solver results at byte offsets the atoms give)
// and the axis table's byte offsets into a frame. The exe's frame has no /GS cookie.
FNVD_NO_STACK_COOKIE void hkSolverBuildJacobianFromAtomsNotContact(const hkpConstraintAtom* atoms,
	int32_t sizeOfAllAtoms, const hkpConstraintQueryIn& tmpIn, hkpConstraintQueryOut& angularLimitInfoBuffer) {
	hkpConstraintQueryOut& out = angularLimitInfoBuffer;
	// The exe's inline rep movsd of the 0x80-byte input.
	hkpConstraintQueryIn kIn = tmpIn;
	// The atoms are a byte stream the exe walks by each atom's size.
	const uint8_t* pCursor = reinterpret_cast<const uint8_t*>(atoms);
	// The exe compares the cursor with in + atomsEnd as unsigned addresses (lea ecx, [ebx+eax]; cmp; jae/jb), so a
	// negative atomsEnd that wraps still enters the loop. A pointer compare lets clang fold it to atomsEnd <= 0.
	const uintptr_t uiEnd =
		reinterpret_cast<uintptr_t>(pCursor) + static_cast<uintptr_t>(static_cast<intptr_t>(sizeOfAllAtoms));

	// The two bodies' constraint frames in world space: rotation columns then translation, read by index.
	hkTransform kLocalToWorldA;
	hkTransform kLocalToWorldB;
	kLocalToWorldA.m_rotation.m_col0 = kIn.m_transformA->m_rotation.m_col0;
	kLocalToWorldA.m_rotation.m_col1 = kIn.m_transformA->m_rotation.m_col1;
	kLocalToWorldA.m_rotation.m_col2 = kIn.m_transformA->m_rotation.m_col2;
	kLocalToWorldA.m_translation = kIn.m_transformA->m_translation;
	kLocalToWorldB.m_rotation.m_col0 = kIn.m_transformB->m_rotation.m_col0;
	kLocalToWorldB.m_rotation.m_col1 = kIn.m_transformB->m_rotation.m_col1;
	kLocalToWorldB.m_rotation.m_col2 = kIn.m_transformB->m_rotation.m_col2;
	kLocalToWorldB.m_translation = kIn.m_transformB->m_translation;
	// Columns 0-2 and the translation (index 3) read by index, as the exe does.
	hkVector4* pColsA = &kLocalToWorldA.m_rotation.m_col0;
	hkVector4* pColsB = &kLocalToWorldB.m_rotation.m_col0;
	// The exe adds the byte offsets of its axis table to the frame's address: raw bytes.
	const uint8_t* pBytesA = reinterpret_cast<const uint8_t*>(&kLocalToWorldA);
	const uint8_t* pBytesB = reinterpret_cast<const uint8_t*>(&kLocalToWorldB);

	// The header schema.
	hkpJacobianSchema* pHeaderSchema = out.m_jacobianSchemas;
	hkpJacobianHeaderSchema* pHeader = &pHeaderSchema->getHeader();
	pHeader->m_solverResultInMainMemory = static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage);
	pHeader->m_bodyAIndex = static_cast<uint16_t>(kIn.m_accumulatorAIndex.m_storage);
	pHeader->m_bodyBIndex = static_cast<uint16_t>(kIn.m_accumulatorBIndex.m_storage);
	pHeaderSchema->m_quad[0] = pHeaderSchema->m_quad[0] + 1.1920928955078125e-07;
	pHeaderSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
	pHeader->m_constraintInstance = kIn.m_constraintInstance;
	pHeader->m_solverResultStriding = 8;
	out.m_jacobianSchemas = pHeaderSchema + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	if (reinterpret_cast<uintptr_t>(pCursor) >= uiEnd) {
		return;
	}

	// The exe's frame: the copies of the input the modifiers pass on, the accumulator copies (shared by the
	// modifier cases, as the exe's frame slots are), the motor input and output, and the angular limit's values.
	hkpConstraintQueryIn kModifierIn;
	hkpVelocityAccumulator kAccA;
	hkpVelocityAccumulator kAccB;
	hkpConstraintMotorInput kMotorInput;
	hkpConstraintMotorOutput kMotorOutput;
	hkVector4 kLimitAxis;
	float fLimitTau = 0.0f;
	float fLimitMin = 0.0f;
	float fLimitMax = 0.0f;
	float fLimitAngle = 0.0f;
	uint8_t uiShift = 1;
	const __m128 kNegate = _mm_load_ps(&reinterpret_cast<float*>(&hkQuadRealNegateMask)[0]);

	for (;;) {
		const hkpConstraintAtom* pAtom = reinterpret_cast<const hkpConstraintAtom*>(pCursor);
		uint32_t uiType = pAtom->m_type.m_storage;
	lDispatch:
		switch (uiType) {
		case hkpConstraintAtom::TYPE_INVALID: {
			// Padding up to the next 16-byte boundary.
			pCursor = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(pCursor) + 15) & ~uintptr_t(15));
			pAtom = reinterpret_cast<const hkpConstraintAtom*>(pCursor);
			uiType = pAtom->m_type.m_storage;
			if (uiType <= hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER) {
				goto lDispatch;
			}
			break;
		}
		case hkpConstraintAtom::TYPE_BRIDGE: {
			const hkpBridgeConstraintAtom* pBridge = static_cast<const hkpBridgeConstraintAtom*>(pAtom);
			out.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			pBridge->m_buildJacobianFunc(pBridge->m_constraintData, kIn, out);
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 800000;
			pCursor += sizeof(hkpBridgeConstraintAtom);
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 800000;
			break;
		}
		case hkpConstraintAtom::TYPE_SET_LOCAL_TRANSFORMS: {
			const hkpSetLocalTransformsConstraintAtom* pSet =
				static_cast<const hkpSetLocalTransformsConstraintAtom*>(pAtom);
			const hkTransform* pTransform = kIn.m_transformA;
			__m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0]);
			__m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0]);
			__m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0]);
			const hkVector4* pLocal = &pSet->m_transformA.m_rotation.m_col0;
			for (int32_t i = 3; i >= 0; --i) {
				__m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pLocal[i].m_quad)[0]);
				_mm_store_ps(&reinterpret_cast<float*>(&pColsA[i].m_quad)[0],
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0)),
						_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2)));
			}
			_mm_store_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_translation.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0])));
			pTransform = kIn.m_transformB;
			c0 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0]);
			c1 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0]);
			c2 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0]);
			pLocal = &pSet->m_transformB.m_rotation.m_col0;
			for (int32_t i = 3; i >= 0; --i) {
				__m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pLocal[i].m_quad)[0]);
				_mm_store_ps(&reinterpret_cast<float*>(&pColsB[i].m_quad)[0],
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0)),
						_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2)));
			}
			_mm_store_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_translation.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0])));
			pCursor += sizeof(hkpSetLocalTransformsConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_SET_LOCAL_TRANSLATIONS: {
			const hkpSetLocalTranslationsConstraintAtom* pSet =
				static_cast<const hkpSetLocalTranslationsConstraintAtom*>(pAtom);
			const hkTransform* pTransform = kIn.m_transformA;
			__m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pSet->m_translationA.m_quad)[0]);
			__m128 kXY =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55),
							   _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0])),
					_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00),
						_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0])));
			__m128 kZT =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA),
							   _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0])),
					_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_translation.m_quad)[0]));
			_mm_store_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0], _mm_add_ps(kXY, kZT));
			pTransform = kIn.m_transformB;
			v = _mm_load_ps(&reinterpret_cast<const float*>(&pSet->m_translationB.m_quad)[0]);
			kXY =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55),
							   _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0])),
					_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00),
						_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0])));
			kZT =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA),
							   _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0])),
					_mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_translation.m_quad)[0]));
			_mm_store_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0], _mm_add_ps(kXY, kZT));
			pCursor += sizeof(hkpSetLocalTranslationsConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_SET_LOCAL_ROTATIONS: {
			// The asm wins over the SDK: the exe's loops run four columns, so they also overwrite each frame's
			// translation with the rotation of the 16 bytes after the atom's rotation (for A, the first column of
			// m_rotationB; for B, the 16 bytes after the atom).
			const hkpSetLocalRotationsConstraintAtom* pSet =
				static_cast<const hkpSetLocalRotationsConstraintAtom*>(pAtom);
			const hkTransform* pTransform = kIn.m_transformA;
			__m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0]);
			__m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0]);
			__m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0]);
			const hkVector4* pLocal = &pSet->m_rotationA.m_col0;
			for (int32_t i = 3; i >= 0; --i) {
				__m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pLocal[i].m_quad)[0]);
				_mm_store_ps(&reinterpret_cast<float*>(&pColsA[i].m_quad)[0],
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
						_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2)));
			}
			pTransform = kIn.m_transformB;
			c0 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col0.m_quad)[0]);
			c1 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col1.m_quad)[0]);
			c2 = _mm_load_ps(&reinterpret_cast<const float*>(&pTransform->m_rotation.m_col2.m_quad)[0]);
			pCursor += sizeof(hkpSetLocalRotationsConstraintAtom);
			pLocal = &pSet->m_rotationB.m_col0;
			for (int32_t i = 3; i >= 0; --i) {
				__m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pLocal[i].m_quad)[0]);
				_mm_store_ps(&reinterpret_cast<float*>(&pColsB[i].m_quad)[0],
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1)),
						_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2)));
			}
			break;
		}
		case hkpConstraintAtom::TYPE_BALL_SOCKET: {
			_hkBallSocketConstraintBuildJacobian(static_cast<const hkpBallSocketConstraintAtom*>(pAtom), &kIn,
				kLocalToWorldA, kLocalToWorldB, out);
			pCursor += sizeof(hkpBallSocketConstraintAtom);
			goto lRuntimeThree;
		}
		case hkpConstraintAtom::TYPE_STIFF_SPRING: {
			const hkpStiffSpringConstraintAtom* pSpring = static_cast<const hkpStiffSpringConstraintAtom*>(pAtom);
			hkp1dLinearBilateralConstraintInfo kInfo;
			__m128 kPivotA = _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0]);
			__m128 kPivotB = _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0]);
			_mm_store_ps(&reinterpret_cast<float*>(&kInfo.m_pivotA.m_quad)[0], kPivotA);
			_mm_store_ps(&reinterpret_cast<float*>(&kInfo.m_pivotB.m_quad)[0], kPivotB);
			__m128 kDiff = _mm_sub_ps(kPivotA, kPivotB);
			__m128 kSq = _mm_mul_ps(kDiff, kDiff);
			float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
			float fCurrent;
			// fcom / test ah, 0x41 / jp: normalises when the length is above epsilon or NaN.
			if (!(fLength <= 1.1920929e-07f)) {
				_mm_store_ps(&reinterpret_cast<float*>(&kInfo.m_constrainedDofW.m_quad)[0], kDiff);
				float fInvLength = 1.0 / fLength;
				_mm_store_ps(&reinterpret_cast<float*>(&kInfo.m_constrainedDofW.m_quad)[0],
					_mm_mul_ps(_mm_set1_ps(fInvLength), kDiff));
				fCurrent = fLength;
			} else {
				kInfo.m_constrainedDofW = hkTransformIdentity.m_rotation.m_col0;
				fCurrent = 0.0f;
			}
			float fError = pSpring->m_length - fCurrent;
			hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs(kInfo, kIn, out, fError);
			pCursor += sizeof(hkpStiffSpringConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_LIN: {
			const hkpLinConstraintAtom* pLin = static_cast<const hkpLinConstraintAtom*>(pAtom);
			hkp1dLinearBilateralConstraintInfo kInfo;
			kInfo.m_pivotA = kLocalToWorldA.m_translation;
			kInfo.m_pivotB = kLocalToWorldB.m_translation;
			kInfo.m_constrainedDofW = pColsB[pLin->m_axisIndex];
			hk1dLinearBilateralConstraintBuildJacobian(kInfo, kIn, out);
			pCursor += sizeof(hkpLinConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_LIN_SOFT: {
			const hkpLinSoftConstraintAtom* pLin = static_cast<const hkpLinSoftConstraintAtom*>(pAtom);
			hkp1dLinearBilateralUserTauConstraintInfo kInfo;
			kInfo.m_pivotA = kLocalToWorldA.m_translation;
			kInfo.m_tau = pLin->m_tau;
			kInfo.m_damping = pLin->m_damping;
			kInfo.m_pivotB = kLocalToWorldB.m_translation;
			kInfo.m_constrainedDofW = pColsB[pLin->m_axisIndex];
			hk1dLinearBilateralConstraintUserTauBuildJacobian(kInfo, kIn, out);
			pCursor += sizeof(hkpLinSoftConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_LIN_LIMIT: {
			const hkpLinLimitConstraintAtom* pLin = static_cast<const hkpLinLimitConstraintAtom*>(pAtom);
			hkp1dLinearLimitInfo kInfo;
			kInfo.m_pivotA = kLocalToWorldA.m_translation;
			kInfo.m_min = pLin->m_min;
			kInfo.m_max = pLin->m_max;
			kInfo.m_pivotB = kLocalToWorldB.m_translation;
			kInfo.m_constrainedDofW = pColsB[pLin->m_axisIndex];
			hk1dLinearLimitBuildJacobian(kInfo, kIn, out);
			pCursor += sizeof(hkpLinLimitConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_LIN_FRICTION: {
			const hkpLinFrictionConstraintAtom* pFriction = static_cast<const hkpLinFrictionConstraintAtom*>(pAtom);
			pCursor += sizeof(hkpLinFrictionConstraintAtom);
			// fcomp / test ah, 5 / jp: no friction when 0 >= the maximum force or it is NaN.
			if (pFriction->m_isEnabled && 0.0f < pFriction->m_maxFrictionForce) {
				hkp1dLinearFrictionInfo kInfo;
				kInfo.m_maxFrictionForce = pFriction->m_maxFrictionForce;
				kInfo.m_pivot = kLocalToWorldA.m_translation;
				kInfo.m_constrainedDofW = pColsB[pFriction->m_frictionAxis];
				kInfo.m_lastSolverResults = static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage);
				hk1dLinearFrictionBuildJacobian(kInfo, kIn, out);
				goto lRuntimeOne;
			}
			uiShift = 1;
			goto lShiftRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_LIN_MOTOR: {
			const hkpLinMotorConstraintAtom* pMotorAtom = static_cast<const hkpLinMotorConstraintAtom*>(pAtom);
			uint8_t* pRuntime = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage);
			uint8_t* pInitialized = pRuntime + pMotorAtom->m_initializedOffset;
			float* pPrevious = reinterpret_cast<float*>(pRuntime + pMotorAtom->m_previousTargetPositionOffset);
			hkpConstraintMotor* pMotor;
			if (pMotorAtom->m_isEnabled.m_bool && (pMotor = pMotorAtom->m_motor) != nullptr) {
				if (*pInitialized == 0) {
					*pPrevious = pMotorAtom->m_targetPosition;
				}
				hkpJacobianSchema* pJac = out.m_jacobianSchemas;
				kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
				kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
				hk1dLinearVelocityMotorBeginJacobian(pColsB[pMotorAtom->m_motorAxis], kLocalToWorldA.m_translation, kIn,
					pJac, &kMotorInput);
				kMotorInput.m_stepInfo = &kIn;
				kMotorInput.m_lastResults = *static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage);
				kMotorInput.m_deltaTarget = pMotorAtom->m_targetPosition - *pPrevious;
				__m128 kDot =
					_mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation
												  .m_quad)[0]),
								   _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0])),
						_mm_load_ps(&reinterpret_cast<float*>(&pColsB[pMotorAtom->m_motorAxis].m_quad)[0]));
				float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kDot, kDot, 0x55), kDot)));
				kMotorInput.m_positionError = *pPrevious - fDistance;
				hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
				hk1dLinearVelocityMotorCommitJacobian(&kMotorOutput, kIn, pJac, out);
				// fucompp / test ah, 0x44 / jp: a target that differs (or is NaN) is stored.
				if (pMotorAtom->m_targetPosition != *pPrevious || *pInitialized == 0) {
					*pPrevious = pMotorAtom->m_targetPosition;
					*pInitialized = 1;
				}
			} else {
				hkpJacobianSchema* pShift = out.m_jacobianSchemas;
				pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
				pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
				pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
				out.m_jacobianSchemas = pShift + 1;
			}
			pCursor += sizeof(hkpLinMotorConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_2D_ANG: {
			const hkp2dAngConstraintAtom* pAng = static_cast<const hkp2dAngConstraintAtom*>(pAtom);
			uint32_t uiAxis = pAng->m_freeRotationAxis;
			const hkVector4* pOther1 = reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[uiAxis + 1]);
			const hkVector4* pOther2 = reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[uiAxis + 2]);
			hkp1dAngularBilateralConstraintInfo kInfo;
			kInfo.m_zeroErrorAxisAinW = pColsA[uiAxis];
			kInfo.m_constrainedDofW = *pOther1;
			kInfo.m_perpZeroErrorAxisBinW = *pOther2;
			hk1dAngularBilateralConstraintBuildJacobian(kInfo, kIn, out);
			kInfo.m_constrainedDofW = *pOther2;
			_mm_store_ps(&reinterpret_cast<float*>(&kInfo.m_perpZeroErrorAxisBinW.m_quad)[0],
				_mm_xor_ps(_mm_load_ps(&reinterpret_cast<const float*>(&pOther1->m_quad)[0]), kNegate));
			hk1dAngularBilateralConstraintBuildJacobian(kInfo, kIn, out);
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 0x10;
			pCursor += sizeof(hkp2dAngConstraintAtom);
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 0x10;
			break;
		}
		case hkpConstraintAtom::TYPE_ANG: {
			const hkpAngConstraintAtom* pAng = static_cast<const hkpAngConstraintAtom*>(pAtom);
			for (int32_t i = pAng->m_firstConstrainedAxis;
				i < pAng->m_firstConstrainedAxis + pAng->m_numConstrainedAxes; ++i) {
				hkp1dAngularBilateralConstraintInfo kInfo;
				kInfo.m_perpZeroErrorAxisBinW = *reinterpret_cast<const hkVector4*>(pBytesA + rdata_10D33C0[i]);
				kInfo.m_zeroErrorAxisAinW = *reinterpret_cast<const hkVector4*>(pBytesA + rdata_10D33C0[i + 1]);
				kInfo.m_constrainedDofW = *reinterpret_cast<const hkVector4*>(pBytesB + rdata_10D33C0[i + 2]);
				hk1dAngularBilateralConstraintBuildJacobian(kInfo, kIn, out);
			}
			uint32_t uiNum = pAng->m_numConstrainedAxes;
			pCursor += sizeof(hkpAngConstraintAtom);
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8 * uiNum;
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8 * uiNum;
			break;
		}
		case hkpConstraintAtom::TYPE_ANG_LIMIT: {
			const hkpAngLimitConstraintAtom* pLimit = static_cast<const hkpAngLimitConstraintAtom*>(pAtom);
			pCursor += sizeof(hkpAngLimitConstraintAtom);
			if (!pLimit->m_isEnabled) {
				kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
				kIn.m_constraintRuntimeInMainMemory.m_storage =
					static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;
				uiShift = 1;
				goto lShift;
			}
			uint32_t uiAxis = pLimit->m_limitAxis;
			__m128 kA1 = _mm_load_ps(reinterpret_cast<const float*>(pBytesA + rdata_10D33C0[uiAxis + 1]));
			fLimitTau = pLimit->m_angularLimitsTauFactor * kIn.m_tau;
			fLimitMin = pLimit->m_minAngle;
			fLimitMax = pLimit->m_maxAngle;
			__m128 kB2 = _mm_load_ps(reinterpret_cast<const float*>(pBytesB + rdata_10D33C0[uiAxis + 2]));
			__m128 kCosV = _mm_mul_ps(kB2, kA1);
			float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
			kLimitAxis = pColsA[uiAxis];
			__m128 kB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[uiAxis].m_quad)[0]);
			__m128 kSinV =
				_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kB2, kB2, 0xC9)),
							   _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kB2, kB2, 0xD2))),
					kA1);
			float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
			fLimitAngle = hkMath_atan2fApproximation(fCos, fSin);
			float fReference =
				static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage)->m_internalSolverData * -1.0;
			// The angle is moved by 2 pi into (reference - pi, reference + pi]; each step is stored as a float.
			if (fReference - fLimitAngle > 3.1415927410125732) {
				do {
					fLimitAngle = fLimitAngle + 6.2831854820251465;
				} while (fReference - fLimitAngle > 3.1415927410125732);
			}
			if (fLimitAngle - fReference > 3.1415927410125732) {
				do {
					fLimitAngle = fLimitAngle - 6.2831854820251465;
				} while (fLimitAngle - fReference > 3.1415927410125732);
			}
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;
			goto lAngularLimit;
		}
		case hkpConstraintAtom::TYPE_TWIST_LIMIT: {
			const hkpTwistLimitConstraintAtom* pTwist = static_cast<const hkpTwistLimitConstraintAtom*>(pAtom);
			uint32_t uiRef = pTwist->m_refAxis;
			fLimitMin = pTwist->m_minAngle;
			fLimitMax = pTwist->m_maxAngle;
			uint32_t uiTwist = pTwist->m_twistAxis;
			__m128 kAxis = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[uiTwist].m_quad)[0]);
			__m128 kSum = _mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&pColsA[uiTwist].m_quad)[0]), kAxis);
			__m128 kSq = _mm_mul_ps(kSum, kSum);
			float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
			float fHalfLength;
			// fcom / test ah, 0x41 / jne: normalises only when the length is above 1e-16.
			if (fLength > 1.000000016862384e-16) {
				fHalfLength = fLength * 0.5;
				float fInvLength = 1.0 / fLength;
				kAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
			} else {
				fHalfLength = 0.0f;
			}
			__m128 kRefB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[uiRef].m_quad)[0]);
			__m128 kAxisD2 = _mm_shuffle_ps(kAxis, kAxis, 0xD2);
			__m128 kAxisC9 = _mm_shuffle_ps(kAxis, kAxis, 0xC9);
			__m128 kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
				_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
			__m128 kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
				_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
			__m128 kRefA = _mm_load_ps(&reinterpret_cast<float*>(&pColsA[uiRef].m_quad)[0]);
			__m128 kCosV = _mm_mul_ps(kRefA, kPerp);
			float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
			__m128 kSinV = _mm_mul_ps(kRefA, kPerp2);
			float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
			fLimitAngle = hkMath_atan2fApproximation(fCos, fSin);
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
			pCursor += sizeof(hkpTwistLimitConstraintAtom);
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;
			_mm_store_ps(&reinterpret_cast<float*>(&kLimitAxis.m_quad)[0], kAxis);
			fLimitTau = pTwist->m_angularLimitsTauFactor * kIn.m_tau * fHalfLength * fHalfLength;
			goto lAngularLimit;
		}
		case hkpConstraintAtom::TYPE_CONE_LIMIT: {
			const hkpConeLimitConstraintAtom* pCone = static_cast<const hkpConeLimitConstraintAtom*>(pAtom);
			__m128 kTwistA = _mm_load_ps(&reinterpret_cast<float*>(&pColsA[pCone->m_twistAxisInA].m_quad)[0]);
			__m128 kRefB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[pCone->m_refAxisInB].m_quad)[0]);
			__m128 kCross =
				_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kTwistA, kTwistA, 0xC9), _mm_shuffle_ps(kRefB, kRefB, 0xD2)),
					_mm_mul_ps(_mm_shuffle_ps(kTwistA, kTwistA, 0xD2), _mm_shuffle_ps(kRefB, kRefB, 0xC9)));
			__m128 kSq = _mm_mul_ps(kCross, kCross);
			float fSinSq = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)));
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;
			pCursor += sizeof(hkpConeLimitConstraintAtom);
			// fcom / test ah, 5 / jp: no limit when the sine is below epsilon (NaN builds the limit).
			if (fSinSq < 1.1920929e-07f) {
				uiShift = 1;
				goto lShift;
			}
			uint32_t uiMode = pCone->m_angleMeasurementMode.m_storage;
			__m128 kCosV = _mm_mul_ps(kTwistA, kRefB);
			float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
			// The exe calls __CIsqrt (0xEC6040) with the float in ST(0).
			float fSin = std::sqrt(fSinSq);
			float fAngle =
				hkMath_atan2fApproximation(fSin, fCos) * buildJacobianFromConeLimitAtom_flipSignOrNot[uiMode] +
				rdata_10D33D8[uiMode];
			// The exe reads the sign at 0x10D33E4 - 4 * mode: the other entry of the {1, -1} table.
			float fSign = buildJacobianFromConeLimitAtom_flipSignOrNot[1 - uiMode];
			fLimitTau = pCone->m_angularLimitsTauFactor * kIn.m_tau;
			float fInvSin = 1.0 / fSin;
			fLimitAngle = fAngle;
			fLimitMin = pCone->m_minAngle;
			fLimitMax = pCone->m_maxAngle;
			_mm_store_ps(&reinterpret_cast<float*>(&kLimitAxis.m_quad)[0],
				_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(fInvSin), kCross), _mm_set1_ps(fSign)));
			uint8_t uiOffset = pCone->m_memOffsetToAngleOffset;
			if (uiOffset > 1) {
				uint8_t* pRuntime = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) - 8;
				float* pAngleOffset = reinterpret_cast<float*>(pRuntime + uiOffset);
				float fOffset = *pAngleOffset;
				float fReference = -reinterpret_cast<hkpSolverResults*>(pRuntime)->m_internalSolverData;
				float fError = fAngle - fOffset;
				float fRemaining = fReference - fError;
				float fHalfStep = kIn.m_frameDeltaTime * 0.5;
				float fScaledOffset = fOffset * 0.05000000074505806;
				float fStep = fHalfStep;
				// fcom / test ah, 5 / jp: the larger of the two; NaN keeps the half step.
				if (fHalfStep < fScaledOffset) {
					fStep = fScaledOffset;
				}
				float fNewOffset = fOffset - fStep;
				float fNewAngle = fError + fStep;
				float fMargin = 0.5 * (fHalfStep + (fStep + fStep));
				// fucompp / test ah, 0x44 / jnp: skipped only when the reference is exactly 0.
				if (fMargin < -fRemaining && 0.0f != fReference) {
					fNewOffset = fNewOffset - fRemaining;
					fNewAngle = fRemaining + fNewAngle;
				}
				// fcom / test ah, 5 / jnp: clamped up to epsilon unless above it (NaN is clamped).
				if (!(1.1920929e-07f < fNewOffset)) {
					fNewOffset = 1.1920929e-07f;
				}
				*pAngleOffset = fNewOffset;
				fLimitAngle = fNewAngle;
			}
			goto lAngularLimit;
		}
		case hkpConstraintAtom::TYPE_ANG_FRICTION: {
			const hkpAngFrictionConstraintAtom* pFriction = static_cast<const hkpAngFrictionConstraintAtom*>(pAtom);
			// fcomp / test ah, 0x44 / jnp: no friction when the maximum torque is exactly 0.
			if (pFriction->m_isEnabled && 0.0f != pFriction->m_maxFrictionTorque) {
				hkp1dAngularFrictionInfo kInfo;
				kInfo.m_maxFrictionTorque = pFriction->m_maxFrictionTorque;
				kInfo.m_constrainedDofW = &pColsA[pFriction->m_firstFrictionAxis];
				kInfo.m_numFriction.m_storage = pFriction->m_numFrictionAxes;
				kInfo.m_lastSolverResults = static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage);
				hk1dAngularFrictionBuildJacobian(kInfo, kIn, out);
			} else {
				hkpJacobianSchema* pShift = out.m_jacobianSchemas;
				pShift->m_shiftSolverResults.m_numSolverResultsToSkip = pFriction->m_numFrictionAxes;
				pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
				pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
				out.m_jacobianSchemas = pShift + 1;
			}
			uint32_t uiNum = pFriction->m_numFrictionAxes;
			pCursor += sizeof(hkpAngFrictionConstraintAtom);
			kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8 * uiNum;
			kIn.m_constraintRuntimeInMainMemory.m_storage =
				static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8 * uiNum;
			break;
		}
		case hkpConstraintAtom::TYPE_ANG_MOTOR: {
			const hkpAngMotorConstraintAtom* pMotorAtom = static_cast<const hkpAngMotorConstraintAtom*>(pAtom);
			uint8_t* pRuntime = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage);
			float* pPrevious = reinterpret_cast<float*>(pRuntime + pMotorAtom->m_previousTargetAngleOffset);
			uint8_t* pInitialized = pRuntime + pMotorAtom->m_initializedOffset;
			hkpConstraintMotor* pMotor = pMotorAtom->m_motor;
			if (pMotor != nullptr && pMotorAtom->m_isEnabled.m_bool) {
				uint32_t uiAxis = pMotorAtom->m_motorAxis;
				__m128 kB2 = _mm_load_ps(reinterpret_cast<const float*>(pBytesB + rdata_10D33C0[uiAxis + 2]));
				__m128 kA1 = _mm_load_ps(reinterpret_cast<const float*>(pBytesA + rdata_10D33C0[uiAxis + 1]));
				__m128 kCosV = _mm_mul_ps(kA1, kB2);
				float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
				__m128 kB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[uiAxis].m_quad)[0]);
				__m128 kSinV =
					_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kB2, kB2, 0xC9)),
								   _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kB2, kB2, 0xD2))),
						kA1);
				float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
				float fAngle = hkMath_atan2fApproximation(fCos, fSin);
				int16_t iLimitOffset = pMotorAtom->m_correspondingAngLimitSolverResultOffset;
				if (iLimitOffset != 0) {
					float fReference =
						reinterpret_cast<hkpSolverResults*>(pRuntime + iLimitOffset)->m_internalSolverData * -1.0;
					if (fReference - fAngle > 3.1415927410125732) {
						do {
							fAngle = fAngle + 6.2831854820251465;
						} while (fReference - fAngle > 3.1415927410125732);
					}
					if (fAngle - fReference > 3.1415927410125732) {
						do {
							fAngle = fAngle - 6.2831854820251465;
						} while (fAngle - fReference > 3.1415927410125732);
					}
				}
				if (*pInitialized == 0) {
					*pPrevious = fAngle;
				}
				kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
				kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
				hkpJacobianSchema* pJac = out.m_jacobianSchemas;
				__m128 kDir = _mm_load_ps(&reinterpret_cast<float*>(&pColsA[uiAxis].m_quad)[0]);
				__m128 kNegDir = _mm_xor_ps(kDir, kNegate);
				const hkRotation& kRotA = kIn.m_bodyA->m_scratch123;
				const hkRotation& kRotB = kIn.m_bodyB->m_scratch123;
				__m128 kJacA =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55),
											  _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col1.m_quad)[0])),
								   _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00),
									   _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col0.m_quad)[0]))),
						_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA),
							_mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col2.m_quad)[0])));
				_mm_store_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0], kJacA);
				__m128 kJacB =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55),
											  _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col1.m_quad)[0])),
								   _mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00),
									   _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col0.m_quad)[0]))),
						_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA),
							_mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col2.m_quad)[0])));
				_mm_store_ps(&reinterpret_cast<float*>(&pJac[1].m_quad)[0], kJacB);
				kJacA = _mm_load_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0]);
				__m128 kMass =
					_mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacA, kJacA),
								   _mm_load_ps(&reinterpret_cast<float*>(&kIn.m_bodyA->m_invMasses.m_quad)[0])),
						_mm_mul_ps(_mm_mul_ps(kJacB, kJacB),
							_mm_load_ps(&reinterpret_cast<float*>(&kIn.m_bodyB->m_invMasses.m_quad)[0])));
				float fInvMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
				float fInvMassEps = fInvMass + 1.1920928955078125e-07;
				float fVirtualMass = 1.0 / fInvMassEps;
				pJac[0].m_quad[3] = fVirtualMass;
				kMotorInput.m_virtualMass = fVirtualMass;
				kMotorInput.m_stepInfo = &kIn;
				kMotorInput.m_lastResults = *static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage);
				kMotorInput.m_deltaTarget = pMotorAtom->m_targetAngle - *pPrevious;
				kMotorInput.m_positionError = *pPrevious - fAngle;
				hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
				pJac[1].m_quad[3] = kMotorOutput.m_targetPosition * kIn.m_substepInvDeltaTime;
				hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
				pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
				pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR;
				out.m_jacobianSchemas = pSchema + 4;
				pSchema[2].m_quad[0] = kIn.m_microStepDeltaTime * kMotorOutput.m_maxForce;
				pSchema[2].m_quad[1] = kMotorOutput.m_minForce * kIn.m_microStepDeltaTime;
				pSchema[2].m_quad[2] = kMotorOutput.m_targetVelocity;
				pSchema[2].m_quad[3] = kMotorOutput.m_tau;
				pSchema[3].m_quad[0] = kMotorOutput.m_damping;
				// fucompp / test ah, 0x44 / jp: a target that differs (or is NaN) is stored.
				if (pMotorAtom->m_targetAngle != *pPrevious || *pInitialized == 0) {
					*pPrevious = pMotorAtom->m_targetAngle;
					*pInitialized = 1;
				}
			} else {
				hkpJacobianSchema* pShift = out.m_jacobianSchemas;
				pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
				pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
				pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
				out.m_jacobianSchemas = pShift + 1;
			}
			pCursor += sizeof(hkpAngMotorConstraintAtom);
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_RAGDOLL_MOTOR: {
			const hkpRagdollMotorConstraintAtom* pMotorAtom = static_cast<const hkpRagdollMotorConstraintAtom*>(pAtom);
			uint8_t* pRuntime = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage);
			uint8_t* pInitialized = pRuntime + pMotorAtom->m_initializedOffset;
			float* pPrevious = reinterpret_cast<float*>(pRuntime + pMotorAtom->m_previousTargetAnglesOffset);
			if (!pMotorAtom->m_isEnabled.m_bool) {
				hkpJacobianSchema* pShift = out.m_jacobianSchemas;
				pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 3;
				pCursor += sizeof(hkpRagdollMotorConstraintAtom);
				pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
				pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
				out.m_jacobianSchemas = pShift + 1;
				goto lRuntimeThree;
			}
			hkMatrix3 kTarget;
			hkMatrix3_setMulMat3Mat3(&kTarget, &kIn.m_transformB->m_rotation, &pMotorAtom->m_target_bRca);
			kMotorInput.m_lastResults.m_internalSolverData = 0.0f;
			kMotorInput.m_lastResults.m_impulseApplied = 0.0f;
			kMotorInput.m_stepInfo = &kIn;
			// The exe's two index triples on its stack: the axis each motor turns about, and its reference axis.
			int32_t aiAxis[3] = {0, 1, 2};
			int32_t aiRef[3] = {1, 0, 0};
			hkVector4* pTargetCols = &kTarget.m_col0;
			for (int32_t i = 0; i < 3; ++i) {
				hkpConstraintMotor* pMotor = pMotorAtom->m_motors[i];
				if (pMotor == nullptr) {
					hkpJacobianSchema* pShift = out.m_jacobianSchemas;
					pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
					pShift->m_shiftSolverResults.m_numSolverResultsToSkip = 1;
					pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
					out.m_jacobianSchemas = pShift + 1;
					continue;
				}
				int32_t iRef = aiRef[i];
				int32_t iAxis = aiAxis[i];
				// The current angle: about the bisector of the two bodies' axes, from the reference axes.
				__m128 kAxis = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[iAxis].m_quad)[0]);
				__m128 kSum = _mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&pColsA[iAxis].m_quad)[0]), kAxis);
				__m128 kSq = _mm_mul_ps(kSum, kSum);
				float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
				if (fLength > 1.000000016862384e-16) {
					float fInvLength = 1.0 / fLength;
					kAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
				}
				__m128 kRefB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[iRef].m_quad)[0]);
				__m128 kAxisD2 = _mm_shuffle_ps(kAxis, kAxis, 0xD2);
				__m128 kAxisC9 = _mm_shuffle_ps(kAxis, kAxis, 0xC9);
				__m128 kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
					_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
				__m128 kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
					_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
				__m128 kRefA = _mm_load_ps(&reinterpret_cast<float*>(&pColsA[iRef].m_quad)[0]);
				__m128 kCosV = _mm_mul_ps(kRefA, kPerp);
				float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
				__m128 kSinV = _mm_mul_ps(kRefA, kPerp2);
				float fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
				float fCurrent = hkMath_atan2fApproximation(fCos, fSin);
				// The target angle: the same with the target frame in place of the first body's.
				__m128 kTargetAxis = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[iAxis].m_quad)[0]);
				kSum = _mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&pTargetCols[iAxis].m_quad)[0]), kTargetAxis);
				kSq = _mm_mul_ps(kSum, kSum);
				fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
				if (fLength > 1.000000016862384e-16) {
					float fInvLength = 1.0 / fLength;
					kTargetAxis = _mm_mul_ps(_mm_set1_ps(fInvLength), kSum);
				}
				kAxisD2 = _mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xD2);
				kAxisC9 = _mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xC9);
				kRefB = _mm_load_ps(&reinterpret_cast<float*>(&pColsB[iRef].m_quad)[0]);
				kPerp = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xD2), kAxisC9),
					_mm_mul_ps(_mm_shuffle_ps(kRefB, kRefB, 0xC9), kAxisD2));
				kPerp2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xC9), kAxisD2),
					_mm_mul_ps(_mm_shuffle_ps(kPerp, kPerp, 0xD2), kAxisC9));
				__m128 kRefT = _mm_load_ps(&reinterpret_cast<float*>(&pTargetCols[iRef].m_quad)[0]);
				kCosV = _mm_mul_ps(kRefT, kPerp);
				fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kCosV, kCosV, 0x55), kCosV)));
				kSinV = _mm_mul_ps(kRefT, kPerp2);
				fSin = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kSinV, kSinV, 0x55), kSinV)));
				float fTarget = hkMath_atan2fApproximation(fCos, fSin);
				if (pInitialized[i] == 0) {
					pPrevious[i] = fCurrent;
					pInitialized[i] = 1;
				}
				hkpJacobianSchema* pJac = out.m_jacobianSchemas;
				__m128 kNegDir = _mm_xor_ps(kNegate, kTargetAxis);
				const hkRotation& kRotA = kIn.m_bodyA->m_scratch123;
				const hkRotation& kRotB = kIn.m_bodyB->m_scratch123;
				__m128 kJacA =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0x55),
											  _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col1.m_quad)[0])),
								   _mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0x00),
									   _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col0.m_quad)[0]))),
						_mm_mul_ps(_mm_shuffle_ps(kTargetAxis, kTargetAxis, 0xAA),
							_mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col2.m_quad)[0])));
				_mm_store_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0], kJacA);
				__m128 kJacB =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55),
											  _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col1.m_quad)[0])),
								   _mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00),
									   _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col0.m_quad)[0]))),
						_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA),
							_mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col2.m_quad)[0])));
				_mm_store_ps(&reinterpret_cast<float*>(&pJac[1].m_quad)[0], kJacB);
				kJacA = _mm_load_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0]);
				__m128 kMass =
					_mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacB, kJacB),
								   _mm_load_ps(&reinterpret_cast<float*>(&kIn.m_bodyB->m_invMasses.m_quad)[0])),
						_mm_mul_ps(_mm_mul_ps(kJacA, kJacA),
							_mm_load_ps(&reinterpret_cast<float*>(&kIn.m_bodyA->m_invMasses.m_quad)[0])));
				float fInvMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
				float fInvMassEps = fInvMass + 1.1920928955078125e-07;
				float fVirtualMass = 1.0 / fInvMassEps;
				pJac[0].m_quad[3] = fVirtualMass;
				kMotorInput.m_virtualMass = fVirtualMass;
				kMotorInput.m_lastResults = static_cast<hkpSolverResults*>(kIn.m_constraintRuntime.m_storage)[i];
				kMotorInput.m_deltaTarget = fTarget - pPrevious[i];
				kMotorInput.m_positionError = pPrevious[i] - fCurrent;
				hkCalcMotorData(pMotor, &kMotorInput, &kMotorOutput);
				pJac[1].m_quad[3] = kMotorOutput.m_targetPosition * kIn.m_substepInvDeltaTime;
				hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
				pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
				pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR;
				out.m_jacobianSchemas = pSchema + 4;
				pSchema[2].m_quad[0] = kMotorOutput.m_maxForce * kIn.m_microStepDeltaTime;
				pSchema[2].m_quad[1] = kMotorOutput.m_minForce * kIn.m_microStepDeltaTime;
				pSchema[2].m_quad[2] = kMotorOutput.m_targetVelocity;
				pSchema[2].m_quad[3] = kMotorOutput.m_tau;
				pSchema[3].m_quad[0] = kMotorOutput.m_damping;
				// fucomp / test ah, 0x44 / jnp: the target is stored unless it equals the previous one.
				if (fTarget != pPrevious[i]) {
					pPrevious[i] = fTarget;
				}
			}
			pCursor += sizeof(hkpRagdollMotorConstraintAtom);
			goto lRuntimeThree;
		}
		case hkpConstraintAtom::TYPE_PULLEY: {
			const hkpPulleyConstraintAtom* pPulley = static_cast<const hkpPulleyConstraintAtom*>(pAtom);
			__m128 kFixedB = _mm_load_ps(&reinterpret_cast<const float*>(&pPulley->m_fixedPivotBinWorld.m_quad)[0]);
			__m128 kFixedA = _mm_load_ps(&reinterpret_cast<const float*>(&pPulley->m_fixedPivotAinWorld.m_quad)[0]);
			__m128 kPivotB = _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldB.m_translation.m_quad)[0]);
			float fRopeLength = pPulley->m_ropeLength;
			hkpJacobianSchema* pJac = out.m_jacobianSchemas;
			float fLeverage = pPulley->m_leverageOnBodyB;
			__m128 kToA =
				_mm_sub_ps(kFixedA, _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0]));
			__m128 kToB = _mm_sub_ps(kPivotB, kFixedB);
			__m128 kSq = _mm_mul_ps(kToA, kToA);
			__m128 kLengthSq =
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
			__m128 kIdentity = _mm_load_ps(&reinterpret_cast<float*>(&hkTransformIdentity.m_rotation.m_col0.m_quad)[0]);
			__m128 kDirA;
			// fcom / test ah, 5 / jp: the identity's first column unless epsilon is below the length.
			if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
				// The SDK's normalize3 with the rsqrtss estimate and one Newton step (the exe's 0.0f at 0x1011D78,
				// 3.0f at 0x1017718 and 0.5f at 0x1016248); a zero length gives zero.
				__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
				__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
				__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
				__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
				kDirA = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToA);
			} else {
				kDirA = kIdentity;
			}
			kSq = _mm_mul_ps(kToB, kToB);
			kLengthSq = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
			__m128 kDirB;
			if (1.1920929e-07f < _mm_cvtss_f32(kLengthSq)) {
				__m128 kEstimate = _mm_rsqrt_ss(kLengthSq);
				__m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_set_ss(0.0f));
				__m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLengthSq, kEstimate), kEstimate));
				__m128 kInv = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
				kDirB = _mm_mul_ps(_mm_shuffle_ps(kInv, kInv, 0), kToB);
			} else {
				kDirB = _mm_xor_ps(kIdentity, kNegate);
			}
			hkpVelocityAccumulator* pBodyA = kIn.m_bodyA;
			hkpVelocityAccumulator* pBodyB = kIn.m_bodyB;
			float fInvDeltaTime = kIn.m_substepInvDeltaTime;
			float fVirtMassFactor = kIn.m_virtMassFactor;
			__m128 kPivotA = _mm_load_ps(&reinterpret_cast<float*>(&kLocalToWorldA.m_translation.m_quad)[0]);
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0], kDirA);
			__m128 kJacLinB = _mm_mul_ps(_mm_set1_ps(fLeverage), kDirB);
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[1].m_quad)[0], kJacLinB);
			__m128 kArmA = _mm_sub_ps(kPivotA, _mm_load_ps(&reinterpret_cast<float*>(&pBodyA->m_scratch0.m_quad)[0]));
			__m128 kArmB = _mm_sub_ps(kPivotB, _mm_load_ps(&reinterpret_cast<float*>(&pBodyB->m_scratch0.m_quad)[0]));
			__m128 kAngA =
				_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kDirA, kDirA, 0xD2), _mm_shuffle_ps(kArmA, kArmA, 0xC9)),
					_mm_mul_ps(_mm_shuffle_ps(kDirA, kDirA, 0xC9), _mm_shuffle_ps(kArmA, kArmA, 0xD2)));
			__m128 kAngB =
				_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xD2), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xC9)),
					_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xC9), _mm_shuffle_ps(kJacLinB, kJacLinB, 0xD2)));
			const hkRotation& kRotA = pBodyA->m_scratch123;
			const hkRotation& kRotB = pBodyB->m_scratch123;
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[2].m_quad)[0],
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x55),
										  _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col1.m_quad)[0])),
							   _mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0x00),
								   _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col0.m_quad)[0]))),
					_mm_mul_ps(_mm_shuffle_ps(kAngA, kAngA, 0xAA),
						_mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col2.m_quad)[0]))));
			__m128 kJacAngB =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x55),
										  _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col1.m_quad)[0])),
							   _mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0x00),
								   _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col0.m_quad)[0]))),
					_mm_mul_ps(_mm_shuffle_ps(kAngB, kAngB, 0xAA),
						_mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col2.m_quad)[0])));
			__m128 kJacAngA = _mm_load_ps(&reinterpret_cast<float*>(&pJac[2].m_quad)[0]);
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[3].m_quad)[0], kJacAngB);
			__m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacAngB, kJacAngB),
										  _mm_load_ps(&reinterpret_cast<float*>(&pBodyB->m_invMasses.m_quad)[0])),
				_mm_mul_ps(_mm_mul_ps(kJacAngA, kJacAngA),
					_mm_load_ps(&reinterpret_cast<float*>(&pBodyA->m_invMasses.m_quad)[0])));
			float fAngularMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
			float fLeverageSq = fLeverage * fLeverage;
			float fLinearMass =
				fLeverageSq * pBodyB->m_invMasses.m_quad[3] + pBodyA->m_invMasses.m_quad[3] + 1.1920928955078125e-07;
			__m128 kSegA = _mm_sub_ps(kFixedA, kPivotA);
			kSq = _mm_mul_ps(kSegA, kSegA);
			float fLengthA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
			float fInvMass = fLinearMass + fAngularMass;
			__m128 kSegB = _mm_sub_ps(kPivotB, kFixedB);
			kSq = _mm_mul_ps(kSegB, kSegB);
			float fLengthB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq))));
			pCursor += sizeof(hkpPulleyConstraintAtom);
			pJac[2].m_quad[3] = fVirtMassFactor / fInvMass;
			float fRope = fLeverage * fLengthB + fLengthA;
			float fError = fRope - fRopeLength;
			pJac[0].m_quad[3] = fError * fInvDeltaTime;
			hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
			out.m_jacobianSchemas = pSchema + 4;
			pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
			pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY;
			goto lRuntimeOne;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT: {
			const hkpSoftContactModifierConstraintAtom* pSoft =
				static_cast<const hkpSoftContactModifierConstraintAtom*>(pAtom);
			out.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			float fTau = pSoft->m_tau;
			kModifierIn = kIn;
			kModifierIn.m_virtMassFactor = fTau * kIn.m_virtMassFactor;
			kModifierIn.m_tau = fTau * kIn.m_tau;
			kModifierIn.m_damping = fTau * kIn.m_damping;
			int32_t iChildSize = pSoft->m_childSize;
			// The contact atom under the modifier chain.
			hkpConstraintAtom* pInner = pSoft->m_child;
			while (pInner->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
				pInner = static_cast<hkpModifierConstraintAtom*>(pInner)->m_child;
			}
			hkpSimpleContactConstraintAtom* pContact = static_cast<hkpSimpleContactConstraintAtom*>(pInner);
			int32_t iNumPoints = pContact->m_numContactPoints;
			hkContactPoint* pPoints = reinterpret_cast<hkContactPoint*>(pContact + 1);
			uint8_t* pProperties = reinterpret_cast<uint8_t*>(pPoints + pContact->m_numReservedContactPoints);
			uint32_t uiStriding = pContact->m_contactPointPropertiesStriding;
			// Each point's allowed penetration grows by the soft contact's speed limit over one substep.
			for (int32_t i = 0; i < iNumPoints; ++i) {
				float fDistance = pPoints[i].m_separatingNormal.m_quad[3];
				float fAllowed =
					pSoft->m_maxAcceleration * kIn.m_microStepDeltaTime * kIn.m_substepDeltaTime / pSoft->m_tau;
				hkpContactPointProperties* pProperty = reinterpret_cast<hkpContactPointProperties*>(pProperties);
				// fcomp / test ah, 0x41 / jne: stored only when the old value is above the new one.
				if (pProperty->m_internalDataA > fAllowed + fDistance) {
					pProperty->m_internalDataA = fAllowed + fDistance;
				}
				pProperties += uiStriding;
			}
			hkpConstraintAtom* pChild = pSoft->m_child;
			if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
				hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild),
					kModifierIn, hkBool{1}, out);
			} else {
				hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, out);
			}
			pCursor += sizeof(hkpSoftContactModifierConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER: {
			const hkpMassChangerModifierConstraintAtom* pMass =
				static_cast<const hkpMassChangerModifierConstraintAtom*>(pAtom);
			hkVector4 kFactorA = pMass->m_factorA;
			kModifierIn = kIn;
			hkVector4 kFactorB = pMass->m_factorB;
			hkpConstraintAtom* pChild = pMass->m_child;
			int32_t iChildSize = pMass->m_childSize;
			kAccA.copyVelocityAndScratch(*kIn.m_bodyA);
			kAccB.copyVelocityAndScratch(*kIn.m_bodyB);
			_mm_store_ps(&reinterpret_cast<float*>(&kAccA.m_invMasses.m_quad)[0],
				_mm_mul_ps(_mm_load_ps(&reinterpret_cast<float*>(&kAccA.m_invMasses.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kFactorA.m_quad)[0])));
			kModifierIn.m_bodyA = &kAccA;
			kModifierIn.m_bodyB = &kAccB;
			_mm_store_ps(&reinterpret_cast<float*>(&kAccB.m_invMasses.m_quad)[0],
				_mm_mul_ps(_mm_load_ps(&reinterpret_cast<float*>(&kAccB.m_invMasses.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kFactorB.m_quad)[0])));
			hkSetInvMassBuildJacobian(kAccA.m_invMasses, kAccB.m_invMasses, out);
			if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
				hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild),
					kModifierIn, hkBool{1}, out);
			} else {
				hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, out);
			}
			hkSetInvMassBuildJacobian(kIn.m_bodyA->m_invMasses, kIn.m_bodyB->m_invMasses, out);
			pCursor += sizeof(hkpMassChangerModifierConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_VISCOUS_SURFACE: {
			const hkpViscousSurfaceModifierConstraintAtom* pViscous =
				static_cast<const hkpViscousSurfaceModifierConstraintAtom*>(pAtom);
			out.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			kModifierIn = kIn;
			int32_t iChildSize = pViscous->m_childSize;
			hkpConstraintAtom* pInner = pViscous->m_child;
			while (pInner->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
				pInner = static_cast<hkpModifierConstraintAtom*>(pInner)->m_child;
			}
			// The info's words at +0xC, +0x14 and +0x1C (m_data[2], [4] and [6]) are zeroed.
			clearContactInfo(&static_cast<hkpSimpleContactConstraintAtom*>(pInner)->m_info);
			hkpConstraintAtom* pChild = pViscous->m_child;
			if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
				hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild),
					kModifierIn, hkBool{1}, out);
			} else {
				hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, out);
			}
			pCursor += sizeof(hkpViscousSurfaceModifierConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE: {
			const hkpMovingSurfaceModifierConstraintAtom* pMoving =
				static_cast<const hkpMovingSurfaceModifierConstraintAtom*>(pAtom);
			hkVector4 kVelocity = pMoving->m_velocity;
			kModifierIn = kIn;
			hkpSimpleContactConstraintAtom* pContact = static_cast<hkpSimpleContactConstraintAtom*>(pMoving->m_child);
			out.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			hkContactPoint* pPoints = reinterpret_cast<hkContactPoint*>(pContact + 1);
			buildJacobianHeader(kIn, out, pPoints + pContact->m_numReservedContactPoints,
				pContact->m_contactPointPropertiesStriding);
			// The exe keeps this flag as a byte: whether the moving body is B (body A is the fixed one).
			uint8_t bIsBodyB = kIn.m_accumulatorAIndex.m_storage == 0;
			if (!bIsBodyB) {
				kAccB = *kIn.m_bodyA;
				kIn.m_bodyA = &kAccB;
			} else {
				kAccB = *kIn.m_bodyB;
				_mm_store_ps(&reinterpret_cast<float*>(&kVelocity.m_quad)[0],
					_mm_xor_ps(_mm_load_ps(&reinterpret_cast<float*>(&kVelocity.m_quad)[0]),
						_mm_load_ps(&reinterpret_cast<float*>(&rdata_10C4C00)[0])));
				kIn.m_bodyB = &kAccB;
			}
			hkVector4 kNormal = pPoints[0].m_separatingNormal;
			__m128 kN = _mm_load_ps(&reinterpret_cast<float*>(&kNormal.m_quad)[0]);
			__m128 kV = _mm_load_ps(&reinterpret_cast<float*>(&kVelocity.m_quad)[0]);
			__m128 kDotV = _mm_mul_ps(kN, kV);
			__m128 kDot =
				_mm_add_ss(_mm_shuffle_ps(kDotV, kDotV, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDotV, kDotV, 0x55), kDotV));
			hkVector4 kTangential;
			_mm_store_ps(&reinterpret_cast<float*>(&kTangential.m_quad)[0],
				_mm_xor_ps(_mm_sub_ps(kV, _mm_mul_ps(_mm_shuffle_ps(kDot, kDot, 0), kN)), kNegate));
			int32_t iIsBodyB = bIsBodyB != 0;
			hkAddVelocityBuildJacobian(kTangential, iIsBodyB, out);
			_mm_store_ps(&reinterpret_cast<float*>(&kAccB.m_linearVel.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&kTangential.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kAccB.m_linearVel.m_quad)[0])));
			hkSimpleContactConstraintDataBuildJacobian(pContact, kModifierIn, hkBool{0}, out);
			_mm_store_ps(&reinterpret_cast<float*>(&kTangential.m_quad)[0],
				_mm_xor_ps(_mm_load_ps(&reinterpret_cast<float*>(&kTangential.m_quad)[0]), kNegate));
			hkAddVelocityBuildJacobian(kTangential, iIsBodyB, out);
			pCursor += sizeof(hkpMovingSurfaceModifierConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT: {
			out.m_jacobianSchemas -= HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			pCursor += sizeof(hkpIgnoreModifierConstraintAtom);
			break;
		}
		case hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER: {
			const hkpCenterOfMassChangerModifierConstraintAtom* pCom =
				static_cast<const hkpCenterOfMassChangerModifierConstraintAtom*>(pAtom);
			kModifierIn = kIn;
			hkpConstraintAtom* pChild = pCom->m_child;
			int32_t iChildSize = pCom->m_childSize;
			hkVector4 kDisplacementA;
			hkVector4 kDisplacementB;
			kDisplacementA.setRotatedDir(kIn.m_transformA->m_rotation, pCom->m_displacementA);
			kDisplacementB.setRotatedDir(kIn.m_transformB->m_rotation, pCom->m_displacementB);
			kAccA.copyVelocityAndScratch(*kIn.m_bodyA);
			kAccB.copyVelocityAndScratch(*kIn.m_bodyB);
			_mm_store_ps(&reinterpret_cast<float*>(&kAccA.m_scratch0.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&kDisplacementA.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kAccA.m_scratch0.m_quad)[0])));
			kModifierIn.m_bodyB = &kAccB;
			_mm_store_ps(&reinterpret_cast<float*>(&kAccB.m_scratch0.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&kDisplacementB.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kAccB.m_scratch0.m_quad)[0])));
			kModifierIn.m_bodyA = &kAccA;
			hkMatrix3 kSkewA;
			hkMatrix3 kSkewB;
			kSkewA.setCrossSkewSymmetric(kDisplacementA);
			kSkewB.setCrossSkewSymmetric(kDisplacementB);
			hkMatrix3 kCopy;
			kCopy.m_col0 = kSkewA.m_col0;
			kCopy.m_col1 = kSkewA.m_col1;
			kCopy.m_col2 = kSkewA.m_col2;
			kSkewA.setMulInverse(kCopy, kAccA.m_scratch123);
			kCopy.m_col0 = kSkewB.m_col0;
			kCopy.m_col1 = kSkewB.m_col1;
			kCopy.m_col2 = kSkewB.m_col2;
			kSkewB.setMulInverse(kCopy, kAccB.m_scratch123);
			// The negated skew matrices: copies scaled by -1.0f (the exe's float at 0x1012054, loaded with movss).
			hkMatrix3 kNegSkewA;
			kNegSkewA.m_col0 = kSkewA.m_col0;
			kNegSkewA.m_col1 = kSkewA.m_col1;
			kNegSkewA.m_col2 = kSkewA.m_col2;
			hkSimdReal kMinusOneA;
			_mm_store_ps(reinterpret_cast<float*>(&kMinusOneA.m_real), _mm_set_ss(-1.0f));
			kNegSkewA.mul(kMinusOneA);
			hkMatrix3 kNegSkewB;
			kNegSkewB.m_col0 = kSkewB.m_col0;
			kNegSkewB.m_col1 = kSkewB.m_col1;
			kNegSkewB.m_col2 = kSkewB.m_col2;
			hkSimdReal kMinusOneB;
			_mm_store_ps(reinterpret_cast<float*>(&kMinusOneB.m_real), _mm_set_ss(-1.0f));
			kNegSkewB.mul(kMinusOneB);
			hkVector4 kLinearA;
			hkVector4 kLinearB;
			kLinearA.setMul3(kNegSkewA, kAccA.m_angularVel);
			kLinearB.setMul3(kNegSkewB, kAccB.m_angularVel);
			_mm_store_ps(&reinterpret_cast<float*>(&kAccA.m_linearVel.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&kAccA.m_linearVel.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kLinearA.m_quad)[0])));
			_mm_store_ps(&reinterpret_cast<float*>(&kAccB.m_linearVel.m_quad)[0],
				_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&kAccB.m_linearVel.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kLinearB.m_quad)[0])));
			hkSetCenterOfMassBuildJacobian(kNegSkewA, kNegSkewB, out);
			if (pChild->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
				hkSimpleContactConstraintDataBuildJacobian(static_cast<hkpSimpleContactConstraintAtom*>(pChild),
					kModifierIn, hkBool{1}, out);
			} else {
				hkSolverBuildJacobianFromAtomsNotContact(pChild, iChildSize, kModifierIn, out);
			}
			hkSetCenterOfMassBuildJacobian(kSkewA, kSkewB, out);
			pCursor += sizeof(hkpCenterOfMassChangerModifierConstraintAtom);
			break;
		}
		default:
			// The exe does not move past an atom it does not know (TYPE_OVERWRITE_PIVOT, TYPE_CONTACT).
			break;
		}
		goto lNext;

	lShiftRuntimeOne:
		kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
		kIn.m_constraintRuntimeInMainMemory.m_storage =
			static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;
	lShift:
		{
			// A schema that only skips uiShift solver results.
			hkpJacobianSchema* pShift = out.m_jacobianSchemas;
			pShift->m_shiftSolverResults.m_numSolverResultsToSkip = uiShift;
			pShift->m_quad[0] = pShift->m_quad[0] + 1.1920928955078125e-07;
			pShift->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER;
			out.m_jacobianSchemas = pShift + 1;
			goto lNext;
		}

	lAngularLimit:
		{
			// The angular limit schema (0x30 bytes) along kLimitAxis, with fLimitTau, fLimitMin, fLimitMax and
			// fLimitAngle.
			hkpVelocityAccumulator* pBodyA = kIn.m_bodyA;
			hkpVelocityAccumulator* pBodyB = kIn.m_bodyB;
			hkpJacobianSchema* pJac = out.m_jacobianSchemas;
			__m128 kDir = _mm_load_ps(&reinterpret_cast<float*>(&kLimitAxis.m_quad)[0]);
			__m128 kNegDir = _mm_xor_ps(kDir, kNegate);
			const hkRotation& kRotA = pBodyA->m_scratch123;
			const hkRotation& kRotB = pBodyB->m_scratch123;
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0],
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x55),
										  _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col1.m_quad)[0])),
							   _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0x00),
								   _mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col0.m_quad)[0]))),
					_mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xAA),
						_mm_load_ps(&reinterpret_cast<const float*>(&kRotA.m_col2.m_quad)[0]))));
			__m128 kJacB =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x55),
										  _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col1.m_quad)[0])),
							   _mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0x00),
								   _mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col0.m_quad)[0]))),
					_mm_mul_ps(_mm_shuffle_ps(kNegDir, kNegDir, 0xAA),
						_mm_load_ps(&reinterpret_cast<const float*>(&kRotB.m_col2.m_quad)[0])));
			__m128 kJacA = _mm_load_ps(&reinterpret_cast<float*>(&pJac[0].m_quad)[0]);
			_mm_store_ps(&reinterpret_cast<float*>(&pJac[1].m_quad)[0], kJacB);
			__m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kJacA, kJacA),
										  _mm_load_ps(&reinterpret_cast<float*>(&pBodyA->m_invMasses.m_quad)[0])),
				_mm_mul_ps(_mm_mul_ps(kJacB, kJacB),
					_mm_load_ps(&reinterpret_cast<float*>(&pBodyB->m_invMasses.m_quad)[0])));
			float fInvMass = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(kMass, kMass, 0x55), kMass)));
			float fInvMassEps = fInvMass + 1.1920928955078125e-07;
			pJac[0].m_quad[3] = fLimitTau / fInvMassEps;
			float fNegInvDeltaTime = -kIn.m_substepInvDeltaTime;
			pJac[1].m_quad[3] = fLimitAngle * fNegInvDeltaTime;
			out.m_jacobianSchemas = pJac + 3;
			pJac[2].m_quad[0] = fLimitMin * fNegInvDeltaTime * fLimitTau;
			pJac[2].m_quad[1] = fNegInvDeltaTime * fLimitMax * fLimitTau;
			pJac[2].m_quad[2] = fLimitTau;
			pJac->m_quad[0] = pJac->m_quad[0] + 1.1920928955078125e-07;
			pJac->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS;
			goto lNext;
		}

	lRuntimeThree:
		kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 0x18;
		kIn.m_constraintRuntimeInMainMemory.m_storage =
			static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 0x18;
		goto lNext;

	lRuntimeOne:
		kIn.m_constraintRuntime.m_storage = static_cast<uint8_t*>(kIn.m_constraintRuntime.m_storage) + 8;
		kIn.m_constraintRuntimeInMainMemory.m_storage =
			static_cast<uint8_t*>(kIn.m_constraintRuntimeInMainMemory.m_storage) + 8;

	lNext:
		if (reinterpret_cast<uintptr_t>(pCursor) >= uiEnd) {
			break;
		}
	}
}
