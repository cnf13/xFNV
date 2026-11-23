#include "Fallout Shared/BGSIdleCollection.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

void BGSIdleCollection::Load(TESFile* apFile) {
    CHUNK_ID eChunkID = apFile->GetTESChunk();
    if (BGSIdleCollection::IsValidChunk(eChunkID)) {
        if (eChunkID > IDLF_ID) {
            if (eChunkID == IDLT_ID) {
                apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fTimerCheckForIdle));
            }
        }
        else {
            switch (eChunkID) {
                case IDLF_ID: {
                    apFile->GetChunkData(&cIdleFlags, 1u);
                    break;
                }
                case IDLA_ID: {
                    if (pIdleArray) {
                        uint32_t uiSize = 4 * cIdleCount;
                        if (apFile->GetChunkSize() == uiSize) {
                            apFile->GetChunkData(pIdleArray, uiSize);
                            // The file keeps a 4-byte form ID for each idle, resolved by InitItem.
                            TESFile::WidenFormIDRecords(pIdleArray, cIdleCount, 4, sizeof(TESIdleForm*), {{0, 0}});
                            if (apFile->QEndian()) {
                                for (int32_t ii = 0; ii < cIdleCount; ++ii) {
                                    ENDIAN<TESIdleForm*>(pIdleArray[ii], false);
                                }
                            }
                        }
                    }
                    break;
                }
                case IDLC_ID: {
                    if (apFile->GetChunkSize() == 1) {
                        apFile->GetChunkData(&cIdleCount, 1u);
                    }
                    else {
                        uint32_t uiCount = 0;
                        apFile->GetChunkData<uint32_t>(uiCount);
                        cIdleCount = uiCount;
                    }
                    if (cIdleCount) {
                        if (pIdleArray) {
                            ourFree(pIdleArray);
                        }
                        pIdleArray = static_cast<TESIdleForm**>(ourMalloc(
                            cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*) ? sizeof(TESIdleForm*) * cIdleCount : SIZE_MAX
                        ));
                        ourMemset(pIdleArray, 0, sizeof(TESIdleForm*) * cIdleCount);
                    }
                    break;
                }
            }
        }
    }
}

BGSIdleCollection::BGSIdleCollection() {
    BGSIdleCollection::InitializeDataComponent();
}

BGSIdleCollection::~BGSIdleCollection() {
    BGSIdleCollection::ClearDataComponent();
}

void BGSIdleCollection::InitializeDataComponent() {
    cIdleFlags = 0;
    cIdleCount = 0;
    pIdleArray = nullptr;
    fTimerCheckForIdle = 0.0f;
}

// Frees the idle array and empties the collection.
void BGSIdleCollection::ClearDataComponent() {
    ourFree(pIdleArray);
    cIdleCount = 0;
    pIdleArray = nullptr;
}

void BGSIdleCollection::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSIdleCollection* pCopy = dynamic_cast<BGSIdleCollection*>(apCopyFC);
    if (pCopy) {
        cIdleFlags = pCopy->cIdleFlags;
        ourFree(pIdleArray);
        pIdleArray = nullptr;
        cIdleCount = 0;
        if (pCopy->cIdleCount) {
            pIdleArray = static_cast<TESIdleForm**>(ourMalloc(
                pCopy->cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*) ? sizeof(TESIdleForm*) * pCopy->cIdleCount
                                                                     : SIZE_MAX
            ));
            ourMemcpy(pIdleArray, pCopy->pIdleArray, sizeof(TESIdleForm*) * pCopy->cIdleCount);
            cIdleCount = pCopy->cIdleCount;
        }
        fTimerCheckForIdle = pCopy->fTimerCheckForIdle;
    }
}

bool BGSIdleCollection::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSIdleCollection* pCompare = dynamic_cast<BGSIdleCollection*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (cIdleFlags != pCompare->cIdleFlags) {
        return true;
    }
    if (cIdleCount != pCompare->cIdleCount) {
        return true;
    }
    if (pCompare->fTimerCheckForIdle != fTimerCheckForIdle) {
        return true;
    }
    return pIdleArray &&
        pCompare->pIdleArray &&
        std::memcmp(pIdleArray, pCompare->pIdleArray, sizeof(TESIdleForm*) * cIdleCount);
}

void BGSIdleCollection::InitItem(TESForm* apOwner) {
    for (uint32_t i = 0; i < cIdleCount; ++i) {
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIdleArray[i]));
        TESForm::AddCompileIndex(uiID, apOwner->GetFile(-1));
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        TESIdleForm* pIdle = dynamic_cast<TESIdleForm*>(pForm);
        if (pIdle) {
            if (pIdle->CheckIdleFile()) {
                pIdleArray[i] = pIdle;
            }
            else {
                int32_t iOwnerFormID = apOwner->iFormID;
                const char* pOwnerEditorID = apOwner->GetFormEditorID();
                Warning(
                    "MASTERFILE: MBGSIdleCollection has an idle '%s' without an animation file on form '%s' "
                    "(%08X).",
                    pIdle->GetFormEditorID(),
                    pOwnerEditorID,
                    iOwnerFormID
                );
                RemoveIdleAtIndex(i--);
            }
        }
        else {
            int32_t iOwnerFormID = apOwner->iFormID;
            Warning(
                "MASTERFILE: Unable to find Idle (%08X) on form '%s' (%08X).",
                uiID,
                apOwner->GetFormEditorID(),
                iOwnerFormID
            );
            RemoveIdleAtIndex(i--);
        }
    }
}

