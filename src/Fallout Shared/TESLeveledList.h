#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class ContainerItemExtra;
class TESContainer;
class TESForm;
class TESGlobal;
class TESObjectREFR;

// The next unsigned random number of the BSRandom singleton (cdecl, 0x487F50; a KEEP body, not registered).
uint32_t Random();

// One entry of a leveled list: the form, how many and from which level.
struct LEVELED_OBJECT {
	LEVELED_OBJECT();
	~LEVELED_OBJECT();

	// The exe's deleting destructor (0x481760) frees through ourFree.
	static void operator delete(void* apMemory);

	TESForm* pForm;
	uint16_t sCount;
	uint16_t sLevel;
	ContainerItemExtra* pItemExtra;
};

// The entries of a leveled list form and how it picks among them.
class TESLeveledList : public BaseFormComponent {
public:
	TESLeveledList();
	~TESLeveledList();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The chance of picking nothing, from the chance global when the list has one.
	virtual uint8_t GetChanceNone();
	// Whether every pick up to the level count is made (the list's "calculate for each item" flag).
	virtual bool GetMultCalc();
	virtual int32_t GetMaxLevelDifference() { return 0; }

	// Reverts the list from the load game buffer.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer);
	// Writes the list's chunks into the record being saved: the chance, the flags, the global and every entry.
	void Save();
	void SetChanceNone(uint8_t acChance);
	// Whether the list uses the calculate-all-below flag.
	bool GetCalcAllBelow();
	// Whether all entries at or below the selected level are used.
	bool GetUseAll();
	BSSimpleList<LEVELED_OBJECT*>* GetLeveledList();
	// Inserts a new entry for apForm after the entries at or below asLevel; returns it.
	LEVELED_OBJECT* AddLeveledObject(uint16_t asLevel, uint16_t asCount, TESForm* apForm, ContainerItemExtra* apExtra);
	// Picks one entry for level asLevel, recursing into nested leveled lists when abRecurse is set.
	void CalculateCurrentForm(uint16_t asLevel, TESForm*& apOutForm, uint16_t& asOutCount,
		ContainerItemExtra*& apOutExtra, bool abRecurse, int32_t aiAllBelowForce);
	// Adds asCount picks for level asLevel to apOutCont.
	void CalculateCurrentFormList(uint16_t asLevel, uint16_t asCount, TESContainer* apOutCont, int32_t aiAllBelowForce);
	// Adds one pick for apRef's level, scaled by its leveled creature multiplier, to apOutCont.
	void CalculateCurrentFormListForRef(TESObjectREFR* apRef, TESContainer* apOutCont);
	void InitItem(TESForm* apOwner);
	void FreeLeveledList();
	// Saves the list's script-added entries.
	void SaveGame(BGSSaveFormBuffer* apSaveFormBuffer);
	// Loads the list's script-added entries.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Removes every entry scripts added to the list.
	void RemoveAllScriptAddedLeveledObjects();
	void AddScriptAddedLeveledObject(TESForm* apOwner, uint16_t asLevel, uint16_t asCount, TESForm* apForm,
		ContainerItemExtra* apExtra);
	void SetCalcAllBelow(bool abAllBelow);
	void SetMultCalc(bool abVal);
	void SetUseAll(bool abVal);

	// The leveled list base of a leveled creature, character, item or spell form, else null.
	static TESLeveledList* GetFormAsLeveledList(TESForm* apForm);

	BSSimpleList<LEVELED_OBJECT*> leveledList;
	uint8_t cChanceNone;
	uint8_t cLLFlags;
	uint8_t padding0E[2];
	TESGlobal* pChanceGlobal;
	BSSimpleList<LEVELED_OBJECT*> ScriptAddedList;
};
inline constexpr fnvd::Vtable TESLeveledList_vftable{};
