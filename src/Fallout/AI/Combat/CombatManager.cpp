#include "Fallout/AI/Combat/CombatManager.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BoltShader.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatGroupStrategy.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DetectionEvent.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <limits>

// The game settings this file defines, with their IDs and values, in their initialization order.
SettingT<INISettingCollection> bDebugCombatDetectionEvents("bDebugCombatDetectionEvents:Combat", false);

CombatManager::CombatManager()
  : CombatGroups(0, 1), fCombatMusicStartedTimeStamp(-std::numeric_limits<float>::max()), iNextGroupID(1),
    spDebugGeometry(nullptr) {
    InitializeData();
}

CombatManager::~CombatManager() {
    ClearData(false);
}

bool CombatManager::CheckShouldFlee(Actor* apAttacker, Actor* apTarget) {
    return false;
}

void CombatManager::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiTMap<Actor*, int64_t>* pTempMap = pLocalActorTargetingMap;
    pLocalActorTargetingMap = ::new NiTMap<Actor*, int64_t>(37u);
    NiTMapItem<Actor*, int64_t>* it = pTempMap->GetFirstPos();
    while (it) {
        int64_t iTargetingBitfield;
        Actor* pActor;
        pTempMap->GetNext(it, pActor, iTargetingBitfield);
        Actor* pLoadedActor = nullptr;
        if (pActor) {
            // The key is the raw form ID LoadGame stored in the pointer slot.
            TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
            pLoadedActor = dynamic_cast<Actor*>(pForm);
        }
        pActor = pLoadedActor;
        if (pLoadedActor) {
            pLocalActorTargetingMap->SetAt(pActor, iTargetingBitfield);
        }
    }
    pTempMap->RemoveAll();
    if (pTempMap) {
        delete pTempMap;
    }
    for (CombatGroup* pGroup : CombatGroups) {
        if (pGroup) {
            pGroup->InitLoadGame(apLoadGameBuffer);
        }
    }
    pThreatMap->InitLoadGame(apLoadGameBuffer);
}

void CombatManager::Update(bool abTimeStopped) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        139
    );
    if (!abTimeStopped) {
        float fElapsedTime = appTimer.fDelta;
        CombatTimer::UpdateTimer(fElapsedTime);
        for (uint32_t i = 0; i < CombatGroups.GetSize(); ++i) {
            CombatGroup* pGroup = CombatGroups[i];
            if (pGroup && !pGroup->IsEmpty()) {
                pGroup->Update();
            }
        }
        pThreatMap->UpdateThreats();
    }
}

void CombatManager::UpdateCombatants() {
    for (uint32_t i = 0; i < CombatGroups.GetSize(); ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            uint32_t uiNumMembers = pGroup->GetMemberCount();
            for (uint32_t j = 0; j < uiNumMembers; ++j) {
                Actor* pMember = pGroup->GetMember(j);
                pMember->UpdateCombat();
                uint32_t uiNewNumMembers = pGroup->GetMemberCount();
                if (uiNewNumMembers != uiNumMembers) {
                    uiNumMembers = uiNewNumMembers;
                    --j;
                }
            }
        }
    }
}

void CombatManager::UpdateMainThread(bool abTimeStopped) {
    uint32_t uiGroupCount = CombatGroups.m_usSize;
    if (uiGroupCount) {
        for (uint32_t i = 0; i < uiGroupCount; ++i) {
            CombatGroup* pGroup = CombatGroups[i];
            if (pGroup && pGroup->IsEmpty()) {
                if (pGroup) {
                    delete pGroup;
                }
                CombatGroup* pNull = nullptr;
                CombatGroups.SetAt(i, pNull);
            }
        }
        if (!CombatGroups.m_usESize) {
            CombatGroups.Compact();
            pLocalActorTargetingMap->RemoveAll();
        }
    }
    if (!abTimeStopped) {
        for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
            CombatGroup* pGroup = CombatGroups[i];
            if (pGroup) {
                uint32_t uiMemberCount = pGroup->GetMemberCount();
                for (uint32_t j = 0; j < uiMemberCount; ++j) {
                    Actor* pActor = pGroup->GetMember(j);
                    CombatController* pCombatController = pActor->GetCombatController();
                    if (pCombatController) {
                        pCombatController->UpdateLowMovement();
                    }
                }
            }
        }
    }
    UpdateDebugGeometry();
}

