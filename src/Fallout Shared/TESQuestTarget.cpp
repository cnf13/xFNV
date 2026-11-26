#include "Fallout Shared/TESQuestTarget.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/rtti.h"

void QSTA_DATA::Endian() {
    BSEndian(uiTargetRef, false);
}

TESQuestTarget::TESQuestTarget() : cFlags(0), m_pTargetRef(nullptr) {}

TESQuestTarget::~TESQuestTarget() {}

TESObjectREFR* TESQuestTarget::GetReference(bool abAllowPickUpActor) {
    TESObjectREFR* pTarget = m_pTargetRef;
    if (pTarget && pTarget->TESForm::GetDelete() && abAllowPickUpActor) {
        TESObjectREFR* pReference = pTarget->TESObjectREFR::GetExtra()->GetReferencePointer();
        if (pReference) {
            return pReference;
        }
    }

    return pTarget;
}

bool TESQuestTarget::GetIgnoreLocks() {
    return (cFlags & FLAG_IGNORE_LOCKS) != 0;
}

// A QSTA chunk: the target reference and the flag byte, byte-swapped on a big-endian file.
void TESQuestTarget::Load(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == QSTA_ID) {
        QSTA_DATA kData;
        ourMemset(&kData, 0, sizeof(kData));
        apFile->GetChunkData(&kData, 0);
        if (apFile->QEndian()) {
            kData.Endian();
        }
        // Copies the chunk's raw 4-byte reference slot straight into the pointer field: the value is raw file data,
        // not a typed object.
        m_pTargetRef = reinterpret_cast<TESObjectREFR*>(kData.uiTargetRef);
        cFlags = kData.cFlags;
    }
}

void TESQuestTarget::InitItem(TESForm* apOwner) {
    if (m_pTargetRef) {
        uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_pTargetRef));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempID, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempID);
        m_pTargetRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (!m_pTargetRef) {
            const char* pEditorID = apOwner->GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find target reference (%08X) on quest (%08X) '%s'.",
                tempID,
                apOwner->iFormID,
                pEditorID
            );
        }
    }
    else {
        const char* pEditorID = apOwner->GetFormEditorID();
        Warning("MASTERFILE: No reference on target for quest (%08X) '%s'.", apOwner->iFormID, pEditorID);
    }
    objConditions.InitItem(apOwner);
}

void TESQuestTarget::operator delete(void* apMemory) {
    ourFree(apMemory);
}
