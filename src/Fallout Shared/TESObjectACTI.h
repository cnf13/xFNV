#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Reconstruction.hpp"

class BGSTalkingActivator;
class TESSound;
class TESWaterForm;

// An activator base form (ACTI).
class TESObjectACTI : public TESBoundAnimObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESScriptableForm,
					  public TESMagicTargetForm,
					  public BGSDestructibleObjectForm,
					  public BGSOpenCloseForm {
public:
	TESObjectACTI();
	~TESObjectACTI() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	// folded with CombatController::GetCombatGroup at 0x4FB070
	TESWaterForm* GetWaterType() override { return pWaterForm; }

	using TESForm::LoadGame;
	using TESForm::Save;
	using TESForm::SaveGame;

	void SetRadioTemplate(TESSound* apSound);
	// The RNAM setter (pRadioStation, +0x84); IDA names 0x4FB090 SetUnk084 (thiscall, ret 4).
	void SetRadioStation(BGSTalkingActivator* apRadio);
	void SetLoopingSound(TESSound* apSound);
	void SetActivateSound(TESSound* apSound);
	void SetActivationString(BSStringT<char> ActivateTextOverrideInput);
	void SetWaterType(TESWaterForm* pWat);
	// The looping sound (pSoundLoop); the exe's shared +0x74 getter (thiscall, ret 0,
	// 0x4FD3A0).
	TESSound* GetLoopingSound();
	// The radio station (pRadioStation); the exe's shared +0x84 getter (thiscall, ret 0,
	// 0x4FD3C0).
	BGSTalkingActivator* GetRadioStation();
	void SetContinuousBroadcast(bool abVal);
	bool GetNonPipboyRadio();
	// A copy of ActivateTextOverride, returned by value through apResult.
	BSStringT<char>* GetActivationString(BSStringT<char>* apResult);

	TESSound* pSoundLoop;
	TESSound* pSoundActivate;
	TESSound* pRadioTemplate;
	TESWaterForm* pWaterForm;
	BGSTalkingActivator* pRadioStation;
	BSStringT<char> ActivateTextOverride;
};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_BGSOpenCloseForm{};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_TESBoundAnimObject{};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectACTI_vftable_TESScriptableForm{};
