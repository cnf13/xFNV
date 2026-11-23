#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

void TESUseItemPackageData::Copy(TESPackageData* apCopy) {
    TESUseItemPackageData* pCopy = dynamic_cast<TESUseItemPackageData*>(apCopy);
    if (pCopy) {
        if (pSearchLocation) {
            delete pSearchLocation;
            pSearchLocation = nullptr;
        }
        if (pCopy->pSearchLocation) {
            pSearchLocation = new PackageLocation;
            pSearchLocation->Copy(pCopy->pSearchLocation, false);
        }
    }
}

bool TESUseItemPackageData::Compare(TESPackageData* apCompare) {
    TESUseItemPackageData* pCompare = dynamic_cast<TESUseItemPackageData*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pSearchLocation) {
        if (pSearchLocation->Compare(pCompare->pSearchLocation)) {
            return true;
        }
    }
    else if (pCompare->pSearchLocation) {
        return true;
    }
    return false;
}

void TESUseItemPackageData::Save() {
    TESForm::AddChunk(PUID_ID);
    if (pSearchLocation) {
        pSearchLocation->Save(PLD2_ID);
    }
}

// A byte saying whether a search location follows, then the location.
void TESUseItemPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t bHasLocation = pSearchLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pSearchLocation->SaveGame(apSaveGameBuffer);
    }
}

// A byte saying whether a search location follows, then the location.
void TESUseItemPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pSearchLocation = new PackageLocation;
        pSearchLocation->LoadGame(apLoadGameBuffer);
    }
}

void TESUseItemPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pSearchLocation) {
        pSearchLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESUseItemPackageData::~TESUseItemPackageData() {
    delete pSearchLocation;
}
