#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"

#include "BSCore/MemoryManager.h"
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
    TESObjectREFR* apRef = static_cast<TESObjectREFR*>(apForm);
    uint8_t bFirstPerson = 0;
    if (static_cast<void*>(apRef) == static_cast<void*>(pPlayer->GetObjectReference())) {
        bFirstPerson = 1;
        apRef = pPlayer;
    }
    BGSLoadFormBuffer* pLoadBuffer = Create(apRef);
    if (apRef->IsReference()) {
        if (apRef->IsActor()) {
            Animation* pAnimation = nullptr;
            if (apRef == pPlayer) {
                pAnimation = pPlayer->GetAnimation(bFirstPerson);
            }
            else {
                pAnimation = apRef->GetAnimation();
            }
            if (pAnimation) {
                pAnimation->LoadGameBGS(pLoadBuffer);
            }
            else {
                uint32_t uiFormID = apRef->GetFormID();
                SaveGameWarning(
                    "ANIMATION: Actor '%s' %08X with queued animation does not have one to load",
                    apRef->GetFormEditorID(), uiFormID
                );
            }
        }
        else {
            apRef->LoadAnimation(pLoadBuffer);
        }
    }
    else {
        uint32_t uiFormID = apRef->GetFormID();
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued havok data is not a reference", apRef->GetFormEditorID(), uiFormID
        );
    }
    if (pLoadBuffer) {
        delete pLoadBuffer;
    }
}

// The form ID is fetched before the editor ID.
void BGSLoadGameSubBuffer::LoadHavokData(TESForm* apForm) {
    TESObjectREFR* apRef = static_cast<TESObjectREFR*>(apForm);
    BGSLoadFormBuffer* pLoadBuffer = Create(apRef);
    if (apRef->IsReference()) {
        apRef->LoadHavokDataBGS(pLoadBuffer);
    }
    else {
        uint32_t uiFormID = apRef->GetFormID();
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued havok data is not a reference", apRef->GetFormEditorID(), uiFormID
        );
    }
    if (pLoadBuffer) {
        delete pLoadBuffer;
    }
}

// The change flags are built in their argument slot.
void BGSLoadGameSubBuffer::SaveAnimation(TESForm* apForm) {
    if (apForm->IsReference()) {
        TESObjectREFR* pref = static_cast<TESObjectREFR*>(apForm);
        BGSSaveFormBuffer saveFormBuffer;
        int32_t iMinorVersion = pSaveLoadManager->GetMinorVersion();
        ENUM_FORM_ID eFormType = apForm->GetFormType();
        BGSChangeFlags kChangeFlags;
        kChangeFlags.~BGSChangeFlags();
        ::new (&kChangeFlags) BGSChangeFlags(4);
        saveFormBuffer.SetHeader(apForm->iFormID, kChangeFlags, eFormType, iMinorVersion);
        saveFormBuffer.pForm = apForm;
        pref->SaveAnimation(&saveFormBuffer);
        this->CopyBuffer(&saveFormBuffer);
    }
    else {
        uint32_t uiFormID = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X passed to BGSLoadGameSubBuffer::SaveAnimation() is not a reference",
            apForm->GetFormEditorID(), uiFormID
        );
    }
}

// The warning names SaveAnimation rather than SaveHavokData, as the game does.
void BGSLoadGameSubBuffer::SaveHavokData(TESForm* apForm) {
    if (apForm->IsReference()) {
        TESObjectREFR* pref = static_cast<TESObjectREFR*>(apForm);
        BGSSaveFormBuffer saveFormBuffer;
        int32_t iMinorVersion = pSaveLoadManager->GetMinorVersion();
        ENUM_FORM_ID eFormType = apForm->GetFormType();
        BGSChangeFlags kChangeFlags;
        kChangeFlags.~BGSChangeFlags();
        ::new (&kChangeFlags) BGSChangeFlags(4);
        saveFormBuffer.SetHeader(apForm->iFormID, kChangeFlags, eFormType, iMinorVersion);
        saveFormBuffer.pForm = apForm;
        pref->SaveHavokData(&saveFormBuffer);
        this->CopyBuffer(&saveFormBuffer);
    }
    else {
        uint32_t uiFormID = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X passed to BGSLoadGameSubBuffer::SaveAnimation() is not a reference",
            apForm->GetFormEditorID(), uiFormID
        );
    }
}

