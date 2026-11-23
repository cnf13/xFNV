#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout/Magic/MagicTarget.h"

class ActiveEffect;
class MagicCaster;
class MagicItem;
class TESObjectREFR;

// The extra data that lets a reference that is not an actor be the target of spells.
class NonActorMagicTarget : public BSExtraData, public MagicTarget {
public:
    NonActorMagicTarget();
    NonActorMagicTarget(TESObjectREFR* apTargetObject);
    ~NonActorMagicTarget() override;

    bool AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect, bool abAreaTarget) override;
    TESObjectREFR* GetTargetStatsObject() override {
        return pTargetObject;
    }
    BSSimpleList<ActiveEffect*>* GetActiveEffectList() override {
        return &lActiveEffectList;
    }

    TESObjectREFR* pTargetObject;
    BSSimpleList<ActiveEffect*> lActiveEffectList;
};
