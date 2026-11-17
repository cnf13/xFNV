#pragma once

#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Reconstruction.hpp"

// A trap's tracked zone, whose update runs the trap-specific target processing.
class TrapEntry : public ZoneEntry {
public:
	TrapEntry(TESObjectREFR* apRef, const hkpCollidable* apCol);

	void Run() override;
};
inline constexpr fnvd::Vtable TrapEntry_vftable{};