// The record's idle chunks: the flags, the count, the check timer, then one chunk of the idles' form IDs.
void BGSIdleCollection::Save() {
    if (!cIdleCount) {
        return;
    }
    TESForm::AddChunk(IDLF_ID, cIdleFlags);
    TESForm::AddChunk(IDLC_ID, cIdleCount);
    TESForm::AddChunk(IDLT_ID, fTimerCheckForIdle);
    // A 16-byte aligned stack buffer of one form ID per idle.
    uint32_t* pIdleIDs = static_cast<uint32_t*>(XFNV_ALLOCA16(4 * cIdleCount));
    for (uint32_t i = 0; i < cIdleCount; ++i) {
        pIdleIDs[i] = pIdleArray[i] ? pIdleArray[i]->GetFormID() : 0;
    }
    TESForm::AddChunkArray(IDLA_ID, pIdleIDs, cIdleCount);
}

int32_t BGSIdleCollection::GetIdleCount() const {
    return cIdleCount;
}

bool BGSIdleCollection::GetDoOnce() {
    return (cIdleFlags & FLAG_DO_ONCE) != 0;
}

BGSIdleCollection::PICK_TYPE BGSIdleCollection::GetPickType() {
    return static_cast<BGSIdleCollection::PICK_TYPE>((cIdleFlags & FLAG_RUN_IN_SEQUENCE) != 0);
}

void BGSIdleCollection::RemoveIdleAtIndex(uint32_t aiIndex) {
    if (aiIndex < cIdleCount) {
        if (cIdleCount == 1) {
            cIdleCount = 0;
            ourFree(pIdleArray);
            pIdleArray = nullptr;
        }
        else {
            --cIdleCount;
            TESIdleForm** pArray = static_cast<TESIdleForm**>(
                ourMalloc(cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*) ? sizeof(TESIdleForm*) * cIdleCount : SIZE_MAX)
            );
            // One signed count serves both copies: the index, then the entries after it.
            int32_t iCopyCount = aiIndex;
            if (iCopyCount > 0) {
                ourMemcpy(pArray, pIdleArray, sizeof(TESIdleForm*) * iCopyCount);
            }
            iCopyCount = cIdleCount - aiIndex;
            if (iCopyCount > 0) {
                ourMemcpy(&pArray[aiIndex], &pIdleArray[aiIndex + 1], sizeof(TESIdleForm*) * iCopyCount);
            }
            ourFree(pIdleArray);
            pIdleArray = pArray;
        }
    }
}

bool BGSIdleCollection::HasIdleForActor(Actor* apActor, TESObjectREFR* apTargetRef) {
    if (apActor) {
        for (int32_t i = 0; i < cIdleCount; ++i) {
            TESIdleForm* pIdle = pIdleArray[i];
            if (pIdle && apActor->CanUseIdle(pIdle) && pIdle->CheckConditions(apActor, apTargetRef, true)) {
                return true;
            }
        }
    }
    return false;
}

uint32_t BGSIdleCollection::GetIdlesForActor(
    Actor* apActor,
    TESObjectREFR* apTargetRef,
    BSSimpleArray<TESIdleForm*, 1024>* apOutIdleArray
) {
    uint32_t uiIdleCount = 0;
    if (apActor) {
        uint32_t uiArraySize = 0;
        if (apOutIdleArray) {
            uiArraySize = apOutIdleArray->iReservedSize;
        }
        for (int32_t i = 0; i < cIdleCount; ++i) {
            TESIdleForm* pIdle = pIdleArray[i];
            if (pIdle && apActor->CanUseIdle(pIdle) && pIdle->CheckConditions(apActor, apTargetRef, true)) {
                if (apOutIdleArray) {
                    if (uiIdleCount >= uiArraySize) {
                        apOutIdleArray->Add(pIdle);
                    }
                    else {
                        apOutIdleArray->SetAt(uiIdleCount, pIdle);
                    }
                }
                ++uiIdleCount;
            }
        }
    }
    return uiIdleCount;
}

bool BGSIdleCollection::IsValidChunk(CHUNK_ID aeChunk) {
    if (static_cast<int32_t>(aeChunk) > static_cast<int32_t>(IDLF_ID)) {
        return aeChunk == IDLN_ID || aeChunk == IDLT_ID;
    }
    return aeChunk == IDLF_ID || aeChunk == IDLA_ID || aeChunk == IDLB_ID || aeChunk == IDLC_ID;
}
