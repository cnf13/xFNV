#include "Fallout Shared/TESClass.h"

#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
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
#include "xfnv/Compiler.h"

// The service flags first, then the tag skills; the class flags are left alone.
void CLASS_DATA::Endian() {
    BSEndian(iServiceFlags, false);
    for (int32_t& eTagSkill : eTagSkills) {
        BSEndian(eTagSkill, false);
    }
}

// The service flags first, then the three tag skills; the class flags are deliberately left alone.
void OLD_CLASS_DATA::Endian() {
    BSEndian(iServiceFlags, false);
    for (int32_t& eTagSkill : eTagSkills) {
        BSEndian(eTagSkill, false);
    }
}

TESClass::TESClass() {
    SetFormType(CLAS_ID);
    TESClass::InitializeData();
}

TESClass::~TESClass() {}

void TESClass::InitializeData() {
    ourMemset(&data, 0, sizeof(data));
    for (int32_t i = 0; i < 4; ++i) {
        SetMajorSkill(i, ActorValue::VALUE_NONE);
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case DATA_ID:
                if (apFile->GetChunkSize() == GetDataSizeAdded() + 0x1C) {
                    LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
                    if (apFile->QEndian()) {
                        data.Endian();
                    }
                }
                else if (apFile->GetChunkSize() == 0x18 || apFile->GetChunkSize() == GetDataSizeAdded() + 0x18) {
                    // The older, three-tag-skill layout: read it into an OLD_CLASS_DATA and copy the fields across.
                    OLD_CLASS_DATA oldData;
                    if (apFile->GetChunkSize() == GetDataSizeAdded() + 0x18) {
                        LoadData(apFile, reinterpret_cast<char*>(&oldData), sizeof(oldData));
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
    char cAttributes[8];
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
    TESAttributes::Save(cAttributes);
    TESForm::AddChunkArray(ATTR_ID, cAttributes, 7u);
    CloseForm();
}

void TESClass::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    BGSChangeFlags kChangeFlags;
    TESForm::SaveGame(apSaveGameBuffer);
    kChangeFlags = apSaveGameBuffer->GetChangeFlags();
    if (kChangeFlags.CheckFlags(CHANGE_CLASS_TAG_SKILLS)) {
        for (int32_t& eTagSkill : data.eTagSkills) {
            apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTagSkill), sizeof(eTagSkill), 0);
        }
    }
}

// Loads the base form's changes, then the four tag skills when the save changed them.
void TESClass::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CLASS_TAG_SKILLS)) {
        // The index is compared unsigned.
        for (int32_t& eTagSkill : data.eTagSkills) {
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eTagSkill), sizeof(eTagSkill));
        }
    }
}

// Only calls TESForm's empty stub, so the body is empty.
void TESClass::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {}

void TESClass::Copy(TESForm* apCopy) {
    TESClass* cl = dynamic_cast<TESClass*>(apCopy);
    if (cl) {
        CopyAllComponents(apCopy);
        ourMemcpy(&data, &cl->data, sizeof(data));
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
        data.cTrainingSkill = static_cast<int8_t>(aeSkill - ActorValue::SKILL_START);
    }
}

bool TESClass::IsTagSkill(ActorValue::Index aiSkill) {
    return data.eTagSkills[0] == aiSkill ||
        data.eTagSkills[1] == aiSkill ||
        data.eTagSkills[2] == aiSkill ||
        data.eTagSkills[3] == aiSkill;
}

bool TESClass::IsGuard() {
    return (data.cClassFlags & CLASS_DATA::FLAG_GUARD) != 0;
}

void TESClass::SetMajorSkill(int32_t aiIndex, ActorValue::Index aeSkill) {
    if (ActorValue::IsSkill(aeSkill) || aeSkill == ActorValue::VALUE_NONE) {
        data.eTagSkills[aiIndex] = aeSkill;
        AddChange(CHANGE_CLASS_TAG_SKILLS);
    }
}

ActorValue::Index TESClass::GetTrainingSkill() {
    return static_cast<ActorValue::Index>(static_cast<uint8_t>(data.cTrainingSkill) + ActorValue::SKILL_START);
}

bool TESClass::CheckData() {
    int32_t iTrainingSkill = GetTrainingSkill();
    if (ActorValue::IsSkill(static_cast<ActorValue::Index>(iTrainingSkill))) {
        return true;
    }
    SetTrainingSkill(ActorValue::BARTER);
    return false;
}

void TESClass::SaveGame() {
    pSaveLoadGameOLD->SaveGameDataOLD(&data, sizeof(data));
    uint8_t cLength = 0;
    if (GetFullName()) {
        cLength = BSstrlen(GetFullName());
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cLength, sizeof(cLength));
    if (cLength) {
        uint32_t uiLength = cLength;
        const char* pcFullName = GetFullName();
        pSaveLoadGameOLD->SaveGameDataOLD(pcFullName, uiLength);
    }
    cLength = 0;
    if (GetTextureName()) {
        cLength = BSstrlen(GetTextureName());
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cLength, sizeof(cLength));
    if (cLength) {
        uint32_t uiLength = cLength;
        const char* pcTextureName = GetTextureName();
        pSaveLoadGameOLD->SaveGameDataOLD(pcTextureName, uiLength);
    }
}

void TESClass::LoadGame() {
    uint8_t cLength;
    pSaveLoadGameOLD->LoadGameDataOLD(&data, sizeof(data));
    pSaveLoadGameOLD->LoadGameDataOLD(&cLength, sizeof(cLength));
    if (cLength) {
        char acFullName[264];
        ourMemset(acFullName, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(acFullName, cLength);
        SetFullName(acFullName);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&cLength, sizeof(cLength));
    if (cLength) {
        char acTextureName[264];
        ourMemset(acTextureName, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(acTextureName, cLength);
        SetTextureName(acTextureName);
    }
}

int16_t TESClass::GetSaveSize() {
    int16_t iSize = 29;
    if (GetFullName()) {
        iSize = BSstrlen(GetFullName()) + 29;
    }
    iSize = iSize + 1;
    if (GetTextureName()) {
        iSize += BSstrlen(GetTextureName());
    }
    return iSize;
}

int32_t TESClass::GetTagSkillCount() {
    return 4;
}
