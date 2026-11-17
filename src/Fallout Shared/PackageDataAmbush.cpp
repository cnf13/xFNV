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
    TESAmbushPackageData* pcopy = dynamic_cast<TESAmbushPackageData*>(apCopy);
    if (pcopy) {
        if (pAmbushLocation) {
            delete pAmbushLocation;
            pAmbushLocation = nullptr;
        }
        if (pcopy->pAmbushLocation) {
            PackageLocation* v2;
            PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
            if (v4) {
                v2 = ::new (v4) PackageLocation;
            }
            else {
                v2 = nullptr;
            }
            pAmbushLocation = v2;
            pAmbushLocation->Copy(pcopy->pAmbushLocation, false);
        }
    }
}

bool TESAmbushPackageData::Compare(TESPackageData* apCompare) {
    TESAmbushPackageData* v4 = dynamic_cast<TESAmbushPackageData*>(apCompare);
    if (!v4) {
        return true;
    }
    if (pAmbushLocation) {
        if (pAmbushLocation->Compare(v4->pAmbushLocation)) {
            return true;
        }
    }
    else {
        if (v4->pAmbushLocation) {
            return true;
        }
    }
    return false;
}

void TESAmbushPackageData::Save() {
    TESForm::AddChunk(static_cast<CHUNK_ID>(1296124752));
    if (pAmbushLocation) {
        pAmbushLocation->Save(PLD2_ID);
    }
}

// The eat and use item data share this entry. A byte saying whether a location follows, then the location.
void TESAmbushPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t bHasLocation = pAmbushLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), 1, 0);
    if (bHasLocation) {
        pAmbushLocation->SaveGame(apSaveGameBuffer);
    }
}

// The eat and use item data share this entry. A byte saying whether a location follows, then the location.
void TESAmbushPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), 1);
    if (bHasLocation) {
        void* pMemory = ourMalloc(sizeof(PackageLocation));
        pAmbushLocation = pMemory ? ::new (pMemory) PackageLocation : nullptr;
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
