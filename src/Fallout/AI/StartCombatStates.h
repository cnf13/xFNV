#pragma once

#include <cstdint>

class Actor;
class CombatGroup;

// The target, group and flags used when an actor starts combat.
class StartCombatStates {
public:
    StartCombatStates();

    Actor* pTarget;
    CombatGroup* pGroup;
    bool bMustEntercombat;
    bool bSendAlarm;
    bool bForceFlee;
    bool bFleeing;
    bool bIgnoreDefensivePackage;
    bool bSpectatorOfCombat;
    uint8_t padding0E[2];
    int32_t iPriority;
};
