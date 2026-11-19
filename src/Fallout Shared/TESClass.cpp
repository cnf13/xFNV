#include "Fallout Shared/TESClass.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The service flags first, then the tag skills; the class flags are left alone.
void CLASS_DATA::Endian() {
    ENDIAN(iServiceFlags, false);
    for (int32_t& eTagSkill : eTagSkills) {
        ENDIAN(eTagSkill, false);
    }
}

// The service flags first, then the three tag skills; the class flags are deliberately left alone, as the game does.
void OLD_CLASS_DATA::Endian() {
    ENDIAN(iServiceFlags, false);
    for (int32_t& eTagSkill : eTagSkills) {
        ENDIAN(eTagSkill, false);
    }
}

TESClass::TESClass() {
    SetFormType(CLAS_ID);
    TESClass::InitializeData();
}

TESClass::~TESClass() {}

void TESClass::InitializeData() {
    int32_t a2;
    ourMemset(&data, 0, sizeof(data));
    for (a2 = 0; a2 < 4; ++a2) {
        SetMajorSkill(a2, static_cast<ActorValue::Index>((-1)));
    }
}

// An empty EDID chunk (malformed data) would leave the buffer unwritten, so one extra byte is reserved and used as a
// terminator.
bool TESClass::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 7) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size, read up to 0x200.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case DATA_ID:
                if (apFile->GetChunkSize() == (GetDataSizeAdded() + 0x1C)) {
                    LoadData(apFile, reinterpret_cast<char*>(&data), 0x1C);
                    if (apFile->QEndian()) {
                        data.Endian();
                    }
                }
                else if (apFile->GetChunkSize() == 0x18 || apFile->GetChunkSize() == (GetDataSizeAdded() + 0x18)) {
                    // The older, three-tag-skill layout: read it into an OLD_CLASS_DATA and copy the fields across.
                    OLD_CLASS_DATA oldData;
                    if (apFile->GetChunkSize() == (GetDataSizeAdded() + 0x18)) {
                        LoadData(apFile, reinterpret_cast<char*>(&oldData), 0x18);
                    }
                    else {
                        apFile->GetChunkData(&oldData, 0x18);
                    }
                    if (apFile->QEndian()) {
                        oldData.Endian();
                    }
                    data.cClassFlags = oldData.cClassFlags;
                    data.iServiceFlags = oldData.iServiceFlags;
                    data.cTrainingLevel = oldData.cTrainingLevel;
                    data.cTrainingSkill = oldData.cTrainingSkill;
                    for (int32_t i = 0; i < 3; ++i) {
                        data.eTagSkills[i] = oldData.eTagSkills[i];
                    }
                }
                else {
                    apFile->GetChunkData(&data, 0x1C);
                    if (apFile->QEndian()) {
                        data.Endian();
                    }
                }
                break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case ATTR_ID: {
                char cAttributes[7];
                apFile->GetChunkData(cAttributes, 7);
                static_cast<TESAttributes*>(this)->Load(cAttributes);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    if (!CheckData()) {
        Warning(
            "MASTERFILE: Class %s contains invalid data. Make sure all attributes and skills are unique.",
            GetFormEditorID()
        );
    }
    return true;
}

void TESClass::Save() {
    char apData[8];
    StartForm();
    TESFullName::Save();
    TESDescription::Save();
    TESTexture::Save(ICON_ID);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(DATA_ID, &data, 28u);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESAttributes::Save(apData);
    TESForm::AddChunkArray(ATTR_ID, apData, 7u);
    CloseForm();
}

void TESClass::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    BGSChangeFlags result;
    TESForm::SaveGame(apSaveGameBuffer);
    BGSChangeFlags* v2 = &(result = apSaveGameBuffer->GetChangeFlags());
    if (v2->CheckFlags(2)) {
        for (int32_t& eTagSkill : data.eTagSkills) {
            apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTagSkill), 4u, 0);
        }
    }
}

