#pragma once

#include "Reconstruction.hpp"

// An empty base: references of a base form deriving from it can be the target of magic.
class TESMagicTargetForm {
public:
	static bool CanBeMagicTarget(TESForm* apForm);
};
