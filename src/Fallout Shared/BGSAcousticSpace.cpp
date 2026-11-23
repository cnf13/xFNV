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
#include "xfnv/Compiler.h"

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
    iWallaPop = 0;
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
    // deliberately not before the ANAM read, which keeps the previous value.
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
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
    soundid = 0;
    if (pNoonSound) {
        soundid = pNoonSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid);
    soundid = 0;
    if (pDuskSound) {
        soundid = pDuskSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid);
    soundid = 0;
    if (pNightSound) {
        soundid = pNightSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid);
    soundid = 0;
    if (pWallaSound) {
        soundid = pWallaSound->iFormID;
    }
    TESForm::AddChunk(SNAM_ID, soundid);
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
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pDawnSound = dynamic_cast<TESSound*>(pForm);
            if (!pDawnSound) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find pDawnSound (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNoonSound));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pNoonSound = dynamic_cast<TESSound*>(pForm);
            if (!pNoonSound) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find pNoonSound (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDuskSound));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pDuskSound = dynamic_cast<TESSound*>(pForm);
            if (!pDuskSound) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find pDuskSound (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNightSound));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pNightSound = dynamic_cast<TESSound*>(pForm);
            if (!pNightSound) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find pNightSound (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundRegion));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pSoundRegion = dynamic_cast<TESRegion*>(pForm);
            if (!pSoundRegion) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find sound region (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWallaSound));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pWallaSound = dynamic_cast<TESSound*>(pForm);
            if (!pWallaSound) {
                int32_t iActivatorID = iFormID;
                Warning(
                    "MASTERFILE: Could not find pWallaSound (%08X) for activator '%s' (%08X).",
                    itempint,
                    GetFormEditorID(),
                    iActivatorID
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSAcousticSpace::Copy(TESForm* apCopy) {
    BGSAcousticSpace* pcopy = dynamic_cast<BGSAcousticSpace*>(apCopy);
    if (pcopy) {
        uint8_t ucIsInterior = pcopy->IsInterior();
        bIsInterior = ucIsInterior;
        SetDawnSound(pcopy->pDawnSound);
        SetNoonSound(pcopy->pNoonSound);
        pDuskSound = pcopy->pDuskSound;
        pNightSound = pcopy->pNightSound;
        pWallaSound = pcopy->pWallaSound;
        iWallaPop = pcopy->iWallaPop;
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
    int32_t iIsInterior = this->IsInterior();
    if (iIsInterior != pcomp->IsInterior()) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (pDawnSound != pcomp->pDawnSound) {
        return true;
    }
    if (pNoonSound != pcomp->pNoonSound) {
        return true;
    }
    if (pDuskSound != pcomp->pDuskSound) {
        return true;
    }
    if (pNightSound != pcomp->pNightSound) {
        return true;
    }
    if (pWallaSound != pcomp->pWallaSound) {
        return true;
    }
    if (iWallaPop != pcomp->iWallaPop) {
        return true;
    }
    if (pSoundRegion != pcomp->pSoundRegion) {
        return true;
    }
    return eEnvType != pcomp->eEnvType;
}

bool BGSAcousticSpace::IsInterior() {
    return bIsInterior;
}

void BGSAcousticSpace::SetNoonSound(TESSound* apSound) {
    pNoonSound = apSound;
}
