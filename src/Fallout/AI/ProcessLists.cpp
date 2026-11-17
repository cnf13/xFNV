#include "Fallout/AI/ProcessLists.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSParallelTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BSDoorHavokController.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/MuzzleFlash.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DetectionEvent.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PatrolPackage.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

#include <bit>

// the mob ChangeProcessLevelTempList is moving; dropping it from the temp list clears it
static MobileObject* pCPLRef;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> iLowProcessingMilliseconds("iLowProcessingMilliseconds", 2);
SettingT<INISettingCollection> bRunMiddleLowLevelProcess("bRunMiddleLowLevelProcess:General", true);
SettingT<INISettingCollection> bTaskletActorSceneGraphUpdates("bTaskletActorSceneGraphUpdates:General", false);
SettingT<INISettingCollection> bTaskletActorAnimMovementUpdates("bTaskletActorAnimMovementUpdates:General", true);
SettingT<INISettingCollection> bTaskletTempEffectsUpdate("bTaskletTempEffectsUpdate:General", false);
ProcessLists ReferenceProcessLists;

void UpdateAnimSceneGraph(void* apData, uint32_t aiIndex) {
    Animation* panim = (*static_cast<BSScrapArray<Animation*>*>(apData))[aiIndex];
    panim->UpdateQueuedScenegraph();
}

void UpdateAnimMovement(void* a1, uint32_t a2) {
    Actor* pActor = (*static_cast<BSScrapArray<Actor*>*>(a1))[a2];
    pActor->UpdateAnimationMovementNoWorldUpdate();
}

void UpdateTempEffect(void* apData, uint32_t aiIndex) {
    EffectTaskData& rData = (*static_cast<BSScrapArray<EffectTaskData, 1024>*>(apData))[aiIndex];
    if (!rData.pTempEffect->Update(rData.fDeltaTime)) {
        rData.bRemove = 1;
    }
}

void* fillchar(void* apDest, int32_t aiValue, uint32_t aCount) {
    return ourMemset(apDest, aiValue, aCount);
}

PROCESS_TYPE MobIterOperator::QProcessType() {
    return eType;
}

// The other fields are deliberately left unset, as the game does; kLock_10340 is only cleared, not constructed.
ProcessLists::ProcessLists()
  : MobProcessArray(), GlobalTempEffectList(), MagicEffectList(), ReferenceMuzzleFlashList(),
    ProjectilePostProcessList(), TempShouldMoveList(), AliveActorList(), fPlayerActionCommentTimer(0.0f),
    fPlayerKnockObjectCommentTimer(0.0f), bPlayerInRadiatedWater(false), bPlayerInRadiationArea(false), TaskManagerAI(),
    DialogPackageList(), kNearbyActorLock(), kUpdateLock(), kLipSyncManager(), bRunScheduals(true), bRunDetection(true),
    bShowDetectionStats(false), pStatdetect(nullptr), bProcessHigh(true), bProcessLow(true), bProcessMHigh(true),
    bProcessMLow(true), bProcessSche(true), bShowSubtitle(false), bUpdatingLowList(false), iNumberHighActors(0),
    fCrimeUpdateTimer(0.0f), iCrimeNumber(0), fRemoveExcessDeadTimer(gs_fRemoveExcessComplexDeadTime.Float()),
    MovementSyncSema(0, 40) {
    kLock_10340.uiOwningThread = 0;
    kLock_10340.uiLockCount = 0;
    // Clears the first list pointer only, as the game does.
    GlobalCrimeListArray[0] = nullptr;
    kNearbyActorLock.Lock(0);
    ourMemset(pActorsCloseToPlayerA, 0, sizeof(pActorsCloseToPlayerA));
    iActorsCloseToPlayer = 0;
    kNearbyActorLock.Unlock();
    QueryPerformanceFrequency(&ulTimerFrequency);
}

// Deletes every crime of the global crime lists and the lists themselves, and empties the projectile and actor lists.
ProcessLists::~ProcessLists() {
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[i];
        for (; pGlobalCrimeList && pGlobalCrimeList->m_item; pGlobalCrimeList->RemoveHead()) {
            delete pGlobalCrimeList->m_item;
        }
        delete pGlobalCrimeList;
    }
    ProjectilePostProcessList.RemoveAll();
    AliveActorList.RemoveAll();
}

void ProcessLists::Update3DAfterResting() {
    if (!bClosingAllSupportMenus) {
        for (uint32_t i = 0; i < MobProcessArray.GetSize(); ++i) {
            MobileObject* pMob = MobProcessArray.GetItem(i);
            Actor* pactor = 0;
            if (pMob) {
                if (pMob->IsActor()) {
                    pactor = static_cast<Actor*>(pMob);
                }
            }
            if (pactor) {
                TESObjectCELL* v1;
                if (!pactor->Get3D() ||
                    (pactor->GetParentCell() && (v1 = pactor->GetParentCell(), TES::pInstance->IsCellLoaded(v1, 0)))) {
                    if (!pactor->Get3D() && !pModelLoader->IsReferenceQueued(pactor) && !pactor->GetDisabled() &&
                        !pactor->GetDelete()) {
                        if (pactor->GetParentCell()) {
                            TESObjectCELL* ParentCell = pactor->GetParentCell();
                            if (TES::pInstance->IsCellLoaded(ParentCell, 1)) {
                                pactor->GetParentCell()->AddReference(pactor, 0);
                                i = 0;
                            }
                        }
                    }
                }
                else {
                    pactor->Set3D(0, 0);
                }
            }
        }
        TES::pInstance->CreateFurnitureList();
        PlaceActorsinBedsOrChairs();
    }
}

void ProcessLists::PrintActorsInDialog() {}

// The bound deliberately lets crime type 5 through, one past GlobalCrimeListArray's five entries, as the game does: its
// read there is the word after the array, GlobalTempEffectList's head item, which this index reads alike.
bool ProcessLists::IsCrimeinGlobalCrimeList(
    Actor* apTarget,
    Actor* apVictim,
    Actor* apAwareCrime,
    PackageNS::CRIME_TYPE aeType,
    TESForm* apform,
    int32_t aiCrimeNumber
) {
    if (aeType <= PackageNS::CRIME_TYPE_NONE || aeType > PackageNS::CRIME_TYPE_COUNT) {
        return 0;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    while (pitem && pitem->m_item) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->ecrime == aeType &&
            (pcrime->pcriminal == apVictim &&
             ((pcrime->pcrimetarget == apTarget || !apTarget) &&
              ((aiCrimeNumber == -1 || aiCrimeNumber == pcrime->iCrimeNumber) &&
               (!apAwareCrime || pcrime->DoesActorKnowaboutCrime(apAwareCrime)))))) {
            return 1;
        }
    }
    return 0;
}

bool ProcessLists::IsActorTalkingThroughActivator(Actor* apAct) {
    TESBoundObject* objectReference;
    uint32_t i;
    MobileObject* v6;
    for (i = 0; i < ((uint32_t)((int32_t)(&MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH))); ++i) {
        v6 = (&MobProcessArray)->GetItem(i);
        if (v6) {
            if (v6->IsTalkingActivator()) {
                objectReference = v6->TESObjectREFR::GetObjectReference();
                bool vCondition1;
                vCondition1 = static_cast<const void*>(static_cast<TESObjectCONT*>(objectReference)->GetCloseSound()) ==
                    static_cast<const void*>(apAct);
                if (vCondition1) {
                    return (bool)1;
                }
            }
        }
    }
    return (bool)0;
}

void ProcessLists::AddTempEffect(BSTempEffect* apTempEffect) {
    if (apTempEffect) {
        TEMP_EFFECT_TYPE eType = apTempEffect->GetType();
        if (eType < MAGIC_HIT_EFFECT_DEFAULT || eType > MAGIC_HIT_EFFECT_SHADER) {
            NiPointer<BSTempEffect> spEffect(apTempEffect);
            GlobalTempEffectList.AddTail(spEffect);
        }
        else {
            NiPointer<BSTempEffect> spEffect(apTempEffect);
            MagicEffectList.AddTail(spEffect);
        }
    }
}

void ProcessLists::FinishMagicShaderHitEffect(TESObjectREFR* apEffectTarget, TESEffectShader* apShader) {
    BSSimpleList<NiPointer<BSTempEffect>>* v3;
    MagicShaderHitEffect* v5;
    BSSimpleList<NiPointer<BSTempEffect>>* ecx0;
    if (!MagicEffectList.IsEmpty()) {
        for (ecx0 = &MagicEffectList; ecx0; ecx0 = ecx0->m_pkNext) {
            v3 = ecx0;
            v5 = static_cast<MagicShaderHitEffect*>(v3->m_item.m_pObject);
            if (v5) {
                if ((NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, v5) && (v5->pTarget == apEffectTarget)) &&
                    (v5->pEffectData == apShader)) {
                    v5->SetFinished((bool)1);
                }
            }
        }
    }
}

uint32_t ProcessLists::GetSaveSize() {
    uint32_t isize = 2;
    for (BSSimpleList<NiPointer<BSTempEffect>>* plist = &GlobalTempEffectList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect->CheckShouldSave()) {
            ++isize;
            isize += ptempEffect->GetSaveSize();
        }
    }
    for (BSSimpleList<NiPointer<BSTempEffect>>* plist = &MagicEffectList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect->CheckShouldSave()) {
            ++isize;
            isize += ptempEffect->GetSaveSize();
        }
    }
    return isize;
}

void ProcessLists::SaveTempEffectsList() {
    uint8_t ctempEffectType_2;
    uint8_t ctempEffectType;
    uint16_t icount = 0;
    BSSimpleList<NiPointer<BSTempEffect>>* plist;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    for (plist = &GlobalTempEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect->CheckShouldSave()) {
            ctempEffectType = ptempEffect->GetType();
            pSaveLoadGameOLD->SaveGameDataOLD(&ctempEffectType, 1u);
            ptempEffect->SaveGame();
            ++icount;
        }
    }
    for (plist = &MagicEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect_2 = plist->m_item.m_pObject;
        if (ptempEffect_2->CheckShouldSave()) {
            ctempEffectType_2 = ptempEffect_2->GetType();
            pSaveLoadGameOLD->SaveGameDataOLD(&ctempEffectType_2, 1u);
            ptempEffect_2->SaveGame();
            ++icount;
        }
    }
    *pcount = icount;
}

void ProcessLists::InitActorLeveledItems() {
    for (int32_t i = 0; i < 2; ++i) {
        uint32_t ProcessLevelHead;
        PROCESS_TYPE a2 = PROCESS_TYPE_LOW;
        if (i) {
            if (i == 1) {
                a2 = PROCESS_TYPE_MIDDLE_LOW;
            }
            ProcessLevelHead = MobProcessArray.GetProcessLevelHead(a2);
        }
        else {
            a2 = PROCESS_TYPE_LOW;
            ProcessLevelHead = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
        }
        for (uint32_t uiIndex = ProcessLevelHead; uiIndex < MobProcessArray.GetProcessLevelTail(a2); ++uiIndex) {
            Actor* v3 = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
            if (v3) {
                if (v3->IsActor() && !v3->IsDead(0)) {
                    v3->ResetInventory(1);
                }
            }
        }
    }
}

void ProcessLists::InsertActorCloseToPlayer(Actor* apActor) {
    if (apActor) {
        bhkCharacterController* pcon = apActor->GetCharController();
        if (pcon) {
            float fdist = pcon->GetLODDistance();
            kNearbyActorLock.Lock(0);
            bool ballreadyExists = 0;
            for (int32_t icurActor = 0; icurActor < iActorsCloseToPlayer; ++icurActor) {
                if (pActorsCloseToPlayerA[icurActor] == apActor) {
                    ballreadyExists = 1;
                    break;
                }
            }
            if (!ballreadyExists) {
                int32_t ilocToInsert;
                for (ilocToInsert = 0; ilocToInsert < iActorsCloseToPlayer; ++ilocToInsert) {
                    bhkCharacterController* potherCon = pActorsCloseToPlayerA[ilocToInsert]->GetCharController();
                    if (potherCon) {
                        if (static_cast<double>(fdist) < potherCon->GetLODDistance()) {
                            break;
                        }
                    }
                }
                if (ilocToInsert < 50 && ilocToInsert < iAINumberActorsComplexScene.Int()) {
                    BSmemmove(
                        &pActorsCloseToPlayerA[ilocToInsert + 1], sizeof(Actor*) * 50,
                        &pActorsCloseToPlayerA[ilocToInsert], sizeof(Actor*) * (49 - ilocToInsert)
                    );
                    pActorsCloseToPlayerA[ilocToInsert] = apActor;
                    if (iActorsCloseToPlayer < iAINumberActorsComplexScene.Int() && iActorsCloseToPlayer < 50) {
                        ++iActorsCloseToPlayer;
                    }
                }
            }
            kNearbyActorLock.Unlock();
        }
    }
}

void ProcessLists::SortActorsCloseToPlayer() {
    int32_t igap;
    kNearbyActorLock.Lock(0);
    int32_t iright = iActorsCloseToPlayer - 1;
    for (igap = 1; igap <= iright / 9; igap = 3 * igap + 1) {
        ;
    }
    while (igap > 0) {
        for (int32_t i = igap; i <= iright; ++i) {
            int32_t j = i;
            Actor* pswap = pActorsCloseToPlayerA[i];
            while (j >= igap && ProcessLists::DistanceListCompareFn(pswap, pActorsCloseToPlayerA[j - igap]) < 0) {
                pActorsCloseToPlayerA[j] = pActorsCloseToPlayerA[j - igap];
                j -= igap;
            }
            pActorsCloseToPlayerA[j] = pswap;
        }
        igap /= 3;
    }
    // Clears the last entry, then stores the decremented count.
    while (iActorsCloseToPlayer > iAINumberActorsComplexScene.Int()) {
        pActorsCloseToPlayerA[iActorsCloseToPlayer - 1] = 0;
        --iActorsCloseToPlayer;
    }
    kNearbyActorLock.Unlock();
}

void ProcessLists::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (uint32_t i = 0; i < 5; ++i) {
        for (BSSimpleList<Crime*>* pIter = GlobalCrimeListArray[i]; pIter;) {
            Crime* pCrime;
            pCrime = pIter->GetItem();
            if (pCrime) {
                pCrime->InitItem(apLoadGameBuffer);
            }
            pIter = pIter->GetNext();
        }
    }
    CalculateAliveActorList();
}

void ProcessLists::FinishGeometryDecalEffects() {
    bool bEmpty;
    bEmpty = GlobalTempEffectList.IsEmpty();
    if (bEmpty) {
        return;
    }
    for (BSSimpleList<NiPointer<BSTempEffect>>* pIter = &GlobalTempEffectList; pIter;) {
        NiPointer<BSTempEffect>* pspEffect;
        pspEffect = &pIter->GetItem();
        BSTempEffect* pEffect;
        pEffect = pspEffect->operator->();
        if (pEffect) {
            bool bDecal;
            bDecal = NiObject::IsKindOf(&BSTempEffectGeometryDecal::ms_RTTI, pEffect);
            if (bDecal) {
                static_cast<BSTempEffectGeometryDecal*>(pEffect)->SetFinished(true);
            }
        }
        pIter = pIter->GetNext();
    }
}

Crime* ProcessLists::GetCrimeByIndex(PackageNS::CRIME_TYPE aeType, uint16_t aiCrimeIndex) {
    uint16_t usIndex = 0;
    for (BSSimpleList<Crime*>* pIter = GlobalCrimeListArray[aeType]; pIter;) {
        bool bEmpty;
        bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        if (usIndex == aiCrimeIndex) {
            Crime* pItem;
            pItem = pIter->GetItem();
            return pItem;
        }
        ++usIndex;
        pIter = pIter->GetNext();
    }
    Warning("AI: When trying to get a crime by its index, the index was larger than the size of the crime list.");
    return nullptr;
}

void ProcessLists::CalculateAliveActorList() {
    AliveActorList.RemoveAll();
    ProcessArray* ProcessArray_ = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t uiIndex = ProcessArray_->GetProcessLevelHead(PROCESS_TYPE_HIGH);; ++uiIndex) {
        uint32_t ProcessLevelTail = ProcessArray_->GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (uiIndex >= ProcessLevelTail) {
            break;
        }
        Actor* Item = static_cast<Actor*>(ProcessArray_->GetItem(uiIndex));
        if (Item) {
            if (Item->IsActor()) {
                Actor* a2 = Item;
                if (!Item->IsDead(0)) {
                    AliveActorList.AddHead(a2);
                }
            }
        }
    }
}

void ProcessLists::UpdateHighListPackages(float afDelta, bool abSleeping) {
    Actor* ptarget;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PROCESSES, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\ProcessLists.cpp", 565
    );
    HighProcess::ClearWanderStarted();
    int32_t uiUpdatedActors = 0;
    int32_t inotupdated = 0;
    iNumberHighActors = 0;
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pmob = MobProcessArray.GetItem(i);
        if (pmob) {
            if (!pmob->IsStillLoading()) {
                if (pmob->GetDelete() ||
                    (pmob->GetDisabled() ||
                     (!pmob->pCurrentProcess || (pmob->GetCurrentProcessType() || ProcessLists::bIsForciblyUpdated)))) {
                    if (pmob->GetDisabled()) {
                        AddActorToTempChangeList(pmob);
                    }
                }
                else {
                    Actor* pact = 0;
                    bool updatefollower = 1;
                    if (pmob) {
                        if (pmob->IsActor()) {
                            pact = static_cast<Actor*>(pmob);
                            ++iNumberHighActors;
                        }
                    }
                    if (pact && pact->CanSkipActorUpdate()) {
                        BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
                        CurrentAIProcess->CheckforNewPackage(pact, 0);
                        updatefollower = 0;
                        ++inotupdated;
                        static_cast<HighProcess*>(pact->pCurrentProcess)->HighProcess::SetSkippedLastUpdate(1);
                    }
                    else {
                        pmob->Process(afDelta);
                        ++uiUpdatedActors;
                    }
                    if (pact) {
                        if (pPlayer->IsSleepingorResting()) {
                            pact->RestoreHealth(2.0f);
                            pact->RestoreFatigue(2.0f);
                        }
                    }
                    if (updatefollower) {
                        ExtraFollower* pfollow;
                        // &pmob->m_Extra is never null.
                        pfollow = (&pmob->m_Extra)->GetFollowerExtra();
                        ExtraFollower* pfollow_2 = pfollow;
                        if (pfollow) {
                            BSSimpleList<Actor*>* pHead;
                            BSSimpleList<Actor*>* pTempList;
                            BSSimpleList<Actor*>* v13 =
                                static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
                            if (v13) {
                                pTempList = ::new (v13) BSSimpleList<Actor*>;
                            }
                            else {
                                pTempList = 0;
                            }
                            BSSimpleList<Actor*>* v12 =
                                static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
                            if (v12) {
                                pHead = ::new (v12) BSSimpleList<Actor*>;
                            }
                            else {
                                pHead = 0;
                            }
                            BSSimpleList<Actor*>* plist = pHead;
                            for (BSSimpleList<Actor*>* ptemp = pfollow_2->pActorlist; ptemp && ptemp->m_item;
                                 ptemp = ptemp->m_pkNext) {
                                BSSimpleList<Actor*>* pFollowerToRemove = ptemp;
                                pHead->AddTail(pFollowerToRemove->m_item);
                            }
                            while (plist && plist->m_item) {
                                ptarget = plist->m_item;
                                if (ptarget) {
                                    if (ptarget != pPlayer) {
                                        if (ptarget->pCurrentProcess) {
                                            TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                                            TESPackage* pEditorpack = (&ptarget->m_Extra)->GetPackageExtra();
                                            if (pEditorpack) {
                                                ptargpack = pEditorpack;
                                            }
                                            if (ptargpack &&
                                                (ptargpack->GetPackType() == PACKAGE_TYPE_FOLLOW ||
                                                 ptargpack->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
                                                if (ptarget->GetCurrentProcessType()) {
                                                    float fHour = appCalendar.GetHour() - 1.0;
                                                    ptarget->pCurrentProcess->SetLastTimeProcessed(fHour);
                                                }
                                            }
                                            else {
                                                pTempList->AddHead(ptarget);
                                            }
                                        }
                                    }
                                }
                                plist = plist->m_pkNext;
                            }
                            for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
                                Actor* apActor = plist->m_item;
                                (&pmob->m_Extra)->RemoveFollower(apActor);
                            }
                            pTempList->RemoveAll();
                            if (pTempList) {
                                delete pTempList;
                            }
                            pHead->RemoveAll();
                            if (pHead) {
                                delete pHead;
                            }
                            if (pfollow_2->pActorlist->IsEmpty()) {
                                (&pmob->m_Extra)->RemoveFollowerExtra();
                            }
                        }
                    }
                }
            }
        }
    }
    float pFollowerToRemovea = appTimer.fDelta;
    DisableExcessDeadActors(pFollowerToRemovea);
}

