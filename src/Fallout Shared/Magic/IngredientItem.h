#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"

// An ingredient's DATA chunk: the cost override and the flags.
struct IngredientItemData {
    // Byte swaps the cost override.
    void Endian();

    int32_t iCostOverride;
    uint8_t iFlags;
    uint8_t padding05[3];
};

// The base form of alchemy ingredients.
class IngredientItem : public MagicItemObject,
                       public TESModelTextureSwap,
                       public TESIcon,
                       public TESScriptableForm,
                       public TESWeightForm,
                       public BGSEquipType,
                       public BGSDestructibleObjectForm {
public:
    // The bits of the ingredient's flag byte.
    enum IngredientFlag : int32_t {
        COST_OVERRIDE = 0x1,
        FOOD_ITEM = 0x2,
    };

    IngredientItem();
    ~IngredientItem() override;

    void InitializeData() override;
    void ClearData() override;
    void InitItem() override;
    bool IsAutoCalc() override;
    void SetAutoCalc(bool abValue) override;
    MagicSystem::SpellType GetSpellType() override {
        return MagicSystem::WORTCRAFT;
    }
    bool IsFood() override {
        return IngredientItem::IsFlagSet(FOOD_ITEM);
    }
    float GetCost(Actor* apActor) override;
    int32_t GetMaxEffectCount() override;

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
    // Whether any bit of aeFlag is set in the ingredient's flag byte.
    bool IsFlagSet(IngredientFlag aiFlag);
    // Sets (a3) or clears (!a3) the bits of a2 in the ingredient's flag byte.
    void SetFlag(char a2, char a3);

    // The bases' operator deletes are ambiguous: the same free (ourFree) as theirs.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    IngredientItemData data;
};
