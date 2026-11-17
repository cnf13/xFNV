#pragma once

#include "BSAudio/BSSoundHandle.h"

class Actor;

// The near and far sound handles of one weapon's fire sound, with the actor that fired it
// and when.
class WeaponNearFarSound {
public:
    WeaponNearFarSound();

    BSSoundHandle soundNear;
    BSSoundHandle soundFar;
    Actor* uiIdentifier;
    uint32_t uiTimestamp;
    float fRelativeDistance;
};
