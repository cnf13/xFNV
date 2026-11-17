#pragma once

#include "Fallout Shared/ExtraDataList.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Reconstruction.hpp"

class MagicTarget;
class TESObjectREFR;

// The extra data that lets a reference that is not an actor cast spells (traps, activators).
class NonActorMagicCaster : public BSExtraData, public MagicCaster {
public:
	NonActorMagicCaster();
	NonActorMagicCaster(TESObjectREFR* apCasterObject);
	~NonActorMagicCaster() override;

	// folded with BSTimer::GetTime at 0x825C00
	TESObjectREFR* GetCasterStatsObject() override { return pCasterObject; }
	// folded with the exe's shared one-field getter at 0x84E3A0
	MagicItem* GetCurrentSpell() override { return pCurrentSpell; }

protected:
	// folded with BSCullingProcess::SetCamera at 0x41FD00
	void SetCurrentSpell(MagicItem* apCurrentSpell) override { pCurrentSpell = apCurrentSpell; }
	// folded with TESMain::GetThread at 0x44EDB0
	MagicTarget* GetDesiredTarget() override { return pDesiredTarget; }
	// folded with ExtraSayToTopicInfo::SetTopic at 0x437730
	void SetDesiredTarget(MagicTarget* apDesiredTarget) override { pDesiredTarget = apDesiredTarget; }

public:
	void CleanUpOtherMobileObjects();

	MagicItem* pCurrentSpell;
	MagicTarget* pDesiredTarget;
	TESObjectREFR* pCasterObject;
};
inline constexpr fnvd::Vtable NonActorMagicCaster_vftable{};
inline constexpr fnvd::Vtable NonActorMagicCaster_vftable_MagicCaster{};
