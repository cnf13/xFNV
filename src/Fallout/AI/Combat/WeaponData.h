#pragma once

#include "BSCore/BSSimpleArray.h"

#include <stdint.h>

class TESObjectWEAP;

// What the combat AI knows about one weapon of an actor: its condition, damage per second and range.
class WeaponData {
public:
    TESObjectWEAP* pWeapon;
    float fCondition;
    float fDamagePerSecond;
    float fAbsoluteMaxRange;
    bool bInRange;
    uint8_t padding11[3];
};
