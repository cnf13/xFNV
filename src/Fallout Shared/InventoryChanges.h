#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/PackageLocation.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class ActorValueOwner;
class AlchemyItem;
class BGSListForm;
enum BIPED_OBJECT : int32_t;
enum COMBAT_WEAPON_TYPE : int32_t;
class ContainerObject;
class ExtraDataList;
class ExtraLeveledItem;
class InventoryChanges;
class InventoryIterator;
class Script;
class ScriptLocals;
class TESActorBase;
class TESAmmo;
class TESBoundObject;
class TESContainer;
class TESLevItem;
class TESObjectREFR;
class TESObjectWEAP;

// One object of a reference's inventory changes: the extra data of its instances, the count change and the object.
class ItemChange {
public:
	ItemChange();
	ItemChange(TESBoundObject* apObject, int32_t aiNumber);
	ItemChange(const ItemChange* apCopy);
	~ItemChange();

	// The weapon mod flags of the first instance (one bit per mod slot), or 0.
	uint8_t GetModSlots();
	// Deletes the extra data lists of every instance.
	void DeleteAllExtra();
	// The owner set on the item's first extra data list that has one.
	TESForm* GetItemOwnership();
	TESBoundObject* GetContainerObject();
	BSSimpleList<ExtraDataList*>* GetExtraDataList();
	// Whether an instance is worn (in the left hand only, with ableft).
	bool GetWorn(bool ableft);
	void SetWorn(bool abflag, bool ableft, bool abdelete);
	// Resolves the form IDs of every instance's extra data after a load.
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Saves the object, the count change and every instance's extra data, with the instance count in front.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Whether an instance's extra data marks it as a leveled item.
	bool HasLeveledExtra();
	// Whether an instance's extra data names an owner other than the default.
	bool HasStolenData();
	// The poison on the first instance, or null.
	AlchemyItem* GetPoison();
	// Poisons the first instance with apPoison unless it is already poisoned.
	void SetPoison(AlchemyItem* apPoison);
	// Removes the poison from the first instance.
	void RemovePoison();
	// The enchantment charge of the change's instances.
	float GetItemCharge();
	// iNumber. The exe calls it out of line.
	int32_t GetCount() { return iNumber; }
	// Sets iNumber; the exe's shared one-field setter 0x6ECD40 (thiscall, ret 4).
	void SetCount(int32_t ainumber);
	// pContainerObj. The exe calls it out of line.
	TESBoundObject* GetItemChangeObject() { return pContainerObj; }
	// The hotkey of the instance, or negative when it has none.
	int32_t GetHotKey();
	// The item's value, scaled by its condition (thiscall, ret 0, ST0 float, 0x4BD400).
	float GetItemValue();
	// The summed counts of the instances (worn ones skipped with abWornIgnore).
	int32_t GetExtraTotalCount(bool abWornIgnore);
	// The summed counts of the instances whose extra data is a container default; empty
	// instances are removed and freed on the way.
	int32_t GetExtraTotalDefaultCount();
	// The script of the first instance that has one, or null.
	Script* GetScript();
	// The script locals of the first instance that has them, or null.
	ScriptLocals* GetScriptLocals();
	// The object's icon file name for apRef, or null.
	const char* GetIconOne(TESObjectREFR* apRef);
	// The object's full name, else the string setting at 0x11D35B8 (thiscall, ret 0,
	// 0x4BE2D0).
	const char* GetFullName();
	// True when the weapon mod effect eEffect is on the instance.
	bool HasModEffectActive(uint8_t eEffect);
	// True when the weapon mod effect eEffect is on the instance; fValue receives the effect's
	// value.
	bool HasModEffectActive(uint8_t eEffect, float& fValue);
	// The instance's health, as a percentage of the maximum with abPercentage (thiscall, ret 4, 0x4BCDB0). Returned
	// in ST0; one path loads the integer max health with fild (exact at 80 bits), so the return is a double.
	double GetItemHealth(bool abPercentage);
	// The weapon's attack damage for apActor scaled by afDamageMultiplier, explosion damage
	// left out with abIgnoreExplosion (thiscall, ret 0xC, 0x4BDF00; returned in ST0).
	float GetModifiedAttackDamage(ActorValueOwner* apActor, float afDamageMultiplier, bool abIgnoreExplosion);
	// The armor's damage threshold scaled by the instance's condition, rounded up; -1 for anything but armor.
	// apActor is unused.
	float GetModifiedDamageThreshold(ActorValueOwner* apActor);
	// The PDB's LoadGame(apLoadGameBuffer) of the old save format: the exe passes no buffer and reads through
	// pSaveLoadGameOLD (thiscall, ret 0, 0x4BE930).
	void LoadGame();
	// Reads the item change from a save buffer.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	void SetItemOwnership(TESForm* apowner);
	// IDA name; the PDB's SaveGame of the old save format (thiscall, ret 0, 0x4BE6F0). The exe's eax at return is
	// whatever it last computed; no caller reads it.
	void SaveGameTES();
	void Copy(ItemChange* apCopy);
	int32_t GetAmountNonDefaultExtra();
	// Hex-Rays gave it a return; the exe's eax at return is whatever it last computed and no caller reads it.
	void RemoveOriginalChanges();
	// Inferred type: return: int32_t: signed uses from 0x4d289e
	int32_t CountLeveledItems();
	ExtraLeveledItem* RemoveLeveledItemExtra();
	void SetItemHealth(float afHealth, InventoryChanges* pchange, ExtraDataList* apextra, bool abdelete);
	void SetIsModding();
	// Hex-Rays gave it a return; the exe's eax at return is whatever it last computed and no caller reads it.
	void RemoveIsModding();
	// The armour rating of the item at its condition, ceiled. apActor is unused; its only
	// caller, Character::GetArmorRating, passes the actor's ActorValueOwner as for
	// GetModifiedDamageThreshold.
	float GetConditionScaledDR(ActorValueOwner* apActor);
	int32_t GetTypeforMenu();
	uint16_t GetSaveSize();
	void InitLoadGame_0();

