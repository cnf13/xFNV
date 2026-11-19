#include "Fallout Shared/TESCasinoChips.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

void CASINO_DATA::Endian() {
    ENDIAN(fBJ_ShufflePercent, false);
    ENDIAN(fBJ_BlackjackPayout, false);
    ENDIAN(iSlotStop_A, false);
    ENDIAN(iSlotStop_B, false);
    ENDIAN(iSlotStop_C, false);
    ENDIAN(iSlotStop_D, false);
    ENDIAN(iSlotStop_E, false);
    ENDIAN(iSlotStop_F, false);
    ENDIAN(iSlotStop_W, false);
    ENDIAN(iBJNumDecks, false);
    ENDIAN(iMaxWinnings, false);
    ENDIAN(casinoChipID, false);
    ENDIAN(questID, false);
}

TESCasinoChips::TESCasinoChips() {
    cFormType = CHIP_ID;
    TESCasinoChips::InitializeData();
}

// The destructor resets the data (InitializeData and ClearData do the same thing).
TESCasinoChips::~TESCasinoChips() {
    TESCasinoChips::InitializeData();
}

void TESCasinoChips::InitializeData() {
    (&cDesc)->Set(nullptr, (uint32_t)0);
}

void TESCasinoChips::ClearData() {
    (&cDesc)->Set(nullptr, (uint32_t)0);
}

bool TESCasinoChips::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 108) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MOSD_ID:
            case MODD_ID:
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case MODS_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read whole.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(
                    pBuffer, uiChunkSize, apFile->GetFormVersion()
                );
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(this, apFile); break;
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case YNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case ZNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case ONAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read whole (size 0).
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                cDesc.Set(pBuffer, 0);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESCasinoChips::Save() {
    char* v1;
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(this)->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESIcon*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    static_cast<BGSPickupPutdownSounds*>(this)->Save();
    if ((&cDesc)->GetLength()) {
        v2 = (&cDesc)->GetLength() + 1;
        v1 = (&cDesc)->pString;
        TESForm::AddChunkArray(ONAM_ID, v1, v2);
    }
    TESForm::CloseForm();
}

uint16_t TESCasinoChips::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESCasinoChips::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    TESValueForm::SaveGame(aiFlags);
}

void TESCasinoChips::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESCasinoChips::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESCasinoChips::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESCasinoChips::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        static_cast<TESModelTextureSwap*>(this)->InitItem(this);
        static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void TESCasinoChips::Copy(TESForm* apCopy) {
    TESCasinoChips* pCopy = dynamic_cast<TESCasinoChips*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        uint8_t FormType = pCopy->GetFormType();
        cFormType = FormType;
        cDesc.Set(pCopy->cDesc.pString, 0u);
    }
}

bool TESCasinoChips::Compare(TESForm* apCompare) {
    TESCasinoChips* pComp = dynamic_cast<TESCasinoChips*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    return cDesc.StrCmp(pComp->cDesc, true) != 0;
}

NiAVObject* TESCasinoChips::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    return TESBoundObject::Clone3D(pRequester, deepCopy);
}

TESCasino::TESCasino() {
    cFormType = CSNO_ID;
}

TESCasino::~TESCasino() {
    TESCasino::ClearData();
}

void TESCasino::InitializeData() {
    ourMemset(&data, 0, sizeof(data));
    for (TESModelTextureSwap& kModel : modelArrayList) {
        kModel.InitializeDataComponent();
    }
    for (TESTexture& kTexture : textureArrayList) {
        kTexture.InitializeDataComponent();
    }
}

void TESCasino::ClearData() {
    ourMemset(&data, 0, sizeof(data));
}

