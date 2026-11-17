#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class ActiveEffect;
class ParticleShaderProperty;
class TESBoundObject;
class TESEffectShader;
class TESObjectREFR;

// A shader-driven hit effect a spell plays on its target.
class MagicShaderHitEffect : public MagicHitEffect {
public:
	MagicShaderHitEffect();
	MagicShaderHitEffect(TESObjectREFR* a1, TESEffectShader* a2, float a3);
	MagicShaderHitEffect(TESObjectREFR* a1, ActiveEffect* a2);
	~MagicShaderHitEffect() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool Update(float afTimeDelta) override;
	TEMP_EFFECT_TYPE GetType() override { return MAGIC_HIT_EFFECT_SHADER; }
	bool CheckShouldSave() override;
	uint16_t GetSaveSize() override;
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) override;
	void SaveGame() override;
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	bool LoadGame() override;
	void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	bool Init() override;
	void ClearTarget() override;
	void UpdatePosition() override;
	uint16_t GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget) override;
	void SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) override;
	void LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) override;
	void InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) override;
	void FinishInitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget, void* apData) override;

	void SetLifeTime(float a2);
	void SetFinished(bool a1);
	void GetAffectedScenegraph(bool& abActorRoot, bool& abSword, float& afSwordLength, NiAVObject*& apMembraneRoot,
		NiAVObject*& apParticleRoot);
	void ResetAlphaTimer();

	bool bWeaponEnchantment;
	TESBoundObject* pWornObject;
	TESEffectShader* pEffectData;
	float fAlphaTimer;
	BSSimpleArray<NiPointer<ParticleShaderProperty>> ParticlePropertyArray;
	NiPointer<NiNode> spParticleNode;
	bool b3rdPerson;
	BSSimpleArray<NiPointer<NiAVObject>> AddOnObjects;
	float fAddonAlpha;
	float fAddonScale;
	NiPointer<BSShaderPPLightingProperty::TextureEffectData> spTexEffectData;

	static NiRTTI ms_RTTI;
};
