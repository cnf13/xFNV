#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/MagicSystem.h"

class EnchantmentItem;

// The enchantment a form carries, with its charge and casting type.
class TESEnchantableForm : public BaseFormComponent {
public:
    TESEnchantableForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    virtual MagicSystem::CastingType GetCastingType() {
        return eCastingType;
    }

    // Resolves the enchantment's form ID for apForm.
    void InitItem(TESForm* apOwner);
    // Stores eCastingType.
    void SetCastingType(MagicSystem::CastingType aeCastingType) {
        eCastingType = aeCastingType;
    }
    // Writes the enchantment's EITM chunk and the amount's EAMT chunk.
    void SaveSubRecord();
    // Stores iAmountofEnchantment from an ANAM/EAMT chunk.
    void SetFormEnchantmentAmount(uint16_t aiAmount);
    EnchantmentItem* GetFormEnchanting();
    // Stores pFormEnchanting.
    void SetFormEnchanting(EnchantmentItem* apEnch);

    // The enchantment of apForm when it is enchantable, else null.
    static EnchantmentItem* GetFormEnchanting(TESForm* apForm);

    EnchantmentItem* pFormEnchanting;
    uint16_t iAmountofEnchantment;
    MagicSystem::CastingType eCastingType;
};
