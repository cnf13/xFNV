#pragma once

#include <cstdint>

class hkp1Lin2AngJacobian;
struct hkpConstraintChainMatrixTriple;
struct hkpConstraintChainTriple;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpVelocityAccumulator;
class hkpVelocityAccumulatorOffset;

void hkBallSocketChainBuildJacobian(
    int32_t numConstraints,
    float tau,
    float damping,
    float cfm,
    const hkpVelocityAccumulatorOffset* accumulators,
    const hkpVelocityAccumulator* accumBase,
    hkp1Lin2AngJacobian* jacobiansEnd,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
);
void hkStiffSpringChainBuildJacobian(
    int32_t numConstraints,
    float tau,
    float damping,
    float cfm,
    const hkpVelocityAccumulatorOffset* accumulators,
    const hkpVelocityAccumulator* accumBase,
    hkp1Lin2AngJacobian* jacobiansEnd,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
);
// The ball socket chain's block decomposition used by hkBallSocketChainBuildJacobian: triples is written as one
// hkpConstraintChainMatrixTriple per constraint; jacobians is read as hkp1Lin2AngJacobian rows, three per constraint;
// accumulators holds byte offsets added to accumsBase before reads of m_invMasses. No callers.
void buildBlockTridiagonalLu(
    hkpConstraintChainMatrixTriple* triples,
    int32_t numConstraints,
    float cfm,
    const hkp1Lin2AngJacobian* jacobians,
    const hkpVelocityAccumulatorOffset* accumulators,
    const hkpVelocityAccumulator* accumsBase
);
// The stiff spring chain's matrix decomposition, also performed inside hkStiffSpringChainBuildJacobian. triples is
// written as one hkpConstraintChainTriple per constraint; jacobians is read as hkp1Lin2AngJacobian rows; accumulators
// holds byte offsets added to accumsBase before reads of m_invMasses.
void buildTridiagonalLu(
    hkpConstraintChainTriple* triples,
    const hkp1Lin2AngJacobian* jacobians,
    int32_t numConstraints,
    float cfm,
    const hkpVelocityAccumulatorOffset* accumulators,
    const hkpVelocityAccumulator* accumsBase
);

// The chain constraints' Jacobian builders. Each writes its chain's schema at out.m_jacobianSchemas: the head, the
// accumulator offsets it copies from accumulators (numConstraints + 1 of them), the chain's matrix decomposition from
// the Jacobians already written after the head, and moves out.m_jacobianSchemas past the schema. jacobiansEnd and in
// are not read. The callers (hkpBallSocketChainData::buildJacobian, hkpStiffSpringChainData::buildJacobian) ignore the
// return value.