void BGSLoadGameSubBuffer::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t uiSize;
    char* apBuffer = pBuffer + 1;
    uiSize = 0;
    char* apBuffera = &apBuffer[LoadVariableSizedValueFromBuffer(apBuffer, uiSize)];
    apSaveGameBuffer->SaveVariableSizedValue(uiSize);
    apSaveGameBuffer->SaveDataToBuffer(apBuffera, uiSize, true);
}

BGSLoadGameSubBuffer::operator char*() {
    return pBuffer;
}

void BGSLoadGameSubBuffer::DeleteBuffer() {
    ourFree(pBuffer);
    pBuffer = nullptr;
}

BGSLoadFormBuffer* BGSLoadGameSubBuffer::Create(TESForm* apForm) {
    BGSLoadFormBuffer* ploadFormBuffer;
    BGSLoadFormBuffer* apMemory = static_cast<BGSLoadFormBuffer*>(ourMalloc(sizeof(BGSLoadFormBuffer)));
    if (apMemory) {
        ploadFormBuffer = ::new (apMemory) BGSLoadFormBuffer;
    }
    else {
        ploadFormBuffer = nullptr;
    }
    char* pheader = pBuffer;
    ploadFormBuffer->pForm = apForm;
    ploadFormBuffer->Header.cVersion = *pheader;
    uint32_t iposition = 1;
    char* padvancedBuffer = pBuffer + 1;
    uint32_t isize = 0;
    uint32_t v2 = LoadVariableSizedValueFromBuffer(padvancedBuffer, isize);
    iposition += v2;
    ploadFormBuffer->SetBuffer(pBuffer, iposition + isize, iposition);
    pBuffer = nullptr;
    return ploadFormBuffer;
}

void BGSLoadGameSubBuffer::LoadCharController(TESForm* apForm) {
    BGSLoadFormBuffer* ploadFormBuffer = Create(apForm);
    if (apForm->IsReference()) {
        if (apForm->IsMobileObject()) {
            if (static_cast<MobileObject*>(apForm)->MobileObject::GetCharController()) {
                MiddleHighProcess* pmiddleHighProcess =
                    static_cast<MiddleHighProcess*>(static_cast<MobileObject*>(apForm)->GetCurrentAIProcess());
                pmiddleHighProcess->LoadCharController(ploadFormBuffer);
            }
            else {
                int32_t CastingType = apForm->iFormID;
                SaveGameWarning(
                    "SAVELOAD: Mobile Object '%s' %08X with queued char controller does not have one to "
                    "load",
                    apForm->GetFormEditorID(), CastingType
                );
            }
        }
        else {
            int32_t v3 = apForm->iFormID;
            SaveGameWarning(
                "SAVELOAD: Reference '%s' %08X with queued char controller is not a mobile object",
                apForm->GetFormEditorID(), v3
            );
        }
    }
    else {
        int32_t v4 = apForm->iFormID;
        SaveGameWarning(
            "SAVELOAD: Form '%s' %08X with queued char controller is not a reference", apForm->GetFormEditorID(), v4
        );
    }
    if (ploadFormBuffer) {
        delete ploadFormBuffer;
    }
}

void BGSLoadGameSubBuffer::CopyBuffer(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isubBufferSize = apSaveGameBuffer->iBufferPosition;
    if (isubBufferSize) {
        uint32_t ivariableValueSize = GetVariableSizedValueSize(isubBufferSize);
        pBuffer = static_cast<char*>(ourMalloc(isubBufferSize + ivariableValueSize + 1));
        char* pbuffer = pBuffer;
        char* pheader = pBuffer;
        *pheader = apSaveGameBuffer->Header.cVersion;
        SaveVariableSizedValueToBuffer(++pbuffer, isubBufferSize);
        ourMemcpy(&pbuffer[ivariableValueSize], apSaveGameBuffer->pBuffer, isubBufferSize);
    }
}

