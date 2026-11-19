#include "Havok/Source/Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h"

#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The solver proper: moves the output velocity on past the support planes chosen so far. One and two planes end the
// solve; three and four planes either solve or drop a plane, and a dropped plane sends it back through the switch on
// the new count.
void hkpSimplexSolver::solve() {
    for (;;) {
        switch (m_numSupportPlanes) {
            case 1: {
                solveOneConstraint(m_supportPlanes[0].m_constraint, this, &m_input->m_velocity, &m_output->m_velocity);
                return;
            }
            case 2: {
                hkVector4 velocity;
                solveOneConstraint(m_supportPlanes[1].m_constraint, this, &m_input->m_velocity, &velocity);
                const hkpSurfaceConstraintInfo* constraint0 = m_supportPlanes[0].m_constraint;
                bool inside;
                hkSimplexSolver_isOutsidePlane(&inside, &velocity.m_quad, constraint0);
                if (!inside) {
                    m_output->m_velocity.m_quad = velocity.m_quad;
                    m_supportPlanes[0] = m_supportPlanes[1];
                    m_numSupportPlanes = 1;
                    return;
                }
                solveTwoConstraints(
                    constraint0, m_supportPlanes[1].m_constraint, &m_input->m_velocity, &m_output->m_velocity, this
                );
                return;
            }
            case 3: {
                hkVector4 velocity;
                solveOneConstraint(m_supportPlanes[2].m_constraint, this, &m_input->m_velocity, &velocity);
                alignas(16) bool inside0;
                hkSimplexSolver_isOutsidePlane(&inside0, &velocity.m_quad, m_supportPlanes[0].m_constraint);
                if (!inside0) {
                    bool inside1;
                    hkSimplexSolver_isOutsidePlane(&inside1, &velocity.m_quad, m_supportPlanes[1].m_constraint);
                    if (!inside1) {
                        m_output->m_velocity.m_quad = velocity.m_quad;
                        m_supportPlanes[0] = m_supportPlanes[2];
                        m_numSupportPlanes = 1;
                        continue;
                    }
                }
                // Solve the third plane with each of the first two; keep the pair whose result the other plane does not
                // push back.
                hkVector4 pairVelocity;
                int32_t i = 0;
                for (; i < 2; ++i) {
                    solveTwoConstraints(
                        m_supportPlanes[i].m_constraint, m_supportPlanes[2].m_constraint, &m_input->m_velocity,
                        &pairVelocity, this
                    );
                    bool inside;
                    if (!*hkSimplexSolver_isOutsidePlane(
                            &inside, &pairVelocity.m_quad, m_supportPlanes[1 - i].m_constraint
                        )) {
                        break;
                    }
                }
                if (i < 2) {
                    m_supportPlanes[0] = m_supportPlanes[i];
                    --m_numSupportPlanes;
                    m_supportPlanes[1] = m_supportPlanes[2];
                    continue;
                }
                solve3d(
                    this, m_supportPlanes[1].m_constraint, m_supportPlanes[2].m_constraint, 1, &m_input->m_velocity,
                    &m_output->m_velocity, m_supportPlanes[0].m_constraint
                );
                return;
            }
            case 4: {
                sortSupportPlanes(this);
                // Solve the fourth plane with each pair of the first three; keep the triple whose result the plane left
                // out does not push back.
                hkVector4 tripleVelocity;
                int32_t i = 1;
                for (; i < 4; ++i) {
                    solve3d(
                        this, m_supportPlanes[(i + 1) % 3].m_constraint, m_supportPlanes[3].m_constraint, 0,
                        &m_input->m_velocity, &tripleVelocity, m_supportPlanes[i % 3].m_constraint
                    );
                    bool inside;
                    hkSimplexSolver_isOutsidePlane(
                        &inside, &tripleVelocity.m_quad, m_supportPlanes[i - 1].m_constraint
                    );
                    if (!inside) {
                        break;
                    }
                }
                if (i < 4) {
                    m_supportPlanes[i - 1] = m_supportPlanes[2];
                    m_supportPlanes[2] = m_supportPlanes[3];
                    m_numSupportPlanes = 3;
                    continue;
                }
                // Every triple failed: solve the four triples from the input velocity in turn, as long as none of them
                // drops a plane. The four constraints and the count are read once, before the first solve.
                hkVector4 velocity;
                velocity.m_quad = m_input->m_velocity.m_quad;
                const hkpSurfaceConstraintInfo* constraint0 = m_supportPlanes[0].m_constraint;
                const hkpSurfaceConstraintInfo* constraint1 = m_supportPlanes[1].m_constraint;
                const hkpSurfaceConstraintInfo* constraint2 = m_supportPlanes[2].m_constraint;
                const hkpSurfaceConstraintInfo* constraint3 = m_supportPlanes[3].m_constraint;
                const int32_t numSupportPlanes = m_numSupportPlanes;
                solve3d(this, constraint1, constraint2, 0, &velocity, &velocity, constraint0);
                if (numSupportPlanes == m_numSupportPlanes) {
                    solve3d(this, constraint1, constraint3, 0, &velocity, &velocity, constraint0);
                }
                if (numSupportPlanes == m_numSupportPlanes) {
                    solve3d(this, constraint2, constraint3, 0, &velocity, &velocity, constraint0);
                }
                if (numSupportPlanes == m_numSupportPlanes) {
                    solve3d(this, constraint2, constraint3, 0, &velocity, &velocity, constraint1);
                }
                m_output->m_velocity.m_quad = velocity.m_quad;
                // Drop the plane with the worst status (the first of them), then clear the status of the three kept.
                hkpSurfaceConstraintInteraction::hkpStatus maxStatus = hkpSurfaceConstraintInteraction::HK_STATUS_OK;
                for (int32_t j = 0; j < 4; ++j) {
                    if (m_supportPlanes[j].m_interaction->m_status >= maxStatus) {
                        maxStatus = m_supportPlanes[j].m_interaction->m_status;
                    }
                }
                for (int32_t j = 0; j < 4; ++j) {
                    if (maxStatus == m_supportPlanes[j].m_interaction->m_status) {
                        m_supportPlanes[j] = m_supportPlanes[3];
                        break;
                    }
                }
                --m_numSupportPlanes;
                m_supportPlanes[0].m_interaction->m_status = hkpSurfaceConstraintInteraction::HK_STATUS_OK;
                m_supportPlanes[1].m_interaction->m_status = hkpSurfaceConstraintInteraction::HK_STATUS_OK;
                m_supportPlanes[2].m_interaction->m_status = hkpSurfaceConstraintInteraction::HK_STATUS_OK;
                continue;
            }
            default: return;
        }
    }
}

