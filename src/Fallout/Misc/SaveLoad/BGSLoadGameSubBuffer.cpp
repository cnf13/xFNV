#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

BGSLoadGameSubBuffer::BGSLoadGameSubBuffer() : pBuffer(nullptr) {}

BGSLoadGameSubBuffer::BGSLoadGameSubBuffer(char* apBuffer) : pBuffer(apBuffer) {}

// The player's saved animation may be keyed by the player's base object; that selects the first-person animation. The
// form ID is fetched before the editor ID.
void BGSLoadGameSubBuffer::LoadAnimation(TESForm* apForm) {
    TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
    uint8_t bFirstPerson = 0;
    if (static_cast<void*>(pRef) == static_cast<void*>(pPlayer->GetObjectReference())) {
        bFirstPerson = 1;
        pRef = pPlayer;
    }
    BGSLoadFormBuffer* pLoadBuffer = Create(pRef);
    if (pRef->IsReference()) {
        if (pRef->IsActor()) {
            Animation* pAnimation = nullptr;
            if (pRef == pPlayer) {
                pAnimation = pPlayer->GetAnimation(bFirstPerson);
            }
            else {
                pAnimation = pRef->GetAnimation();
            }
            if (pAnimation) {
                pAnimation->LoadGame(pLoadBuffer);
            }
            else {
                uint32_t uiFormID = pRef->GetFormID();
                SaveGameWarning(
                    "ANIMATION: Actor '%s' %08X with queued animation does not have one to load",
                    pRef->GetFormEditorID(),
                    uiFormID
                );
            }
        }
        else {
            pRef->LoadAnimation(pLoadBuffer);
        }
    }
    else {
        uint32_t uiFormID = pRef->GetFormID();
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued havok data is not a reference",
            pRef->GetFormEditorID(),
            uiFormID
        );
    }
    if (pLoadBuffer) {
        delete pLoadBuffer;
    }
}

// The form ID is fetched before the editor ID.
void BGSLoadGameSubBuffer::LoadHavokData(TESForm* apForm) {
    TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
    BGSLoadFormBuffer* pLoadBuffer = Create(pRef);
    if (pRef->IsReference()) {
        pRef->LoadHavokData(pLoadBuffer);
    }
    else {
        uint32_t uiFormID = pRef->GetFormID();
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued havok data is not a reference",
            pRef->GetFormEditorID(),
            uiFormID
        );
    }
    if (pLoadBuffer) {
        delete pLoadBuffer;
    }
}

// The change flags are built in their argument slot.
void BGSLoadGameSubBuffer::SaveAnimation(TESForm* apForm) {
    if (apForm->IsReference()) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
        BGSSaveFormBuffer saveFormBuffer;
        int32_t iMinorVersion = pSaveLoadManager->GetMinorVersion();
        ENUM_FORM_ID eFormType = apForm->GetFormType();
        BGSChangeFlags kChangeFlags(4);
        saveFormBuffer.SetHeader(apForm->iFormID, kChangeFlags, eFormType, iMinorVersion);
        saveFormBuffer.pForm = apForm;
        pRef->SaveAnimation(&saveFormBuffer);
        this->CopyBuffer(&saveFormBuffer);
    }
    else {
        uint32_t uiFormID = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X passed to BGSLoadGameSubBuffer::SaveAnimation() is not a reference",
            apForm->GetFormEditorID(),
            uiFormID
        );
    }
}

// The warning names SaveAnimation rather than SaveHavokData.
void BGSLoadGameSubBuffer::SaveHavokData(TESForm* apForm) {
    if (apForm->IsReference()) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
        BGSSaveFormBuffer saveFormBuffer;
        int32_t iMinorVersion = pSaveLoadManager->GetMinorVersion();
        ENUM_FORM_ID eFormType = apForm->GetFormType();
        BGSChangeFlags kChangeFlags(4);
        saveFormBuffer.SetHeader(apForm->iFormID, kChangeFlags, eFormType, iMinorVersion);
        saveFormBuffer.pForm = apForm;
        pRef->SaveHavokData(&saveFormBuffer);
        this->CopyBuffer(&saveFormBuffer);
    }
    else {
        uint32_t uiFormID = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X passed to BGSLoadGameSubBuffer::SaveAnimation() is not a reference",
            apForm->GetFormEditorID(),
            uiFormID
        );
    }
}

