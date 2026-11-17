#pragma once

#include <cstdint>

class TESObjectREFR;

// A door a combat group searches through: it, the door it leads to and how far the search has got.
struct CombatSearchDoor {
    TESObjectREFR* pDoor;
    TESObjectREFR* pLinkedDoor;
    uint8_t cFailedAttempts;
    bool bInvestigated;
    bool bReserved;
    uint8_t padding0B;
};
