#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"

class SpellItem;
class TESSound;

// A potion's ENIT data: the cost override, the flags, the addiction and its chance, and the consumption sound.
struct AlchemyItemData {
    // Byte-swaps the cost, the addiction, its chance and the sound read from a big-endian file.
    void Endian();

    int32_t iCostOverride;
    // Endian leaves this field alone (a byte of flags).
    uint8_t iFlags;
    SpellItem* pAddictionItem;
    float fAddictionChance;
    TESSound* pConsumptionSound;
};

// The base form of potions, food and drink.
class AlchemyItem : public MagicItemObject,
                    public TESModelTextureSwap,
                    public TESIcon,
                    public BGSMessageIcon,
                    public TESScriptableForm,
                    public TESWeightForm,
                    public BGSEquipType,
                    public BGSDestructibleObjectForm,
                    public BGSPickupPutdownSounds {
public:
    enum AlchemyFlag : int32_t {
        COST_OVERRIDE = 0x1,
        FOOD_ITEM = 0x2,
        MEDICINE = 0x4,
    };

    AlchemyItem();
    ~AlchemyItem() override;

    void InitializeData() override;
    void ClearData() override;
    void InitItem() override;
    bool IsAutoCalc() override;
    void SetAutoCalc(bool abValue) override;
    MagicSystem::SpellType GetSpellType() override {
        return MagicSystem::POTION;
    }
    bool IsMedicine() override {
        return AlchemyItem::IsFlagSet(MEDICINE);
    }
    bool IsFood() override;
    float GetCost(Actor* apActor) override;

protected:
    void Endian() override;
    CHUNK_ID GetChunkID() override;
    void* GetData() override {
        return &data;
    }
    uint32_t GetDataSize() override;
    bool CompareMagicItemData(MagicItem* apCompare) override;
    void CopyMagicItemData(MagicItem* apCopy) override;
    void SaveMagicItemComponents() override;
    void SaveMagicItemData() override;
    void LoadMagicItemChunk(TESFile* apFile, CHUNK_ID apChunkID) override;

public:
    // Whether any of aeFlag is set in the data flags.
    bool IsFlagSet(AlchemyFlag aiFlag);
    void SetFlag(AlchemyFlag aiFlag, bool abValue);
    int32_t GetCostOverride() {
        return data.iCostOverride;
    }
    SpellItem* GetAddictionItem();
    void SetAddictionItem(SpellItem* apObject);
    TESSound* GetConsumptionSound();
    void SetConsumptionSound(TESSound* apObject);
    bool IsWater();

    // The bases' operator deletes are ambiguous: the same free (ourFree) as theirs.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    AlchemyItemData data;
    // The icon the message shown when it is used carries.
    TESIcon MessageIcon;
};