void ProcessLists::RunDetectionForAllActors() {
    bool b360LineofSight;
    Actor* pmob = 0;
    MobileObject* pMobile = 0;
    uint32_t uiHighActorCount = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
    float ftime = appTimer.fDelta * static_cast<double>(uiHighActorCount);
    ftime = BSMIN(ftime, 5.0f);
    if (!uiHighActorCount) {
        pPlayer->SetHostileDetection(0);
    }
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);; ++i) {
        uint32_t uiHighActorCount_2 = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= uiHighActorCount_2) {
            break;
        }
        pMobile = MobProcessArray.GetItem(i);
        if (pMobile) {
            if (pMobile->IsActor()) {
                pmob = static_cast<Actor*>(pMobile);
                Actor* pdetectact = static_cast<Actor*>(pMobile);
                if (pdetectact->bProcessMe) {
                    if (pdetectact->Get3D()) {
                        float fdistancefromplayer = pdetectact->GetDistanceFromReference(pPlayer, 0, 0);
                        if (pdetectact != pPlayer && !pSaveLoadGameOLD->GetLoading()) {
                            float fDetectionTimer = pmob->GetDetectionTimer();
                            if (!i) {
                                pPlayer->SetHostileDetection(0);
                            }
                            if (!(fDetectionTimer < 0.0) || (pdetectact->IsUnConscious() || pmob->IsDead(0))) {
                                if (fDetectionTimer < 0.0) {
                                    pPlayer->RemovePerceivedActor(pmob);
                                }
                                pmob->ReduceDetectionTimer();
                            }
                            else {
                                if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) <= 0.0) {
                                    float fevalTime = RandomFloat(0.0f, ftime);
                                    pmob->EvaluateDetection(fevalTime);
                                    TESActorBaseData* pBaseData =
                                        static_cast<TESActorBaseData*>(pdetectact->GetActorBase());
                                    if (!pBaseData->GetIsGhost()) {
                                        BaseProcess* pBaseProcess = pdetectact->pCurrentProcess;
                                        if (pBaseProcess->ShouldRunPlayerDetection(pdetectact)) {
                                            bool blinesight = 0;
                                            bool bcombat = pmob->IsInCombat();
                                            b360LineofSight = 0;
                                            int32_t iplayerdetect = pPlayer->GetDetectionLevelAgainstActor(
                                                1, pmob, blinesight, bcombat, 0, 0, &b360LineofSight
                                            );
                                            FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                                            bool battack = 0;
                                            if (pmob->GetShouldAttackActor(pPlayer, 0, ereaction, 0) ||
                                                pmob->IsInCombatWithActor(pPlayer)) {
                                                battack = 1;
                                            }
                                            else {
                                                if (ereaction == FIGHT_REACTION_NEUTRAL ||
                                                    ereaction == FIGHT_REACTION_ENEMY) {
                                                    if (pmob->GetActorBase()->AIData.bAggroRadius) {
                                                        if (!pmob->ShouldSkipFallOutBehavior(
                                                                FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR
                                                            )) {
                                                            battack = 1;
                                                        }
                                                    }
                                                }
                                            }
                                            if (pmob) {
                                                float* v2 = &fShaderShadowUpdateDistance.Float();
                                                if (*v2 > static_cast<double>(fdistancefromplayer)) {
                                                    ReferenceProcessLists.InsertActorCloseToPlayer(pdetectact);
                                                }
                                            }
                                            double dDetectionValue = static_cast<double>(iplayerdetect);
                                            if (!(fPlayerDetectActorValue.Float() < dDetectionValue)) {
                                                if (b360LineofSight || blinesight) {
                                                    pPlayer->InsertPerceivedActor(pmob, battack, 1);
                                                }
                                                else {
                                                    pPlayer->RemovePerceivedActor(pmob);
                                                }
                                            }
                                            else {
                                                pPlayer->InsertPerceivedActor(pmob, battack, 0);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void ProcessLists::UpdateActorsMovement(float afDelta, bool abSleeping, PROCESS_TYPE apLevel) {
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = parray->GetProcessLevelHead(apLevel); i < parray->GetProcessLevelTail(apLevel); ++i) {
        MobileObject* pact = parray->GetItem(i);
        if (!pact || !pact->IsStillLoading()) {
            if (pact && pact->IsActor()) {
                if (apLevel) {
                    double v4 = static_cast<double>(appTimer.iLastTime) / 1000.0;
                    afDelta = v4 - pact->pCurrentProcess->fHourLastProcessed;
                }
                // Passes the raw byte of abSleeping on.
                static_cast<Actor*>(pact)->UpdateMovement(afDelta, abSleeping);
                if (apLevel == PROCESS_TYPE_HIGH) {
                    static_cast<HighProcess*>(pact->pCurrentProcess)
                        ->HighProcess::DetectDoorsForPathing(static_cast<Actor*>(pact));
                }
            }
            else {
                if (pact->IsProjectile()) {
                    static_cast<Projectile*>(pact)->Projectile::UpdateIfNotInArrowsList(0.0f);
                }
                else {
                    if (pact->IsExplosion()) {
                        static_cast<Explosion*>(pact)->Explosion::ProcessMovement(0.0f);
                    }
                }
            }
        }
    }
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::IsActorTargetingREFinPackage(TESObjectREFR* apActor, PACKAGE_TYPE apType, bool bOnlyHigh) {
    BSSimpleList<TESObjectREFR*>* preflist_4 = 0;
    MobileObject* v28 = 0;
    bool v31 = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            uint32_t v5;
            if (!bOnlyHigh || (v5 = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH), i < v5)) {
                uint32_t v7;
                uint32_t ProcessLevelHead = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_LOW);
                v31 = i >= ProcessLevelHead &&
                    (v7 = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_LOW), i < v7);
                Actor* pactor = 0;
                if (pMob->IsActor()) {
                    pactor = static_cast<Actor*>(pMob);
                }
                if (pactor) {
                    if (pactor->IsInCombat() && apType == PACKAGE_TYPE_COMBAT) {
                        TESPackage* pbasepack = pactor->GetPackageSetAsPcurrent();
                        CombatController* ppack = 0;
                        if (pbasepack) {
                            if (pbasepack->GetPackType() == PACKAGE_TYPE_COMBAT) {
                                ppack = static_cast<CombatController*>(pbasepack);
                            }
                        }
                        if (bOnlyHigh && ppack) {
                            if (ppack->GetTarget() == apActor) {
                                if (!preflist_4) {
                                    BSSimpleList<TESObjectREFR*>* preflist;
                                    BSSimpleList<TESObjectREFR*>* v21 = static_cast<BSSimpleList<TESObjectREFR*>*>(
                                        ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>))
                                    );
                                    if (v21) {
                                        preflist = ::new (v21) BSSimpleList<TESObjectREFR*>;
                                    }
                                    else {
                                        preflist = 0;
                                    }
                                    BSSimpleList<TESObjectREFR*>* v22 = preflist;
                                    preflist_4 = preflist;
                                }
                                if (v31 && (pactor->IsCreature() || !pactor->GetRefPersists())) {
                                    goto LABEL_30;
                                }
                                TESObjectREFR* a2 = pactor;
                                preflist_4->AddHead(a2);
                            }
                        }
                        else {
                            if (ppack && ppack->IsActoraCombatTarget(static_cast<Actor*>(apActor))) {
                                if (!preflist_4) {
                                    BSSimpleList<TESObjectREFR*>* preflist_2;
                                    BSSimpleList<TESObjectREFR*>* v18 = static_cast<BSSimpleList<TESObjectREFR*>*>(
                                        ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>))
                                    );
                                    if (v18) {
                                        preflist_2 = ::new (v18) BSSimpleList<TESObjectREFR*>;
                                    }
                                    else {
                                        preflist_2 = 0;
                                    }
                                    BSSimpleList<TESObjectREFR*>* v19 = preflist_2;
                                    preflist_4 = preflist_2;
                                }
                                if (v31 && (pactor->IsCreature() || !pactor->GetRefPersists())) {
                                LABEL_30:;
                                    pactor->StopCombat(0);
                                    continue;
                                }
                                TESObjectREFR* item = pactor;
                                preflist_4->AddHead(item);
                            }
                        }
                    }
                    else {
                        TESPackage* ppack_2 = pactor->GetCurrentPackage();
                        if (ppack_2) {
                            if (pactor->IsActoraGuard() || apType != PACKAGE_TYPE_ALARM) {
                                BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                                if (CurrentAIProcess->GetTarget() == apActor && ppack_2->GetPackType() == apType) {
                                    if (!preflist_4) {
                                        BSSimpleList<TESObjectREFR*>* preflist_3;
                                        BSSimpleList<TESObjectREFR*>* v15 = static_cast<BSSimpleList<TESObjectREFR*>*>(
                                            ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>))
                                        );
                                        if (v15) {
                                            preflist_3 = ::new (v15) BSSimpleList<TESObjectREFR*>;
                                        }
                                        else {
                                            preflist_3 = 0;
                                        }
                                        BSSimpleList<TESObjectREFR*>* v16 = preflist_3;
                                        preflist_4 = preflist_3;
                                    }
                                    TESObjectREFR* v14 = pactor;
                                    preflist_4->AddHead(v14);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return preflist_4;
}

Actor* ProcessLists::GetActorRefInHigh(TESNPC* apNpc, bool abDetection) {
    Actor* pResult = 0;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        TESForm* pMobileObject = MobProcessArray.GetItem(uiIndex);
        if (pMobileObject) {
            if (!pResult && !pMobileObject->IsStillLoading()) {
                Actor* pActor = 0;
                if (pMobileObject->IsActor()) {
                    pActor = static_cast<Actor*>(pMobileObject);
                }
                if (pActor) {
                    TESNPC* pBaseForm = 0;
                    if (pActor->IsNpc()) {
                        pBaseForm = static_cast<TESNPC*>(pActor->GetObjectReference());
                    }
                    if (pBaseForm && apNpc == pBaseForm) {
                        pResult = pActor;
                    }
                    else {
                        ExtraDataList* Extra = pActor->GetExtra();
                        if (Extra->GetLevCreaOriginalBase() == apNpc) {
                            pResult = pActor;
                        }
                    }
                }
            }
        }
    }
    return pResult;
}

void ProcessLists::AddReference(
    MobileObject* apActor,
    PROCESS_TYPE aWhichList,
    bool aiWhere,
    bool abinsert,
    MobileObject* apInsertActor
) {
    MobProcessArray.AddActor(apActor, aWhichList);
}

void ProcessLists::RemoveReference(MobileObject* apActor, PROCESS_TYPE aWhichList) {
    MobProcessArray.RemoveActor(apActor, aWhichList);
}

void ProcessLists::SetPlayerInRadiatedWater(bool abIrradiated) {
    bPlayerInRadiationArea = abIrradiated;
}

void ProcessLists::RunActorScripts() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob) {
            continue;
        }
        if (pMob->IsActor()) {
            pMob->RunScript();
        }
    }
}

void ProcessLists::RunActorMagicUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor()) {
            static_cast<Actor*>(pMob)->UpdateMagic(false, false);
        }
    }
}

void ProcessLists::RunActorUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        if (pActor->bProcessMe) {
            pMob->Update(0.0f);
        }
    }
}

void ProcessLists::RunActorRagdollAnimationUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        bool bRagdoll = pActor->pRagdollController && pActor->pRagdollController->GetRagdollAnimEnable();
        if (!pMob->Get3D() || !bRagdoll || static_cast<uint8_t>(pMob->Get3D()->GetNotVisible()) ||
            pMob->IsInDialoguewithPlayer()) {
            if (bRagdoll) {
                pActor->pRagdollController->ReinitRBControllerOnUpdate(true);
            }
        }
        else {
            pActor->UpdateRagdollAnimation(1);
        }
    }
}

void ProcessLists::RunActorAnimationUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        TESObjectCELL* pCell = pMob->GetParentCell();
        if (pMob->Get3D() && !pMob->GetDelete() && !pMob->GetDisabled() && pCell && pCell->GetAttached() &&
            pActor->bProcessMe) {
            pMob->UpdateAnimation(0.0f);
        }
    }
}

void ProcessLists::SkipHighListFadeIn() {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob) {
            continue;
        }
        if (pMob->IsActor()) {
            static_cast<HighProcess*>(pMob->GetCurrentAIProcess())->SkipFadeIn(static_cast<Actor*>(pMob));
        }
    }
}

void ProcessLists::ReconcileDetectionDataForActor() {
    if (pPlayer->GetCurrentAIProcess()) {
        static_cast<HighProcess*>(pPlayer->GetCurrentAIProcess())->ReconcileDetectionData(pPlayer);
    }

    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob) {
            continue;
        }
        if (pMob->IsActor()) {
            static_cast<HighProcess*>(pMob->GetCurrentAIProcess())->ReconcileDetectionData(static_cast<Actor*>(pMob));
        }
    }
}

void ProcessLists::InsertAliveActor(Actor* apActor) {
    if (apActor && !AliveActorList.IsInList(apActor)) {
        AliveActorList.AddHead(apActor);
    }
}

float ProcessLists::GetSystemTimeClock() {
    return fSystemTimeClock;
}

void ProcessLists::IterateMobs(MobIterOperator& arIterOp) {
    uint32_t uiTail = MobProcessArray.GetProcessLevelTail(arIterOp.QProcessType());
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(arIterOp.QProcessType()); i < uiTail; ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && !pMob->IsStillLoading()) {
            arIterOp.Operate(pMob);
        }
    }
}

// Calls GetProcessLevelTail again on every pass.
uint32_t ProcessLists::CountHighActors() {
    uint32_t uiCount = 0;
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob) {
            continue;
        }
        // Tests the whole byte returned by IsActor.
        bool bActor = pMob->IsActor();
        if (static_cast<uint8_t>(bActor) != 0) {
            ++uiCount;
        }
    }
    return uiCount;
}

bool ProcessLists::IsActorNCloseToPlayer(Actor* apActor, int32_t aiNumber) {
    if (!apActor || apActor == pPlayer) {
        return true;
    }
    kNearbyActorLock.Lock(nullptr);
    aiNumber = BSMIN(aiNumber, static_cast<int32_t>(iActorsCloseToPlayer));
    bool bFound = false;
    for (int32_t i = 0; i < aiNumber; ++i) {
        if (pActorsCloseToPlayerA[i] == apActor) {
            bFound = true;
            break;
        }
    }
    kNearbyActorLock.Unlock();
    return bFound;
}

void ProcessLists::RemoveActorFromTempChangeList(MobileObject* apActor) {
    if (apActor == pCPLRef) {
        pCPLRef = nullptr;
    }
    TempShouldMoveList.Remove(apActor);
    if (apActor) {
        apActor->bInTempChangeList = false;
    }
}

void ProcessLists::AddActorToTempChangeList(MobileObject* apActor) {
    if (TempShouldMoveList.IsInList(apActor)) {
        return;
    }
    if (!apActor->IsActor()) {
        // a mob that is no actor always moves, without the in-list flag
        apActor->SetNeedToChangeProcess(true);
        TempShouldMoveList.AddHead(apActor);
        return;
    }
    if (!apActor->GetCurrentAIProcess()) {
        return;
    }
    PROCESS_TYPE eCurrentLevel = apActor->GetCurrentAIProcess()->GetProcessLevel();
    PROCESS_TYPE eDesiredLevel = apActor->GetDesiredProcessLevel();
    if (apActor->IsDead(false) || eCurrentLevel != eDesiredLevel || apActor->GetCurrentAIProcess()->IsAFollower() ||
        apActor->GetDelete() || pPlayer->IsSleepingorResting()) {
        apActor->SetNeedToChangeProcess(true);
        TempShouldMoveList.AddHead(apActor);
        if (apActor) {
            apActor->bInTempChangeList = true;
        }
    }
}

void ProcessLists::UpdateFollowerTempList() {
    for (BSSimpleList<void*>* pIter = &TempShouldMoveList; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        Actor* pActor = nullptr;
        if (static_cast<MobileObject*>(pIter->GetItem())->IsActor()) {
            pActor = static_cast<Actor*>(pIter->GetItem());
        }
        if (!pActor || pActor->IsStillLoading() || !pActor->GetCurrentAIProcess()) {
            continue;
        }
        if (!pActor->GetCurrentAIProcess()->GetPackageThatIsRunning()) {
            continue;
        }
        TESObjectREFR* pTarget = pActor->GetCurrentPackageTarget();
        TESPackage* pPackage = pActor->GetCurrentAIProcess()->GetPackageThatIsRunning();
        if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_ESCORT) {
            TESObjectREFR* pEscortTarget = pActor->GetCurrentPackageTarget();
            if (pEscortTarget && pEscortTarget->IsActor()) {
                pActor->GetCurrentAIProcess()->CreateFollowforEscort(pActor, pPackage, false);
            }
        }
        else if (pTarget && pTarget->IsActor() && pTarget != pPlayer) {
            pActor->AddFollower(static_cast<Actor*>(pTarget));
        }
    }
}

void ProcessLists::MoveFollowersOnToPlayer() {
    for (uint32_t i = 0; i < MobProcessArray.GetSize(); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        if (!pActor || pActor->IsDead(false)) {
            continue;
        }
        if ((pPlayer->IsActorFollowingPlayer(pActor) &&
             pPlayer->GetDistanceFromReference(pActor, false, false) > 350.0) ||
            pActor->IsPlayerTeammate()) {
            pActor->EndInterruptPackage(false);
            pActor->MoveActorAndFollowers(pPlayer, false, false);
        }
    }
}

// The list walk is the BSSimpleList iterator used where items may be removed: past the first node the iterator moves on
// before the item is handled, and a removal from the head node keeps it on the head.
void ProcessLists::UpdateReferenceMuzzleFlashes(float afDeltaTime) {
    if (ReferenceMuzzleFlashList.IsEmpty()) {
        return;
    }
    BSSimpleList<void*>* pIter = &ReferenceMuzzleFlashList;
    bool bFirst = true;
    while (pIter) {
        MuzzleFlash* pFlash = static_cast<MuzzleFlash*>(pIter->GetItem());
        if (!bFirst) {
            pIter = pIter->GetNext();
        }
        bool bRemove = pFlash != nullptr;
        if (pFlash) {
            bRemove = !pFlash->GetEnabled();
            pFlash->Update(afDeltaTime, pFlash->GetParentRef());
            if (bRemove) {
                ReferenceMuzzleFlashList.Remove(pFlash);
                if (pFlash) {
                    delete pFlash;
                }
            }
        }
        if (!bRemove && bFirst) {
            pIter = pIter->GetNext();
            bFirst = false;
        }
    }
}

void ProcessLists::RemoveAliveActor(Actor* apActor) {
    (&AliveActorList)->Remove(apActor);
}

void ProcessLists::RemoveActorCloseToPlayer(Actor* pactor) {
    kNearbyActorLock.Lock(0);
    for (int32_t icurActor = 0; icurActor < iActorsCloseToPlayer; ++icurActor) {
        if (pActorsCloseToPlayerA[icurActor] == pactor) {
            BSmemmove(
                &pActorsCloseToPlayerA[icurActor], sizeof(pActorsCloseToPlayerA), &pActorsCloseToPlayerA[icurActor + 1],
                sizeof(Actor*) * (50 - icurActor - 1)
            );
            pActorsCloseToPlayerA[iActorsCloseToPlayer - 1] = nullptr;
            --iActorsCloseToPlayer;
            break;
        }
    }
    kNearbyActorLock.Unlock();
}

void ProcessLists::RemoveActorInDetectionLists(Actor* apActor) {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = 0;
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        if (pactor) {
            BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
            CurrentAIProcess->RemoveDetectionActor(apActor, 3);
        }
    }
}