// Loads the base form's changes, then the four tag skills when the save changed them (change flag 2).
void TESClass::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(2)) {
        // The index is compared unsigned.
        for (int32_t& eTagSkill : data.eTagSkills) {
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eTagSkill), 4);
        }
    }
}

// Only calls TESForm's empty stub, so the body is empty.
void TESClass::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {}

void TESClass::Copy(TESForm* apCopy) {
    TESClass* cl = dynamic_cast<TESClass*>(apCopy);
    if (cl) {
        CopyAllComponents(apCopy);
        CLASS_DATA* pSourceData = &cl->data;
        ourMemcpy(&data, pSourceData, sizeof(data));
    }
}

bool TESClass::Compare(TESForm* apCompare) {
    TESClass* pComp = dynamic_cast<TESClass*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    return std::memcmp(&data, &pComp->data, sizeof(data)) != 0;
}

void TESClass::SetTrainingSkill(ActorValue::Index aeSkill) {
    if (ActorValue::IsSkill(aeSkill)) {
        data.cTrainingSkill = (int8_t)(((int32_t)aeSkill) - 32);
    }
}

bool TESClass::IsTagSkill(ActorValue::Index aiSkill) {
    return (bool)((uint8_t)((data.eTagSkills[0] == ((int32_t)aiSkill)) ||
                            ((data.eTagSkills[1] == ((int32_t)aiSkill)) ||
                             ((data.eTagSkills[2] == ((int32_t)aiSkill)) ||
                              (data.eTagSkills[3] == ((int32_t)aiSkill))))));
}

bool TESClass::IsGuard() {
    return ((data.cClassFlags & 2) != 0);
}

void TESClass::SetMajorSkill(int32_t aiIndex, ActorValue::Index aeSkill) {
    if (ActorValue::IsSkill(aeSkill) || aeSkill == ActorValue::VALUE_NONE) {
        data.eTagSkills[aiIndex] = aeSkill;
        AddChange(2);
    }
}

ActorValue::Index TESClass::GetTrainingSkill() {
    return static_cast<ActorValue::Index>(((uint8_t)data.cTrainingSkill) + 32);
}

bool TESClass::CheckData() {
    int8_t v4 = 1;
    int32_t trainingSkill = GetTrainingSkill();
    if (ActorValue::IsSkill(static_cast<ActorValue::Index>(trainingSkill))) {
        return (bool)v4;
    }
    SetTrainingSkill(ActorValue::BARTER);
    return false;
}

void TESClass::SaveGameTES() {
    uint8_t apData;
    pSaveLoadGameOLD->SaveGameDataOLD(&data, 28u);
    apData = 0;
    if (GetFullName()) {
        apData = BSstrlen(GetFullName());
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&apData, 1u);
    if (apData) {
        uint32_t v3 = apData;
        const char* FullName = GetFullName();
        pSaveLoadGameOLD->SaveGameDataOLD(FullName, v3);
    }
    apData = 0;
    if (GetTextureName()) {
        apData = BSstrlen(GetTextureName());
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&apData, 1u);
    if (apData) {
        uint32_t v4 = apData;
        const char* TextureName = GetTextureName();
        pSaveLoadGameOLD->SaveGameDataOLD(TextureName, v4);
    }
}

void TESClass::LoadGameTES() {
    uint8_t apData;
    pSaveLoadGameOLD->LoadGameDataOLD(&data, 28u);
    pSaveLoadGameOLD->LoadGameDataOLD(&apData, 1u);
    if (apData) {
        char apDest[264];
        ourMemset(apDest, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(apDest, apData);
        SetFullName(apDest);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&apData, 1u);
    if (apData) {
        char apModel[264];
        ourMemset(apModel, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(apModel, apData);
        SetTextureName(apModel);
    }
}

int16_t TESClass::GetSaveSize() {
    int16_t v3 = 29;
    if (GetFullName()) {
        v3 = BSstrlen(GetFullName()) + 29;
    }
    int16_t v4 = v3 + 1;
    if (GetTextureName()) {
        v4 += BSstrlen(GetTextureName());
    }
    return v4;
}

int32_t TESClass::GetTagSkillCount() {
    return 4;
}
