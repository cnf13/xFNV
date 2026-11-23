#include "Fallout/Magic/MagicSprayProjectile.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

MagicSprayProjectile::MagicSprayProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apEffectItem,
    EffectSetting* apAVEffect,
    NiPoint3 aLocation,
    NiPoint3 aRotation
)
  : MagicProjectile(apCaster, apSpell, apEffectItem, apAVEffect, aLocation, aRotation) {
    MobileObject::InitHavok();
}

void MagicSprayProjectile::Update(float afDelta) {}