CombatGroup* CombatManager::CreateCombatGroup() {
    CombatGroup* pGroup;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        353
    );
    pGroup = new CombatGroup(iNextGroupID++);
    uint32_t uiPos = CombatGroups.AddFirstEmpty(pGroup);
    if (uiPos > 64) {
        Warning(
            "COMBAT: In CombatManager, MAX_NUM_COMBAT_GROUPS( %i ) has been exceeded to %i.  Its value needs to be "
            "raised.",
            64,
            uiPos
        );
    }
    pGroup->iGroupNum = uiPos;
    return pGroup;
}

CombatGroup* CombatManager::SplitPlayerCombatGroup(Actor* apMember) {
    CombatGroup* pPlayerGroup = pPlayer->GetCombatGroup();
    pPlayerGroup->RemoveMember(apMember);
    if (pPlayerGroup->GetMemberCount() > 1) {
        CombatGroup* pOldGroup = pPlayerGroup;
        pPlayerGroup->RemoveMember(pPlayer);
        pPlayerGroup = CreateCombatGroup();
        pPlayerGroup->AddMember(pPlayer);
        pPlayer->SetCombatGroup(pPlayerGroup);
        pPlayerGroup->CopyTargets(pOldGroup);
        for (uint32_t i = 0; i < pOldGroup->GetMemberCount(); ++i) {
            Actor* pMember = pOldGroup->GetMember(i);
            if (pMember->IsPlayerTeammate()) {
                if (pPlayerGroup->AddMember(pMember)) {
                    pOldGroup->RemoveMember(pMember);
                    pMember->SetCombatGroup(pPlayerGroup);
                    --i;
                }
                else {
                    const char* pName;
                    if (pMember->GetFormEditorIDLength()) {
                        pName = pMember->GetFormEditorID();
                    }
                    else {
                        pName = pMember->GetFullName();
                    }
                    int32_t iFormID = pMember->GetFormID();
                    Warning(
                        "COMBAT: Could not move player teammate %s (%08X) to the player's new group in "
                        "CombatManager::SplitPlayerCombatGroup()",
                        pName,
                        iFormID
                    );
                }
            }
        }
    }
    pPlayerGroup->AddTarget(apMember);
    CombatGroup* pNewGroup = CreateCombatGroup();
    pNewGroup->AddMember(apMember);
    return pNewGroup;
}

bool CombatManager::GetWantToHelpTarget(Actor* apActor, Actor* apAttacker) {
    CombatGroup* pGroup = apAttacker->GetCombatGroup();
    if (!pGroup) {
        return false;
    }
    uint32_t uiTargetCount = pGroup->GetTargetCount();
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        Actor* pTarget = pGroup->GetTarget(i);
        if (GetWanttoHelpGroup(apActor, pTarget)) {
            return true;
        }
    }
    return false;
}

void CombatManager::TargetLocalActor(Actor* apActor, CombatGroup* apGroup) {
    int64_t iTargetingBitfield = 0;
    if (!pLocalActorTargetingMap->GetAt(apActor, iTargetingBitfield)) {
        iTargetingBitfield = 0;
    }
    int64_t iGroupBit = 1 << apGroup->iGroupNum;
    if ((iGroupBit & iTargetingBitfield) == 0) {
        pLocalActorTargetingMap->SetAt(apActor, iGroupBit | iTargetingBitfield);
    }
}

void CombatManager::UntargetLocalActor(Actor* apActor, CombatGroup* apGroup) {
    uint64_t uiTargetingBitfield;
    uiTargetingBitfield = 0;
    if (pLocalActorTargetingMap->GetAt(apActor, *reinterpret_cast<int64_t*>(&uiTargetingBitfield))) {
        int64_t iGroupBit = 1 << apGroup->iGroupNum;
        if ((iGroupBit & uiTargetingBitfield) != 0) {
            pLocalActorTargetingMap->SetAt(apActor, uiTargetingBitfield & ~iGroupBit);
        }
    }
}

