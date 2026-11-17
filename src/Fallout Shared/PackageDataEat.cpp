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
    TESEatPackageData* v5 = dynamic_cast<TESEatPackageData*>(apCopy);
    if (v5) {
        if (pSearchLocation) {
            delete pSearchLocation;
            pSearchLocation = 0;
        }
        if (v5->pSearchLocation) {
            PackageLocation* v2;
            PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
            if (v4) {
                v2 = ::new (v4) PackageLocation;
            }
            else {
                v2 = 0;
            }
            pSearchLocation = v2;
            pSearchLocation->Copy(v5->pSearchLocation, 0);
        }
    }
}

bool TESEatPackageData::Compare(TESPackageData* apCompare) {
    TESEatPackageData* v4 = dynamic_cast<TESEatPackageData*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (pSearchLocation) {
        if (pSearchLocation->Compare(v4->pSearchLocation)) {
            return (bool)1;
        }
    }
    else {
        if (v4->pSearchLocation) {
            return (bool)1;
        }
    }
    return (bool)0;
}

void TESEatPackageData::Save() {
    TESForm::AddChunk(static_cast<CHUNK_ID>(1145391952));
    if (pSearchLocation) {
        pSearchLocation->Save(PLD2_ID);
    }
}

// A byte saying whether a search location follows, then the location.
void TESEatPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t bHasLocation = pSearchLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), 1, 0);
    if (bHasLocation) {
        pSearchLocation->SaveGame(apSaveGameBuffer);
    }
}

// A byte saying whether a search location follows, then the location.
void TESEatPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), 1);
    if (bHasLocation) {
        void* pMemory = ourMalloc(sizeof(PackageLocation));
        pSearchLocation = pMemory ? ::new (pMemory) PackageLocation : nullptr;
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
