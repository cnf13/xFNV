#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Reconstruction.hpp"

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
	// folded with TESMain::GetThread at 0x44EDB0
	TESObjectREFR* GetTargetStatsObject() override { return pTargetObject; }
	// folded with hkpCollidable::getBroadPhaseHandle at 0x7D6BB0
	BSSimpleList<ActiveEffect*>* GetActiveEffectList() override { return &lActiveEffectList; }

	TESObjectREFR* pTargetObject;
	BSSimpleList<ActiveEffect*> lActiveEffectList;
};
inline constexpr fnvd::Vtable NonActorMagicTarget_vftable{};
inline constexpr fnvd::Vtable NonActorMagicTarget_vftable_MagicTarget{};