void ProcessLists::NullOutTargetRef(TESObjectREFR* apRef, bool abDeleted) {
    TESObjectREFR* item;
    uint32_t i;
    if (apRef->IsActor()) {
        if ((&pPlayer->m_Extra)->GetFollowerExtra()) {
            if ((&pPlayer->m_Extra)->GetFollowerExtra()->pActorlist) {
                item = apRef;
                Actor* pItemActor = static_cast<Actor*>(item);
                (&pPlayer->m_Extra)->GetFollowerExtra()->pActorlist->Remove(pItemActor);
            }
        }
        if (pPlayer->pCurrentProcess) {
            BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
            CurrentAIProcess->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
        }
        RemoveAliveActor(static_cast<Actor*>(apRef));
    }
    if (pPlayer->GetDialogueItemTarget() == apRef) {
        pPlayer->SetDialogueItemTarget(0);
    }
    if (pPlayer->pCurrentProcess) {
        BaseProcess* v29 = pPlayer->pCurrentProcess;
        if (v29->GetLastHeadTrackTarget() == apRef) {
            BaseProcess* v28 = pPlayer->pCurrentProcess;
            v28->ClearLastHeadTrackTarget();
        }
        BaseProcess* v27 = pPlayer->pCurrentProcess;
        if (v27->GetLastGreeted() == apRef) {
            BaseProcess* v26 = pPlayer->pCurrentProcess;
            v26->SetLastGreeted(0);
        }
        BaseProcess* pproc = pPlayer->pCurrentProcess;
        DetectionEvent* pevent = pproc->GetActorsDetectionEvent(pPlayer);
        if (pevent) {
            if (pevent->pRef == apRef) {
                if (pPlayer->pCurrentProcess) {
                    BaseProcess* v24 = pPlayer->pCurrentProcess;
                    v24->RemoveDetectionEvent();
                }
            }
        }
    }
    if (apRef->IsActor()) {
        pPlayer->RemoveActorInDispositionModifier(static_cast<Actor*>(apRef));
    }
    MobileObject* v73 = 0;
    for (i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = 0;
        Projectile* pprojectile = 0;
        if (pMob) {
            if (!pMob->IsStillLoading()) {
                if (pMob->IsActor()) {
                    pactor = static_cast<Actor*>(pMob);
                }
                else {
                    if (pMob->IsProjectile()) {
                        pprojectile = static_cast<Projectile*>(pMob);
                    }
                }
                BSSimpleList<Actor*>* ptemp = 0;
                ExtraFollower* pfollow = 0;
                if (pactor) {
                    pfollow = (&pactor->m_Extra)->GetFollowerExtra();
                }
                if (pfollow) {
                    ptemp = pfollow->pActorlist;
                }
                if (pactor) {
                    if ((&pactor->m_Extra)->GetFollowerExtra()) {
                        if (apRef->IsActor()) {
                            Actor* pact_2 = static_cast<Actor*>(apRef);
                            (&pactor->m_Extra)->RemoveFollower(static_cast<Actor*>(apRef));
                        }
                    }
                }
                if (pprojectile) {
                    if (pprojectile->GetShooter() == apRef) {
                        pprojectile->pShooter = 0;
                    }
                }
                if (!pMob || pactor) {
                    while (pactor) {
                        pactor->HandleDeletedReferenceForActorMover(apRef);
                        // Tests the address &pactor->m_Extra, which is never null.
                        if ((&pactor->m_Extra)->GetHeadTrackTargetExtra() == apRef) {
                            (&pactor->m_Extra)->SetHeadTrackTargetExtra(0);
                        }
                        BaseProcess* pproc_2 = pactor->pCurrentProcess;
                        if (pactor->GetDialogueItemTarget() == apRef) {
                            pactor->SetDialogueItemTarget(0);
                            if (pproc_2) {
                                pproc_2->SetGreetingFlag(0);
                                pproc_2->ClearMovementStoped();
                            }
                        }
                        if (pproc_2) {
                            if (pproc_2->eLevel < PROCESS_TYPE_MIDDLE_LOW) {
                                pproc_2->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
                                pproc_2->RemoveAvoidPathingNode(apRef);
                                pproc_2->ClearPathLookTarget(apRef);
                                DetectionEvent* pevent_2 = pproc_2->GetActorsDetectionEvent(pPlayer);
                                if (pevent_2) {
                                    if (pevent_2->pRef == apRef) {
                                        pproc_2->RemoveDetectionEvent();
                                    }
                                }
                                if (pproc_2->GetLastHeadTrackTarget() == apRef) {
                                    pproc_2->ClearLastHeadTrackTarget();
                                }
                                if (pproc_2->GetLastGreeted() == apRef) {
                                    pproc_2->SetLastGreeted(0);
                                }
                                if (apRef->IsActor()) {
                                    pproc_2->RemoveSpokenToActor(static_cast<Actor*>(apRef));
                                }
                                TESPackage* ppack = pproc_2->GetRunOncePackage();
                                if (ppack) {
                                    ActorPackage* pdata = pproc_2->GetActorPackageThatIsRunning();
                                    if (pdata && pdata->pTarget == apRef) {
                                        pactor->EndInterruptPackage(0);
                                    }
                                    else {
                                        if (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                                            DialoguePackage* ppack_2 = static_cast<DialoguePackage*>(ppack);
                                            if (ppack_2->pTargetActor == apRef) {
                                                pactor->EndDialogue();
                                            }
                                        }
                                    }
                                }
                            }
                            pproc_2->ClearObjectFromAcquireList(apRef);
                            ExtraPackage* pextrapack = static_cast<ExtraPackage*>((&pactor->m_Extra)->GetExtraData(25));
                            if (pextrapack) {
                                if (pextrapack->pTarg == apRef) {
                                    pextrapack->pTarg = 0;
                                }
                            }
                            if (pactor->IsFleeing(0)) {
                                TESPackage* pbasepack = pactor->GetCurrentPackage();
                                FleePackage* v55 = 0;
                                if (pbasepack) {
                                    if (pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                                        v55 = static_cast<FleePackage*>(pbasepack);
                                    }
                                }
                                if (v55) {
                                    if (v55->pRefFleeTo == apRef) {
                                        v55->SetRefFleeTo(0);
                                    }
                                }
                                pactor->RemoveFleeTarget(apRef);
                            }
                            else {
                                if (pactor->IsinSpectator() && apRef->IsActor()) {
                                    TESPackage* pbasepack_2 = pactor->GetCurrentPackage();
                                    Actor* prefAsActor = static_cast<Actor*>(apRef);
                                    SpectatorPackage* pspec = 0;
                                    if (pbasepack_2) {
                                        if (pbasepack_2->GetPackType() == PACKAGE_TYPE_SPECTATOR) {
                                            pspec = static_cast<SpectatorPackage*>(pbasepack_2);
                                        }
                                    }
                                    if (pspec) {
                                        pspec->HandleDeletedActor(prefAsActor);
                                    }
                                }
                            }
                            pproc_2->HandleDeletedReference(apRef);
                            ExtraDataList* pextraData = &pactor->m_Extra;
                            if (pextraData) {
                                ActorPackageData* pactorPackageData = pextraData->GetActorPackageData();
                                if (pactorPackageData) {
                                    pactorPackageData->HandleDeletedReference(apRef);
                                }
                                TESObjectREFR* pextraTarget = pextraData->GetPackageExtraTarget();
                                if (pextraTarget == apRef) {
                                    pextraData->SetPackageExtraTarget(0);
                                }
                            }
                            if (apRef->IsActor()) {
                                // Tests apRef for null before taking its MagicTarget subobject.
                                MagicTarget* p_z;
                                if (apRef) {
                                    p_z = static_cast<Actor*>(apRef);
                                }
                                else {
                                    p_z = 0;
                                }
                                if (pproc_2->GetDesiredTarget() == p_z) {
                                    pproc_2->SetDesiredTarget(0);
                                }
                            }
                            if (!pproc_2->eLevel) {
                                pproc_2->ClearDialogTarget();
                            }
                            if (pactor->GetDialogueItemTarget() == apRef) {
                                pactor->SetDialogueItemTarget(0);
                            }
                            if (abDeleted) {
                                TESPackage* pcurrpack = pactor->GetCurrentPackage();
                                if (pcurrpack) {
                                    if (pcurrpack->pPackLoc) {
                                        if (pcurrpack->pPackLoc->GetLocReference() == apRef) {
                                            pcurrpack->pPackLoc->SetLocReference(0);
                                        }
                                    }
                                }
                                TESPackage* pprocpack = pproc_2->GetCurrentPackage();
                                if (pprocpack) {
                                    if (pprocpack->pPackLoc) {
                                        if (pprocpack->pPackLoc->GetLocReference() == apRef) {
                                            pprocpack->pPackLoc->SetLocReference(0);
                                        }
                                    }
                                }
                            }
                            CombatController* pcomcont = pactor->GetCombatController();
                            if (pcomcont) {
                                pcomcont->RemoveTarget(static_cast<Actor*>(apRef));
                            }
                            if (pproc_2->GetTarget() == apRef) {
                                pproc_2->SetTarget(0);
                            }
                            if (pactor->GetCurrentTarget() == apRef) {
                                pactor->SetCurrentTarget(0);
                            }
                            TESPackage* prunoncepack = pproc_2->GetRunOncePackage();
                            if (pactor->GetCurrentPackageTarget() == apRef ||
                                (prunoncepack &&
                                 (prunoncepack->GetPackageTarget() &&
                                  prunoncepack->GetPackageTarget()->GetTargReference() == apRef))) {
                                if (pactor->IsinDialogue()) {
                                    pactor->EndDialogue();
                                }
                                else {
                                    TESPackage* CurrentPackage = pactor->GetCurrentPackage();
                                    if (CurrentPackage->GetIsCreated()) {
                                        pactor->EndInterruptPackage(0);
                                    }
                                    else {
                                        pproc_2->ClearCurrentPackage();
                                        pproc_2->ClearRunOncePackage();
                                    }
                                }
                            }
                            pproc_2->RemoveHeadTrackTarget(apRef);
                            if (pproc_2->GetCurrentFurniture() == apRef) {
                                FurnitureMark* v5 = pproc_2->GetFurnitureMarker();
                                v5->SetHeading(0.0f);
                                FurnitureMark* v6 = pproc_2->GetFurnitureMarker();
                                v6->SetType(0);
                                pproc_2->GetFurnitureMarker()->SetPosition(NiPoint3_ZERO);
                                BaseProcess* v22 = pactor->pCurrentProcess;
                                FurnitureMark* v7 = v22->GetFurnitureMarker();
                                pproc_2->SetFurniture(0, 127, v7);
                            }
                            if (ptemp && ptemp->m_item) {
                                pactor = 0;
                                Actor* v8 = ptemp->m_item;
                                if (v8->IsActor()) {
                                    pactor = ptemp->m_item;
                                }
                                ptemp = ptemp->m_pkNext;
                            }
                            else {
                                pactor = 0;
                            }
                        }
                        else {
                            pactor = 0;
                        }
                    }
                }
                else {
                    if (pMob->GetTalkingActivator()) {
                        if (apRef == pMob->GetTalkingActivator()) {
                            pMob->EndDialogue();
                        }
                    }
                }
            }
        }
    }
    for (BSSimpleList<void*>* plist = &TempShouldMoveList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor_2 = 0;
        Projectile* pprojectile_2 = 0;
        TESObjectREFR* pref = static_cast<TESObjectREFR*>(plist->m_item);
        if (pref) {
            if (!pref->IsStillLoading()) {
                TESObjectREFR* m_item = static_cast<TESObjectREFR*>(plist->m_item);
                if (m_item->IsActor()) {
                    pactor_2 = static_cast<Actor*>(static_cast<TESObjectREFR*>(plist->m_item));
                }
                else {
                    TESObjectREFR* v19 = static_cast<TESObjectREFR*>(plist->m_item);
                    if (v19->IsProjectile()) {
                        pprojectile_2 = static_cast<Projectile*>(static_cast<TESObjectREFR*>(plist->m_item));
                    }
                }
                BSSimpleList<Actor*>* ptemp_2 = 0;
                ExtraFollower* pfollow_2 = 0;
                if (pactor_2) {
                    pfollow_2 = (&pactor_2->m_Extra)->GetFollowerExtra();
                }
                if (pfollow_2) {
                    ptemp_2 = pfollow_2->pActorlist;
                }
                if (pprojectile_2) {
                    if (pprojectile_2->GetShooter() == apRef) {
                        pprojectile_2->pShooter = 0;
                    }
                }
                while (pactor_2) {
                    BaseProcess* pproc_3 = pactor_2->pCurrentProcess;
                    if (pactor_2->GetDialogueItemTarget() == apRef) {
                        pactor_2->SetDialogueItemTarget(0);
                        if (pproc_3) {
                            pproc_3->SetGreetingFlag(0);
                            pproc_3->ClearMovementStoped();
                        }
                    }
                    if (pproc_3) {
                        if (i < 2) {
                            pproc_3->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
                        }
                        pproc_3->ClearObjectFromAcquireList(apRef);
                        ExtraPackage* pextrapack_2 = static_cast<ExtraPackage*>((&pactor_2->m_Extra)->GetExtraData(25));
                        if (pextrapack_2) {
                            if (pextrapack_2->pTarg == apRef) {
                                pextrapack_2->pTarg = 0;
                            }
                        }
                        if (pactor_2->IsFleeing(0)) {
                            TESPackage* pbasepack_3 = pactor_2->GetCurrentPackage();
                            FleePackage* v34 = 0;
                            if (pbasepack_3) {
                                if (pbasepack_3->GetPackType() == PACKAGE_TYPE_FLEE) {
                                    v34 = static_cast<FleePackage*>(pbasepack_3);
                                }
                            }
                            if (v34) {
                                if (v34->pRefFleeTo == apRef) {
                                    v34->SetRefFleeTo(0);
                                }
                            }
                            pactor_2->RemoveFleeTarget(apRef);
                        }
                        if (apRef->IsActor()) {
                            if (pactor_2->pCurrentProcess) {
                                // Tests apRef for null before taking its MagicTarget subobject.
                                MagicTarget* v18;
                                if (apRef) {
                                    v18 = static_cast<Actor*>(apRef);
                                }
                                else {
                                    v18 = 0;
                                }
                                BaseProcess* pproc_4 = pactor_2->pCurrentProcess;
                                if (pproc_4->GetDesiredTarget() == v18) {
                                    BaseProcess* pproc_5 = pactor_2->pCurrentProcess;
                                    pproc_5->SetDesiredTarget(0);
                                }
                            }
                        }
                        if (!pproc_3->eLevel) {
                            pproc_3->ClearDialogTarget();
                        }
                        if (pactor_2->GetDialogueItemTarget() == apRef) {
                            pactor_2->SetDialogueItemTarget(0);
                        }
                        TESPackage* pcurrpack_2 = pactor_2->GetCurrentPackage();
                        if (pcurrpack_2) {
                            if (pcurrpack_2->pPackLoc) {
                                if (pcurrpack_2->pPackLoc->GetLocReference() == apRef) {
                                    pcurrpack_2->pPackLoc->SetLocReference(0);
                                }
                            }
                        }
                        TESPackage* pprocpack_2 = pproc_3->GetCurrentPackage();
                        if (pprocpack_2) {
                            if (pprocpack_2->pPackLoc) {
                                if (pprocpack_2->pPackLoc->GetLocReference() == apRef) {
                                    pprocpack_2->pPackLoc->SetLocReference(0);
                                }
                            }
                        }
                        CombatController* pcomcont_2 = pactor_2->GetCombatController();
                        if (pcomcont_2) {
                            pcomcont_2->RemoveTarget(static_cast<Actor*>(apRef));
                        }
                        if (pproc_3->GetTarget() == apRef) {
                            pproc_3->SetTarget(0);
                        }
                        if (pactor_2->GetCurrentTarget() == apRef) {
                            pactor_2->SetCurrentTarget(0);
                        }
                        TESPackage* prunoncepack_2 = pproc_3->GetRunOncePackage();
                        if (pactor_2->GetCurrentPackageTarget() == apRef ||
                            (prunoncepack_2 &&
                             (prunoncepack_2->GetPackageTarget() &&
                              prunoncepack_2->GetPackageTarget()->GetTargReference() == apRef))) {
                            if (pactor_2->IsinDialogue()) {
                                pactor_2->EndDialogue();
                            }
                            else {
                                TESPackage* v10 = pactor_2->GetCurrentPackage();
                                if (v10->GetIsCreated()) {
                                    pactor_2->EndInterruptPackage(0);
                                }
                                else {
                                    pproc_3->ClearCurrentPackage();
                                    pproc_3->ClearRunOncePackage();
                                }
                            }
                        }
                        pproc_3->RemoveHeadTrackTarget(apRef);
                        if (pproc_3->GetCurrentFurniture() == apRef) {
                            FurnitureMark* v11 = pproc_3->GetFurnitureMarker();
                            v11->SetHeading(0.0f);
                            FurnitureMark* v12 = pproc_3->GetFurnitureMarker();
                            v12->SetType(0);
                            pproc_3->GetFurnitureMarker()->SetPosition(NiPoint3_ZERO);
                            BaseProcess* v15 = pactor_2->pCurrentProcess;
                            FurnitureMark* v13 = v15->GetFurnitureMarker();
                            pproc_3->SetFurniture(0, 127, v13);
                        }
                        if (ptemp_2 && ptemp_2->m_item) {
                            pactor_2 = 0;
                            Actor* v14 = ptemp_2->m_item;
                            if (v14->IsActor()) {
                                pactor_2 = ptemp_2->m_item;
                            }
                            ptemp_2 = ptemp_2->m_pkNext;
                        }
                        else {
                            pactor_2 = 0;
                        }
                    }
                    else {
                        pactor_2 = 0;
                    }
                }
            }
        }
    }
}

bool ProcessLists::FlareHitEffect(TESObjectREFR* apEffectTarget, MagicSystem::FlareType aeFlareType) {
    if (MagicEffectList.IsEmpty()) {
        return 0;
    }
    MagicShaderHitEffect* pflareEffect = 0;
    float effectAge = 3.4028235e+38f;
    for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
        MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
        if (pshaderEffect) {
            if (pshaderEffect->GetType() == MAGIC_HIT_EFFECT_SHADER) {
                ActiveEffect* pae = pshaderEffect->pEffect;
                if (pae) {
                    if (static_cast<double>(effectAge) > pshaderEffect->BSTempEffect::fAge) {
                        if (pae->pTarget) {
                            MagicTarget* Target = pae->pTarget;
                            if (Target->GetTargetStatsObject() == apEffectTarget && Interface::PreloadWorldMap()) {
                                pflareEffect = pshaderEffect;
                                effectAge = pshaderEffect->BSTempEffect::fAge;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!pflareEffect) {
        return 0;
    }
    pflareEffect->Init();
    pflareEffect->ResetAlphaTimer();
    return 1;
}

void ProcessLists::HandleInvalidPositions() {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor() && !pMob->IsDead(false)) {
            pActor = static_cast<Actor*>(pMob);
        }
        if (pActor) {
            pActor->HandleInvalidPosition();
        }
    }
}

void ProcessLists::ParallelActorAnimationMovementUpdates() {
    uint32_t uiHead = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
    uint32_t uiTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
    uint32_t uiCount = uiTail - uiHead;
    bool bTasklet = bTaskletActorAnimMovementUpdates.Bool();
    if (!uiCount) {
        return;
    }

    alignas(BSScrapArray<Actor*>) uint8_t aArrayStorage[sizeof(BSScrapArray<Actor*>)];
    BSScrapArray<Actor*>& kArray = *::new (aArrayStorage) BSScrapArray<Actor*>;
    for (uint32_t i = uiHead; i < uiTail; ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        if (!pActor->Get3D() || pActor->GetDelete() || pActor->GetDisabled() || !pActor->GetParentCell() ||
            !pActor->GetParentCell()->GetAttached()) {
            continue;
        }
        if (!pActor->bProcessMe) {
            continue;
        }
        if (bTasklet) {
            kArray.Add(pActor);
        }
        else {
            pActor->UpdateAnimationMovementNoWorldUpdate();
        }
    }
    if (kArray.GetSize()) {
        BSParallelArrayTask kTask;
        FNVD_RECONSTRUCT(BSParallelArrayTask)(&kTask, &UpdateAnimMovement, &kArray, kArray.GetSize());
        pParallelTaskManager->RunTask(&kTask);
    }
    kArray.~BSScrapArray<Actor*>();
}

// Puts each high actor that stands idle into the furniture its package wants: the package target or location when
// it is furniture in the actor's space, else a free bed (sleep package) or chair (eat package, with food) from the
// bed and chair list, dropping the ones that are full. An actor whose furniture is full is moved onto the navmesh.
// When an actor was placed, the critical IO queue is loaded; the bed and chair list is cleared at the end.
void ProcessLists::PlaceActorsinBedsOrChairs() {
    BSSimpleList<TESObjectREFR*>* pList = TES::pInstance->GetBedChairList();
    TESObjectREFR* pRef = nullptr;
    TESBoundObject* pBase = nullptr;
    bool bPlaced = false;
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        if (!pActor || pActor->IsDead(false) || !pActor->IsNpc() || pActor->IsMoving() || pActor->IsTurning() ||
            pActor->GetSitSleepState() != SIT_SLEEP_NORMAL) {
            continue;
        }
        if (pActor->GetCurrentAIProcess()->GetProcessLevel() != PROCESS_TYPE_HIGH) {
            continue;
        }

        pActor->UpdateMagic(false, false);
        TESForm* pSpace = pActor->GetSpace();
        TESObjectREFR* pFurniture = nullptr;
        bool bSleep = false;
        bool bMoveToNavmesh = false;
        PackageLocation* pPackLoc = pActor->GetCurrentPackageLocation();
        TESObjectREFR* pTarget = pActor->GetCurrentPackageTarget();
        TESPackage* pPackage = pActor->GetCurrentPackage();
        if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_PATROL) {
            ActorPackageData* pData = pActor->GetCurrentAIProcess()->GetInstanceDataThatIsRunning();
            if (pData && pData->GetPackageType() == PACKAGE_TYPE_PATROL) {
                pTarget = static_cast<PatrolActorPackageData*>(pData)->GetCurrentReference();
            }
        }
        TESObjectREFR* pLocRef = nullptr;
        if (pPackLoc) {
            pLocRef = pPackLoc->GetLocReference();
        }
        if (pPackLoc && pPackLoc->GetLocType() == PLT_LINKEDREF) {
            pLocRef = pActor->GetLinkedRef();
        }
        if (!pLocRef) {
            pLocRef = pActor->GetCurrentAIProcess() ? pActor->GetCurrentAIProcess()->GetGenericLocation() : nullptr;
        }
        FurnitureMark* pMarker = pActor->GetCurrentAIProcess()->GetFurnitureMarker();
        int32_t iIndex = -1;

        if (pTarget && pTarget->IsFurniture() && pTarget->GetSpace() == pSpace) {
            pFurniture = pTarget;
            pBase = pFurniture->GetObjectReference();
            if (static_cast<TESFurniture*>(pBase)->CanSleepOn()) {
                bSleep = true;
            }
            iIndex = pFurniture->GetFirstFreeMarkerIndex(true);
            if (iIndex == -1 || !pFurniture->GetMarkerAtIndex(iIndex, *pMarker)) {
                pFurniture = nullptr;
                bMoveToNavmesh = true;
            }
        }
        else if (pLocRef && pLocRef->IsFurniture() && pLocRef->GetSpace() == pSpace) {
            pFurniture = pLocRef;
            pBase = pFurniture->GetObjectReference();
            if (static_cast<TESFurniture*>(pBase)->CanSleepOn()) {
                bSleep = true;
            }
            iIndex = pFurniture->GetFirstFreeMarkerIndex(true);
            if (iIndex == -1 || !pFurniture->GetMarkerAtIndex(iIndex, *pMarker)) {
                pFurniture = nullptr;
                bMoveToNavmesh = true;
            }
        }
        else if (
            pPackage && pPackage->GetPackType() == PACKAGE_TYPE_SLEEP &&
            pPackage->IsActorAtLocation(pActor, false, -1.0f, false)
        ) {
            if (!pActor->IsSleeping()) {
                pActor->GetCurrentAIProcess()->SetProcedureIndexRunning(1);
            }
            while (pList && !pList->IsEmpty() && !pFurniture) {
                pRef = pList->GetItem();
                bool bRemoved = false;
                if (pRef->GetSpace() == pSpace && (!pRef->GetOwner() || pRef->IsAnOwner(pActor, true))) {
                    pBase = pRef->GetObjectReference();
                    if (static_cast<TESFurniture*>(pBase)->CanSleepOn()) {
                        iIndex = pRef->GetFirstFreeMarkerIndex(true);
                        if (iIndex != -1 && pRef->GetMarkerAtIndex(iIndex, *pMarker)) {
                            pFurniture = pRef;
                            bSleep = true;
                        }
                        else {
                            pList->Remove(pRef);
                            pList = TES::pInstance->GetBedChairList();
                            bRemoved = true;
                        }
                    }
                }
                if (!bRemoved) {
                    pList = pList->GetNext();
                }
            }
        }
        else if (
            pPackage && pPackage->GetPackType() == PACKAGE_TYPE_EAT &&
            pPackage->IsActorAtLocation(pActor, false, -1.0f, false) && pActor->GetBestFoodItem()
        ) {
            if (!pActor->IsEating()) {
                pActor->GetCurrentAIProcess()->AddToProcedureIndexRunning(pActor, 1);
                pActor->SetActionComplete(true);
            }
            while (pList && !pList->IsEmpty() && !pFurniture) {
                pRef = pList->GetItem();
                bool bRemoved = false;
                if (pRef->GetSpace() == pSpace && pRef && (!pRef->GetOwner() || pRef->IsAnOwner(pActor, true))) {
                    pBase = pRef->GetObjectReference();
                    if (static_cast<TESFurniture*>(pBase)->CanSitOn()) {
                        iIndex = pRef->GetFirstFreeMarkerIndex(true);
                        if (iIndex != -1 && pRef->GetMarkerAtIndex(iIndex, *pMarker)) {
                            pFurniture = pRef;
                            bSleep = false;
                        }
                        else {
                            pList->Remove(pRef);
                            pList = TES::pInstance->GetBedChairList();
                            bRemoved = true;
                        }
                    }
                }
                if (!bRemoved) {
                    pList = pList->GetNext();
                }
            }
        }

        if (pFurniture && pActor->Get3D()) {
            bPlaced = pActor->PutActorInChairBedQuick(pFurniture, pMarker, iIndex, bSleep);
        }
        else if (bMoveToNavmesh) {
            const NiPoint3* pLocation = &pActor->GetLocationOnReference();
            NiPoint3 kPoint = *pLocation;
            TESObjectCELL* pCell = pActor->GetParentCell();
            TESWorldSpace* pWorld = pActor->GetWorldSpace();
            Pathing::FindClosestPointOnNavmesh(pWorld, pCell, *pLocation, &kPoint);
            pActor->SetLocationOnReference(kPoint);
            pActor->Update3DPosition();
        }
    }
    if (bPlaced) {
        pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_CRITICAL);
    }
    TES::pInstance->ClearBedChairList();
}

// The list walk keeps the previous node: a removal leaves the iterator on the head node, or moves it past the
// previous node when the removed item was further on.
void ProcessLists::PostProcessProjectiles() {
    if (ProjectilePostProcessList.IsEmpty()) {
        return;
    }
    BSSimpleList<void*>* pPrev = &ProjectilePostProcessList;
    BSSimpleList<void*>* pIter = pPrev;
    while (pIter && !pIter->IsEmpty()) {
        void* pItem = pIter->GetItem();
        bool bRemove = false;
        Projectile* pProjectile = dynamic_cast<Projectile*>(static_cast<MobileObject*>(pItem));
        if (pProjectile) {
            bRemove = pProjectile->PostProcess(0.0f);
        }
        if (bRemove) {
            ProjectilePostProcessList.Remove(pItem);
            if (pIter != pPrev) {
                pIter = pPrev->GetNext();
            }
        }
        else {
            pPrev = pIter;
            pIter = pIter->GetNext();
        }
    }
}

void ProcessLists::UpdateHighListImpacts(float afDelta) {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || pMob->IsStillLoading() || pMob->GetDelete() || pMob->GetDisabled()) {
            continue;
        }
        if (pMob->IsProjectile()) {
            static_cast<Projectile*>(pMob)->ProcessImpacts();
        }
        else if (pMob->IsExplosion()) {
            static_cast<Explosion*>(pMob)->ApplyImpactDecals();
        }
    }
}

void ProcessLists::UpdateAnimationScenegraph() {
    for (uint32_t uiIndex = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH); ++uiIndex) {
        Actor* v6 = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        if (v6) {
            if (v6->IsActor()) {
                Animation* v5 = v6->GetAnimation();
                bool updated = 0;
                if (v5) {
                    updated = v5->UpdateQueuedScenegraph();
                }
                if (v6->QUpdateLighting()) {
                    if (updated) {
                        NiNode* v2 = v6->Get3D();
                        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
                        ShadowSceneNode_->UpdateObjectLighting(v2, 0);
                        v6->SetUpdateLighting(0);
                    }
                }
            }
        }
    }
}

// Updates the animation scene graphs of the high and middle-high actors: with the setting on, the animations are
// collected and the worker threads update them; otherwise each is updated here. An actor whose lighting waits on
// the update gets it redone, unless the serial update did not run; with the workers it is redone before they run.
void ProcessLists::ParallelUpdateAnimationScenegraph() {
    uint32_t uiHead = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
    uint32_t uiTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
    uint32_t uiCount = uiTail - uiHead;
    bool bTasklet = bTaskletActorSceneGraphUpdates.Bool();
    if (!uiCount) {
        return;
    }

    alignas(BSScrapArray<Animation*>) uint8_t aArrayStorage[sizeof(BSScrapArray<Animation*>)];
    BSScrapArray<Animation*>& kArray = *::new (aArrayStorage) BSScrapArray<Animation*>;
    for (uint32_t i = uiHead; i < uiTail; ++i) {
        MobileObject* pObject = MobProcessArray.GetItem(i);
        if (!pObject || !pObject->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pObject);
        Animation* pAnimData = pActor->GetAnimation();
        if (!pAnimData) {
            continue;
        }
        bool bUpdated = true;
        if (bTasklet) {
            kArray.Add(pAnimData);
        }
        else {
            bUpdated = pAnimData->UpdateQueuedScenegraph();
        }
        if (pActor->QUpdateLighting() && bUpdated) {
            // Get3D is called before the shadow scene node is fetched.
            NiNode* p3D = pActor->Get3D();
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateObjectLighting(p3D, false);
            pActor->SetUpdateLighting(false);
        }
    }
    if (kArray.GetSize()) {
        BSParallelArrayTask kTask;
        FNVD_RECONSTRUCT(BSParallelArrayTask)(&kTask, &UpdateAnimSceneGraph, &kArray, kArray.GetSize());
        pParallelTaskManager->RunTask(&kTask);
    }
    kArray.~BSScrapArray<Animation*>();
}

void ProcessLists::UpdateNonRenderSafeStuff(float afDelta) {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor() && !pMob->IsStillLoading()) {
            Actor* pActor = static_cast<Actor*>(pMob);
            pActor->HandlePostAnimationActions();
            pActor->GetCurrentAIProcess()->CheckForExpiredDetectionEvent();
            if (pActor->pPenetrationDetection->QInitialOnNextUpdate()) {
                // The group is read before Get3D.
                CFilter kFilter;
                pActor->GetCollisionFilter(&kFilter.iFilter);
                int32_t iGroup = static_cast<int32_t>(kFilter.GetGroup());
                NiNode* p3D = pActor->Get3D();
                pActor->pPenetrationDetection->Initialize(p3D, iGroup);
            }
            pActor->pPenetrationDetection->Update();
            if (!pActor->IsDead(false)) {
                TESHavokUtilities::ConsolidateSimIslands(pActor);
            }
            pActor->UpdateNonRenderSafeMovement(afDelta, pPlayer->IsSleepingorResting());
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            if (pProcess) {
                pProcess->CalculateLightValue(pActor, false);
                pProcess->ResolveEquipmentQueue(pActor);
                pProcess->UpdateReparentWeapon(pActor);
            }
        }
        else if (pMob && pMob->IsExplosion()) {
            static_cast<Explosion*>(pMob)->UpdateNonRenderSafeMovement(afDelta);
        }
    }
    UpdateReferenceMuzzleFlashes(afDelta);
    if (pPlayer->GetCurrentAIProcess()) {
        pPlayer->GetCurrentAIProcess()->ResolveEquipmentQueue(pPlayer);
    }
}

