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
        Actor* pEscorted;
        pEscorted = pNode->GetItem();
        if (pEscorted) {
            apSaveGameBuffer->SaveFormID(pEscorted, 0);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void EscortActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    const TESEscortPackageData* EscortPackageData;
    Actor* pForm;
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pPackage = dynamic_cast<TESPackage*>(FormByNumericID);
    if (pPackage) {
        EscortPackageData = pPackage->GetEscortPackageData();
    }
    else {
        EscortPackageData = 0;
    }
    pPackageData = EscortPackageData;
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = 0;
        apLoadGameBuffer->LoadFormID(pForm);
        EscortedActors.AddHead(pForm);
    }
}

void EscortActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BSSimpleList<Actor*>* plist = &EscortedActors;
    BSSimpleList<Actor*>* plastList = 0;
    while (plist && !plist->IsEmpty()) {
        Actor* pForm = plist->m_item;
        bool bremove = 0;
        if (pForm) {
            // The Actor* holds the form ID that LoadGame stored in the entry, read back as uint32_t.
            TESForm* FormByNumericID =
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<Actor*>(FormByNumericID);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = 1;
            }
        }
        else {
            bremove = 1;
        }
        if (bremove) {
            if (plastList) {
                BSSimpleList<Actor*>* v3 = plist;
                plastList->Remove(v3->m_item);
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
    pActor = apLoadGameBuffer->GetActor();
}

void EscortActorPackageData::RemoveEscortedActor(Actor* apActor) {
    EscortedActors.Remove(apActor);
}

void EscortActorPackageData::AddEscortedActor(Actor* apActor) {
    bool bInList;
    bInList = EscortedActors.IsInList(apActor);
    if (!bInList) {
        EscortedActors.AddHead(apActor);
    }
}

int32_t EscortActorPackageData::GetNumEscortedActorsOfType(Actor* apEscorter, TESBoundObject* apType) {
    Actor* v5;
    Actor* v6;
    BSSimpleList<Actor*>* ecx0 = &EscortedActors;
    while (ecx0) {
        v6 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (v6) {
            if (v6->GetDelete() || (!v6->IsFollowing(apEscorter))) {
                (&EscortedActors)->Remove(v6);
                bool vCondition1;
                vCondition1 = !(&EscortedActors)->IsEmpty();
                if (vCondition1) {
                    ecx0 = &EscortedActors;
                }
            }
        }
    }
    int32_t v7 = 0;
    for (ecx0 = &EscortedActors; ecx0; ecx0 = ecx0->m_pkNext) {
        v5 = ecx0->m_item;
        if (v5) {
            if (v5->GetIsOfType(apType)) {
                ++v7;
            }
        }
    }
    return v7;
}
