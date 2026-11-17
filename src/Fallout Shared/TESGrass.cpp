#include "Fallout Shared/TESGrass.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "SDK/MSVC/rtti.h"

SettingT<INISettingCollection> TESGrass::fWaveOffsetRange("fWaveOffsetRange:Grass", 1.75f);

// The three slope and density bytes and the flags byte need no swap.
void TESGrass::GRASS_DATA::Endian() {
    ENDIAN(sDistanceFromWaterLevel, false);
    ENDIAN(eUnderwater, false);
    ENDIAN(fPositionRange, false);
    ENDIAN(fHeightRange, false);
    ENDIAN(fColorRange, false);
    ENDIAN(fWavePeriod, false);
}

TESGrass::TESGrass() : TESBoundObject(), TESModel() {
    SetFormType(GRAS_ID);
    TESGrass::InitializeData();
}

TESGrass::~TESGrass() {}

void TESGrass::InitializeData() {
    Data.cDensity = 30;
    Data.cMinSlopeDegrees = 0;
    Data.cMaxSlopeDegrees = 90;
    Data.sDistanceFromWaterLevel = 0;
    Data.eUnderwater = TESGrass::GWS_ABOVE_ONLY_ATLEAST;
    Data.fPositionRange = 0x1p+5f;
    Data.fHeightRange = 0.20000000298023224f;
    Data.fColorRange = 0x1p-1f;
    Data.fWavePeriod = 10.0f;
    Data.cFlags = 0;
}

// Clears the GRASS_DATA, then reads the chunks: the editor ID, the bounds, the model, the data (swapped in place for
// a big-endian file) and the name.
bool TESGrass::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != GRAS_ID) {
        return false;
    }
    ourMemset(&Data, 0, sizeof(Data));
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case DATA_ID: LoadData(apFile, reinterpret_cast<char*>(&Data), 0x20u); break;
        }
        if (uiChunk == DATA_ID && apFile->QEndian()) {
            Data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESGrass::Save() {
    TESForm::StartForm();
    static_cast<TESModel*>(this)->Save(MODL_ID, MODT_ID);
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    TESForm::__SaveData(&Data, 0x20u);
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    TESForm::CloseForm();
}

void TESGrass::InitItem() {
    if (!TESForm::GetInitialized()) {
        TESForm::SetInitialized(true);
    }
}

void TESGrass::Copy(TESForm* apCopy) {
    TESGrass* pcopy = dynamic_cast<TESGrass*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        void* HeadPartList = &pcopy->Data;
        void* v2 = &Data;
        ourMemcpy(v2, HeadPartList, 32u);
    }
}

bool TESGrass::Compare(TESForm* apCompare) {
    TESGrass* v16 = dynamic_cast<TESGrass*>(apCompare);
    if (!v16) {
        return true;
    }
    if (TESForm::CompareAllComponents(v16)) {
        return true;
    }
    int32_t v3 = (int32_t)GetDensity();
    if (((uint32_t)v3) != ((uint32_t)v16->GetDensity())) {
        return true;
    }
    int32_t v4 = (int32_t)GetMinSlopeDegrees();
    if (((uint32_t)v4) != ((uint32_t)v16->GetMinSlopeDegrees())) {
        return true;
    }
    int32_t v5 = (int32_t)GetMaxSlopeDegrees();
    if (((uint32_t)v5) != ((uint32_t)v16->GetMaxSlopeDegrees())) {
        return true;
    }
    int32_t v6 = (int32_t)GetDistanceFromWaterLevel();
    if (((uint32_t)v6) != ((uint32_t)v16->GetDistanceFromWaterLevel())) {
        return true;
    }
    uint32_t v7 = GetUnderwaterState();
    if (v7 != ((uint32_t)((int32_t)v16->GetUnderwaterState()))) {
        return true;
    }
    double v8 = (double)GetPositionRange();
    if (v8 != v16->GetPositionRange()) {
        return true;
    }
    double v9 = (double)GetHeightRange();
    if (v9 != v16->GetHeightRange()) {
        return true;
    }
    double v10 = (double)GetColorRange();
    if (v10 != v16->GetColorRange()) {
        return true;
    }
    double v11 = (double)GetWavePeriod();
    if (v11 != v16->GetWavePeriod()) {
        return true;
    }
    int32_t v12 = (int32_t)GetVertexLighting();
    if (((uint32_t)v12) != ((uint32_t)((uint8_t)v16->GetVertexLighting()))) {
        return true;
    }
    int32_t v13 = (int32_t)GetUniformScaling();
    if (((uint32_t)v13) != ((uint32_t)((uint8_t)v16->GetUniformScaling()))) {
        return true;
    }
    int32_t v14 = (int32_t)GetFitToSlope();
    return (bool)((uint8_t)(((uint32_t)v14) != ((uint32_t)((uint8_t)v16->GetFitToSlope()))));
}

uint8_t TESGrass::GetDensity() {
    return (uint8_t)Data.cDensity;
}

bool TESGrass::SetDensity(uint8_t acDensity) {
    if (((uint32_t)((int32_t)acDensity)) > 0x64u) {
        return false;
    }
    Data.cDensity = (int8_t)acDensity;
    return true;
}

