#pragma once

#include "BSCore/BSCoreDefines.h"

#include <cstdint>

// The skill mastery levels and their thresholds (static members only). MagicFormulas::GetMagicSpellLevel returns a
// Value.
class SkillLevel {
public:
    enum Value : int32_t {
        NOVICE = 0,
        APPRENTICE = 1,
        JOURNEYMAN = 2,
        EXPERT = 3,
        MASTER = 4,
        SKILL_LEVEL_COUNT = 5,
    };
};