void ProcessLists::RemoveDetectionEventsOnActors() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor()) {
            pMob->GetCurrentAIProcess()->RemoveDetectionEvent();
        }
    }
}

// Loads the dynamic idle of dead actors whose base form still has health, and knocks down the living ones that
// are out of fatigue.
void ProcessLists::AnimateDeadActors() {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pObject = MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pObject && pObject->IsActor()) {
            pActor = static_cast<Actor*>(pObject);
        }
        if (pActor && pActor->IsDead(false)) {
            // GetHealth returns an unsigned value, but the health test is signed.
            if (static_cast<int32_t>(pActor->GetActorBase()->GetHealth()) > 0 && !pActor->GetHavokDeath()) {
                pActor->GetCurrentAIProcess()->LoadDynamicIdle(pActor);
            }
        }
        else if (pActor) {
            if (pActor->GetFatiguePercentage() <= 0.0) {
                // The actor base is converted to its TESActorBaseData without a null test, hence the reference.
                TESActorBaseData& rBaseData = *pActor->GetActorBase();
                if (rBaseData.GetFatigue() > 0) {
                    BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                    if (pProcess) {
                        pProcess->KnockExplosion(pActor, pActor->GetLocationOnReference(), 0.0f);
                    }
                }
            }
        }
    }
}

// Stamps the game date for the low processes, then sets up the package locations of the low list actors that are
// alive; the others are marked dead when they have a base object and leave the low list.
void ProcessLists::InitPackageLocations() {
    fGameHour = appCalendar.GetHour();
    iGameYear = appCalendar.GetYear();
    LowProcess::eCurrentGameMonth = appCalendar.GetMonth();
    LowProcess::cCurrentGameDay = appCalendar.GetDay();
    MobileObject* pMob = nullptr;
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        Actor* pActor = nullptr;
        pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        if (!pActor) {
            continue;
        }
        Script::InitActionList(pActor, pActor->GetExtra());
        if (!pActor->GetDelete() && pActor->GetObjectReference() && pActor->GetActorValue(ActorValue::HEALTH) > 0) {
            pActor->InitPackageLocations(false);
        }
        else {
            if (!pActor->GetDelete() && pActor->GetObjectReference() &&
                pActor->GetActorValue(ActorValue::HEALTH) <= 0) {
                pActor->SetLifeState(ACTOR_LIFE_STATE_DEAD);
            }
            if (pActor->GetCurrentAIProcess()) {
                MobProcessArray.RemoveActor(pMob, PROCESS_TYPE_LOW);
            }
        }
    }
}

void ProcessLists::UpdateProcessLists() {
    AutoMemContext apMemory(
        MEM_CONTEXT_PROCESSES, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\ProcessLists.cpp", 1313
    );
    if (fPlayerActionCommentTimer >= 0.0) {
        fPlayerActionCommentTimer = fPlayerActionCommentTimer - appTimer.GetSeconds();
    }
    if (fPlayerKnockObjectCommentTimer >= 0.0) {
        fPlayerKnockObjectCommentTimer = fPlayerKnockObjectCommentTimer - appTimer.GetSeconds();
    }
    fGameHour = appCalendar.GetHour();
    iGameYear = appCalendar.GetYear();
    LowProcess::eCurrentGameMonth = appCalendar.GetMonth();
    LowProcess::cCurrentGameDay = appCalendar.GetDay();
    FollowerBarks::ProcessFollowerBarks();
    TESObjectREFR* afDelta = pPlayer->GetRecentAction();
    PLAYER_ACTION LastAction = static_cast<PLAYER_ACTION>(pPlayer->GetLastAction());
    CommentOnPlayerAction(LastAction, afDelta, 0);
    BSDoorHavokController::ReenableDoors();
    if (iNumHWThreads.UInt() == 1) {
        BGSDestructibleObjectForm::UpdateDestructibleObjects();
    }
    float Seconds = appTimer.GetSeconds();
    float v3 = GetSystemTimeClock() + Seconds;
    SetSystemTimeClock(v3);
    int32_t v8 = static_cast<int32_t>(1.0 / Seconds);
    bool v7 = 1;
    iNumberHighActors = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
    fSecondsPassedNoProcess = fSecondsPassedNoProcess + Seconds;
    if (v8 < 50) {
        if (v8 >= 10) {
            if (!(fSecondsPassedNoProcess < 1.0)) {
                fSecondsPassedNoProcess = 0.0;
            }
            else {
                fSecondsPassedNoProcess = (1.0 - Seconds) / 2.0 + fSecondsPassedNoProcess;
                v7 = 0;
            }
        }
        else {
            if (!(fSecondsPassedNoProcess < 1.0)) {
                fSecondsPassedNoProcess = 0.0;
            }
            else {
                v7 = 0;
                fSecondsPassedNoProcess = fSecondsPassedNoProcess + Seconds;
            }
        }
    }
    if (v7) {
        if (bProcessLow && (!Main::GetSceneIsComplex() && ReferenceProcessLists.kUpdateLock.TryLock())) {
            UpdateLowList(0.0, 0);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        if (bProcessMLow && !Main::GetSceneIsComplex()) {
            ReferenceProcessLists.kUpdateLock.Lock(0);
            UpdateMiddleLowList(0.0, 0);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        if (bProcessMHigh) {
            ReferenceProcessLists.kUpdateLock.Lock(0);
            UpdateMiddleHighList(0.0, 0);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        if (!(fCrimeUpdateTimer <= 0.0)) {
            fCrimeUpdateTimer = fCrimeUpdateTimer - appTimer.GetSeconds();
        }
        else {
            UpdateTheCrimeList();
        }
    }
}

void ProcessLists::ChangeProcessLevel(BSSimpleList<TESObjectREFR*>* apList, bool abChangeLevel) {
    BSSimpleList<TESObjectREFR*>* pHead = apList;
    while (apList && apList->m_item) {
        MobileObject* pRef = 0;
        TESObjectREFR* m_item = apList->m_item;
        if (m_item->IsMobileObject()) {
            pRef = static_cast<MobileObject*>(apList->m_item);
        }
        apList = apList->m_pkNext;
        if (pRef) {
            if (abChangeLevel) {
                if (pRef->GetDelete() || pRef->GetDisabled()) {
                    if (pRef->pCurrentProcess) {
                        if (ReferenceProcessLists.kUpdateLock.TryLock()) {
                            PROCESS_TYPE CurrentProcessType = pRef->GetCurrentProcessType();
                            MobProcessArray.RemoveActor(pRef, CurrentProcessType);
                            pRef->DeleteCurrentProcess();
                            ReferenceProcessLists.kUpdateLock.Unlock();
                        }
                    }
                }
                else {
                    if (pRef != pPlayer) {
                        if (pRef->pCurrentProcess) {
                            if (!pRef->ChangeProcessLevel()) {
                                apList = pHead;
                            }
                        }
                    }
                }
            }
            else {
                AddActorToTempChangeList(pRef);
            }
        }
    }
}

void ProcessLists::ProcessActors(BSSimpleList<TESObjectREFR*>* apList) {
    BSSimpleList<TESObjectREFR*>* pActList;
    BSSimpleList<TESObjectREFR*>* v8 =
        static_cast<BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
    if (v8) {
        pActList = ::new (v8) BSSimpleList<TESObjectREFR*>;
    }
    else {
        pActList = 0;
    }
    while (apList && apList->m_item) {
        TESObjectREFR* m_item = apList->m_item;
        if (m_item->IsActor()) {
            pActList->AddHead(apList->m_item);
        }
        apList = apList->m_pkNext;
    }
    while (pActList && pActList->m_item) {
        Actor* pRef = static_cast<Actor*>(pActList->m_item);
        if (pRef) {
            if (pRef != pPlayer) {
                if (pRef->pCurrentProcess) {
                    if (!pRef->GetDelete() && !pRef->GetDisabled() && !pRef->IsInCombat()) {
                        BaseProcess* v3;
                        float flastprocesstime = pRef->pCurrentProcess->fHourLastProcessed;
                        float fhour = appCalendar.GetHour();
                        float fdelta = fhour - flastprocesstime;
                        if (flastprocesstime > static_cast<double>(fhour)) {
                            fdelta = fhour + 24.0 - flastprocesstime;
                        }
                        bool isescorting = 0;
                        if (fdelta < 0.05) {
                            fdelta = 0.0f;
                        }
                        BaseProcess* CurrentAIProcess = pRef->pCurrentProcess;
                        if (CurrentAIProcess->GetCurrentPackage()) {
                            BaseProcess* v4 = pRef->pCurrentProcess;
                            TESPackage* v2 = v4->GetCurrentPackage();
                            if (v2->GetPackType() == PACKAGE_TYPE_ESCORT) {
                                isescorting = 1;
                            }
                        }
                        if ((!isescorting && !pRef->IsFollowing()) ||
                            (v3 = pRef->pCurrentProcess, v3->GetTarget() != pPlayer)) {
                            pRef->Process(fdelta);
                        }
                    }
                }
            }
        }
        pActList->RemoveHead();
    }
    pActList->RemoveAll();
    if (pActList) {
        delete pActList;
    }
}

// Moves the mobs queued on the temp change list to their new process level, unless the player sleeps. Dead actors whose
// respawn time passed in an unloaded cell are resurrected, disabled and deleted mobs lose their process, and dialogue
// activators move to the high list while they talk. A handled mob leaves the list and the walk starts over; whatever is
// left is flagged to change process later. A list item that is no mobile object stalls the walk, deliberately, as the
// game does. The lock and AddReference go through the engine's ReferenceProcessLists, not this.
void ProcessLists::ChangeProcessLevelTempList() {
    if (pPlayer->IsSleepingorResting()) {
        return;
    }
    BSSimpleList<void*>* pHead = &TempShouldMoveList;
    BSSimpleList<void*>* pIter = pHead;
    while (pIter && pIter->GetItem()) {
        pCPLRef = nullptr;
        if (static_cast<TESForm*>(pIter->GetItem())->IsMobileObject()) {
            pCPLRef = static_cast<MobileObject*>(pIter->GetItem());
        }
        if (!pCPLRef) {
            continue;
        }
        Actor* pActor = nullptr;
        if (pCPLRef->IsActor()) {
            pActor = static_cast<Actor*>(pCPLRef);
        }
        pIter = pIter->GetNext();
        if (pCPLRef->IsStillLoading()) {
            continue;
        }

        if ((!pCPLRef->IsActor() || pCPLRef->GetCurrentAIProcess()) &&
            (!pCPLRef->IsTalkingActivator() || GetDialogMenuParterRef() == pCPLRef)) {
            // The death time must not be equal to 0 (NaN goes on); the hours are converted as unsigned. The hours
            // passed must be strictly greater.
            bool bRespawn = false;
            if (pActor && pActor->IsDead(false) && !pActor->GetEssential() && pActor->GetRespawn() &&
                !TES::pInstance->IsCellLoaded(pActor->GetParentCell(), false) &&
                pActor->GetCurrentAIProcess()->GetDeathTime() != 0.0) {
                double fDeathTime = pActor->GetCurrentAIProcess()->GetDeathTime();
                double fClearTime = static_cast<double>(TESObjectCELL::GetHoursToRespawnCell()) + fDeathTime;
                bRespawn = static_cast<double>(appCalendar.GetHoursPassed()) > fClearTime;
            }
            if (bRespawn) {
                BGSEncounterZone* pZone = pActor->GetEncounterZone();
                if (pZone && pZone->QNeverReset()) {
                    continue;
                }
                pActor->Resurrect(true, false, false);
                BSSimpleList<TESObjectREFR*>* pDroppedList = pActor->GetExtra()->GetDroppedItemList();
                if (pDroppedList) {
                    while (pDroppedList && !pDroppedList->IsEmpty()) {
                        TESObjectREFR* pItem = pDroppedList->GetItem();
                        pItem->GetExtra()->SetItemDropper(nullptr);
                        if (!pItem->IsQuestObject()) {
                            pItem->MarkAsDeleted();
                        }
                        pDroppedList->RemoveHead();
                    }
                    pActor->GetExtra()->RemoveDroppedItemList();
                }
                void* pRemove = pCPLRef;
                TempShouldMoveList.Remove(pRemove);
                if (pCPLRef) {
                    pCPLRef->bInTempChangeList = false;
                }
                pIter = pHead;
            }
            else if (pCPLRef->GetDisabled() && ReferenceProcessLists.kUpdateLock.TryLock()) {
                if (pCPLRef->GetCurrentAIProcess()) {
                    MobProcessArray.RemoveActor(pCPLRef, pCPLRef->GetCurrentProcessType());
                }
                pCPLRef->DeleteCurrentProcess();
                void* pRemove = pCPLRef;
                TempShouldMoveList.Remove(pRemove);
                if (pCPLRef) {
                    pCPLRef->bInTempChangeList = false;
                }
                pIter = pHead;
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
            else if (
                (pCPLRef->IsTalkingActivator() && Interface::GetMenuModeType() != Interface::eDialogMenu) ||
                ((pCPLRef->GetFormFlags() & 0x20000) && !pCPLRef->IsTalkingActivator())
            ) {
                pCPLRef->SetNeedToChangeProcess(false);
                void* pRemove = pCPLRef;
                TempShouldMoveList.Remove(pRemove);
                if (pCPLRef) {
                    pCPLRef->bInTempChangeList = false;
                }
                // The projectile is kept in a local that is never read, as the game does.
                Projectile* pProjectile = nullptr;
                if (pCPLRef->IsProjectile()) {
                    pProjectile = static_cast<Projectile*>(pCPLRef);
                }
                (void)pProjectile;
                if (pCPLRef->GetDelete() && !pCPLRef->GetFile(-1)) {
                    if (pCPLRef->GetCurrentAIProcess()) {
                        MobProcessArray.RemoveActor(pCPLRef, pCPLRef->GetCurrentProcessType());
                    }
                    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                    delete pCPLRef;
                    ReferenceProcessLists.kUpdateLock.Unlock();
                }
                else if (pCPLRef->GetDelete() || pCPLRef->GetDisabled()) {
                    if (pCPLRef->GetCurrentAIProcess() && ReferenceProcessLists.kUpdateLock.TryLock()) {
                        MobProcessArray.RemoveActor(pCPLRef, pCPLRef->GetCurrentProcessType());
                        pCPLRef->DeleteCurrentProcess();
                        ReferenceProcessLists.kUpdateLock.Unlock();
                    }
                }
                else if (pCPLRef->GetCurrentAIProcess() && pCPLRef->GetCurrentAIProcess()->IsAFollower()) {
                    TESPackage* pPackage = pCPLRef->GetCurrentAIProcess()->GetCurrentPackage();
                    if (!pPackage ||
                        (pPackage->GetPackType() != PACKAGE_TYPE_FOLLOW &&
                         pPackage->GetPackType() != PACKAGE_TYPE_ESCORT)) {
                        pCPLRef->GetCurrentAIProcess()->SetFollower(false);
                        MobProcessArray.AddActor(pCPLRef, pCPLRef->GetCurrentAIProcess()->GetProcessLevel());
                    }
                }
                else {
                    pCPLRef->ChangeProcessLevel();
                    if (pCPLRef && (pCPLRef->GetFormFlags() & 0x20000)) {
                        pCPLRef->SetNeedToChangeProcess(false);
                    }
                }
                pIter = pHead;
            }
        }
        else {
            if (pCPLRef->IsTalkingActivator() && pCPLRef->IsTalking()) {
                ReferenceProcessLists.AddReference(pCPLRef, PROCESS_TYPE_HIGH, 0, false, nullptr);
            }
            else if (
                (!pCPLRef->GetDelete() || pCPLRef->GetFile(-1)) &&
                (!pCPLRef->IsTalkingActivator() || pCPLRef->IsTalking())
            ) {
                if ((pCPLRef->GetDelete() || pCPLRef->GetDisabled()) && pCPLRef->GetCurrentAIProcess() &&
                    ReferenceProcessLists.kUpdateLock.TryLock()) {
                    MobProcessArray.RemoveActor(pCPLRef, pCPLRef->GetCurrentProcessType());
                    pCPLRef->DeleteCurrentProcess();
                    ReferenceProcessLists.kUpdateLock.Unlock();
                }
            }
            else {
                if (pCPLRef->GetCurrentAIProcess()) {
                    MobProcessArray.RemoveActor(pCPLRef, pCPLRef->GetCurrentProcessType());
                }
                ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                delete pCPLRef;
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
            void* pRemove = pCPLRef;
            TempShouldMoveList.Remove(pRemove);
            if (pCPLRef) {
                pCPLRef->bInTempChangeList = false;
            }
            pIter = pHead;
        }
    }

    for (pIter = pHead; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        static_cast<MobileObject*>(pIter->GetItem())->SetNeedToChangeProcess(true);
    }
}

// Refreshes the magic shaders of the high list actors that ask for it, then updates both temp effect lists and frees
// the finished effects. The list walks are the iterator UpdateReferenceMuzzleFlashes uses. IsActor is called on a null
// mob too, as the game does.
void ProcessLists::UpdateTempEffects(float afDeltaTime) {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        BaseProcess* pProcess = pMob ? pMob->GetCurrentAIProcess() : nullptr;
        if (pMob->IsActor() && pProcess && pProcess->GetShouldRefreshMagicShadersEffects()) {
            pProcess->RefreshMagicShaderEffects(static_cast<Actor*>(pMob));
        }
    }

    if (!GlobalTempEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pIter = &GlobalTempEffectList;
        bool bFirst = true;
        while (pIter) {
            NiPointer<BSTempEffect>& rspItem = pIter->GetItem();
            BSTempEffect* pEffect = rspItem.operator->();
            if (!bFirst) {
                pIter = pIter->GetNext();
            }
            if (pEffect && !pEffect->Update(afDeltaTime)) {
                pEffect->Detach();
                GarbageCollector::Add(pEffect);
                NiPointer<BSTempEffect> spEffect(pEffect);
                GlobalTempEffectList.Remove(spEffect);
            }
            else if (bFirst) {
                pIter = pIter->GetNext();
                bFirst = false;
            }
        }
    }

    if (!MagicEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pIter = &MagicEffectList;
        bool bFirst = true;
        while (pIter) {
            NiPointer<BSTempEffect>& rspItem = pIter->GetItem();
            BSTempEffect* pEffect = rspItem.operator->();
            if (!bFirst) {
                pIter = pIter->GetNext();
            }
            // the magic effects are not detached
            if (pEffect && !pEffect->Update(afDeltaTime)) {
                GarbageCollector::Add(pEffect);
                NiPointer<BSTempEffect> spEffect(pEffect);
                MagicEffectList.Remove(spEffect);
            }
            else if (bFirst) {
                pIter = pIter->GetNext();
                bFirst = false;
            }
        }
    }
}

// Fills a scrap array with every temp effect, lets the worker threads update them, then frees the finished ones;
// with the setting off, the serial UpdateTempEffects runs instead.
void ProcessLists::UpdateTempEffectsParallel(float afDeltaTime) {
    if (!bTaskletTempEffectsUpdate.Bool()) {
        UpdateTempEffects(afDeltaTime);
        return;
    }
    int32_t iCount;
    iCount = GlobalTempEffectList.ItemsInList();
    iCount += MagicEffectList.ItemsInList();
    if (!iCount) {
        return;
    }

    alignas(BSScrapArray<EffectTaskData>) uint8_t aArrayStorage[sizeof(BSScrapArray<EffectTaskData>)];
    BSScrapArray<EffectTaskData>& kTaskArray = *::new (aArrayStorage) BSScrapArray<EffectTaskData>;
    for (BSSimpleList<NiPointer<BSTempEffect>>* pIter = &GlobalTempEffectList; pIter; pIter = pIter->GetNext()) {
        NiPointer<BSTempEffect>& rspItem = pIter->GetItem();
        BSTempEffect* pEffect = rspItem.operator->();
        if (pEffect) {
            uint32_t uiIndex = kTaskArray.AddUninitialized();
            kTaskArray.GetAt(uiIndex).pTempEffect = pEffect;
            kTaskArray.GetAt(uiIndex).fDeltaTime = afDeltaTime;
            kTaskArray.GetAt(uiIndex).bGlobal = true;
            kTaskArray.GetAt(uiIndex).bRemove = false;
        }
    }
    for (BSSimpleList<NiPointer<BSTempEffect>>* pIter = &MagicEffectList; pIter; pIter = pIter->GetNext()) {
        NiPointer<BSTempEffect>& rspItem = pIter->GetItem();
        BSTempEffect* pEffect = rspItem.operator->();
        if (pEffect) {
            uint32_t uiIndex = kTaskArray.AddUninitialized();
            kTaskArray.GetAt(uiIndex).pTempEffect = pEffect;
            kTaskArray.GetAt(uiIndex).fDeltaTime = afDeltaTime;
            kTaskArray.GetAt(uiIndex).bGlobal = false;
            kTaskArray.GetAt(uiIndex).bRemove = false;
        }
    }

    if (kTaskArray.GetSize()) {
        BSParallelArrayTask kTask;
        FNVD_RECONSTRUCT(BSParallelArrayTask)(&kTask, &UpdateTempEffect, &kTaskArray, kTaskArray.GetSize());
        pParallelTaskManager->RunTask(&kTask);
        for (uint32_t i = 0; i < kTaskArray.GetSize(); ++i) {
            EffectTaskData& rData = kTaskArray[i];
            if (!rData.bRemove) {
                continue;
            }
            GarbageCollector::Add(rData.pTempEffect);
            if (rData.bGlobal) {
                NiPointer<BSTempEffect> spEffect(rData.pTempEffect);
                GlobalTempEffectList.Remove(spEffect);
            }
            else {
                NiPointer<BSTempEffect> spEffect(rData.pTempEffect);
                MagicEffectList.Remove(spEffect);
            }
        }
    }
    kTaskArray.~BSScrapArray<EffectTaskData>();
}

ProcessArray* ProcessLists::GetProcessArray() {
    return &MobProcessArray;
}

void ProcessLists::FinishMagicHitEffect(TESObjectREFR* apEffectTarget) {
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicHitEffect* phitEffect = static_cast<MagicHitEffect*>(pelist->m_item.m_pObject);
            if (phitEffect) {
                if (NiObject::IsKindOf_0(&MagicHitEffect::ms_RTTI, phitEffect) &&
                    phitEffect->pTarget == apEffectTarget) {
                    phitEffect->ClearTarget();
                    phitEffect->bFinished = 1;
                }
            }
        }
    }
}

uint32_t ProcessLists::GetNumberOfActorsInHigh() const {
    return iNumberHighActors;
}

BSSimpleList<Actor*>* ProcessLists::GetAliveActorList() {
    return &AliveActorList;
}

void ProcessLists::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    fPlayerActionCommentTimer = 0.0f;
    fPlayerKnockObjectCommentTimer = 0.0f;
    iCrimeNumber = 0;
    for (uint32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* pGlobalCrimeList;
        for (pGlobalCrimeList = GlobalCrimeListArray[i]; pGlobalCrimeList && pGlobalCrimeList->m_item;
             pGlobalCrimeList->RemoveHead()) {
            Crime* pcrime = pGlobalCrimeList->m_item;
            if (pcrime) {
                delete pcrime;
            }
        }
        if (pGlobalCrimeList) {
            delete pGlobalCrimeList;
        }
        GlobalCrimeListArray[i] = 0;
    }
}

void ProcessLists::RevertTempEffects(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (BSSimpleList<NiPointer<BSTempEffect>>* plist = &GlobalTempEffectList; plist && !plist->IsEmpty();
         plist->RemoveHead()) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect) {
            ptempEffect->Detach();
        }
    }
    for (BSSimpleList<NiPointer<BSTempEffect>>* plist = &MagicEffectList; plist && !plist->IsEmpty();
         plist->RemoveHead()) {}
    BSTempEffect::RemoveAllDecals();
    BGSDecalNode::RemoveAllDecals();
    BGSDecalManager::GetSingleton();
    BGSDecalManager::RemoveAllDecals();
}

