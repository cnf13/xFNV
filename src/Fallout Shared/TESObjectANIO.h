#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"

class TESIdleForm;

// An animation object: a model plus the idle that plays on it.
class TESObjectANIO : public TESForm, public TESModelTextureSwap {
public:
	TESObjectANIO();
	~TESObjectANIO() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Whether apIdle is this object's idle.
	bool GetUsesIdle(TESIdleForm* apIdle);

	TESIdleForm* pIdleAnim;
};