void BGSLoadGameSubBuffer::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t isubBufferSize = apLoadGameBuffer->LoadVariableSizedValue();
    if (isubBufferSize) {
        uint32_t ivariableValueSize = GetVariableSizedValueSize(isubBufferSize);
        char* pbuffer = static_cast<char*>(ourMalloc(isubBufferSize + ivariableValueSize + 1));
        pBuffer = pbuffer;
        *pbuffer++ = apLoadGameBuffer->GetVersion();
        SaveVariableSizedValueToBuffer(pbuffer, isubBufferSize);
        char* NextPos = apLoadGameBuffer->GetNextPos();
        ourMemcpy(&pbuffer[ivariableValueSize], NextPos, isubBufferSize);
        apLoadGameBuffer->AdvanceBuffer(isubBufferSize);
    }
}

// The maps are built and destroyed by NiTMap's own constructor and destructor; the destructor frees the buffers of
// the queued sub-buffers, which the maps do not own.
BGSSaveLoadQueuedSubBufferMap::BGSSaveLoadQueuedSubBufferMap() {}

BGSSaveLoadQueuedSubBufferMap::~BGSSaveLoadQueuedSubBufferMap() {
    for (uint32_t i = 0; i < 3; ++i) {
        NiTMapItem<TESForm*, BGSLoadGameSubBuffer>* it = QueuedSubBuffers[i].GetFirstPos();
        while (it) {
            TESForm* pForm = nullptr;
            BGSLoadGameSubBuffer kSubBuffer;
            QueuedSubBuffers[i].GetNext(it, pForm, kSubBuffer);
            kSubBuffer.DeleteBuffer();
        }
    }
}

void BGSSaveLoadQueuedSubBufferMap::LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_TYPE aeType) {
    BGSLoadGameSubBuffer subBuffer;
    NiTMapItem<TESForm*, BGSLoadGameSubBuffer>* it = QueuedSubBuffers[aeType].GetFirstPos();
    while (it) {
        TESForm* pref_2 = nullptr;
        subBuffer.~BGSLoadGameSubBuffer();
        ::new (&subBuffer) BGSLoadGameSubBuffer;
        QueuedSubBuffers[aeType].GetNext(it, pref_2, subBuffer);
        bool bLoaded3D = true;
        TESObjectREFR* pref = nullptr;
        TESObjectREFR* v4 = nullptr;
        if (pref_2->IsReference()) {
            pref = static_cast<TESObjectREFR*>(pref_2);
            bLoaded3D = static_cast<TESObjectREFR*>(pref_2)->TESObjectREFR::Get3D() != nullptr;
            if (pref->IsActor()) {
                v4 = pref;
            }
        }
        if (bLoaded3D) {
            if (aeType) {
                if (aeType == QUEUED_SUB_BUFFER_CHAR_CONTROLLER) {
                    subBuffer.LoadCharController(pref_2);
                }
                else {
                    if (aeType == QUEUED_SUB_BUFFER_HAVOK_DATA) {
                        subBuffer.LoadHavokData(pref_2);
                    }
                }
            }
            else {
                subBuffer.LoadAnimation(pref_2);
            }
        }
        else {
            if (aeType) {
                if (aeType == QUEUED_SUB_BUFFER_CHAR_CONTROLLER) {
                LABEL_19:;
                    subBuffer.DeleteBuffer();
                }
                else {
                    if ((aeType == QUEUED_SUB_BUFFER_HAVOK_DATA) && (pref)) {
                        (&pref->m_Extra)->SetSavedHavokData(subBuffer.pBuffer);
                    }
                }
            }
            else {
                if (v4) {
                    goto LABEL_19;
                }
                if (pref) {
                    (&pref->m_Extra)->SetSavedAnimation(subBuffer.pBuffer);
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
    subBuffer.~BGSLoadGameSubBuffer();
    ::new (&subBuffer) BGSLoadGameSubBuffer;
    subBuffer.LoadGame(apLoadFormBuffer);
    if (subBuffer.pBuffer) {
        TESForm* pform;
        if (apParentForm) {
            pform = apParentForm;
        }
        else {
            pform = apLoadFormBuffer->GetForm();
        }
        QueuedSubBuffers[aeType].SetAt(pform, subBuffer);
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
