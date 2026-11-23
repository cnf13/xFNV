#include "Fallout/AI/PatrolPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

// The other fields are deliberately left unset, as the game's constructor does.
PatrolActorPackageData::PatrolActorPackageData()
  : ActorPackageData(), eState(PATROL_STATE_INIT), pActor(nullptr), pPackage(nullptr), pPackageData(nullptr),
    References() {}

void PatrolActorPackageData::HandleDeletedReference(TESObjectREFR* apDeletedObj) {
    if (apDeletedObj == pActor) {
        ClearReservedPatrolRef();
        eState = PATROL_STATE_DEBUG_ACTOR_DELETED;
        pActor = nullptr;
        pPackage = nullptr;
        pPackageData = nullptr;
    }
    else {
        for (uint32_t i = 0; i < References.iSize; ++i) {
            if (*static_cast<TESObjectREFR**>(&References.GetAt(i)) == apDeletedObj) {
                References.Remove(i, true);
                if (iCurrentReference >= i) {
                    --iCurrentReference;
                }
                --i;
            }
        }
        if (pActor && !pSaveLoadGame->GetSaveGameLoading() && !SetCurrentReference(iCurrentReference)) {
            ResumeRepeatablePatrol(&pActor->GetLocationOnReference());
        }
    }
}

void PatrolActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&fCurrentTimeAtReference, sizeof(fCurrentTimeAtReference), 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentReference, sizeof(iCurrentReference), 0);
    apSaveGameBuffer->SaveDataEndian(&bPatrolDirection, sizeof(bPatrolDirection), 0);
    apSaveGameBuffer->SaveDataEndian(&bCircularArray, sizeof(bCircularArray), 0);
    apSaveGameBuffer->SaveDataEndian(&iLoopAroundIndex, sizeof(iLoopAroundIndex), 0);
    apSaveGameBuffer->SaveFormID(pPackage, 0);
    uint32_t isaveGameCount = References.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        apSaveGameBuffer->SaveFormID(References[i], 0);
    }
}

void PatrolActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PatrolActorPackageData::PATROL_STATE eLoadedState;
    apLoadGameBuffer->LoadData(&this->eState, sizeof(this->eState));
    // Both bounds are tested: a loaded state outside INIT..AT_REF becomes INIT.
    if (this->eState >= PatrolActorPackageData::PATROL_STATE_INIT &&
        this->eState <= PatrolActorPackageData::PATROL_STATE_AT_REF) {
        eLoadedState = this->eState;
    }
    else {
        eLoadedState = PatrolActorPackageData::PATROL_STATE_INIT;
    }
    this->eState = eLoadedState;
    apLoadGameBuffer->LoadData(&fCurrentTimeAtReference, sizeof(fCurrentTimeAtReference));
    apLoadGameBuffer->LoadData(&iCurrentReference, sizeof(iCurrentReference));
    apLoadGameBuffer->LoadData(&bPatrolDirection, sizeof(bPatrolDirection));
    apLoadGameBuffer->LoadData(&bCircularArray, sizeof(bCircularArray));
    apLoadGameBuffer->LoadData(&iLoopAroundIndex, sizeof(iLoopAroundIndex));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pPackage = dynamic_cast<TESPackage*>(pForm);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    References.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        // Each slot holds the reference's form ID until InitLoadGame resolves it.
        apLoadGameBuffer->LoadFormID(References[i]);
    }
}

void PatrolActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t isaveGameCount = References.iSize;
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        TESObjectREFR* pResolved;
        bool bremove = false;
        TESObjectREFR** ppatrolRef = &References[i];
        // The slot holds the form ID LoadGame read into it.
        if (*ppatrolRef) {
            TESForm* pForm = TESForm::GetFormByNumericID(*reinterpret_cast<uint32_t*>(ppatrolRef));
            pResolved = dynamic_cast<TESObjectREFR*>(pForm);
        }
        else {
            pResolved = nullptr;
        }
        *ppatrolRef = pResolved;
        if (*ppatrolRef) {
            (*ppatrolRef)->SetTargeted(true);
        }
        else {
            bremove = true;
            uint32_t inewReference = iCurrentReference ? iCurrentReference - 1 : 0;
            iCurrentReference = inewReference;
        }
        if (bremove) {
            References.RemoveFast(i--, 1u);
            --isaveGameCount;
        }
    }
    pActor = apLoadGameBuffer->GetActor();
    if (pPackage) {
        if (!pPackageData) {
            pPackageData = pPackage->GetPatrolPackageData();
        }
        if (!SetCurrentReference(iCurrentReference)) {
            ResumeRepeatablePatrol(&pActor->GetLocationOnReference());
        }
        if (!References.IsEmpty() && bCircularArray) {
            TESObjectREFR* ppatrolRef = References.GetAt(iCurrentReference);
            ExtraDataList* ppatrolExtra = ppatrolRef ? &ppatrolRef->m_Extra : nullptr;
            if (ppatrolExtra) {
                ppatrolExtra->SetPatrolRefInUse(pActor);
            }
        }
    }
}

