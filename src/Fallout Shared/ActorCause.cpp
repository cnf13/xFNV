#include "Fallout Shared/ActorCause.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

BSSimpleList<ActorCause*> ActorCause::kCauseList;

ActorCause::ActorCause()
  : pActor(nullptr), pWitnesses(nullptr), kOrigin(NiPoint3_ZERO), iActorCauseID(iActorCauseIDCounter++), iRefCount(0) {
    ActorCause* pCause = this;
    kCauseList.AddHead(pCause);
}

ActorCause::ActorCause(Actor* apActor)
  : pActor(apActor), pWitnesses(nullptr), kOrigin(apActor ? *apActor->QLocationOnReference() : NiPoint3_ZERO),
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
    if ((!NiAtomicDecrement(*reinterpret_cast<int32_t*>(&iRefCount))) && (this)) {
        delete this;
    }
}

void ActorCause::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&kOrigin, 12u);
    apLoadGameBuffer->LoadData(&iActorCauseID, 4u);
    apLoadGameBuffer->LoadFormID(pActor);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (isaveGameCount) {
        if (pWitnesses) {
            pWitnesses->RemoveAll();
        }
        else {
            BSSimpleList<DetectionState*>* v3;
            BSSimpleList<DetectionState*>* v6 =
                static_cast<BSSimpleList<DetectionState*>*>(ourMalloc(sizeof(BSSimpleList<DetectionState*>)));
            if (v6) {
                v3 = ::new (v6) BSSimpleList<DetectionState*>;
            }
            else {
                v3 = nullptr;
            }
            pWitnesses = v3;
        }
        for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
            DetectionState* pdetectionState;
            DetectionState* apMemory = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
            if (apMemory) {
                pdetectionState = ::new (apMemory) DetectionState;
            }
            else {
                pdetectionState = nullptr;
            }
            DetectionState* pdetectionState_2 = pdetectionState;
            pdetectionState->LoadGame(apLoadGameBuffer);
            pWitnesses->AddHead(pdetectionState_2);
        }
    }
}

void ActorCause::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&kOrigin), 0xC, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iActorCauseID), 4, 0);
    apSaveGameBuffer->SaveFormID(pActor, 0);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<DetectionState*>* pNode = pWitnesses; pNode;) {
        DetectionState* pData;
        pData = pNode->GetItem();
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
    void* pWitnesses = (void*)this->pWitnesses;
    if (pWitnesses) {
        delete static_cast<BSSimpleList<ActiveEffect*>*>(pWitnesses);
    }
    this->pWitnesses = nullptr;
    kOrigin = NiPoint3_ZERO;
    return &kOrigin;
}

void ActorCause::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    Actor* v3;
    if (pActor) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        v3 = dynamic_cast<Actor*>(FormByNumericID);
    }
    else {
        v3 = nullptr;
    }
    pActor = v3;
    for (DetectionState* pdetectionState : *pWitnesses) {
        if (pdetectionState) {
            pdetectionState->InitLoadGame();
        }
    }
}

void ActorCause::InitLoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSSimpleList<ActorCause*>* plist = &ActorCause::kCauseList;
    BSSimpleList<ActorCause*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        ActorCause* pactorCause = plist->m_item;
        bool bremove = false;
        if (pactorCause) {
            // Tests only the low 16 bits of the count.
            if (static_cast<uint16_t>(pactorCause->iRefCount)) {
                pactorCause->InitLoadGame(apLoadGameBuffer);
            }
            else {
                pactorCause->iActorCauseID = -1;
                delete pactorCause;
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
}

void ActorCause::RevertActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    (&ActorCause::kCauseList)->RemoveAll();
}

void ActorCause::LoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer) {
    ActorCause* v1;
    void* apMemory;
    uint32_t i;
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&ActorCause::iActorCauseIDCounter)), 4u);
    uint32_t variableSizedValue = apLoadGameBuffer->LoadVariableSizedValue();
    for (i = 0; i < variableSizedValue; ++i) {
        apMemory = ourMalloc(sizeof(ActorCause));
        if (apMemory) {
            v1 = ::new (apMemory) ActorCause;
        }
        else {
            v1 = nullptr;
        }
        v1->LoadGame(apLoadGameBuffer);
    }
}

void ActorCause::ClearActor(Actor* apActor) {
    BSSimpleList<ActorCause*>* plist = &ActorCause::kCauseList;
    while (plist && !plist->IsEmpty()) {
        ActorCause* pcause = plist->m_item;
        plist = plist->m_pkNext;
        if ((pcause) && (pcause->IsValid())) {
            if (pcause->pActor == apActor) {
                pcause->FreeDetectionList();
            }
            BSSimpleList<DetectionState*>* pdetection = pcause->pWitnesses;
            bool bfound = false;
            while (!bfound && (pdetection && pdetection->m_item)) {
                DetectionState* pstate = pdetection->m_item;
                pdetection = pdetection->m_pkNext;
                if (pstate->pActor == apActor) {
                    pcause->pWitnesses->Remove(pstate);
                    ourFree(pstate);
                    bfound = true;
                }
            }
        }
    }
}

ActorCause* ActorCause::GetActorCauseByID(int32_t aiActorCauseID) {
    if (aiActorCauseID < 0) {
        return nullptr;
    }
    for (ActorCause* v2 : ActorCause::kCauseList) {
        if (v2 && v2->iActorCauseID == aiActorCauseID) {
            return v2;
        }
    }
    return nullptr;
}

void ActorCause::SaveActorCausesList(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&ActorCause::iActorCauseIDCounter), 4u, 0);
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (ActorCause* v1 : ActorCause::kCauseList) {
        if (v1) {
            v1->SaveGame(apSaveGameBuffer);
            ++aiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
}
