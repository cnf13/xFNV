#pragma once

#include "Fallout Shared/TESObjectMISC.h"

// A key base form: a misc item with the same members (both allocate 0xAC bytes).
class TESKey : public TESObjectMISC {
public:
	TESKey();
	~TESKey() override;
};