PatrolActorPackageData::~PatrolActorPackageData() {
    if (!pDataHandler->GetClearingData()) {
        ClearReservedPatrolRef();
    }
    References.SetSize(0, true);
    eState = PATROL_STATE_DEBUG_DESTRUCTED;
    pActor = nullptr;
    pPackage = nullptr;
    pPackageData = nullptr;
}

bool PatrolActorPackageData::Init(Actor* apActor, TESPackage* apPackage) {
    pActor = apActor;
    pPackage = apPackage;
    pPackageData = apPackage->GetPatrolPackageData();
    iCurrentReference = 0;
    bPatrolDirection = true;
    iLoopAroundIndex = 0;
    bool bvalid = BuildReferenceArray();
    if (bvalid && pPackageData->bRepeatable) {
        ResumeRepeatablePatrol(&apActor->GetLocationOnReference());
    }

    return bvalid;
}

bool PatrolActorPackageData::SetNextReferenceForPatrol(bool abWaitForOccupiedPoints) {
    uint32_t inextReference = iCurrentReference;
    bool bkeepPatrolling = true;
    bool brepeatable = pPackageData ? pPackageData->bRepeatable : false;
    if (bPatrolDirection) {
        inextReference = inextReference + 1;
        if (inextReference == References.iSize) {
            if (brepeatable) {
                if (bCircularArray) {
                    inextReference = iLoopAroundIndex;
                }
                else {
                    if (References.iSize == 1) {
                        inextReference = 0;
                    }
                    else {
                        inextReference -= 2;
                    }
                    bPatrolDirection = !bPatrolDirection;
                }
            }
            else {
                inextReference = 0;
                bkeepPatrolling = false;
            }
        }
    }
    else {
        inextReference = inextReference - 1;
        if (inextReference == -1) {
            if (brepeatable) {
                if (bCircularArray) {
                    inextReference = 0;
                    bkeepPatrolling = false;
                }
                else {
                    inextReference = References.iSize != 1;
                    bPatrolDirection = !bPatrolDirection;
                }
            }
            else {
                inextReference = 0;
                bkeepPatrolling = false;
            }
        }
    }
    if (abWaitForOccupiedPoints) {
        if (inextReference != iCurrentReference && !SetCurrentReference(inextReference)) {
            uint32_t isize = References.iSize;
            uint32_t iavailable = isize - 1;
            for (uint32_t i = 0; i < isize; ++i) {
                TESObjectREFR* pref = References.GetAt(i);
                ExtraDataList* pnewExtra = pref ? &pref->m_Extra : nullptr;
                if (i != iCurrentReference && (!pnewExtra || !pnewExtra->GetPatrolRefCanBeUsedBy(pActor))) {
                    --iavailable;
                }
            }
            if (!iavailable) {
                return SetNextReferenceForPatrol(false);
            }
        }
    }
    else {
        iCurrentReference = inextReference;
    }
    return bkeepPatrolling;
}

void PatrolActorPackageData::ClearReservedPatrolRef() {
    ExtraDataList* pExtraDataList;
    TESObjectREFR* pReference;
    if (iCurrentReference < References.BSSimpleArray<TESObjectREFR*>::GetSize()) {
        pReference = References.GetAt(iCurrentReference);
        if (pReference) {
            pExtraDataList = pReference->TESObjectREFR::GetExtra();
        }
        else {
            pExtraDataList = nullptr;
        }
        if (pExtraDataList) {
            pExtraDataList->SetPatrolRefInUse(nullptr);
        }
    }
}

TESObjectREFR* PatrolActorPackageData::GetCurrentReference() {
    if (References.IsEmpty()) {
        return nullptr;
    }

    return References.GetAt(iCurrentReference);
}

