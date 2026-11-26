#include "Fallout Shared/TESAIForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bFixAIPackagesOnLoad("bFixAIPackagesOnLoad:General", false);
SettingT<GameSettingCollection> fConfidenceFoolhardy("fConfidenceFoolhardy", 0.0f);
SettingT<GameSettingCollection> fConfidenceBrave("fConfidenceBrave", 0.5f);
SettingT<GameSettingCollection> fConfidenceAverage("fConfidenceAverage", 1.0f);
SettingT<GameSettingCollection> fConfidenceCautious("fConfidenceCautious", 2.0f);
SettingT<GameSettingCollection> fConfidenceCowardly("fConfidenceCowardly", 1000.0f);

// Swaps the two 32-bit fields read from a big-endian file; the byte fields are left alone.
void AIDATA::Endian() {
    BSEndian(iServiceFlags, false);
    BSEndian(uiAggroRadius, false);
}

TESAIForm::TESAIForm() {
    TESAIForm::InitializeDataComponent();
}

TESAIForm::~TESAIForm() {
    TESAIForm::ClearDataComponent();
}

void TESAIForm::InitializeDataComponent() {
    AIData.cAggression = 0;
    AIData.cConfidence = 4;
    AIData.cEnergy = 0;
    AIData.cResponsibility = 0;
    AIData.cMood = 0;
    AIData.iServiceFlags = 0;
    AIData.cTrainingSkill = 0;
    AIData.cTrainingLevel = 0;
    AIData.cAssistance = 0;
    AIData.bAggroRadius = 0;
    AIData.uiAggroRadius = 0;
    if (!confidenceSet) {
        fConfidences[0] = fConfidenceCowardly.Float();
        fConfidences[1] = fConfidenceCautious.Float();
        fConfidences[2] = fConfidenceAverage.Float();
        fConfidences[3] = fConfidenceBrave.Float();
        fConfidences[4] = fConfidenceFoolhardy.Float();
        confidenceSet = 1;
    }
}

void TESAIForm::ClearDataComponent() {
    AIPackList.listPackages.RemoveAll();
}

void TESAIForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESAIForm* pAIForm = dynamic_cast<TESAIForm*>(apCopyFC);
    if (pAIForm) {
        CopyAIData(pAIForm);
        CopyAIPackageList(pAIForm);
    }
}

bool TESAIForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESAIForm* pCompare = dynamic_cast<TESAIForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (AIData.cAggression != pCompare->AIData.cAggression) {
        return true;
    }
    int32_t iAssistance = GetAssistance();
    if (iAssistance != pCompare->GetAssistance()) {
        return true;
    }
    if (AIData.cConfidence != pCompare->AIData.cConfidence) {
        return true;
    }
    if (AIData.cEnergy != pCompare->AIData.cEnergy) {
        return true;
    }
    int32_t iResponsibility = GetResponsibility();
    if (iResponsibility != pCompare->GetResponsibility()) {
        return true;
    }
    if (AIData.cMood != pCompare->AIData.cMood) {
        return true;
    }
    if (AIData.uiAggroRadius != pCompare->AIData.uiAggroRadius) {
        return true;
    }
    int32_t iHasAggroRadius = GetHasAggroRadius();
    if (iHasAggroRadius != pCompare->GetHasAggroRadius()) {
        return true;
    }
    if (AIData.iServiceFlags != pCompare->AIData.iServiceFlags) {
        return true;
    }
    ActorValue::Index eTrainingSkill = GetTrainingSkill();
    if (eTrainingSkill != pCompare->GetTrainingSkill()) {
        return true;
    }
    return AIData.cTrainingLevel != pCompare->AIData.cTrainingLevel || AIPackList.Compare(&pCompare->AIPackList);
}

uint16_t TESAIForm::GetSaveSize(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_ACTOR_BASE_AIDATA) != 0) {
        return 5;
    }
    return 0;
}

void TESAIForm::SaveGame(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_ACTOR_BASE_AIDATA) != 0) {
        pSaveLoadGameOLD->SaveGameDataOLD(&AIData, 1);
        pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cConfidence, sizeof(AIData.cConfidence));
        pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cEnergy, sizeof(AIData.cEnergy));
        pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cResponsibility, sizeof(AIData.cResponsibility));
        pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cMood, sizeof(AIData.cMood));
    }
}

void TESAIForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if ((aiFlags & CHANGE_ACTOR_BASE_AIDATA) != 0) {
        pSaveLoadGameOLD->LoadGameDataOLD(&AIData, 1);
        pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cConfidence, sizeof(AIData.cConfidence));
        pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cEnergy, sizeof(AIData.cEnergy));
        pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cResponsibility, sizeof(AIData.cResponsibility));
        pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cMood, sizeof(AIData.cMood));
    }
}

void TESAIForm::AddPackage(TESPackage* apPack) {
    AIPackList.listPackages.AddTailByValue(apPack);
}

void TESAIForm::InitItem(TESForm* apOwner) {
    if (!bFixAIPackagesOnLoad.Bool()) {
        AIPackList.InitItem(apOwner);
    }
}

void TESAIForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_AIDATA)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&AIData), sizeof(AIData), 0);
    }
}

void TESAIForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_AIDATA)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&AIData), sizeof(AIData));
    }
}

void TESAIForm::SetAggression(uint8_t acAggression, bool abAddChange) {
    AIData.cAggression = acAggression;
    if (abAddChange) {
        AddFormChange(CHANGE_AI_DATA);
    }
}

void TESAIForm::SetConfidence(uint8_t acConfidence, bool abAddChange) {
    AIData.cConfidence = acConfidence;
    if (abAddChange) {
        AddFormChange(CHANGE_AI_DATA);
    }
}

void TESAIForm::SetEnergy(uint8_t acEnergy, bool abAddChange) {
    AIData.cEnergy = acEnergy;
    if (abAddChange) {
        AddFormChange(CHANGE_AI_DATA);
    }
}

void TESAIForm::SetResponsibility(uint8_t acResponsibility, bool abAddChange) {
    AIData.cResponsibility = acResponsibility;
    if (abAddChange) {
        AddFormChange(CHANGE_AI_DATA);
    }
}

void TESAIForm::SetMood(uint8_t acMood) {
    if (acMood < 8) {
        AIData.cMood = acMood;
        AddFormChange(CHANGE_AI_DATA);
    }
}

void TESAIForm::SetAssistance(uint8_t acAssistance) {
    AIData.cAssistance = acAssistance;
    AddFormChange(CHANGE_AI_DATA);
}

void TESAIForm::SetHasAggroRadius(uint8_t abAggro) {
    AIData.bAggroRadius = abAggro;
    AddFormChange(CHANGE_AI_DATA);
}

void TESAIForm::SetAggroRadius(uint32_t auiRadius) {
    AIData.uiAggroRadius = auiRadius;
    AddFormChange(CHANGE_AI_DATA);
}

void TESAIForm::SetServiceFlags(uint32_t aiServeFlags, bool abAddChange) {
    AIData.iServiceFlags = aiServeFlags;
    if (abAddChange) {
        AddFormChange(CHANGE_AI_DATA);
    }
}

bool TESAIForm::GetOffersService(uint32_t aiServeFlag) {
    return (aiServeFlag & AIData.iServiceFlags) != 0;
}

// Skill actor values start at index 32.
ActorValue::Index TESAIForm::GetTrainingSkill() const {
    return static_cast<ActorValue::Index>(AIData.cTrainingSkill + 32);
}

// Copies each AI setting through apAIForm's getters and this form's setters.
void TESAIForm::CopyAIData(TESAIForm* apAIForm) {
    SetAggression(apAIForm->GetAggression(), true);
    SetAssistance(apAIForm->GetAssistance());
    SetConfidence(apAIForm->GetConfidence(), true);
    SetEnergy(apAIForm->GetEnergy(), true);
    SetResponsibility(apAIForm->GetResponsibility(), true);
    SetMood(apAIForm->GetMood());
    SetHasAggroRadius(apAIForm->GetHasAggroRadius());
    SetAggroRadius(apAIForm->GetAggroRadius());
    SetServiceFlags(apAIForm->GetServiceFlags(), true);
    SetTrainingSkill(apAIForm->GetTrainingSkill());
    SetTrainingSkillLevel(apAIForm->GetTrainingSkillLevel());
}

void TESAIForm::CopyAIPackageList(TESAIForm* apAIForm) {
    AIPackList.listPackages.CopyList(&apAIForm->AIPackList.listPackages);
}

