#pragma once

#include "Fallout Shared/ExtraDataList.h"
#include "Fallout/Magic/MagicCaster.h"

class MagicTarget;
class TESObjectREFR;

// The extra data that lets a reference that is not an actor cast spells (traps, activators).
class NonActorMagicCaster : public BSExtraData, public MagicCaster {
public:
    NonActorMagicCaster();
    NonActorMagicCaster(TESObjectREFR* apCasterObject);
    ~NonActorMagicCaster() override;

    TESObjectREFR* GetCasterStatsObject() override {
        return pCasterObject;
    }
    MagicItem* GetCurrentSpell() override {
        return pCurrentSpell;
    }

protected:
    void SetCurrentSpell(MagicItem* apCurrentSpell) override {
        pCurrentSpell = apCurrentSpell;
    }
    MagicTarget* GetDesiredTarget() override {
        return pDesiredTarget;
    }
    void SetDesiredTarget(MagicTarget* apDesiredTarget) override {
        pDesiredTarget = apDesiredTarget;
    }

public:
    void CleanUpOtherMobileObjects();

    MagicItem* pCurrentSpell;
    MagicTarget* pDesiredTarget;
    TESObjectREFR* pCasterObject;
};
