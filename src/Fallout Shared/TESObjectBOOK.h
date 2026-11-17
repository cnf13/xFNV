#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"

class TESObjectREFR;

// The book's data: its flags and the skill it teaches.
struct OBJ_BOOK {
	uint8_t cFlags;
	uint8_t eTeaches;
};

// A book base form. Only some of its members are declared.
class TESObjectBOOK : public TESBoundObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESIcon,
					  public TESScriptableForm,
					  public TESEnchantableForm,
					  public TESValueForm,
					  public TESWeightForm,
					  public TESDescription,
					  public BGSDestructibleObjectForm,
					  public BGSMessageIcon,
					  public BGSPickupPutdownSounds {
public:
	using TESDescription::GetDescription;

	TESObjectBOOK();
	~TESObjectBOOK() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjectToGet, int32_t aiCount) override;
	using TESForm::Save;

	bool GetCantTake();
	bool IsScroll();
	// The book's text: TESDescription::GetDescription(nullptr, DESC_ID) through the TESDescription base. Its body calls
	// the base's two-argument virtual by the same name, so the using keeps that overload visible through a book.
	const char* GetDescription();
	ActorValue::Index GetTeaches();
	bool Read(TESObjectREFR* apActionRef);

	OBJ_BOOK Data;
};

// The game settings TESObjectBOOK.cpp defines.
extern SettingT<GameSettingCollection> fBookPerkBonus;
