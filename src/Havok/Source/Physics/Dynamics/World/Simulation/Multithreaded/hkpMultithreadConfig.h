#pragma once

#include <cstdint>

// The settings of the multithreaded simulation.
struct hkpMultithreadConfig {
    hkpMultithreadConfig();

    int32_t m_maxNumConstraintsSolvedSingleThreaded;
};