uint32_t CombatManager::GetCombatantCount(Actor* apActor, uint32_t* apNumLost) {
    CombatTimeStamp kLastDetectedTimeStamp;
    uint32_t uiCount = 0;
    if (apNumLost) {
        *apNumLost = 0;
    }
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup && (!apActor || pGroup->IsTarget(apActor))) {
            uint32_t uiGroupCount = pGroup->GetMemberCount();
            if (uiGroupCount) {
                CombatGroup* pPlayerGroup = pPlayer->GetCombatGroup();
                if (pGroup == pPlayerGroup) {
                    --uiGroupCount;
                }
            }
            uiCount += uiGroupCount;
            if (apActor && apNumLost) {
                pGroup->GetTargetLastDetectedTimeStamp(&kLastDetectedTimeStamp, apActor);
                double dElapsed = kLastDetectedTimeStamp.ElapsedTime();
                if (fCombatDetectionLostTime.Float() < dElapsed) {
                    *apNumLost += uiGroupCount;
                }
            }
        }
    }
    return uiCount;
}

void CombatManager::AddLocalActor(Actor* apActor) {
    int64_t iTargetingBitfield = 0;
    if (!pLocalActorTargetingMap->GetAt(apActor, iTargetingBitfield)) {
        pLocalActorTargetingMap->SetAt(apActor, 0);
    }
}

void CombatManager::NotifyCombatGroupAddedTarget(CombatGroup* apGroup, Actor* apTarget) {
    TargetLocalActor(apTarget, apGroup);
}

void CombatManager::NotifyCombatGroupRemovedTarget(CombatGroup* apGroup, Actor* apTarget) {
    UntargetLocalActor(apTarget, apGroup);
}

void CombatManager::NotifyCombatGroupAddedMember(CombatGroup* apGroup, Actor* apActor) {
    AddLocalActor(apActor);
}

void CombatManager::RemoveLocalActor(Actor* apActor) {
    pLocalActorTargetingMap->RemoveAt(apActor);
}

void CombatManager::ClearReferencePointers(TESObjectREFR* apRef) {
    if (pSaveLoadGame->GetSaveGameLoading()) {
        return;
    }
    Actor* pActor = nullptr;
    if (apRef->IsActor()) {
        pActor = static_cast<Actor*>(apRef);
    }
    for (uint32_t i = 0; i < CombatGroups.GetSize(); ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            pGroup->ClearReferencePointers(apRef);
        }
    }
    if (pActor) {
        RemoveLocalActor(pActor);
        pTargetMap->RemoveTarget(pActor);
    }
    pThreatMap->RemoveThreat(apRef);
}

CombatGroup* CombatManager::GetCombatGroup(uint32_t aiGroupID) {
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup && pGroup->iGroupID == aiGroupID) {
            return pGroup;
        }
    }
    return nullptr;
}

CombatGroup* CombatManager::GetCombatGroupByIndex(uint32_t aiIndex) {
    return CombatGroups[aiIndex];
}

