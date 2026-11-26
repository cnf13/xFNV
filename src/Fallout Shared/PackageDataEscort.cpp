#include "BSCore/NewOverloads.h"
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
    BSEndian(iFollowPackRadius, false);
}

// From the package's file data: an empty search location and the radius the file data holds.
TESEscortPackageData::TESEscortPackageData(const PACK_ESCORT_DATA* apEscData)
  : pSearchLocation(nullptr), iFollowPackRadius(apEscData->iFollowPackRadius) {}

void TESEscortPackageData::Copy(TESPackageData* apCopy) {
    TESEscortPackageData* pCopy = dynamic_cast<TESEscortPackageData*>(apCopy);
    if (pCopy) {
        if (pSearchLocation) {
            delete pSearchLocation;
            pSearchLocation = nullptr;
        }
        if (pCopy->pSearchLocation) {
            pSearchLocation = new PackageLocation;
            pSearchLocation->Copy(pCopy->pSearchLocation, false);
        }
        iFollowPackRadius = pCopy->iFollowPackRadius;
    }
}

bool TESEscortPackageData::Compare(TESPackageData* apCompare) {
    TESEscortPackageData* pCompare = dynamic_cast<TESEscortPackageData*>(apCompare);
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
    return iFollowPackRadius != pCompare->iFollowPackRadius;
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFollowPackRadius), sizeof(iFollowPackRadius), 0);
    uint8_t bHasLocation = pSearchLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pSearchLocation->SaveGame(apSaveGameBuffer);
    }
}

void TESEscortPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFollowPackRadius), sizeof(iFollowPackRadius));
    bool bHasLocation = false;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pSearchLocation = new PackageLocation;
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
