#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

// Swaps the radius read from a big-endian file. PACK_FOLLOW_DATA::Endian shares the code.
void PACK_ESCORT_DATA::Endian() {
    ENDIAN(iFollowPackRadius, false);
}

// From the package's file data: an empty search location and the radius the file data holds.
TESEscortPackageData::TESEscortPackageData(const PACK_ESCORT_DATA* apEscData)
  : pSearchLocation(nullptr), iFollowPackRadius(apEscData->iFollowPackRadius) {}

void TESEscortPackageData::Copy(TESPackageData* apCopy) {
    TESEscortPackageData* pcopy = dynamic_cast<TESEscortPackageData*>(apCopy);
    if (pcopy) {
        if (pSearchLocation) {
            delete pSearchLocation;
            pSearchLocation = 0;
        }
        if (pcopy->pSearchLocation) {
            PackageLocation* v2;
            PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
            if (v4) {
                v2 = ::new (v4) PackageLocation;
            }
            else {
                v2 = 0;
            }
            pSearchLocation = v2;
            pSearchLocation->Copy(pcopy->pSearchLocation, 0);
        }
        iFollowPackRadius = pcopy->iFollowPackRadius;
    }
}

bool TESEscortPackageData::Compare(TESPackageData* apCompare) {
    TESEscortPackageData* v4 = dynamic_cast<TESEscortPackageData*>(apCompare);
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
    return (bool)((uint8_t)(iFollowPackRadius != v4->iFollowPackRadius));
}

void TESEscortPackageData::Save() {
    PACK_ESCORT_DATA escStruct;
    ourMemset(&escStruct, 0, sizeof(escStruct));
    escStruct.iFollowPackRadius = iFollowPackRadius;
    if (TESForm::QEndianSwapOnSave()) {
        escStruct.Endian();
    }
    TESForm::__AddChunkData(PKES_ID, &escStruct, 4u);
    if (TESForm::QEndianSwapOnSave()) {
        escStruct.Endian();
    }
    if (pSearchLocation) {
        pSearchLocation->Save(PLD2_ID);
    }
}

// The follow data shares this entry. The radius, then a byte saying whether a location follows and the location.
void TESEscortPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFollowPackRadius), 4, 0);
    uint8_t bHasLocation = pSearchLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), 1, 0);
    if (bHasLocation) {
        pSearchLocation->SaveGame(apSaveGameBuffer);
    }
}

void TESEscortPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bhasLocation;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFollowPackRadius), 4u);
    bhasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bhasLocation), 1u);
    if (bhasLocation) {
        PackageLocation* v2;
        PackageLocation* v4 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
        if (v4) {
            v2 = ::new (v4) PackageLocation;
        }
        else {
            v2 = 0;
        }
        pSearchLocation = v2;
        pSearchLocation->LoadGame(apLoadGameBuffer);
    }
}

void TESEscortPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pSearchLocation) {
        pSearchLocation->InitLoadGame(apLoadGameBuffer);
    }
}

TESEscortPackageData::~TESEscortPackageData() {
    delete pSearchLocation;
}
