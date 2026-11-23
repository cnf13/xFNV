#include "Fallout/AI/SpectatorPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <new>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDebugSpectatorThreats("bDebugSpectatorThreats:General", false);

using SpectatorThreatInfoArray = BSSimpleArray<SpectatorThreatInfo, 1024>;

using BSSimpleArray_SpectatorThreatInfo_1024 = BSSimpleArray<SpectatorThreatInfo, 1024>;

SpectatorThreatInfo::SpectatorThreatInfo()
  : pSource(nullptr), pTarget(nullptr), eThreatLevel(THREAT_LEVEL_NONE), iLastPerceivedTime(0), fRadius(0.0f),
    LastKnownPosSource(NiPoint3::ZERO), LastKnownPosTarget(NiPoint3::ZERO), bHasSource(false), bHasTarget(false),
    spDebugGeometry(nullptr) {}

SpectatorThreatInfo::~SpectatorThreatInfo() {
    if (spDebugGeometry.m_pObject) {
        if (spDebugGeometry->GetParent()) {
            spDebugGeometry->GetParent()->DetachChild(spDebugGeometry.m_pObject);
        }
        spDebugGeometry = nullptr;
    }
}

void SpectatorThreatInfo::Update() {
    iLastPerceivedTime = pSource ? BSGetTickCount() : iLastPerceivedTime;
    const NiPoint3* pSourcePos = pSource ? &pSource->GetLocationOnReference() : &LastKnownPosSource;
    LastKnownPosSource.x = pSourcePos->x;
    LastKnownPosSource.y = pSourcePos->y;
    LastKnownPosSource.z = pSourcePos->z;
    const NiPoint3* pTargetPos = pTarget ? &pTarget->GetLocationOnReference() : &LastKnownPosTarget;
    LastKnownPosTarget.x = pTargetPos->x;
    LastKnownPosTarget.y = pTargetPos->y;
    LastKnownPosTarget.z = pTargetPos->z;
    CombatController* pCombatController = pSource ? pSource->GetCombatController() : nullptr;
    TESObjectWEAP* pWeapon = pCombatController ? pCombatController->GetCurrentWeapon() : nullptr;
    THREAT_LEVEL eNewThreatLevel = THREAT_LEVEL_NONE;
    if (pWeapon) {
        switch (pWeapon->GetWeaponType()) {
            case WEAPON_HAND_TO_HAND:
                eNewThreatLevel = THREAT_LEVEL_EXPLOSION;
                fRadius = 512.0f;
                break;
            case WEAPON_ONE_HAND_MELEE:
                eNewThreatLevel = THREAT_LEVEL_PROJECTILE_WEAPONS;
                fRadius = 256.0f;
                break;
            case WEAPON_TWO_HAND_MELEE:
                eNewThreatLevel = THREAT_LEVEL_MELEE_WEAPONS;
                fRadius = 128.0f;
                break;
            case WEAPON_ONE_HAND_PISTOL:
                eNewThreatLevel = THREAT_LEVEL_EXPLOSION;
                fRadius = 512.0f;
                break;
            case WEAPON_ONE_HAND_PISTOL_ENERGY:
                eNewThreatLevel = THREAT_LEVEL_EXPLOSION;
                fRadius = 256.0f;
                break;
            default: break;
        }
    }
    eThreatLevel = static_cast<THREAT_LEVEL>(BSMAX(eThreatLevel, eNewThreatLevel));
}

// Watches aSource and its combat target: marks both targeted, then updates the threat.
void SpectatorThreatInfo::SetupThreatFromActorToCombatTarget(Actor& aSource) {
    pSource = &aSource;
    pTarget = aSource.GetCombatTarget();
    bHasSource = true;
    bHasTarget = pTarget != nullptr;
    pSource->TESObjectREFR::SetTargeted(true);
    if (pTarget) {
        pTarget->TESObjectREFR::SetTargeted(true);
    }
    Update();
}

void SpectatorThreatInfo::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pSourceActor = nullptr;
    if (pSource) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSource));
        pSourceActor = dynamic_cast<Actor*>(pForm);
    }
    pSource = pSourceActor;
    Actor* pTargetActor = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pTarget));
        pTargetActor = dynamic_cast<Actor*>(pForm);
    }
    pTarget = pTargetActor;
    if (pSource) {
        pSource->TESObjectREFR::SetTargeted(true);
    }
    if (pTarget) {
        pTarget->TESObjectREFR::SetTargeted(true);
    }
}