bool ProcessLists::AreHostilActorsNear(bool abInterior) {
    float fdistance = fHostileActorExteriorDistance.Float();
    bool bflag = 0;
    if (abInterior) {
        fdistance = fHostileActorInteriorDistance.Float();
    }
    float v13 = 0.0f;
    TESObjectREFR* v10 = 0;
    for (uint32_t i = 0; !bflag; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = 0;
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        if (pactor) {
            if (pactor != pPlayer && !pactor->IsDead(0) && !pactor->GetDisabled()) {
                if (pactor->pCurrentProcess) {
                    BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                    if (CurrentAIProcess->GetCommandingActor()) {
                        BaseProcess* v4 = pactor->pCurrentProcess;
                        pactor = v4->GetCommandingActor();
                    }
                }
                float fdist = pPlayer->GetDistanceFromReference(pactor, 0, 0);
                if (fdistance >= static_cast<double>(fdist)) {
                    pactor->GetDisposition(pPlayer, 0);
                    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                    if (pactor->IsInCombatWithActor(pPlayer)) {
                        bflag = 1;
                    }
                    else {
                        if (pactor->GetShouldAttackActor(pPlayer, 0, ereaction, 0)) {
                            bflag = 1;
                        }
                    }
                }
            }
        }
    }
    return bflag;
}

bool ProcessLists::IsPlayerInRadiationArea() {
    return (bool)((uint8_t)(bPlayerInRadiatedWater || bPlayerInRadiationArea));
}

void ProcessLists::ResetPlayerActionsCommentTimer() {
    fPlayerActionCommentTimer = (float)(fPlayerActionCommentTimer + (-0.5));
}

void ProcessLists::SetSystemTimeClock(float afvalue) {
    ProcessLists::fSystemTimeClock = afvalue;
    if (afvalue > 100000.0) {
        ProcessLists::fSystemTimeClock = (float)0.0;
    }
    if (std::isnan(ProcessLists::fSystemTimeClock) || (!std::isfinite(ProcessLists::fSystemTimeClock))) {
        ProcessLists::fSystemTimeClock = (float)0.0;
    }
}

int32_t ProcessLists::GetCrimeNumber() {
    uint32_t iCrimeNumber;
    iCrimeNumber = this->iCrimeNumber;
    this->iCrimeNumber = iCrimeNumber + 1;
    return (int32_t)iCrimeNumber;
}

void ProcessLists::RemoveTempEffectsForCell(TESObjectCELL* apCell) {
    if (!GlobalTempEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pelist = &GlobalTempEffectList;
        BSSimpleList<NiPointer<BSTempEffect>>* plastlist = 0;
        while (pelist && !pelist->IsEmpty()) {
            BSTempEffect* peffect = pelist->m_item.m_pObject;
            if (peffect->pCell == apCell) {
                peffect->pCell = 0;
                GarbageCollector::Add(peffect);
                if (plastlist) {
                    {
                        NiPointer<BSTempEffect> spEffect(peffect);
                        plastlist->Remove(spEffect);
                    }
                    pelist = plastlist->m_pkNext;
                }
                else {
                    pelist->RemoveHead();
                }
            }
            else {
                plastlist = pelist;
                pelist = pelist->m_pkNext;
            }
        }
    }
    if (!MagicEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pelist_2 = &MagicEffectList;
        BSSimpleList<NiPointer<BSTempEffect>>* plastlist_2 = 0;
        while (pelist_2 && !pelist_2->IsEmpty()) {
            BSTempEffect* peffect_2 = pelist_2->m_item.m_pObject;
            if (peffect_2->pCell == apCell) {
                peffect_2->pCell = 0;
                GarbageCollector::Add(peffect_2);
                if (plastlist_2) {
                    {
                        NiPointer<BSTempEffect> spEffect(peffect_2);
                        plastlist_2->Remove(spEffect);
                    }
                    pelist_2 = plastlist_2->m_pkNext;
                }
                else {
                    pelist_2->RemoveHead();
                }
            }
            else {
                plastlist_2 = pelist_2;
                pelist_2 = pelist_2->m_pkNext;
            }
        }
    }
}

BSSimpleList<TESObjectREFR*>* ProcessLists::GetActorTargetinganObject(uint32_t aId, Actor* apActorPickup) {
    BSSimpleList<TESObjectREFR*>* preflist_2 = 0;
    Actor* pActor = 0;
    Actor* pMobileObject = 0;
    TESObjectREFR* pref = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(aId));
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        uint32_t Size = MobProcessArray.GetSize();
        if (uiIndex >= Size) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(uiIndex);
        Actor* pactor = 0;
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        if (pactor) {
            BaseProcess* pproc = pactor->pCurrentProcess;
            if (pproc) {
                pproc->ClearObjectFromAcquireList(aId);
                if (pref) {
                    pproc->RemoveHeadTrackTarget(pref);
                }
            }
            if (pactor->GetCurrentTarget()) {
                TESObjectREFR* v4 = pactor->GetCurrentTarget();
                if (v4->iFormID == aId && apActorPickup != pactor) {
                    if (!preflist_2) {
                        BSSimpleList<TESObjectREFR*>* preflist;
                        BSSimpleList<TESObjectREFR*>* v9 =
                            static_cast<BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
                        if (v9) {
                            preflist = ::new (v9) BSSimpleList<TESObjectREFR*>;
                        }
                        else {
                            preflist = 0;
                        }
                        BSSimpleList<TESObjectREFR*>* v10 = preflist;
                        preflist_2 = preflist;
                    }
                    TESObjectREFR* a2 = pactor;
                    preflist_2->AddHead(a2);
                }
            }
        }
    }
    return preflist_2;
}

Actor* ProcessLists::FindOwnerSellsObject(TESObjectREFR* pRef) {
    Actor* v7 = 0;
    for (uint32_t uiIndex = 0; !v7 && uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        Actor* pObject = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        Actor* pActor = 0;
        if (pObject) {
            if (pObject->IsActor()) {
                pActor = pObject;
            }
        }
        while (pActor) {
            if (pRef->IsAnOwner(pActor, 1)) {
                TESObject* ObjectReference = pRef->GetObjectReference();
                if (pActor->GetActorBase()->SellBuysItem(ObjectReference)) {
                    v7 = pActor;
                }
            }
            pActor = 0;
        }
    }
    return v7;
}

void ProcessLists::RemoveAllCrimesforAnActor(Actor* apActor) {
    BSSimpleList<Crime*>* pCrimes = 0;
    void* ecx0 = 0;
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[i];
        BSSimpleList<Crime*>* pitem = pGlobalCrimeList;
        while (pitem && pitem->m_item) {
            Crime* pcrime = pitem->m_item;
            if (pcrime->pcriminal == apActor) {
                for (BSSimpleList<TESFaction*>* pfactlist = pcrime->pListofEnemyFactions;
                     pfactlist && !pfactlist->IsEmpty(); pfactlist = pfactlist->m_pkNext) {
                    TESFaction* pfac = pfactlist->m_item;
                    if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, pcrime)) {
                        pfac->SetPCAnEnemy(0);
                    }
                }
                pGlobalCrimeList->Remove(pcrime);
                pitem = pGlobalCrimeList;
            }
            else {
                pitem = pitem->m_pkNext;
            }
        }
    }
}

// The bound deliberately lets crime type 5 through, one past GlobalCrimeListArray's five entries, as the game does: its
// read there is the word after the array, GlobalTempEffectList's head item, which this index reads alike.
void ProcessLists::RemoveCrimeGlobalCrimeList(Crime* apCrime) {
    if (apCrime) {
        if (apCrime->ecrime > PackageNS::CRIME_TYPE_NONE && apCrime->ecrime <= PackageNS::CRIME_TYPE_COUNT) {
            BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[apCrime->ecrime];
            if (pGlobalCrimeList) {
                pGlobalCrimeList->Remove(apCrime);
            }
        }
    }
}

void ProcessLists::UpdateRadiationList() {
    NiTMapItem<uint32_t, TESObjectREFR*>* iter;
    bool bclearRadiation = 0;
    float fheaviestPlayerRadiation = 0.0f;
    int32_t pheaviestRadiation = 0;
    LoadedReferenceMap* pmap = &pLoadedReferenceCollection->RadiationMap;
    if (pmap) {
        iter = pmap->GetFirstPos();
        if (iter) {
            bRadiationExists = 1;
        }
        else {
            if (bRadiationExists) {
                bRadiationExists = 0;
                bclearRadiation = 1;
            }
        }
        while (iter) {
            ExtraDataList* pextra;
            uint32_t referenceID = 0;
            TESObjectREFR* preference = 0;
            pmap->GetNext(iter, referenceID, preference);
            if (preference) {
                pextra = &preference->m_Extra;
            }
            else {
                pextra = 0;
            }
            ExtraDataList* pextra_2 = pextra;
            if (pextra) {
                float fradius = pextra_2->GetRadius();
                float fradiation = pextra_2->GetRadiation();
                for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
                    MobileObject* pMob = MobProcessArray.GetItem(i);
                    Actor* pactor = 0;
                    if (pMob) {
                        if (pMob->IsActor()) {
                            pactor = static_cast<Actor*>(pMob);
                        }
                    }
                    if (pactor) {
                        HighProcess* pProcess = static_cast<HighProcess*>(pactor->pCurrentProcess);
                        double dRadReEvaluationTimer = pProcess->GetRadReEvaluationTimer();
                        if (fRadiationReevaluationRate.Float() < dRadReEvaluationTimer) {
                            float fdistance = pactor->GetDistanceFromReference(preference, 0, 1);
                            float fadjustedRadiation =
                                GamePlayFormulas::GetRadiationLevel(fradiation, fradius, fdistance);
                            float fresistance = pactor->GetRadiationResistanceMult();
                            float fadjustedRadiation_2 = fadjustedRadiation * fresistance;
                            if (fRadiationAvoidThreshold.Float() < static_cast<double>(fadjustedRadiation_2) &&
                                static_cast<double>(fadjustedRadiation_2) > pProcess->GetCurrentRadiation()) {
                                if (pactor->ShouldActorAvoidRadiation()) {
                                    if (pProcess->IsAvoidAreaInAvoidPathingList(preference)) {
                                        if (pactor->IsPathingComplete()) {
                                            pactor->InitiateAvoidPackage(preference, 0.0f);
                                        }
                                        else {
                                            pactor->SetMoveMode(512u);
                                        }
                                    }
                                    else {
                                        TESObjectREFR* afRadius = preference;
                                        NiPoint3 v34 = preference->GetLocationOnReference();
                                        pProcess->AddAvoidPathingArea(
                                            pactor, v34, fradius, 3.4028235e+38f, fadjustedRadiation_2, afRadius, 0
                                        );
                                    }
                                }
                                pProcess->SetCurrentRadiation(fadjustedRadiation_2);
                            }
                            float afTimer = pProcess->GetCurrentRadiation();
                            pProcess->SetRadiationDelta(afTimer);
                        }
                    }
                }
                HighProcess* pProcess_2 = static_cast<HighProcess*>(pPlayer->pCurrentProcess);
                float fdistance_2 = pPlayer->GetDistanceFromReference(preference, 0, 1);
                float fadjustedRadiation_3 = GamePlayFormulas::GetRadiationLevel(fradiation, fradius, fdistance_2);
                if (fadjustedRadiation_3 > 0.0) {
                    float fresistance_2 = pPlayer->GetRadiationResistanceMult();
                    fadjustedRadiation_3 = fadjustedRadiation_3 * fresistance_2;
                }
                if (pPlayer) {
                    if (fadjustedRadiation_3 > 0.0 &&
                        static_cast<double>(fadjustedRadiation_3) > pProcess_2->GetCurrentRadiation()) {
                        pProcess_2->SetCurrentRadiation(fadjustedRadiation_3);
                    }
                }
                if (fadjustedRadiation_3 > static_cast<double>(fheaviestPlayerRadiation)) {
                    fheaviestPlayerRadiation = fadjustedRadiation_3;
                }
                float afTimera = pProcess_2->GetCurrentRadiation();
                pProcess_2->SetRadiationDelta(afTimera);
            }
        }
    }
    bPlayerInRadiatedWater = fheaviestPlayerRadiation > 0.0;
    if (bclearRadiation) {
        for (uint32_t i_2 = 0; i_2 < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i_2) {
            Actor* pactor_2;
            MobileObject* pMob_2 = MobProcessArray.GetItem(i_2);
            if (pMob_2 && pMob_2->IsActor()) {
                pactor_2 = static_cast<Actor*>(pMob_2);
            }
            else {
                pactor_2 = 0;
            }
            if (pactor_2) {
                HighProcess* pProcess_3 = static_cast<HighProcess*>(pactor_2->pCurrentProcess);
                if (pProcess_3) {
                    pProcess_3->SetRadiationDelta(0.0f);
                }
            }
        }
        HighProcess* pProcess_4 = static_cast<HighProcess*>(pPlayer->pCurrentProcess);
        if (pProcess_4) {
            pProcess_4->SetRadiationDelta(0.0f);
        }
    }
}

bool ProcessLists::IsActorDetected(Actor* apActor) {
    bool v8 = 0;
    Actor* v9 = 0;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH) && !v8; ++uiIndex) {
        TESForm* v7 = MobProcessArray.GetItem(uiIndex);
        if (v7) {
            if (v7->IsActor()) {
                v9 = static_cast<Actor*>(v7);
            }
        }
        if (v9) {
            if (v9->IsNpc() && v9->GetDetectionLevelValue(apActor) == DETECTION_SEEN) {
                CombatController* v2;
                CombatController* v3;
                if (!v9->IsInCombat() ||
                    (!v9->GetCombatController() ||
                     ((v2 = v9->GetCombatController(), v2->GetTarget() != apActor) ||
                      (v3 = v9->GetCombatController(), !static_cast<uint8_t>(v3->IsNotDetectingTarget()))))) {
                    v8 = 1;
                }
            }
        }
    }
    return v8;
}

void ProcessLists::NullOutActorsPackage(TESPackage* apPackage) {
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = parray->GetProcessLevelHead(PROCESS_TYPE_HIGH); i < parray->GetProcessLevelTail(PROCESS_TYPE_LOW);
         ++i) {
        MobileObject* pmob = parray->GetItem(i);
        Actor* pactor = 0;
        if (pmob) {
            if (pmob->IsActor() && !pmob->IsStillLoading()) {
                pactor = static_cast<Actor*>(pmob);
            }
        }
        while (pactor) {
            TESPackage* pPackageRunning = 0;
            if (pactor->pCurrentProcess) {
                BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                pPackageRunning = CurrentAIProcess->GetPackageThatIsRunning();
            }
            if (pactor->pCurrentProcess) {
                BaseProcess* v5 = pactor->pCurrentProcess;
                if (pPackageRunning != v5->GetCurrentPackage()) {
                    MiddleHighProcess* pProcess = 0;
                    if (pactor->pCurrentProcess) {
                        PROCESS_TYPE eLevel = pactor->pCurrentProcess->eLevel;
                        if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                            pProcess = static_cast<MiddleHighProcess*>(pactor->pCurrentProcess);
                        }
                    }
                    if (pPackageRunning == apPackage) {
                        if (pProcess) {
                            pProcess->ClearRunOncePackage();
                        }
                    }
                }
            }
            if (pactor->pCurrentProcess) {
                BaseProcess* v4 = pactor->pCurrentProcess;
                if (v4->GetCurrentPackage() == apPackage) {
                    BaseProcess* v3 = pactor->pCurrentProcess;
                    v3->ClearCurrentPackage();
                    if (!pSaveLoadGame->GetSaveGameLoading()) {
                        BaseProcess* v2 = pactor->pCurrentProcess;
                        v2->CheckforNewPackage(pactor, 1);
                    }
                }
            }
            if ((&pactor->m_Extra)->GetPackageExtra() == apPackage) {
                (&pactor->m_Extra)->SetPackageExtra(0, 0, 0, 0, 0, 0);
            }
            pactor = 0;
        }
    }
}

