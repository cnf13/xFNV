#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

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
    void Update(float) override;
    void FinishInitLoadGame(Actor*) override;
    bool CheckDeletedCaster(MagicCaster*) override;
    void CopyData(ActiveEffect*) override;
    void Start() override;
    void Finish() override;

    void Start3D();
    void Update3D(float afDelta);

    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    static AbsorbEffect* Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

    NiPointer<NiNode> spBoltNode;
    NiPointer<NiNode> spSource;
    NiPointer<NiNode> spTarget;
    NiPointer<BSShaderProperty> spBoltProperty;
};
