#pragma once

#include "Reconstruction.hpp"

class hkp1Lin2AngJacobian;
struct hkpConstraintChainMatrixTriple;
struct hkpConstraintChainTriple;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpVelocityAccumulator;
class hkpVelocityAccumulatorOffset;

void hkBallSocketChainBuildJacobian(int32_t numConstraints, float tau, float damping, float cfm,
	const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumBase,
	hkp1Lin2AngJacobian* jacobiansEnd, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
void hkStiffSpringChainBuildJacobian(int32_t numConstraints, float tau, float damping, float cfm,
	const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumBase,
	hkp1Lin2AngJacobian* jacobiansEnd, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
// An out-of-line copy of the ball socket chain's block decomposition that hkBallSocketChainBuildJacobian (0xDD0AD0)
// inlines: triples (ecx) is written as one hkpConstraintChainMatrixTriple per constraint in 0x90-byte steps
// (0xDD06C4, 0xDD0AA0); jacobians is read as hkp1Lin2AngJacobian rows, three per constraint (0xDD0719, 0xDD0831);
// accumulators holds byte offsets added to accumsBase (0xDD072E) before reads of m_invMasses (+0x30). No callers.
// The exe takes triples in ecx and the rest on the stack, popped by the caller (usercall, ret 0). Its body runs on
// through 0xDD06D4 and 0xDD06E0 to the epilogue at 0xDD0AC2.
void buildBlockTridiagonalLu(hkpConstraintChainMatrixTriple* triples, int32_t numConstraints, float cfm,
	const hkp1Lin2AngJacobian* jacobians, const hkpVelocityAccumulatorOffset* accumulators,
	const hkpVelocityAccumulator* accumsBase);
// An out-of-line copy of the stiff spring chain's matrix decomposition that hkStiffSpringChainBuildJacobian
// (0xDD1180) inlines: triples (ecx) is written as one hkpConstraintChainTriple per constraint in 0xC-byte steps
// (0xDD1105, 0xDD114A); jacobians (edx) is read as hkp1Lin2AngJacobian rows in 0x30-byte steps (0xDD1038,
// 0xDD1145); accumulators holds byte offsets added to accumsBase (0xDD103F) before reads of m_invMasses (+0x30).
// No callers.
// The exe takes triples in ecx, jacobians in edx and the rest on the stack, popped by the caller (usercall, ret 0).
void buildTridiagonalLu(hkpConstraintChainTriple* triples, const hkp1Lin2AngJacobian* jacobians, int32_t numConstraints,
	float cfm, const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumsBase);

// The chain constraints' Jacobian builders (hkpChainBuildJacobian.cpp in hkpconstraintsolver.pdb; the SDK ships no
// header). Each writes its chain's schema at out.m_jacobianSchemas: the head, the accumulator offsets it copies from
// accumulators (numConstraints + 1 of them), the chain's matrix decomposition from the Jacobians already written
// after the head, and moves out.m_jacobianSchemas past the schema. jacobiansEnd and in are not read. cdecl, ret 0;
// the callers (hkpBallSocketChainData::buildJacobian 0xD53E80, hkpStiffSpringChainData::buildJacobian 0xE3B0F0)
// ignore eax.
