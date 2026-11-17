#include "Fallout Shared/TESAIForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/Package.h"
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
SettingT<GameSettingCollection> gs_fConfidenceFoolhardy("fConfidenceFoolhardy", 0.0f);
SettingT<GameSettingCollection> gs_fConfidenceBrave("fConfidenceBrave", 0.5f);
SettingT<GameSettingCollection> gs_fConfidenceAverage("fConfidenceAverage", 1.0f);
SettingT<GameSettingCollection> gs_fConfidenceCautious("fConfidenceCautious", 2.0f);
SettingT<GameSettingCollection> gs_fConfidenceCowardly("fConfidenceCowardly", 1000.0f);

// Swaps the two 32-bit fields read from a big-endian file; the byte fields are left alone.
void AIDATA::Endian() {
	ENDIAN(iServiceFlags, false);
	ENDIAN(uiAggroRadius, false);
}

TESAIForm::TESAIForm() {
	TESAIForm::InitializeDataComponent();
}

TESAIForm::~TESAIForm() {
	TESAIForm::ClearDataComponent();
}

void TESAIForm::InitializeDataComponent() {
	AIData.cAggression = (uint8_t)0;
	AIData.cConfidence = (uint8_t)4;
	AIData.cEnergy = (uint8_t)0;
	AIData.cResponsibility = (uint8_t)0;
	AIData.cMood = (uint8_t)0;
	AIData.iServiceFlags = (uint32_t)0;
	AIData.cTrainingSkill = (uint8_t)0;
	AIData.cTrainingLevel = (uint8_t)0;
	AIData.cAssistance = (uint8_t)0;
	AIData.bAggroRadius = (uint8_t)0;
	AIData.uiAggroRadius = (uint32_t)0;
	if (!confidenceSet) {
		fConfidences[0] = (float)((&gs_fConfidenceCowardly)->Float());
		fConfidences[1] = (float)((&gs_fConfidenceCautious)->Float());
		fConfidences[2] = (float)((&gs_fConfidenceAverage)->Float());
		fConfidences[3] = (float)((&gs_fConfidenceBrave)->Float());
		fConfidences[4] = (float)((&gs_fConfidenceFoolhardy)->Float());
		confidenceSet = (uint8_t)1;
	}
}

void TESAIForm::ClearDataComponent() {
	AIPackList.listPackages.RemoveAll();
}

void TESAIForm::CopyComponent(BaseFormComponent* apCopyFC) {
	TESAIForm* v3 = dynamic_cast<TESAIForm*>(apCopyFC);
	if (v3) {
		CopyAIData(v3);
		CopyAIPackageList(v3);
	}
}

bool TESAIForm::CompareComponent(BaseFormComponent* apCompareFC) {
	TESAIForm* pCompare = dynamic_cast<TESAIForm*>(apCompareFC);
	if (!pCompare) {
		return 1;
	}
	int32_t ExtraType = AIData.cAggression;
	if (ExtraType != pCompare->AIData.cAggression) {
		return 1;
	}
	int32_t Assistance = GetAssistance();
	if (Assistance != pCompare->GetAssistance()) {
		return 1;
	}
	int32_t Priority = AIData.cConfidence;
	if (Priority != pCompare->AIData.cConfidence) {
		return 1;
	}
	int32_t FlyCamera = AIData.cEnergy;
	if (FlyCamera != pCompare->AIData.cEnergy) {
		return 1;
	}
	int32_t Responsibility = GetResponsibility();
	if (Responsibility != pCompare->GetResponsibility()) {
		return 1;
	}
	int32_t EntryPoint = AIData.cMood;
	if (EntryPoint != pCompare->AIData.cMood) {
		return 1;
	}
	uint32_t Time = AIData.uiAggroRadius;
	if (Time != pCompare->AIData.uiAggroRadius) {
		return 1;
	}
	int32_t HasAggroRadius = GetHasAggroRadius();
	if (HasAggroRadius != pCompare->GetHasAggroRadius()) {
		return 1;
	}
	int32_t CastingType = AIData.iServiceFlags;
	if (CastingType != pCompare->AIData.iServiceFlags) {
		return 1;
	}
	ActorValue::Index TrainingSkill = GetTrainingSkill();
	if (TrainingSkill != pCompare->GetTrainingSkill()) {
		return 1;
	}
	int32_t RepeatFire = AIData.cTrainingLevel;
	return RepeatFire != pCompare->AIData.cTrainingLevel || AIPackList.Compare(&pCompare->AIPackList);
}

uint16_t TESAIForm::GetSaveSize(uint32_t aiFlags) {
	uint16_t v3 = 0;
	if ((aiFlags & 8) != 0) {
		return (uint16_t)5;
	}
	return v3;
}

