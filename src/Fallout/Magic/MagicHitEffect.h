#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"

class ActiveEffect;
class NiAVObject;
class TESObjectREFR;

// The temp effect a spell's active effect plays on its target while it lasts.
class MagicHitEffect : public BSTempEffect {
public:
	MagicHitEffect();
	MagicHitEffect(TESObjectREFR* a1, ActiveEffect* a2);
	~MagicHitEffect() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool Update(float afTimeDelta) override;
	TEMP_EFFECT_TYPE GetType() override { return MAGIC_HIT_EFFECT_DEFAULT; }
	virtual bool Init() { return false; }
	virtual void ClearTarget() { pTarget = nullptr; }
	virtual void UpdatePosition() {}
	virtual uint16_t GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget);
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) override;
	virtual void SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	virtual void LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget);
	void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	virtual void InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget);
	virtual void FinishInitLoadGame(ActiveEffect*, TESObjectREFR*, void*) {}
	void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	using BSTempEffect::GetSaveSize;
	using BSTempEffect::LoadGame;
	using BSTempEffect::SaveGame;

	void UpdateParentCell(NiAVObject* apRoot);

	static BSSimpleList<MagicHitEffect*>* InstantiateHitEffects(ActiveEffect* apEffect);

	ActiveEffect* pEffect;
	TESObjectREFR* pTarget;
	float fAge;
	bool bFinished;

	static NiRTTI ms_RTTI;
};
