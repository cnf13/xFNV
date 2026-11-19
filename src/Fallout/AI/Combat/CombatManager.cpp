#include "Fallout/AI/Combat/CombatManager.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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

// The game settings this file defines, with their IDs and values, in their initialization order.
SettingT<INISettingCollection> bDebugCombatDetectionEvents("bDebugCombatDetectionEvents:Combat", false);

CombatManager::CombatManager()
  : CombatGroups(0, 1), fCombatMusicStartedTimeStamp(-3.4028235e+38f), iNextGroupID(1), spDebugGeometry(nullptr) {
    InitializeData();
}

CombatManager::~CombatManager() {
    ClearData(false);
}

bool CombatManager::CheckShouldFlee(Actor* apAttacker, Actor* apTarget) {
    return false;
}

void CombatManager::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiTMap<Actor*, int64_t>* pNewMap;
    NiTMap<Actor*, int64_t>* ptempMap = pLocalActorTargetingMap;
    NiTMap<Actor*, int64_t>* apMemory =
        static_cast<NiTMap<Actor*, int64_t>*>(ourMalloc(sizeof(NiTMap<Actor*, int64_t>)));
    if (apMemory) {
        pNewMap = ::new (apMemory) NiTMap<Actor*, int64_t>(37u);
    }
    else {
        pNewMap = nullptr;
    }
    pLocalActorTargetingMap = pNewMap;
    NiTMapItem<Actor*, int64_t>* it = ptempMap->GetFirstPos();
    while (it) {
        Actor* pactor_2;
        int64_t itargetingBitfield;
        Actor* pactor;
        ptempMap->GetNext(it, pactor, itargetingBitfield);
        if (pactor) {
            // The key is the raw form ID LoadGame stored in the pointer slot.
            TESForm* FormByNumericID =
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pactor)));
            pactor_2 = dynamic_cast<Actor*>(FormByNumericID);
        }
        else {
            pactor_2 = nullptr;
        }
        pactor = pactor_2;
        if (pactor_2) {
            pLocalActorTargetingMap->SetAt(pactor, itargetingBitfield);
        }
    }
    ptempMap->RemoveAll();
    if (ptempMap) {
        delete ptempMap;
    }
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pgroup = CombatGroups.operator[](i);
        if (pgroup) {
            pgroup->InitLoadGame(apLoadGameBuffer);
        }
    }
    pThreatMap->InitLoadGame(apLoadGameBuffer);
}