// The sums, products and the division below are written in double and narrowed to float where the game stores a float,
// to match the game's double-precision arithmetic. The comparisons of two hkReals and of an hkReal against a float
// constant are exact either way.
void hkSimplexSolverSolve(const hkpSimplexSolverInput& input, hkpSimplexSolverOutput& output) {
    // The solver is 16-aligned; its support planes start out uninitialised.
    alignas(16) hkpSimplexSolver solver;
    output.m_velocity = input.m_velocity;
    float timeRemaining = input.m_deltaTime;
    solver.m_numSupportPlanes = 0;
    output.m_position = input.m_position;
    solver.m_currentTime = 0.0f;
    solver.m_input = &input;
    solver.m_output = &output;
    for (int32_t i = 0; i < input.m_numConstraints; ++i) {
        hkpSurfaceConstraintInteraction& interaction = output.m_planeInteractions[i];
        interaction.m_surfaceTime = 0.0f;
        interaction.m_penaltyDistance = 0.0f;
        interaction.m_stopped = 0;
        interaction.m_touched = 0;
        interaction.m_status = hkpSurfaceConstraintInteraction::HK_STATUS_OK;
    }
    // The negated compare also exits on an unordered (NaN) time.
    if (!(timeRemaining >= 0.0f)) {
        return;
    }
    for (;;) {
        int32_t chosenIndex = -1;
        float bestTime = timeRemaining;
        const float currentTime = solver.m_currentTime;
        for (int32_t i = 0; i < input.m_numConstraints; ++i) {
            if ((solver.m_numSupportPlanes < 1 || solver.m_supportPlanes[0].m_index != i) &&
                (solver.m_numSupportPlanes < 2 || solver.m_supportPlanes[1].m_index != i) &&
                (solver.m_numSupportPlanes < 3 || solver.m_supportPlanes[2].m_index != i) &&
                output.m_planeInteractions[i].m_status == hkpSurfaceConstraintInteraction::HK_STATUS_OK) {
                const hkpSurfaceConstraintInfo* constraint = input.m_constraints + i;
                hkVector4 difference = output.m_velocity;
                difference.sub4(constraint->m_velocity);
                const float distance = -difference.dot3(constraint->m_plane);
                // The compare falls through for a NaN distance as well as for a positive one.
                if (!(distance <= 0.0f)) {
                    hkVector4 offset;
                    offset.setAll(-currentTime);
                    offset.mul4(constraint->m_velocity);
                    offset.add4(output.m_position);
                    float timeToImpact = constraint->m_plane.dot4xyz1(offset);
                    // FLT_EPSILON.
                    if (timeToImpact <= 1.1920929e-07f) {
                        timeToImpact = 0.0f;
                    }
                    const hkpSurfaceConstraintInteraction& interaction = output.m_planeInteractions[i];
                    const float penalty = static_cast<float>(
                        static_cast<double>(interaction.m_penaltyDistance) + static_cast<double>(timeToImpact)
                    );
                    if (static_cast<double>(distance) * static_cast<double>(bestTime) > static_cast<double>(penalty)) {
                        chosenIndex = i;
                        bestTime = static_cast<float>(static_cast<double>(penalty) / static_cast<double>(distance));
                    }
                }
            }
        }
        if (static_cast<double>(bestTime) > 0.00009999999747378752) {
            solver.m_currentTime = static_cast<float>(static_cast<double>(currentTime) + static_cast<double>(bestTime));
            hkVector4 scaledVelocity;
            scaledVelocity.setAll(bestTime);
            scaledVelocity.mul4(output.m_velocity);
            output.m_position.add4(scaledVelocity);
            timeRemaining = static_cast<float>(static_cast<double>(timeRemaining) - static_cast<double>(bestTime));
            for (int32_t j = 0; j < solver.m_numSupportPlanes; ++j) {
                hkpSurfaceConstraintInteraction* interaction = solver.m_supportPlanes[j].m_interaction;
                interaction->m_surfaceTime =
                    static_cast<float>(static_cast<double>(interaction->m_surfaceTime) + static_cast<double>(bestTime));
                interaction->m_touched = 1;
            }
            output.m_deltaTime = solver.m_currentTime;
            if (input.m_minDeltaTime < solver.m_currentTime) {
                return;
            }
        }
        if (chosenIndex < 0) {
            output.m_deltaTime = input.m_deltaTime;
            return;
        }
        hkpSimplexSolver::SupportPlane& plane = solver.m_supportPlanes[solver.m_numSupportPlanes];
        ++solver.m_numSupportPlanes;
        hkpSurfaceConstraintInteraction* interaction = output.m_planeInteractions + chosenIndex;
        plane.m_index = chosenIndex;
        plane.m_constraint = input.m_constraints + chosenIndex;
        plane.m_interaction = interaction;
        // FLT_EPSILON is added in double precision, and the sum is doubled in double as well.
        interaction->m_penaltyDistance =
            static_cast<float>((static_cast<double>(interaction->m_penaltyDistance) + 1.1920928955078125e-07) * 2.0);
        solver.solve();
        if (!(timeRemaining >= 0.0f)) {
            return;
        }
    }
}

