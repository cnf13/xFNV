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
            pStartLocation = 0;
        }
        if (pcopy->pStartLocation) {
            PackageLocation* v2;
            PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
            if (v4) {
                v2 = ::new (v4) PackageLocation;
            }
            else {
                v2 = 0;
            }
            pStartLocation = v2;
            pStartLocation->Copy(pcopy->pStartLocation, 0);
        }
    }
}

bool TESFollowPackageData::Compare(TESPackageData* apCompare) {
    TESFollowPackageData* v4 = dynamic_cast<TESFollowPackageData*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (v4->fTriggerRadius != fTriggerRadius) {
        return (bool)1;
    }
    if (pStartLocation) {
        if (pStartLocation->Compare(v4->pStartLocation)) {
            return (bool)1;
        }
    }
    else {
        if (v4->pStartLocation) {
            return (bool)1;
        }
    }
    return (bool)0;
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fTriggerRadius), 4, 0);
    uint8_t bHasLocation = pStartLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), 1, 0);
    if (bHasLocation) {
        pStartLocation->SaveGame(apSaveGameBuffer);
    }
}

// The trigger radius, a byte saying whether a start location follows, then the location.
void TESFollowPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fTriggerRadius), 4);
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), 1);
    if (bHasLocation) {
        void* pMemory = ourMalloc(sizeof(PackageLocation));
        pStartLocation = pMemory ? ::new (pMemory) PackageLocation : nullptr;
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