NiPoint3 SpectatorThreatInfo::GetLastKnownPosSource() const {
    NiPoint3 kPos;
    kPos = LastKnownPosSource;
    return kPos;
}

NiPoint3 SpectatorThreatInfo::GetLastKnownPosTarget() const {
    NiPoint3 kPos;
    kPos = LastKnownPosTarget;
    return kPos;
}

// Builds the node in a temporary and copies it into nodeOut.
void SpectatorThreatInfo::GetAvoidNode(PathingAvoidNode& nodeOut) const {
    if (bHasSource) {
        if (bHasTarget) {
            nodeOut = PathingAvoidNode(GetLastKnownPosSource(), GetLastKnownPosTarget(), fRadius, 100.0f);
        }
        else {
            nodeOut = PathingAvoidNode(GetLastKnownPosSource(), fRadius, 100.0f);
        }
    }
    else if (bHasTarget) {
        nodeOut = PathingAvoidNode(GetLastKnownPosTarget(), fRadius, 100.0f);
    }
}

void SpectatorThreatInfo::GetAvoidNode(PathingAvoidNode* apPathingAvoidNode) const {
    NiPoint3 kSourcePos;
    NiPoint3 kTargetPos;
    if (bHasSource) {
        float fAvoidRadius = fRadius;
        if (bHasTarget) {
            kTargetPos = GetLastKnownPosTarget();
            kSourcePos = GetLastKnownPosSource();
            *apPathingAvoidNode = PathingAvoidNode(kSourcePos, kTargetPos, fAvoidRadius, 100.0f);
        }
        else {
            kSourcePos = GetLastKnownPosSource();
            *apPathingAvoidNode = PathingAvoidNode(kSourcePos, fAvoidRadius, 100.0f);
        }
    }
    else if (bHasTarget) {
        float fAvoidRadius = fRadius;
        kTargetPos = GetLastKnownPosTarget();
        *apPathingAvoidNode = PathingAvoidNode(kTargetPos, fAvoidRadius, 100.0f);
    }
}

void SpectatorThreatInfo::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&eThreatLevel, sizeof(eThreatLevel), 0);
    uint32_t uiTimeSincePerceived = BSGetTickCount() - iLastPerceivedTime;
    apSaveGameBuffer->SaveDataEndian(&uiTimeSincePerceived, sizeof(uiTimeSincePerceived), 0);
    apSaveGameBuffer->SaveDataEndian(&fRadius, sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&LastKnownPosSource, sizeof(LastKnownPosSource), 0);
    apSaveGameBuffer->SaveDataEndian(&LastKnownPosTarget, sizeof(LastKnownPosTarget), 0);
    apSaveGameBuffer->SaveDataEndian(&bHasSource, sizeof(bHasSource), 0);
    apSaveGameBuffer->SaveDataEndian(&bHasTarget, sizeof(bHasTarget), 0);
    apSaveGameBuffer->SaveFormID(pSource, 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void SpectatorThreatInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&eThreatLevel, sizeof(eThreatLevel));
    uint32_t uiTimeSincePerceived = 0;
    apLoadGameBuffer->LoadData(&uiTimeSincePerceived, sizeof(uiTimeSincePerceived));
    iLastPerceivedTime = BSGetTickCount() - uiTimeSincePerceived;
    apLoadGameBuffer->LoadData(&fRadius, sizeof(fRadius));
    apLoadGameBuffer->LoadData(&LastKnownPosSource, sizeof(LastKnownPosSource));
    apLoadGameBuffer->LoadData(&LastKnownPosTarget, sizeof(LastKnownPosTarget));
    apLoadGameBuffer->LoadData(&bHasSource, sizeof(bHasSource));
    apLoadGameBuffer->LoadData(&bHasTarget, sizeof(bHasTarget));
    apLoadGameBuffer->LoadFormID(pSource);
    apLoadGameBuffer->LoadFormID(pTarget);
}

void SpectatorThreatInfo::operator delete(void* apMemory) {
    ourFree(apMemory);
}

SpectatorThreatInfo* SpectatorThreatInfo::Assign(SpectatorThreatInfo* apThis, const SpectatorThreatInfo& arOther) {
    apThis->pSource = arOther.pSource;
    apThis->pTarget = arOther.pTarget;
    apThis->eThreatLevel = arOther.eThreatLevel;
    apThis->iLastPerceivedTime = arOther.iLastPerceivedTime;
    apThis->fRadius = arOther.fRadius;
    apThis->LastKnownPosSource = arOther.LastKnownPosSource;
    apThis->LastKnownPosTarget = arOther.LastKnownPosTarget;
    apThis->bHasSource = arOther.bHasSource;
    apThis->bHasTarget = arOther.bHasTarget;
    apThis->spDebugGeometry = arOther.spDebugGeometry;
    return apThis;
}

