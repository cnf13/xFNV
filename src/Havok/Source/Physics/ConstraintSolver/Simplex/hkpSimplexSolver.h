#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpSurfaceConstraintInfo.h"

// What one solver run did with one surface constraint: whether it was used, how long it held the character and how
// far the character had to be pushed off it.
class hkpSurfaceConstraintInteraction {
public:
    enum hkpStatus : int32_t {
        HK_STATUS_OK = 0x0,
        HK_STATUS_3D_FAILURE = 0x1,
        HK_STATUS_2D_FAILURE = 0x2,
    };

    uint8_t m_touched;
    uint8_t m_stopped;
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

// The solver's working set: the support planes it has chosen so far (four at most) and the query it answers.
class hkpSimplexSolver {
public:
    // One chosen surface: its index in the input, the constraint and the interaction record.
    struct SupportPlane {
        int32_t m_index;
        const hkpSurfaceConstraintInfo* m_constraint;
        hkpSurfaceConstraintInteraction* m_interaction;
    };

    // The solver proper: switches on the number of support planes and moves the output velocity on past them.
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
int32_t sortSupportPlanes(hkpSimplexSolver* solver);
bool* hkSimplexSolver_isOutsidePlane(bool* result, const __m128* velocity, const hkpSurfaceConstraintInfo* constraint);
void solveOneConstraint(
    const hkpSurfaceConstraintInfo* constraint,
    hkpSimplexSolver* solver,
    const hkVector4* velocity,
    hkVector4* velocityOut
);
void solve3d(
    hkpSimplexSolver* solver,
    const hkpSurfaceConstraintInfo* constraintA,
    const hkpSurfaceConstraintInfo* constraintB,
    int32_t sortPlanes,
    const hkVector4* velocity,
    hkVector4* velocityOut,
    const hkpSurfaceConstraintInfo* constraintC
);
void solveTwoConstraints(
    const hkpSurfaceConstraintInfo* constraintA,
    const hkpSurfaceConstraintInfo* constraintB,
    const hkVector4* velocity,
    hkVector4* velocityOut,
    hkpSimplexSolver* solver
);
