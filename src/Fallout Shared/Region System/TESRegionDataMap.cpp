#include "Fallout Shared/Region System/TESRegionDataMap.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "xfnv/Compiler.h"

#include <cmath>

TESRegionDataMap::TESRegionDataMap() : TESRegionData() {
    MapName.Set("Default Region Name", 0);
}

TESRegionDataMap::TESRegionDataMap(TESRegionDataMap* apSource) : TESRegionData(apSource) {
    TESRegionDataMap::SetMapName(apSource->GetMapName());
}

TESRegionDataMap::~TESRegionDataMap() {}

void TESRegionDataMap::Save() {
    TESRegionData::Save();
    if (MapName.GetLength()) {
        uint32_t uiLenPlusOne = MapName.GetLenPlusOne();
        TESForm::AddChunkArray(RDMP_ID, MapName.pString, uiLenPlusOne);
    }
}

REGION_DATA_ID TESRegionDataMap::GetID() const {
    return REGION_DATA_MAP_ID;
}

TESRegionData* TESRegionDataMap::Copy() {
    return new TESRegionDataMap(this);
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataMap::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataMap::BlendInto(const TESRegionData* apRegionData, uint32_t aiTotalBlending) {
    if (apRegionData && apRegionData->GetID() == REGION_DATA_MAP_ID && aiTotalBlending) {
        // The source's downcast: GetID says apRegionData is map data.
        const TESRegionDataMap* pSource = static_cast<const TESRegionDataMap*>(apRegionData);
        if (!bIgnore) {
            if (apRegionData->bIgnore) {
                return;
            }
            if (bOverride) {
                if (apRegionData->bOverride) {
                    int32_t iPriority = apRegionData->cPriority;
                    if (iPriority > cPriority) {
                        SetOverride(apRegionData->bOverride);
                        SetPriority(apRegionData->cPriority);
                        SetMapName(pSource->GetMapName());
                    }
                }
                return;
            }
            if (!apRegionData->bOverride) {
                int32_t iPriority = apRegionData->cPriority;
                if (iPriority > cPriority) {
                    SetMapName(pSource->GetMapName());
                }
                int32_t iOwnSquare = cPriority * cPriority;
                int32_t iOwnRemainder = 100 - cPriority;
                float fOwnWeight = static_cast<float>(apRegionData->cPriority * iOwnRemainder + iOwnSquare);
                int32_t iSourceRemainder = 100 - apRegionData->cPriority;
                int32_t iSourceCross = cPriority * iSourceRemainder;
                float fTotalWeight = static_cast<double>(
                                         apRegionData->cPriority * apRegionData->cPriority + iSourceCross
                                     ) +
                    fOwnWeight;
                float fnewPriority = fTotalWeight * 0.004999999888241291;
                int32_t iNewPriority = static_cast<int32_t>(std::floor(fnewPriority));
                SetPriority(iNewPriority);
                return;
            }
        }
        SetOverride(apRegionData->bOverride);
        SetPriority(apRegionData->cPriority);
        SetMapName(pSource->GetMapName());
    }
}

bool TESRegionDataMap::Validate() {
    return true;
}

bool TESRegionDataMap::LoadRegionDataMap(TESFile* apFile) {
    if (!apFile || apFile->GetTESChunk() != RDMP_ID) {
        return false;
    }
    // A 16-byte aligned stack buffer of the chunk's size.
    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
    apFile->GetChunkData(pBuffer, 0);
    MapName.Set(pBuffer, 0);
    return true;
}

// The map name's text.
const char* TESRegionDataMap::GetMapName() const {
    return MapName.pString;
}

void TESRegionDataMap::GetMapName(BSStringT<char>* apMapName) const {
    apMapName->Set(MapName.pString, 0);
}

bool TESRegionDataMap::SetMapName(const char* apMapName) {
    if (!MapName.StrCmp(apMapName, true)) {
        return false;
    }
    MapName.Set(apMapName, 0);
    return true;
}
