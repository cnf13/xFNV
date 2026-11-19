#include "Fallout Shared/TESObjectARMA.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

TESObjectARMA::TESObjectARMA() {
    SetFormType(0x60u);
    TESObjectARMA::InitializeData();
}

TESObjectARMA::~TESObjectARMA() {}

void TESObjectARMA::InitializeData() {
    ourMemset(&data.sRating, 0, sizeof(OBJ_ARMO) - offsetof(OBJ_ARMO, sRating));
}

// Reads the armor addon's chunks. An armor addon shares the armor's layout, so the model, icon and message icon chunks
// land on the same members; the addon sets its own form type when it is done. The editor ID buffer is emptied before
// the chunk is read: GetChunkData writes at most the chunk's bytes, and a zero-size chunk leaves the buffer unwritten,
// so uninitialised data would otherwise be read as the editor ID.
bool TESObjectARMA::Load(TESFile* apFile) {
    if ((uint8_t)apFile->GetTESForm() != 96) {
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                // the empty string, for a chunk that writes nothing
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case BMDT_ID:
                if (apFile->GetFormVersion() >= 0xBu) {
                    apFile->GetChunkData(&bipedModelData, 8u);
                    if (apFile->QEndian()) {
                        bipedModelData.Endian();
                    }
                }
                else {
                    ADDON_DATA oldData;
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD2_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD3_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel + 1;
                pModel->SetModel(pBuffer);
                break;
            }
            case MOD4_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel + 1;
                pModel->SetModel(pBuffer);
                break;
            }
            case BMCT_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->inventoryIcon[0].SetTextureName(pBuffer);
                break;
            }
            case ICO2_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->inventoryIcon[1].SetTextureName(pBuffer);
                break;
            }
            case MICO_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->messageIcon[0].SetMessageIconTextureName(pBuffer);
                break;
            }
            case MIC2_ID: {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESBipedModelForm*>(this)->messageIcon[1].SetMessageIconTextureName(pBuffer);
                break;
            }
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case ETYP_ID: {
                uint32_t uiEquipType = 0;
                apFile->GetChunkData(uiEquipType);
                static_cast<BGSEquipType*>(this)->SetEquipType((BGSEquipType::EQUIP_TYPE)uiEquipType);
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
    SetFormType(24);
    return true;
}

void TESObjectARMA::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESBipedModelForm*>(this)->Save();
    static_cast<BGSEquipType*>(this)->Save();
    TESForm::SaveData();
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &data, 0xCu);
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::CloseForm();
}

uint16_t TESObjectARMA::GetSaveSize(uint32_t aiFlags) {
    return TESForm::GetSaveSize(aiFlags);
}

void TESObjectARMA::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
}

void TESObjectARMA::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectARMA::InitItem() {
    const char* v1;
    if (!TESForm::GetInitialized()) {
        if (!bipedModelData.iBipedObjectSlots) {
            v1 = GetFormEditorID();
            Warning("MASTERFILE: Armor '%s' needs to have biped slots selected in the editor.", v1);
        }
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        static_cast<TESBipedModelForm*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void TESObjectARMA::Copy(TESForm* apCopy) {
    if (dynamic_cast<TESObjectARMO*>(apCopy)) {
        ourMemcpy(&data, &apCopy[15].pSourceFiles, sizeof(data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESObjectARMA::Compare(TESForm* apCompare) {
    TESForm* v4 = dynamic_cast<TESObjectARMA*>(apCompare);
    if (!v4) {
        return true;
    }
    if (CompareAllComponents(v4)) {
        return true;
    }
    return std::memcmp(&data, &v4[15].pSourceFiles, sizeof(data)) != 0;
}

// The flag and condition bytes need no swap.
void OBJ_ARMO::Endian() {
    ENDIAN(sRating, false);
    ENDIAN(fDamageThreshold, false);
}