bool CombatManager::CheckExplosionAttack(
    Actor* apAttacker,
    BGSExplosion* apExplosion,
    const NiPoint3& arTargetedPoint,
    float afTime
) {
    uint32_t uiNumNonTargetsInRadius;
    uint32_t uiNumGroupMembersInRadius;
    uint32_t apOutNumSelf[2];
    uint32_t uiNumTargets;
    if (!apExplosion) {
        return true;
    }
    float fRadius = apExplosion->GetRadiusBSUnits();
    float fDamage = apExplosion->Data.fDamage;
    if (fDangerousProjectileExplosionDamage.Float() > static_cast<double>(fDamage)) {
        return true;
    }
    fRadius = CombatFormulas::CalcExplosionDamageDistance(
        fRadius,
        fDamage,
        fDangerousProjectileExplosionDamage.Float()
    );
    if (fDangerousProjectileExplosionRadius.Float() > static_cast<double>(fRadius)) {
        return true;
    }
    apOutNumSelf[0] = 0;
    uiNumGroupMembersInRadius = 0;
    uiNumTargets = 0;
    uiNumNonTargetsInRadius = 0;
    apOutNumSelf[1] = FindLocalActorsInRadius(
        arTargetedPoint,
        fRadius,
        afTime,
        apAttacker,
        apOutNumSelf,
        &uiNumGroupMembersInRadius,
        &uiNumTargets,
        &uiNumNonTargetsInRadius
    );
    TESCombatStyle* pCombatStyle = apAttacker->GetCombatStyle();
    if (apOutNumSelf[0] && !pCombatStyle->GetExplosiveWeaponsIgnoreSelf()) {
        return false;
    }
    if (uiNumGroupMembersInRadius && !pCombatStyle->GetExplosiveWeaponsIgnoreGroup()) {
        return false;
    }
    if (!uiNumNonTargetsInRadius) {
        return true;
    }
    int32_t iResponsibility = apAttacker->GetActorValue(ActorValue::RESPONSIBILITY);
    return iResponsibility < iAvoidHurtingNonTargetsResponsibility.Int() ||
        pCombatStyle->GetExplosiveWeaponsIgnoreSpectators();
}

void CombatManager::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            pGroup->FinishLoadGame(apLoadGameBuffer);
        }
    }
    pThreatMap->FinishLoadGame(apLoadGameBuffer);
}

void CombatManager::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    ClearData(true);
    InitializeData();
}

bool CombatManager::GetWanttoHelpGroup(Actor* apActor, Actor* apActorToHelp) {
    if (!apActor->GetShouldHelp(apActorToHelp)) {
        return false;
    }
    if (apActor && apActorToHelp) {
        CombatGroup* pTargetsCombatGroup = apActorToHelp->GetCombatGroup();
        if (pTargetsCombatGroup) {
            uint32_t uiTargetCount = pTargetsCombatGroup->GetTargetCount();
            for (uint32_t i = 0; i < uiTargetCount; ++i) {
                Actor* pTarget = pTargetsCombatGroup->GetTarget(i);
                if (apActor->GetShouldHelp(pTarget)) {
                    return false;
                }
            }
            uint32_t uiMemberCount = pTargetsCombatGroup->GetMemberCount();
            for (int32_t j = 0; static_cast<uint32_t>(j) < uiMemberCount; ++j) {
                PlayerCharacter* pMember = static_cast<PlayerCharacter*>(pTargetsCombatGroup->GetMember(j));
                FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
                if (apActor->GetShouldAttackActor(pMember, false, eReaction, false)) {
                    return false;
                }
            }
        }
    }

    return true;
}

void CombatManager::AddSpectatorForCombatGroup(Actor* apSpectator, Actor* apCombatant) {
    if (apSpectator != apCombatant) {
        BaseProcess* pProcess = apSpectator->MobileObject::GetCurrentAIProcess();
        SpectatorPackage* pPackage = static_cast<SpectatorPackage*>(pProcess->GetCurrentPackage());
        if (!pPackage || pPackage->TESPackage::GetPackType() != PACKAGE_TYPE_SPECTATOR) {
            apSpectator->InitiateSpectator();
            pPackage = static_cast<SpectatorPackage*>(pProcess->GetCurrentPackage());
        }
        if (pPackage && pPackage->TESPackage::GetPackType() == PACKAGE_TYPE_SPECTATOR) {
            pPackage->AddThreatFromActorToCombatTarget(*apCombatant);
        }

        AddLocalActor(apSpectator);
    }
}

CombatController* CombatManager::CreateCombatController(Actor* apActor, TESPackage* apPackage) {
    CombatGroup* pGroup = CreateCombatGroup();
    pGroup->AddMember(apActor);
    return new CombatController(apActor, pGroup, apPackage);
}

void CombatManager::InitializeData() {
    pCoverMap = ::new CombatCoverMap;
    pThreatMap = ::new CombatThreatMap;
    pTargetMap = ::new CombatTargetMap;
    pLocalActorTargetingMap = ::new NiTMap<Actor*, int64_t>(37u);
}