// Sorts the support planes: a plane moves after a later one of lower priority, or of the same priority whose
// constraint velocity is not longer (squared lengths compared); the planes are swapped whole, member by member.
int32_t sortSupportPlanes(hkpSimplexSolver* a1) {
    hkpSimplexSolver* v1 = a1;
    int32_t m_numSupportPlanes = a1->m_numSupportPlanes;
    int32_t m_numSupportPlanes_2 = m_numSupportPlanes - 1;
    if (m_numSupportPlanes - 1 > 0) {
        m_numSupportPlanes_2 = 1;
        int32_t m_numSupportPlanes_3 = 1;
        hkpSimplexSolver::SupportPlane* pPlaneI = &a1->m_supportPlanes[0];
        do {
            int32_t m_numSupportPlanes_1 = m_numSupportPlanes_2;
            if (m_numSupportPlanes_2 < m_numSupportPlanes) {
                hkpSimplexSolver::SupportPlane* pPlaneJ = pPlaneI + 1;
                do {
                    const hkpSurfaceConstraintInfo* m_constraint = pPlaneI->m_constraint;
                    int32_t m_priority = pPlaneI->m_constraint->m_priority;
                    int32_t m_priority_1 = pPlaneJ->m_constraint->m_priority;
                    if (m_priority >= m_priority_1) {
                        bool bSwap = m_priority != m_priority_1;
                        if (!bSwap) {
                            // The squared lengths are SSE sums; the order is kept only when J's is the longer (a NaN
                            // swaps).
                            const __m128 kI =
                                _mm_mul_ps(m_constraint->m_velocity.m_quad, m_constraint->m_velocity.m_quad);
                            const float fLenI = _mm_cvtss_f32(
                                _mm_add_ss(_mm_shuffle_ps(kI, kI, 170), _mm_add_ss(_mm_shuffle_ps(kI, kI, 85), kI))
                            );
                            const __m128 kJ = _mm_mul_ps(
                                pPlaneJ->m_constraint->m_velocity.m_quad, pPlaneJ->m_constraint->m_velocity.m_quad
                            );
                            const float fLenJ = _mm_cvtss_f32(
                                _mm_add_ss(_mm_shuffle_ps(kJ, kJ, 170), _mm_add_ss(_mm_shuffle_ps(kJ, kJ, 85), kJ))
                            );
                            bSwap = !(fLenJ > fLenI);
                        }
                        if (bSwap) {
                            int32_t v13 = pPlaneI->m_index;
                            hkpSurfaceConstraintInteraction* v14 = pPlaneI->m_interaction;
                            pPlaneI->m_index = pPlaneJ->m_index;
                            pPlaneI->m_constraint = pPlaneJ->m_constraint;
                            pPlaneI->m_interaction = pPlaneJ->m_interaction;
                            pPlaneJ->m_index = v13;
                            pPlaneJ->m_constraint = m_constraint;
                            pPlaneJ->m_interaction = v14;
                        }
                    }
                    v1 = a1;
                    ++pPlaneJ;
                    ++m_numSupportPlanes_1;
                }
                while (m_numSupportPlanes_1 < a1->m_numSupportPlanes);
                m_numSupportPlanes_2 = m_numSupportPlanes_3;
            }
            m_numSupportPlanes = v1->m_numSupportPlanes;
            ++m_numSupportPlanes_2;
            ++pPlaneI;
            m_numSupportPlanes_3 = m_numSupportPlanes_2;
        }
        while (m_numSupportPlanes_2 - 1 < m_numSupportPlanes - 1);
    }
    return m_numSupportPlanes_2;
}

