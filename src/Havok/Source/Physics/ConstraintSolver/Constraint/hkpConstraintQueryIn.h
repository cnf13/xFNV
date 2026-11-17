#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"

struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpJacobianSchema;
struct hkpSetLocalTranslationsConstraintAtom;
struct hkpSimpleContactConstraintAtom;
class hkpSimpleContactConstraintData;
class hkpVelocityAccumulator;
// The world's buffer of constraints over their violation limit; this code only passes its address on.
class hkpViolatedConstraintArray;
class hkTransform;

// Declared class to match its forward declarations. The world allocates it and its constructor zeroes the count.
class hkpViolatedConstraintArray {
public:
    uint32_t m_nextFreeElement;
    hkpConstraintInstance* m_constraints[0x80];
};

// The step's times and solver factors a constraint's Jacobian builder reads.
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

// The input of a constraint's Jacobian builder: the step info, the two bodies and the constraint.
class alignas(16) hkpConstraintQueryIn : public hkpConstraintQueryStepInfo {
public:
    // Fills the step info from the solver info and the step.
    void
    set(const hkpSolverInfo& si, const hkStepInfo& stepInfo, hkpViolatedConstraintArray* violatedConstraintsBuffer) {
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
        // hkSimdReal(const float&) written out: the float in lane 0, the others cleared.
        m_maxConstraintViolationSqrd.m_real = _mm_load_ss(&si.m_maxConstraintViolationSqrd);
    }

    hkpVelocityAccumulator* m_bodyA;
    hkpVelocityAccumulator* m_bodyB;
    const hkTransform* m_transformA;
    const hkTransform* m_transformB;
    float m_tau;
    float m_damping;
    // hkSimdReal is 16 bytes on a 16-byte boundary; the 8 bytes after m_damping are its alignment padding.
    hkSimdReal m_maxConstraintViolationSqrd;
    hkpConstraintInstance* m_constraintInstance;
    hkPadSpu<void*> m_constraintRuntime;
    hkpViolatedConstraintArray* m_violatedConstraints;
    hkPadSpu<uint32_t> m_accumulatorAIndex;
    hkPadSpu<uint32_t> m_accumulatorBIndex;
    hkPadSpu<void*> m_constraintRuntimeInMainMemory;
};

// Where a constraint's Jacobian builder writes the schemas it builds: the next schema's slot. The build-jacobians job
// keeps it as a local and passes its address to the builders.
class hkpConstraintQueryOut {
public:
    hkpJacobianSchema* m_jacobianSchemas;
};

// Builds the Jacobian schemas of a constraint's atoms that are not a contact and writes their schemas through
// angularLimitInfoBuffer. The body is in Atom/hkpBuildJacobianFromAtoms.cpp.
void hkSolverBuildJacobianFromAtomsNotContact(
    const hkpConstraintAtom* atoms,
    int32_t sizeOfAllAtoms,
    const hkpConstraintQueryIn& tmpIn,
    hkpConstraintQueryOut& angularLimitInfoBuffer
);
// Fires the contact point callbacks of a contact constraint's atom for the step aiStep.
// Defined in hkpSimpleContactConstraintData.cpp.
void hkSimpleContactConstraintData_fireCallbacks(
    hkpSimpleContactConstraintData* constraintData,
    const hkpConstraintQueryIn* in,
    hkpSimpleContactConstraintAtom* atom,
    hkpContactPointEvent::Type type
);
void setLocalTranslationsToWorld(
    const hkpConstraintQueryIn* apIn,
    const hkpSetLocalTranslationsConstraintAtom* apAtom,
    hkTransform* apOutA,
    hkTransform* apOutB
);
