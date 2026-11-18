#include "Fallout Shared/BGSAddonNode.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSAddonNodeSoundHandleExtra.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

// The index handed to the next master particle system loaded.
extern uint32_t uiParticleSystemIndex;

uint32_t uiParticleSystemIndex;
NiRTTI BGSAddonNodeSoundHandleExtra::ms_RTTI("BGSAddonNodeSoundHandleExtra", &NiExtraData::ms_RTTI);

// The cap is the only multi-byte field.
void ADDON_DATA::Endian() {
    ENDIAN(usMasterParticleCap, false);
}

BGSAddonNode::BGSAddonNode() : iIndex(0xFFFFFFFF) {
    SetFormType(ADDN_ID);
    BGSAddonNode::InitializeData();
}

BGSAddonNode::~BGSAddonNode() {
    if (pDataHandler) {
        pDataHandler->RemoveAddonNode(this);
    }
}

// Clears the addon node's sound, particle data and master-system index.
void BGSAddonNode::InitializeData() {
    pSound = nullptr;
    ourMemset(&Data, 0, sizeof(Data));
    iMasterParticleSystemIndex = 0;
}

// Reads the addon node's index, sound, particle data and model chunks.
bool BGSAddonNode::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != ADDN_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == MODL_ID || uiChunk == MODD_ID || uiChunk == MODT_ID || uiChunk == MOSD_ID) {
            TESModel::LoadModelChunk(this, apFile);
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == DATA_ID) {
            apFile->GetChunkData(iIndex);
        }
        else if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == MODS_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            uint32_t uiDataSize = apFile->GetChunkSize();
            char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiDataSize, 128));
            apFile->GetChunkData(pBuffer, uiDataSize);
            UnpackTextureSwapChunkData(pBuffer, uiDataSize, apFile->GetFormVersion());
        }
        else if (uiChunk == DNAM_ID) {
            apFile->GetChunkData(&Data, 4u);
            if (apFile->QEndian()) {
                Data.Endian();
            }
            if (IsMasterParticleSystem()) {
                iMasterParticleSystemIndex = uiParticleSystemIndex++;
            }
        }
        else if (uiChunk == SNAM_ID) {
            // Stores the form ID straight into the sound pointer, as the game does.
            uint32_t uiSoundFormID = 0;
            apFile->GetChunkData(uiSoundFormID);
            pSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundFormID));
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSAddonNode::Save() {
    StartForm();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::AddChunk(DATA_ID, iIndex);
    if (pSound) {
        TESForm::AddChunk(SNAM_ID, pSound->iFormID);
    }
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &Data, 4u);
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    CloseForm();
}

// Resolves its texture swaps and sound form once, then marks the node initialized.
void BGSAddonNode::InitItem() {
    if (GetInitialized()) {
        return;
    }

    TESModelTextureSwap::InitItem(this);
    if (pSound) {
        // The stored pointer bits are treated as the numeric form ID in this local.
        uint32_t soundFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSound));
        TESFile* file = GetFile(-1);
        TESForm::AddCompileIndex(soundFormID, file);
        TESForm* form = TESForm::GetFormByNumericID(soundFormID);
        pSound = dynamic_cast<TESSound*>(form);
        if (!pSound) {
            TESForm* formBase = this;
            const char* nodeName = formBase->GetFormEditorID();
            Warning("MASTERFILE: Unable to find sound (%08X) on addon node '%s'.", soundFormID, nodeName);
        }
    }
    SetInitialized(true);
}

void BGSAddonNode::Copy(TESForm* apCopy) {
    BGSAddonNode* v3 = dynamic_cast<BGSAddonNode*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(v3);
        iIndex = v3->iIndex;
        pSound = v3->pSound;
        ourMemcpy(&Data, &v3->Data, sizeof(Data));
    }
}

bool BGSAddonNode::Compare(TESForm* apCompare) {
    BGSAddonNode* v4 = dynamic_cast<BGSAddonNode*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return true;
    }
    if (iIndex != v4->iIndex) {
        return true;
    }
    if (pSound == v4->pSound) {
        return (std::memcmp(&Data, &v4->Data, sizeof(Data)) != 0);
    }
    return true;
}

uint32_t BGSAddonNode::GetIndex() {
    return iIndex;
}

uint32_t BGSAddonNode::GetMasterParticleSystemIndex() {
    return iMasterParticleSystemIndex;
}

BGSAddonNodeSoundHandleExtra::BGSAddonNodeSoundHandleExtra() : NiExtraData(Tag()), SoundHandle() {}

BGSAddonNodeSoundHandleExtra::~BGSAddonNodeSoundHandleExtra() {
    SoundHandle.Stop();
}

void BGSAddonNodeSoundHandleExtra::InitTag() {
    NiFixedString* v0;
    void* v1 = ourMalloc(sizeof(NiFixedString));
    if (v1) {
        v0 = ::new (v1) NiFixedString(const_cast<char*>("ASH"));
    }
    else {
        v0 = nullptr;
    }
    BGSAddonNodeSoundHandleExtra::pTag = v0;
}

const NiFixedString& BGSAddonNodeSoundHandleExtra::Tag() {
    return *pTag;
}

NiFixedString* BGSAddonNodeSoundHandleExtra::KillSDM() {
    NiFixedString* result = BGSAddonNodeSoundHandleExtra::pTag;
    delete BGSAddonNodeSoundHandleExtra::pTag;
    BGSAddonNodeSoundHandleExtra::pTag = nullptr;
    return result;
}
