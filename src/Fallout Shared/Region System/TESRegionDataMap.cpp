#include "Fallout Shared/Region System/TESRegionDataMap.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

#include <cmath>

TESRegionDataMap::TESRegionDataMap() : TESRegionData() {
    MapName.Set("Default Region Name", 0);
}

TESRegionDataMap::TESRegionDataMap(TESRegionDataMap* apSource) : TESRegionData(apSource) {
    TESRegionDataMap::SetMapName(apSource->GetMapName());
}

TESRegionDataMap::~TESRegionDataMap() {}

void TESRegionDataMap::Save() {
    char* v1;
    uint32_t lenPlusOne;
    TESRegionData::Save();
    if ((&MapName)->GetLength()) {
        lenPlusOne = (&MapName)->GetLenPlusOne();
        v1 = (&MapName)->pString;
        TESForm::AddChunkArray(RDMP_ID, v1, lenPlusOne);
    }
}

REGION_DATA_ID TESRegionDataMap::GetID() const {
    return REGION_DATA_MAP_ID;
}

TESRegionData* TESRegionDataMap::Copy() {
    void* v4 = ourMalloc(sizeof(TESRegionDataMap));
    if (v4) {
        return ::new (v4) TESRegionDataMap(this);
    }
    else {
        return nullptr;
    }
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataMap::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataMap::BlendInto(const TESRegionData* a1, uint32_t a2) {
    if (a1 && (a1->GetID() == REGION_DATA_MAP_ID && a2)) {
        // The source's downcast: GetID says a1 is map data.
        const TESRegionDataMap* pSource = static_cast<const TESRegionDataMap*>(a1);
        if (bIgnore) {
            goto LABEL_6;
        }
        if (a1->bIgnore) {
            return;
        }
        if (bOverride) {
            if (a1->bOverride) {
                int32_t iPriority = a1->cPriority;
                if (iPriority > cPriority) {
                    SetOverride(a1->bOverride);
                    uint8_t v6 = a1->cPriority;
                    SetPriority(v6);
                    SetMapName(pSource->GetMapName());
                }
            }
            return;
        }
        if (a1->bOverride) {
        LABEL_6:;
            SetOverride(a1->bOverride);
            uint8_t v3 = a1->cPriority;
            SetPriority(v3);
            SetMapName(pSource->GetMapName());
            return;
        }
        int32_t v8 = a1->cPriority;
        if (v8 > cPriority) {
            SetMapName(pSource->GetMapName());
        }
        int32_t v10 = cPriority;
        int32_t v11 = cPriority * v10;
        int32_t v12 = 100 - cPriority;
        float v18 = static_cast<float>(a1->cPriority * v12 + v11);
        int32_t v13 = 100 - a1->cPriority;
        int32_t v14 = cPriority * v13;
        int32_t v15 = a1->cPriority;
        float v19 = static_cast<double>(a1->cPriority * v15 + v14) + v18;
        float fnewPriority = v19 * 0.004999999888241291;
        int32_t v16 = static_cast<int32_t>(std::floor(fnewPriority));
        SetPriority(v16);
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
    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
    apFile->GetChunkData(pBuffer, 0);
    MapName.Set(pBuffer, 0);
    return true;
}

// The map name's text.
const char* TESRegionDataMap::GetMapName() const {
    return MapName.pString;
}

void TESRegionDataMap::GetMapName(BSStringT<char>* a2) const {
    char* v2 = MapName.pString;
    a2->Set(v2, (uint32_t)0);
}

bool TESRegionDataMap::SetMapName(const char* apMapName) {
    if (!(&MapName)->StrCmp(apMapName, true)) {
        return false;
    }
    (&MapName)->Set(apMapName, (uint32_t)0);
    return true;
}
