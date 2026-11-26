#pragma once

#include <cstdint>

// A texture's degrade state, as returned by NiTexture::QState.
enum PRIORITY_STATE : int32_t {
    PS_NONE = 0,
    PS_DEGRADING = 1,
    PS_UPGRADING = 2,
    PS_DEGRADED = 3,
    PS_UPGRADED = 4,
};

// Bethesda's texture degrade priority: the low, high and current priority and the degrade state.
class BSDegradePriority {
public:
    // Returns cState.
    PRIORITY_STATE QState();

    uint8_t cPriorityL;
    uint8_t cPriorityH;
    uint8_t cPriorityC;
    // A PRIORITY_STATE, stored as a byte.
    uint8_t cState;
};

inline PRIORITY_STATE BSDegradePriority::QState() {
    return static_cast<PRIORITY_STATE>(cState);
}
