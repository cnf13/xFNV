#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

void TESDialoguePackageData::Copy(TESPackageData* apCopy) {
	TESDialoguePackageData* pcopy = dynamic_cast<TESDialoguePackageData*>(apCopy);
	if (pcopy) {
		fFov = pcopy->fFov;
		fDistanceStartTalking = pcopy->fDistanceStartTalking;
		bNOHeadtracking = pcopy->bNOHeadtracking;
		if (pTargetLocation) {
			delete pTargetLocation;
			pTargetLocation = 0;
		}
		if (pcopy->pTargetLocation) {
			PackageLocation* v2;
			PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
			if (v4) {
				v2 = ::new (v4) PackageLocation;
			} else {
				v2 = 0;
			}
			pTargetLocation = v2;
			pTargetLocation->Copy(pcopy->pTargetLocation, 0);
		}
		ptopic = pcopy->ptopic;
		bDoNotControlTarget = pcopy->bDoNotControlTarget;
		bSayTo = pcopy->bSayTo;
		eTriggerType = pcopy->eTriggerType;
	}
}

bool TESDialoguePackageData::Compare(TESPackageData* apCompare) {
	TESDialoguePackageData* v4 = dynamic_cast<TESDialoguePackageData*>(apCompare);
	if (!v4) {
		return (bool)1;
	}
	if (v4->fFov != fFov) {
		return (bool)1;
	}
	if (v4->fDistanceStartTalking != fDistanceStartTalking) {
		return (bool)1;
	}
	if (((uint32_t)((uint8_t)bNOHeadtracking)) != ((uint32_t)((int8_t)v4->bNOHeadtracking))) {
		return (bool)1;
	}
	if (ptopic != v4->ptopic) {
		return (bool)1;
	}
	if (pTargetLocation) {
		if (pTargetLocation->Compare(v4->pTargetLocation)) {
			return (bool)1;
		}
	} else {
		if (v4->pTargetLocation) {
			return (bool)1;
		}
	}
	return (bool)((uint8_t)((((uint32_t)((uint8_t)bDoNotControlTarget)) !=
								((uint32_t)((int8_t)v4->bDoNotControlTarget))) ||
		((((uint32_t)((uint8_t)bSayTo)) != ((uint32_t)((int8_t)v4->bSayTo))) || (eTriggerType != v4->eTriggerType))));
}

// Writes the PKDD chunk from a zeroed copy of the data (bSpeakerMoveTalk stays 0, the topic is saved as its form ID),
// swapped around the write for a big-endian save, then the target location as PLD2.
void TESDialoguePackageData::Save() {
	PACK_DIALOGUE_DATA kData;
	ourMemset(&kData, 0, sizeof(kData));
	kData.bNOHeadtracking = bNOHeadtracking;
	kData.fFov = fFov;
	kData.bDoNotControlTarget = bDoNotControlTarget;
	kData.bSayTo = bSayTo;
	kData.eTriggerType = eTriggerType;
	kData.fDistanceStartTalking = fDistanceStartTalking;
	if (ptopic)
		kData.iTopicID = ptopic->GetFormID();
	if (TESForm::QEndianSwapOnSave())
		kData.Endian();
	TESForm::__AddChunkData(PKDD_ID, &kData, sizeof(kData));
	if (TESForm::QEndianSwapOnSave())
		kData.Endian();
	if (pTargetLocation)
		pTargetLocation->Save(PLD2_ID);
}

// The data fields one by one, the topic's form ID, then a byte saying whether a target location follows and the
// location.
void TESDialoguePackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fFov), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bNOHeadtracking), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoNotControlTarget), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSpeakerMoveTalk), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDistanceStartTalking), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSayTo), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTriggerType), 4, 0);
	apSaveGameBuffer->SaveFormID(ptopic, 0);
	uint8_t bHasLocation = pTargetLocation != nullptr;
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), 1, 0);
	if (bHasLocation)
		pTargetLocation->SaveGame(apSaveGameBuffer);
}

// Reads what SaveGame writes.
void TESDialoguePackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFov), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bNOHeadtracking), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNotControlTarget), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeakerMoveTalk), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDistanceStartTalking), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSayTo), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eTriggerType), 4);
	ptopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
	uint8_t bHasLocation = 0;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), 1);
	if (bHasLocation) {
		void* pMemory = ourMalloc(sizeof(PackageLocation));
		pTargetLocation = pMemory ? ::new (pMemory) PackageLocation : nullptr;
		pTargetLocation->LoadGame(apLoadGameBuffer);
	}
}

// Slot 7: passes the load-game buffer on to the target location, if there is one.
void TESDialoguePackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (pTargetLocation)
		pTargetLocation->InitLoadGame(apLoadGameBuffer);
}

TESDialoguePackageData::~TESDialoguePackageData() {
	delete pTargetLocation;
}
