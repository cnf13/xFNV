#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

void PACK_FOLLOW_DATA::Endian() {
    ENDIAN(fTriggerRadius, false);
}

void TESFollowPackageData::Copy(TESPackageData* apCopy) {
    TESFollowPackageData* pcopy = dynamic_cast<TESFollowPackageData*>(apCopy);
    if (pcopy) {
        fTriggerRadius = pcopy->fTriggerRadius;
        if (pStartLocation) {
            delete pStartLocation;
            pStartLocation = nullptr;
        }
        if (pcopy->pStartLocation) {
            pStartLocation = new PackageLocation;
            pStartLocation->Copy(pcopy->pStartLocation, false);
        }
    }
}

bool TESFollowPackageData::Compare(TESPackageData* apCompare) {
    TESFollowPackageData* pCompare = dynamic_cast<TESFollowPackageData*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pCompare->fTriggerRadius != fTriggerRadius) {
        return true;
    }
    if (pStartLocation) {
        if (pStartLocation->Compare(pCompare->pStartLocation)) {
            return true;
        }
    }
    else if (pCompare->pStartLocation) {
        return true;
    }
    return false;
}

void TESFollowPackageData::Save() {
    PACK_FOLLOW_DATA kData;
    ourMemset(&kData, 0, sizeof(PACK_FOLLOW_DATA));
    kData.fTriggerRadius = fTriggerRadius;
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(PKFD_ID, &kData, sizeof(PACK_FOLLOW_DATA));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    if (pStartLocation) {
        pStartLocation->Save(PLD2_ID);
    }
}

// The trigger radius, a byte saying whether a start location follows, then the location.
void TESFollowPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fTriggerRadius), sizeof(fTriggerRadius), 0);
    uint8_t bHasLocation = pStartLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pStartLocation->SaveGame(apSaveGameBuffer);
    }
}

// The trigger radius, a byte saying whether a start location follows, then the location.
void TESFollowPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fTriggerRadius), sizeof(fTriggerRadius));
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pStartLocation = new PackageLocation;
        pStartLocation->LoadGame(apLoadGameBuffer);
    }
}

void TESFollowPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pStartLocation) {
        pStartLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESFollowPackageData::~TESFollowPackageData() {
    delete pStartLocation;
}