void CombatManager::ClearData(bool abNotifyGroupMembers) {
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            if (abNotifyGroupMembers) {
                pGroup->RemoveFromMembers();
            }
            delete pGroup;
        }
    }
    CombatGroups.RemoveAll();
    if (pCoverMap) {
        delete pCoverMap;
    }
    if (pThreatMap) {
        delete pThreatMap;
    }
    if (pTargetMap) {
        delete pTargetMap;
    }
    pLocalActorTargetingMap->RemoveAll();
    if (pLocalActorTargetingMap) {
        delete pLocalActorTargetingMap;
    }
    if (spDebugGeometry.m_pObject && spDebugGeometry->m_pkParent) {
        spDebugGeometry->m_pkParent->DetachChild(spDebugGeometry.m_pObject);
    }

    spDebugGeometry = static_cast<NiNode*>(nullptr);
}

void CombatManager::UpdateDebugGeometry() {
    NiPoint3 actorLoc;
    for (CombatGroup* pGroup : CombatGroups) {
        if (pGroup) {
            pGroup->CreateDebugGeometry(false);
            uint32_t uiMemberCount = pGroup->GetMemberCount();
            for (uint32_t j = 0; j < uiMemberCount; ++j) {
                Actor* pActor = pGroup->GetMember(j);
                if (pActor) {
                    pActor->GetCombatController();
                }
            }
        }
    }
    if (spDebugGeometry.m_pObject && spDebugGeometry->m_pkParent) {
        spDebugGeometry->m_pkParent->DetachChild(spDebugGeometry.m_pObject);
    }

    spDebugGeometry = static_cast<NiNode*>(nullptr);
    if (pPlayer->GetParentCell() && pPlayer->GetParentCell()->GetMarkerNode()) {
        if (bDebugCombatDetectionEvents.Bool()) {
            if (!spDebugGeometry.m_pObject) {
                spDebugGeometry = new NiNode(0);
            }
            uint32_t uiNumHighActors = ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
            for (uint32_t i = 0; i < uiNumHighActors + 1; ++i) {
                MobileObject* pMob = i == uiNumHighActors ? pPlayer : ReferenceProcessLists.MobProcessArray.GetItem(i);
                Actor* pActor = static_cast<Actor*>(pMob);
                if (pMob && pActor->IsActor()) {
                    DetectionEvent* pDetectionEvent = pActor->pCurrentProcess->GetActorsDetectionEvent(pActor);
                    if (pDetectionEvent) {
                        NiColorA arrowColor(0.0f, 0.25f, 0.25f, 0.5f);
                        NiColorA sphereColor(0.0f, 0.75f, 0.75f, 0.5f);
                        float fRadius = static_cast<double>(pDetectionEvent->iActionValue) + 32.0;
                        NiAVObject* pSphere = BSTestObjects::CreateSphere(fRadius, 10, 10, &sphereColor);
                        pSphere->SetTranslate(pDetectionEvent->Location);
                        spDebugGeometry->AttachChild(pSphere, 1);
                        actorLoc = pActor->GetLocationOnReference();
                        NiAVObject* pArrow = BSTestObjects::Create3DArrow(
                            actorLoc,
                            pDetectionEvent->Location,
                            2.0f,
                            2.0f,
                            &arrowColor,
                            &sphereColor
                        );
                        spDebugGeometry->AttachChild(pArrow, 1);
                    }
                }
            }
        }
        if (spDebugGeometry.m_pObject) {
            NiAlphaProperty* pAlpha = new NiAlphaProperty;
            pAlpha->SetAlphaBlending(true);
            spDebugGeometry->AttachProperty(pAlpha);
            AddNoLightingPropertyRecurse(spDebugGeometry.m_pObject);
            BSShaderManager::PrepareObject(spDebugGeometry.m_pObject, 0, 0);
            pPlayer->GetParentCell()->GetMarkerNode()->AttachChild(spDebugGeometry.m_pObject, 1);
            spDebugGeometry->UpdateProperties();
            NiUpdateData kUpdateData(0.0f, false, false);
            spDebugGeometry->Update(kUpdateData);
        }
    }
}

