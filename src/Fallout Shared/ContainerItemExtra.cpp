#include "Fallout Shared/ContainerItemExtra.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESHealthForm.h"

void ContainerItemExtra::AddToExtraDataList(ExtraDataList* apExtra, TESHealthForm* apHealthForm) {
    if (apExtra) {
        if (pOwnerForm) {
            apExtra->SetOwnership(pOwnerForm);
            int32_t FormType = pOwnerForm->GetFormType();
            if (FormType == 8) {
                if (this->iOwnerRank != -1) {
                    apExtra->SetRank(this->iOwnerRank);
                }
            }
            else {
                if (FormType == 42) {
                    // The exe tests the union's dword, the whole global pointer here.
                    if (this->pOwnerGlobal) {
                        apExtra->SetGlobal(this->pOwnerGlobal);
                    }
                }
            }
        }
        if (apHealthForm && (!FloatEQ(fHealthMult, 1.0f, 0.0001f))) {
            uint32_t ibasehealth = apHealthForm->GetFormHealth();
            if (ibasehealth) {
                float fhealth = static_cast<double>(ibasehealth) * fHealthMult;
                apExtra->SetHealth(fhealth);
            }
        }
    }
}

TESGlobal* ContainerItemExtra::GetOwnerGlobal() {
    if (pOwnerForm && (pOwnerForm->GetFormType() == NPC__ID)) {
        // The exe returns the union's dword: the whole global pointer here.
        return pOwnerGlobal;
    }
    else {
        return nullptr;
    }
}

// The whole union, the global pointer's high half included.
ContainerItemExtra::ContainerItemExtra() : pOwnerForm(nullptr), pOwnerGlobal(nullptr), fHealthMult(1.0f) {}

bool FloatEQ(float aX, float aY, float afPrecision) {
    if (afPrecision < 0.0) {
        return false;
    }
    if (!(aY < static_cast<double>(aX))) {
        return afPrecision > (aY - aX);
    }
    return afPrecision > (aX - aY);
}

// An entry is loaded with the owner's raw form ID in pOwnerForm and the global's in the union; resolves both through
// the form tables. The form ID is fetched before the editor ID, so the two are hoisted in that order.
void ContainerItemExtra::InitItem(TESForm* apOwner) {
    if (pOwnerForm) {
        // The field still holds the owner's raw form ID.
        uint32_t uiOwnerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwnerForm));
        TESForm::AddCompileIndex(uiOwnerID, apOwner->GetFile(-1));
        pOwnerForm = TESForm::GetFormByNumericID(uiOwnerID);
        if (!pOwnerForm) {
            uint32_t uiFormID = apOwner->GetFormID();
            const char* pEditorID = apOwner->GetFormEditorID();
            Warning(
                "MASTERFILE: Failed to find owner form (%08X) in init for ContainerItemExtra for form '%s' "
                "(%08X).",
                uiOwnerID,
                pEditorID,
                uiFormID
            );
        }
    }
    if (!pOwnerForm) {
        iUnion = 0;
    }
    else if (pOwnerForm->GetFormType() == NPC__ID && iUnion != 0) {
        uint32_t uiGlobalID = iUnion;
        TESForm::AddCompileIndex(uiGlobalID, apOwner->GetFile(-1));
        pOwnerGlobal = dynamic_cast<TESGlobal*>(TESForm::GetFormByNumericID(uiGlobalID));
        if (!pOwnerGlobal) {
            uint32_t uiFormID = apOwner->GetFormID();
            const char* pEditorID = apOwner->GetFormEditorID();
            Warning(
                "MASTERFILE: Failed to find owner global (%08X) in init for ContainerItemExtra for form '%s' "
                "(%08X).",
                uiGlobalID,
                pEditorID,
                uiFormID
            );
        }
    }
}

bool ContainerItemExtra::Load(TESFile* apFile) {
    CONTAINER_ITEM_EXTRA loaddata;
    bool bsuccess = false;
    if (apFile && (apFile->GetTESChunk() == COED_ID)) {
        apFile->GetChunkData(&loaddata, 12u);
        if (apFile->QEndian()) {
            loaddata.Endian();
        }
        // The owner's form ID goes into pOwnerForm, which InitItem resolves.
        pOwnerForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(loaddata.iOwnerID));
        iUnion = loaddata.iUnion;
        fHealthMult = loaddata.fHealthMult;
        return true;
    }

    return bsuccess;
}

void ContainerItemExtra::Copy(ContainerItemExtra* apCopy) {
    if (apCopy) {
        *this = *apCopy;
    }
}

bool ContainerItemExtra::IsDefault() {
    return !pOwnerForm && fHealthMult == 1.0;
}

bool ContainerItemExtra::Compare(ContainerItemExtra* apCompare) {
    if (!apCompare) {
        return true;
    }
    if (pOwnerForm != apCompare->pOwnerForm) {
        return true;
    }
    if (pOwnerGlobal == apCompare->pOwnerGlobal) {
        return apCompare->fHealthMult != fHealthMult;
    }
    return true;
}

// Writes the entry as a COED chunk: the owner's form ID, then the owner global's form ID or the raw union (the
// faction rank), then the health; nothing when the entry has the default owner and health.
void ContainerItemExtra::Save() {
    if (IsDefault()) {
        return;
    }
    CONTAINER_ITEM_EXTRA kData;
    if (pOwnerForm) {
        kData.iOwnerID = pOwnerForm->GetFormID();
        TESGlobal* pGlobal = GetOwnerGlobal();
        if (pGlobal) {
            kData.iUnion = pGlobal->GetFormID();
        }
        else {
            kData.iUnion = iUnion;
        }
    }
    else {
        kData.iOwnerID = 0;
        kData.iUnion = 0;
    }
    kData.fHealthMult = fHealthMult;
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(COED_ID, &kData, sizeof(kData));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
}

void ContainerItemExtra::AdjustHealthPercent(float afHealthPerc) {
    fHealthMult = BSMAX(0.0f, BSMIN(1.0f, afHealthPerc));
}

void CONTAINER_ITEM_EXTRA::Endian() {
    ENDIAN(iOwnerID, false);
    ENDIAN(iUnion, false);
    ENDIAN(fHealthMult, false);
}
