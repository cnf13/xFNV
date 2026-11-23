#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

TESPackageData::TESPackageData() {}

TESPackageData::~TESPackageData() {}

// The base data differs from every other data.
bool TESPackageData::Compare(TESPackageData* apData) {
    return true;
}

void TESPackageData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void TESAmbushPackageData::Copy(TESPackageData* apCopy) {
    TESAmbushPackageData* pCopy = dynamic_cast<TESAmbushPackageData*>(apCopy);
    if (pCopy) {
        if (pAmbushLocation) {
            delete pAmbushLocation;
            pAmbushLocation = nullptr;
        }
        if (pCopy->pAmbushLocation) {
            PackageLocation* pLocation;
            pLocation = new PackageLocation;
            pAmbushLocation = pLocation;
            pAmbushLocation->Copy(pCopy->pAmbushLocation, false);
        }
    }
}

bool TESAmbushPackageData::Compare(TESPackageData* apCompare) {
    TESAmbushPackageData* pCompare = dynamic_cast<TESAmbushPackageData*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pAmbushLocation) {
        if (pAmbushLocation->Compare(pCompare->pAmbushLocation)) {
            return true;
        }
    }
    else if (pCompare->pAmbushLocation) {
        return true;
    }
    return false;
}

void TESAmbushPackageData::Save() {
    TESForm::AddChunk(PKAM_ID);
    if (pAmbushLocation) {
        pAmbushLocation->Save(PLD2_ID);
    }
}

// The eat and use item data share this entry. A byte saying whether a location follows, then the location.
void TESAmbushPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t bHasLocation = pAmbushLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pAmbushLocation->SaveGame(apSaveGameBuffer);
    }
}

// The eat and use item data share this entry. A byte saying whether a location follows, then the location.
void TESAmbushPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pAmbushLocation = new PackageLocation;
        pAmbushLocation->LoadGame(apLoadGameBuffer);
    }
}

// The eat, escort, follow and use item data share this entry; each keeps its location in the field at +4.
void TESAmbushPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pAmbushLocation) {
        pAmbushLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESAmbushPackageData::~TESAmbushPackageData() {
    delete pAmbushLocation;
}
