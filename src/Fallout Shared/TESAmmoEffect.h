#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"

// What an ammo effect changes.
enum AMMO_EFFECT_TYPE : uint32_t {
	AMMO_EFFECT_DAM = 0,
	AMMO_EFFECT_DR = 1,
	AMMO_EFFECT_DT = 2,
	AMMO_EFFECT_SPREAD = 3,
	AMMO_EFFECT_CONDITION = 4,
	AMMO_EFFECT_FATIGUE = 5,
	AMMO_EFFECT_TYPE_COUNT = 6,
};

// How an ammo effect's value combines with the weapon's.
enum AMMO_OPERATION_TYPE : uint32_t {
	AMMO_OPERATION_PLUS = 0,
	AMMO_OPERATION_TIMES = 1,
	AMMO_OPERATION_MINUS = 2,
	AMMO_OPERATION_COUNT = 3,
};

// An ammo effect's DATA chunk.
struct AMMO_EFFECT_DATA {
	// Byte-swaps the three dwords.
	void Endian();

	AMMO_EFFECT_TYPE iAmmoEffectType;
	AMMO_OPERATION_TYPE iOperation;
	float fValue;
};

// An ammo effect form (AMEF).
class TESAmmoEffect : public TESForm, public TESFullName {
public:
	TESAmmoEffect();
	~TESAmmoEffect() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	void BuildMenuString(char* astrResult, uint32_t auiResultLen);

	// Returns afOriginalValue changed by every effect of aiEffectType in pEffectList.
	static float ApplyAmmoEffect(AMMO_EFFECT_TYPE aiEffectType, BSSimpleList<TESAmmoEffect*>* pEffectList,
		float afOriginalValue);

	AMMO_EFFECT_DATA data;
};

extern SettingT<GameSettingCollection> sAmmoEffectSpread;
extern SettingT<GameSettingCollection> sAmmoEffectDR;
extern SettingT<GameSettingCollection> sAmmoEffectDAM;
extern SettingT<GameSettingCollection> sAmmoEffectDT;
extern SettingT<GameSettingCollection> sAmmoEffectCondition;
extern SettingT<GameSettingCollection> sAmmoEffectFatigue;