	static void operator delete(void* apMemory);

	BSSimpleList<ExtraDataList*>* pExtraObjectList;
	int32_t iNumber;
	TESBoundObject* pContainerObj;
};

// The changes made to a reference's inventory relative to its base container, plus cached carry weights.
class InventoryChanges {
public:
	// State shared by the inventory enumeration routines.
	class IterData {
	public:
		~IterData();

		static void operator delete(void* apMemory);

		// IDA name; not in the PDB. The container's filtered list: GetIterator stores TESContainer::GetFilteredList's
		// result here (0x4CA200: 0x4ca2a9 call, 0x4ca2b1 mov dword ptr [ecx], eax).
		BSSimpleList<ContainerObject*>* pFilteredList;
		// IDA name; not in the PDB. The node of pFilteredList the walk is at: GetIterator starts it at the head
		// (0x4CA200: 0x4ca2db mov ecx, [eax]; 0x4ca2dd mov dword ptr [edx+4], ecx) and Iterate advances it with the
		// node's next (0x4CA330: 0x4caaa0 call 0x726070; 0x4caaa8 store).
		BSSimpleList<ContainerObject*>* pItemList;
		// IDA name; not in the PDB. Only ever initialised to 0 (0x4CA200: 0x4ca2b6 mov dword ptr [edx+8], 0); no
		// body reads it and the PDB has no member here, so its type is its 4-byte size.
		uint32_t dword8;
		// IDA name; not in the PDB. The change Iterate returns, or null (0x4CA330: cleared at 0x4ca37f, set at
		// 0x4ca7c6).
		ItemChange* pCurrentItem;
		// IDA name; not in the PDB. The original container Iterate asks for object counts (0x4CA330: 0x4ca42c
		// mov ecx, [edx+0x10]; 0x4ca42f call TESContainer::GetObjectCount).
		TESContainer* pContainer;
		// Not named in the PDB or IDA; name chosen here. The node of the inventory's change list the walk is at:
		// Iterate starts it at InventoryChanges::pListofChanges (0x4CA330: 0x4caacc mov eax, [edx]; 0x4caace
		// mov dword ptr [ecx+0x14], eax) and advances it (0x4caf8d, 0x4caf9b).
		BSSimpleList<ItemChange*>* pChangeList;
		// Not named in the PDB or IDA; name chosen here. 0 while the walk is over the container's objects, 1 once it
		// is over the inventory's changes (0x4CA330: 0x4caabc mov dword ptr [eax+0x18], 1; compared at 0x4ca389 and
		// 0x4caad4).
		uint32_t uiPhase;
		// IDA name; not in the PDB. The node of the change's extra data list the walk is at (0x4CA330: set at
		// 0x4ca6e6, advanced at 0x4ca7e8).
		BSSimpleList<ExtraDataList*>* pExtraLists;
		// Not named in the PDB or IDA; name chosen here. The node of the change's extra data list the second phase
		// totals the non-default counts with (0x4CA330: set at 0x4cac47, advanced at 0x4cacb0).
		BSSimpleList<ExtraDataList*>* pCountExtraLists;
		// Not named in the PDB or IDA; name chosen here. The node of the change's extra data list the second phase
		// adds an item per non-default instance with (0x4CA330: set at 0x4cad11, advanced at 0x4cadfc).
		BSSimpleList<ExtraDataList*>* pAddExtraLists;
	};

