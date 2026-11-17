#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix6.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"

class hkpJacobianSchema;

// Out-of-line wrappers of the schema's static accessors, which the chain data calls from outside the solver library.
// getLinearJacobians returns the linear Jacobians, three per constraint, right after the schema head.
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
    // Inline accessors, and const views of the last two. After the head come three linear and three angular Jacobians
    // per constraint, the matrix buffer (one block triple per constraint), the velocity and temp buffers (2 *
    // numConstraints + 1 hkVector8), the accumulator offsets (one per body) and the child constraints' motor states.
    // The schema stream is raw memory laid out by byte offsets, hence the casts.
    const hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() const {
        return const_cast<hkpJacobianPoweredChainSchema*>(this)->getAccumulatorOffsetsBase();
    }

    const hkp3dAngularMotorSolverInfo* getChildConstraintStatusBase() const {
        return const_cast<hkpJacobianPoweredChainSchema*>(this)->getChildConstraintStatusBase();
    }

    hkpConstraintChainMatrix6Triple* getMatrixBuffer() {
        return reinterpret_cast<hkpConstraintChainMatrix6Triple*>(
            getAngularJacobians(reinterpret_cast<hkpJacobianSchema*>(this), m_numConstraints) + 3 * m_numConstraints
        );
    }

    hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() {
        return reinterpret_cast<hkpVelocityAccumulatorOffset*>(
            reinterpret_cast<hkVector8*>(getMatrixBuffer() + m_numConstraints) + 2 * m_numConstraints + 1
        );
    }

    hkp3dAngularMotorSolverInfo* getChildConstraintStatusBase() {
        return reinterpret_cast<hkp3dAngularMotorSolverInfo*>(getAccumulatorOffsetsBase() + m_numConstraints + 1);
    }

    static hkp1Lin2AngJacobian* getLinearJacobians(hkpJacobianSchema* schema) {
        return reinterpret_cast<hkp1Lin2AngJacobian*>(
            reinterpret_cast<char*>(schema) + sizeof(hkpJacobianPoweredChainSchema)
        );
    }

    static hkp2AngJacobian* getAngularJacobians(hkpJacobianSchema* schema, int32_t numConstraints) {
        return reinterpret_cast<hkp2AngJacobian*>(getLinearJacobians(schema) + 3 * numConstraints);
    }

    // The size stored in m_size for auNumConstraints constraints, rounded up to the next multiple of 16.
    static uint32_t getSize(uint16_t auNumConstraints) {
        const uint32_t uiBytes = static_cast<uint32_t>(
            sizeof(hkpJacobianPoweredChainSchema) +
            auNumConstraints *
                (3 * sizeof(hkp1Lin2AngJacobian) + 3 * sizeof(hkp2AngJacobian) +
                 sizeof(hkpConstraintChainMatrix6Triple) + 2 * sizeof(hkVector8) +
                 sizeof(hkpVelocityAccumulatorOffset) + sizeof(hkp3dAngularMotorSolverInfo)) +
            sizeof(hkVector8) + sizeof(hkpVelocityAccumulatorOffset)
        );
        return (uiBytes + 15) & ~15u;
    }

    uint32_t m_typeBuffer;
    uint16_t m_numConstraints;

    // Anonymous union: the status pointer padded to 8 bytes.
    union {
        uint8_t* m_statusFlagsOutput;
        uint64_t m_padding1;
    };

    uint32_t m_size;
    float m_maxTorqueHysterisys;
    float m_tau;
    float m_damping;
};