void ProcessLists::StopCombatandAlarmonActor(Actor* a1, bool a2) {
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = parray->GetProcessLevelHead(PROCESS_TYPE_HIGH);; ++i) {
        uint32_t ProcessLevelTail = parray->GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pmob = parray->GetItem(i);
        Actor* pactor = 0;
        if (pmob) {
            if (pmob->IsActor()) {
                pactor = static_cast<Actor*>(pmob);
            }
        }
        if (pactor) {
            if (pactor->IsInCombatWithActor(a1)) {
                pactor->StopCombat(a1);
            }
            else {
                if (pactor) {
                    if (!a2 && pactor->IsAlarmed()) {
                        if (pactor->GetCurrentPackage()->GetPackageTarget()) {
                            TESPackage* CurrentPackage = pactor->GetCurrentPackage();
                            if (CurrentPackage->GetPackageTarget()->GetTargReference() == a1 || !a1) {
                                pactor->EndInterruptPackage(0);
                                BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                                CurrentAIProcess->ClearActionHeadTrackTarget(1);
                            }
                        }
                    }
                }
            }
            if (a1 == pPlayer) {
                pactor->RemoveEnemyFlagFromAllFactions();
                (&pactor->m_Extra)->RemovePlayerCrimeListExtra(pactor);
                if (pactor->IsCreature()) {
                    BSSimpleList<FACTION_RANK*>* apList2;
                    TESFaction* pmyFactionArray[128];
                    BSSimpleList<FACTION_RANK*>* pmyFactionList = &pactor->GetActorBase()->listFactions;
                    ExtraFactionChanges* pmyFactionChanges = (&pactor->m_Extra)->GetFactionChangesExtra();
                    if (pmyFactionChanges) {
                        apList2 = pmyFactionChanges->pFactionChanges;
                    }
                    else {
                        apList2 = 0;
                    }
                    uint32_t imyCount = pactor->IntegrateFactionLists(pmyFactionArray, 128u, pmyFactionList, apList2);
                    for (uint32_t i_2 = 0; i_2 < imyCount; ++i_2) {
                        if (pmyFactionArray[i_2]) {
                            if (pmyFactionArray[i_2]->GetPCAnEnemy()) {
                                pmyFactionArray[i_2]->SetPCAnEnemy(0);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ProcessLists::UpdateTheCrimeList() {
    Crime* v2;
    int32_t i;
    BSSimpleList<Crime*>* v5 = nullptr;
    BSSimpleList<Crime*>* ecx0 = nullptr;
    for (i = 0; i < 5; ++i) {
        v5 = GlobalCrimeListArray[i];
        ecx0 = v5;
        while (ecx0 && ecx0->m_item) {
            v2 = ecx0->m_item;
            if (v2->ShouldCrimeBeRemoved()) {
                v2->ClearActorKnowList();
                v5->Remove(v2);
                if (v2) {
                    delete v2;
                }
                ecx0 = v5;
            }
            else {
                ecx0 = ecx0->m_pkNext;
            }
        }
    }
    fCrimeUpdateTimer = (float)(((double)((int32_t)iNumberHighActors)) + 10.0);
}

void ProcessLists::SendGrenadeMineAlert(TESObjectREFR* apShooter, Projectile* apProjectile) {
    ;
}

void ProcessLists::UpdateMiddleHighList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* TempList;
    MobileObject* pmob;
    pmob = 0;
    char v16 = 1;
    BSSimpleList<MobileObject*>* v7 =
        static_cast<BSSimpleList<MobileObject*>*>(ourMalloc(sizeof(BSSimpleList<MobileObject*>)));
    if (v7) {
        TempList = ::new (v7) BSSimpleList<MobileObject*>;
    }
    else {
        TempList = 0;
    }
    uint32_t ilow = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
    for (uint32_t i = ilow; i < ihigh; ++i) {
        pmob = MobProcessArray.GetItem(i);
        if (pmob) {
            if (!pmob->IsStillLoading()) {
                if (pmob->GetDisabled() ||
                    (pmob->GetDelete() || pmob->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_HIGH)) {
                    TempList->AddHead(pmob);
                }
                else {
                    if (!ProcessLists::bIsForciblyUpdated) {
                        float flastprocesstime = pmob->pCurrentProcess->fHourLastProcessed;
                        float fhour = appCalendar.GetHour();
                        pmob->IsActor();
                        if (flastprocesstime == -1.0) {
                            pmob->InitLastTimeProcessed();
                        }
                        else {
                            if (fhour - flastprocesstime >= 0.15000000596046448 ||
                                (fhour < static_cast<double>(flastprocesstime) || afDelta != 0.0)) {
                                BaseProcess* CurrentAIProcess = pmob->pCurrentProcess;
                                if (CurrentAIProcess->CheckCombatDelayTimer()) {
                                    pmob->Process(afDelta);
                                    if (Script::GetProcessScripts()) {
                                        pmob->RunScript();
                                    }
                                    Actor* pact = 0;
                                    if (pmob) {
                                        if (pmob->IsActor()) {
                                            pact = static_cast<Actor*>(pmob);
                                        }
                                    }
                                    if (pact) {
                                        if (pact->GetLifeState() == ACTOR_LIFE_STATE_DYING) {
                                            v16 = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for (BSSimpleList<MobileObject*>* plisttemp = TempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_MIDDLE_HIGH);
        if (pmob->IsTransient()) {
            ReferenceProcessLists.kUpdateLock.Lock(0);
            if (pmob) {
                delete pmob;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        else {
            if (pmob->GetDisabled()) {
                ReferenceProcessLists.kUpdateLock.Lock(0);
                BaseProcess* v6 = pmob->pCurrentProcess;
                if (v6) {
                    delete v6;
                }
                pmob->SetCurrentAIProcess(0);
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
        }
    }
    TempList->RemoveAll();
    if (TempList) {
        delete TempList;
    }
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::SendActorsYellAlarm(Crime* apCrime, BSSimpleList<TESObjectREFR*>* apDoorList) {
    BSSimpleList<TESObjectREFR*>* preflist;
    void* v14 = ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>));
    if (v14) {
        preflist = ::new (v14) BSSimpleList<TESObjectREFR*>;
    }
    else {
        preflist = 0;
    }
    Actor* apCriminal = apCrime->pcriminal;
    TESObjectCELL* pcrimcell = apCriminal->GetParentCell();
    TESWorldSpace* pcrimworld = apCriminal->GetWorldSpace();
    apCrime->breported = 1;
    if (!apCriminal) {
        return preflist;
    }
    MobileObject* v29 = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = 0;
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        BSSimpleList<Actor*>* ptemp = 0;
        while (pactor) {
            float fdistance = 3.4028235e+38f;
            if (!pactor->IsDead(0) && !pactor->GetDisabled() && !pactor->GetDelete() && !pactor->IsDead(0)) {
                TESObjectCELL* pactorcell = pactor->GetParentCell();
                TESWorldSpace* pactorworld = pactor->GetWorldSpace();
                AlarmPackage* palarm = 0;
                TESPackage* pbasepack = pactor->GetCurrentPackage();
                if (pbasepack) {
                    if (pbasepack->GetPackType() == PACKAGE_TYPE_ALARM) {
                        palarm = static_cast<AlarmPackage*>(pbasepack);
                    }
                }
                if (!palarm || !palarm->IsCrimeInList(apCrime)) {
                    if ((pactorcell && pactorcell == pcrimcell) ||
                        (pcrimworld == pactorworld &&
                         ((pactorcell && !pactorcell->GetInterior()) || (pcrimcell && !pcrimcell->GetInterior())))) {
                        fdistance = pactor->GetDistanceFromReference(apCriminal, 0, 0);
                        double v5 = static_cast<double>(iCrimeAlarmRecDistance.Int());
                        if (fdistance <= v5) {
                            TESObjectREFR* a2 = (TESObjectREFR*)pactor;
                            preflist->AddHead(a2);
                        }
                    }
                    else {
                        for (TESObjectREFR* pref : apDoorList->ItemsUntilNull()) {
                            if (pref) {
                                DoorTeleportData* pTel = pref->GetTeleport();
                                TESObjectREFR* pDoorRef = 0;
                                if (pTel) {
                                    TESWorldSpace* WorldSpace;
                                    pDoorRef = pTel->pLinkedDoor;
                                    TESObjectCELL* ParentCell = pDoorRef->GetParentCell();
                                    if (ParentCell == pactorcell ||
                                        (!pDoorRef->GetParentCell() &&
                                         (WorldSpace = pDoorRef->GetWorldSpace(),
                                          WorldSpace == pactor->GetWorldSpace()))) {
                                        fdistance = pactor->GetDistanceFromReference(pDoorRef, 0, 0);
                                        double v8 = static_cast<double>(iCrimeAlarmRecDistance.Int());
                                        if (fdistance <= v8) {
                                            TESObjectREFR* item = (TESObjectREFR*)pactor;
                                            preflist->AddHead(item);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (ptemp && ptemp->m_item) {
                pactor = 0;
                Actor* m_item = ptemp->m_item;
                if (m_item->IsActor()) {
                    pactor = ptemp->m_item;
                }
                ptemp = ptemp->m_pkNext;
            }
            else {
                pactor = 0;
            }
        }
    }
    if (preflist) {
        preflist->Sort(&ProcessLists::DistanceListCompareFn);
    }
    return preflist;
}

void ProcessLists::UpdateLowList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* TempList;
    _LARGE_INTEGER icurrentTime;
    MobileObject* pmob;
    _LARGE_INTEGER imaxTime;
    pmob = 0;
    BSSimpleList<MobileObject*>* pObjectList =
        static_cast<BSSimpleList<MobileObject*>*>(ourMalloc(sizeof(BSSimpleList<MobileObject*>)));
    if (pObjectList) {
        TempList = ::new (pObjectList) BSSimpleList<MobileObject*>;
    }
    else {
        TempList = 0;
    }
    int32_t v18 = 0;
    QueryPerformanceCounter(&imaxTime);
    uint32_t v3 = iLowProcessingMilliseconds.Int();
    imaxTime.QuadPart += v3 * ProcessLists::ulTimerFrequency.QuadPart / 1000;
    bool bfirst = 1;
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
    if (!iArrayNumbStopped || iArrayNumbStopped >= ihigh) {
        iArrayNumbStopped = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
    }
    int32_t iamount = ihigh;
    if (abSleeping) {
        iamount = ihigh;
    }
    float fcurrenthour = appCalendar.GetHour();
    bool bstopflag = 0;
    for (uint32_t i = iArrayNumbStopped; i < iamount && !bstopflag; ++i) {
        pmob = MobProcessArray.GetItem(i);
        if (!abSleeping && !(i % 5)) {
            QueryPerformanceCounter(&icurrentTime);
            if (icurrentTime.QuadPart >= imaxTime.QuadPart) {
                bstopflag = 1;
            }
        }
        if (pmob) {
            BaseProcess* pproc = pmob->pCurrentProcess;
            if (pproc) {
                if (!pmob->IsStillLoading()) {
                    if (pmob->GetDisabled() || (pmob->GetDelete() || pproc->eLevel != PROCESS_TYPE_LOW)) {
                        TempList->AddHead(pmob);
                    }
                    else {
                        if (!ProcessLists::bIsForciblyUpdated) {
                            float flastprocesstime;
                            if (!pmob->GetRunsInLow() ||
                                (flastprocesstime = pproc->fHourLastProcessed, flastprocesstime == -1.0)) {
                                static_cast<LowProcess*>(pproc)->LowProcess::ComputeLastTimeProcessed();
                            }
                            else {
                                float fhour = fcurrenthour - flastprocesstime;
                                if ((fhour >= 1.0 || (fhour < 0.0 || afDelta != 0.0)) &&
                                    pproc->CheckCombatDelayTimer()) {
                                    pmob->Process(afDelta);
                                    if (Script::GetProcessScripts()) {
                                        pmob->RunScript();
                                    }
                                    Actor* pact = 0;
                                    if (pmob) {
                                        if (pmob->IsActor()) {
                                            pact = static_cast<Actor*>(pmob);
                                        }
                                    }
                                    if (pact) {
                                        if (pact->GetLifeState() == ACTOR_LIFE_STATE_DYING) {
                                            bfirst = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        iArrayNumbStopped = i + 1;
    }
    for (BSSimpleList<MobileObject*>* plisttemp = TempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_LOW);
        if (pmob->GetDisabled()) {
            ReferenceProcessLists.kUpdateLock.Lock(0);
            BaseProcess* pproc_2 = pmob->pCurrentProcess;
            if (pproc_2) {
                delete pproc_2;
            }
            pmob->SetCurrentAIProcess(0);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    TempList->RemoveAll();
    if (TempList) {
        delete TempList;
    }
}

void ProcessLists::UpdateMiddleLowList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* TempList;
    MobileObject* pmob = 0;
    BSSimpleList<MobileObject*>* v8 =
        static_cast<BSSimpleList<MobileObject*>*>(ourMalloc(sizeof(BSSimpleList<MobileObject*>)));
    if (v8) {
        TempList = ::new (v8) BSSimpleList<MobileObject*>;
    }
    else {
        TempList = 0;
    }
    bool v18 = 1;
    uint32_t ilow = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_LOW);
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_LOW);
    for (uint32_t i = ilow; i < ihigh; ++i) {
        pmob = MobProcessArray.GetItem(i);
        Actor* pact = 0;
        if (pmob) {
            if (!pmob->IsStillLoading()) {
                if (pmob->IsActor()) {
                    pact = static_cast<Actor*>(pmob);
                }
                if (pmob->GetDisabled() ||
                    (pmob->GetDelete() || pmob->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_LOW)) {
                    TempList->AddHead(pmob);
                }
                else {
                    if (!ProcessLists::bIsForciblyUpdated) {
                        if (pmob->GetRunsInLow() || (!pact || pact->IsInCombat())) {
                            float flastprocesstime = pmob->pCurrentProcess->fHourLastProcessed;
                            float fhour = appCalendar.GetHour();
                            if (flastprocesstime == -1.0) {
                                pmob->InitLastTimeProcessed();
                            }
                            else {
                                if (fhour - flastprocesstime >= 0.30000001192092896 ||
                                    (fhour < static_cast<double>(flastprocesstime) || afDelta != 0.0)) {
                                    BaseProcess* CurrentAIProcess = pmob->pCurrentProcess;
                                    if (CurrentAIProcess->CheckCombatDelayTimer()) {
                                        pmob->Process(afDelta);
                                        if (Script::GetProcessScripts()) {
                                            pmob->RunScript();
                                        }
                                        Actor* pact_2 = 0;
                                        if (pmob) {
                                            if (pmob->IsActor()) {
                                                pact_2 = static_cast<Actor*>(pmob);
                                            }
                                        }
                                        if (pact_2) {
                                            if (pact_2->GetLifeState() == ACTOR_LIFE_STATE_DYING) {
                                                v18 = 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            BaseProcess* v4 = pmob->pCurrentProcess;
                            v4->ComputeLastTimeProcessed();
                        }
                    }
                }
            }
        }
    }
    for (BSSimpleList<MobileObject*>* plisttemp = TempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_MIDDLE_LOW);
        if (pmob->GetDisabled()) {
            ReferenceProcessLists.kUpdateLock.Lock(0);
            BaseProcess* v7 = pmob->pCurrentProcess;
            if (v7) {
                delete v7;
            }
            pmob->SetCurrentAIProcess(0);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    TempList->RemoveAll();
    if (TempList) {
        delete TempList;
    }
}

void ProcessLists::ProcessHighActorsForFastTravel() {
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        PlayerCharacter* pActor = static_cast<PlayerCharacter*>(MobProcessArray.GetItem(uiIndex));
        if (pActor) {
            if (pActor->IsActor() && !pActor->IsStillLoading() &&
                (pActor->GetParentCell() || pActor->GetWorldSpace()) && pActor != pPlayer) {
                if (pActor->GetCurrentAIProcess()) {
                    if (!pActor->GetDelete() && !pActor->GetDisabled() && !pActor->IsInCombat()) {
                        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                        float LastTimeProcessed = pProcess->GetLastTimeProcessed();
                        float fCurrentHour = appCalendar.GetHour();
                        float v8 = fCurrentHour - LastTimeProcessed;
                        if (LastTimeProcessed > static_cast<double>(fCurrentHour)) {
                            v8 = fCurrentHour + 24.0 - LastTimeProcessed;
                        }
                        bool bIsEscort = 0;
                        if (v8 < 0.05) {
                            v8 = 0.0;
                        }
                        if (pActor->GetCurrentAIProcess()) {
                            pProcess = pActor->GetCurrentAIProcess();
                            if (pProcess->GetCurrentPackage()) {
                                pProcess = pActor->GetCurrentAIProcess();
                                TESPackage* v2 = pProcess->GetCurrentPackage();
                                if (v2->GetPackType() == PACKAGE_TYPE_ESCORT) {
                                    bIsEscort = 1;
                                }
                            }
                        }
                        if ((!bIsEscort && !pActor->IsFollowing()) ||
                            (pProcess = pActor->GetCurrentAIProcess(), pProcess->GetTarget() != pPlayer)) {
                            pActor->Process(v8);
                        }
                    }
                }
            }
        }
    }
}

Actor* ProcessLists::GetActorRefInHigh(TESFaction* apFaction, bool abDetection, bool abClosetoPlayer) {
    Actor* v14 = 0;
    float v13 = 10000.0f;
    Actor* v15 = 0;
    PlayerCharacter* v11 = 0;
    Actor* v12 = 0;
    for (uint32_t uiIndex = 0;
         (!v14 || !abClosetoPlayer) && uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        PlayerCharacter* pActor = 0;
        Actor* pObject = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        if (pObject) {
            if (pObject->IsActor()) {
                pActor = static_cast<PlayerCharacter*>(pObject);
            }
        }
        if (pActor) {
            if (!pActor->IsDead(0) && !pActor->IsUnConscious() &&
                pActor->GetActorBase()->GetFactionRank(apFaction, pActor == pPlayer) > -1) {
                BaseProcess* pProc;
                if (!abDetection || (pProc = pActor->pCurrentProcess, pProc->GetDetectionActor(pPlayer, 0) > 0)) {
                    v14 = pActor;
                    if (abClosetoPlayer) {
                        float DistanceFromReference = pActor->GetDistanceFromReference(pPlayer, 0, 0);
                        if (v13 > static_cast<double>(DistanceFromReference)) {
                            v15 = pActor;
                            if (pActor->IsCreature()) {
                                v12 = pActor;
                                v15 = 0;
                            }
                            v13 = DistanceFromReference;
                        }
                    }
                    if (pActor->IsCreature()) {
                        v11 = pActor;
                        v14 = 0;
                    }
                }
            }
        }
    }
    if (v14 || !v11) {
        if (abClosetoPlayer) {
            if (v15) {
                return v15;
            }
        }
    }
    else {
        v14 = v11;
        if (abClosetoPlayer) {
            return v12;
        }
    }
    return v14;
}

BSSimpleList<TESObjectREFR*>* ProcessLists::SendCrimetoHighList(Crime* apCrime) {
    BSSimpleList<TESObjectREFR*>* preflist_2 = 0;
    Actor* pcriminal = apCrime->pcriminal;
    if (pcriminal && (pcriminal->GetDisabled() || (pcriminal->GetDelete() || pcriminal->IsDead(1)))) {
        return 0;
    }
    int32_t iguards = 0;
    if (pcriminal) {
        BaseProcess* CurrentAIProcess = pcriminal->pCurrentProcess;
        iguards = CurrentAIProcess->GetNumberGuardsArresting();
    }
    if (iguards > 0) {
        BSSimpleList<TESObjectREFR*>* pPersuelist = IsActorTargetingREFinPackage(pcriminal, PACKAGE_TYPE_ALARM, 0);
        if (pPersuelist) {
            iguards = pPersuelist->ItemsInList();
        }
        else {
            iguards = 0;
        }
        BaseProcess* v5 = pcriminal->pCurrentProcess;
        v5->SetNumberGuardsArresting(iguards);
    }
    MobileObject* pMobileObject = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        Actor* pactor = 0;
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        if (pactor) {
            bool blinesight;
            if (!pactor->IsUnConscious() && !pactor->IsParalyzed() && pactor != pcriminal && !pactor->IsDead(0) &&
                !pactor->GetDisabled() &&
                pactor->GetDetectionLevelAgainstActor(0, pcriminal, blinesight, 0, 0, 0, 0) > 0) {
                if (!preflist_2) {
                    BSSimpleList<TESObjectREFR*>* preflist;
                    BSSimpleList<TESObjectREFR*>* v9 =
                        static_cast<BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
                    if (v9) {
                        preflist = ::new (v9) BSSimpleList<TESObjectREFR*>;
                    }
                    else {
                        preflist = 0;
                    }
                    BSSimpleList<TESObjectREFR*>* v10 = preflist;
                    preflist_2 = preflist;
                }
                TESObjectREFR* item = pactor;
                preflist_2->AddHead(item);
            }
        }
    }
    if (preflist_2) {
        preflist_2->Sort(&ProcessLists::DistanceListCompareFn);
    }
    return preflist_2;
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::SendResistArresttoGuards(Actor* apActor, BSSimpleList<TESObjectREFR*>* apDoorList) {
    BSSimpleList<TESObjectREFR*>* preflist;
    void* v18 = ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>));
    if (v18) {
        preflist = ::new (v18) BSSimpleList<TESObjectREFR*>;
    }
    else {
        preflist = 0;
    }
    TESObjectCELL* pcrimcell = apActor->GetParentCell();
    TESWorldSpace* pcrimworld = apActor->GetWorldSpace();
    if (!apActor) {
        return preflist;
    }
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    int32_t iamount = CurrentAIProcess->GetNumberGuardsArresting();
    BaseProcess* v12 = pPlayer->pCurrentProcess;
    v12->ModNumberGuardsArresting(1 - iamount);
    Actor* pMobileObject = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            Actor* pactor = 0;
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            float fdistance = 3.4028235e+38f;
            BSSimpleList<Actor*>* ptemp = 0;
            while (pactor) {
                TESObjectCELL* pactorcell = pactor->GetParentCell();
                TESWorldSpace* pactorworld = pactor->GetWorldSpace();
                if (!pactor->IsInCombat() && pactor->IsActoraGuard()) {
                    if ((pactorcell && (pactorcell->GetInterior() && pactorcell == pcrimcell)) ||
                        (pcrimworld == pactorworld &&
                         ((pactorcell && !pactorcell->GetInterior()) || (pcrimcell && !pcrimcell->GetInterior())))) {
                        fdistance = pactor->GetDistanceFromReference(apActor, 0, 0);
                        double v5 = static_cast<double>(iCrimeAlarmRecDistance.Int());
                        if (fdistance <= v5) {
                            BaseProcess* v11 = pactor->pCurrentProcess;
                            v11->EnterCombat(pactor, apActor, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
                            TESObjectREFR* a2 = (TESObjectREFR*)pactor;
                            preflist->AddHead(a2);
                        }
                    }
                    else {
                        for (TESObjectREFR* pref : apDoorList->ItemsUntilNull()) {
                            if (pref) {
                                DoorTeleportData* pTel = pref->GetTeleport();
                                TESObjectREFR* pDoorRef = 0;
                                if (pTel) {
                                    TESWorldSpace* WorldSpace;
                                    pDoorRef = pTel->pLinkedDoor;
                                    TESObjectCELL* ParentCell = pDoorRef->GetParentCell();
                                    if (ParentCell == pactorcell ||
                                        (!pDoorRef->GetParentCell() &&
                                         (WorldSpace = pDoorRef->GetWorldSpace(),
                                          WorldSpace == pactor->GetWorldSpace()))) {
                                        fdistance = pactor->GetDistanceFromReference(pDoorRef, 0, 0);
                                        double v8 = static_cast<double>(iCrimeAlarmRecDistance.Int());
                                        if (fdistance <= v8) {
                                            BaseProcess* v10 = pactor->pCurrentProcess;
                                            v10->EnterCombat(pactor, apActor, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
                                            TESObjectREFR* item = (TESObjectREFR*)pactor;
                                            preflist->AddHead(item);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (ptemp && ptemp->m_item) {
                    pactor = 0;
                    Actor* m_item = ptemp->m_item;
                    if (m_item->IsActor()) {
                        pactor = ptemp->m_item;
                    }
                    ptemp = ptemp->m_pkNext;
                }
                else {
                    pactor = 0;
                }
            }
        }
    }
    return preflist;
}

BSSimpleList<TESObjectREFR*>* ProcessLists::GetActorsProcessCrimeAlarm(Crime* apCrime, bool bdelete) {
    BSSimpleList<TESObjectREFR*>* preflist_2 = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            Actor* pactor = 0;
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            if (pactor) {
                AlarmPackage* palarm = 0;
                TESPackage* pbasepack = pactor->GetCurrentPackage();
                if (pbasepack) {
                    if (pbasepack->GetPackType() == PACKAGE_TYPE_ALARM) {
                        palarm = static_cast<AlarmPackage*>(pbasepack);
                    }
                }
                if (palarm) {
                    if (palarm->IsCrimeInList(apCrime)) {
                        if (!preflist_2) {
                            BSSimpleList<TESObjectREFR*>* preflist;
                            BSSimpleList<TESObjectREFR*>* v8 = static_cast<BSSimpleList<TESObjectREFR*>*>(
                                ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>))
                            );
                            if (v8) {
                                preflist = ::new (v8) BSSimpleList<TESObjectREFR*>;
                            }
                            else {
                                preflist = 0;
                            }
                            preflist_2 = preflist;
                        }
                        if (bdelete) {
                            palarm->RemoveCrime(apCrime, pactor);
                        }
                        else {
                            TESObjectREFR* pRef = pactor;
                            preflist_2->AddHead(pRef);
                        }
                    }
                }
            }
        }
    }
    if (bdelete) {
        if (preflist_2) {
            delete preflist_2;
        }
        return 0;
    }
    return preflist_2;
}

void ProcessLists::TurnOffMovementinHigh() {
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        Actor* v4 = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        Actor* v2 = 0;
        if (v4) {
            v2 = v4;
        }
        if (v2) {
            v2->SetMovementComplete(1);
            v2->PickAnimations(1.0, 1.0);
        }
    }
}

BSSimpleList<Crime*>* ProcessLists::RemoveCrimesForYieldCheck(float afTime) {
    BSSimpleList<Crime*>* pReturnList;
    FNVD_INLINED_LOCAL(CombatTimeStamp, kTimeStamp, std::nullopt, std::nullopt);
    BSSimpleList<Crime*>* plist = GlobalCrimeListArray[3];
    BSSimpleList<Crime*>* v7 = static_cast<BSSimpleList<Crime*>*>(ourMalloc(sizeof(BSSimpleList<Crime*>)));
    if (v7) {
        pReturnList = ::new (v7) BSSimpleList<Crime*>;
    }
    else {
        pReturnList = 0;
    }
    bool bflag = 0;
    BSSimpleList<Crime*>* plastlist = 0;
    while (plist && (!plist->IsEmpty() && !bflag)) {
        Crime* pcrime = plist->m_item;
        CombatTimeStamp* pTimeStamp = pcrime->GetTimeStamp(&kTimeStamp);
        if (!(static_cast<double>(afTime) >= pTimeStamp->ElapsedTime())) {
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            for (BSSimpleList<TESFaction*>* pfactlist = pcrime->pListofEnemyFactions;
                 pfactlist && !pfactlist->IsEmpty(); pfactlist = pfactlist->m_pkNext) {
                TESFaction* pfac = pfactlist->m_item;
                if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, pcrime)) {
                    pfac->SetPCAnEnemy(0);
                }
            }
            Actor* pact = static_cast<Actor*>(pcrime->pcrimetarget);
            if (pact) {
                if (pact->IsActor()) {
                    void* v9 = (void*)pact;
                    BSSimpleList<Crime*>* pcrimelist = (&pact->m_Extra)->GetPlayerCrimeList();
                    if (pcrimelist) {
                        pcrimelist->Remove(pcrime);
                    }
                }
            }
            plist->Remove(pcrime);
            plist = plastlist;
            if (!plastlist) {
                plist = GlobalCrimeListArray[3];
            }
            pReturnList->AddHead(pcrime);
        }
    }
    if (pReturnList->IsEmpty()) {
        if (pReturnList) {
            delete pReturnList;
        }
        return 0;
    }
    return pReturnList;
}

void ProcessLists::AddtoGlobalCrimeList(Crime* apCrime) {
    if (apCrime) {
        if (apCrime->ecrime > PackageNS::CRIME_TYPE_NONE && apCrime->ecrime < PackageNS::CRIME_TYPE_COUNT) {
            BSSimpleList<Crime*>* pGlobalCrimeList_2 = GlobalCrimeListArray[apCrime->ecrime];
            if (!pGlobalCrimeList_2) {
                BSSimpleList<Crime*>* pGlobalCrimeList;
                BSSimpleList<Crime*>* v4 = static_cast<BSSimpleList<Crime*>*>(ourMalloc(sizeof(BSSimpleList<Crime*>)));
                if (v4) {
                    pGlobalCrimeList = ::new (v4) BSSimpleList<Crime*>;
                }
                else {
                    pGlobalCrimeList = 0;
                }
                pGlobalCrimeList_2 = pGlobalCrimeList;
            }
            if (!pGlobalCrimeList_2->IsInList(apCrime)) {
                pGlobalCrimeList_2->AddHead(apCrime);
            }
            GlobalCrimeListArray[apCrime->ecrime] = pGlobalCrimeList_2;
        }
    }
}

bool ProcessLists::IsEnemyFactionofOtherCrime(TESFaction* apFaction, Crime* apCrime) {
    BSSimpleList<Crime*>* plist = GlobalCrimeListArray[3];
    bool bflag = 0;
    while (plist && (!plist->IsEmpty() && !bflag)) {
        Crime** v3 = &plist->m_item;
        Crime* pcrime = *v3;
        if (*v3 != apCrime) {
            Crime* v4 = *v3;
            if (pcrime->pListofEnemyFactions) {
                bflag = pcrime->pListofEnemyFactions->IsInList(apFaction);
            }
        }
        plist = plist->m_pkNext;
    }
    if (!bflag) {
        BSSimpleList<Crime*>* plist_2 = GlobalCrimeListArray[4];
        bool bflag_2 = 0;
        while (plist_2 && (!plist_2->IsEmpty() && !bflag_2)) {
            Crime** v5 = &plist_2->m_item;
            Crime* pcrime_2 = *v5;
            if (*v5 != apCrime) {
                Crime* v6 = *v5;
                if (pcrime_2->pListofEnemyFactions) {
                    bflag_2 = pcrime_2->pListofEnemyFactions->IsInList(apFaction);
                }
            }
            plist_2 = plist_2->m_pkNext;
        }
    }
    return bflag;
}

void ProcessLists::BuildFollowerListRecursive(Actor* apActor, BSSimpleList<Actor*>& aReturnList) {
    ExtraFollower* pfollow = (&apActor->m_Extra)->GetFollowerExtra();
    if (pfollow) {
        for (BSSimpleList<Actor*>* pfollowList = pfollow->pActorlist; pfollowList && !pfollowList->IsEmpty();
             pfollowList = pfollowList->m_pkNext) {
            Actor* pactor = pfollowList->m_item;
            if (!aReturnList.IsInList(pactor)) {
                aReturnList.AddHead(pactor);
                BuildFollowerListRecursive(pactor, aReturnList);
            }
        }
    }
    Actor* pMobileObject = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        Actor* pactor_2 = static_cast<Actor*>(MobProcessArray.GetItem(i));
        Actor* pactor_3 = 0;
        if (pactor_2) {
            if (pactor_2->IsActor()) {
                pactor_3 = pactor_2;
                if (pactor_2->GetCurrentPackage()) {
                    TESPackage* v5;
                    TESPackage* CurrentPackage = pactor_3->GetCurrentPackage();
                    if (CurrentPackage->GetPackType() == PACKAGE_TYPE_ESCORT ||
                        (v5 = pactor_3->GetCurrentPackage(), v5->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
                        if (pactor_3->GetCurrentPackageTarget() == apActor && !aReturnList.IsInList(pactor_3)) {
                            aReturnList.AddHead(pactor_3);
                            BuildFollowerListRecursive(pactor_3, aReturnList);
                        }
                    }
                }
            }
        }
    }
}

int32_t ProcessLists::GetHighestDetectionLevelForActor(Actor* apActor, bool abIncludeFollowers) {
    int32_t plist = 0;
    int32_t ilevel = -100;
    int32_t itotallevel = -(-0x7FFFFFFF);
    pPlayer->SetActorInSneakRange(0);
    MobileObject* pObject = 0;
    for (uint32_t i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = 0;
        if (pMob) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
        }
        bool bskipThisOne = 0;
        if (!abIncludeFollowers) {
            if (pactor) {
                BSSimpleList<Actor*> followerList;
                BuildFollowerListRecursive(apActor, followerList);
                if (followerList.IsInList(pactor) || (apActor == pPlayer && pactor->bPlayerTeammate)) {
                    bskipThisOne = 1;
                }
            }
        }
        if (!bskipThisOne && (pactor && (!pactor->IsDead(0) && (!pactor->GetDisabled() && !pactor->IsUnConscious())))) {
            FIGHT_REACTION ffight;
            if (apActor == pPlayer && !pPlayer->GetActorInSneakRange()) {
                float fdistance = apActor->GetDistanceFromReference(pactor, 0, 0);
                if (fdistance <= static_cast<double>(iSneakSkillUseDistance.Int())) {
                    pPlayer->SetActorInSneakRange(1);
                }
            }
            if (pactor->IsInCombat() || pactor->IsAlarmed()) {
                if (pactor->IsInCombat()) {
                    if (pactor->GetCombatController()) {
                        if (pactor->GetCombatController()->GetTarget() == apActor) {
                            CombatController* v4 = pactor->GetCombatController();
                            if (!static_cast<uint8_t>(v4->IsNotDetectingTarget())) {
                                return 100;
                            }
                        }
                    }
                }
                if (pactor->IsAlarmed() && pactor->GetCurrentPackageTarget() == pPlayer) {
                    return 100;
                }
            }
            BaseProcess* pProcess = pactor->pCurrentProcess;
            ilevel = pProcess->GetDetectionActor(apActor, 0);
            if (ilevel == 0x7FFFFFFF) {
                ilevel = 0;
            }
            if (ilevel <= 100) {
                if (ilevel < -100) {
                    ilevel = -100;
                }
            }
            else {
                ilevel = 100;
            }
            if (ilevel > 0 && pactor->GetShouldAttackActor(pPlayer, 0, ffight, 0)) {
                pPlayer->SetHostileDetection(1);
            }
        }
        if (itotallevel == 0x7FFFFFFF || itotallevel < ilevel) {
            itotallevel = ilevel;
        }
    }
    if (itotallevel == 0x7FFFFFFF) {
        return 0;
    }
    return itotallevel;
}

void ProcessLists::MoveActorsToReference(TESObjectREFR* apMarker) {
    if (apMarker) {
        for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetSize(); ++uiIndex) {
            TESForm* pObject = MobProcessArray.GetItem(uiIndex);
            Actor* pActor = 0;
            if (pObject) {
                if (pObject->IsActor()) {
                    pActor = static_cast<Actor*>(pObject);
                }
            }
            if (pActor) {
                if (!pActor->IsDead(0) &&
                    (pPlayer->IsActorFollowingEscortingPlayer(pActor, 0) || pActor->IsPlayerTeammate())) {
                    pActor->EndInterruptPackage(0);
                    pActor->MoveActorAndFollowers(apMarker, static_cast<bool>(0), static_cast<bool>(0));
                }
            }
        }
    }
}

void ProcessLists::RemoveParticleTempEffectsFor3D(NiAVObject* apFollow3D) {
    if (apFollow3D) {
        if (!GlobalTempEffectList.IsEmpty()) {
            BSSimpleList<NiPointer<BSTempEffect>>* pelist = &GlobalTempEffectList;
            BSSimpleList<NiPointer<BSTempEffect>>* plastlist = 0;
            while (pelist && !pelist->IsEmpty()) {
                BSTempEffectParticle* ppart;
                BSTempEffect* peffect = pelist->m_item.m_pObject;
                if (peffect->GetType() == TEMP_EFFECT_PARTICLE &&
                    ((ppart = dynamic_cast<BSTempEffectParticle*>(peffect)) != 0 &&
                     ppart->GetFollowObject() == apFollow3D)) {
                    NiAVObject* ppartObj = ppart->GetParticleObject();
                    if (ppartObj) {
                        ppartObj->SetAppCulled(1);
                    }
                    ppart->pCell = 0;
                    GarbageCollector::Add(peffect);
                    if (plastlist) {
                        {
                            NiPointer<BSTempEffect> spEffect(peffect);
                            plastlist->Remove(spEffect);
                        }
                        pelist = plastlist->m_pkNext;
                    }
                    else {
                        pelist->RemoveHead();
                    }
                }
                else {
                    plastlist = pelist;
                    pelist = pelist->m_pkNext;
                }
            }
        }
    }
}

MagicShaderHitEffect* ProcessLists::FindActiveShaderHitEffect(TESObjectREFR* apEffectTarget) {
    MagicShaderHitEffect* pactiveEffect = 0;
    float effectAge = 3.4028235e+38f;
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect) {
                if (NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                    pshaderEffect->pTarget == apEffectTarget &&
                    static_cast<double>(effectAge) > pshaderEffect->BSTempEffect::fAge &&
                    !pshaderEffect->bWeaponEnchantment) {
                    pactiveEffect = pshaderEffect;
                    effectAge = pshaderEffect->BSTempEffect::fAge;
                }
            }
        }
    }
    return pactiveEffect;
}

MagicShaderHitEffect*
ProcessLists::FindAndCleanupWeaponShaderHitEffect(TESObjectREFR* apEffectTarget, TESEffectShader* apShader) {
    MagicShaderHitEffect* pactiveEffect = 0;
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect) {
                if (NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                    pshaderEffect->pTarget == apEffectTarget && pshaderEffect->bWeaponEnchantment &&
                    !pshaderEffect->bFinished) {
                    if (pshaderEffect->pEffectData != apShader || pactiveEffect) {
                        pshaderEffect->SetFinished(1);
                    }
                    else {
                        pactiveEffect = pshaderEffect;
                    }
                }
            }
        }
    }
    return pactiveEffect;
}

uint32_t ProcessLists::EndAllNonPlayerConversations() {
    uint32_t result;
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        result = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (uiIndex >= result) {
            break;
        }
        TESForm* v5 = MobProcessArray.GetItem(uiIndex);
        MobileObject* v3 = 0;
        if (v5) {
            if (v5->IsActor()) {
                v3 = static_cast<MobileObject*>(v5);
            }
        }
        if (v3) {
            if (v3->IsinDialogue() && v3->GetCurrentTarget() != pPlayer) {
                v3->EndDialogue();
            }
        }
    }
    return result;
}

// Ends every high-process magic and arrow projectile (virtual SetDelete(true)), kills the other projectiles without an
// alt-trigger or detonating explosion, then ends the middle-high arrows.
int32_t ProcessLists::GetActiveMagicProjectileCount() {
    int32_t icount;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        MobileObject* Item = MobProcessArray.GetItem(uiIndex);
        MagicProjectile* pmagicProjectile = dynamic_cast<MagicProjectile*>(Item);
        if (pmagicProjectile) {
            pmagicProjectile->SetDelete(true);
        }
        else {
            ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(Item);
            if (parrow) {
                parrow->SetDelete(true);
            }
            else {
                Projectile* v5 = dynamic_cast<Projectile*>(Item);
                if (v5) {
                    if (!v5->GetProjectileBase() ||
                        (!v5->GetProjectileBase()->GetHasExplosionAltTrigger() &&
                         !v5->GetProjectileBase()->GetHasExplosionDetonates())) {
                        v5->Kill();
                    }
                }
            }
        }
    }
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);; ++i) {
        icount = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
        if (i >= icount) {
            break;
        }
        ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(MobProcessArray.GetItem(i));
        if (parrow) {
            parrow->SetDelete(true);
        }
    }
    return icount;
}

void ProcessLists::NullOutActorInCrimeList(TESObjectREFR* apRef) {
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* plist = GlobalCrimeListArray[i];
        BSSimpleList<Crime*>* plastlist = 0;
        while (plist && !plist->IsEmpty()) {
            Crime* pcrime = plist->m_item;
            bool badvanced = 0;
            if (pcrime->pcriminal == apRef || pcrime->pcrimetarget == apRef) {
                if (plastlist) {
                    plastlist->Remove(pcrime);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
                if (pcrime) {
                    delete pcrime;
                }
                badvanced = 1;
            }
            else {
                if (apRef->IsActor()) {
                    pcrime->RemoveActorKnowList(static_cast<Actor*>(apRef));
                }
            }
            if (!badvanced) {
                plastlist = plist;
                plist = plist->m_pkNext;
            }
        }
    }
}

int16_t ProcessLists::GetCrimesSaveSize() {
    int16_t v5 = 4;
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* ecx0 = GlobalCrimeListArray[i];
        v5 += 2;
        while (ecx0 && !ecx0->IsEmpty()) {
            Crime* v3 = ecx0->m_item;
            v5 += v3->GetSaveSize();
            ecx0 = ecx0->m_pkNext;
        }
    }
    return v5;
}

void ProcessLists::SaveGame_Crimes() {
    pSaveLoadGameOLD->SaveGameDataOLD(&ProcessLists::fSystemTimeClock, 4u);
    for (int32_t i = 0; i < 5; ++i) {
        uint16_t usCount = 0;
        uint16_t* BufferPointer = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usCount, 2u);
        for (BSSimpleList<Crime*>* ecx0 = GlobalCrimeListArray[i]; ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
            ecx0->m_item->SaveGame();
            ++usCount;
        }
        *BufferPointer = usCount;
    }
}

void ProcessLists::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPlayerActionCommentTimer), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPlayerKnockObjectCommentTimer), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCrimeUpdateTimer), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCrimeNumber), 4u, 0);
    for (uint32_t i = 0; i < 5; ++i) {
        uint32_t aiVariableValue = 0;
        uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
        for (BSSimpleList<Crime*>* ecx0 = GlobalCrimeListArray[i]; ecx0; ecx0 = ecx0->m_pkNext) {
            Crime* v3 = ecx0->m_item;
            if (v3) {
                v3->Save(apSaveGameBuffer);
                ++aiVariableValue;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    }
}

void ProcessLists::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPlayerActionCommentTimer), 4u);
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPlayerKnockObjectCommentTimer), 4u);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCrimeUpdateTimer), 4u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCrimeNumber), 4u);
    for (uint32_t i = 0; i < 5; ++i) {
        uint32_t VariableSizedValue = apLoadGameBuffer->LoadVariableSizedValue();
        if (VariableSizedValue) {
            if (GlobalCrimeListArray[i]) {
                GlobalCrimeListArray[i]->RemoveAll();
            }
            else {
                BSSimpleList<Crime*>* v3;
                BSSimpleList<Crime*>* v6 = static_cast<BSSimpleList<Crime*>*>(ourMalloc(sizeof(BSSimpleList<Crime*>)));
                if (v6) {
                    v3 = ::new (v6) BSSimpleList<Crime*>;
                }
                else {
                    v3 = 0;
                }
                GlobalCrimeListArray[i] = v3;
            }
            for (uint32_t j = 0; j < VariableSizedValue; ++j) {
                Crime* v2;
                Crime* apMemory = static_cast<Crime*>(ourMalloc(sizeof(Crime)));
                if (apMemory) {
                    v2 = ::new (apMemory) Crime;
                }
                else {
                    v2 = 0;
                }
                Crime* a2 = v2;
                v2->LoadGame(apLoadGameBuffer);
                GlobalCrimeListArray[i]->AddHead(a2);
            }
        }
    }
}

void ProcessLists::FlushNonPersistActors(int32_t aeMode) {
    BSSimpleList<void*>* v5;
    BSSimpleList<void*>* v6;
    void* v8;
    BSSimpleList<void*>* pInner;
    void* item;
    void* a2;
    BSSimpleList<void*>* pCloseActors = static_cast<BSSimpleList<void*>*>(ourMalloc(sizeof(BSSimpleList<void*>)));
    if (pCloseActors) {
        v6 = ::new (pCloseActors) BSSimpleList<void*>;
    }
    else {
        v6 = 0;
    }
    BSSimpleList<void*>* pFarActors = static_cast<BSSimpleList<void*>*>(ourMalloc(sizeof(BSSimpleList<void*>)));
    if (pFarActors) {
        v5 = ::new (pFarActors) BSSimpleList<void*>;
    }
    else {
        v5 = 0;
    }
    TESObjectREFR* pActor = 0;
    TESObjectREFR* m_item = 0;
    MobileObject* pMobileObject = 0;
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (uiIndex >= ProcessLevelTail) {
            break;
        }
        pMobileObject = MobProcessArray.GetItem(uiIndex);
        pActor = 0;
        if (pMobileObject) {
            if (pMobileObject->IsActor()) {
                pActor = pMobileObject;
            }
        }
        if (pActor) {
            if (!pActor->GetPersistFlagValue()) {
                if (aeMode == 2) {
                    ReferenceProcessLists.kUpdateLock.Lock(0);
                    TESObjectREFR* v10 = pActor;
                    if (pActor) {
                        delete v10;
                    }
                    ReferenceProcessLists.kUpdateLock.Unlock();
                    uiIndex = 0;
                }
                else {
                    if (!(pActor->GetDistanceFromReference(pPlayer, 0, 0) > 3000.0)) {
                        item = pActor;
                        v6->AddHead(item);
                    }
                    else {
                        a2 = pActor;
                        v5->AddHead(a2);
                    }
                }
            }
        }
    }
    if (aeMode == 1) {
        pInner = 0;
        BSSimpleList<void*>* pIter = 0;
        bool v17 = 0;
        if (v5->IsEmpty()) {
            pIter = v6;
            v17 = 1;
        }
        else {
            pIter = v5;
        }
        while (pIter && pIter->m_item) {
            pActor = static_cast<TESObjectREFR*>(pIter->m_item);
            if (pActor->IsCreature() && !v17) {
                pInner = v6;
                while (pInner && pInner->m_item) {
                    m_item = static_cast<TESObjectREFR*>(pInner->m_item);
                    const char* Model_ = pActor->GetModel();
                    const char* v3 = m_item->GetModel();
                    if (BSstricmp(v3, Model_)) {
                        pInner = pInner->m_pkNext;
                    }
                    else {
                        v6->Remove(m_item);
                        v8 = m_item;
                        pInner->Remove(v8);
                        pInner = v6;
                        ReferenceProcessLists.kUpdateLock.Lock(0);
                        if (m_item) {
                            delete m_item;
                        }
                        ReferenceProcessLists.kUpdateLock.Unlock();
                    }
                }
            }
            pIter->RemoveHead();
            ReferenceProcessLists.kUpdateLock.Lock(0);
            if (pActor) {
                delete pActor;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    else {
        if (!aeMode) {
            while (v5 && v5->m_item) {
                pActor = static_cast<TESObjectREFR*>(v5->m_item);
                v5->RemoveHead();
                ReferenceProcessLists.kUpdateLock.Lock(0);
                if (pActor) {
                    delete pActor;
                }
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
        }
    }
    v5->RemoveAll();
    if (v5) {
        delete v5;
    }
    v6->RemoveAll();
}

void ProcessLists::ReferDestructorNullOut(TESObjectREFR* apActor) {
    Actor* pFollower;
    uint32_t i;
    pCombatManager->ClearReferencePointers(apActor);
    if (apActor->IsActor()) {
        if ((&pPlayer->m_Extra)->GetFollowerExtra()) {
            if ((&pPlayer->m_Extra)->GetFollowerExtra()->pActorlist) {
                pFollower = static_cast<Actor*>(apActor);
                (&pPlayer->m_Extra)->GetFollowerExtra()->pActorlist->Remove(pFollower);
            }
        }
        if (pPlayer->pCurrentProcess) {
            BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
            CurrentAIProcess->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
        }
        if (pPlayer->GetDialogueItemTarget() == apActor) {
            pPlayer->SetDialogueItemTarget(0);
        }
    }
    TESObjectREFR* pprojectile__pexplosion__pprojectile = 0;
    for (i = 0;; ++i) {
        uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
        if (i >= ProcessLevelTail) {
            break;
        }
        pprojectile__pexplosion__pprojectile = MobProcessArray.GetItem(i);
        if (pprojectile__pexplosion__pprojectile && !pprojectile__pexplosion__pprojectile->IsStillLoading()) {
            Actor* pactor = 0;
            if (pprojectile__pexplosion__pprojectile->IsActor()) {
                pactor = static_cast<Actor*>(pprojectile__pexplosion__pprojectile);
            }
            if (pactor) {
                BaseProcess* pproc = pactor->pCurrentProcess;
                if (pactor->GetDialogueItemTarget() == apActor) {
                    pactor->SetDialogueItemTarget(0);
                    if (pproc) {
                        pproc->SetGreetingFlag(0);
                        pproc->ClearMovementStoped();
                    }
                }
                if (pproc) {
                    BaseProcess* v14 = pactor->pCurrentProcess;
                    TESPackage* pCurrentrunning = v14->GetPackageThatIsRunning();
                    TESPackage* pcurrentpack = pactor->GetPackageSetAsPcurrent();
                    if (pCurrentrunning) {
                        if (pCurrentrunning->GetIsCreated() && pCurrentrunning->GetPackType() != PACKAGE_TYPE_COMBAT) {
                            if (pCurrentrunning->GetPackageTarget()) {
                                if (pCurrentrunning->GetPackageTarget()->GetTargReference() == apActor) {
                                    pCurrentrunning->SetPackageTarget(0);
                                }
                            }
                            if (pCurrentrunning->GetLocationReference(pactor) == apActor) {
                                pCurrentrunning->SetPackageLocation(0);
                            }
                        }
                    }
                    if (pcurrentpack) {
                        if (pcurrentpack->GetIsCreated() && pcurrentpack->GetPackType() != PACKAGE_TYPE_COMBAT) {
                            if (pcurrentpack->GetPackageTarget()) {
                                if (pcurrentpack->GetPackageTarget()->GetTargReference() == apActor) {
                                    pcurrentpack->SetPackageTarget(0);
                                }
                            }
                            if (pcurrentpack->GetLocationReference(pactor) == apActor) {
                                pcurrentpack->SetPackageLocation(0);
                            }
                        }
                    }
                    uint32_t v3 = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
                    if (i < v3) {
                        TESPackage* v4;
                        pproc->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
                        if (pactor->GetCurrentPackageTarget() == apActor ||
                            (pproc->GetRunOncePackage() &&
                             (pproc->GetRunOncePackage()->GetPackageTarget() &&
                              (v4 = pproc->GetRunOncePackage(),
                               v4->GetPackageTarget()->GetTargReference() == apActor)))) {
                            if (pactor->IsinDialogue()) {
                                pactor->EndDialogue();
                            }
                            else {
                                TESPackage* CurrentPackage;
                                if (pactor->GetCurrentPackage() &&
                                    (CurrentPackage = pactor->GetCurrentPackage(), CurrentPackage->GetIsCreated())) {
                                    pactor->EndInterruptPackage(0);
                                }
                                else {
                                    pproc->ClearCurrentPackage();
                                    pproc->ClearRunOncePackage();
                                }
                            }
                        }
                        MagicCaster* pcast = apActor->GetMagicCaster();
                        pactor->CheckDeletedCaster(pcast);
                        uint32_t v6 = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
                        if (i < v6) {
                            pproc->RemoveHeadTrackTarget(apActor);
                        }
                    }
                    pproc->ClearObjectFromAcquireList(apActor);
                    pproc->ClearPathLookTarget(apActor);
                    ExtraPackage* pextrapack = static_cast<ExtraPackage*>((&pactor->m_Extra)->GetExtraData(25));
                    if (pextrapack) {
                        if (pextrapack->pTarg == apActor) {
                            pextrapack->pTarg = 0;
                        }
                    }
                    if (pactor->pMyKiller == apActor) {
                        pactor->pMyKiller = 0;
                    }
                    if (pactor->IsFleeing(0)) {
                        TESPackage* pbasepack = pactor->GetCurrentPackage();
                        TESPackage* pbasepack_2 = 0;
                        if (pbasepack) {
                            if (pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                                pbasepack_2 = pbasepack;
                            }
                        }
                        if (pbasepack_2) {
                            if (static_cast<FleePackage*>(pbasepack_2)->pRefFleeTo == apActor) {
                                static_cast<FleePackage*>(pbasepack_2)->SetRefFleeTo(0);
                            }
                        }
                        pactor->RemoveFleeTarget(apActor);
                    }
                    if (pactor->IsInCombat()) {
                        CombatController* pcombat = pactor->GetCombatController();
                        if (pcombat) {
                            pcombat->RemoveTarget(static_cast<Actor*>(apActor));
                        }
                    }
                    (&pactor->m_Extra)->RemoveFollower(static_cast<Actor*>(apActor));
                    if (pproc->GetTarget() == apActor) {
                        pproc->SetTarget(0);
                    }
                    if (pactor->GetCurrentTarget() == apActor) {
                        pactor->SetCurrentTarget(0);
                    }
                }
            }
            else {
                if (pprojectile__pexplosion__pprojectile->IsProjectile()) {
                    Projectile* pprojectile = static_cast<Projectile*>(pprojectile__pexplosion__pprojectile);
                    static_cast<Projectile*>(pprojectile__pexplosion__pprojectile)
                        ->Projectile::CheckRefDeleted(apActor);
                    if (pprojectile->GetDisabled()) {
                        Projectile* v7 = static_cast<Projectile*>(MobProcessArray.GetItem(i));
                        if (v7 != pprojectile) {
                            --i;
                        }
                    }
                }
                else {
                    if (pprojectile__pexplosion__pprojectile->IsExplosion()) {
                        Explosion* v28 = static_cast<Explosion*>(pprojectile__pexplosion__pprojectile);
                        static_cast<Explosion*>(pprojectile__pexplosion__pprojectile)
                            ->Explosion::CheckRefDeleted(apActor);
                    }
                }
            }
        }
    }
    for (BSSimpleList<void*>* plist = &TempShouldMoveList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor_2 = 0;
        if (static_cast<TESObjectREFR*>(plist->m_item)->IsActor()) {
            pactor_2 = static_cast<Actor*>(static_cast<TESObjectREFR*>(plist->m_item));
        }
        if (!pactor_2 || pactor_2->IsStillLoading()) {
            if (pprojectile__pexplosion__pprojectile->IsProjectile()) {
                static_cast<Projectile*>(pprojectile__pexplosion__pprojectile)->Projectile::CheckRefDeleted(apActor);
            }
            else {
                if (pprojectile__pexplosion__pprojectile->IsExplosion()) {
                    static_cast<Explosion*>(pprojectile__pexplosion__pprojectile)->Explosion::CheckRefDeleted(apActor);
                }
            }
        }
        else {
            BaseProcess* pproc_2 = pactor_2->pCurrentProcess;
            if (pactor_2->GetDialogueItemTarget() == apActor) {
                pactor_2->SetDialogueItemTarget(0);
                if (pproc_2) {
                    pproc_2->SetGreetingFlag(0);
                    pproc_2->ClearMovementStoped();
                }
            }
            if (pproc_2) {
                BaseProcess* v12 = pactor_2->pCurrentProcess;
                TESPackage* pCurrentrunning_2 = v12->GetPackageThatIsRunning();
                TESPackage* pcurrentpack_2 = pactor_2->GetPackageSetAsPcurrent();
                if (pCurrentrunning_2) {
                    if (pCurrentrunning_2->GetIsCreated() && pCurrentrunning_2->GetPackType() != PACKAGE_TYPE_COMBAT) {
                        if (pCurrentrunning_2->GetPackageTarget()) {
                            if (pCurrentrunning_2->GetPackageTarget()->GetTargReference() == apActor) {
                                pCurrentrunning_2->SetPackageTarget(0);
                            }
                        }
                        if (pCurrentrunning_2->GetLocationReference(pactor_2) == apActor) {
                            pCurrentrunning_2->SetPackageLocation(0);
                        }
                    }
                }
                if (pcurrentpack_2) {
                    if (pcurrentpack_2->GetIsCreated() && pcurrentpack_2->GetPackType() != PACKAGE_TYPE_COMBAT) {
                        if (pcurrentpack_2->GetPackageTarget()) {
                            if (pcurrentpack_2->GetPackageTarget()->GetTargReference() == apActor) {
                                pcurrentpack_2->SetPackageTarget(0);
                            }
                        }
                        if (pcurrentpack_2->GetLocationReference(pactor_2) == apActor) {
                            pcurrentpack_2->SetPackageLocation(0);
                        }
                    }
                }
                if (i < 2) {
                    TESPackage* v10;
                    pproc_2->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
                    if (pactor_2->GetCurrentPackageTarget() == apActor ||
                        (pproc_2->GetRunOncePackage() &&
                         (pproc_2->GetRunOncePackage()->GetPackageTarget() &&
                          (v10 = pproc_2->GetRunOncePackage(),
                           v10->GetPackageTarget()->GetTargReference() == apActor)))) {
                        if (pactor_2->IsinDialogue()) {
                            pactor_2->EndDialogue();
                        }
                        else {
                            TESPackage* v11 = pactor_2->GetCurrentPackage();
                            if (v11->GetIsCreated()) {
                                pactor_2->EndInterruptPackage(0);
                            }
                            else {
                                pproc_2->ClearCurrentPackage();
                                pproc_2->ClearRunOncePackage();
                            }
                        }
                    }
                    MagicCaster* pcast_2 = apActor->GetMagicCaster();
                    pactor_2->CheckDeletedCaster(pcast_2);
                    if (!i) {
                        pproc_2->RemoveHeadTrackTarget(apActor);
                    }
                }
                pproc_2->ClearObjectFromAcquireList(apActor);
                ExtraPackage* pextrapack_2 = static_cast<ExtraPackage*>((&pactor_2->m_Extra)->GetExtraData(25));
                if (pextrapack_2) {
                    if (pextrapack_2->pTarg == apActor) {
                        pextrapack_2->pTarg = 0;
                    }
                }
                if (pactor_2->pMyKiller == apActor) {
                    pactor_2->pMyKiller = 0;
                }
                if (pactor_2->IsFleeing(0)) {
                    TESPackage* pbasepack_3 = pactor_2->GetCurrentPackage();
                    TESPackage* pbasepack_4 = 0;
                    if (pbasepack_3) {
                        if (pbasepack_3->GetPackType() == PACKAGE_TYPE_FLEE) {
                            pbasepack_4 = pbasepack_3;
                        }
                    }
                    if (pbasepack_4) {
                        if (static_cast<FleePackage*>(pbasepack_4)->pRefFleeTo == apActor) {
                            static_cast<FleePackage*>(pbasepack_4)->SetRefFleeTo(0);
                        }
                    }
                    pactor_2->RemoveFleeTarget(apActor);
                }
                if (pactor_2->IsInCombat()) {
                    CombatController* pcombat_2 = pactor_2->GetCombatController();
                    if (pcombat_2) {
                        pcombat_2->RemoveTarget(static_cast<Actor*>(apActor));
                    }
                }
                (&pactor_2->m_Extra)->RemoveFollower(static_cast<Actor*>(apActor));
                if (pproc_2->GetTarget() == apActor) {
                    pproc_2->SetTarget(0);
                }
                if (pactor_2->GetCurrentTarget() == apActor) {
                    pactor_2->SetCurrentTarget(0);
                }
            }
        }
    }
    RemoveActorCloseToPlayer(static_cast<Actor*>(apActor));
}

void ProcessLists::DisableExcessDeadActors(float afDeltaTime) {
    fRemoveExcessDeadTimer = fRemoveExcessDeadTimer - afDeltaTime;
    if (fRemoveExcessDeadTimer <= 0.0) {
        int32_t* v3;
        int32_t iactorsTotal;
        float v9;
        uint32_t v23 = 0;
        uint32_t v24 = 0;
        BSSimpleList<Actor*> deadActors;
        TESForm* v20 = 0;
        for (uint32_t uiIndex = 0;; ++uiIndex) {
            Actor* pactor_item;
            uint32_t ProcessLevelTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
            if (uiIndex >= ProcessLevelTail) {
                break;
            }
            Actor* pactor = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
            pactor_item = 0;
            if (pactor) {
                if (pactor->IsActor() && !pactor->IsStillLoading()) {
                    ++v24;
                    pactor_item = pactor;
                    if (pactor->GetLifeState() == ACTOR_LIFE_STATE_DEAD) {
                        BaseProcess* pprocess = pactor_item->pCurrentProcess;
                        if (pprocess) {
                            if (!pprocess->eLevel && pprocess->GetFadeState() == BaseProcess::FADE_NORMAL &&
                                !pactor_item->IsQuestObject() && !pactor_item->HasQuestObject() &&
                                !pactor_item->TESObjectREFR::IsDead(0) && pprocess->GetGreetingTimer() < 0.0 &&
                                (!pactor_item->GetEnableStateChildList() ||
                                 pactor_item->GetEnableStateChildList()->IsEmpty())) {
                                ++v23;
                                deadActors.AddTail(pactor_item);
                            }
                        }
                    }
                }
            }
        }
        bool bcomplex = Main::GetSceneIsComplex();
        if (v24 < 25) {
            bcomplex = 0;
        }
        if (bcomplex) {
            v9 = gs_fRemoveExcessComplexDeadTime.Float();
        }
        else {
            v9 = fRemoveExcessDeadTime.Float();
        }
        fRemoveExcessDeadTimer = v9;
        if (bcomplex) {
            v3 = &iRemoveExcessDeadComplexCount.Int();
            int32_t v4 = *v3;
        }
        else {
            v3 = &iRemoveExcessDeadCount.Int();
            int32_t v5 = *v3;
        }
        uint32_t v25 = *v3;
        if (bcomplex) {
            iactorsTotal = iRemoveExcessDeadComplexTotalActorCount.Int();
        }
        else {
            iactorsTotal = iRemoveExcessDeadTotalActorCount.Int();
        }
        uint32_t v27 = iactorsTotal;
        if (v23 >= v25 && v24 >= v27) {
            if (bcomplex) {
                for (BSSimpleList<Actor*>* plist = &deadActors; plist && (!plist->IsEmpty() && v23 >= v25);
                     plist = plist->m_pkNext) {
                    Actor* pactor_2 = plist->m_item;
                    if (pactor_2) {
                        static_cast<HighProcess*>(pactor_2->pCurrentProcess)->HighProcess::FadeAndDisable(pactor_2);
                        --v23;
                    }
                }
            }
            else {
                Actor* poldestDeadActor = 0;
                float foldestDeadTime = 3.4028235e+38f;
                for (BSSimpleList<Actor*>* plist_2 = &deadActors; plist_2 && !plist_2->IsEmpty();
                     plist_2 = plist_2->m_pkNext) {
                    Actor* pactor_3 = plist_2->m_item;
                    if (pactor_3) {
                        BaseProcess* v7;
                        if (!poldestDeadActor ||
                            (v7 = pactor_3->pCurrentProcess,
                             static_cast<double>(foldestDeadTime) > v7->GetGreetingTimer())) {
                            poldestDeadActor = pactor_3;
                            BaseProcess* CurrentAIProcess = pactor_3->pCurrentProcess;
                            foldestDeadTime = CurrentAIProcess->GetGreetingTimer();
                        }
                    }
                }
                if (poldestDeadActor) {
                    static_cast<HighProcess*>(poldestDeadActor->pCurrentProcess)
                        ->HighProcess::FadeAndDisable(poldestDeadActor);
                }
            }
        }
        deadActors.RemoveAll();
    }
}

void ProcessLists::UnloadAllActorPermanentMagic() {
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        TESForm* v6 = MobProcessArray.GetItem(uiIndex);
        Actor* v4 = 0;
        if (v6) {
            if (v6->IsActor()) {
                v4 = static_cast<Actor*>(v6);
            }
        }
        if (v4) {
            v4->UnloadPermanentMagic();
        }
    }
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH); ++i) {
        TESForm* v7 = MobProcessArray.GetItem(i);
        Actor* v2 = 0;
        if (v7) {
            if (v7->IsActor()) {
                v2 = static_cast<Actor*>(v7);
            }
        }
        if (v2) {
            v2->UnloadPermanentMagic();
        }
    }
    if (pPlayer) {
        pPlayer->UnloadPermanentMagic();
    }
}

void ProcessLists::RebuildActorsCloseToPlayer() {
    kNearbyActorLock.Lock(0);
    iActorsCloseToPlayer = 0;
    fillchar(pActorsCloseToPlayerA, 0, 200u);
    kNearbyActorLock.Unlock();
    Actor* v5 = 0;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        TESForm* v4 = MobProcessArray.GetItem(uiIndex);
        if (v4) {
            if (v4->IsActor()) {
                v5 = static_cast<Actor*>(v4);
            }
        }
        if (v5) {
            float DistanceFromReference = v5->GetDistanceFromReference(pPlayer, 0, 0);
            if (fShaderShadowUpdateDistance.Float() > static_cast<double>(DistanceFromReference)) {
                ReferenceProcessLists.InsertActorCloseToPlayer(v5);
            }
        }
    }
    ReferenceProcessLists.SortActorsCloseToPlayer();
}

void ProcessLists::UpdateHighListAnimation(float afDelta, bool abSleeping) {
    for (uint32_t uiIndex = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        MobileObject* v5 = MobProcessArray.GetItem(uiIndex);
        if (v5) {
            if (v5->IsActor()) {
                v5->UpdateAnimation(afDelta);
            }
        }
    }
}

void ProcessLists::RemoveReferenceMuzzleFlash(TESObjectREFR* apParentRef) {
    MuzzleFlash* pflash;
    if (!ReferenceMuzzleFlashList.IsEmpty()) {
        BSSimpleList<void*>* plist = &ReferenceMuzzleFlashList;
        bool bfirst = 1;
        while (plist) {
            pflash = static_cast<MuzzleFlash*>(plist->m_item);
            if (!bfirst) {
                plist = plist->m_pkNext;
            }
            bool bremove = 0;
            if (pflash) {
                if (pflash->pParentReference == apParentRef) {
                    ReferenceMuzzleFlashList.Remove(*reinterpret_cast<void**>(&pflash));
                    if (pflash) {
                        delete pflash;
                    }
                    bremove = 1;
                }
            }
            if (!bremove && bfirst) {
                plist = plist->m_pkNext;
                bfirst = 0;
            }
        }
    }
}

void ProcessLists::SpawnReferenceMuzzleFlash(Projectile* apProjectile, TESObjectREFR* apParentRef) {
    MuzzleFlash* pcurrentFlash;
    if (apParentRef) {
        if (apProjectile) {
            pcurrentFlash = 0;
            for (BSSimpleList<void*>* plist = &ReferenceMuzzleFlashList; !pcurrentFlash && (plist && !plist->IsEmpty());
                 plist = plist->m_pkNext) {
                MuzzleFlash* pflash = static_cast<MuzzleFlash*>(plist->m_item);
                if (pflash) {
                    BGSProjectile* Time = pflash->pProjectile;
                    if (Time == apProjectile->GetProjectileBase()) {
                        TESObjectWEAP* Count = pflash->pWeaponSource;
                        if (Count == apProjectile->pWeaponSource && pflash->pParentReference == apParentRef) {
                            pcurrentFlash = pflash;
                        }
                    }
                }
            }
            if (!pcurrentFlash) {
                MuzzleFlash* pcurrentFlash_2;
                MuzzleFlash* apMemory = static_cast<MuzzleFlash*>(ourMalloc(sizeof(MuzzleFlash)));
                if (apMemory) {
                    pcurrentFlash_2 = ::new (apMemory) MuzzleFlash(apProjectile, apParentRef);
                }
                else {
                    pcurrentFlash_2 = 0;
                }
                pcurrentFlash = pcurrentFlash_2;
                apParentRef->SetTargeted(1);
                void* pFlashItem = pcurrentFlash;
                ReferenceMuzzleFlashList.AddHead(pFlashItem);
            }
            if (pcurrentFlash) {
                pcurrentFlash->Enable();
            }
        }
    }
}

// aeType 5 (CRIME_TYPE_COUNT), one past the lists, is deliberately accepted, as the game does: it then reads the
// pointer after GlobalCrimeListArray, the head of GlobalTempEffectList.
Crime* ProcessLists::GetCrimeinGlobalCrimeList(PackageNS::CRIME_TYPE aeType, int32_t aiCrime) {
    if (aeType <= PackageNS::CRIME_TYPE_NONE || aeType > PackageNS::CRIME_TYPE_COUNT) {
        return 0;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    Crime* preturncrime = 0;
    while (pitem && (pitem->m_item && !preturncrime)) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->iCrimeNumber == aiCrime) {
            preturncrime = pcrime;
        }
    }
    return preturncrime;
}

// Accepts aeType 5, one past the lists, as the other overload does; this is deliberate, as the game does.
Crime*
ProcessLists::GetCrimeinGlobalCrimeList(Actor* apCriminal, TESObjectREFR* apVictim, PackageNS::CRIME_TYPE aeType) {
    if (aeType <= PackageNS::CRIME_TYPE_NONE || aeType > PackageNS::CRIME_TYPE_COUNT) {
        return 0;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    Crime* preturncrime = 0;
    while (pitem && (pitem->m_item && !preturncrime)) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->pcriminal == apCriminal && pcrime->pcrimetarget == apVictim) {
            preturncrime = pcrime;
        }
    }
    return preturncrime;
}

uint16_t ProcessLists::GetCrimeIndex(PackageNS::CRIME_TYPE aeType, Crime* apCrime) {
    uint16_t v5 = 0;
    for (BSSimpleList<Crime*>* ecx0 = GlobalCrimeListArray[aeType]; ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
        if (apCrime == ecx0->m_item) {
            return v5;
        }
        ++v5;
    }
    Warning("AI: When trying to get a crime index, the crime was not found in the crime lists.");
    return 0;
}

void ProcessLists::FinishMagicShaderHitEffectAll(TESEffectShader* apShader) {
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect) {
                if (NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                    pshaderEffect->pEffectData == apShader) {
                    pshaderEffect->SetFinished(1);
                }
            }
        }
    }
}

void ProcessLists::FinishMagicModelHitEffect(TESObjectREFR* apEffectTarget, char* apFilename) {
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicModelHitEffect* psmodelEffect = static_cast<MagicModelHitEffect*>(pelist->m_item.m_pObject);
            if (psmodelEffect) {
                if (NiObject::IsKindOf_0(&MagicModelHitEffect::ms_RTTI, psmodelEffect) &&
                    psmodelEffect->pTarget == apEffectTarget && !BSstrcmp(psmodelEffect->pFilename, apFilename)) {
                    psmodelEffect->bFinished = 1;
                }
            }
        }
    }
}