SpectatorPackage::SpectatorPackage()
  : iLastPathRequestTime(0), fConversationTimer(0.0f), fEventRadius(0.0f), bFlagCompleted(0),
    kEventCenter(NiPoint3::ZERO), kThreatInfos(32u, 0), spDebugGeometry(nullptr) {
    SetPackType(PACKAGE_TYPE_SPECTATOR);
    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_OBSERVE_COMBAT;
}

SpectatorPackage::~SpectatorPackage() {
    if (spDebugGeometry.m_pObject) {
        if (spDebugGeometry->GetParent()) {
            spDebugGeometry->GetParent()->DetachChild(spDebugGeometry.m_pObject);
        }
        spDebugGeometry = nullptr;
    }
}

void SpectatorPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    uint32_t uiPathTimeDelta = appTimer.iLastTime - iLastPathRequestTime;
    apSaveGameBuffer->SaveDataEndian(&uiPathTimeDelta, sizeof(uiPathTimeDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&fTimer, sizeof(fTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fConversationTimer, sizeof(fConversationTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fEventRadius, sizeof(fEventRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&bFlagCompleted, sizeof(bFlagCompleted), 0);
    apSaveGameBuffer->SaveDataEndian(&kEventCenter, sizeof(kEventCenter), 0);
    // The address &kThreatInfos is never null.
    uint32_t uiSaveGameCount = kThreatInfos.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        SpectatorThreatInfo* pThreatInfo = &kThreatInfos[i];
        pThreatInfo->SaveGame(apSaveGameBuffer);
    }
}

void SpectatorPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    int32_t iPathTimeDelta = 0;
    apLoadGameBuffer->LoadData(&iPathTimeDelta, sizeof(iPathTimeDelta));
    iLastPathRequestTime = appTimer.iLastTime - iPathTimeDelta;
    apLoadGameBuffer->LoadData(&fTimer, sizeof(fTimer));
    apLoadGameBuffer->LoadData(&fConversationTimer, sizeof(fConversationTimer));
    apLoadGameBuffer->LoadData(&fEventRadius, sizeof(fEventRadius));
    apLoadGameBuffer->LoadData(&bFlagCompleted, sizeof(bFlagCompleted));
    apLoadGameBuffer->LoadData(&kEventCenter, sizeof(kEventCenter));
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    kThreatInfos.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        SpectatorThreatInfo* pThreatInfo = &kThreatInfos[i];
        pThreatInfo->LoadGame(apLoadGameBuffer);
    }
}

void SpectatorPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
    uint32_t uiCount = kThreatInfos.BSSimpleArray<SpectatorThreatInfo>::GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        SpectatorThreatInfo* pThreatInfo = &kThreatInfos[i];
        pThreatInfo->InitLoadGame(apLoadGameBuffer);
    }
}

void SpectatorPackage::Copy(TESForm* apCopy) {
    TESPackage::Copy(apCopy);
    SpectatorPackage* pCopy = dynamic_cast<SpectatorPackage*>(apCopy);
    if (pCopy) {
        iLastPathRequestTime = pCopy->iLastPathRequestTime;
        fTimer = pCopy->fTimer;
        fConversationTimer = pCopy->fConversationTimer;
        fEventRadius = pCopy->fEventRadius;
        kEventCenter = pCopy->kEventCenter;
        bFlagCompleted = pCopy->bFlagCompleted;
        int32_t iSize = pCopy->kThreatInfos.iSize;
        kThreatInfos.SetSize(static_cast<uint32_t>(iSize), true);
        for (int32_t i = 0; i < iSize; ++i) {
            SpectatorThreatInfo* pSourceInfo = &pCopy->kThreatInfos[i];
            SpectatorThreatInfo* pDestInfo = &kThreatInfos[i];
            SpectatorThreatInfo::Assign(pDestInfo, *pSourceInfo);
        }
    }
}

uint16_t SpectatorPackage::GetSaveSize() {
    return static_cast<uint16_t>(TESPackage::GetSaveSize() + 8);
}

void SpectatorPackage::SaveGame() {
    TESPackage::SaveGame();
    SaveGameDataOLD(&fTimer, sizeof(fTimer));
    SaveGameDataOLD(&fConversationTimer, sizeof(fConversationTimer));
}