void CombatManager::Update(bool abTimeStopped) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_COMBAT_AI, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        139
    );
    if (!abTimeStopped) {
        float afElapsedTime = appTimer.fDelta;
        CombatTimer::UpdateTimer(afElapsedTime);
        for (uint32_t i = 0; i < CombatGroups.GetSize(); ++i) {
            CombatGroup* pgroup = CombatGroups.operator[](i);
            if ((pgroup) && (!pgroup->IsEmpty())) {
                pgroup->Update();
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
    uint32_t igroupCount = CombatGroups.m_usSize;
    if (igroupCount) {
        for (uint32_t i = 0; i < igroupCount; ++i) {
            CombatGroup* pgroup = CombatGroups.operator[](i);
            if ((pgroup) && (pgroup->IsEmpty())) {
                CombatGroup* pDeleted = pgroup;
                if (pDeleted) {
                    delete pDeleted;
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
        for (uint32_t i_2 = 0;; ++i_2) {
            uint32_t igroupCount_2 = CombatGroups.m_usSize;
            if (i_2 >= igroupCount_2) {
                break;
            }
            CombatGroup* pgroup_3 = CombatGroups.operator[](i_2);
            if (pgroup_3) {
                uint32_t imemberCount = pgroup_3->GetMemberCount();
                for (uint32_t j = 0; j < imemberCount; ++j) {
                    Actor* pactor = pgroup_3->GetMember(j);
                    CombatController* pcombatController = pactor->GetCombatController();
                    if (pcombatController) {
                        pcombatController->UpdateLowMovement();
                    }
                }
            }
        }
    }
    UpdateDebugGeometry();
}

CombatGroup* CombatManager::CreateCombatGroup() {
    CombatGroup* pgroup;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_COMBAT_AI, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        353
    );
    CombatGroup* pMemory = static_cast<CombatGroup*>(ourMalloc(sizeof(CombatGroup)));
    if (pMemory) {
        uint32_t aiGroupID = iNextGroupID;
        iNextGroupID = aiGroupID + 1;
        pgroup = ::new (pMemory) CombatGroup(aiGroupID);
    }
    else {
        pgroup = nullptr;
    }
    CombatGroup* pgroup_2 = pgroup;
    uint32_t ipos = CombatGroups.AddFirstEmpty(pgroup_2);
    if (ipos > 64) {
        Warning(
            "COMBAT: In CombatManager, MAX_NUM_COMBAT_GROUPS( %i ) has been exceeded to %i.  Its value needs to be "
            "raised.",
            64, ipos
        );
    }
    pgroup_2->iGroupNum = ipos;
    return pgroup_2;
}

CombatGroup* CombatManager::SplitPlayerCombatGroup(Actor* apMember) {
    CombatGroup* CombatGroup_ = pPlayer->GetCombatGroup();
    CombatGroup_->RemoveMember(apMember);
    if (CombatGroup_->GetMemberCount() > 1) {
        CombatGroup* apGroup = CombatGroup_;
        CombatGroup_->RemoveMember(pPlayer);
        CombatGroup_ = CreateCombatGroup();
        CombatGroup_->AddMember(pPlayer);
        pPlayer->SetCombatGroup(CombatGroup_);
        CombatGroup_->CopyTargets(apGroup);
        for (uint32_t aiIndex = 0; aiIndex < apGroup->GetMemberCount(); ++aiIndex) {
            Actor* Member = apGroup->GetMember(aiIndex);
            if (Member->IsPlayerTeammate()) {
                if (CombatGroup_->AddMember(Member)) {
                    apGroup->RemoveMember(Member);
                    Member->SetCombatGroup(CombatGroup_);
                    --aiIndex;
                }
                else {
                    const char* FullName;
                    if (Member->GetFormEditorIDLength()) {
                        FullName = Member->GetFormEditorID();
                    }
                    else {
                        FullName = Member->GetFullName();
                    }
                    int32_t CastingType = Member->GetFormID();
                    Warning(
                        "COMBAT: Could not move player teammate %s (%08X) to the player's new group in "
                        "CombatManager::SplitPlayerCombatGroup()",
                        FullName, CastingType
                    );
                }
            }
        }
    }
    CombatGroup_->AddTarget(apMember);
    CombatGroup* v9 = CreateCombatGroup();
    v9->AddMember(apMember);
    return v9;
}

bool CombatManager::GetWantToHelpTarget(Actor* apActor, Actor* apAttacker) {
    Actor* target;
    uint32_t i;
    CombatGroup* v7 = apAttacker->GetCombatGroup();
    if (!v7) {
        return false;
    }
    uint32_t targetCount = v7->GetTargetCount();
    for (i = 0; i < targetCount; ++i) {
        target = v7->GetTarget(i);
        if (GetWanttoHelpGroup(apActor, target)) {
            return true;
        }
    }
    return false;
}

void CombatManager::TargetLocalActor(Actor* apActor, CombatGroup* apGroup) {
    int64_t a3 = 0;
    if (!pLocalActorTargetingMap->GetAt(apActor, a3)) {
        a3 = 0;
    }
    int64_t v5 = 1 << apGroup->iGroupNum;
    if ((v5 & a3) == 0) {
        pLocalActorTargetingMap->SetAt(apActor, v5 | a3);
    }
}

void CombatManager::UntargetLocalActor(Actor* apActor, CombatGroup* apGroup) {
    uint64_t a3;
    a3 = 0;
    if (pLocalActorTargetingMap->GetAt(apActor, *reinterpret_cast<int64_t*>(&a3))) {
        int64_t v5 = 1 << apGroup->iGroupNum;
        if ((v5 & a3) != 0) {
            pLocalActorTargetingMap->SetAt(apActor, a3 & ~v5);
        }
    }
}

uint32_t CombatManager::GetCombatantCount(Actor* apActor, uint32_t* apNumLost) {
    CombatTimeStamp flastDetectedTimeStamp;
    uint32_t icount = 0;
    if (apNumLost) {
        *apNumLost = 0;
    }
    for (uint32_t i = 0;; ++i) {
        uint32_t Size = CombatGroups.m_usSize;
        if (i >= Size) {
            break;
        }
        CombatGroup* pgroup = CombatGroups.operator[](i);
        if ((pgroup) && (!apActor || pgroup->IsTarget(apActor))) {
            uint32_t igroupCount = pgroup->GetMemberCount();
            if (igroupCount) {
                CombatGroup* pPlayerGroup = pPlayer->GetCombatGroup();
                if (pgroup == pPlayerGroup) {
                    --igroupCount;
                }
            }
            icount += igroupCount;
            if ((apActor) && (apNumLost)) {
                pgroup->GetTargetLastDetectedTimeStamp(&flastDetectedTimeStamp, apActor);
                double fElapsed = flastDetectedTimeStamp.ElapsedTime();
                if (fCombatDetectionLostTime.Float() < fElapsed) {
                    *apNumLost += igroupCount;
                }
            }
        }
    }
    return icount;
}

void CombatManager::AddLocalActor(Actor* apActor) {
    int64_t val = 0;
    if (!pLocalActorTargetingMap->GetAt(apActor, val)) {
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
    uint32_t i_2 = 0;
    for (uint32_t i = CombatGroups.m_usSize; i_2 < i; i = CombatGroups.m_usSize) {
        CombatGroup* pgroup = CombatGroups.operator[](i_2);
        if (pgroup && pgroup->iGroupID == aiGroupID) {
            return pgroup;
        }
        ++i_2;
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
    uint32_t inumNonTargetsInRadius;
    uint32_t inumGroupMembersInRadius;
    uint32_t apOutNumSelf[2];
    uint32_t apOutNumTargets;
    if (!apExplosion) {
        return true;
    }
    float fradius = apExplosion->GetRadiusBSUnits();
    float fdamage = apExplosion->Data.fDamage;
    if (fDangerousProjectileExplosionDamage.Float() > static_cast<double>(fdamage)) {
        return true;
    }
    float* v6 = &fDangerousProjectileExplosionDamage.Float();
    fradius = CombatFormulas::CalcExplosionDamageDistance(fradius, fdamage, *v6);
    if (fDangerousProjectileExplosionRadius.Float() > static_cast<double>(fradius)) {
        return true;
    }
    apOutNumSelf[0] = 0;
    inumGroupMembersInRadius = 0;
    apOutNumTargets = 0;
    inumNonTargetsInRadius = 0;
    apOutNumSelf[1] = FindLocalActorsInRadius(
        arTargetedPoint, fradius, afTime, apAttacker, apOutNumSelf, &inumGroupMembersInRadius, &apOutNumTargets,
        &inumNonTargetsInRadius
    );
    TESCombatStyle* pcombatStyle = apAttacker->GetCombatStyle();
    if (apOutNumSelf[0] && !pcombatStyle->GetExplosiveWeaponsIgnoreSelf()) {
        return false;
    }
    if (inumGroupMembersInRadius && !pcombatStyle->GetExplosiveWeaponsIgnoreGroup()) {
        return false;
    }
    if (!inumNonTargetsInRadius) {
        return true;
    }
    int32_t v7 = apAttacker->GetActorValue(ActorValue::RESPONSIBILITY);
    return v7 < iAvoidHurtingNonTargetsResponsibility.Int() || pcombatStyle->GetExplosiveWeaponsIgnoreSpectators();
}

void CombatManager::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t i;
    CombatGroup* v4;
    uint32_t v5 = 0;
    for (i = (uint32_t)(&CombatGroups)->m_usSize; v5 < i; i = (uint32_t)(&CombatGroups)->m_usSize) {
        v4 = (&CombatGroups)->operator[](v5);
        if (v4) {
            v4->FinishLoadGame(apLoadGameBuffer);
        }
        ++v5;
    }
    pThreatMap->FinishLoadGame(apLoadGameBuffer);
}

void CombatManager::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    ClearData(true);
    InitializeData();
}

bool CombatManager::GetWanttoHelpGroup(Actor* apActor, Actor* apActorToHelp) {
    PlayerCharacter* pMember;
    FIGHT_REACTION a4;
    int32_t j;
    Actor* apActora;
    uint32_t i;
    uint32_t memberCount;
    uint32_t targetCount;
    CombatGroup* pTargetsCombatGroup;
    int32_t v12 = 0;
    if (!apActor->GetShouldHelp(apActorToHelp)) {
        return false;
    }
    if ((apActor) && (apActorToHelp)) {
        pTargetsCombatGroup = apActorToHelp->GetCombatGroup();
        if (pTargetsCombatGroup) {
            targetCount = pTargetsCombatGroup->GetTargetCount();
            for (i = 0; i < targetCount; ++i) {
                apActora = pTargetsCombatGroup->GetTarget(i);
                if (apActor->GetShouldHelp(apActora)) {
                    return false;
                }
            }
            memberCount = pTargetsCombatGroup->GetMemberCount();
            for (j = 0; ((uint32_t)j) < memberCount; ++j) {
                pMember = static_cast<PlayerCharacter*>(pTargetsCombatGroup->GetMember(j));
                a4 = FIGHT_REACTION_NEUTRAL;
                if (apActor->GetShouldAttackActor(pMember, false, a4, false)) {
                    return false;
                }
            }
        }
    }

    return true;
}

void CombatManager::AddSpectatorForCombatGroup(Actor* apSpectator, Actor* apCombatant) {
    BaseProcess* currentAIProcess;
    SpectatorPackage* pPackage;
    if (apSpectator != apCombatant) {
        currentAIProcess = apSpectator->MobileObject::GetCurrentAIProcess();
        pPackage = static_cast<SpectatorPackage*>(currentAIProcess->GetCurrentPackage());
        if ((!pPackage) || (((int32_t)pPackage->TESPackage::GetPackType()) != 24)) {
            apSpectator->InitiateSpectator();
            pPackage = static_cast<SpectatorPackage*>(currentAIProcess->GetCurrentPackage());
        }
        if ((pPackage) && (((int32_t)pPackage->TESPackage::GetPackType()) == 24)) {
            pPackage->AddThreatFromActorToCombatTarget(*apCombatant);
        }

        AddLocalActor(apSpectator);
    }
}

CombatController* CombatManager::CreateCombatController(Actor* apActor, TESPackage* apPackage) {
    CombatGroup* combatGroup = CreateCombatGroup();
    combatGroup->AddMember(apActor);
    void* v5 = ourMalloc(sizeof(CombatController));
    if (v5) {
        return ::new (v5) CombatController(apActor, combatGroup, apPackage);
    }

    return nullptr;
}

void CombatManager::InitializeData() {
    NiTMap<Actor*, int64_t>* v1;
    CombatTargetMap* v2;
    CombatThreatMap* v3;
    CombatCoverMap* v4;
    CombatCoverMap* apMemory = static_cast<CombatCoverMap*>(ourMalloc(sizeof(CombatCoverMap)));
    if (apMemory) {
        v4 = ::new (apMemory) CombatCoverMap;
    }
    else {
        v4 = nullptr;
    }
    pCoverMap = v4;
    CombatThreatMap* v8 = static_cast<CombatThreatMap*>(ourMalloc(sizeof(CombatThreatMap)));
    if (v8) {
        v3 = ::new (v8) CombatThreatMap;
    }
    else {
        v3 = nullptr;
    }
    pThreatMap = v3;
    CombatTargetMap* v7 = static_cast<CombatTargetMap*>(ourMalloc(sizeof(CombatTargetMap)));
    if (v7) {
        v2 = ::new (v7) CombatTargetMap;
    }
    else {
        v2 = nullptr;
    }
    pTargetMap = v2;
    NiTMap<Actor*, int64_t>* v6 = static_cast<NiTMap<Actor*, int64_t>*>(ourMalloc(sizeof(NiTMap<Actor*, int64_t>)));
    if (v6) {
        v1 = ::new (v6) NiTMap<Actor*, int64_t>(37u);
    }
    else {
        v1 = nullptr;
    }
    pLocalActorTargetingMap = v1;
}

void CombatManager::ClearData(bool abNotifyGroupMembers) {
    uint32_t i_2 = 0;
    for (uint32_t i = CombatGroups.m_usSize; i_2 < i; i = CombatGroups.m_usSize) {
        CombatGroup* pgroup = CombatGroups.operator[](i_2);
        if (pgroup) {
            if (abNotifyGroupMembers) {
                pgroup->RemoveFromMembers();
            }
            delete pgroup;
        }
        ++i_2;
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
    if ((spDebugGeometry.m_pObject) && (spDebugGeometry.m_pObject->m_pkParent)) {
        NiNode* Parent = spDebugGeometry.m_pObject->m_pkParent;
        Parent->DetachChild(spDebugGeometry.m_pObject);
    }

    spDebugGeometry.operator=(static_cast<NiNode*>(nullptr));
}

void CombatManager::UpdateDebugGeometry() {
    NiPoint3 actorLoc;
    for (uint32_t i = 0; i < CombatGroups.m_usSize; ++i) {
        CombatGroup* pgroup = CombatGroups.operator[](i);
        if (pgroup) {
            pgroup->CreateDebugGeometry(eFalse);
            uint32_t imemberCount = pgroup->GetMemberCount();
            for (uint32_t j = 0; j < imemberCount; ++j) {
                Actor* pactor = pgroup->GetMember(j);
                if (pactor) {
                    pactor->GetCombatController();
                }
            }
        }
    }
    if ((spDebugGeometry.m_pObject) && (spDebugGeometry.m_pObject->m_pkParent)) {
        NiNode* Parent = spDebugGeometry.m_pObject->m_pkParent;
        Parent->DetachChild(spDebugGeometry.m_pObject);
    }

    spDebugGeometry.operator=(static_cast<NiNode*>(nullptr));
    if ((pPlayer->GetParentCell()) && (pPlayer->GetParentCell()->GetMarkerNode())) {
        if (bDebugCombatDetectionEvents.Bool()) {
            if (!spDebugGeometry.m_pObject) {
                NiNode* v7;
                NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
                if (apMemory) {
                    v7 = ::new (apMemory) NiNode(0);
                }
                else {
                    v7 = nullptr;
                }
                spDebugGeometry.operator=(v7);
            }
            uint32_t inumHighActors = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
            for (uint32_t i_2 = 0; i_2 < inumHighActors + 1; ++i_2) {
                MobileObject* pmob;
                if (i_2 == inumHighActors) {
                    pmob = pPlayer;
                }
                else {
                    pmob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i_2);
                }
                Actor* pactor_2 = static_cast<Actor*>(pmob);
                if ((pmob) && (pactor_2->IsActor())) {
                    Actor* pactor_3 = pactor_2;
                    BaseProcess* pBaseProc = pactor_2->pCurrentProcess;
                    DetectionEvent* pdetectionEvent = pBaseProc->GetActorsDetectionEvent(pactor_3);
                    if (pdetectionEvent) {
                        NiColorA arrowColor(0.0f, 0.25f, 0.25f, 0.5f);
                        NiColorA sphereColor(0.0f, 0.75f, 0.75f, 0.5f);
                        NiColorA kUnusedColor(1.0f, 0.62f, 0.16f, 0.5f);
                        float fradius = static_cast<double>(pdetectionEvent->iActionValue) + 32.0;
                        NiAVObject* psphere = BSTestObjects::CreateSphere(fradius, 10, 10, &sphereColor);
                        psphere->SetTranslate(pdetectionEvent->Location);
                        NiNode* pDebugRoot = spDebugGeometry.m_pObject;
                        pDebugRoot->AttachChild(psphere, 1);
                        actorLoc = pactor_3->GetLocationOnReference();
                        NiAVObject* parrow = BSTestObjects::Create3DArrow(
                            actorLoc, pdetectionEvent->Location, 2.0f, 2.0f, &arrowColor, &sphereColor
                        );
                        NiNode* v3 = spDebugGeometry.m_pObject;
                        v3->AttachChild(parrow, 1);
                    }
                }
            }
        }
        if (spDebugGeometry.m_pObject) {
            NiAlphaProperty* palpha;
            NiAlphaProperty* v10 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
            if (v10) {
                palpha = ::new (v10) NiAlphaProperty;
            }
            else {
                palpha = nullptr;
            }
            palpha->SetAlphaBlending(true);
            spDebugGeometry.m_pObject->AttachProperty(palpha);
            AddNoLightingPropertyRecurse(spDebugGeometry.m_pObject);
            BSShaderManager::PrepareObject(spDebugGeometry.m_pObject, 0, 0);
            NiNode* MarkerNode = pPlayer->GetParentCell()->GetMarkerNode();
            MarkerNode->AttachChild(spDebugGeometry.m_pObject, 1);
            spDebugGeometry.m_pObject->UpdateProperties();
            NiUpdateData UpdateData(0.0f, false, false);
            spDebugGeometry.m_pObject->Update(UpdateData);
        }
    }
}

CombatController*
CombatManager::AddCombatant(Actor* apActor, Actor* apTarget, uint32_t aiPriority, bool abHighPriority) {
    AutoMemContext apMemory(
        MEM_CONTEXT_COMBAT_AI, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        423
    );
    CombatController* v11 = apActor->GetCombatController();
    if (v11) {
        CombatGroup* apGroupa = v11->pCombatGroup;
        apGroupa->AddTarget(apTarget);
    }
    else {
        CombatController* v6;
        if (!apActor->CanAttackActor(apTarget)) {
        LABEL_3:;
            return nullptr;
        }
        CombatGroup* apGroup = CreateCombatGroup();
        apGroup->AddMember(apActor);
        CombatController* v8 = static_cast<CombatController*>(ourMalloc(sizeof(CombatController)));
        if (v8) {
            v6 = ::new (v8) CombatController(apActor, apGroup, nullptr);
        }
        else {
            v6 = nullptr;
        }
        v11 = v6;
        if (!apGroup->AddTarget(apTarget)) {
            if (v6) {
                delete v6;
            }
            goto LABEL_3;
        }
    }
    return v11;
}

CombatController* CombatManager::AddGroupMember(Actor* apActor, CombatGroup* apGroup) {
    AutoMemContext apMemory(
        MEM_CONTEXT_COMBAT_AI, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp",
        454
    );
    CombatController* pCombatController = apActor->GetCombatController();
    if (pCombatController) {
        CombatGroup* arg0 = pCombatController->GetCombatGroup();
        if (arg0 != apGroup && apGroup->MergeGroup(arg0)) {
            arg0->RemoveAllMembers();
        }
    }
    else {
        if (apGroup->AddMember(apActor)) {
            CombatController* v4;
            pCombatController = static_cast<CombatController*>(ourMalloc(sizeof(CombatController)));
            if (pCombatController) {
                v4 = ::new (pCombatController) CombatController(apActor, apGroup, nullptr);
            }
            else {
                v4 = nullptr;
            }
            pCombatController = v4;
        }
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
    uint32_t inumActorsInRadius = 0;
    uint32_t inumSelfInRadius = 0;
    uint32_t inumGroupMembersInRadius = 0;
    uint32_t inumTargetsInRadius = 0;
    uint32_t inumNonTargetsInRadius = 0;
    CombatGroup* pcombatGroup = apAttackingActor->GetCombatGroup();
    float fsquaredRadius = afRadius * afRadius;
    int64_t igrouptargetBit = 0;
    if (pcombatGroup) {
        igrouptargetBit = 1ll << pcombatGroup->iGroupNum;
    }
    NiTMapItem<Actor*, int64_t>* it = pLocalActorTargetingMap->GetFirstPos();
    while (it) {
        Actor* pactor = nullptr;
        int64_t itargetingBitfield = 0;
        pLocalActorTargetingMap->GetNext(it, pactor, itargetingBitfield);
        if ((pactor) && (!pactor->IsDead(false) && !pactor->GetDisabled() && !pactor->GetDelete())) {
            NiPoint3 actorLoc = pactor->CalculateAnticipatedLocation(afTime);
            NiPoint3 vec = aLocation - actorLoc;
            double v9 = vec.SqrLength();
            if (fsquaredRadius > v9) {
                ++inumActorsInRadius;
                if (pactor == apAttackingActor) {
                    ++inumSelfInRadius;
                }
                else {
                    CombatGroup* v10 = pactor->GetCombatGroup();
                    if (v10 == pcombatGroup) {
                        ++inumGroupMembersInRadius;
                    }
                    else {
                        if ((igrouptargetBit & itargetingBitfield) != 0) {
                            ++inumTargetsInRadius;
                        }
                        else {
                            ++inumNonTargetsInRadius;
                        }
                    }
                }
            }
        }
    }
    if (apOutNumSelf) {
        *apOutNumSelf = inumSelfInRadius;
    }
    if (apOutNumGroupMembers) {
        *apOutNumGroupMembers = inumGroupMembersInRadius;
    }
    if (apOutNumTargets) {
        *apOutNumTargets = inumTargetsInRadius;
    }
    if (apOutNumNonTargets) {
        *apOutNumNonTargets = inumNonTargetsInRadius;
    }
    return inumActorsInRadius;
}

// fCombatMusicStartedTimeStamp is read through its operator float.
bool CombatManager::ShouldPlayCombatMusic() {
    CombatTimeStamp flastDetectedTimeSTamp;
    bool bplayingMusic = -3.4028235e+38f != static_cast<float>(fCombatMusicStartedTimeStamp);
    if (!CombatMusicUpdateTimer.Check() || pSaveLoadGame->GetSaveGameLoading()) {
        return bplayingMusic;
    }
    CombatMusicUpdateTimer.Reset(fCombatMusicUpdateTime.Float());
    bool bplayCombatMusic = false;
    float fnearbyGroupCombatStrength = 0.0f;
    bool bnearbyGroupDetected = false;
    float fplayerCombatStrength = pPlayer->CalculateCombatStrength(-1.0f);
    for (uint32_t i = 0;; ++i) {
        uint32_t Size = CombatGroups.m_usSize;
        if (i >= Size) {
            break;
        }
        CombatGroup* pgroup = CombatGroups.operator[](i);
        if (pgroup) {
            if (pgroup->IsSearching() || (!pgroup->GetTargetCount() || pgroup->IsFleeing())) {
                continue;
            }
            if (pgroup->IsTarget(pPlayer)) {
                pgroup->GetTargetLastDetectedTimeStamp(&flastDetectedTimeSTamp, pPlayer);
                double fElapsed = flastDetectedTimeSTamp.ElapsedTime();
                if (fCombatDetectionLostTime.Float() > fElapsed) {
                    if (bplayingMusic) {
                        bplayCombatMusic = true;
                    }
                    else {
                        float fgroupCombatStrength = 0.0f;
                        uint32_t imemberCount = pgroup->GetMemberCount();
                        for (uint32_t i_2 = 0; i_2 < imemberCount; ++i_2) {
                            Actor* pmember = pgroup->GetMember(i_2);
                            if (pPlayer->IsPerceivedActorDetected(pmember)) {
                                CombatController* pcombatController = pmember->GetCombatController();
                                if (pcombatController) {
                                    fgroupCombatStrength =
                                        pcombatController->pCombatState->GetCombatStrength() + fgroupCombatStrength;
                                }
                            }
                        }
                        if (fgroupCombatStrength > 0.0) {
                            float fthreatRatio = fplayerCombatStrength / fgroupCombatStrength;
                            if (fCombatMusicPlayerTargetedThreatRatio.Float() > static_cast<double>(fthreatRatio)) {
                                bplayCombatMusic = true;
                            }
                        }
                    }
                }
            }
            else {
                if (pgroup->ShouldPlayCombatMusic()) {
                    if (bplayingMusic) {
                        if (pgroup->FindMemberNearActor(pPlayer, fCombatMusicNearCombatOuterRadius.Float())) {
                            bplayCombatMusic = true;
                        }
                    }
                    else {
                        if (pgroup->FindMemberNearActor(pPlayer, fCombatMusicNearCombatInnerRadius.Float())) {
                            if (!bnearbyGroupDetected) {
                                uint32_t imemberCount_2 = pgroup->GetMemberCount();
                                for (uint32_t i_3 = 0; i_3 < imemberCount_2; ++i_3) {
                                    Actor* pmember_2 = pgroup->GetMember(i_3);
                                    if (pPlayer->IsPerceivedActorDetected(pmember_2)) {
                                        bnearbyGroupDetected = true;
                                        break;
                                    }
                                }
                            }
                            fnearbyGroupCombatStrength =
                                pgroup->GetMemberCombatStrength(nullptr) + fnearbyGroupCombatStrength;
                            if ((bnearbyGroupDetected) &&
                                (fnearbyGroupCombatStrength >
                                 fplayerCombatStrength * fCombatMusicPlayerNearStrengthMult.Float())) {
                                bplayCombatMusic = true;
                            }
                        }
                    }
                }
            }
        }
        if (bplayCombatMusic) {
            break;
        }
    }
    if (bplayingMusic != bplayCombatMusic) {
        if (bplayCombatMusic) {
            CombatMusicStopTimer.Clear();
            CombatTimeStamp kStarted(CombatTimer::GetTimer());
            fCombatMusicStartedTimeStamp.fTimeStamp = kStarted.fTimeStamp;
        }
        else {
            if (CombatMusicStopTimer.IsSet()) {
                if (CombatMusicStopTimer.Check()) {
                    CombatTimeStamp kStopped(-3.4028235e+38f);
                    fCombatMusicStartedTimeStamp.fTimeStamp = kStopped.fTimeStamp;
                    CombatMusicStopTimer.Clear();
                }
            }
            else {
                CombatMusicStopTimer.Reset(fCombatMusicStopTime.Float());
            }
        }
    }
    return -3.4028235e+38f != static_cast<float>(fCombatMusicStartedTimeStamp);
}

void CombatManager::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t igroupNum;
    uint32_t igroupID;
    apSaveGameBuffer->SaveDataEndian(&iNextGroupID, 4u, 0);
    uint32_t inumCombatGroups = 0;
    uint32_t icountPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (uint32_t i = 0;; ++i) {
        uint32_t Size = CombatGroups.m_usSize;
        if (i >= Size) {
            break;
        }
        CombatGroup* pgroup = CombatGroups.operator[](i);
        if (pgroup) {
            igroupNum = pgroup->iGroupNum;
            apSaveGameBuffer->SaveDataEndian(&igroupNum, 4u, 0);
            igroupID = pgroup->iGroupID;
            apSaveGameBuffer->SaveDataEndian(&igroupID, 4u, 0);
            pgroup->SaveGame(apSaveGameBuffer);
            ++inumCombatGroups;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(inumCombatGroups, icountPosition);
    pThreatMap->SaveGame(apSaveGameBuffer);
    uint32_t icount = 0;
    uint32_t isavePosition = apSaveGameBuffer->StartVariableSizedValue();
    NiTMapItem<Actor*, int64_t>* it = pLocalActorTargetingMap->GetFirstPos();
    while (it) {
        int64_t val;
        Actor* pactor;
        pLocalActorTargetingMap->GetNext(it, pactor, val);
        apSaveGameBuffer->SaveFormID(pactor, 0);
        ++icount;
    }
    apSaveGameBuffer->SaveVariableSizedValue(icount, isavePosition);
    fCombatMusicStartedTimeStamp.SaveGame(apSaveGameBuffer);
    CombatMusicStopTimer.SaveGame(apSaveGameBuffer);
    CombatMusicUpdateTimer.SaveGame(apSaveGameBuffer);
}

void CombatManager::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    Actor* pactor;
    uint32_t igroupNum;
    uint32_t igroupID;
    apLoadGameBuffer->LoadData(&iNextGroupID, 4u);
    uint32_t inumCombatGroups = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < inumCombatGroups; ++i) {
        CombatGroup* pgroup;
        igroupNum = 0;
        apLoadGameBuffer->LoadData(&igroupNum, 4u);
        igroupID = 0;
        apLoadGameBuffer->LoadData(&igroupID, 4u);
        CombatGroup* apMemory = static_cast<CombatGroup*>(ourMalloc(sizeof(CombatGroup)));
        if (apMemory) {
            pgroup = ::new (apMemory) CombatGroup(0);
        }
        else {
            pgroup = nullptr;
        }
        CombatGroup* pgroup_2 = pgroup;
        pgroup->iGroupNum = igroupNum;
        pgroup_2->iGroupID = igroupID;
        pgroup_2->LoadGame(apLoadGameBuffer);
        CombatGroups.SetAtGrow(igroupNum, pgroup_2);
    }
    pThreatMap->LoadGame(apLoadGameBuffer);
    uint32_t icount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i_2 = 0; i_2 < icount; ++i_2) {
        pactor = nullptr;
        // The map is keyed by the loaded form ID until InitLoadGame resolves it to the actor; the raw ID is written
        // into the pointer slot.
        apLoadGameBuffer->LoadFormID(pactor);
        pLocalActorTargetingMap->SetAt(pactor, 0);
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
        CombatManager* v0;
        AutoMemContext apMemory(
            MEM_CONTEXT_COMBAT_AI, true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatManager.cpp", 67
        );
        CombatManager* v1 = static_cast<CombatManager*>(ourMalloc(sizeof(CombatManager)));
        if (v1) {
            v0 = ::new (v1) CombatManager;
        }
        else {
            v0 = nullptr;
        }
        pCombatManager = v0;
        CombatAction::InitActions();
        CombatGoal::InitGoals();
        CombatGroupStrategy::InitGroupStrategies();
        CombatUtilities::Init();
        CombatDialogueManager::Create();
    }
}
