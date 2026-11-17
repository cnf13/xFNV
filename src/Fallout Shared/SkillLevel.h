#pragma once

#include "Reconstruction.hpp"

template <class T> void ENDIAN(T& arValue, bool abUnused);

// The skill mastery levels and their thresholds (PDB: Fallout Shared\SkillLevel.cpp; static members only).
// MagicFormulas::GetMagicSpellLevel (0x6491D0) returns a Value.
class SkillLevel {
public:
	// PDB values.
	enum Value : int32_t {
		NOVICE = 0,
		APPRENTICE = 1,
		JOURNEYMAN = 2,
		EXPERT = 3,
		MASTER = 4,
		SKILL_LEVEL_COUNT = 5,
	};
};