void SpectatorPackage::LoadGame() {
    TESPackage::LoadGame();
    LoadGameDataOLD(&fTimer, sizeof(fTimer));
    LoadGameDataOLD(&fConversationTimer, sizeof(fConversationTimer));
}

void SpectatorPackage::InitLoadGame() {
    TESPackage::InitLoadGame();
}

void SpectatorPackage::AddThreatFromActorToCombatTarget(Actor& aSourceActor) {
    Actor* pTargetActor = aSourceActor.GetCombatTarget();
    int32_t iSize = kThreatInfos.iSize;
    SpectatorThreatInfo* pStoredInfo = nullptr;
    for (int32_t i = 0; i < iSize; ++i) {
        SpectatorThreatInfo* pScanInfo = &kThreatInfos[i];
        if (pScanInfo->pSource == &aSourceActor && pScanInfo->pTarget == pTargetActor) {
            pStoredInfo = pScanInfo;
            break;
        }
    }
    if (!pStoredInfo) {
        int32_t iIndex = kThreatInfos.iSize;
        kThreatInfos.SetSize(iIndex + 1, true);
        pStoredInfo = &kThreatInfos[iIndex];
    }
    pStoredInfo->SetupThreatFromActorToCombatTarget(aSourceActor);
    ComputeCenterPoint();
}

NiPoint3 SpectatorPackage::GetEventCenter() {
    return kEventCenter;
}

// Empties the output array, reserves two entries per threat, then appends each threat's source actor when there is one.
void SpectatorPackage::CreateThreatArray(BSSimpleArray<Actor*>* apRetThreats) {
    apRetThreats->Clear(true);
    int32_t iThreatCount = static_cast<int32_t>(kThreatInfos.GetSize());
    apRetThreats->SetReservedSize(static_cast<uint32_t>(iThreatCount) * 2);
    for (int32_t i = 0; i < iThreatCount; ++i) {
        Actor* pSource = kThreatInfos[static_cast<uint32_t>(i)].GetSource();
        if (pSource) {
            apRetThreats->Add(pSource);
        }
    }
}

bool SpectatorPackage::IsComplete() const {
    return !kThreatInfos.iSize || bFlagCompleted;
}

