#include "Fallout/Magic/NonActorMagicTarget.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"

NonActorMagicTarget::NonActorMagicTarget() : BSExtraData(0x33u), MagicTarget(), pTargetObject(nullptr) {}

NonActorMagicTarget::NonActorMagicTarget(TESObjectREFR* apTargetObject)
  : BSExtraData(0x33u), MagicTarget(), pTargetObject(apTargetObject) {}

NonActorMagicTarget::~NonActorMagicTarget() {}

bool NonActorMagicTarget::AddTarget(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    ActiveEffect* apActiveEffect,
    bool abAreaTarget
) {
    bool v7 = MagicTarget::AddTarget(apCaster, apSpell, apActiveEffect, abAreaTarget);
    if (v7) {
        UpdateTarget(1.0);
    }
    return v7;
}
