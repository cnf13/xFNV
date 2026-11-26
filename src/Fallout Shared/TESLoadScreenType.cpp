#include "Fallout Shared/TESLoadScreenType.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Font.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "xfnv/Compiler.h"

void DisplayData::Init() {
    uDisplayPositionX = 0;
    uDisplayPositionY = 0;
    uDisplayWidth = 0;
    uDisplayHeight = 0;
    fDisplayRotationRads = 0.0;
    eDisplayFont = FONT_7;
    TextColorR = 255.0f;
    TextColorG = 255.0f;
    TextColorB = 255.0f;
    uJustification = 1;
}

// The first window's fields in the game's order (justification first, font last), then the second's (justification
// last), then the data type, the two fonts and the stat count.
void LoadScreenType_Data::Endian() {
    BSEndian(DisplayData1.uJustification, false);
    BSEndian(DisplayData1.uDisplayHeight, false);
    BSEndian(DisplayData1.uDisplayPositionX, false);
    BSEndian(DisplayData1.uDisplayPositionY, false);
    BSEndian(DisplayData1.fDisplayRotationRads, false);
    BSEndian(DisplayData1.uDisplayWidth, false);
    BSEndian(DisplayData1.TextColorR, false);
    BSEndian(DisplayData1.TextColorG, false);
    BSEndian(DisplayData1.TextColorB, false);
    BSEndian(DisplayData2.uDisplayHeight, false);
    BSEndian(DisplayData2.uDisplayPositionX, false);
    BSEndian(DisplayData2.uDisplayPositionY, false);
    BSEndian(DisplayData2.fDisplayRotationRads, false);
    BSEndian(DisplayData2.uDisplayWidth, false);
    BSEndian(DisplayData2.TextColorR, false);
    BSEndian(DisplayData2.TextColorG, false);
    BSEndian(DisplayData2.TextColorB, false);
    BSEndian(DisplayData2.uJustification, false);
    BSEndian(DataType, false);
    BSEndian(DisplayData1.eDisplayFont, false);
    BSEndian(DisplayData2.eDisplayFont, false);
    BSEndian(uNumStatsToDisplay, false);
}

// The second window's justification and the stat count are set last.
LoadScreenType_Data* LoadScreenType_Data::Init() {
    DisplayData1.Init();
    DisplayData2.Init();
    DataType = LSDT_NONE;
    DisplayData2.uJustification = 4;
    uNumStatsToDisplay = 3;
    return this;
}

TESLoadScreenType::TESLoadScreenType() : TESForm() {
    data.Init();
    SetFormType(0x6Eu);
}

TESLoadScreenType::~TESLoadScreenType() {}

// The LSCT record: its data, read big-endian from the file. No form type is read here.
bool TESLoadScreenType::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (!uiChunk) {
            break;
        }
        // The switch compares the record code signed.
        switch (static_cast<int32_t>(uiChunk)) {
            case 0x41544144: // 'DATA'
                // LoadData copies the chunk's raw bytes over the struct
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(LoadScreenType_Data));
                break;
            case 0x44494445: { // 'EDID'
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case 0x444E424F: // 'OBND'
                LoadObjectBound(apFile);
                break;
        }
    }
    while (apFile->NextChunk());
    if (apFile->QEndian()) {
        data.Endian();
    }
    return true;
}

void TESLoadScreenType::Save() {
    TESForm::StartForm();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::CloseForm();
}

uint16_t TESLoadScreenType::GetSaveSize(uint32_t aiFlags) {
    return TESForm::GetSaveSize(aiFlags);
}

void TESLoadScreenType::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
}

void TESLoadScreenType::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
}

void TESLoadScreenType::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESLoadScreenType::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
}

void TESLoadScreenType::Copy(TESForm* apCopy) {
    TESLoadScreenType* pLoadScreenType = dynamic_cast<TESLoadScreenType*>(apCopy);
    if (pLoadScreenType) {
        ourMemcpy(&data, &pLoadScreenType->data, sizeof(data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESLoadScreenType::Compare(TESForm* apCompare) {
    TESLoadScreenType* pLoadScreenType = dynamic_cast<TESLoadScreenType*>(apCompare);
    if (!pLoadScreenType) {
        return true;
    }
    return std::memcmp(&data, &pLoadScreenType->data, sizeof(data)) || TESForm::CompareAllComponents(pLoadScreenType);
}