uint8_t TESGrass::GetMinSlopeDegrees() {
    return (uint8_t)Data.cMinSlopeDegrees;
}

bool TESGrass::SetMinSlopeDegrees(uint8_t acMinSlopeDegrees) {
    if ((((uint32_t)((int32_t)acMinSlopeDegrees)) > 0x5Au) ||
        (((int32_t)acMinSlopeDegrees) > ((int32_t)((uint8_t)Data.cMaxSlopeDegrees)))) {
        return false;
    }
    Data.cMinSlopeDegrees = (int8_t)acMinSlopeDegrees;
    return true;
}

uint8_t TESGrass::GetMaxSlopeDegrees() {
    return (uint8_t)Data.cMaxSlopeDegrees;
}

bool TESGrass::SetMaxSlopeDegrees(uint8_t acMaxSlopeDegrees) {
    if ((((uint32_t)((int32_t)acMaxSlopeDegrees)) > 0x5Au) ||
        (((int32_t)acMaxSlopeDegrees) < ((int32_t)((uint8_t)Data.cMinSlopeDegrees)))) {
        return false;
    }
    Data.cMaxSlopeDegrees = (int8_t)acMaxSlopeDegrees;
    return true;
}

float TESGrass::GetMinSlope() {
    return (float)(((double)((uint8_t)Data.cMinSlopeDegrees)) * 0.01745329238474369);
}

// cMaxSlopeDegrees is read as unsigned despite its signed field type, as the game does, before the multiply by the
// double DEGREES_2_RADIAN, and the product is rounded to a float local.
float TESGrass::GetMaxSlope() {
    float fResult = static_cast<float>(static_cast<uint8_t>(Data.cMaxSlopeDegrees) * 0.01745329238474369);
    return fResult;
}

uint16_t TESGrass::GetDistanceFromWaterLevel() {
    return (uint16_t)Data.sDistanceFromWaterLevel;
}

void TESGrass::SetDistanceFromWaterLevel(uint16_t asDistanceFromWaterLevel) {
    Data.sDistanceFromWaterLevel = (int16_t)asDistanceFromWaterLevel;
}

TESGrass::GRASS_WATER_STATE TESGrass::GetUnderwaterState() {
    return Data.eUnderwater;
}

float TESGrass::GetPositionRange() {
    return Data.fPositionRange;
}

bool TESGrass::SetPositionRange(float afRange) {
    if ((!(afRange >= 0.0)) || (!(afRange <= 512.0))) {
        return false;
    }
    Data.fPositionRange = afRange;
    return true;
}

float TESGrass::GetHeightRange() {
    return Data.fHeightRange;
}

bool TESGrass::SetHeightRange(float afRange) {
    if ((!(afRange >= 0.0)) || (!(afRange <= 1.0))) {
        return false;
    }
    Data.fHeightRange = afRange;
    return true;
}

float TESGrass::GetColorRange() {
    return Data.fColorRange;
}

bool TESGrass::SetColorRange(float afRange) {
    if ((!(afRange >= 0.0)) || (!(afRange <= 1.0))) {
        return false;
    }
    Data.fColorRange = afRange;
    return true;
}

float TESGrass::GetWavePeriod() {
    return Data.fWavePeriod;
}

bool TESGrass::SetWavePeriod(float afPeriod) {
    if ((!(afPeriod > 0.0)) || (!(afPeriod <= 100.0))) {
        return false;
    }
    Data.fWavePeriod = afPeriod;
    return true;
}

bool TESGrass::GetVertexLighting() {
    return ((Data.cFlags & 1) != 0);
}

void TESGrass::SetVertexLighting(bool abFlag) {
    int8_t cFlags;
    int8_t v3;
    if (abFlag) {
        Data.cFlags |= 1u;
        cFlags = Data.cFlags;
    }
    else {
        Data.cFlags &= (~1u);
        v3 = Data.cFlags;
    }
    int8_t v4 = Data.cFlags;
}

bool TESGrass::GetUniformScaling() {
    return ((Data.cFlags & 2) != 0);
}

void TESGrass::SetUniformScaling(bool abFlag) {
    int8_t cFlags;
    int8_t v3;
    if (abFlag) {
        Data.cFlags |= 2u;
        cFlags = Data.cFlags;
    }
    else {
        Data.cFlags &= (~2u);
        v3 = Data.cFlags;
    }
    int8_t v4 = Data.cFlags;
}

bool TESGrass::GetFitToSlope() {
    return ((((uint8_t)Data.cFlags) & 4) != 0);
}

void TESGrass::SetFitToSlope(bool abFlag) {
    int8_t cFlags;
    int8_t v3;
    if (abFlag) {
        Data.cFlags |= 4u;
        cFlags = Data.cFlags;
    }
    else {
        Data.cFlags &= (~4u);
        v3 = Data.cFlags;
    }
    int8_t v4 = Data.cFlags;
}

char* TESGrass::GetModelPath() {
    if (cModel.pString) {
        return cModel.pString;
    }

    return const_cast<char*>("");
}

float TESGrass::GetWaveOffsetRange() {
    return (&TESGrass::fWaveOffsetRange)->Float();
}