void SpectatorPackage::ComputeCenterPoint() {
    if (kThreatInfos.iSize) {
        int32_t i;
        int32_t iSize = kThreatInfos.iSize;
        int32_t iCount = 0;
        NiPoint3 kMaxPoint = NiPoint3::ZERO;
        NiPoint3 kMinPoint = NiPoint3::ZERO;
        for (i = 0; i < iSize; ++i) {
            SpectatorThreatInfo* pInfo = &kThreatInfos[i];
            if (pInfo->bHasSource && pInfo->bHasTarget) {
                float fTargetX = pInfo->GetLastKnownPosTarget().x;
                kMaxPoint.x = BSMAX(pInfo->GetLastKnownPosSource().x, fTargetX);
                float fTargetY = pInfo->GetLastKnownPosTarget().y;
                kMaxPoint.y = BSMAX(pInfo->GetLastKnownPosSource().y, fTargetY);
                float fTargetZ = pInfo->GetLastKnownPosTarget().z;
                kMaxPoint.z = BSMAX(pInfo->GetLastKnownPosSource().z, fTargetZ);
                fTargetX = pInfo->GetLastKnownPosTarget().x;
                kMinPoint.x = BSMIN(pInfo->GetLastKnownPosSource().x, fTargetX);
                fTargetY = pInfo->GetLastKnownPosTarget().y;
                kMinPoint.y = BSMIN(pInfo->GetLastKnownPosSource().y, fTargetY);
                fTargetZ = pInfo->GetLastKnownPosTarget().z;
                kMinPoint.z = BSMIN(pInfo->GetLastKnownPosSource().z, fTargetZ);
                iCount += 2;
                break;
            }
            if (pInfo->bHasSource) {
                kMaxPoint = pInfo->GetLastKnownPosSource();
                kMinPoint = pInfo->GetLastKnownPosSource();
                ++iCount;
                break;
            }
            if (pInfo->bHasTarget) {
                kMaxPoint = pInfo->GetLastKnownPosTarget();
                kMinPoint = pInfo->GetLastKnownPosTarget();
                ++iCount;
                break;
            }
        }
        ++i;
        while (i < iSize) {
            SpectatorThreatInfo* pInfo = &kThreatInfos[i];
            if (pInfo->bHasSource) {
                kMaxPoint.x = BSMAX(kMaxPoint.x, pInfo->GetLastKnownPosSource().x);
                kMaxPoint.y = BSMAX(kMaxPoint.y, pInfo->GetLastKnownPosSource().y);
                kMaxPoint.z = BSMAX(kMaxPoint.z, pInfo->GetLastKnownPosSource().z);
                kMinPoint.x = BSMIN(kMinPoint.x, pInfo->GetLastKnownPosSource().x);
                kMinPoint.y = BSMIN(kMinPoint.y, pInfo->GetLastKnownPosSource().y);
                kMinPoint.z = BSMIN(kMinPoint.z, pInfo->GetLastKnownPosSource().z);
                ++iCount;
            }
            if (pInfo->bHasTarget) {
                kMaxPoint.x = BSMAX(kMaxPoint.x, pInfo->GetLastKnownPosTarget().x);
                kMaxPoint.y = BSMAX(kMaxPoint.y, pInfo->GetLastKnownPosTarget().y);
                kMaxPoint.z = BSMAX(kMaxPoint.z, pInfo->GetLastKnownPosTarget().z);
                kMinPoint.x = BSMIN(kMinPoint.x, pInfo->GetLastKnownPosTarget().x);
                kMinPoint.y = BSMIN(kMinPoint.y, pInfo->GetLastKnownPosTarget().y);
                kMinPoint.z = BSMIN(kMinPoint.z, pInfo->GetLastKnownPosTarget().z);
                ++iCount;
            }
            ++i;
        }
        if (iCount) {
            NiPoint3 kCenterPoint = kMaxPoint;
            kCenterPoint += kMinPoint;
            kCenterPoint *= 0.5f;
            float fDistanceSquared = 0.0f;
            float fFarthestDistanceSquared = 0.0f;
            for (i = 0; i < iSize; ++i) {
                SpectatorThreatInfo* pInfo = &kThreatInfos[i];
                if (pInfo->bHasSource || pInfo->bHasTarget) {
                    if (pInfo->bHasSource) {
                        fDistanceSquared = (kCenterPoint - pInfo->GetLastKnownPosSource()).SqrLength();
                        if (fFarthestDistanceSquared <= static_cast<double>(fDistanceSquared)) {
                            fFarthestDistanceSquared = fDistanceSquared;
                        }
                    }
                    if (pInfo->bHasTarget) {
                        fDistanceSquared = (kCenterPoint - pInfo->GetLastKnownPosTarget()).SqrLength();
                        if (fFarthestDistanceSquared <= static_cast<double>(fDistanceSquared)) {
                            fFarthestDistanceSquared = fDistanceSquared;
                        }
                    }
                }
            }
            kEventCenter = kCenterPoint;
            fEventRadius = std::sqrtf(fFarthestDistanceSquared);
        }
        else {
            bFlagCompleted = 1;
        }
    }
}

void SpectatorPackage::MakeCombatComment(Actor* apActor, bool bflag) {
    if (bflag) {
        if (kThreatInfos.iSize) {
            int32_t iChance = Random() % 100;
            Actor* pFirstTarget = kThreatInfos.GetFirst().pTarget;
            Actor* pCommentTarget = pFirstTarget && pFirstTarget->IsActor() ? pFirstTarget : nullptr;
            if (pCommentTarget) {
                if (apActor->pCurrentProcess) {
                    BaseProcess* pProcess = apActor->pCurrentProcess;
                    if (!pProcess->GetGreetingFlag() &&
                        fAISocialchanceForConversation.Float() > static_cast<double>(iChance) &&
                        fConversationTimer <= 0.0) {
                        TESTopic* pTopic = TESTopic::GetTopic(DT_MISCELLANEOUS, 0);
                        if (pTopic) {
                            apActor->SetDialogueItemTarget(pCommentTarget);
                            apActor->pCurrentProcess->ProcessGreet(apActor, pTopic, false, false, true, false);
                            fConversationTimer = fAISpectatorCommentTimer.Float();
                        }
                    }
                }
                fConversationTimer = fConversationTimer - appTimer.fDelta;
            }
        }
    }
    else {
        apActor->pCurrentProcess->SetGreetingFlag(false);
        apActor->StopMoving();
    }
}