// Whether the velocity a2 moves into r_eax's plane: the dot product of (a2 - plane velocity) with the plane normal,
// against -0.001. Stores 1 when the dot is below it or unordered: a NaN counts as inside.
bool* hkSimplexSolver_isOutsidePlane(bool* a1, const __m128* a2, const hkpSurfaceConstraintInfo* r_eax) {
    __m128 v3 = _mm_mul_ps(_mm_sub_ps(*a2, r_eax->m_velocity.m_quad), r_eax->m_plane.m_quad);
    const float fDot =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v3, v3, 170), _mm_add_ss(_mm_shuffle_ps(v3, v3, 85), v3)));
    *a1 = !(fDot >= -0.001f);
    return a1;
}

// Clips the velocity r_eax against one surface constraint a1 into r_esi. The lane sums are added as (y + x) + z in the
// low lane, and the plane dot is squared; in the friction tests and the scale factors, each value is rounded to a float
// local of its own. The frictions are plain float copies.
void solveOneConstraint(
    const hkpSurfaceConstraintInfo* a1,
    hkpSimplexSolver* a2,
    const hkVector4* r_eax,
    hkVector4* r_esi
) {
    const __m128 kPlane = a1->m_plane.m_quad;
    const float fStaticFriction = a1->m_staticFriction;
    const __m128 kVelocity = a1->m_velocity.m_quad;
    const float fDynamicFriction = a1->m_dynamicFriction;
    const hkpSimplexSolverInput* pInput = a2->m_input;
    const __m128 kRel = _mm_sub_ps(r_eax->m_quad, kVelocity);
    const __m128 kPd = _mm_mul_ps(kPlane, kRel);
    const __m128 kDot = _mm_add_ss(_mm_shuffle_ps(kPd, kPd, 170), _mm_add_ss(_mm_shuffle_ps(kPd, kPd, 85), kPd));
    const __m128 kRr = _mm_mul_ps(kRel, kRel);
    const float fRelLen2 =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kRr, kRr, 170), _mm_add_ss(_mm_shuffle_ps(kRr, kRr, 85), kRr)));
    __m128 kTangent = _mm_sub_ps(kRel, _mm_mul_ps(_mm_shuffle_ps(kDot, kDot, 0), kPlane));
    const float fDot2 = _mm_cvtss_f32(_mm_mul_ss(kDot, kDot));
    const __m128 kUd = _mm_mul_ps(pInput->m_upVector.m_quad, kTangent);
    const float fUpDot =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kUd, kUd, 170), _mm_add_ss(_mm_shuffle_ps(kUd, kUd, 85), kUd)));
    // Takes the down friction unless the tangent points up (a NaN takes the down one).
    const float fExtra = fUpDot > 0.0f ? a1->m_extraUpStaticFriction : a1->m_extraDownStaticFriction;
    if (!(fExtra > 0.0f)) {
        const float fStatic2 = fStaticFriction * fStaticFriction;
        // The friction goes on when the length is greater or unordered.
        if (fRelLen2 <= (fStatic2 + 1.0) * fDot2) {
            r_esi->m_quad = kVelocity;
            return;
        }
    }
    else {
        float fProj = 0.0f;
        const __m128 kUp = pInput->m_upVector.m_quad;
        __m128 kPerp = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kPlane, kPlane, 210), _mm_shuffle_ps(kUp, kUp, 201)),
            _mm_mul_ps(_mm_shuffle_ps(kPlane, kPlane, 201), _mm_shuffle_ps(kUp, kUp, 210))
        );
        const __m128 kPp = _mm_mul_ps(kPerp, kPerp);
        const float fPerpLen2 =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kPp, kPp, 170), _mm_add_ss(_mm_shuffle_ps(kPp, kPp, 85), kPp)));
        if (fPerpLen2 > 1.1920929e-07f) {
            const float fPerpLen = std::sqrt(fPerpLen2);
            const float fInvPerpLen = 1.0 / fPerpLen;
            kPerp = _mm_mul_ps(_mm_set1_ps(fInvPerpLen), kPerp);
            const __m128 kPt = _mm_mul_ps(kPerp, kTangent);
            fProj =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kPt, kPt, 170), _mm_add_ss(_mm_shuffle_ps(kPt, kPt, 85), kPt)));
            const float fStatic2 = fStaticFriction * fStaticFriction;
            const float fProj2 = fProj * fProj;
            if (fProj2 <= fStatic2 * fDot2) {
                kTangent = _mm_sub_ps(kTangent, _mm_mul_ps(_mm_set1_ps(fProj), kPerp));
                fProj = 0.0f;
            }
        }
        const float fCone2 = (fExtra + fStaticFriction) * (fExtra + fStaticFriction);
        const float fRest = fRelLen2 - fProj * fProj - fDot2;
        // A greater or unordered rest goes on to the friction.
        if (fRest <= fCone2 * fDot2) {
            // Only an equal compare returns.
            if (fProj == 0.0f) {
                r_esi->m_quad = kVelocity;
                return;
            }
            kTangent = _mm_mul_ps(_mm_set1_ps(fProj), kPerp);
        }
    }
    if (fDynamicFriction < 1.0f) {
        const __m128 kTt = _mm_mul_ps(kTangent, kTangent);
        const float fTan2 =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kTt, kTt, 170), _mm_add_ss(_mm_shuffle_ps(kTt, kTt, 85), kTt)));
        if (fTan2 >= 1.1920929e-07f && fRelLen2 * 0.0010000000474974513 < fTan2) {
            const float fRatio = fRelLen2 / fTan2;
            const float fRoot = std::sqrt(fRatio);
            const float fScale = fRoot * (1.0 - fDynamicFriction) + fDynamicFriction;
            const __m128 kScaled = _mm_mul_ps(_mm_set1_ps(fScale), kTangent);
            const __m128 kSp = _mm_mul_ps(kPlane, kScaled);
            const __m128 kN = _mm_add_ss(_mm_shuffle_ps(kSp, kSp, 170), _mm_add_ss(_mm_shuffle_ps(kSp, kSp, 85), kSp));
            r_esi->m_quad = _mm_add_ps(kVelocity, _mm_sub_ps(kScaled, _mm_mul_ps(_mm_shuffle_ps(kN, kN, 0), kPlane)));
            return;
        }
    }
    r_esi->m_quad = _mm_add_ps(kVelocity, kTangent);
}

