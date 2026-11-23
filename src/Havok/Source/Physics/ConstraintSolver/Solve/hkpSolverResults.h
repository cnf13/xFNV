#pragma once

// What the solver reports back for one constraint row: the impulse it applied, and the solver's own data.
class hkpSolverResults {
public:
    hkpSolverResults() {
        m_impulseApplied = 0.0f;
        m_internalSolverData = 0.0f;
    }

    float m_impulseApplied;
    float m_internalSolverData;
};