void ProcessLists::CommentOnPlayerAction(PLAYER_ACTION eaction, TESObjectREFR* apObject, bool abMakecomment) {
    if (eaction) {
        int32_t iTopicIndex;
        bool bDetected;
        iTopicIndex = 0;
        if (fPlayerActionCommentTimer <= 0.0 || abMakecomment) {
            Actor* pTalk = 0;
            bDetected = 0;
            int32_t idistance = 350;
            if (eaction == PLAYER_ACTION_IRON_SITES) {
                iTopicIndex = 8;
                pTalk = pPlayer->GetCombatTarget();
                if (pTalk) {
                    if (pTalk->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                        (!pTalk->CanSpeak() ||
                         (pTalk->IsDead(0) ||
                          (pTalk->IsInCombat() ||
                           ((pTalk->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS) &&
                             !pTalk->bPlayerTeammate) ||
                            static_cast<double>(350) < pTalk->GetDistanceFromReference(pPlayer, 0, 0)))))) {
                        pTalk = 0;
                    }
                    else {
                        if (pTalk->GetIsTalking()) {
                            BaseProcess* pBaseProcess = pTalk->pCurrentProcess;
                            if (pBaseProcess->GetGreetingFlag() &&
                                pTalk->GetDetectionLevelAgainstActor(0, pPlayer, bDetected, 0, 0, 0, 0) <= 0) {
                                pTalk = 0;
                            }
                        }
                    }
                }
            }
            else {
                kNearbyActorLock.Lock(0);
                int32_t icurActor = 0;
                while (icurActor < iActorsCloseToPlayer && !pTalk) {
                    bool bisTeammate;
                    Actor* pact = pActorsCloseToPlayerA[icurActor];
                    bool bsetactor = 1;
                    if (pact) {
                        bisTeammate = pact->bPlayerTeammate;
                    }
                    else {
                        bisTeammate = 0;
                    }
                    if (pact) {
                        if (pact->pCurrentProcess) {
                            if (pact->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING && pact->CanSpeak() &&
                                !pact->GetIsTalking()) {
                                BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
                                if (!CurrentAIProcess->GetGreetingFlag() &&
                                    pact->GetDetectionLevelAgainstActor(0, pPlayer, bDetected, 0, 0, 0, 0) > 0 &&
                                    (!pact->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS) ||
                                     bisTeammate) &&
                                    static_cast<double>(idistance) > pact->GetDistanceFromReference(pPlayer, 0, 0) &&
                                    !pact->IsDead(0) && !pact->IsInCombat()) {
                                    if (eaction == PLAYER_ACTION_KNOCKING_OVER_OBJECTS ||
                                        eaction == PLAYER_ACTION_Z_KEY_OBJECT) {
                                        if (apObject &&
                                            (apObject->IsAnOwner(pact, 1) && !(fPlayerKnockObjectCommentTimer > 0.0))) {
                                            fPlayerKnockObjectCommentTimer = fCommentOnPlayerKnockingThings.Float();
                                        }
                                        else {
                                            bsetactor = 0;
                                        }
                                    }
                                    if (bsetactor) {
                                        pTalk = pact;
                                    }
                                }
                            }
                        }
                    }
                    if (++icurActor >= iActorsCloseToPlayer && (idistance < 1000 && !pTalk)) {
                        icurActor = 0;
                        idistance += 200;
                    }
                }
                kNearbyActorLock.Unlock();
            }
            switch (eaction) {
                case PLAYER_ACTION_SWING_MELEE_WEAPON: {
                    iTopicIndex = 1;
                    if (pPlayer->IsInCombat()) {
                        pTalk = 0;
                    }
                    break;
                }
                case PLAYER_ACTION_THROW_GRANADE: {
                    iTopicIndex = 2;
                    if (pPlayer->IsInCombat()) {
                        pTalk = 0;
                    }
                    break;
                }
                case PLAYER_ACTION_FIRE_WEAPON: {
                    iTopicIndex = 3;
                    if (pPlayer->IsInCombat() || (pTalk && pTalk->bPlayerTeammate)) {
                        pTalk = 0;
                    }
                    break;
                }
                case PLAYER_ACTION_LAY_MINE: {
                    iTopicIndex = 4;
                    break;
                }
                case PLAYER_ACTION_Z_KEY_OBJECT: {
                    iTopicIndex = 5;
                    break;
                }
                case PLAYER_ACTION_JUMPING: {
                    iTopicIndex = 6;
                    break;
                }
                case PLAYER_ACTION_KNOCKING_OVER_OBJECTS: {
                    iTopicIndex = 7;
                    break;
                }
                case PLAYER_ACTION_STAND_ON_TABLE_CHAIR: {
                    iTopicIndex = 10;
                    break;
                }
                case PLAYER_ACTION_DESTROY_OBJECT: {
                    iTopicIndex = 9;
                    break;
                }
                case PLAYER_ACTION_LOCKED_OBJECT: {
                    iTopicIndex = 13;
                    break;
                }
                case PLAYER_ACTION_PICKPOCKET: {
                    iTopicIndex = 14;
                    break;
                }
                default: {
                    break;
                }
            }
            if (pTalk) {
                TESTopic* pTopic = TESTopic::GetTopic(DT_TOPIC, iTopicIndex);
                pTalk->StartGreetingPlayer(pTopic);
                fPlayerActionCommentTimer = fCommentOnPlayerActionsTimer.Float();
            }
        }
    }
}

