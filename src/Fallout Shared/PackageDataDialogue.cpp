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
            pTargetLocation = nullptr;
        }
        if (pcopy->pTargetLocation) {
            PackageLocation* pLocation;
            pLocation = new PackageLocation;
            pTargetLocation = pLocation;
            pTargetLocation->Copy(pcopy->pTargetLocation, false);
        }
        ptopic = pcopy->ptopic;
        bDoNotControlTarget = pcopy->bDoNotControlTarget;
        bSayTo = pcopy->bSayTo;
        eTriggerType = pcopy->eTriggerType;
    }
}

bool TESDialoguePackageData::Compare(TESPackageData* apCompare) {
    TESDialoguePackageData* pCompare = dynamic_cast<TESDialoguePackageData*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pCompare->fFov != fFov) {
        return true;
    }
    if (pCompare->fDistanceStartTalking != fDistanceStartTalking) {
        return true;
    }
    if (bNOHeadtracking != pCompare->bNOHeadtracking) {
        return true;
    }
    if (ptopic != pCompare->ptopic) {
        return true;
    }
    if (pTargetLocation) {
        if (pTargetLocation->Compare(pCompare->pTargetLocation)) {
            return true;
        }
    }
    else if (pCompare->pTargetLocation) {
        return true;
    }
    return bDoNotControlTarget != pCompare->bDoNotControlTarget ||
        bSayTo != pCompare->bSayTo ||
        eTriggerType != pCompare->eTriggerType;
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
    if (ptopic) {
        kData.iTopicID = ptopic->GetFormID();
    }
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(PKDD_ID, &kData, sizeof(kData));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    if (pTargetLocation) {
        pTargetLocation->Save(PLD2_ID);
    }
}

// The data fields one by one, the topic's form ID, then a byte saying whether a target location follows and the
// location.
void TESDialoguePackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fFov), sizeof(fFov), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bNOHeadtracking), sizeof(bNOHeadtracking), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoNotControlTarget), sizeof(bDoNotControlTarget), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSpeakerMoveTalk), sizeof(bSpeakerMoveTalk), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDistanceStartTalking), sizeof(fDistanceStartTalking), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSayTo), sizeof(bSayTo), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTriggerType), sizeof(eTriggerType), 0);
    apSaveGameBuffer->SaveFormID(ptopic, 0);
    uint8_t bHasLocation = pTargetLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pTargetLocation->SaveGame(apSaveGameBuffer);
    }
}

// Reads what SaveGame writes.
void TESDialoguePackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFov), sizeof(fFov));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bNOHeadtracking), sizeof(bNOHeadtracking));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNotControlTarget), sizeof(bDoNotControlTarget));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeakerMoveTalk), sizeof(bSpeakerMoveTalk));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDistanceStartTalking), sizeof(fDistanceStartTalking));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSayTo), sizeof(bSayTo));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eTriggerType), sizeof(eTriggerType));
    ptopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pTargetLocation = new PackageLocation;
        pTargetLocation->LoadGame(apLoadGameBuffer);
    }
}

// Slot 7: passes the load-game buffer on to the target location, if there is one.
void TESDialoguePackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pTargetLocation) {
        pTargetLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESDialoguePackageData::~TESDialoguePackageData() {
    delete pTargetLocation;
}