	// The PDB's callback type of IterateInventory: the item's change and the caller's data.
	using IterateInventoryCallback = bool (*)(ItemChange*, void*);

	InventoryChanges(TESObjectREFR* apRef);
	~InventoryChanges();

	bool ContainerHasObjects(TESBoundObject* apObj, PACKAGE_OBJECT_TYPE aeObjectType, int32_t ainumber,
		PACKAGE_OBJECT_TYPE& aeMatchId, uint32_t aId, TESObjectREFR* apRef);
	void RemoveGold(TESObjectREFR* apRef, int32_t aiamount, TESObjectREFR* apCont);
	int32_t GetGoldAmount();
	int32_t GetCount(bool abcountplayable);
	bool WearingVoiceModulateItem();
	// Gives the change pChange an extra data list holding aiNumber of apObject for apActor
	// (apExtra copied when given).
	void CreateExtraDataList(TESBoundObject* apObject, int32_t aiNumber, Actor* apActor, ExtraDataList* apExtra,
		ItemChange* pChange);
	// Calls apCallback on each item with apData; aiIterateFlags is a bit mask (0x4D45B7 tests
	// the dword's bit 0); returns the count of items it called apCallback on (thiscall, ret 0xC, 0x4D4530).
	uint32_t IterateInventory(IterateInventoryCallback apCallback, void* apData, uint32_t aiIterateFlags);
	// The total value of the items; both flags are bytes.
	uint32_t GetInventoryValue(bool abCountQuest, bool abCountGold);
	// Adds to apReturnList the entries of the inventory whose object is apObj.
	void GetItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList);
	// The first object of form type aeFormID in the inventory, or null; *ainumb receives its
	// count.
	TESBoundObject* GetObjectbyType(ENUM_FORM_ID aeFormID, int32_t& ainumb);
	// Resolves the inventory's form IDs after a TES-format load.
	void InitLoadGameTES();
	// Writes the inventory's changes into the old save.
	void SaveGame();
	// Reads the inventory's changes from the old save.
	void LoadGame();
	// Saves the inventory's changes into apSaveGameBuffer with no current form and flags
	// 0x400, then restores both.
	// Inferred type: apSaveGameBuffer: BGSSaveFormBuffer* from 0x562300 (parameter 0 of TESObjectREFR::SaveGame), its
	// members used at 0x4d4090
	void SaveGameBGS(BGSSaveFormBuffer* apSaveGameBuffer);

	bool IsEmpty();
	// The bytes the changes take in a save.
	uint16_t GetSaveSize();
	// A new change for the item on hotkey aiKey, or null; the caller deletes it.
	ItemChange* GetHotKeyItem(int32_t aiKey);
	// A new array of the changes on the hotkeys aiFirst through aiLast, null per hotkey
	// without one; a change whose instances hold no hotkey in range is freed.
	// Inferred type: aiFirst: int32_t: signed uses from 0x4bf66f; aiLast: int32_t: signed uses from 0x4bf66f
	ItemChange** GetHotKeyItem(int32_t aiFirst, int32_t aiLast);
	// IDA name; not in the PDB. A new iterator over the changes, which the caller frees with
	// IterData::DeletingDestruct (thiscall, ret 0, 0x4CA200; it allocates the 0x28-byte IterData).
	IterData* GetIterator();
	// IDA name; not in the PDB. The next change of the iterator, or null at the end (thiscall, ret 4, 0x4CA330; it
	// reads the IterData fields up to +0x24).
	ItemChange* Iterate(IterData* apIterator);
	// True when the inventory holds the object with form ID aId, or an instance whose
	// reference has that ID.
	bool HasObjectId(uint32_t aId);

	int32_t GetObjectCount(TESBoundObject* apObject);
	bool HasQuestObject();
	bool HasScript();
	ItemChange* GetInventoryItem(TESBoundObject* apObject, uint32_t aId);
	ExtraDataList* WearingObject(TESBoundObject* apObj, bool bshouldwear);
	// the first change of the object; with aId set, the one whose instance references the form aId
	ItemChange* GetObjectInList(TESBoundObject* apObj, bool ableveled, uint32_t aId);
	// a new change holding the first worn instance that fills the body part, or null
	ItemChange* GetWornItem(BIPED_OBJECT aeBodyPart, bool bOnlyArmor);
	// unequips everything the actor wears
	void RemoveAllObjectsWorn(bool abSkipCanNotWear, TESObjectREFR* apRef);
	// the first ammo of the weapon's ammo list the inventory holds, else its ammo when held
	TESAmmo* GetBestAmmoForWeapon(TESObjectWEAP* pWeapon, bool& bNeedsAmmo);
	// Steps arIterator to the next change, stored in arpItemChange; false at the end.
	bool GetNextItem(InventoryIterator& arIterator, ItemChange*& arpItemChange, uint32_t aiIterateFlags);
	// Unequips the object's worn instances.
	bool RemoveWorn(bool* abRemoved, TESBoundObject* apObject, int32_t aiNumber, Actor* apActor, ExtraDataList* apExtra,
		bool ableft, bool abforce, ItemChange* apItemChange, bool* apbExtraDeleted);
	// equips aiNumber of the object on the actor, splitting the instance off a stack when needed
	void AddWorn(TESBoundObject* apObject, int32_t aiNumber, Actor* apActor, ExtraDataList* apExtra, bool ableft,
		bool abForceEquip);
	// adds ainumber of the object with the instance's extra data (a new list when null), merging equal instances
	void AddItem(TESBoundObject* apObject, ExtraDataList* pextra, int32_t ainumber);
	// merges the change into the change of the same object, else appends it; abDelete frees a merged change
	void AddChangeItem(ItemChange* apChange, bool abDelete);
	// a new change for the ammo the actor base wants most: base container ammo still held, then added ammo
	ItemChange* GetBestAmmo(TESActorBase* apActorBase, bool abUseCanNotWear);
	// a new change for the weapon of the combat type (COMBAT_WEAPON_TYPE_COUNT: any) with the highest combat DPS;
	// afvalue gets that score through CombatFormulas::ChooseMeleeWeapon
	ItemChange* GetBestWeapon(TESActorBase* apActorBase, float& afvalue, COMBAT_WEAPON_TYPE aeType,
		bool abUseCanNotWear);
	// a new change for the armor filling the body slot with the highest condition-scaled rating plus threshold;
	// with abUseCanNotWear, the worn item there when it may not be unequipped
	ItemChange* GetBestArmor(TESActorBase* apActorBase, BIPED_OBJECT aeBodySlot, bool abUseCanNotWear);
	// the first food (alchemy or ingredient) of the base container still held, else of the added items
	ItemChange* GetBestFood();

	void ClearAllChangeItems();
	float GetInventoryWeight(bool abIsHardcore);
	float RemoveAllItems(TESObjectREFR* apRef, TESObjectREFR* apStolenContainer, bool abStealing, bool abowner,
		bool abRemoveOwnership, bool abSilent, int32_t aiFormType, BGSListForm* apDoNotRemoveList);
	void RemoveItemType(ENUM_FORM_ID aeType, bool abStealing, int32_t ainumber);
	// Removes ainumber of apObject (the instance apExtra) for apRef, dropping it into the
	// world or moving it to apOtherContainer; the PDB's TESObjectREFR* result. The PDB's eleven parameters and one
	// more (thiscall, ret 0x30, 0x4C37D0): apItemChange, the object's change when the caller has it, else looked up
	// with GetObjectInList(apObject, true, 0).
	// Inferred type: apItemChange ([ebp+0x34]) stands in for GetObjectInList's ItemChange* result (0x4C38BA vs
	// 0x4C38AD) and is ItemChange::GetWorn's this (0x4C3E2F).
	TESObjectREFR* RemoveItem(TESObjectREFR* apRef, TESBoundObject* apObject, bool abstealing, int32_t ainumber,
		ExtraDataList* apExtra, bool abDropWorld, TESObjectREFR* apOtherContainer, const NiPoint3* apPoint,
		const NiPoint3* apRotate, bool abdelete, bool abPreferStolen, ItemChange* apItemChange);
	TESContainer* GetOriginalContainer();
	// forgets the cached container weight and, for actors, tells the reference to drop its cached carry weight
	void ResetCachedWeights();
	void ForceWeightRecalculation();

	// whether an instance of the changes carries the leveled item index aiIndex
	bool IsLevelItemInited(int32_t aiIndex);
	// rolls each leveled item of the base container not rolled yet and adds the result to the changes
	void InitLeveledItems();
	// turns the base container entries with per-entry extra data into changes
	void InitFromContainerExtra();
	// resolves every change's form IDs after a load
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// drops the leveled item instances from the changes, and the changes that end up empty
	void RemoveDefaultLeveledItems();
	// gives every scripted item, of the base container and of the changes, an instance with its script and
	// locals, and runs the script once on a temporary reference
	void InitScripts();
	// runs the scripts of the instances on pTempRef placed at pOwner; true when one ran
	bool RunScripts(TESObjectREFR* pOwner);
	// Empty in this build: the engine's shared empty stub.
	void CleanUpBoundItems(Actor* apActor);
	// pRef. The exe calls it out of line.
	TESObjectREFR* GetRef() { return pRef; }
	// Removes the items stolen from apOwner held in apStolenContainer from the inventory of
	// apRef.
	void RemoveStolenItems(TESObjectREFR* apRef, TESObjectREFR* apStolenContainer, TESForm* apOwner);
	// Inferred type: a1: BGSLoadFormBuffer* from 0x5624f4 (parameter 0 of TESObjectREFR::LoadGame), its members used at
	// 0x4d418b
	void LoadGame_ObjectsList(BGSLoadFormBuffer* a1);
	ItemChange* GetItembyIndex(int32_t aiIndex);
	void GetItemModGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList);
	void RemoveHotKeyItem(ItemChange* apItem, int32_t a3);
	void SetHotKeyItem(ItemChange* apItem, ExtraDataList* apExtra, int32_t aiKey);
	void AddItem(TESObjectREFR* apObject, int32_t ainumber, bool abAddRefId, bool abWearit);
	TESBoundObject* GetObjectbyPackObjType(PACKAGE_OBJECT_TYPE aeObjectType, int32_t& ainumb);
	// Inferred type: a1: TESObjectREFR* from 0x4c4732 (parameter 0 of InventoryChanges::RemoveItem), its members used
	// at 0x4c6e09; a2: TESBoundObject* from 0x4c502d (parameter 1 of InventoryChanges::RemoveItem); a3: int32_t: signed
	// uses from 0x4c6f26; a5: NiPoint3* from 0x4c4732 (parameter 7 of InventoryChanges::RemoveItem), its members used
	// at 0x4c6dea
	TESObjectREFR* DropItemIntoWorld(TESObjectREFR* a1, TESBoundObject* a2, int32_t a3, TESObjectREFR* a4,
		const NiPoint3* a5, const NiPoint3* a6);
	TESBoundObject* GetLeveledCalculatedBaseObject(TESLevItem* apLevelItem);
	void CleanUpBadData();
	void DuplicateAllItems(TESObjectREFR* apRef, TESObjectREFR* apDupContainer);
	float RemoveAllTypedItems(TESObjectREFR* apRef, TESObjectREFR* apStolenContainer, bool abStealing, bool abowner,
		bool abRemoveOwnership, bool abSilent, int32_t aiFormType, BGSListForm* apDoNotRemoveList);
	float GetEquippedWeight(Actor* apOwner);
	void CreateCopyofList(InventoryChanges* apCopy, TESObjectREFR* apRef, bool bdontcopyquestitems);
	void GetRepairItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList);
	int32_t GetExtraDataListIndexForItem(TESBoundObject* apObject, ExtraDataList* apExtra);
	ExtraDataList* GetExtraDataListFromIndexForItem(TESBoundObject* apObject, int32_t aiExtraIndex);

	// the reference's changes, created on demand for containers and actors; original call
	static InventoryChanges* GetInventoryChanges(TESObjectREFR* apRef);
	static TESObjectREFR* GetTempRef();
	static void operator delete(void* apMemory);
	static void KillTempRef();

	BSSimpleList<ItemChange*>* pListofChanges;
	TESObjectREFR* pRef;
	float fcontainerweight;
	float fpreviousContainerWeight;
	bool bcountdirty;
	uint8_t padding11[3];

	// the reference the instance scripts run on
	static inline TESObjectREFR* pTempRef;
	static BSSpinLock kRunScriptsLock;
	// RemoveWorn stores the ExtraDataList it merged a removed one into.
	static inline ExtraDataList* pRestackedList;
};

// The game settings InventoryChanges.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fPackRatThreshold;
extern SettingT<GameSettingCollection> fPackRatModifier;
