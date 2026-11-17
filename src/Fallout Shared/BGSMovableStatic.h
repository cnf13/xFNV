#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectSTAT.h"

// The movable static's data record (BGSMovableStatic +0x5C).
struct MOVABLE_STATIC_DATA {
	MOVABLE_STATIC_DATA();

	MOVABLE_STATIC_DATA& Copy(MOVABLE_STATIC_DATA& aCopy);
	// Whether the flags differ.
	bool Compare(MOVABLE_STATIC_DATA& aComp);

	uint8_t cFlags;
};

// A movable static form (MSTT): a static with a looping sound and its data flags.
class BGSMovableStatic : public TESFullName, public BGSDestructibleObjectForm, public TESObjectSTAT {
public:
	BGSMovableStatic();
	~BGSMovableStatic() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	TESSound* GetLoopingSound();
	void SetLoopingSound(TESSound* apSound);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	TESSound* pSoundLoop;
	MOVABLE_STATIC_DATA data;
};