// Solves the velocity a6 on the corner of three constraint planes (a2, a3, r_eax), or, where the planes are degenerate
// or the corner velocity is too fast, drops to their pairs through solveTwoConstraints. The lane sums are summed as (y
// + x) + z in the low lane.
void solve3d(
    hkpSimplexSolver* a1,
    const hkpSurfaceConstraintInfo* a2,
    const hkpSurfaceConstraintInfo* a3,
    int32_t a4,
    const hkVector4* a5,
    hkVector4* a6,
    const hkpSurfaceConstraintInfo* r_eax
) {
    const hkpSurfaceConstraintInfo* pConstraintA = a2;
    const hkpSurfaceConstraintInfo* pConstraintB = a3;
    const hkpSurfaceConstraintInfo* pConstraintC = r_eax;
    const __m128 kPlaneA = a2->m_plane.m_quad;
    const __m128 kPlaneB = a3->m_plane.m_quad;
    const __m128 kPlaneBZxy = _mm_shuffle_ps(kPlaneB, kPlaneB, 210);
    const __m128 kPlaneBYzx = _mm_shuffle_ps(kPlaneB, kPlaneB, 201);
    const __m128 kPlaneAYzx = _mm_shuffle_ps(kPlaneA, kPlaneA, 201);
    const __m128 kPlaneAZxy = _mm_shuffle_ps(kPlaneA, kPlaneA, 210);
    const __m128 kCrossBA = _mm_sub_ps(_mm_mul_ps(kPlaneBZxy, kPlaneAYzx), _mm_mul_ps(kPlaneBYzx, kPlaneAZxy));
    const __m128 kPlaneC = r_eax->m_plane.m_quad;
    const __m128 kPlaneCZxy = _mm_shuffle_ps(kPlaneC, kPlaneC, 210);
    const __m128 kPlaneCYzx = _mm_shuffle_ps(kPlaneC, kPlaneC, 201);
    const __m128 kCrossCA = _mm_sub_ps(_mm_mul_ps(kPlaneCYzx, kPlaneAZxy), _mm_mul_ps(kPlaneCZxy, kPlaneAYzx));
    const __m128 kCrossCB = _mm_sub_ps(_mm_mul_ps(kPlaneCZxy, kPlaneBYzx), _mm_mul_ps(kPlaneCYzx, kPlaneBZxy));
    const __m128 kDet = _mm_mul_ps(kPlaneC, kCrossBA);
    const float fDet =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDet, kDet, 170), _mm_add_ss(_mm_shuffle_ps(kDet, kDet, 85), kDet)));
    const float fAbsDet = std::fabs(fDet);
    // Only a smaller |det| fails; a NaN goes on.
    if (!(fAbsDet < 1.1920929e-07f)) {
        const __m128 kDb = _mm_mul_ps(a3->m_velocity.m_quad, kPlaneB);
        const float fDotB =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDb, kDb, 170), _mm_add_ss(_mm_shuffle_ps(kDb, kDb, 85), kDb)));
        const __m128 kDa = _mm_mul_ps(a2->m_velocity.m_quad, kPlaneA);
        const float fDotA =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDa, kDa, 170), _mm_add_ss(_mm_shuffle_ps(kDa, kDa, 85), kDa)));
        const float fInvDet = 1.0 / fDet;
        const __m128 kDc = _mm_mul_ps(r_eax->m_velocity.m_quad, kPlaneC);
        const __m128 kDotC = _mm_add_ss(_mm_shuffle_ps(kDc, kDc, 170), _mm_add_ss(_mm_shuffle_ps(kDc, kDc, 85), kDc));
        const __m128 kRhs = _mm_unpacklo_ps(
            _mm_unpacklo_ps(kDotC, _mm_set_ss(fDotB)), _mm_unpacklo_ps(_mm_set_ss(fDotA), _mm_set_ss(zero))
        );
        const __m128 kSum = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 85), kCrossCB), _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 0), kCrossBA)
        );
        const __m128 kPoint =
            _mm_mul_ps(_mm_add_ps(kSum, _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 170), kCrossCA)), _mm_set1_ps(fInvDet));
        const __m128 kTooFast =
            _mm_cmplt_ps(a1->m_input->m_maxSurfaceVelocity.m_quad, _mm_and_ps(kPoint, hkVector4_setAbs4_absMask));
        if (!(_mm_movemask_ps(kTooFast) & 7)) {
            a6->m_quad = kPoint;
            return;
        }
    }
    if (a4) {
        sortSupportPlanes(a1);
        pConstraintC = a1->m_supportPlanes[0].m_constraint;
        pConstraintA = a1->m_supportPlanes[1].m_constraint;
        pConstraintB = a1->m_supportPlanes[2].m_constraint;
    }
    // Each constraint's index is a truncating shift right by 6 of its byte distance from the constraints array (the
    // 64-byte element), not a pointer difference's exact division, as the game does.
    const char* pConstraints = reinterpret_cast<const char*>(a1->m_input->m_constraints);
    a1->m_output->m_planeInteractions[(reinterpret_cast<const char*>(pConstraintC) - pConstraints) >> 6].m_status =
        hkpSurfaceConstraintInteraction::HK_STATUS_3D_FAILURE;
    a1->m_output->m_planeInteractions[(reinterpret_cast<const char*>(pConstraintA) - pConstraints) >> 6].m_status =
        hkpSurfaceConstraintInteraction::HK_STATUS_3D_FAILURE;
    a1->m_output->m_planeInteractions[(reinterpret_cast<const char*>(pConstraintB) - pConstraints) >> 6].m_status =
        hkpSurfaceConstraintInteraction::HK_STATUS_3D_FAILURE;
    const int32_t iNumSupportPlanes = a1->m_numSupportPlanes;
    solveTwoConstraints(pConstraintC, pConstraintA, a5, a6, a1);
    if (iNumSupportPlanes == a1->m_numSupportPlanes) {
        solveTwoConstraints(pConstraintC, pConstraintB, a6, a6, a1);
    }
    if (iNumSupportPlanes == a1->m_numSupportPlanes) {
        solveTwoConstraints(pConstraintA, pConstraintB, a6, a6, a1);
    }
}

