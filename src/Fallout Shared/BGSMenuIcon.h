#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"

// A menu icon form (MICN).
class BGSMenuIcon : public TESForm, public TESIcon {
public:
	BGSMenuIcon();
	~BGSMenuIcon() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
