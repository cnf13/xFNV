#include "Fallout Shared/TESObjectARMO.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMA.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

SettingT<GameSettingCollection> sArmorWeightLight("sArmorWeightLight", "Light");
SettingT<GameSettingCollection> sArmorWeightHeavy("sArmorWeightHeavy", "Heavy");
SettingT<GameSettingCollection> sArmorWeightMedium("sArmorWeightMedium", "Medium");
static SettingT<GameSettingCollection> iArmorWeightHelmet("iArmorWeightHelmet", 5);
static SettingT<GameSettingCollection> iArmorWeightCuirass("iArmorWeightCuirass", 30);
static SettingT<GameSettingCollection> iArmorWeightGreaves("iArmorWeightGreaves", 15);
static SettingT<GameSettingCollection> iArmorWeightGauntlets("iArmorWeightGauntlets", 5);
static SettingT<GameSettingCollection> iArmorWeightBoots("iArmorWeightBoots", 20);
static SettingT<GameSettingCollection> iArmorWeightShield("iArmorWeightShield", 15);

// Reads the armor's chunks. The two foley chunks are conditional: SNAM only fills the sound list of an armor that is
// its own foley base, and TNAM only sets the template of one that is not. When a chunk is empty (malformed data), its
// string buffer is initialised to "" and BMDT's old ADDON_DATA to zeroes.
bool TESObjectARMO::Load(TESFile* apFile) {
    if ((uint8_t)apFile->GetTESForm() != 24) {
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
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptableForm->InitItem(this);
                break;
            }
            case ENAM_ID:
            case EITM_ID: {
                uint32_t uiEnchantmentID = 0;
                apFile->GetChunkData(uiEnchantmentID);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchanting(
                    reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(uiEnchantmentID))
                );
                break;
            }
            case ANAM_ID:
            case EAMT_ID: {
                int16_t sAmount = 0;
                apFile->LoadWord(&sAmount);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchantmentAmount((uint16_t)sAmount);
                break;
            }
            case BMDT_ID:
                if (apFile->GetFormVersion() >= 0xBu) {
                    apFile->GetChunkData(&bipedModelData, 8u);
                    if (apFile->QEndian()) {
                        bipedModelData.Endian();
                    }
                }
                else {
                    // Zeroed, as an empty chunk leaves it unwritten (see above).
                    ADDON_DATA oldData{};
                    apFile->GetChunkData(&oldData, 4u);
                    if (apFile->QEndian()) {
                        oldData.Endian();
                    }
                    bipedModelData.sFlags = oldData.cFlags;
                    bipedModelData.iBipedObjectSlots = oldData.usMasterParticleCap;
                }
                bipedModelData.sFlags &= ~0x10u;
                break;
            case MODL_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD2_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD3_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel + 1;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD4_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel + 1;
                pModel->SetModel(pBuffer);
                break;
            }
            case BMCT_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = &static_cast<TESBipedModelForm*>(this)->ConstraintTemplate;
                pModel->SetModel(pBuffer);
                break;
            }
            case MODT_ID:
                TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->bipedModel, apFile);
                break;
            case MO2T_ID:
                TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->worldModel, apFile);
                break;
            case MO3T_ID:
                TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->bipedModel + 1, apFile);
                break;
            case MO4T_ID:
                TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->worldModel + 1, apFile);
                break;
            case MODD_ID: TESModel::LoadModelChunk(static_cast<TESBipedModelForm*>(this)->bipedModel, apFile); break;
            case MOSD_ID:
                TESModel::LoadModelChunk(static_cast<TESBipedModelForm*>(this)->bipedModel + 1, apFile);
                break;
            case MODS_ID:
                TESModelTextureSwap::LoadModelTextureSwapChunk(
                    static_cast<TESBipedModelForm*>(this)->bipedModel, apFile
                );
                break;
            case MO2S_ID:
                TESModelTextureSwap::LoadModelTextureSwapChunk(
                    static_cast<TESBipedModelForm*>(this)->worldModel, apFile
                );
                break;
            case MO3S_ID:
                TESModelTextureSwap::LoadModelTextureSwapChunk(
                    static_cast<TESBipedModelForm*>(this)->bipedModel + 1, apFile
                );
                break;
            case MO4S_ID:
                TESModelTextureSwap::LoadModelTextureSwapChunk(
                    static_cast<TESBipedModelForm*>(this)->worldModel + 1, apFile
                );
                break;
            case ICON_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->inventoryIcon[0].SetTextureName(pBuffer);
                break;
            }
            case ICO2_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->inventoryIcon[1].SetTextureName(pBuffer);
                break;
            }
            case MICO_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->messageIcon[0].SetMessageIconTextureName(pBuffer);
                break;
            }
            case MIC2_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->messageIcon[1].SetMessageIconTextureName(pBuffer);
                break;
            }
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case REPL_ID: {
                uint32_t uiListFormID = 0;
                apFile->GetChunkData(uiListFormID);
                static_cast<BGSRepairItemList*>(this)->pListForm =
                    reinterpret_cast<BGSListForm*>(static_cast<uintptr_t>(uiListFormID));
                break;
            }
            case BIPL_ID: {
                uint32_t uiListFormID = 0;
                apFile->GetChunkData(uiListFormID);
                static_cast<BGSBipedModelList*>(this)->SetBipedModelList(
                    reinterpret_cast<BGSListForm*>(static_cast<uintptr_t>(uiListFormID))
                );
                break;
            }
            case ETYP_ID: {
                uint32_t uiEquipType = 0;
                apFile->GetChunkData(uiEquipType);
                static_cast<BGSEquipType*>(this)->SetEquipType((BGSEquipType::EQUIP_TYPE)uiEquipType);
                break;
            }
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
            case BNAM_ID: {
                uint32_t uiFoleyBase = 0;
                apFile->GetChunkData(uiFoleyBase);
                bIsFoleyBase = (bool)(uiFoleyBase != 0);
                break;
            }
            case TNAM_ID:
                if (!bIsFoleyBase) {
                    uint32_t uiTemplateID = 0;
                    apFile->GetChunkData(uiTemplateID);
                    pFoleyTemplate = reinterpret_cast<TESObjectARMO*>(static_cast<uintptr_t>(uiTemplateID));
                }
                break;
            case SNAM_ID:
                if (bIsFoleyBase) {
                    if (!pFoleySoundList) {
                        void* pMemory = ourMalloc(sizeof(BSSimpleList<FOLEY_SOUND*>));
                        BSSimpleList<FOLEY_SOUND*>* pFoleyList = nullptr;
                        if (pMemory) {
                            pFoleyList = ::new (pMemory) BSSimpleList<FOLEY_SOUND*>;
                        }
                        pFoleySoundList = pFoleyList;
                    }
                    FOLEY_SOUND* pFoleySound = nullptr;
                    void* pMemory = ourMalloc(sizeof(FOLEY_SOUND));
                    if (pMemory) {
                        pFoleySound = ::new (pMemory) FOLEY_SOUND;
                    }
                    LoadData(apFile, reinterpret_cast<char*>(pFoleySound), 0xCu);
#if XFNV_64BIT
                    // The file's record is 0xC bytes, the sound's 4-byte form ID first.
                    TESFile::WidenFormIDRecords(
                        pFoleySound, 1, 0xC, sizeof(FOLEY_SOUND), {{0, offsetof(FOLEY_SOUND, pSound)}}
                    );
#endif
                    if (apFile->QEndian()) {
                        pFoleySound->Endian();
                    }
                    pFoleySoundList->AddHead(pFoleySound);
                }
                break;
            case DATA_ID: {
                int32_t iDataSizeAdded = GetDataSizeAdded();
                uint32_t uiChunkSize = apFile->GetChunkSize();
                if (uiChunkSize - (uint32_t)iDataSizeAdded == 2) {
                    ADDON_DATA oldData;
                    ourMemset(&oldData, 0, 2u);
                    LoadData(apFile, reinterpret_cast<char*>(&oldData), 2u);
                    if (apFile->QEndian()) {
                        oldData.Endian();
                    }
                    ourMemcpy(&data, &oldData, 2u);
                    data.cFlags = 0;
                }
                else {
                    LoadData(apFile, nullptr, 0);
                }
                break;
            }
            case DNAM_ID:
                ourMemset(&data, 0, sizeof(data));
                apFile->GetChunkData(&data, 0xCu);
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            default: break;
        }
        if (uiChunk == DATA_ID && apFile->QEndian()) {
            data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

// eType is set to 0, which no enumerator of ENUM_FOLEY_SOUND (17..22) names, as the game does.
FOLEY_SOUND::FOLEY_SOUND() : pSound(nullptr), iChance(0), eType(static_cast<ENUM_FOLEY_SOUND>(0)) {}

void FOLEY_SOUND::Endian() {}

TESObjectARMO::TESObjectARMO() {
    SetFormType(24u);
    TESObjectARMO::InitializeData();
}

TESObjectARMO::~TESObjectARMO() {}

void TESObjectARMO::InitializeData() {
    ourMemset(&data.sRating, 0, sizeof(OBJ_ARMO) - offsetof(OBJ_ARMO, sRating));
    bIsFoleyBase = false;
    pFoleySoundList = (BSSimpleList<FOLEY_SOUND*>*)nullptr;
    static_cast<TESEnchantableForm*>(this)->TESEnchantableForm::SetCastingType(MagicSystem::CONSTANT_EFFECT);
}

void TESObjectARMO::Save() {
    FOLEY_SOUND tsnd;
    StartForm();
    TESFullName::Save();
    TESScriptableForm::Save();
    SaveSubRecord();
    TESBipedModelForm::Save();
    BGSDestructibleObjectForm::Save();
    BGSRepairItemList::Save();
    BGSBipedModelList::Save();
    BGSEquipType::Save();
    BGSPickupPutdownSounds::Save();
    SaveData();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &data, 12u);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::AddChunk(BNAM_ID, static_cast<uint32_t>(bIsFoleyBase));
    if (bIsFoleyBase) {
        if (this->pFoleySoundList) {
            BSSimpleList<FOLEY_SOUND*>* tList = this->pFoleySoundList;
            while (tList && tList->m_item) {
                FOLEY_SOUND* fsnd = tList->m_item;
                tList = tList->m_pkNext;
                if ((fsnd) && (fsnd->pSound)) {
                    tsnd.~FOLEY_SOUND();
                    ::new (&tsnd) FOLEY_SOUND;
                    // The saved record holds the sound's form ID in the pointer field.
                    tsnd.pSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(fsnd->pSound->iFormID));
                    tsnd.iChance = fsnd->iChance;
                    tsnd.eType = fsnd->eType;
                    if (TESForm::QEndianSwapOnSave()) {
                        tsnd.Endian();
                    }
                    TESForm::__AddChunkData(SNAM_ID, &tsnd, 12u);
                    if (TESForm::QEndianSwapOnSave()) {
                        tsnd.Endian();
                    }
                }
            }
        }
    }
    else {
        // An armor that borrows its foley sounds saves its template's form ID.
        if (this->pFoleyTemplate) {
            uint32_t tint = this->pFoleyTemplate->iFormID;
            TESForm::AddChunk(TNAM_ID, tint);
        }
    }
    CloseForm();
}