void BGSLoadGameSubBuffer::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    char* pPos = pBuffer + 1;
    uint32_t uiSize = 0;
    pPos = &pPos[LoadVariableSizedValueFromBuffer(pPos, uiSize)];
    apSaveGameBuffer->SaveVariableSizedValue(uiSize);
    apSaveGameBuffer->SaveDataToBuffer(pPos, uiSize, true);
}

BGSLoadGameSubBuffer::operator char*() {
    return pBuffer;
}

void BGSLoadGameSubBuffer::DeleteBuffer() {
    ourFree(pBuffer);
    pBuffer = nullptr;
}

BGSLoadFormBuffer* BGSLoadGameSubBuffer::Create(TESForm* apForm) {
    BGSLoadFormBuffer* pLoadFormBuffer;
    pLoadFormBuffer = new BGSLoadFormBuffer;
    char* pHeader = pBuffer;
    pLoadFormBuffer->pForm = apForm;
    pLoadFormBuffer->Header.cVersion = *pHeader;
    uint32_t uiPosition = 1;
    char* pAdvancedBuffer = pBuffer + 1;
    uint32_t uiSize = 0;
    uiPosition += LoadVariableSizedValueFromBuffer(pAdvancedBuffer, uiSize);
    pLoadFormBuffer->SetBuffer(pBuffer, uiPosition + uiSize, uiPosition);
    pBuffer = nullptr;
    return pLoadFormBuffer;
}

void BGSLoadGameSubBuffer::LoadCharController(TESForm* apForm) {
    BGSLoadFormBuffer* pLoadFormBuffer = Create(apForm);
    if (apForm->IsReference()) {
        if (apForm->IsMobileObject()) {
            if (static_cast<MobileObject*>(apForm)->MobileObject::GetCharController()) {
                MiddleHighProcess* pMiddleHighProcess = static_cast<MiddleHighProcess*>(
                    static_cast<MobileObject*>(apForm)->GetCurrentAIProcess()
                );
                pMiddleHighProcess->LoadCharController(pLoadFormBuffer);
            }
            else {
                int32_t iFormID = apForm->iFormID;
                SaveGameWarning(
                    "SAVELOAD: Mobile Object '%s' %08X with queued char controller does not have one to "
                    "load",
                    apForm->GetFormEditorID(),
                    iFormID
                );
            }
        }
        else {
            int32_t iFormID = apForm->iFormID;
            SaveGameWarning(
                "SAVELOAD: Reference '%s' %08X with queued char controller is not a mobile object",
                apForm->GetFormEditorID(),
                iFormID
            );
        }
    }
    else {
        int32_t iFormID = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued char controller is not a reference",
            apForm->GetFormEditorID(),
            iFormID
        );
    }
    if (pLoadFormBuffer) {
        delete pLoadFormBuffer;
    }
}

void BGSLoadGameSubBuffer::CopyBuffer(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t uiSubBufferSize = apSaveGameBuffer->iBufferPosition;
    if (uiSubBufferSize) {
        uint32_t uiVariableValueSize = GetVariableSizedValueSize(uiSubBufferSize);
        pBuffer = static_cast<char*>(ourMalloc(uiSubBufferSize + uiVariableValueSize + 1));
        char* pbuffer = pBuffer;
        char* pHeader = pBuffer;
        *pHeader = apSaveGameBuffer->Header.cVersion;
        SaveVariableSizedValueToBuffer(++pbuffer, uiSubBufferSize);
        ourMemcpy(&pbuffer[uiVariableValueSize], apSaveGameBuffer->pBuffer, uiSubBufferSize);
    }
}

