#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Reconstruction.hpp"

// The chameleon effect: its actor value modifier makes the target blend into its surroundings. Only the members the
// ported code needs are declared.
class ChameleonEffect : public ValueModifierEffect {
public:
	ChameleonEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~ChameleonEffect() override;

	ActiveEffect* Copy() override;
	// ActiveEffect slots 11, 13, 12, 16, 17, 19, 20 and 22.
	void FinishInitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void CopyData(ActiveEffect* apDestination) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

protected:
	void Update(float afSecondsElapsed) override;

public:
	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
inline constexpr fnvd::Vtable ChameleonEffect_vftable{};
