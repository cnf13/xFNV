#pragma once

#include "Fallout/Magic/CommandEffect.h"
#include "Reconstruction.hpp"

// The Command Humanoid magic effect: the CommandEffect that accepts humanoid targets. It adds no fields.
class CommandHumanoidEffect : public CommandEffect {
public:
	CommandHumanoidEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~CommandHumanoidEffect() override;

	ActiveEffect* Copy() override;
	bool CheckTarget(MagicTarget* apTarget) override;

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
inline constexpr fnvd::Vtable CommandHumanoidEffect_vftable{};
