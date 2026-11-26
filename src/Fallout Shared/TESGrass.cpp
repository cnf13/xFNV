#include "Fallout Shared/TESGrass.h"

#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

SettingT<INISettingCollection> TESGrass::fWaveOffsetRange("fWaveOffsetRange:Grass", 1.75f);

// The three slope and density bytes and the flags byte need no swap.
void TESGrass::GRASS_DATA::Endian() {
    BSEndian(sDistanceFromWaterLevel, false);
    BSEndian(eUnderwater, false);
    BSEndian(fPositionRange, false);
    BSEndian(fHeightRange, false);
    BSEndian(fColorRange, false);
    BSEndian(fWavePeriod, false);
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case DATA_ID: LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(Data)); break;
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
        Data.Endian();
    }
    TESForm::__SaveData(&Data, sizeof(Data));
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::CloseForm();
}

void TESGrass::InitItem() {
    if (!TESForm::GetInitialized()) {
        TESForm::SetInitialized(true);
    }
}

void TESGrass::Copy(TESForm* apCopy) {
    TESGrass* pCopy = dynamic_cast<TESGrass*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        ourMemcpy(&Data, &pCopy->Data, sizeof(Data));
    }
}

bool TESGrass::Compare(TESForm* apCompare) {
    TESGrass* pCompare = dynamic_cast<TESGrass*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (TESForm::CompareAllComponents(pCompare)) {
        return true;
    }
    int32_t iDensity = GetDensity();
    if (iDensity != pCompare->GetDensity()) {
        return true;
    }
    int32_t iMinSlopeDegrees = GetMinSlopeDegrees();
    if (iMinSlopeDegrees != pCompare->GetMinSlopeDegrees()) {
        return true;
    }
    int32_t iMaxSlopeDegrees = GetMaxSlopeDegrees();
    if (iMaxSlopeDegrees != pCompare->GetMaxSlopeDegrees()) {
        return true;
    }
    int32_t iDistanceFromWaterLevel = GetDistanceFromWaterLevel();
    if (iDistanceFromWaterLevel != pCompare->GetDistanceFromWaterLevel()) {
        return true;
    }
    uint32_t uiUnderwaterState = GetUnderwaterState();
    if (uiUnderwaterState != static_cast<uint32_t>(pCompare->GetUnderwaterState())) {
        return true;
    }
    double dPositionRange = GetPositionRange();
    if (dPositionRange != pCompare->GetPositionRange()) {
        return true;
    }
    double dHeightRange = GetHeightRange();
    if (dHeightRange != pCompare->GetHeightRange()) {
        return true;
    }
    double dColorRange = GetColorRange();
    if (dColorRange != pCompare->GetColorRange()) {
        return true;
    }
    double dWavePeriod = GetWavePeriod();
    if (dWavePeriod != pCompare->GetWavePeriod()) {
        return true;
    }
    int32_t iVertexLighting = GetVertexLighting();
    if (iVertexLighting != pCompare->GetVertexLighting()) {
        return true;
    }
    int32_t iUniformScaling = GetUniformScaling();
    if (iUniformScaling != pCompare->GetUniformScaling()) {
        return true;
    }
    int32_t iFitToSlope = GetFitToSlope();
    return iFitToSlope != pCompare->GetFitToSlope();
}

uint8_t TESGrass::GetDensity() {
    return static_cast<uint8_t>(Data.cDensity);
}

bool TESGrass::SetDensity(uint8_t acDensity) {
    if (acDensity > 0x64u) {
        return false;
    }
    Data.cDensity = static_cast<int8_t>(acDensity);
    return true;
}

uint8_t TESGrass::GetMinSlopeDegrees() {
    return static_cast<uint8_t>(Data.cMinSlopeDegrees);
}

bool TESGrass::SetMinSlopeDegrees(uint8_t acMinSlopeDegrees) {
    if (acMinSlopeDegrees > 0x5Au || acMinSlopeDegrees > static_cast<uint8_t>(Data.cMaxSlopeDegrees)) {
        return false;
    }
    Data.cMinSlopeDegrees = static_cast<int8_t>(acMinSlopeDegrees);
    return true;
}

uint8_t TESGrass::GetMaxSlopeDegrees() {
    return static_cast<uint8_t>(Data.cMaxSlopeDegrees);
}

