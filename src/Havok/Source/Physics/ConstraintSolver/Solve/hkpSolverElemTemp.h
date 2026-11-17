#pragma once

#include "Reconstruction.hpp"

// The solver's per-element scratch value: the impulse applied so far, the impulse factor used or the right-hand-side
// delta, depending on the solver stage.
class hkpSolverElemTemp {
public:
	union {
		float m_impulseApplied;
		float m_usedImpulseFactor;
		float m_deltaRhs;
	};
};
