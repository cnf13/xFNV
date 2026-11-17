#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESObjectACTI.h"

class BGSNote;
class BGSTerminal;
class TESObjectREFR;

// One entry of a terminal's menu: its texts, the result script, the conditions that show it, and the note or
// submenu it opens.
struct TERMINAL_MENU_ITEM {
	TERMINAL_MENU_ITEM();

	// Whether choosing the item adds its note to the player (cFlags bit 1).
	bool IsAutoAdd();
	// Whether choosing the item redraws the menu (cFlags bit 2).
	bool ForcesRedraw();
	// Runs the result script on apActivator with its locals.
	void RunResultScript(TESObjectREFR* apActivator);
	void SetSelected(bool abVal);
	bool IsSelected() const;

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	BSStringT<char> ResponseText;
	BSStringT<char> ResultText;
	Script ResultScript;
	TESCondition Conditions;
	BGSNote* pDisplayNote;
	BGSTerminal* pSubMenu;
	uint8_t cFlags;
};

// A terminal's hacking difficulty, flags and server type.
struct TERMINAL_DATA {
	uint8_t cDifficulty;
	uint8_t cFlags;
	uint8_t cServerType;
	uint8_t cUnused;
};

// A terminal base form (TERM).
class BGSTerminal : public TESObjectACTI, public TESDescription {
public:
	BGSTerminal();
	~BGSTerminal() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIDFlag,
		TESBoundObject* apObjectToGet, int32_t aiCount) override;
	void HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) override;
	bool GetBlockActivatesWhileOpening() override { return true; }
	using TESForm::Revert;
	using TESObjectACTI::LoadGame;
	using TESObjectACTI::Save;
	using TESObjectACTI::SaveGame;

	// The skill required at the reference's adjusted hacking difficulty.
	float GetMinSkillRequiredToHack(TESObjectREFR* apItemActivated);
	// The terminal's difficulty, or the level of apRef's terminal lock state when that is set.
	LOCK_LEVEL GetRawDifficulty(TESObjectREFR* apRef);
	// Removes every entry of the terminal's menu.
	void ClearMenuItemList();
	// The hacking difficulty adjusted by the level of apRef, 4 at most.
	LOCK_LEVEL GetHackDifficultyLockLevel(TESObjectREFR* apRef);
	void SetPassword(BGSNote* apPassword);
	void AddMenuItem(TERMINAL_MENU_ITEM* apItem);
	// The server text setting's string for Data.cServerType, or null past the 10 server types.
	const char* GetServerText();

	BSSimpleList<TERMINAL_MENU_ITEM*>* GetMenuItemList();
	bool IsLeveled();
	bool IsUnlocked();
	// Whether the terminal is unlocked for apRef, whose terminal state overrides the base flag.
	bool IsUnlocked(TESObjectREFR* apRef);
	uint32_t GetHackMiniGameValue(TESObjectREFR* apRef);
	bool IsHackEligible(TESObjectREFR* apItemActivated, float afSkill);
	bool UsesAltColor();
	bool HidesWelcomeTextWhenDisplayingImage();
	void SetRawDifficulty(TESObjectREFR* apRef, LOCK_LEVEL aeVal);
	void SetUnlocked(TESObjectREFR* apRef);

	static bool IsTerminalRefInUse(TESObjectREFR* apRef);
	static void ResetState(TESObjectREFR* apRef);
	static void SetLockedOut(TESObjectREFR* a1, char a2);

	BSSimpleList<TERMINAL_MENU_ITEM*> MenuItemList;
	BGSNote* pPassword;
	TERMINAL_DATA Data;
};

// The server text settings (sTerminalServerText1..10) by server type.
using TerminalServerTextArray = Setting* [10];
extern TerminalServerTextArray s_TerminalServerTexts;

// The game settings BGSTerminal.cpp defines.
extern SettingT<GameSettingCollection> sTerminalServerText6;
extern SettingT<GameSettingCollection> sTerminalServerText2;
extern SettingT<GameSettingCollection> sTerminalServerText7;
extern SettingT<GameSettingCollection> sTerminalServerText10;
extern SettingT<GameSettingCollection> sTerminalServerText9;
extern SettingT<GameSettingCollection> sTerminalServerText3;
extern SettingT<GameSettingCollection> sTerminalServerText8;
extern SettingT<GameSettingCollection> sTerminalServerText5;
extern SettingT<GameSettingCollection> sTerminalLocked;
extern SettingT<GameSettingCollection> sHackIneligible;
extern SettingT<GameSettingCollection> sTerminalServerText1;
extern SettingT<GameSettingCollection> sTerminalServerText4;
extern SettingT<GameSettingCollection> fHackLevelMult;