uint16_t TESObjectARMO::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectARMO::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectARMO::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESObjectARMO::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectARMO::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESObjectARMO::InitItem() {
    if (!GetInitialized()) {
        if (!bipedModelData.iBipedObjectSlots) {
            Warning("MASTERFILE: Armor '%s' needs to have biped slots selected in the editor.", GetFormEditorID());
        }
        TESScriptableForm::InitItem(this);
        TESEnchantableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESBipedModelForm::InitItem(this);
        BGSRepairItemList::InitItem(this);
        BGSBipedModelList::InitItem(this);
        BGSPickupPutdownSounds::InitItem(this);
        if (bIsFoleyBase || !this->pFoleySoundList) {
            if ((bIsFoleyBase) && (this->pFoleySoundList)) {
                BSSimpleList<FOLEY_SOUND*>* tList = this->pFoleySoundList;
                while (tList && tList->m_item) {
                    FOLEY_SOUND* fsnd = tList->m_item;
                    tList = tList->m_pkNext;
                    if (fsnd->pSound) {
                        fsnd->pSound = static_cast<TESSound*>(TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fsnd->pSound))
                        ));
                    }
                }
            }
        }
        else {
            // An armor that borrows its foley sounds holds its base armor's form ID in the field until now.
            pFoleyTemplate = static_cast<TESObjectARMO*>(
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFoleyTemplate)))
            );
        }
        SetInitialized(true);
    }
}