// Clips the velocity a3 against the crease of two constraint planes into a4, or, where the planes are parallel or the
// crease velocity is too fast, against each plane in priority order. The lane sums are (y + x) + z in the low lane; the
// friction tests and the scale factors each store their value in a float local of its own. The plane dots are written
// as plain float copies.
void solveTwoConstraints(
    const hkpSurfaceConstraintInfo* a1,
    const hkpSurfaceConstraintInfo* a2,
    const hkVector4* a3,
    hkVector4* a4,
    hkpSimplexSolver* r_edi
) {
    const __m128 kPlane1 = a1->m_plane.m_quad;
    const __m128 kPlane1Yzx = _mm_shuffle_ps(kPlane1, kPlane1, 201);
    const __m128 kPlane2 = a2->m_plane.m_quad;
    const __m128 kPlane2Yzx = _mm_shuffle_ps(kPlane2, kPlane2, 201);
    const __m128 kPlane2Zxy = _mm_shuffle_ps(kPlane2, kPlane2, 210);
    const __m128 kPlane1Zxy = _mm_shuffle_ps(kPlane1, kPlane1, 210);
    const __m128 kCross = _mm_sub_ps(_mm_mul_ps(kPlane2Zxy, kPlane1Yzx), _mm_mul_ps(kPlane2Yzx, kPlane1Zxy));
    const __m128 kCc = _mm_mul_ps(kCross, kCross);
    const float fLen2 =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCc, kCc, 170), _mm_add_ss(_mm_shuffle_ps(kCc, kCc, 85), kCc)));
    // A length at or below epsilon fails; a NaN goes on.
    if (!(fLen2 <= 1.1920929e-07f)) {
        const float fLen = std::sqrt(fLen2);
        const float fInvLen = 1.0 / fLen;
        const __m128 kInvLen = _mm_set1_ps(fInvLen);
        const __m128 kN = _mm_mul_ps(kInvLen, kCross);
        const __m128 kNZxy = _mm_shuffle_ps(kN, kN, 210);
        const __m128 kNYzx = _mm_shuffle_ps(kN, kN, 201);
        const __m128 kCross2 = _mm_sub_ps(_mm_mul_ps(kNZxy, kPlane2Yzx), _mm_mul_ps(kNYzx, kPlane2Zxy));
        const __m128 kCross1 = _mm_sub_ps(_mm_mul_ps(kNYzx, kPlane1Zxy), _mm_mul_ps(kNZxy, kPlane1Yzx));
        const __m128 kVelocity2 = a2->m_velocity.m_quad;
        const __m128 kD2 = _mm_mul_ps(kVelocity2, kPlane2);
        const float fDot2 =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kD2, kD2, 170), _mm_add_ss(_mm_shuffle_ps(kD2, kD2, 85), kD2)));
        const __m128 kVelocity1 = a1->m_velocity.m_quad;
        const __m128 kD1 = _mm_mul_ps(kVelocity1, kPlane1);
        const __m128 kVn = _mm_mul_ps(_mm_add_ps(kVelocity1, kVelocity2), kN);
        const float fDot1 =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kD1, kD1, 170), _mm_add_ss(_mm_shuffle_ps(kD1, kD1, 85), kD1)));
        const float fVn =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kVn, kVn, 170), _mm_add_ss(_mm_shuffle_ps(kVn, kVn, 85), kVn)));
        const float fHalfVn = fVn * 0.5;
        const __m128 kRhs = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(fHalfVn), _mm_set_ss(fDot2)),
            _mm_unpacklo_ps(_mm_set_ss(fDot1), _mm_set_ss(zero))
        );
        const __m128 kSum = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 0), kCross), _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 85), kCross2)
        );
        const __m128 kPoint =
            _mm_mul_ps(_mm_add_ps(kSum, _mm_mul_ps(_mm_shuffle_ps(kRhs, kRhs, 170), kCross1)), kInvLen);
        const hkpSimplexSolverInput* pInput = r_edi->m_input;
        const __m128 kTooFast =
            _mm_cmplt_ps(pInput->m_maxSurfaceVelocity.m_quad, _mm_and_ps(kPoint, hkVector4_setAbs4_absMask));
        if (!(_mm_movemask_ps(kTooFast) & 7)) {
            const __m128 kRel = _mm_sub_ps(a3->m_quad, kPoint);
            const __m128 kRr = _mm_mul_ps(kRel, kRel);
            const float fRelLen2 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kRr, kRr, 170), _mm_add_ss(_mm_shuffle_ps(kRr, kRr, 85), kRr)));
            const __m128 kUn = _mm_mul_ps(pInput->m_upVector.m_quad, kN);
            const float fUpDot =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kUn, kUn, 170), _mm_add_ss(_mm_shuffle_ps(kUn, kUn, 85), kUn)));
            const __m128 kRn = _mm_mul_ps(kRel, kN);
            const float fRelN =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kRn, kRn, 170), _mm_add_ss(_mm_shuffle_ps(kRn, kRn, 85), kRn)));
            const float fStaticSum = a1->m_staticFriction + a2->m_staticFriction;
            // Uses the down frictions unless the product is greater than zero.
            const double dExtra = fRelN * fUpDot > 0.0 ? a1->m_extraUpStaticFriction + a2->m_extraUpStaticFriction
                                                       : a1->m_extraDownStaticFriction + a2->m_extraDownStaticFriction;
            const float fMu = dExtra * fUpDot + fStaticSum;
            const float fMuHalf = fMu * 0.5;
            const float fDynamic = (a1->m_dynamicFriction + a2->m_dynamicFriction) * 0.5;
            const float fRelN2 = fRelN * fRelN;
            const float fMuHalf2 = fMuHalf * fMuHalf;
            // A smaller or unordered product goes on to the friction.
            if ((fRelLen2 - fRelN2) * fMuHalf2 >= fRelN2) {
                a4->m_quad = kPoint;
                return;
            }
            float fScale = fRelN;
            if (fDynamic < 1.0f && 0.0010000000474974513 * fRelLen2 < fRelN * fRelN) {
                const float fInvRelN = 1.0 / fRelN;
                const float fAbsInvRelN = std::fabs(fInvRelN);
                const float fRoot = std::sqrt(fRelLen2);
                const float fRootRatio = fRoot * fAbsInvRelN;
                const float fFactor = fRootRatio * (1.0 - fDynamic) + fDynamic;
                fScale = fFactor * fRelN;
            }
            a4->m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fScale), kN), kPoint);
            return;
        }
    }
    // Each constraint's index is the truncating shift by 6 of its byte distance from the constraints array (the 64-byte
    // element), with the array reloaded for each store, not a pointer difference's exact division.
    r_edi->m_output
        ->m_planeInteractions
            [(reinterpret_cast<const char*>(a1) - reinterpret_cast<const char*>(r_edi->m_input->m_constraints)) >> 6]
        .m_status = hkpSurfaceConstraintInteraction::HK_STATUS_2D_FAILURE;
    r_edi->m_output
        ->m_planeInteractions
            [(reinterpret_cast<const char*>(a2) - reinterpret_cast<const char*>(r_edi->m_input->m_constraints)) >> 6]
        .m_status = hkpSurfaceConstraintInteraction::HK_STATUS_2D_FAILURE;
    if (a1->m_priority <= a2->m_priority) {
        solveOneConstraint(a1, r_edi, a3, a4);
        solveOneConstraint(a2, r_edi, a4, a4);
    }
    else {
        solveOneConstraint(a2, r_edi, a3, a4);
        solveOneConstraint(a1, r_edi, a4, a4);
    }
}

void sub_D24650(int32_t a1) {
    ;
}
