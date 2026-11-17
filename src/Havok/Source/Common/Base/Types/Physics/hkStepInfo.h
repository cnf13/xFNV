#pragma once

#include "Reconstruction.hpp"

// The time span of one simulation step.
struct hkStepInfo {
	float m_startTime;
	float m_endTime;
	float m_deltaTime;
	float m_invDeltaTime;
};
