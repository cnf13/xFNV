#pragma once

// What the solver reports back for one constraint row: the impulse it applied, and the solver's own data.
class hkpSolverResults {
public:
    float m_impulseApplied;
    float m_internalSolverData;
};