bool PatrolActorPackageData::BuildReferenceArray() {
    TESObjectREFR* pcurrentReference = PatrolActorPackageData::GetInitialReferenceForSetup(pActor, pPackage);
    bCircularArray = false;
    References.SetSize(0, true);
    bool brepeatable = pPackageData ? pPackageData->bRepeatable : false;
    while (pcurrentReference) {
        pcurrentReference->SetTargeted(true);
        References.Add(pcurrentReference);
        pcurrentReference = pcurrentReference->GetLinkedRef();
        if (!pcurrentReference) {
            break;
        }
        int32_t iloopIndex = References.Find(
            pcurrentReference,
            0,
            &BSSimpleArray<TESObjectREFR*>::DefaultEqualFunctionByAddress
        );
        if (iloopIndex != -1) {
            if (brepeatable) {
                bCircularArray = true;
                iLoopAroundIndex = iloopIndex;
            }
            else {
                References.Add(pcurrentReference);
            }
            return !References.IsEmpty();
        }
    }
    return !References.IsEmpty();
}

void PatrolActorPackageData::ResumeRepeatablePatrol(const NiPoint3* arPoint) {
    if (pActor) {
        int32_t iRefCount = References.iSize;
        SetCurrentReference(0);
        this->bPatrolDirection = true;
        const NiPoint3* pPrevLocation = nullptr;
        uint32_t uiPrevReference = 0;
        bool bPrevDirection = true;
        TESObjectREFR* pCurrentRef = GetCurrentReference();
        if (pCurrentRef) {
            const NiPoint3* pCurrentLocation = &pCurrentRef->GetLocationOnReference();
            float fBestSqrDistance = std::numeric_limits<float>::max();
            uint32_t uiBestReference = 0;
            bool bBestDirection = true;
            char cFound = 0;
            for (int32_t i = 0; i < iRefCount; ++i) {
                ExtraDataList* pExtra;
                uiPrevReference = this->iCurrentReference;
                bPrevDirection = this->bPatrolDirection;
                pPrevLocation = pCurrentLocation;
                SetNextReferenceForPatrol(false);
                TESObjectREFR* pNextRef = GetCurrentReference();
                if (pNextRef) {
                    pExtra = pNextRef->GetExtra();
                }
                else {
                    pExtra = nullptr;
                }
                if (!pExtra || pExtra->GetPatrolRefCanBeUsedBy(pActor)) {
                    NiPoint3 kOffset;
                    pCurrentRef = GetCurrentReference();
                    if (!pCurrentRef) {
                        break;
                    }
                    pCurrentLocation = &pCurrentRef->GetLocationOnReference();
                    kOffset = *pPrevLocation - *arPoint;
                    float fSqrDistance = kOffset.SqrXYLength();
                    if (fBestSqrDistance > static_cast<double>(fSqrDistance)) {
                        fBestSqrDistance = fSqrDistance;
                        uiBestReference = uiPrevReference;
                        bBestDirection = bPrevDirection;
                        cFound = 1;
                    }
                }
            }
            if (cFound) {
                SetCurrentReference(uiBestReference);
                this->bPatrolDirection = bBestDirection;
            }
            this->bPatrolDirection = true;
        }
    }
}

bool PatrolActorPackageData::SetCurrentReference(uint32_t aiRefIndex) {
    int32_t iclampedRefIndex;
    if (!References.IsEmpty() && bCircularArray) {
        iclampedRefIndex = aiRefIndex;
        int32_t isize = References.iSize - 1;
        BSClamp<int32_t>(iclampedRefIndex, 0, isize);
        bool bsuccess = true;
        TESObjectREFR* pnewRef = References.GetAt(iclampedRefIndex);
        ExtraDataList* pnewExtra = pnewRef ? &pnewRef->m_Extra : nullptr;
        if (pnewExtra) {
            if (pnewExtra->GetPatrolRefCanBeUsedBy(pActor)) {
                ClearReservedPatrolRef();
                pnewExtra->SetPatrolRefInUse(pActor);
                iCurrentReference = iclampedRefIndex;
            }
            else {
                return false;
            }
        }
        return bsuccess;
    }

    iCurrentReference = aiRefIndex;
    return true;
}

TESObjectREFR* PatrolActorPackageData::GetInitialReferenceForSetup(Actor* apActor, TESPackage* apPack) {
    TESObjectREFR* pInitialRef = nullptr;
    PackageLocation* pLocation = apPack->GetLocation();
    if (pLocation) {
        TESObjectREFR* pLocReference = pLocation->GetLocReference();
        if (pLocReference) {
            pInitialRef = pLocReference;
        }
    }
    if (!pInitialRef) {
        TESObjectREFR* pLinkedRef = apActor->TESObjectREFR::GetLinkedRef();
        if (pLinkedRef) {
            return pLinkedRef;
        }
    }
    return pInitialRef;
}
