#pragma once

#include "Fallout/Magic/MagicProjectile.h"
#include "Reconstruction.hpp"

class EffectSetting;
class MagicCaster;
class MagicItem;

// The spray spell projectile (PDB names). It adds no members src uses.
class MagicSprayProjectile : public MagicProjectile {
public:
	MagicSprayProjectile(MagicCaster* a1, MagicItem* a2, EffectItem* a3, EffectSetting* a4, NiPoint3 a5, NiPoint3 a6);

	// Does nothing.
	void Update(float afDelta) override;
};
inline constexpr fnvd::Vtable MagicSprayProjectile_vftable_TESChildCell{};
inline constexpr fnvd::Vtable MagicSprayProjectile_vftable{};
