#include "Fallout Shared/BGSAcousticSpace.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

BGSAcousticSpace::BGSAcousticSpace() {
    SetFormType(ASPC_ID);
    BGSAcousticSpace::InitializeData();
}

BGSAcousticSpace::~BGSAcousticSpace() {}

void BGSAcousticSpace::InitializeData() {
    pDawnSound = nullptr;
    pNoonSound = nullptr;
    pDuskSound = nullptr;
    pNightSound = nullptr;
    pWallaSound = nullptr;
    iWallaPop = (uint32_t)0;
    bIsInterior = true;
    pSoundRegion = nullptr;
    eEnvType = CET_DEFAULT;
}

// Reads the five sounds (SNAM, in dawn/noon/dusk/night/walla order), the environment type (ANAM), the walla pop count
// (WNAM), the sound region (RDAT), the interior flag (INAM), the editor ID and the object bounds. The INAM value is
// initialised to 0 so that an INAM chunk shorter than four bytes (malformed data) does not leave it unwritten when it
// is tested.
bool BGSAcousticSpace::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    uint32_t uiSoundIndex = 0;
    // One variable is shared by SNAM, RDAT and ANAM. It is zeroed on entry and before the SNAM and RDAT reads, but
    // deliberately not before the ANAM read, which keeps the previous value, as the game does.
    uint32_t uiFormID = 0;
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == INAM_ID) {
            uint32_t uiInterior = 0;
            apFile->GetChunkData(uiInterior);
            bIsInterior = uiInterior != 0;
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == SNAM_ID) {
            // The form ID is stored straight into the sound pointer it picks.
            uiFormID = 0;
            apFile->GetChunkData(uiFormID);
            TESSound* pSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID));
            switch (uiSoundIndex) {
                case 4: SetWallaSound(pSound); break;
                case 3: SetNightSound(pSound); break;
                case 2: SetDuskSound(pSound); break;
                case 1: SetNoonSound(pSound); break;
                default: SetDawnSound(pSound); break;
            }
            ++uiSoundIndex;
        }
        else if (uiChunk == WNAM_ID) {
            apFile->GetChunkData(iWallaPop);
        }
        else if (uiChunk == RDAT_ID) {
            // Stores the form ID straight into the region pointer.
            uiFormID = 0;
            apFile->GetChunkData(uiFormID);
            pSoundRegion = reinterpret_cast<TESRegion*>(static_cast<uintptr_t>(uiFormID));
        }
        else if (uiChunk == ANAM_ID) {
            apFile->GetChunkData(uiFormID);
            eEnvType = static_cast<CELL_ENVIRO_TYPE>(uiFormID);
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSAcousticSpace::Save() {
    StartForm();
    uint32_t soundid = 0;
    if (pDawnSound) {
        soundid = pDawnSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid);
    uint32_t soundid_2 = 0;
    if (pNoonSound) {
        soundid_2 = pNoonSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid_2);
    uint32_t soundid_3 = 0;
    if (pDuskSound) {
        soundid_3 = pDuskSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid_3);
    uint32_t soundid_4 = 0;
    if (pNightSound) {
        soundid_4 = pNightSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid_4);
    uint32_t soundid_5 = 0;
    if (pWallaSound) {
        soundid_5 = pWallaSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid_5);
    TESForm::AddChunk(WNAM_ID, iWallaPop);
    if (pSoundRegion) {
        TESForm::AddChunk(RDAT_ID, pSoundRegion->iFormID);
    }
    // The enum promotes to its int32_t underlying type, selecting the int overload.
    TESForm::AddChunk(ANAM_ID, eEnvType);
    if (bIsInterior) {
        TESForm::AddChunk(INAM_ID, 1u);
    }
    else {
        TESForm::AddChunk(INAM_ID, 0u);
    }
    CloseForm();
}

void BGSAcousticSpace::InitItem() {
    if (!GetInitialized()) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDawnSound));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pDawnSound = dynamic_cast<TESSound*>(FormByNumericID);
            if (!pDawnSound) {
                int32_t CastingType = iFormID;
                const char* v13 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find pDawnSound (%08X) for activator '%s' (%08X).", itempint, v13,
                    CastingType
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNoonSound));
        if (itempint) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempint);
            pNoonSound = dynamic_cast<TESSound*>(v4);
            if (!pNoonSound) {
                int32_t v20 = iFormID;
                const char* v14 = GetFormEditorID();
                Warning("MASTERFILE: Could not find pNoonSound (%08X) for activator '%s' (%08X).", itempint, v14, v20);
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDuskSound));
        if (itempint) {
            TESFile* v5 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v5);
            TESForm* v6 = TESForm::GetFormByNumericID(itempint);
            pDuskSound = dynamic_cast<TESSound*>(v6);
            if (!pDuskSound) {
                int32_t v21 = iFormID;
                const char* v15 = GetFormEditorID();
                Warning("MASTERFILE: Could not find pDuskSound (%08X) for activator '%s' (%08X).", itempint, v15, v21);
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNightSound));
        if (itempint) {
            TESFile* v7 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v7);
            TESForm* v8 = TESForm::GetFormByNumericID(itempint);
            pNightSound = dynamic_cast<TESSound*>(v8);
            if (!pNightSound) {
                int32_t v22 = iFormID;
                const char* v16 = GetFormEditorID();
                Warning("MASTERFILE: Could not find pNightSound (%08X) for activator '%s' (%08X).", itempint, v16, v22);
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundRegion));
        if (itempint) {
            TESFile* v9 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v9);
            TESForm* v10 = TESForm::GetFormByNumericID(itempint);
            pSoundRegion = dynamic_cast<TESRegion*>(v10);
            if (!pSoundRegion) {
                int32_t v23 = iFormID;
                const char* v17 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find sound region (%08X) for activator '%s' (%08X).", itempint, v17, v23
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWallaSound));
        if (itempint) {
            TESFile* v11 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v11);
            TESForm* v12 = TESForm::GetFormByNumericID(itempint);
            pWallaSound = dynamic_cast<TESSound*>(v12);
            if (!pWallaSound) {
                int32_t v24 = iFormID;
                const char* v18 = GetFormEditorID();
                Warning("MASTERFILE: Could not find pWallaSound (%08X) for activator '%s' (%08X).", itempint, v18, v24);
            }
        }
        SetInitialized(true);
    }
}

