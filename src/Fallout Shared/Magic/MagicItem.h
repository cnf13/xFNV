#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"
#include "Reconstruction.hpp"

// A named list of magic effects: spells, enchantments, alchemy and ingredients. No virtual destructor in the exe.
class MagicItem : public TESFullName, public EffectItemList {
public:
	// Why a caster cannot cast an item (MagicCaster::CheckCast's reason, GetCannotCastString's message).
	enum CannotCastReason : int32_t {
		NOCAST_OK = 0,
		NOCAST_MAGICKA = 1,
		NOCAST_SILENCED = 2,
		NOCAST_SKILLCHECK = 3,
		NOCAST_POWERUSED = 4,
		NOCAST_MULTIPLEBOUNDEFFECTS = 5,
		NOCAST_RANGEDUNDERWATER = 6,
	};

	MagicItem();
	~MagicItem();

	virtual bool IsAutoCalc() = 0;
	virtual void SetAutoCalc(bool abValue) = 0;
	virtual MagicSystem::SpellType GetSpellType() = 0;
	virtual bool IgnoreResistance() { return false; }
	virtual bool GetNoAbsorbOrReflect() { return false; }

protected:
	virtual CHUNK_ID GetChunkID() = 0;
	virtual void* GetData() = 0;
	virtual uint32_t GetDataSize() = 0;
	virtual bool CompareMagicItemData(MagicItem* apCompare) = 0;
	virtual void CopyMagicItemData(MagicItem* apCopy) = 0;
	virtual void SaveMagicItemComponents() {}
	virtual void SaveMagicItemData() = 0;
	virtual void LoadMagicItemChunk(TESFile*, CHUNK_ID) {}

public:
	// Loads the models and sounds of the item's effects, now with abNow.
	void Preload(bool abNow);
	void Unload(bool abRelease);
	// Whether the item's models and sounds are loaded (thiscall, ret 0, result al, 0x40B470).
	bool IsLoaded();
	// Whether the item is still loading (thiscall, ret 0, result al, 0x40B6B0).
	bool IsLoading();
	// Finishes the item once its effects' models are loaded.
	void FinishedLoading();
	uint32_t GetMagicItemFormID();
	// The effect setting of the item's costliest effect of range aeRange.
	EffectSetting* GetAVEffect(MagicSystem::Range aeRange);
	// Fills apString with the message for aeReason; the exe's hidden result pointer is the first stack argument and
	// comes back in eax (thiscall, ret 8, 0x40BEF0).
	BSStringT<char>* GetCannotCastString(BSStringT<char>* apString, CannotCastReason aeReason);

protected:
	// Resolves the form IDs of the item's effects and counts the hostile ones.
	void MagicItemInitItem(TESForm* apForm);

public:
	// Whether the item's models and sounds are loaded or queued (thiscall, ret 0, result al, 0x40B580).
	bool IsLoadingOrLoaded();

	static void DecSummonedCreatureLoadedCount();
	// The magic item with the save game's form ID aiFormID.
	static MagicItem* GetMagicItemByNumericID(uint32_t aiFormID);
	static int32_t IncSummonedCreatureLoadedCount();

	static inline int32_t iSummonedCreatureLoadedCount;
};
inline constexpr fnvd::Vtable MagicItem_vftable{};
inline constexpr fnvd::Vtable MagicItem_vftable_EffectItemList{};

// GameSettingCollection: 0xf3607d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastSilenced;

// GameSettingCollection: 0xf360dd calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastMultipleBoundEffects;

// GameSettingCollection: 0xf35fed calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastOKText;

// GameSettingCollection: 0xf3604d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastInsufficientSkill;

// GameSettingCollection: 0xf360ad calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastPowerUsed;

// GameSettingCollection: 0xf3601d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastInsufficientMagicka;

// GameSettingCollection: 0xf3610d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sMagicCastRangedUnderwater;
