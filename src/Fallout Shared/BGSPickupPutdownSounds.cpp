#include "Fallout Shared/BGSPickupPutdownSounds.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

BGSPickupPutdownSounds::BGSPickupPutdownSounds() {
    BGSPickupPutdownSounds::InitializeDataComponent();
}

void BGSPickupPutdownSounds::InitializeDataComponent() {
    pPickupSound = nullptr;
    pPutdownSound = nullptr;
}

void BGSPickupPutdownSounds::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSPickupPutdownSounds* pCopy = dynamic_cast<BGSPickupPutdownSounds*>(apCopyFC);
    if (pCopy) {
        pPickupSound = pCopy->pPickupSound;
        pPutdownSound = pCopy->pPutdownSound;
    }
}

bool BGSPickupPutdownSounds::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSPickupPutdownSounds* pCompare = dynamic_cast<BGSPickupPutdownSounds*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    TESSound* pPickup = GetPickupSound();
    bool bSamePickup = pPickup == pCompare->GetPickupSound();
    if (bSamePickup) {
        TESSound* pPutdown = GetPutdownSound();
        if (pPutdown == pCompare->GetPutdownSound()) {
            return false;
        }
    }
    return true;
}

void BGSPickupPutdownSounds::InitItem(TESForm* apOwner) {
    uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPickupSound));
    TESFile* pOwnerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
    if (itempInt) {
        TESForm::AddCompileIndex(itempInt, pOwnerFile);
        TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
        pPickupSound = dynamic_cast<TESSound*>(pForm);
        if (!pPickupSound) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find pickup sound (%08X) for owner object \"%s\".",
                        itempInt,
                        pEditorID
                    );
                }
                else {
                    int32_t iOwnerID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find pickup sound  (%08X) for owner object (%08X).",
                        itempInt,
                        iOwnerID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find pickup sound  (%08X) for unknown owner.", itempInt);
            }
        }
    }
    itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPutdownSound));
    if (itempInt) {
        TESForm::AddCompileIndex(itempInt, pOwnerFile);
        TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
        pPutdownSound = dynamic_cast<TESSound*>(pForm);
        if (!pPutdownSound) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find putdown sound (%08X) for owner object \"%s\".",
                        itempInt,
                        pEditorID
                    );
                }
                else {
                    int32_t iOwnerID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find putdown sound  (%08X) for owner object (%08X).",
                        itempInt,
                        iOwnerID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find putdown sound  (%08X) for unknown owner.", itempInt);
            }
        }
    }
}

void BGSPickupPutdownSounds::Save() {
    if (pPickupSound) {
        TESForm::AddChunk(YNAM_ID, pPickupSound->iFormID);
    }
    if (pPutdownSound) {
        TESForm::AddChunk(ZNAM_ID, pPutdownSound->iFormID);
    }
}

TESSound* BGSPickupPutdownSounds::GetPutdownSound() {
    return pPutdownSound;
}

TESSound* BGSPickupPutdownSounds::GetPickupSound() {
    return pPickupSound;
}