void TESAIForm::AddFormChange(int32_t aiChangeFlags) {
    TESActorBase* pParent = static_cast<TESActorBase*>(this);
    if (pParent) {
        pParent->AddChange(aiChangeFlags);
    }
}

uint8_t TESAIForm::GetResponsibility() {
    return AIData.cResponsibility;
}

void TESAIForm::SetAIData(AIDATA* pAIData) {
    if (pAIData) {
        SetAggression(pAIData->cAggression, true);
        SetAssistance(pAIData->cAssistance);
        SetConfidence(pAIData->cConfidence, true);
        SetEnergy(pAIData->cEnergy, true);
        SetResponsibility(pAIData->cResponsibility, true);
        SetServiceFlags(pAIData->iServiceFlags, true);
        SetMood(pAIData->cMood);
        SetHasAggroRadius(pAIData->bAggroRadius);
        SetAggroRadius(pAIData->uiAggroRadius);
        AIData.cTrainingSkill = pAIData->cTrainingSkill;
        AIData.cTrainingLevel = pAIData->cTrainingLevel;
        if (AIData.cConfidence >= 5u) {
            AIData.cConfidence = 0;
        }
    }
}

uint8_t TESAIForm::GetMood() {
    return AIData.cMood;
}

void TESAIForm::SetTrainingSkill(ActorValue::Index aeSkill) {
    if (ActorValue::IsSkill(aeSkill)) {
        AIData.cTrainingSkill = static_cast<uint8_t>(aeSkill - ActorValue::SKILL_START);
    }
    AddFormChange(CHANGE_AI_DATA);
}

void TESAIForm::SetTrainingSkillLevel(uint8_t aiVal) {
    AIData.cTrainingLevel = aiVal;
    AddFormChange(CHANGE_AI_DATA);
}

bool TESAIForm::SellBuysItem(TESObject* apObject) {
    uint8_t ucHasBuySellFlag = 0;
    switch (apObject->TESForm::GetFormType()) {
        case ARMO_ID: {
            ucHasBuySellFlag = GetOffersService(2);
            break;
        }
        case BOOK_ID: {
            ucHasBuySellFlag = GetOffersService(8);
            break;
        }
        case MISC_ID:
        case COBJ_ID:
        case IMOD_ID: {
            ucHasBuySellFlag = GetOffersService(1024);
            break;
        }
        case WEAP_ID:
        case AMMO_ID: {
            ucHasBuySellFlag = GetOffersService(1);
            break;
        }
        case ALCH_ID: {
            switch (BGSEquipType::GetEquipType(apObject)) {
                case BGSEquipType::ET_CHEMS: {
                    ucHasBuySellFlag = GetOffersService(32);
                    break;
                }
                case BGSEquipType::ET_STIMPACK: {
                    ucHasBuySellFlag = GetOffersService(64);
                    break;
                }
                case BGSEquipType::ET_FOOD: {
                    ucHasBuySellFlag = GetOffersService(16);
                    break;
                }
                case BGSEquipType::ET_ALCOHOL: {
                    ucHasBuySellFlag = GetOffersService(4);
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }
    return ucHasBuySellFlag != 0;
}

TESPackage* TESAIForm::GetCurrentPackage(Actor* apActor) {
    if (apActor) {
        TESActorBase* pActorBase = apActor->GetActorBase();
        TESAIForm* pAIForm = static_cast<TESAIForm*>(pActorBase);
        if (pAIForm) {
            return pAIForm->AIPackList.FindCurrentPackage(apActor);
        }
    }
    return nullptr;
}

// The value of confidence level acConfidence; 0 past the last level.
float TESAIForm::GetConfidenceLevel(uint8_t acConfidence) {
    if (acConfidence < 5) {
        return fConfidences[acConfidence];
    }
    return 0.0f;
}

void TESAIForm::GetMissedPackages(Actor* apActor, BSSimpleList<TESPackage*>* apPacks, float afStartdate, float afhour) {
    if (apActor) {
        TESActorBase* pActorBase = apActor->GetActorBase();
        TESAIForm* pAIForm = static_cast<TESAIForm*>(pActorBase);
        if (pAIForm) {
            pAIForm->AIPackList.FindMissedPackages(apActor, apPacks, afStartdate, afhour);
        }
    }
}