void SpectatorPackage::LookAtCombatants(Actor* apActor, bool bflag) {
    TESObjectREFR* pLookTarget = nullptr;
    if (bflag && kThreatInfos.iSize) {
        Actor* pFirstTarget = kThreatInfos.GetFirst().pTarget;
        if (pFirstTarget && pFirstTarget->IsActor()) {
            pLookTarget = pFirstTarget;
        }
    }
    if (pLookTarget) {
        apActor->pCurrentProcess->SetActionHeadTrackTarget(pLookTarget);
    }
    else {
        apActor->pCurrentProcess->ClearActionHeadTrackTarget(true);
    }
}

void SpectatorPackage::Update(Actor* pOwner) {
    char cChanged = 0;
    int32_t iSize = kThreatInfos.iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        SpectatorThreatInfo* pInfo = &kThreatInfos[i];
        if (pInfo->pSource && pInfo->pTarget) {
            Actor* pCurrentTarget = pInfo->pSource->GetCombatTarget();
            if (pCurrentTarget && pCurrentTarget != pInfo->pTarget) {
                AddThreatFromActorToCombatTarget(*pOwner);
                cChanged = 1;
            }
        }
    }
    iSize = kThreatInfos.iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        SpectatorThreatInfo* pInfo = &kThreatInfos[i];
        Actor* pSource = pInfo->pSource;
        Actor* pTarget = pInfo->pTarget;
        uint32_t uiNow = BSGetTickCount();
        if (uiNow > pInfo->iLastPerceivedTime + 2000) {
            double dNow = static_cast<double>(BSGetTickCount());
            double dLastPerceived = static_cast<double>(pInfo->iLastPerceivedTime);
            if (fAISpectatorRememberThreatTimer.Float() * 1000.0 + dLastPerceived < dNow) {
                kThreatInfos.RemoveFast(static_cast<uint32_t>(i), 1u);
                --iSize;
                --i;
                cChanged = 1;
            }
            else if (
                pTarget &&
                (pTarget->IsDead(false) || pTarget->GetDelete() || pTarget->IsParalyzed() || pTarget->IsKnockedOut())
            ) {
                pInfo->pTarget = nullptr;
                cChanged = 1;
            }
            else {
                bool bStillThreat = !pSource || !pSource->IsNpc() || !pOwner->pCurrentProcess;
                if (!bStillThreat) {
                    DetectionState* pDetection = pOwner->pCurrentProcess
                        ? pOwner->pCurrentProcess->GetDetectionState(pSource, 0)
                        : nullptr;
                    bStillThreat = pDetection && pDetection->iLevel > 0;
                }
                if (bStillThreat && (!pSource || !pTarget || pSource->GetCombatTarget() == pTarget)) {
                    cChanged = 1;
                    pInfo->Update();
                }
            }
        }
    }
    if (cChanged) {
        ComputeCenterPoint();
    }
}

void SpectatorPackage::HandleDeletedActor(void* apActor) {
    int32_t iSize = kThreatInfos.iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        SpectatorThreatInfo* pScanInfo = &kThreatInfos[i];
        if (pScanInfo->pSource == apActor) {
            kThreatInfos.RemoveFast(i, 1u);
            --iSize;
            --i;
        }
        else if (pScanInfo->pTarget == apActor) {
            pScanInfo->pTarget = nullptr;
            pScanInfo->bHasTarget = false;
        }
    }
}

void SpectatorPackage::RemoveActorAsThreatSourceOrTarget(Actor& aActor) {
    int32_t iSize = kThreatInfos.iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        SpectatorThreatInfo* pScanInfo = &kThreatInfos[i];
        if (pScanInfo->pSource == &aActor || pScanInfo->pTarget == &aActor) {
            kThreatInfos.RemoveFast(static_cast<uint32_t>(i), 1u);
            --iSize;
            --i;
        }
    }
}

PathingAvoidNodeArray* SpectatorPackage::CreateAvoidNodeArray() {
    PathingAvoidNodeArray* pAvoidNodeArray = new PathingAvoidNodeArray;
    int32_t iSize = kThreatInfos.iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        const SpectatorThreatInfo& kScanInfo = kThreatInfos[i];
        int32_t iNodeArrayIndex = pAvoidNodeArray->iSize;
        pAvoidNodeArray->SetSize(iNodeArrayIndex + 1, true);
        PathingAvoidNode* pAvoidNode = &pAvoidNodeArray->GetAt(iNodeArrayIndex);
        kScanInfo.GetAvoidNode(pAvoidNode);
    }
    return pAvoidNodeArray;
}

void SpectatorPackage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
