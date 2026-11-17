#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESProduceForm.h"
#include "Reconstruction.hpp"

// A flora form (FLOR): an activator that yields an ingredient. Its own members are not declared yet.
class TESFlora : public TESProduceForm, public TESObjectACTI {
public:
	TESFlora();
	~TESFlora() override;

	// the TESForm slots: the exe's bodies take the TESObjectACTI part and adjust it to the flora themselves
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjectToGet, int32_t aiCount) override;
	using TESForm::Save;
};
inline constexpr fnvd::Vtable TESFlora_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESFlora_vftable_BGSOpenCloseForm{};
inline constexpr fnvd::Vtable TESFlora_vftable_TESBoundAnimObject{};
inline constexpr fnvd::Vtable TESFlora_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESFlora_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESFlora_vftable_TESProduceForm{};
inline constexpr fnvd::Vtable TESFlora_vftable_TESScriptableForm{};

// GameSettingCollection: 0xf459ed calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sFloraSuccessMessage;

// GameSettingCollection: 0xf45a1d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sFloraFailureMessage;
