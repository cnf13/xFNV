#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESProduceForm.h"

// A flora form (FLOR): an activator that yields an ingredient. Its own members are not declared yet.
class TESFlora : public TESProduceForm, public TESObjectACTI {
public:
	TESFlora();
	~TESFlora() override;

	// The TESForm overrides: these bodies take the TESObjectACTI part and adjust it to the flora themselves.
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjectToGet, int32_t aiCount) override;
	using TESForm::Save;
};

extern SettingT<GameSettingCollection> gs_sFloraSuccessMessage;

extern SettingT<GameSettingCollection> gs_sFloraFailureMessage;
