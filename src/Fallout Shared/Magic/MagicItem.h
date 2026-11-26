#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"

// A named list of magic effects: spells, enchantments, alchemy and ingredients. It has no virtual destructor.
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
    virtual bool IgnoreResistance() {
        return false;
    }
    virtual bool GetNoAbsorbOrReflect() {
        return false;
    }

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
    // Whether the item's models and sounds are loaded.
    bool IsLoaded();
    bool IsLoading();
    // Finishes the item once its effects' models are loaded.
    void FinishedLoading();
    uint32_t GetMagicItemFormID();
    // The effect setting of the item's costliest effect of range aeRange.
    EffectSetting* GetAVEffect(MagicSystem::Range aeRange);
    // Fills apString with the message for aeReason.
    BSStringT<char> GetCannotCastString(CannotCastReason aeReason);

protected:
    // Resolves the form IDs of the item's effects and counts the hostile ones.
    void MagicItemInitItem(TESForm* apForm);

public:
    // Whether the item's models and sounds are loaded or queued.
    bool IsLoadingOrLoaded();

    static void DecSummonedCreatureLoadedCount();
    // The magic item with the save game's form ID aiFormID.
    static MagicItem* GetMagicItemByNumericID(uint32_t aiFormID);
    static int32_t IncSummonedCreatureLoadedCount();

    static inline int32_t iSummonedCreatureLoadedCount;
};

extern SettingT<GameSettingCollection> sMagicCastSilenced;

extern SettingT<GameSettingCollection> sMagicCastMultipleBoundEffects;

extern SettingT<GameSettingCollection> sMagicCastOKText;

extern SettingT<GameSettingCollection> sMagicCastInsufficientSkill;

extern SettingT<GameSettingCollection> sMagicCastPowerUsed;

extern SettingT<GameSettingCollection> sMagicCastInsufficientMagicka;

extern SettingT<GameSettingCollection> sMagicCastRangedUnderwater;