CombatController*
CombatManager::AddCombatant(Actor* apActor, Actor* apTarget, uint32_t aiPriority, bool abHighPriority) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        423
    );
    CombatController* pCombatController = apActor->GetCombatController();
    if (pCombatController) {
        pCombatController->pCombatGroup->AddTarget(apTarget);
    }
    else {
        if (!apActor->CanAttackActor(apTarget)) {
            return nullptr;
        }
        CombatGroup* pGroup = CreateCombatGroup();
        pGroup->AddMember(apActor);
        pCombatController = new CombatController(apActor, pGroup, nullptr);
        if (!pGroup->AddTarget(apTarget)) {
            if (pCombatController) {
                delete pCombatController;
            }
            return nullptr;
        }
    }
    return pCombatController;
}

CombatController* CombatManager::AddGroupMember(Actor* apActor, CombatGroup* apGroup) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        454
    );
    CombatController* pCombatController = apActor->GetCombatController();
    if (pCombatController) {
        CombatGroup* pOldGroup = pCombatController->GetCombatGroup();
        if (pOldGroup != apGroup && apGroup->MergeGroup(pOldGroup)) {
            pOldGroup->RemoveAllMembers();
        }
    }
    else if (apGroup->AddMember(apActor)) {
        pCombatController = new CombatController(apActor, apGroup, nullptr);
    }
    return pCombatController;
}

uint32_t CombatManager::FindLocalActorsInRadius(
    NiPoint3 aLocation,
    float afRadius,
    float afTime,
    Actor* apAttackingActor,
    uint32_t* apOutNumSelf,
    uint32_t* apOutNumGroupMembers,
    uint32_t* apOutNumTargets,
    uint32_t* apOutNumNonTargets
) {
    uint32_t uiNumActorsInRadius = 0;
    uint32_t uiNumSelfInRadius = 0;
    uint32_t uiNumGroupMembersInRadius = 0;
    uint32_t uiNumTargetsInRadius = 0;
    uint32_t uiNumNonTargetsInRadius = 0;
    CombatGroup* pCombatGroup = apAttackingActor->GetCombatGroup();
    float fSquaredRadius = afRadius * afRadius;
    int64_t iGroupTargetBit = 0;
    if (pCombatGroup) {
        iGroupTargetBit = 1ll << pCombatGroup->iGroupNum;
    }
    NiTMapItem<Actor*, int64_t>* it = pLocalActorTargetingMap->GetFirstPos();
    while (it) {
        Actor* pActor = nullptr;
        int64_t iTargetingBitfield = 0;
        pLocalActorTargetingMap->GetNext(it, pActor, iTargetingBitfield);
        if (pActor && !pActor->IsDead(false) && !pActor->GetDisabled() && !pActor->GetDelete()) {
            NiPoint3 actorLoc = pActor->CalculateAnticipatedLocation(afTime);
            NiPoint3 vec = aLocation - actorLoc;
            double dDistSqr = vec.SqrLength();
            if (fSquaredRadius > dDistSqr) {
                ++uiNumActorsInRadius;
                if (pActor == apAttackingActor) {
                    ++uiNumSelfInRadius;
                }
                else if (pActor->GetCombatGroup() == pCombatGroup) {
                    ++uiNumGroupMembersInRadius;
                }
                else if ((iGroupTargetBit & iTargetingBitfield) != 0) {
                    ++uiNumTargetsInRadius;
                }
                else {
                    ++uiNumNonTargetsInRadius;
                }
            }
        }
    }
    if (apOutNumSelf) {
        *apOutNumSelf = uiNumSelfInRadius;
    }
    if (apOutNumGroupMembers) {
        *apOutNumGroupMembers = uiNumGroupMembersInRadius;
    }
    if (apOutNumTargets) {
        *apOutNumTargets = uiNumTargetsInRadius;
    }
    if (apOutNumNonTargets) {
        *apOutNumNonTargets = uiNumNonTargetsInRadius;
    }
    return uiNumActorsInRadius;
}