void BGSAcousticSpace::Copy(TESForm* apCopy) {
    BGSAcousticSpace* pcopy = dynamic_cast<BGSAcousticSpace*>(apCopy);
    if (pcopy) {
        uint8_t IsInterior = pcopy->IsInterior();
        bIsInterior = IsInterior;
        SetDawnSound(pcopy->pDawnSound);
        SetNoonSound(pcopy->pNoonSound);
        pDuskSound = pcopy->pDuskSound;
        pNightSound = pcopy->pNightSound;
        pWallaSound = pcopy->pWallaSound;
        uint32_t UnderwaterState = pcopy->iWallaPop;
        iWallaPop = UnderwaterState;
        pSoundRegion = pcopy->pSoundRegion;
        eEnvType = pcopy->eEnvType;
        CopyAllComponents(pcopy);
    }
}

bool BGSAcousticSpace::Compare(TESForm* apCompare) {
    BGSAcousticSpace* pcomp = dynamic_cast<BGSAcousticSpace*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t IsInterior = this->IsInterior();
    if (IsInterior != pcomp->IsInterior()) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    TESSound* Interior = pDawnSound;
    if (Interior != pcomp->pDawnSound) {
        return true;
    }
    TESSound* ConversationData = pNoonSound;
    if (ConversationData != pcomp->pNoonSound) {
        return true;
    }
    TESSound* EnchantmentSource = pDuskSound;
    if (EnchantmentSource != pcomp->pDuskSound) {
        return true;
    }
    TESSound* ParentCell = pNightSound;
    if (ParentCell != pcomp->pNightSound) {
        return true;
    }
    TESSound* State = pWallaSound;
    if (State != pcomp->pWallaSound) {
        return true;
    }
    int32_t UnderwaterState = iWallaPop;
    if (UnderwaterState != pcomp->iWallaPop) {
        return true;
    }
    TESRegion* CombatStyle = pSoundRegion;
    if (CombatStyle != pcomp->pSoundRegion) {
        return true;
    }
    int32_t v11 = eEnvType;
    return v11 != pcomp->eEnvType;
}

bool BGSAcousticSpace::IsInterior() {
    return (bool)((uint8_t)bIsInterior);
}

void BGSAcousticSpace::SetNoonSound(TESSound* apSound) {
    pNoonSound = apSound;
}