void ProcessLists::UpdatePlayerFollowers() {
    Actor* ptarget;
    ExtraFollower* pfollower = (&pPlayer->m_Extra)->GetFollowerExtra();
    if (pfollower) {
        BSSimpleList<Actor*>* pHead;
        BSSimpleList<Actor*>* pTempList;
        BSSimpleList<Actor*>* v3 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
        if (v3) {
            pTempList = ::new (v3) BSSimpleList<Actor*>;
        }
        else {
            pTempList = 0;
        }
        BSSimpleList<Actor*>* v2 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
        if (v2) {
            pHead = ::new (v2) BSSimpleList<Actor*>;
        }
        else {
            pHead = 0;
        }
        BSSimpleList<Actor*>* plist = pHead;
        for (BSSimpleList<Actor*>* ptemp = pfollower->pActorlist; ptemp && ptemp->m_item; ptemp = ptemp->m_pkNext) {
            pHead->AddTail(ptemp->m_item);
        }
        while (plist && plist->m_item) {
            ptarget = plist->m_item;
            if (ptarget) {
                if (ptarget != pPlayer) {
                    if (ptarget->pCurrentProcess) {
                        TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                        TESPackage* pEditorpack = (&ptarget->m_Extra)->GetPackageExtra();
                        if (pEditorpack) {
                            ptargpack = pEditorpack;
                        }
                        if (ptargpack &&
                            (ptargpack->GetPackType() == PACKAGE_TYPE_FOLLOW ||
                             ptargpack->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
                            if (ptarget->GetCurrentProcessType()) {
                                ptarget->StopCombat(0);
                                ptarget->EndInterruptPackage(0);
                                ptarget->Process(3600.0f);
                            }
                        }
                        else {
                            pTempList->AddHead(ptarget);
                        }
                    }
                }
            }
            plist = plist->m_pkNext;
        }
        for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
            Actor* apActor = plist->m_item;
            (&pPlayer->m_Extra)->RemoveFollower(apActor);
        }
        pTempList->RemoveAll();
        if (pTempList) {
            delete pTempList;
        }
        pHead->RemoveAll();
        if (pHead) {
            delete pHead;
        }
        if (pfollower->pActorlist->IsEmpty()) {
            (&pPlayer->m_Extra)->RemoveFollowerExtra();
        }
    }
}

int32_t ProcessLists::DistanceListCompareFn(TESObjectREFR* apCompare1, TESObjectREFR* apCompare2) {
    float distanceFromReference = (float)apCompare1->GetDistanceFromReference(pPlayer, 0, 0);
    float v5 = (float)apCompare2->GetDistanceFromReference(pPlayer, 0, 0);
    if (!(v5 > ((double)distanceFromReference))) {
        return (int32_t)((uint8_t)(v5 < ((double)distanceFromReference)));
    }
    else {
        return (-1);
    }
}

void ProcessLists::DeleteAttachedArrows(TESBoundObject* a1, int32_t a2, Actor* a3, uint8_t a4, uint8_t a5) {
    int32_t icount = 0;
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = 0; icount < a2 && i < parray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(parray->GetItem(i));
        if (parrow) {
            if (parrow->GetAttachedRef() == a3 && parrow->GetObjectReference() == a1 &&
                (!a5 || parrow->GetAddedToInventory())) {
                if (a4) {
                    delete parrow;
                }
                else {
                    parrow->SetState(ArrowProjectile::STATE_DELETING);
                }
                ++icount;
            }
        }
    }
    for (uint32_t i_2 = parray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         icount < a2 && i_2 < parray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH); ++i_2) {
        ArrowProjectile* parrow_2 = dynamic_cast<ArrowProjectile*>(parray->GetItem(i_2));
        if (parrow_2) {
            if (parrow_2->GetAttachedRef() == a3 && parrow_2->GetObjectReference() == a1 &&
                (!a5 || parrow_2->GetAddedToInventory())) {
                delete parrow_2;
                ++icount;
            }
        }
    }
}

void bhkRagdollController::ReinitRBControllerOnUpdate(bool abReinit) {
    bReinitRBController = abReinit;
}

void BSTempEffect::RemoveAllDecals() {
    kDecals.RemoveAll();
    iNumDecals = 0;
}

void BSTempEffectGeometryDecal::SetFinished(bool abFinished) {
    bFinished = abFinished;
}

uint8_t PlayerCharacter::GetActorInSneakRange() {
    return bActorinSneakRange;
}
