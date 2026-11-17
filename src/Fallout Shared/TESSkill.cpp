#include "Fallout Shared/TESSkill.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTexture.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

static SettingT<GameSettingCollection> sSpecNameCombat("sSpecNameCombat", "Combat");
static SettingT<GameSettingCollection> sSpecNameMagic("sSpecNameMagic", "Magic");
static SettingT<GameSettingCollection> sSpecNameStealth("sSpecNameStealth", "Stealth");
static SettingT<GameSettingCollection>
    sNoviceSkillLevelText("sNoviceSkillLevelText", "You have a basic understanding of this skill.");
// Two empty strings built at start-up; nothing reads them.
static BSStringT<char> sUnreadString0;
static BSStringT<char> sUnreadString1;

void SKILL_DATA::Endian() {
    ENDIAN(eIndex, false);
    ENDIAN(eAttribute, false);
    ENDIAN(eSpecialization, false);
    for (int32_t i = 0; i < 2; ++i) {
        ENDIAN(fActionValue[i], false);
    }
}

TESSkill::TESSkill() : TESForm(), TESDescription(), TESTexture() {
    SetFormType(0xFu);
    TESSkill::InitializeData();
}

TESSkill::~TESSkill() {}

void TESSkill::InitializeData() {
    ResetData();
}

// SKIL. The four rank descriptions are read by their own codes; DATA's byte swap happens after the case, on the
// chunk the switch read.
bool TESSkill::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != SKIL_ID) {
        return false;
    }
    LoadForm(apFile);
    for (;;) {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case DATA_ID:
                // The DATA chunk is a raw 0x14-byte buffer.
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&kData)), 0x14);
                break;
            case DESC_ID: TESDescription::LoadDescriptionData(this, apFile); break;
            case OBND_ID: LoadObjectBound(apFile); break;
            case ANAM_ID: TESDescription::LoadDescriptionData(&kDescriptions[0], apFile); break;
            case JNAM_ID: TESDescription::LoadDescriptionData(&kDescriptions[1], apFile); break;
            case ENAM_ID: TESDescription::LoadDescriptionData(&kDescriptions[2], apFile); break;
            case MNAM_ID: TESDescription::LoadDescriptionData(&kDescriptions[3], apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            default: break;
        }
        if (uiChunk == DATA_ID && apFile->QEndian()) {
            kData.Endian();
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

// Writes the INDX, description, icon and DATA chunks, then the four rank descriptions.
void TESSkill::Save() {
    StartForm();
    AddChunk(INDX_ID, static_cast<uint32_t>(kData.eIndex));
    TESDescription::Save();
    TESTexture::Save(ICON_ID);
    if (QEndianSwapOnSave()) {
        kData.Endian();
    }
    __SaveData(&kData, 20);
    if (QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESDescription::SaveDescriptionData(&kDescriptions[0], ANAM_ID);
    TESDescription::SaveDescriptionData(&kDescriptions[1], JNAM_ID);
    TESDescription::SaveDescriptionData(&kDescriptions[2], ENAM_ID);
    TESDescription::SaveDescriptionData(&kDescriptions[3], MNAM_ID);
    CloseForm();
}

void TESSkill::Copy(TESForm* apCopy) {
    TESSkill* pSk = dynamic_cast<TESSkill*>(apCopy);
    if (pSk) {
        CopyAllComponents(apCopy);
        ourMemcpy(&kData, &pSk->kData, sizeof(kData));
        for (int32_t i = 0; i < 4; ++i) {
            kDescriptions[i].CopyComponent(&pSk->kDescriptions[i]);
        }
    }
}

bool TESSkill::Compare(TESForm* apCompare) {
    TESSkill* pComp = dynamic_cast<TESSkill*>(apCompare);
    if (!pComp) {
        return 1;
    }
    if (CompareAllComponents(pComp)) {
        return 1;
    }
    void* FrustumPlanes = &pComp->kData;
    if (std::memcmp(&kData, FrustumPlanes, sizeof(kData))) {
        return 1;
    }
    for (int32_t i = 0; i < 4; ++i) {
        if (kDescriptions[i].CompareComponent(&pComp->kDescriptions[i])) {
            return 1;
        }
    }
    return 0;
}

void TESSkill::ResetData() {
    int32_t j;
    int32_t i;
    kData.eIndex = static_cast<ActorValue::Index>(-1);
    kData.eSpecialization = SPECIALIZATION_COMBAT;
    kData.eAttribute = ActorValue::STRENGTH;
    for (i = 0; i < 2; ++i) {
        kData.fActionValue[i] = (float)1.0;
    }
    for (j = 0; j < 4; ++j) {
        (&kDescriptions[j])->InitializeDataComponent();
    }
    SetAltered((bool)0);
    TESForm::ClearFiles();
}
