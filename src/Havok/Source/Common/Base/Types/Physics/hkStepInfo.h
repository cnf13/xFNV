#pragma once

// The time span of one simulation step.
struct hkStepInfo {
    hkStepInfo() = default;
    hkStepInfo(float startTime, float endTime) {
        set(startTime, endTime);
    }

    // The inverse of a zero delta time is zero.
    void set(float startTime, float endTime) {
        m_startTime = startTime;
        m_endTime = endTime;
        m_deltaTime = endTime - startTime;
        m_invDeltaTime = m_deltaTime == 0.0f ? 0.0f : 1.0f / m_deltaTime;
    }

    float m_startTime;
    float m_endTime;
    float m_deltaTime;
    float m_invDeltaTime;
};