void TESAIForm::SaveGame(uint32_t aiFlags) {
	if ((((int8_t)aiFlags) & 8) != 0) {
		pSaveLoadGameOLD->SaveGameDataOLD(&AIData, 1);
		pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cConfidence, 1);
		pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cEnergy, 1);
		pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cResponsibility, 1);
		pSaveLoadGameOLD->SaveGameDataOLD(&AIData.cMood, 1);
	}
}

void TESAIForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	if ((((int8_t)aiFlags) & 8) != 0) {
		pSaveLoadGameOLD->LoadGameDataOLD(&AIData, 1);
		pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cConfidence, 1);
		pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cEnergy, 1);
		pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cResponsibility, 1);
		pSaveLoadGameOLD->LoadGameDataOLD(&AIData.cMood, 1);
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
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&AIData), 0x14, 0);
	}
}

void TESAIForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_AIDATA)) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&AIData), 0x14);
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
	return ((aiServeFlag & AIData.iServiceFlags) != 0);
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
	BSSimpleList<TESPackage*>* FormList = &apAIForm->AIPackList.listPackages;
	BSSimpleList<TESPackage*>* v2 = &AIPackList.listPackages;
	v2->CopyList(FormList);
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
		SetAggression(pAIData->cAggression, 1);
		SetAssistance(pAIData->cAssistance);
		SetConfidence(pAIData->cConfidence, 1);
		SetEnergy(pAIData->cEnergy, 1);
		SetResponsibility(pAIData->cResponsibility, 1);
		SetServiceFlags(pAIData->iServiceFlags, 1);
		SetMood(pAIData->cMood);
		SetHasAggroRadius(pAIData->bAggroRadius);
		SetAggroRadius(pAIData->uiAggroRadius);
		AIData.cTrainingSkill = pAIData->cTrainingSkill;
		AIData.cTrainingLevel = pAIData->cTrainingLevel;
		if (((uint32_t)((int32_t)AIData.cConfidence)) >= 5u) {
			AIData.cConfidence = (uint8_t)0;
		}
	}
}

uint8_t TESAIForm::GetMood() {
	return AIData.cMood;
}

void TESAIForm::SetTrainingSkill(ActorValue::Index aeSkill) {
	if (ActorValue::IsSkill(aeSkill)) {
		AIData.cTrainingSkill = (uint8_t)(aeSkill - 32);
	}
	AddFormChange(CHANGE_AI_DATA);
}

void TESAIForm::SetTrainingSkillLevel(uint8_t aiVal) {
	AIData.cTrainingLevel = aiVal;
	AddFormChange(CHANGE_AI_DATA);
}

bool TESAIForm::SellBuysItem(TESObject* apObject) {
	uint8_t hasBuySellFlag = 0;
	switch ((int32_t)apObject->TESForm::GetFormType()) {
	case 24: {
		hasBuySellFlag = (uint8_t)GetOffersService(2);
		break;
	}
	case 25: {
		hasBuySellFlag = (uint8_t)GetOffersService(8);
		break;
	}
	case 31:
	case 50:
	case 103: {
		hasBuySellFlag = (uint8_t)GetOffersService(1024);
		break;
	}
	case 40:
	case 41: {
		hasBuySellFlag = (uint8_t)GetOffersService(1);
		break;
	}
	case 47: {
		switch ((int32_t)BGSEquipType::GetEquipType(apObject)) {
		case 10: {
			hasBuySellFlag = (uint8_t)GetOffersService(32);
			break;
		}
		case 11: {
			hasBuySellFlag = (uint8_t)GetOffersService(64);
			break;
		}
		case 12: {
			hasBuySellFlag = (uint8_t)GetOffersService(16);
			break;
		}
		case 13: {
			hasBuySellFlag = (uint8_t)GetOffersService(4);
			break;
		}
		default: {
			return (bool)hasBuySellFlag;
		}
		}
		break;
	}
	default: {
		return (bool)hasBuySellFlag;
	}
	}
	return (bool)hasBuySellFlag;
}

TESPackage* TESAIForm::GetCurrentPackage(Actor* apActor) {
	TESPackage* ppack = 0;
	if (apActor) {
		TESAIForm* pAIForm;
		TESActorBase* pactorbase = apActor->GetActorBase();
		if (pactorbase) {
			pAIForm = static_cast<TESAIForm*>(pactorbase);
		} else {
			pAIForm = 0;
		}
		if (pAIForm) {
			return pAIForm->AIPackList.FindCurrentPackage(apActor);
		}
	}
	return ppack;
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
		TESAIForm* paiform;
		TESActorBase* pactorbase = apActor->GetActorBase();
		if (pactorbase) {
			paiform = static_cast<TESAIForm*>(pactorbase);
		} else {
			paiform = 0;
		}
		if (paiform) {
			paiform->AIPackList.FindMissedPackages(apActor, apPacks, afStartdate, afhour);
		}
	}
}
