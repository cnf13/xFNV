#include "Fallout Shared/BGSEquipType.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTexture.h"
#include "SDK/MSVC/rtti.h"

BGSEquipType::BGSEquipType() : BaseFormComponent() {
    BGSEquipType::InitializeDataComponent();
}

void BGSEquipType::InitializeDataComponent() {
    eEquipType = ET_BIG_GUN;
}

void BGSEquipType::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSEquipType* pCopy = dynamic_cast<BGSEquipType*>(apCopyFC);
    if (pCopy) {
        eEquipType = pCopy->eEquipType;
    }
}

bool BGSEquipType::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSEquipType* pCompare = dynamic_cast<BGSEquipType*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    return eEquipType != pCompare->eEquipType;
}

void BGSEquipType::Save() {
    TESForm::AddChunk(ETYP_ID, static_cast<uint32_t>(eEquipType));
}

void BGSEquipType::GenerateDefaultMenuIcons() {
    for (int32_t i = 0; i < 14; ++i) {
        uint32_t uiID = i + 600;
        TESForm* pForm = TESForm::GetFormByNumericID(i + 600);
        BGSMenuIcon* pIcon = dynamic_cast<BGSMenuIcon*>(pForm);
        if (!pIcon) {
            char cBuffer[264];
            pIcon = new BGSMenuIcon;
            pIcon->SetFormID(uiID, true);
            BSstrcpy(cBuffer, 260, "MenuIconEquipType");
            BSstrcat(cBuffer, 260, BGSEquipType::pEquipTypeNames[i]);
            pIcon->SetFormEditorID(cBuffer);
            BSstrcpy(cBuffer, 260, "TypeIcons\\hot_keys_icon_");
            BSstrcat(cBuffer, 260, BGSEquipType::pEquipTypeNames[i]);
            BSstrcat(cBuffer, 260, ".dds");
            pIcon->SetTextureName(cBuffer);
            pDataHandler->AddFormToDataHandler(pIcon);
        }
        BGSEquipType::pEquipTypeMenuIcons[i] = pIcon;
    }
}

BGSEquipType::EQUIP_TYPE BGSEquipType::GetEquipType(TESForm* apForm) {
    BGSEquipType::EQUIP_TYPE eRetType = BGSEquipType::ET_NONE;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case ARMO_ID: eRetType = static_cast<BGSEquipType*>(static_cast<TESObjectARMO*>(apForm))->eEquipType; break;
            case CLOT_ID: eRetType = static_cast<BGSEquipType*>(static_cast<TESObjectCLOT*>(apForm))->eEquipType; break;
            case INGR_ID:
                eRetType = static_cast<BGSEquipType*>(static_cast<IngredientItem*>(apForm))->eEquipType;
                break;
            case WEAP_ID: eRetType = static_cast<BGSEquipType*>(static_cast<TESObjectWEAP*>(apForm))->eEquipType; break;
            case ALCH_ID: eRetType = static_cast<BGSEquipType*>(static_cast<AlchemyItem*>(apForm))->eEquipType; break;
            default: break;
        }
    }
    return eRetType;
}

BGSMenuIcon* BGSEquipType::GetEquipTypeIcon(BGSEquipType::EQUIP_TYPE aeType) {
    BGSMenuIcon* pIcon = nullptr;
    if (aeType > ET_NONE && aeType < ET_COUNT) {
        pIcon = BGSEquipType::pEquipTypeMenuIcons[aeType];
    }
    return pIcon;
}

const char* BGSEquipType::GetEquipTypeIconFilename(EQUIP_TYPE aeType) {
    BGSMenuIcon* pIcon = GetEquipTypeIcon(aeType);
    if (pIcon) {
        return pIcon->GetTextureName();
    }
    return "";
}