void TESObjectARMO::Copy(TESForm* apCopy) {
    TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(apCopy);
    if (parmo) {
        CopyAllComponents(apCopy);
        data = parmo->data;
        if ((bIsFoleyBase) && (this->pFoleySoundList)) {
            for (FOLEY_SOUND*& rpItem : this->pFoleySoundList->ItemsUntilNull()) {
                ourFree(rpItem);
            }
            this->pFoleySoundList->RemoveAll();
            delete this->pFoleySoundList;
        }

        bIsFoleyBase = parmo->bIsFoleyBase;
        if (bIsFoleyBase) {
            BSSimpleList<FOLEY_SOUND*>* v3;
            BSSimpleList<FOLEY_SOUND*>* v6 =
                static_cast<BSSimpleList<FOLEY_SOUND*>*>(ourMalloc(sizeof(BSSimpleList<FOLEY_SOUND*>)));
            if (v6) {
                v3 = ::new (v6) BSSimpleList<FOLEY_SOUND*>;
            }
            else {
                v3 = nullptr;
            }
            this->pFoleySoundList = v3;
            if (parmo->pFoleySoundList) {
                BSSimpleList<FOLEY_SOUND*>* tList_2 = parmo->pFoleySoundList;
                while (tList_2 && tList_2->m_item) {
                    FOLEY_SOUND* fsnd = tList_2->m_item;
                    tList_2 = tList_2->m_pkNext;
                    if ((fsnd) && (fsnd->pSound)) {
                        FOLEY_SOUND* tsnd;
                        FOLEY_SOUND* apMemory = static_cast<FOLEY_SOUND*>(ourMalloc(sizeof(FOLEY_SOUND)));
                        if (apMemory) {
                            tsnd = ::new (apMemory) FOLEY_SOUND;
                        }
                        else {
                            tsnd = nullptr;
                        }
                        FOLEY_SOUND* tsnd_2 = tsnd;
                        *tsnd = *fsnd;
                        this->pFoleySoundList->AddHead(tsnd_2);
                    }
                }
            }
        }
        else {
            this->pFoleySoundList = parmo->pFoleySoundList;
        }
    }
}

