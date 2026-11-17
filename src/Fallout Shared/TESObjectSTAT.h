#pragma once

#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"

class TESSound;

// A static base form (STAT).
class TESObjectSTAT : public TESBoundObject, public TESModelTextureSwap {
public:
	TESObjectSTAT();
	~TESObjectSTAT() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	char cPassThroughSoundIndex;
	TESSound* pRandomSound;
};
