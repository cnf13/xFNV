#include "Fallout/AI/EscortPackage.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

EscortActorPackageData::EscortActorPackageData() : pActor(nullptr), pPackage(nullptr), pPackageData(nullptr) {}

EscortActorPackageData::~EscortActorPackageData() {
    EscortedActors.RemoveAll();
}

void EscortActorPackageData::HandleDeletedReference(TESObjectREFR* apDeletedObj) {
    if (apDeletedObj->IsActor()) {
        RemoveEscortedActor(static_cast<Actor*>(apDeletedObj));
    }
}

PACKAGE_TYPE EscortActorPackageData::GetPackageType() {
    return PACKAGE_TYPE_ESCORT;
}

// The package, then every escorted actor, with their count in front.
void EscortActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pPackage, 0);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<Actor*>* pNode = &EscortedActors; pNode;) {
        Actor* pEscorted = pNode->GetItem();
        if (pEscorted) {
            apSaveGameBuffer->SaveFormID(pEscorted, 0);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void EscortActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pForm;
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pPackageForm = TESForm::GetFormByNumericID(uiFormID);
    pPackage = dynamic_cast<TESPackage*>(pPackageForm);
    pPackageData = pPackage ? pPackage->GetEscortPackageData() : nullptr;
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t uiLoop = 0; uiLoop < uiSaveGameCount; ++uiLoop) {
        pForm = nullptr;
        apLoadGameBuffer->LoadFormID(pForm);
        EscortedActors.AddHead(pForm);
    }
}

void EscortActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BSSimpleList<Actor*>* pList = &EscortedActors;
    BSSimpleList<Actor*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        Actor* pForm = pList->m_item;
        bool bRemove = false;
        if (pForm) {
            // The Actor* holds the form ID that LoadGame stored in the entry, read back as uint32_t.
            TESForm* pLoadedForm = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm))
            );
            pForm = dynamic_cast<Actor*>(pLoadedForm);
            if (pForm) {
                pList->SetItem(pForm);
            }
            else {
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
    pActor = apLoadGameBuffer->GetActor();
}

void EscortActorPackageData::RemoveEscortedActor(Actor* apActor) {
    EscortedActors.Remove(apActor);
}

void EscortActorPackageData::AddEscortedActor(Actor* apActor) {
    bool bInList = EscortedActors.IsInList(apActor);
    if (!bInList) {
        EscortedActors.AddHead(apActor);
    }
}

int32_t EscortActorPackageData::GetNumEscortedActorsOfType(Actor* apEscorter, TESBoundObject* apType) {
    Actor* pEscorted;
    BSSimpleList<Actor*>* pNode = &EscortedActors;
    while (pNode) {
        pEscorted = pNode->m_item;
        pNode = pNode->m_pkNext;
        if (pEscorted && (pEscorted->GetDelete() || !pEscorted->IsFollowing(apEscorter))) {
            EscortedActors.Remove(pEscorted);
            if (!EscortedActors.IsEmpty()) {
                pNode = &EscortedActors;
            }
        }
    }
    int32_t iCount = 0;
    for (pNode = &EscortedActors; pNode; pNode = pNode->m_pkNext) {
        pEscorted = pNode->m_item;
        if (pEscorted && pEscorted->GetIsOfType(apType)) {
            ++iCount;
        }
    }
    return iCount;
}