// fCombatMusicStartedTimeStamp is read through its operator float.
bool CombatManager::ShouldPlayCombatMusic() {
    CombatTimeStamp kLastDetectedTimeSTamp;
    bool bPlayingMusic = -std::numeric_limits<float>::max() != static_cast<float>(fCombatMusicStartedTimeStamp);
    if (!CombatMusicUpdateTimer.Check() || pSaveLoadGame->GetSaveGameLoading()) {
        return bPlayingMusic;
    }
    CombatMusicUpdateTimer.Reset(fCombatMusicUpdateTime.Float());
    bool bPlayCombatMusic = false;
    float fNearbyGroupCombatStrength = 0.0f;
    bool bNearbyGroupDetected = false;
    float fPlayerCombatStrength = pPlayer->CalculateCombatStrength(-1.0f);
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            if (pGroup->IsSearching() || !pGroup->GetTargetCount() || pGroup->IsFleeing()) {
                continue;
            }
            if (pGroup->IsTarget(pPlayer)) {
                pGroup->GetTargetLastDetectedTimeStamp(&kLastDetectedTimeSTamp, pPlayer);
                double dElapsed = kLastDetectedTimeSTamp.ElapsedTime();
                if (fCombatDetectionLostTime.Float() > dElapsed) {
                    if (bPlayingMusic) {
                        bPlayCombatMusic = true;
                    }
                    else {
                        float fGroupCombatStrength = 0.0f;
                        uint32_t uiMemberCount = pGroup->GetMemberCount();
                        for (uint32_t j = 0; j < uiMemberCount; ++j) {
                            Actor* pMember = pGroup->GetMember(j);
                            if (pPlayer->IsPerceivedActorDetected(pMember)) {
                                CombatController* pCombatController = pMember->GetCombatController();
                                if (pCombatController) {
                                    fGroupCombatStrength = pCombatController->pCombatState->GetCombatStrength() +
                                        fGroupCombatStrength;
                                }
                            }
                        }
                        if (fGroupCombatStrength > 0.0) {
                            float fThreatRatio = fPlayerCombatStrength / fGroupCombatStrength;
                            if (fCombatMusicPlayerTargetedThreatRatio.Float() > static_cast<double>(fThreatRatio)) {
                                bPlayCombatMusic = true;
                            }
                        }
                    }
                }
            }
            else if (pGroup->ShouldPlayCombatMusic()) {
                if (bPlayingMusic) {
                    if (pGroup->FindMemberNearActor(pPlayer, fCombatMusicNearCombatOuterRadius.Float())) {
                        bPlayCombatMusic = true;
                    }
                }
                else if (pGroup->FindMemberNearActor(pPlayer, fCombatMusicNearCombatInnerRadius.Float())) {
                    if (!bNearbyGroupDetected) {
                        uint32_t uiMemberCount = pGroup->GetMemberCount();
                        for (uint32_t j = 0; j < uiMemberCount; ++j) {
                            Actor* pMember = pGroup->GetMember(j);
                            if (pPlayer->IsPerceivedActorDetected(pMember)) {
                                bNearbyGroupDetected = true;
                                break;
                            }
                        }
                    }
                    fNearbyGroupCombatStrength = pGroup->GetMemberCombatStrength(nullptr) + fNearbyGroupCombatStrength;
                    if (bNearbyGroupDetected &&
                        fNearbyGroupCombatStrength >
                            fPlayerCombatStrength * fCombatMusicPlayerNearStrengthMult.Float()) {
                        bPlayCombatMusic = true;
                    }
                }
            }
        }
        if (bPlayCombatMusic) {
            break;
        }
    }
    if (bPlayingMusic != bPlayCombatMusic) {
        if (bPlayCombatMusic) {
            CombatMusicStopTimer.Clear();
            CombatTimeStamp kStarted(CombatTimer::GetTimer());
            fCombatMusicStartedTimeStamp.fTimeStamp = kStarted.fTimeStamp;
        }
        else if (CombatMusicStopTimer.IsSet()) {
            if (CombatMusicStopTimer.Check()) {
                CombatTimeStamp kStopped(-std::numeric_limits<float>::max());
                fCombatMusicStartedTimeStamp.fTimeStamp = kStopped.fTimeStamp;
                CombatMusicStopTimer.Clear();
            }
        }
        else {
            CombatMusicStopTimer.Reset(fCombatMusicStopTime.Float());
        }
    }
    return -std::numeric_limits<float>::max() != static_cast<float>(fCombatMusicStartedTimeStamp);
}

