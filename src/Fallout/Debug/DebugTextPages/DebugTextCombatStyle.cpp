#include "Fallout Shared/TESCombatStyle.h"

bool TESCombatStyle::IsFlagSet(TESCombatStyle::COMBAT_STYLE_FLAGS aeFlag) {
	return (aeFlag & Data.iFlags) != 0;
}
