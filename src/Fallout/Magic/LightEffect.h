#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class NiPointLight;

// The light spell effect: it keeps a point light on its target while it lasts.
class LightEffect : public ActiveEffect {
public:
    LightEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    ~LightEffect() override;

    ActiveEffect* Copy() override;
    void Update(float afSecondsElapsed) override;
    void FinishInitLoadGame(Actor* apActor) override;
    void Revert(BGSLoadFormBuffer*) override;
    void Revert(Actor*) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void CopyData(ActiveEffect* apDestination) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    void Finish() override;

    // Detaches the point light from the target's 3D.
    void DetachLight();
    void AttachLight();

    // Attaches the brightest light effect on apTarget to its 3D.
    static void AttachBestLightEffect(MagicTarget* apTarget);
    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    static LightEffect* Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

    NiPointer<NiPointLight> spPointLight;

    static inline int32_t iLightCount;
};