void CombatManager::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iNextGroupID, sizeof(iNextGroupID), 0);
    uint32_t uiNumCombatGroups = 0;
    uint32_t uiCountPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pGroup = CombatGroups[i];
        if (pGroup) {
            uint32_t uiGroupNum = pGroup->iGroupNum;
            apSaveGameBuffer->SaveDataEndian(&uiGroupNum, sizeof(uiGroupNum), 0);
            uint32_t uiGroupID = pGroup->iGroupID;
            apSaveGameBuffer->SaveDataEndian(&uiGroupID, sizeof(uiGroupID), 0);
            pGroup->SaveGame(apSaveGameBuffer);
            ++uiNumCombatGroups;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiNumCombatGroups, uiCountPosition);
    pThreatMap->SaveGame(apSaveGameBuffer);
    uint32_t uiCount = 0;
    uint32_t uiSavePosition = apSaveGameBuffer->StartVariableSizedValue();
    NiTMapItem<Actor*, int64_t>* it = pLocalActorTargetingMap->GetFirstPos();
    while (it) {
        int64_t iTargetingBitfield;
        Actor* pActor;
        pLocalActorTargetingMap->GetNext(it, pActor, iTargetingBitfield);
        apSaveGameBuffer->SaveFormID(pActor, 0);
        ++uiCount;
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiSavePosition);
    fCombatMusicStartedTimeStamp.SaveGame(apSaveGameBuffer);
    CombatMusicStopTimer.SaveGame(apSaveGameBuffer);
    CombatMusicUpdateTimer.SaveGame(apSaveGameBuffer);
}

void CombatManager::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&iNextGroupID, sizeof(iNextGroupID));
    uint32_t uiNumCombatGroups = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiNumCombatGroups; ++i) {
        uint32_t uiGroupNum = 0;
        apLoadGameBuffer->LoadData(&uiGroupNum, sizeof(uiGroupNum));
        uint32_t uiGroupID = 0;
        apLoadGameBuffer->LoadData(&uiGroupID, sizeof(uiGroupID));
        CombatGroup* pGroup = new CombatGroup(0);
        pGroup->iGroupNum = uiGroupNum;
        pGroup->iGroupID = uiGroupID;
        pGroup->LoadGame(apLoadGameBuffer);
        CombatGroups.SetAtGrow(uiGroupNum, pGroup);
    }
    pThreatMap->LoadGame(apLoadGameBuffer);
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        Actor* pActor = nullptr;
        // The map is keyed by the loaded form ID until InitLoadGame resolves it to the actor; the raw ID is written
        // into the pointer slot.
        apLoadGameBuffer->LoadFormID(pActor);
        pLocalActorTargetingMap->SetAt(pActor, 0);
    }
    fCombatMusicStartedTimeStamp.LoadGame(apLoadGameBuffer);
    CombatMusicStopTimer.LoadGame(apLoadGameBuffer);
    CombatMusicUpdateTimer.LoadGame(apLoadGameBuffer);
}

void CombatManager::Destroy() {
    FreeCombatActions();
    CombatGoal::DeleteGoals();
    DestroyCombatGroupStrategies();
    CombatDialogueManager::Destroy();
    if (pCombatManager) {
        delete pCombatManager;
    }
    pCombatManager = nullptr;
}

void CombatManager::Create() {
    if (!pCombatManager) {
        AutoMemContext kMemContext(
            MEM_CONTEXT_COMBAT_AI,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
            67
        );
        pCombatManager = new CombatManager;
        CombatAction::InitActions();
        CombatGoal::InitGoals();
        CombatGroupStrategy::InitGroupStrategies();
        CombatUtilities::Init();
        CombatDialogueManager::Create();
    }
}
