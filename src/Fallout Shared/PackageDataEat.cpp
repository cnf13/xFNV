#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

void TESEatPackageData::Copy(TESPackageData* apCopy) {
    TESEatPackageData* pEatData = dynamic_cast<TESEatPackageData*>(apCopy);
    if (pEatData) {
        if (pSearchLocation) {
            delete pSearchLocation;
            pSearchLocation = nullptr;
        }
        if (pEatData->pSearchLocation) {
            PackageLocation* pLocation = new PackageLocation;
            pSearchLocation = pLocation;
            pSearchLocation->Copy(pEatData->pSearchLocation, false);
        }
    }
}

bool TESEatPackageData::Compare(TESPackageData* apCompare) {
    TESEatPackageData* pEatData = dynamic_cast<TESEatPackageData*>(apCompare);
    if (!pEatData) {
        return true;
    }
    if (pSearchLocation) {
        if (pSearchLocation->Compare(pEatData->pSearchLocation)) {
            return true;
        }
    }
    else if (pEatData->pSearchLocation) {
        return true;
    }
    return false;
}

void TESEatPackageData::Save() {
    TESForm::AddChunk(PKED_ID);
    if (pSearchLocation) {
        pSearchLocation->Save(PLD2_ID);
    }
}

// A byte saying whether a search location follows, then the location.
void TESEatPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t bHasLocation = pSearchLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pSearchLocation->SaveGame(apSaveGameBuffer);
    }
}

// A byte saying whether a search location follows, then the location.
void TESEatPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pSearchLocation = new PackageLocation;
        pSearchLocation->LoadGame(apLoadGameBuffer);
    }
}

void TESEatPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pSearchLocation) {
        pSearchLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESEatPackageData::~TESEatPackageData() {
    delete pSearchLocation;
}
