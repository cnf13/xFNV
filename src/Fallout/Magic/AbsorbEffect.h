#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSShaderProperty;
class EffectItem;
class MagicCaster;
class MagicItem;
class NiNode;

class AbsorbEffect : public ValueModifierEffect {
public:
	AbsorbEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~AbsorbEffect() override;

	ActiveEffect* Copy() override;
	// Inferred type: a1: float from 0x80312d (passed to ValueModifierEffect::Update 0)
	void Update(float) override;
	void FinishInitLoadGame(Actor*) override;
	bool CheckDeletedCaster(MagicCaster*) override;
	void CopyData(ActiveEffect*) override;
	void Start() override;
	void Finish() override;

	void Start3D();
	void Update3D(float afDelta);

	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
	static AbsorbEffect* Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

	NiPointer<NiNode> spBoltNode;
	NiPointer<NiNode> spSource;
	NiPointer<NiNode> spTarget;
	NiPointer<BSShaderProperty> spBoltProperty;
};
inline constexpr fnvd::Vtable AbsorbEffect_vftable{};
