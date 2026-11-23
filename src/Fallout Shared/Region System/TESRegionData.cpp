#include "Fallout Shared/Region System/TESRegionData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

void REGION_DATA_BASE::Endian() {
    ENDIAN(iDataTypeID, false);
}

TESRegionData::TESRegionData() : bOverride(false), bIgnore(false), cPriority(50) {}

TESRegionData::TESRegionData(TESRegionData* arData)
  : bOverride(arData->bOverride), bIgnore(arData->bIgnore), cPriority(arData->cPriority) {}

TESRegionData::~TESRegionData() {}

// The header is zeroed first, so its two padding bytes are written as zero; it is swapped for the write and back.
void TESRegionData::Save() {
    REGION_DATA_BASE kData;
    ourMemset(&kData, 0, sizeof(REGION_DATA_BASE));
    kData.iDataTypeID = GetID();
    kData.bOverride = GetOverride();
    kData.cPriority = GetPriority();
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(RDAT_ID, &kData, sizeof(REGION_DATA_BASE));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
}

bool TESRegionData::LoadRegionData(REGION_DATA_BASE* apData) {
    if (!apData) {
        return false;
    }
    bOverride = apData->bOverride;
    SetPriority(apData->cPriority);
    return true;
}

bool TESRegionData::SetPriority(const uint8_t acPriority) {
    if (acPriority > 0x64u) {
        return false;
    }
    cPriority = acPriority;
    return true;
}

bool TESRegionData::GetOverride() {
    return bOverride;
}

uint8_t TESRegionData::GetPriority() {
    return cPriority;
}

void TESRegionData::operator delete(void* apMemory) {
    ourFree(apMemory);
}
