#pragma once

#include <cstdint>

// The world's settings for the collision agents' iterative linear casts.
class hkpCollisionAgentConfig {
public:
    float m_iterativeLinearCastEarlyOutDistance;
    int32_t m_iterativeLinearCastMaxIterations;
    uint8_t padding08[8];
};