void BGSLoadGameSubBuffer::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t uiSubBufferSize = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiSubBufferSize) {
        uint32_t uiVariableValueSize = GetVariableSizedValueSize(uiSubBufferSize);
        char* pbuffer = static_cast<char*>(ourMalloc(uiSubBufferSize + uiVariableValueSize + 1));
        pBuffer = pbuffer;
        *pbuffer++ = apLoadGameBuffer->GetVersion();
        SaveVariableSizedValueToBuffer(pbuffer, uiSubBufferSize);
        char* pNextPos = apLoadGameBuffer->GetNextPos();
        ourMemcpy(&pbuffer[uiVariableValueSize], pNextPos, uiSubBufferSize);
        apLoadGameBuffer->AdvanceBuffer(uiSubBufferSize);
    }
}

// The maps are built and destroyed by NiTMap's own constructor and destructor; the destructor frees the buffers of
// the queued sub-buffers, which the maps do not own.
BGSSaveLoadQueuedSubBufferMap::BGSSaveLoadQueuedSubBufferMap() {}

BGSSaveLoadQueuedSubBufferMap::~BGSSaveLoadQueuedSubBufferMap() {
    for (auto& rMap : QueuedSubBuffers) {
        NiTMapItem<TESForm*, BGSLoadGameSubBuffer>* it = rMap.GetFirstPos();
        while (it) {
            TESForm* pForm = nullptr;
            BGSLoadGameSubBuffer kSubBuffer;
            rMap.GetNext(it, pForm, kSubBuffer);
            kSubBuffer.DeleteBuffer();
        }
    }
}

void BGSSaveLoadQueuedSubBufferMap::LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_TYPE aeType) {
    NiTMapItem<TESForm*, BGSLoadGameSubBuffer>* it = QueuedSubBuffers[aeType].GetFirstPos();
    while (it) {
        TESForm* pForm = nullptr;
        BGSLoadGameSubBuffer subBuffer;
        QueuedSubBuffers[aeType].GetNext(it, pForm, subBuffer);
        bool bLoaded3D = true;
        TESObjectREFR* pRef = nullptr;
        TESObjectREFR* pActorRef = nullptr;
        if (pForm->IsReference()) {
            pRef = static_cast<TESObjectREFR*>(pForm);
            bLoaded3D = pRef->TESObjectREFR::Get3D() != nullptr;
            if (pRef->IsActor()) {
                pActorRef = pRef;
            }
        }
        if (bLoaded3D) {
            switch (aeType) {
                case QUEUED_SUB_BUFFER_ANIMATION: {
                    subBuffer.LoadAnimation(pForm);
                    break;
                }
                case QUEUED_SUB_BUFFER_CHAR_CONTROLLER: {
                    subBuffer.LoadCharController(pForm);
                    break;
                }
                case QUEUED_SUB_BUFFER_HAVOK_DATA: {
                    subBuffer.LoadHavokData(pForm);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else {
            switch (aeType) {
                case QUEUED_SUB_BUFFER_ANIMATION: {
                    if (pActorRef) {
                        subBuffer.DeleteBuffer();
                    }
                    else if (pRef) {
                        pRef->m_Extra.SetSavedAnimation(subBuffer.pBuffer);
                    }
                    break;
                }
                case QUEUED_SUB_BUFFER_CHAR_CONTROLLER: {
                    subBuffer.DeleteBuffer();
                    break;
                }
                case QUEUED_SUB_BUFFER_HAVOK_DATA: {
                    if (pRef) {
                        pRef->m_Extra.SetSavedHavokData(subBuffer.pBuffer);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    QueuedSubBuffers[aeType].RemoveAll();
}

void BGSSaveLoadQueuedSubBufferMap::QueueSubBuffer(
    QUEUED_SUB_BUFFER_TYPE aeType,
    BGSLoadFormBuffer* apLoadFormBuffer,
    TESForm* apParentForm
) {
    BGSLoadGameSubBuffer subBuffer;
    subBuffer.LoadGame(apLoadFormBuffer);
    if (subBuffer.pBuffer) {
        TESForm* pForm = apParentForm ? apParentForm : apLoadFormBuffer->GetForm();
        QueuedSubBuffers[aeType].SetAt(pForm, subBuffer);
    }
}

void BGSSaveLoadQueuedSubBufferMap::LoadAllQueuedSubBuffers() {
    LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_ANIMATION);
    LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_CHAR_CONTROLLER);
    LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_HAVOK_DATA);
}

void BGSSaveLoadQueuedSubBufferMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}
