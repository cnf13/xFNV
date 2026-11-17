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
    uint32_t v2;
    TESValueForm* v4 = dynamic_cast<TESValueForm*>(apCopyFC);
    if (v4) {
        v2 = v4->TESValueForm::GetFormValue();
        SetFormValue(v2);
    }
}

bool TESValueForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESValueForm* v5 = dynamic_cast<TESValueForm*>(apCompareFC);
    if (!v5) {
        return (bool)1;
    }
    uint32_t v3 = TESValueForm::GetFormValue();
    return (bool)((uint8_t)(v3 != v5->TESValueForm::GetFormValue()));
}

uint16_t TESValueForm::GetSaveSize(uint32_t aiFlags) {
    uint16_t v3 = 0;
    if ((aiFlags & 2) != 0) {
        return (uint16_t)4;
    }
    return v3;
}

void TESValueForm::SaveGame(uint32_t aiFlags) {
    if ((((int8_t)aiFlags) & 2) != 0) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iValue, 4);
    }
}

void TESValueForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if ((((int8_t)aiFlags) & 2) != 0) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iValue, 4);
    }
}

void TESValueForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(2)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iValue), 4, 0);
    }
}

void TESValueForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(2)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iValue), 4);
    }
}

void TESValueForm::SetFormValue(int32_t aiValue) {
    iValue = aiValue;
    TESForm* pForm = dynamic_cast<TESForm*>(this);
    if (pForm) {
        pForm->AddChange(2);
    }
}

// The gold value of apForm: its own value, a magic item's cost or an alchemy item's cost, else -1. Each cost is a
// virtual call on the item's EffectItemList subobject with a null actor, truncated by _ftol2_sse.
int32_t TESValueForm::GetFormValue(TESForm* apForm) {
    TESValueForm* pValueForm = dynamic_cast<TESValueForm*>(apForm);
    if (pValueForm) {
        return static_cast<int32_t>(pValueForm->GetFormValue());
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
