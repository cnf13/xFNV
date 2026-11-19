#include "Fallout Shared/TESLoadScreenType.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
    ENDIAN(DisplayData1.uJustification, false);
    ENDIAN(DisplayData1.uDisplayHeight, false);
    ENDIAN(DisplayData1.uDisplayPositionX, false);
    ENDIAN(DisplayData1.uDisplayPositionY, false);
    ENDIAN(DisplayData1.fDisplayRotationRads, false);
    ENDIAN(DisplayData1.uDisplayWidth, false);
    ENDIAN(DisplayData1.TextColorR, false);
    ENDIAN(DisplayData1.TextColorG, false);
    ENDIAN(DisplayData1.TextColorB, false);
    ENDIAN(DisplayData2.uDisplayHeight, false);
    ENDIAN(DisplayData2.uDisplayPositionX, false);
    ENDIAN(DisplayData2.uDisplayPositionY, false);
    ENDIAN(DisplayData2.fDisplayRotationRads, false);
    ENDIAN(DisplayData2.uDisplayWidth, false);
    ENDIAN(DisplayData2.TextColorR, false);
    ENDIAN(DisplayData2.TextColorG, false);
    ENDIAN(DisplayData2.TextColorB, false);
    ENDIAN(DisplayData2.uJustification, false);
    ENDIAN(DataType, false);
    ENDIAN(DisplayData1.eDisplayFont, false);
    ENDIAN(DisplayData2.eDisplayFont, false);
    ENDIAN(uNumStatsToDisplay, false);
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
        uint32_t iChunk = apFile->GetTESChunk();
        if (!iChunk) {
            break;
        }
        // The switch compares the record code signed.
        switch (static_cast<int32_t>(iChunk)) {
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
        (&data)->Endian();
    }
    return true;
}

void TESLoadScreenType::Save() {
    TESForm::StartForm();
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::__SaveData(&data, 0x58u);
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
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
    TESLoadScreenType* v3 = dynamic_cast<TESLoadScreenType*>(apCopy);
    if (v3) {
        ourMemcpy(&data, &v3->data, sizeof(data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESLoadScreenType::Compare(TESForm* apCompare) {
    TESLoadScreenType* v5 = dynamic_cast<TESLoadScreenType*>(apCompare);
    if (!v5) {
        return true;
    }
    return (std::memcmp(&data, &v5->data, sizeof(data)) || (((int8_t)TESForm::CompareAllComponents(v5)) != 0));
}
