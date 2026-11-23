#include "Fallout Shared/TESValueForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

TESValueForm::TESValueForm() {
    TESValueForm::InitializeDataComponent();
}

void TESValueForm::InitializeDataComponent() {
    iValue = 0;
}

void TESValueForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESValueForm* pCopy = dynamic_cast<TESValueForm*>(apCopyFC);
    if (pCopy) {
        uint32_t uiValue = pCopy->TESValueForm::GetFormValue();
        SetFormValue(uiValue);
    }
}

bool TESValueForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESValueForm* pCompare = dynamic_cast<TESValueForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    uint32_t uiValue = TESValueForm::GetFormValue();
    return uiValue != pCompare->TESValueForm::GetFormValue();
}

uint16_t TESValueForm::GetSaveSize(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_BASE_OBJECT_VALUE) != 0) {
        return 4;
    }
    return 0;
}

void TESValueForm::SaveGame(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_BASE_OBJECT_VALUE) != 0) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iValue, sizeof(iValue));
    }
}

void TESValueForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if ((aiFlags & CHANGE_BASE_OBJECT_VALUE) != 0) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iValue, sizeof(iValue));
    }
}

void TESValueForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BASE_OBJECT_VALUE)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iValue), sizeof(iValue), 0);
    }
}

void TESValueForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BASE_OBJECT_VALUE)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iValue), sizeof(iValue));
    }
}

void TESValueForm::SetFormValue(int32_t aiValue) {
    iValue = aiValue;
    TESForm* pForm = dynamic_cast<TESForm*>(this);
    if (pForm) {
        pForm->AddChange(CHANGE_BASE_OBJECT_VALUE);
    }
}

// The gold value of apForm: its own value, a magic item's cost or an alchemy item's cost, else -1. Each cost is a
// virtual call on the item's EffectItemList subobject with a null actor, truncated by _ftol2_sse.
int32_t TESValueForm::GetFormValue(TESForm* apForm) {
    TESValueForm* pValueForm = dynamic_cast<TESValueForm*>(apForm);
    if (pValueForm) {
        return pValueForm->GetFormValue();
    }
    MagicItem* pMagicItem = dynamic_cast<MagicItem*>(apForm);
    if (pMagicItem) {
        EffectItemList* pEffects = static_cast<EffectItemList*>(pMagicItem);
        return static_cast<int32_t>(static_cast<int64_t>(pEffects->GetCost(nullptr)));
    }
    AlchemyItem* pAlchemyItem = dynamic_cast<AlchemyItem*>(apForm);
    if (pAlchemyItem) {
        EffectItemList* pEffects = static_cast<EffectItemList*>(pAlchemyItem);
        return static_cast<int32_t>(static_cast<int64_t>(pEffects->GetCost(nullptr)));
    }
    return -1;
}
