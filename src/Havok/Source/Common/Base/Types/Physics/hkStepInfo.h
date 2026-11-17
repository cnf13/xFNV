#pragma once

// The time span of one simulation step.
struct hkStepInfo {
    float m_startTime;
    float m_endTime;
    float m_deltaTime;
    float m_invDeltaTime;
};