bool TESCasino::Load(TESFile* apFile) {
    // The form type is kept as a byte and only that byte is compared.
    if (static_cast<uint8_t>(apFile->GetTESForm()) != CSNO_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    // MODL fills the models from slot 0 up; MOD2, MOD3 and MOD4 are slots 7, 8 and 9. ICON fills the textures from
    // slot 0 up, ICO2 from slot 7 up.
    int32_t iModel = 0;
    int32_t iIcon = 0;
    int32_t iIcon2 = 7;
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MODL_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read whole.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                // The model is set through the virtual SetModel call, as the game does.
                TESModel* pModel = &modelArrayList[iModel];
                pModel->SetModel(pBuffer);
                TESModel::LoadModelTextureChunk(&modelArrayList[iModel], apFile);
                ++iModel;
                break;
            }
            case MOD2_ID: {
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = &modelArrayList[7];
                pModel->SetModel(pBuffer);
                TESModel::LoadModelTextureChunk(&modelArrayList[7], apFile);
                break;
            }
            case MOD3_ID: {
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = &modelArrayList[8];
                pModel->SetModel(pBuffer);
                TESModel::LoadModelTextureChunk(&modelArrayList[8], apFile);
                break;
            }
            case MOD4_ID: {
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = &modelArrayList[9];
                pModel->SetModel(pBuffer);
                TESModel::LoadModelTextureChunk(&modelArrayList[9], apFile);
                break;
            }
            case ICON_ID:
                TESTexture::LoadTextureChunk(&textureArrayList[iIcon], apFile);
                ++iIcon;
                break;
            case ICO2_ID:
                TESTexture::LoadTextureChunk(&textureArrayList[iIcon2], apFile);
                ++iIcon2;
                break;
            case DATA_ID:
                LoadData(apFile, reinterpret_cast<char*>(&data), 0x38);
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESCasino::Save() {
    StartForm();
    TESFullName::Save();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, 56u);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    for (int32_t i = 0; i < 8; ++i) {
        if (modelArrayList[i].GetModelLength()) {
            modelArrayList[i].Save(MODL_ID, MODT_ID, MODS_ID);
        }
    }
    if (modelArrayList[7].GetModelLength()) {
        modelArrayList[7].Save(MOD2_ID, MODT_ID, MODS_ID);
    }
    if (modelArrayList[8].GetModelLength()) {
        modelArrayList[8].Save(MOD3_ID, MODT_ID, MODS_ID);
    }
    if (modelArrayList[9].GetModelLength()) {
        modelArrayList[9].Save(MOD4_ID, MODT_ID, MODS_ID);
    }
    for (int32_t j = 0; j < 7; ++j) {
        textureArrayList[j].Save(ICON_ID);
    }
    for (int32_t k = 7; k < 11; ++k) {
        textureArrayList[k].Save(ICO2_ID);
    }
    CloseForm();
}

void TESCasino::InitItem() {
    TESFile* file = TESForm::GetFile((-1));
    TESForm::AddCompileIndex(data.casinoChipID, file);
}

void TESCasino::Copy(TESForm* apCopy) {
    const char* v2;
    TESTexture* nthSlotReel;
    TESQuest* winningsQuest;
    uint8_t formType;
    const char* fullName_0;
    TESModelTextureSwap* nthModel;
    int32_t index;
    int32_t i;
    TESCasino* pCasino = dynamic_cast<TESCasino*>(apCopy);
    if (pCasino) {
        TESForm::CopyAllComponents(pCasino);
        for (i = 0; i < 10; ++i) {
            nthModel = pCasino->GetModel(i);
            v2 = nthModel->GetModel();
            (&modelArrayList[i])->SetModel(v2);
        }
        for (index = 0; index < 11; ++index) {
            nthSlotReel = pCasino->GetTexture(index);
            fullName_0 = nthSlotReel->TESTexture::GetTextureName();
            (&textureArrayList[index])->TESTexture::SetTextureName(fullName_0);
        }
        if (pCasino->GetQuest()) {
            winningsQuest = pCasino->GetQuest();
            data.questID = winningsQuest->TESForm::GetFormID();
        }
        else {
            data.questID = (uint32_t)0;
        }
        data = *pCasino->GetData();
        formType = pCasino->GetFormType();
        TESForm::SetFormType(formType);
    }
}

bool TESCasino::Compare(TESForm* apCompare) {
    TESCasino* pComp = dynamic_cast<TESCasino*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    for (int32_t i = 0; i < 11; ++i) {
        TESTexture* Texture = pComp->GetTexture(i);
        if (BSstrcmp(textureArrayList[i].GetTextureName(), Texture->GetTextureName())) {
            return true;
        }
    }
    for (int32_t i_2 = 0; i_2 < 10; ++i_2) {
        TESModelTextureSwap* Model_ = pComp->GetModel(i_2);
        const char* v6 = Model_->GetModel();
        const char* v4 = modelArrayList[i_2].GetModel();
        if (BSstrcmp(v4, v6)) {
            return true;
        }
    }
    TESQuest* FormByNumericID = static_cast<TESQuest*>(TESForm::GetFormByNumericID(data.questID));
    return FormByNumericID != pComp->GetQuest() || std::memcmp(&data, &pComp->data, sizeof(data)) != 0;
}

TESTexture* TESCasino::GetTexture(int32_t index) {
    return &textureArrayList[index];
}

TESQuest* TESCasino::GetQuest() {
    return static_cast<TESQuest*>(TESForm::GetFormByNumericID(data.questID));
}

int32_t TESCasino::GetMaxWinnings() {
    return (int32_t)((uint32_t)data.iMaxWinnings);
}

TESModelTextureSwap* TESCasino::GetModel(int32_t a2) {
    return &modelArrayList[a2];
}
