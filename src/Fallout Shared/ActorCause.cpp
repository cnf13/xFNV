#include "Fallout Shared/ActorCause.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

BSSimpleList<ActorCause*> ActorCause::kCauseList;

ActorCause::ActorCause()
  : pActor(nullptr), pWitnesses(nullptr), kOrigin(NiPoint3::ZERO), iActorCauseID(iActorCauseIDCounter++), iRefCount(0) {
    ActorCause* pCause = this;
    kCauseList.AddHead(pCause);
}

ActorCause::ActorCause(Actor* apActor)
  : pActor(apActor), pWitnesses(nullptr), kOrigin(apActor ? *apActor->QLocationOnReference() : NiPoint3::ZERO),
    iActorCauseID(iActorCauseIDCounter++), iRefCount(0) {
    BaseProcess* pProcess = pActor ? pActor->pCurrentProcess : nullptr;
    if (pProcess) {
        pWitnesses = pProcess->GetWhoDetectsMe(pActor);
    }
    ActorCause* pCause = this;
    kCauseList.AddHead(pCause);
}

ActorCause::~ActorCause() {
    pActor = nullptr;
    while (pWitnesses && !pWitnesses->IsEmpty()) {
        ourFree(pWitnesses->m_item);
        pWitnesses->RemoveHead();
    }
    delete pWitnesses;
    pWitnesses = nullptr;
    // While a save loads, a cause whose ID is -1 is not in kCauseList.
    bool bInList = true;
    if (pSaveLoadGame->GetSaveGameLoading()) {
        bInList = iActorCauseID != 0xFFFFFFFF;
    }
    if (bInList) {
        ActorCause* pCause = this;
        kCauseList.Remove(pCause);
    }
}

void ActorCause::DecRefCount() {
    if (!NiAtomicDecrement(*reinterpret_cast<int32_t*>(&iRefCount)) && this) {
        delete this;
    }
}

void ActorCause::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&kOrigin, sizeof(kOrigin));
    apLoadGameBuffer->LoadData(&iActorCauseID, sizeof(iActorCauseID));
    apLoadGameBuffer->LoadFormID(pActor);
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiSaveGameCount) {
        if (pWitnesses) {
            pWitnesses->RemoveAll();
        }
        else {
            pWitnesses = new BSSimpleList<DetectionState*>;
        }
        for (uint32_t uiLoop = 0; uiLoop < uiSaveGameCount; ++uiLoop) {
            DetectionState* pDetectionState = new DetectionState;
            pDetectionState->LoadGame(apLoadGameBuffer);
            pWitnesses->AddHead(pDetectionState);
        }
    }
}

void ActorCause::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&kOrigin), sizeof(kOrigin), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iActorCauseID), sizeof(iActorCauseID), 0);
    apSaveGameBuffer->SaveFormID(pActor, 0);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<DetectionState*>* pNode = pWitnesses; pNode;) {
        DetectionState* pData = pNode->GetItem();
        if (pData) {
            pData->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

NiPoint3* ActorCause::FreeDetectionList() {
    pActor = nullptr;
    while (this->pWitnesses && !this->pWitnesses->IsEmpty()) {
        ourFree(this->pWitnesses->m_item);
        this->pWitnesses->RemoveHead();
    }
    delete this->pWitnesses;
    this->pWitnesses = nullptr;
    kOrigin = NiPoint3::ZERO;
    return &kOrigin;
}

void ActorCause::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    Actor* pLoadedActor = nullptr;
    if (pActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        pLoadedActor = dynamic_cast<Actor*>(pForm);
    }
    pActor = pLoadedActor;
    for (DetectionState* pDetectionState : *pWitnesses) {
        if (pDetectionState) {
            pDetectionState->InitLoadGame();
        }
    }
}

void ActorCause::InitLoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSSimpleList<ActorCause*>* pList = &ActorCause::kCauseList;
    BSSimpleList<ActorCause*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        ActorCause* pActorCause = pList->m_item;
        bool bRemove = false;
        if (pActorCause) {
            // Tests only the low 16 bits of the count.
            if (static_cast<uint16_t>(pActorCause->iRefCount)) {
                pActorCause->InitLoadGame(apLoadGameBuffer);
            }
            else {
                pActorCause->iActorCauseID = -1;
                delete pActorCause;
                bRemove = true;
            }
        }
        else {
            bRemove = true;
        }
        if (bRemove) {
            if (pLastList) {
                pLastList->Remove(pList->m_item);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
        }
        else {
            pLastList = pList;
            pList = pList->m_pkNext;
        }
    }
}

void ActorCause::RevertActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    ActorCause::kCauseList.RemoveAll();
}

void ActorCause::LoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(
        static_cast<char*>(static_cast<void*>(&ActorCause::iActorCauseIDCounter)),
        sizeof(ActorCause::iActorCauseIDCounter)
    );
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ActorCause* pCause = new ActorCause;
        pCause->LoadGame(apLoadGameBuffer);
    }
}

void ActorCause::ClearActor(Actor* apActor) {
    BSSimpleList<ActorCause*>* pList = &ActorCause::kCauseList;
    while (pList && !pList->IsEmpty()) {
        ActorCause* pCause = pList->m_item;
        pList = pList->m_pkNext;
        if (pCause && pCause->IsValid()) {
            if (pCause->pActor == apActor) {
                pCause->FreeDetectionList();
            }
            BSSimpleList<DetectionState*>* pDetection = pCause->pWitnesses;
            bool bFound = false;
            while (!bFound && pDetection && pDetection->m_item) {
                DetectionState* pState = pDetection->m_item;
                pDetection = pDetection->m_pkNext;
                if (pState->pActor == apActor) {
                    pCause->pWitnesses->Remove(pState);
                    ourFree(pState);
                    bFound = true;
                }
            }
        }
    }
}

ActorCause* ActorCause::GetActorCauseByID(int32_t aiActorCauseID) {
    if (aiActorCauseID < 0) {
        return nullptr;
    }
    for (ActorCause* pCause : ActorCause::kCauseList) {
        if (pCause && pCause->iActorCauseID == aiActorCauseID) {
            return pCause;
        }
    }
    return nullptr;
}

void ActorCause::SaveActorCausesList(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&ActorCause::iActorCauseIDCounter),
        sizeof(ActorCause::iActorCauseIDCounter),
        0
    );
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (ActorCause* pCause : ActorCause::kCauseList) {
        if (pCause) {
            pCause->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}