bool TESGrass::SetMaxSlopeDegrees(uint8_t acMaxSlopeDegrees) {
    if (acMaxSlopeDegrees > 0x5Au || acMaxSlopeDegrees < static_cast<uint8_t>(Data.cMinSlopeDegrees)) {
        return false;
    }
    Data.cMaxSlopeDegrees = static_cast<int8_t>(acMaxSlopeDegrees);
    return true;
}

float TESGrass::GetMinSlope() {
    return static_cast<float>(static_cast<uint8_t>(Data.cMinSlopeDegrees) * 0.01745329238474369);
}

// cMaxSlopeDegrees is read as unsigned despite its signed field type, before the multiply by the
// double DEGREES_2_RADIAN, and the product is rounded to a float local.
float TESGrass::GetMaxSlope() {
    float fResult = static_cast<float>(static_cast<uint8_t>(Data.cMaxSlopeDegrees) * 0.01745329238474369);
    return fResult;
}

uint16_t TESGrass::GetDistanceFromWaterLevel() {
    return static_cast<uint16_t>(Data.sDistanceFromWaterLevel);
}

void TESGrass::SetDistanceFromWaterLevel(uint16_t asDistanceFromWaterLevel) {
    Data.sDistanceFromWaterLevel = static_cast<int16_t>(asDistanceFromWaterLevel);
}

TESGrass::GRASS_WATER_STATE TESGrass::GetUnderwaterState() {
    return Data.eUnderwater;
}

float TESGrass::GetPositionRange() {
    return Data.fPositionRange;
}

bool TESGrass::SetPositionRange(float afRange) {
    if (!(afRange >= 0.0) || !(afRange <= 512.0)) {
        return false;
    }
    Data.fPositionRange = afRange;
    return true;
}

float TESGrass::GetHeightRange() {
    return Data.fHeightRange;
}

bool TESGrass::SetHeightRange(float afRange) {
    if (!(afRange >= 0.0) || !(afRange <= 1.0)) {
        return false;
    }
    Data.fHeightRange = afRange;
    return true;
}

float TESGrass::GetColorRange() {
    return Data.fColorRange;
}

bool TESGrass::SetColorRange(float afRange) {
    if (!(afRange >= 0.0) || !(afRange <= 1.0)) {
        return false;
    }
    Data.fColorRange = afRange;
    return true;
}

float TESGrass::GetWavePeriod() {
    return Data.fWavePeriod;
}

bool TESGrass::SetWavePeriod(float afPeriod) {
    if (!(afPeriod > 0.0) || !(afPeriod <= 100.0)) {
        return false;
    }
    Data.fWavePeriod = afPeriod;
    return true;
}

bool TESGrass::GetVertexLighting() {
    return (Data.cFlags & GRASS_DATA::VERTEX_LIGHTING) != 0;
}

void TESGrass::SetVertexLighting(bool abFlag) {
    if (abFlag) {
        Data.cFlags |= GRASS_DATA::VERTEX_LIGHTING;
    }
    else {
        Data.cFlags &= ~GRASS_DATA::VERTEX_LIGHTING;
    }
}

bool TESGrass::GetUniformScaling() {
    return (Data.cFlags & GRASS_DATA::UNIFORM_SCALING) != 0;
}

void TESGrass::SetUniformScaling(bool abFlag) {
    if (abFlag) {
        Data.cFlags |= GRASS_DATA::UNIFORM_SCALING;
    }
    else {
        Data.cFlags &= ~GRASS_DATA::UNIFORM_SCALING;
    }
}

bool TESGrass::GetFitToSlope() {
    return (Data.cFlags & GRASS_DATA::FIT_TO_SLOPE) != 0;
}

void TESGrass::SetFitToSlope(bool abFlag) {
    if (abFlag) {
        Data.cFlags |= GRASS_DATA::FIT_TO_SLOPE;
    }
    else {
        Data.cFlags &= ~GRASS_DATA::FIT_TO_SLOPE;
    }
}

char* TESGrass::GetModelPath() {
    if (cModel.pString) {
        return cModel.pString;
    }

    return const_cast<char*>("");
}

float TESGrass::GetWaveOffsetRange() {
    return TESGrass::fWaveOffsetRange.Float();
}