bool TESObjectARMO::Compare(TESForm* apCompare) {
    TESObjectARMO* pcomp = dynamic_cast<TESObjectARMO*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (std::memcmp(&data, &pcomp->data, sizeof(data))) {
        return true;
    }
    if (bIsFoleyBase != pcomp->bIsFoleyBase) {
        return true;
    }
    if (bIsFoleyBase) {
        if (!pcomp->pFoleySoundList) {
            return true;
        }
        if (!this->pFoleySoundList) {
            return true;
        }
        BSSimpleList<FOLEY_SOUND*>* v3 = pcomp->pFoleySoundList;
        if (v3->m_item != this->pFoleySoundList->m_item) {
            return true;
        }
        for (FOLEY_SOUND*& rpItem : this->pFoleySoundList->ItemsUntilNull()) {
            bool bFound = false;
            for (FOLEY_SOUND*& rpItem_2 : pcomp->pFoleySoundList->ItemsUntilNull()) {
                FOLEY_SOUND* m_item = rpItem;
                if (m_item->pSound == rpItem_2->pSound) {
                    int32_t iChance = rpItem->iChance;
                    if (iChance == rpItem_2->iChance) {
                        FOLEY_SOUND* v6 = rpItem;
                        if (v6->eType == rpItem_2->eType) {
                            bFound = true;
                            break;
                        }
                    }
                }
            }
            if (!bFound) {
                return true;
            }
        }
    }
    else {
        if (this->pFoleySoundList != pcomp->pFoleySoundList) {
            return true;
        }
    }
    return false;
}

bool TESObjectARMO::ModulatesVoice() {
    return (data.cFlags & 1) != 0;
}

float TESObjectARMO::GetCalculatedArmorRating() {
    return (float)(((double)data.sRating) / 100.0);
}

uint8_t TESObjectARMO::GetArmorWeight() {
    if (static_cast<TESBipedModelForm*>(this)->IsHeavy()) {
        return (uint8_t)1;
    }
    if (static_cast<TESBipedModelForm*>(this)->IsMedium()) {
        return (uint8_t)2;
    }
    return (uint8_t)0;
}

TESSound* TESObjectARMO::GetFoleySound(ENUM_FOLEY_SOUND aeType) {
    BSSimpleList<FOLEY_SOUND*>* pList;
    TESSound* pSound = nullptr;
    if (bIsFoleyBase) {
        pList = this->pFoleySoundList;
    }
    else if (this->pFoleyTemplate) {
        // An armor that borrows its foley sounds uses its template's list.
        pList = this->pFoleyTemplate->pFoleySoundList;
    }
    else {
        pList = nullptr;
    }
    while (pList && (!pList->IsEmpty() && !pSound)) {
        FOLEY_SOUND* pData = pList->m_item;
        pList = pList->m_pkNext;
        if ((pData->pSound) && (pData->eType == aeType && static_cast<int32_t>(Random() & 99) < pData->iChance))
        // Random() & 99 is compared with the chance byte signed.
        {
            pSound = pData->pSound;
        }
    }
    return pSound;
}
