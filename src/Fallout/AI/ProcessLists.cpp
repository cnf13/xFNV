#include "Fallout/AI/ProcessLists.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
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
#include "BSMain/XINISetting.h"
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
#include "SDK/MSVC/rtti.h"

#include <bit>
#include <limits>

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

void UpdateAnimMovement(void* apData, uint32_t aiIndex) {
    Actor* pActor = (*static_cast<BSScrapArray<Actor*>*>(apData))[aiIndex];
    pActor->UpdateAnimationMovementNoWorldUpdate();
}

void UpdateTempEffect(void* apData, uint32_t aiIndex) {
    EffectTaskData& rData = (*static_cast<BSScrapArray<EffectTaskData, 1024>*>(apData))[aiIndex];
    if (!rData.pTempEffect->Update(rData.fDeltaTime)) {
        rData.bRemove = true;
    }
}

void* fillchar(void* apDest, int32_t aiValue, uint32_t aCount) {
    return ourMemset(apDest, aiValue, aCount);
}

PROCESS_TYPE MobIterOperator::QProcessType() {
    return eType;
}

// The other fields are deliberately left unset; kLock_10340 is only cleared, not constructed.
ProcessLists::ProcessLists()
  : MobProcessArray(), GlobalTempEffectList(), MagicEffectList(), ReferenceMuzzleFlashList(),
    ProjectilePostProcessList(), TempShouldMoveList(), AliveActorList(), fPlayerActionCommentTimer(0.0f),
    fPlayerKnockObjectCommentTimer(0.0f), bPlayerInRadiatedWater(false), bPlayerInRadiationArea(false), TaskManagerAI(),
    DialogPackageList(), kNearbyActorLock(), kUpdateLock(), kLipSyncManager(), bRunScheduals(true), bRunDetection(true),
    bShowDetectionStats(false), pStatdetect(nullptr), bProcessHigh(true), bProcessLow(true), bProcessMHigh(true),
    bProcessMLow(true), bProcessSche(true), bShowSubtitle(false), bUpdatingLowList(false), iNumberHighActors(0),
    fCrimeUpdateTimer(0.0f), iCrimeNumber(0), fRemoveExcessDeadTimer(fRemoveExcessDeadComplexTime.Float()),
    MovementSyncSema(0, 40) {
    kLock_10340.uiOwningThread = 0;
    kLock_10340.uiLockCount = 0;
    // Clears the first list pointer only.
    GlobalCrimeListArray[0] = nullptr;
    kNearbyActorLock.Lock(nullptr);
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
            Actor* pactor = nullptr;
            if (pMob && pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }

            if (pactor) {
                TESObjectCELL* pCell;
                if (!pactor->Get3D() ||
                    (pactor->GetParentCell() &&
                     (pCell = pactor->GetParentCell(), TES::pInstance->IsCellLoaded(pCell, false)))) {
                    if (!pactor->Get3D() &&
                        !pModelLoader->IsReferenceQueued(pactor) &&
                        !pactor->GetDisabled() &&
                        !pactor->GetDelete() &&
                        pactor->GetParentCell()) {
                        TESObjectCELL* pParentCell = pactor->GetParentCell();
                        if (TES::pInstance->IsCellLoaded(pParentCell, true)) {
                            pactor->GetParentCell()->AddReference(pactor, false);
                            i = 0;
                        }
                    }
                }
                else {
                    pactor->Set3D(nullptr, false);
                }
            }
        }
        TES::pInstance->CreateFurnitureList();
        PlaceActorsinBedsOrChairs();
    }
}

void ProcessLists::PrintActorsInDialog() {}

// The bound deliberately lets crime type 5 through, one past GlobalCrimeListArray's five entries: its
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
        return false;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    while (pitem && pitem->m_item) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->ecrime == aeType &&
            pcrime->pcriminal == apVictim &&
            (pcrime->pcrimetarget == apTarget || !apTarget) &&
            (aiCrimeNumber == -1 || aiCrimeNumber == pcrime->iCrimeNumber) &&
            (!apAwareCrime || pcrime->DoesActorKnowaboutCrime(apAwareCrime))) {
            return true;
        }
    }
    return false;
}

bool ProcessLists::IsActorTalkingThroughActivator(Actor* apAct) {
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsTalkingActivator()) {
            TESBoundObject* pObjectRef = pMob->TESObjectREFR::GetObjectReference();
            bool bSpeaker = static_cast<const void*>(static_cast<TESObjectCONT*>(pObjectRef)->GetCloseSound()) ==
                static_cast<const void*>(apAct);
            if (bSpeaker) {
                return true;
            }
        }
    }
    return false;
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
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pNode = &MagicEffectList; pNode; pNode = pNode->m_pkNext) {
            MagicShaderHitEffect* pEffect = static_cast<MagicShaderHitEffect*>(pNode->m_item.m_pObject);
            if (pEffect &&
                NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pEffect) &&
                pEffect->pTarget == apEffectTarget &&
                pEffect->pEffectData == apShader) {
                pEffect->SetFinished(true);
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
    uint8_t ctempEffectType;
    uint16_t icount = 0;
    BSSimpleList<NiPointer<BSTempEffect>>* plist;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    for (plist = &GlobalTempEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect->CheckShouldSave()) {
            ctempEffectType = ptempEffect->GetType();
            pSaveLoadGameOLD->SaveGameDataOLD(&ctempEffectType, sizeof(ctempEffectType));
            ptempEffect->SaveGame();
            ++icount;
        }
    }
    for (plist = &MagicEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        BSTempEffect* ptempEffect = plist->m_item.m_pObject;
        if (ptempEffect->CheckShouldSave()) {
            ctempEffectType = ptempEffect->GetType();
            pSaveLoadGameOLD->SaveGameDataOLD(&ctempEffectType, sizeof(ctempEffectType));
            ptempEffect->SaveGame();
            ++icount;
        }
    }
    *pcount = icount;
}

void ProcessLists::InitActorLeveledItems() {
    for (int32_t i = 0; i < 2; ++i) {
        uint32_t uiHead;
        PROCESS_TYPE eLevel = PROCESS_TYPE_LOW;
        if (i) {
            if (i == 1) {
                eLevel = PROCESS_TYPE_MIDDLE_LOW;
            }
            uiHead = MobProcessArray.GetProcessLevelHead(eLevel);
        }
        else {
            eLevel = PROCESS_TYPE_LOW;
            uiHead = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
        }
        for (uint32_t uiIndex = uiHead; uiIndex < MobProcessArray.GetProcessLevelTail(eLevel); ++uiIndex) {
            Actor* pActor = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
            if (pActor && pActor->IsActor() && !pActor->IsDead(false)) {
                pActor->ResetInventory(true);
            }
        }
    }
}

void ProcessLists::InsertActorCloseToPlayer(Actor* apActor) {
    if (apActor) {
        bhkCharacterController* pcon = apActor->GetCharController();
        if (pcon) {
            float fdist = pcon->GetLODDistance();
            kNearbyActorLock.Lock(nullptr);
            bool ballreadyExists = false;
            for (int32_t icurActor = 0; icurActor < iActorsCloseToPlayer; ++icurActor) {
                if (pActorsCloseToPlayerA[icurActor] == apActor) {
                    ballreadyExists = true;
                    break;
                }
            }
            if (!ballreadyExists) {
                int32_t ilocToInsert;
                for (ilocToInsert = 0; ilocToInsert < iActorsCloseToPlayer; ++ilocToInsert) {
                    bhkCharacterController* potherCon = pActorsCloseToPlayerA[ilocToInsert]->GetCharController();
                    if (potherCon && static_cast<double>(fdist) < potherCon->GetLODDistance()) {
                        break;
                    }
                }
                if (ilocToInsert < 50 && ilocToInsert < iAINumberActorsComplexScene.Int()) {
                    BSmemmove(
                        &pActorsCloseToPlayerA[ilocToInsert + 1],
                        sizeof(Actor*) * 50,
                        &pActorsCloseToPlayerA[ilocToInsert],
                        sizeof(Actor*) * (49 - ilocToInsert)
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
    kNearbyActorLock.Lock(nullptr);
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
        pActorsCloseToPlayerA[iActorsCloseToPlayer - 1] = nullptr;
        --iActorsCloseToPlayer;
    }
    kNearbyActorLock.Unlock();
}

void ProcessLists::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (BSSimpleList<Crime*>* pCrimeList : GlobalCrimeListArray) {
        for (BSSimpleList<Crime*>* pIter = pCrimeList; pIter;) {
            Crime* pCrime = pIter->GetItem();
            if (pCrime) {
                pCrime->InitItem(apLoadGameBuffer);
            }
            pIter = pIter->GetNext();
        }
    }
    CalculateAliveActorList();
}

void ProcessLists::FinishGeometryDecalEffects() {
    if (GlobalTempEffectList.IsEmpty()) {
        return;
    }
    for (BSSimpleList<NiPointer<BSTempEffect>>* pIter = &GlobalTempEffectList; pIter;) {
        BSTempEffect* pEffect = pIter->GetItem();
        if (pEffect) {
            if (NiObject::IsKindOf(&BSTempEffectGeometryDecal::ms_RTTI, pEffect)) {
                static_cast<BSTempEffectGeometryDecal*>(pEffect)->SetFinished(true);
            }
        }
        pIter = pIter->GetNext();
    }
}

Crime* ProcessLists::GetCrimeByIndex(PackageNS::CRIME_TYPE aeType, uint16_t aiCrimeIndex) {
    uint16_t usIndex = 0;
    for (BSSimpleList<Crime*>* pIter = GlobalCrimeListArray[aeType]; pIter;) {
        if (pIter->IsEmpty()) {
            break;
        }
        if (usIndex == aiCrimeIndex) {
            return pIter->GetItem();
        }
        ++usIndex;
        pIter = pIter->GetNext();
    }
    Warning("AI: When trying to get a crime by its index, the index was larger than the size of the crime list.");
    return nullptr;
}

void ProcessLists::CalculateAliveActorList() {
    AliveActorList.RemoveAll();
    ProcessArray* pArray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t uiIndex = pArray->GetProcessLevelHead(PROCESS_TYPE_HIGH);
         uiIndex < pArray->GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++uiIndex) {
        Actor* pActor = static_cast<Actor*>(pArray->GetItem(uiIndex));
        if (pActor && pActor->IsActor()) {
            if (!pActor->IsDead(false)) {
                AliveActorList.AddHead(pActor);
            }
        }
    }
}

void ProcessLists::UpdateHighListPackages(float afDelta, bool abSleeping) {
    Actor* ptarget;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PROCESSES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\ProcessLists.cpp",
        565
    );
    HighProcess::ClearWanderStarted();
    iNumberHighActors = 0;
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        MobileObject* pmob = MobProcessArray.GetItem(i);
        if (pmob && !pmob->IsStillLoading()) {
            if (pmob->GetDelete() ||
                pmob->GetDisabled() ||
                !pmob->pCurrentProcess ||
                pmob->GetCurrentProcessType() ||
                ProcessLists::bIsForciblyUpdated) {
                if (pmob->GetDisabled()) {
                    AddActorToTempChangeList(pmob);
                }
            }
            else {
                Actor* pact = nullptr;
                bool updatefollower = true;
                if (pmob && pmob->IsActor()) {
                    pact = static_cast<Actor*>(pmob);
                    ++iNumberHighActors;
                }

                if (pact && pact->CanSkipActorUpdate()) {
                    pact->pCurrentProcess->CheckforNewPackage(pact, false);
                    updatefollower = false;
                    static_cast<HighProcess*>(pact->pCurrentProcess)->HighProcess::SetSkippedLastUpdate(true);
                }
                else {
                    pmob->Process(afDelta);
                }
                if (pact && pPlayer->IsSleepingorResting()) {
                    pact->RestoreHealth(2.0f);
                    pact->RestoreFatigue(2.0f);
                }

                if (updatefollower) {
                    // &pmob->m_Extra is never null.
                    ExtraFollower* pfollow = pmob->m_Extra.GetFollowerExtra();
                    if (pfollow) {
                        BSSimpleList<Actor*>* pHead;
                        BSSimpleList<Actor*>* pTempList = new BSSimpleList<Actor*>;
                        pHead = new BSSimpleList<Actor*>;
                        BSSimpleList<Actor*>* plist = pHead;
                        for (BSSimpleList<Actor*>* ptemp = pfollow->pActorlist; ptemp && ptemp->m_item;
                             ptemp = ptemp->m_pkNext) {
                            pHead->AddTail(ptemp->m_item);
                        }
                        while (plist && plist->m_item) {
                            ptarget = plist->m_item;
                            if (ptarget && ptarget != pPlayer && ptarget->pCurrentProcess) {
                                TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                                TESPackage* pEditorpack = ptarget->m_Extra.GetPackageExtra();
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

                            plist = plist->m_pkNext;
                        }
                        for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
                            pmob->m_Extra.RemoveFollower(plist->m_item);
                        }
                        pTempList->RemoveAll();
                        if (pTempList) {
                            delete pTempList;
                        }
                        pHead->RemoveAll();
                        if (pHead) {
                            delete pHead;
                        }
                        if (pfollow->pActorlist->IsEmpty()) {
                            pmob->m_Extra.RemoveFollowerExtra();
                        }
                    }
                }
            }
        }
    }
    DisableExcessDeadActors(appTimer.fDelta);
}

void ProcessLists::RunDetectionForAllActors() {
    bool b360LineofSight;
    Actor* pmob = nullptr;
    MobileObject* pMobile = nullptr;
    uint32_t uiHighActorCount = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
    float ftime = appTimer.fDelta * static_cast<double>(uiHighActorCount);
    ftime = BSMIN(ftime, 5.0f);
    if (!uiHighActorCount) {
        pPlayer->SetHostileDetection(false);
    }
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        pMobile = MobProcessArray.GetItem(i);
        if (pMobile && pMobile->IsActor()) {
            pmob = static_cast<Actor*>(pMobile);
            Actor* pdetectact = static_cast<Actor*>(pMobile);
            if (pdetectact->bProcessMe && pdetectact->Get3D()) {
                float fdistancefromplayer = pdetectact->GetDistanceFromReference(pPlayer, false, false);
                if (pdetectact != pPlayer && !pSaveLoadGameOLD->GetLoading()) {
                    float fDetectionTimer = pmob->GetDetectionTimer();
                    if (!i) {
                        pPlayer->SetHostileDetection(false);
                    }
                    if (!(fDetectionTimer < 0.0) || pdetectact->IsUnConscious() || pmob->IsDead(false)) {
                        if (fDetectionTimer < 0.0) {
                            pPlayer->RemovePerceivedActor(pmob);
                        }
                        pmob->ReduceDetectionTimer();
                    }
                    else if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) <= 0.0) {
                        float fevalTime = RandomFloat(0.0f, ftime);
                        pmob->EvaluateDetection(fevalTime);
                        TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(pdetectact->GetActorBase());
                        if (!pBaseData->GetIsGhost()) {
                            BaseProcess* pBaseProcess = pdetectact->pCurrentProcess;
                            if (pBaseProcess->ShouldRunPlayerDetection(pdetectact)) {
                                bool blinesight = false;
                                bool bcombat = pmob->IsInCombat();
                                b360LineofSight = false;
                                int32_t iplayerdetect = pPlayer->GetDetectionLevelAgainstActor(
                                    true,
                                    pmob,
                                    blinesight,
                                    bcombat,
                                    false,
                                    false,
                                    &b360LineofSight
                                );
                                FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                                bool battack = false;
                                if (pmob->GetShouldAttackActor(pPlayer, false, ereaction, false) ||
                                    pmob->IsInCombatWithActor(pPlayer)) {
                                    battack = true;
                                }
                                else if (
                                    (ereaction == FIGHT_REACTION_NEUTRAL || ereaction == FIGHT_REACTION_ENEMY) &&
                                    pmob->GetActorBase()->AIData.bAggroRadius &&
                                    !pmob->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR)
                                ) {
                                    battack = true;
                                }
                                if (pmob) {
                                    if (fShaderShadowUpdateDistance.Float() >
                                        static_cast<double>(fdistancefromplayer)) {
                                        ReferenceProcessLists.InsertActorCloseToPlayer(pdetectact);
                                    }
                                }
                                double dDetectionValue = static_cast<double>(iplayerdetect);
                                if (!(fPlayerDetectActorValue.Float() < dDetectionValue)) {
                                    if (b360LineofSight || blinesight) {
                                        pPlayer->InsertPerceivedActor(pmob, battack, true);
                                    }
                                    else {
                                        pPlayer->RemovePerceivedActor(pmob);
                                    }
                                }
                                else {
                                    pPlayer->InsertPerceivedActor(pmob, battack, false);
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
                    double dCurrentTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
                    afDelta = dCurrentTime - pact->pCurrentProcess->fHourLastProcessed;
                }
                // Passes the raw byte of abSleeping on.
                static_cast<Actor*>(pact)->UpdateMovement(afDelta, abSleeping);
                if (apLevel == PROCESS_TYPE_HIGH) {
                    static_cast<HighProcess*>(pact->pCurrentProcess)
                        ->HighProcess::DetectDoorsForPathing(static_cast<Actor*>(pact));
                }
            }
            else if (pact->IsProjectile()) {
                static_cast<Projectile*>(pact)->Projectile::UpdateIfNotInArrowsList(0.0f);
            }
            else if (pact->IsExplosion()) {
                static_cast<Explosion*>(pact)->Explosion::ProcessMovement(0.0f);
            }
        }
    }
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::IsActorTargetingREFinPackage(TESObjectREFR* apActor, PACKAGE_TYPE apType, bool bOnlyHigh) {
    BSSimpleList<TESObjectREFR*>* pRefList = nullptr;
    bool bMiddleLow = false;
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            if (!bOnlyHigh || i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
                bMiddleLow = i >= MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_LOW) &&
                    i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_LOW);
                Actor* pactor = nullptr;
                if (pMob->IsActor()) {
                    pactor = static_cast<Actor*>(pMob);
                }
                if (pactor) {
                    if (pactor->IsInCombat() && apType == PACKAGE_TYPE_COMBAT) {
                        TESPackage* pbasepack = pactor->GetPackageSetAsPcurrent();
                        CombatController* ppack = nullptr;
                        if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_COMBAT) {
                            ppack = static_cast<CombatController*>(pbasepack);
                        }

                        if (bOnlyHigh && ppack) {
                            if (ppack->GetTarget() == apActor) {
                                if (!pRefList) {
                                    pRefList = new BSSimpleList<TESObjectREFR*>;
                                }
                                if (bMiddleLow && (pactor->IsCreature() || !pactor->GetRefPersists())) {
                                    pactor->StopCombat(nullptr);
                                    continue;
                                }
                                pRefList->AddHead(pactor);
                            }
                        }
                        else if (ppack && ppack->IsActoraCombatTarget(static_cast<Actor*>(apActor))) {
                            if (!pRefList) {
                                pRefList = new BSSimpleList<TESObjectREFR*>;
                            }
                            if (bMiddleLow && (pactor->IsCreature() || !pactor->GetRefPersists())) {
                                pactor->StopCombat(nullptr);
                                continue;
                            }
                            pRefList->AddHead(pactor);
                        }
                    }
                    else {
                        TESPackage* pcurrentPack = pactor->GetCurrentPackage();
                        if (pcurrentPack && (pactor->IsActoraGuard() || apType != PACKAGE_TYPE_ALARM)) {
                            if (pactor->pCurrentProcess->GetTarget() == apActor &&
                                pcurrentPack->GetPackType() == apType) {
                                if (!pRefList) {
                                    pRefList = new BSSimpleList<TESObjectREFR*>;
                                }
                                pRefList->AddHead(pactor);
                            }
                        }
                    }
                }
            }
        }
    }
    return pRefList;
}

Actor* ProcessLists::GetActorRefInHigh(TESNPC* apNpc, bool abDetection) {
    Actor* pResult = nullptr;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        TESForm* pMobileObject = MobProcessArray.GetItem(uiIndex);
        if (pMobileObject && !pResult && !pMobileObject->IsStillLoading()) {
            Actor* pActor = nullptr;
            if (pMobileObject->IsActor()) {
                pActor = static_cast<Actor*>(pMobileObject);
            }
            if (pActor) {
                TESNPC* pBaseForm = nullptr;
                if (pActor->IsNpc()) {
                    pBaseForm = static_cast<TESNPC*>(pActor->GetObjectReference());
                }
                if (pBaseForm && apNpc == pBaseForm) {
                    pResult = pActor;
                }
                else if (pActor->GetExtra()->GetLevCreaOriginalBase() == apNpc) {
                    pResult = pActor;
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor()) {
            static_cast<Actor*>(pMob)->UpdateMagic(false, false);
        }
    }
}

void ProcessLists::RunActorUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        bool bRagdoll = pActor->pRagdollController && pActor->pRagdollController->GetRagdollAnimEnable();
        if (!pMob->Get3D() ||
            !bRagdoll ||
            static_cast<uint8_t>(pMob->Get3D()->GetNotVisible()) ||
            pMob->IsInDialoguewithPlayer()) {
            if (bRagdoll) {
                pActor->pRagdollController->ReinitRBControllerOnUpdate(true);
            }
        }
        else {
            pActor->UpdateRagdollAnimation(true);
        }
    }
}

void ProcessLists::RunActorAnimationUpdates() {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (!pMob || !pMob->IsActor()) {
            continue;
        }
        Actor* pActor = static_cast<Actor*>(pMob);
        TESObjectCELL* pCell = pMob->GetParentCell();
        if (pMob->Get3D() &&
            !pMob->GetDelete() &&
            !pMob->GetDisabled() &&
            pCell &&
            pCell->GetAttached() &&
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
    aiNumber = BSMIN(aiNumber, iActorsCloseToPlayer);
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
    if (apActor->IsDead(false) ||
        eCurrentLevel != eDesiredLevel ||
        apActor->GetCurrentAIProcess()->IsAFollower() ||
        apActor->GetDelete() ||
        pPlayer->IsSleepingorResting()) {
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
    AliveActorList.Remove(apActor);
}

void ProcessLists::RemoveActorCloseToPlayer(Actor* pactor) {
    kNearbyActorLock.Lock(nullptr);
    for (int32_t icurActor = 0; icurActor < iActorsCloseToPlayer; ++icurActor) {
        if (pActorsCloseToPlayerA[icurActor] == pactor) {
            BSmemmove(
                &pActorsCloseToPlayerA[icurActor],
                sizeof(pActorsCloseToPlayerA),
                &pActorsCloseToPlayerA[icurActor + 1],
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
        Actor* pactor = nullptr;
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
        }

        if (pactor) {
            pactor->pCurrentProcess->RemoveDetectionActor(apActor, 3);
        }
    }
}

void ProcessLists::NullOutTargetRef(TESObjectREFR* apRef, bool abDeleted) {
    uint32_t i;
    if (apRef->IsActor()) {
        if (pPlayer->m_Extra.GetFollowerExtra() && pPlayer->m_Extra.GetFollowerExtra()->pActorlist) {
            Actor* pItemActor = static_cast<Actor*>(apRef);
            pPlayer->m_Extra.GetFollowerExtra()->pActorlist->Remove(pItemActor);
        }

        if (pPlayer->pCurrentProcess) {
            pPlayer->pCurrentProcess->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
        }
        RemoveAliveActor(static_cast<Actor*>(apRef));
    }
    if (pPlayer->GetDialogueItemTarget() == apRef) {
        pPlayer->SetDialogueItemTarget(nullptr);
    }
    if (pPlayer->pCurrentProcess) {
        if (pPlayer->pCurrentProcess->GetLastHeadTrackTarget() == apRef) {
            pPlayer->pCurrentProcess->ClearLastHeadTrackTarget();
        }
        if (pPlayer->pCurrentProcess->GetLastGreeted() == apRef) {
            pPlayer->pCurrentProcess->SetLastGreeted(nullptr);
        }
        BaseProcess* pproc = pPlayer->pCurrentProcess;
        DetectionEvent* pevent = pproc->GetActorsDetectionEvent(pPlayer);
        if (pevent && pevent->pRef == apRef && pPlayer->pCurrentProcess) {
            pPlayer->pCurrentProcess->RemoveDetectionEvent();
        }
    }
    if (apRef->IsActor()) {
        pPlayer->RemoveActorInDispositionModifier(static_cast<Actor*>(apRef));
    }
    for (i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = nullptr;
        Projectile* pprojectile = nullptr;
        if (pMob && !pMob->IsStillLoading()) {
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            else if (pMob->IsProjectile()) {
                pprojectile = static_cast<Projectile*>(pMob);
            }
            BSSimpleList<Actor*>* ptemp = nullptr;
            ExtraFollower* pfollow = nullptr;
            if (pactor) {
                pfollow = pactor->m_Extra.GetFollowerExtra();
            }
            if (pfollow) {
                ptemp = pfollow->pActorlist;
            }
            if (pactor && pactor->m_Extra.GetFollowerExtra() && apRef->IsActor()) {
                pactor->m_Extra.RemoveFollower(static_cast<Actor*>(apRef));
            }

            if (pprojectile && pprojectile->GetShooter() == apRef) {
                pprojectile->pShooter = nullptr;
            }

            if (!pMob || pactor) {
                while (pactor) {
                    pactor->HandleDeletedReferenceForActorMover(apRef);
                    // Tests the address &pactor->m_Extra, which is never null.
                    if (pactor->m_Extra.GetHeadTrackTargetExtra() == apRef) {
                        pactor->m_Extra.SetHeadTrackTargetExtra(nullptr);
                    }
                    BaseProcess* pproc = pactor->pCurrentProcess;
                    if (pactor->GetDialogueItemTarget() == apRef) {
                        pactor->SetDialogueItemTarget(nullptr);
                        if (pproc) {
                            pproc->SetGreetingFlag(false);
                            pproc->ClearMovementStoped();
                        }
                    }
                    if (pproc) {
                        if (pproc->eLevel < PROCESS_TYPE_MIDDLE_LOW) {
                            pproc->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
                            pproc->RemoveAvoidPathingNode(apRef);
                            pproc->ClearPathLookTarget(apRef);
                            DetectionEvent* pevent = pproc->GetActorsDetectionEvent(pPlayer);
                            if (pevent && pevent->pRef == apRef) {
                                pproc->RemoveDetectionEvent();
                            }

                            if (pproc->GetLastHeadTrackTarget() == apRef) {
                                pproc->ClearLastHeadTrackTarget();
                            }
                            if (pproc->GetLastGreeted() == apRef) {
                                pproc->SetLastGreeted(nullptr);
                            }
                            if (apRef->IsActor()) {
                                pproc->RemoveSpokenToActor(static_cast<Actor*>(apRef));
                            }
                            TESPackage* ppack = pproc->GetRunOncePackage();
                            if (ppack) {
                                ActorPackage* pdata = pproc->GetActorPackageThatIsRunning();
                                if (pdata && pdata->pTarget == apRef) {
                                    pactor->EndInterruptPackage(false);
                                }
                                else if (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                                    DialoguePackage* pdialoguePack = static_cast<DialoguePackage*>(ppack);
                                    if (pdialoguePack->pTargetActor == apRef) {
                                        pactor->EndDialogue();
                                    }
                                }
                            }
                        }
                        pproc->ClearObjectFromAcquireList(apRef);
                        ExtraPackage* pextrapack = static_cast<ExtraPackage*>(pactor->m_Extra.GetExtraData(25));
                        if (pextrapack && pextrapack->pTarg == apRef) {
                            pextrapack->pTarg = nullptr;
                        }

                        if (pactor->IsFleeing(false)) {
                            TESPackage* pbasepack = pactor->GetCurrentPackage();
                            FleePackage* pfleePack = nullptr;
                            if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                                pfleePack = static_cast<FleePackage*>(pbasepack);
                            }

                            if (pfleePack && pfleePack->pRefFleeTo == apRef) {
                                pfleePack->SetRefFleeTo(nullptr);
                            }

                            pactor->RemoveFleeTarget(apRef);
                        }
                        else {
                            if (pactor->IsinSpectator() && apRef->IsActor()) {
                                TESPackage* pbasepack = pactor->GetCurrentPackage();
                                Actor* prefAsActor = static_cast<Actor*>(apRef);
                                SpectatorPackage* pspec = nullptr;
                                if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_SPECTATOR) {
                                    pspec = static_cast<SpectatorPackage*>(pbasepack);
                                }

                                if (pspec) {
                                    pspec->HandleDeletedActor(prefAsActor);
                                }
                            }
                        }
                        pproc->HandleDeletedReference(apRef);
                        ExtraDataList* pextraData = &pactor->m_Extra;
                        if (pextraData) {
                            ActorPackageData* pactorPackageData = pextraData->GetActorPackageData();
                            if (pactorPackageData) {
                                pactorPackageData->HandleDeletedReference(apRef);
                            }
                            TESObjectREFR* pextraTarget = pextraData->GetPackageExtraTarget();
                            if (pextraTarget == apRef) {
                                pextraData->SetPackageExtraTarget(nullptr);
                            }
                        }
                        if (apRef->IsActor()) {
                            // Tests apRef for null before taking its MagicTarget subobject.
                            MagicTarget* pMagicTarget = static_cast<Actor*>(apRef);
                            if (pproc->GetDesiredTarget() == pMagicTarget) {
                                pproc->SetDesiredTarget(nullptr);
                            }
                        }
                        if (!pproc->eLevel) {
                            pproc->ClearDialogTarget();
                        }
                        if (pactor->GetDialogueItemTarget() == apRef) {
                            pactor->SetDialogueItemTarget(nullptr);
                        }
                        if (abDeleted) {
                            TESPackage* pcurrpack = pactor->GetCurrentPackage();
                            if (pcurrpack && pcurrpack->pPackLoc && pcurrpack->pPackLoc->GetLocReference() == apRef) {
                                pcurrpack->pPackLoc->SetLocReference(nullptr);
                            }

                            TESPackage* pprocpack = pproc->GetCurrentPackage();
                            if (pprocpack && pprocpack->pPackLoc && pprocpack->pPackLoc->GetLocReference() == apRef) {
                                pprocpack->pPackLoc->SetLocReference(nullptr);
                            }
                        }
                        CombatController* pcomcont = pactor->GetCombatController();
                        if (pcomcont) {
                            pcomcont->RemoveTarget(static_cast<Actor*>(apRef));
                        }
                        if (pproc->GetTarget() == apRef) {
                            pproc->SetTarget(nullptr);
                        }
                        if (pactor->GetCurrentTarget() == apRef) {
                            pactor->SetCurrentTarget(nullptr);
                        }
                        TESPackage* prunoncepack = pproc->GetRunOncePackage();
                        if (pactor->GetCurrentPackageTarget() == apRef ||
                            (prunoncepack &&
                             prunoncepack->GetPackageTarget() &&
                             prunoncepack->GetPackageTarget()->GetTargReference() == apRef)) {
                            if (pactor->IsinDialogue()) {
                                pactor->EndDialogue();
                            }
                            else if (pactor->GetCurrentPackage()->GetIsCreated()) {
                                pactor->EndInterruptPackage(false);
                            }
                            else {
                                pproc->ClearCurrentPackage();
                                pproc->ClearRunOncePackage();
                            }
                        }
                        pproc->RemoveHeadTrackTarget(apRef);
                        if (pproc->GetCurrentFurniture() == apRef) {
                            pproc->GetFurnitureMarker()->SetHeading(0.0f);
                            pproc->GetFurnitureMarker()->SetType(0);
                            pproc->GetFurnitureMarker()->SetPosition(NiPoint3::ZERO);
                            FurnitureMark* pMarker = pactor->pCurrentProcess->GetFurnitureMarker();
                            pproc->SetFurniture(nullptr, 127, pMarker);
                        }
                        if (ptemp && ptemp->m_item) {
                            pactor = nullptr;
                            if (ptemp->m_item->IsActor()) {
                                pactor = ptemp->m_item;
                            }
                            ptemp = ptemp->m_pkNext;
                        }
                        else {
                            pactor = nullptr;
                        }
                    }
                    else {
                        pactor = nullptr;
                    }
                }
            }
            else if (pMob->GetTalkingActivator() && apRef == pMob->GetTalkingActivator()) {
                pMob->EndDialogue();
            }
        }
    }
    for (BSSimpleList<void*>* plist = &TempShouldMoveList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor = nullptr;
        Projectile* pprojectile = nullptr;
        TESObjectREFR* pref = static_cast<TESObjectREFR*>(plist->m_item);
        if (pref && !pref->IsStillLoading()) {
            if (static_cast<TESObjectREFR*>(plist->m_item)->IsActor()) {
                pactor = static_cast<Actor*>(static_cast<TESObjectREFR*>(plist->m_item));
            }
            else if (static_cast<TESObjectREFR*>(plist->m_item)->IsProjectile()) {
                pprojectile = static_cast<Projectile*>(static_cast<TESObjectREFR*>(plist->m_item));
            }
            BSSimpleList<Actor*>* ptemp = nullptr;
            ExtraFollower* pfollow = nullptr;
            if (pactor) {
                pfollow = pactor->m_Extra.GetFollowerExtra();
            }
            if (pfollow) {
                ptemp = pfollow->pActorlist;
            }
            if (pprojectile && pprojectile->GetShooter() == apRef) {
                pprojectile->pShooter = nullptr;
            }

            while (pactor) {
                BaseProcess* pproc = pactor->pCurrentProcess;
                if (pactor->GetDialogueItemTarget() == apRef) {
                    pactor->SetDialogueItemTarget(nullptr);
                    if (pproc) {
                        pproc->SetGreetingFlag(false);
                        pproc->ClearMovementStoped();
                    }
                }
                if (pproc) {
                    if (i < 2) {
                        pproc->RemoveDetectionActor(static_cast<Actor*>(apRef), 3);
                    }
                    pproc->ClearObjectFromAcquireList(apRef);
                    ExtraPackage* pextrapack = static_cast<ExtraPackage*>(pactor->m_Extra.GetExtraData(25));
                    if (pextrapack && pextrapack->pTarg == apRef) {
                        pextrapack->pTarg = nullptr;
                    }

                    if (pactor->IsFleeing(false)) {
                        TESPackage* pbasepack = pactor->GetCurrentPackage();
                        FleePackage* pfleePack = nullptr;
                        if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                            pfleePack = static_cast<FleePackage*>(pbasepack);
                        }

                        if (pfleePack && pfleePack->pRefFleeTo == apRef) {
                            pfleePack->SetRefFleeTo(nullptr);
                        }

                        pactor->RemoveFleeTarget(apRef);
                    }
                    if (apRef->IsActor() && pactor->pCurrentProcess) {
                        // Tests apRef for null before taking its MagicTarget subobject.
                        MagicTarget* pMagicTarget = static_cast<Actor*>(apRef);
                        if (pactor->pCurrentProcess->GetDesiredTarget() == pMagicTarget) {
                            pactor->pCurrentProcess->SetDesiredTarget(nullptr);
                        }
                    }

                    if (!pproc->eLevel) {
                        pproc->ClearDialogTarget();
                    }
                    if (pactor->GetDialogueItemTarget() == apRef) {
                        pactor->SetDialogueItemTarget(nullptr);
                    }
                    TESPackage* pcurrpack = pactor->GetCurrentPackage();
                    if (pcurrpack && pcurrpack->pPackLoc && pcurrpack->pPackLoc->GetLocReference() == apRef) {
                        pcurrpack->pPackLoc->SetLocReference(nullptr);
                    }

                    TESPackage* pprocpack = pproc->GetCurrentPackage();
                    if (pprocpack && pprocpack->pPackLoc && pprocpack->pPackLoc->GetLocReference() == apRef) {
                        pprocpack->pPackLoc->SetLocReference(nullptr);
                    }

                    CombatController* pcomcont = pactor->GetCombatController();
                    if (pcomcont) {
                        pcomcont->RemoveTarget(static_cast<Actor*>(apRef));
                    }
                    if (pproc->GetTarget() == apRef) {
                        pproc->SetTarget(nullptr);
                    }
                    if (pactor->GetCurrentTarget() == apRef) {
                        pactor->SetCurrentTarget(nullptr);
                    }
                    TESPackage* prunoncepack = pproc->GetRunOncePackage();
                    if (pactor->GetCurrentPackageTarget() == apRef ||
                        (prunoncepack &&
                         prunoncepack->GetPackageTarget() &&
                         prunoncepack->GetPackageTarget()->GetTargReference() == apRef)) {
                        if (pactor->IsinDialogue()) {
                            pactor->EndDialogue();
                        }
                        else if (pactor->GetCurrentPackage()->GetIsCreated()) {
                            pactor->EndInterruptPackage(false);
                        }
                        else {
                            pproc->ClearCurrentPackage();
                            pproc->ClearRunOncePackage();
                        }
                    }
                    pproc->RemoveHeadTrackTarget(apRef);
                    if (pproc->GetCurrentFurniture() == apRef) {
                        pproc->GetFurnitureMarker()->SetHeading(0.0f);
                        pproc->GetFurnitureMarker()->SetType(0);
                        pproc->GetFurnitureMarker()->SetPosition(NiPoint3::ZERO);
                        FurnitureMark* pMarker = pactor->pCurrentProcess->GetFurnitureMarker();
                        pproc->SetFurniture(nullptr, 127, pMarker);
                    }
                    if (ptemp && ptemp->m_item) {
                        pactor = nullptr;
                        if (ptemp->m_item->IsActor()) {
                            pactor = ptemp->m_item;
                        }
                        ptemp = ptemp->m_pkNext;
                    }
                    else {
                        pactor = nullptr;
                    }
                }
                else {
                    pactor = nullptr;
                }
            }
        }
    }
}

bool ProcessLists::FlareHitEffect(TESObjectREFR* apEffectTarget, MagicSystem::FlareType aeFlareType) {
    if (MagicEffectList.IsEmpty()) {
        return false;
    }
    MagicShaderHitEffect* pflareEffect = nullptr;
    float effectAge = std::numeric_limits<float>::max();
    for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
        MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
        if (pshaderEffect && pshaderEffect->GetType() == MAGIC_HIT_EFFECT_SHADER) {
            ActiveEffect* pae = pshaderEffect->pEffect;
            if (pae && static_cast<double>(effectAge) > pshaderEffect->BSTempEffect::fAge && pae->pTarget) {
                if (pae->pTarget->GetTargetStatsObject() == apEffectTarget && Interface::PreloadWorldMap()) {

                    pflareEffect = pshaderEffect;
                    effectAge = pshaderEffect->BSTempEffect::fAge;
                }
            }
        }
    }
    if (!pflareEffect) {
        return false;
    }
    pflareEffect->Init();
    pflareEffect->ResetAlphaTimer();
    return true;
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
        if (!pActor->Get3D() ||
            pActor->GetDelete() ||
            pActor->GetDisabled() ||
            !pActor->GetParentCell() ||
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
        BSParallelArrayTask kTask(&UpdateAnimMovement, &kArray, kArray.GetSize());
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
        if (!pActor ||
            pActor->IsDead(false) ||
            !pActor->IsNpc() ||
            pActor->IsMoving() ||
            pActor->IsTurning() ||
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
            pPackage &&
            pPackage->GetPackType() == PACKAGE_TYPE_SLEEP &&
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
            pPackage &&
            pPackage->GetPackType() == PACKAGE_TYPE_EAT &&
            pPackage->IsActorAtLocation(pActor, false, -1.0f, false) &&
            pActor->GetBestFoodItem()
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
         uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         ++uiIndex) {
        Actor* pActor = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        if (pActor && pActor->IsActor()) {
            Animation* pAnimData = pActor->GetAnimation();
            bool bUpdated = false;
            if (pAnimData) {
                bUpdated = pAnimData->UpdateQueuedScenegraph();
            }
            if (pActor->QUpdateLighting() && bUpdated) {
                NiNode* p3D = pActor->Get3D();
                BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(p3D, false);
                pActor->SetUpdateLighting(false);
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
        BSParallelArrayTask kTask(&UpdateAnimSceneGraph, &kArray, kArray.GetSize());
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
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
            if (pActor->GetActorBase()->GetHealth() > 0 && !pActor->GetHavokDeath()) {
                pActor->GetCurrentAIProcess()->LoadDynamicIdle(pActor);
            }
        }
        else if (pActor && pActor->GetFatiguePercentage() <= 0.0) {
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

// Stamps the game date for the low processes, then sets up the package locations of the low list actors that are
// alive; the others are marked dead when they have a base object and leave the low list.
void ProcessLists::InitPackageLocations() {
    fGameHour = appCalendar.GetHour();
    iGameYear = appCalendar.GetYear();
    LowProcess::eCurrentGameMonth = appCalendar.GetMonth();
    LowProcess::cCurrentGameDay = appCalendar.GetDay();
    MobileObject* pMob = nullptr;
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
         ++i) {
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
            if (!pActor->GetDelete() &&
                pActor->GetObjectReference() &&
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
        MEM_CONTEXT_PROCESSES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\ProcessLists.cpp",
        1313
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
    TESObjectREFR* pRecentAction = pPlayer->GetRecentAction();
    PLAYER_ACTION eLastAction = static_cast<PLAYER_ACTION>(pPlayer->GetLastAction());
    CommentOnPlayerAction(eLastAction, pRecentAction, false);
    BSDoorHavokController::ReenableDoors();
    if (iNumHWThreads.UInt() == 1) {
        BGSDestructibleObjectForm::UpdateDestructibleObjects();
    }
    float fSeconds = appTimer.GetSeconds();
    float fClock = GetSystemTimeClock() + fSeconds;
    SetSystemTimeClock(fClock);
    int32_t iFrameRate = static_cast<int32_t>(1.0 / fSeconds);
    bool bProcess = true;
    iNumberHighActors = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
    fSecondsPassedNoProcess = fSecondsPassedNoProcess + fSeconds;
    if (iFrameRate < 50) {
        if (iFrameRate >= 10) {
            if (!(fSecondsPassedNoProcess < 1.0)) {
                fSecondsPassedNoProcess = 0.0;
            }
            else {
                fSecondsPassedNoProcess = (1.0 - fSeconds) / 2.0 + fSecondsPassedNoProcess;
                bProcess = false;
            }
        }
        else if (!(fSecondsPassedNoProcess < 1.0)) {
            fSecondsPassedNoProcess = 0.0;
        }
        else {
            bProcess = false;
            fSecondsPassedNoProcess = fSecondsPassedNoProcess + fSeconds;
        }
    }
    if (bProcess) {
        if (bProcessLow && !Main::GetSceneIsComplex() && ReferenceProcessLists.kUpdateLock.TryLock()) {
            UpdateLowList(0.0, false);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        if (bProcessMLow && !Main::GetSceneIsComplex()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            UpdateMiddleLowList(0.0, false);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        if (bProcessMHigh) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            UpdateMiddleHighList(0.0, false);
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
        MobileObject* pRef = nullptr;
        if (apList->m_item->IsMobileObject()) {
            pRef = static_cast<MobileObject*>(apList->m_item);
        }
        apList = apList->m_pkNext;
        if (pRef) {
            if (abChangeLevel) {
                if (pRef->GetDelete() || pRef->GetDisabled()) {
                    if (pRef->pCurrentProcess && ReferenceProcessLists.kUpdateLock.TryLock()) {
                        MobProcessArray.RemoveActor(pRef, pRef->GetCurrentProcessType());

                        pRef->DeleteCurrentProcess();
                        ReferenceProcessLists.kUpdateLock.Unlock();
                    }
                }
                else if (pRef != pPlayer && pRef->pCurrentProcess && !pRef->ChangeProcessLevel()) {
                    apList = pHead;
                }
            }
            else {
                AddActorToTempChangeList(pRef);
            }
        }
    }
}

void ProcessLists::ProcessActors(BSSimpleList<TESObjectREFR*>* apList) {
    BSSimpleList<TESObjectREFR*>* pActList = new BSSimpleList<TESObjectREFR*>;
    while (apList && apList->m_item) {
        if (apList->m_item->IsActor()) {
            pActList->AddHead(apList->m_item);
        }
        apList = apList->m_pkNext;
    }
    while (pActList && pActList->m_item) {
        Actor* pRef = static_cast<Actor*>(pActList->m_item);
        if (pRef &&
            pRef != pPlayer &&
            pRef->pCurrentProcess &&
            !pRef->GetDelete() &&
            !pRef->GetDisabled() &&
            !pRef->IsInCombat()) {
            float flastprocesstime = pRef->pCurrentProcess->fHourLastProcessed;
            float fhour = appCalendar.GetHour();
            float fdelta = fhour - flastprocesstime;
            if (flastprocesstime > static_cast<double>(fhour)) {
                fdelta = fhour + 24.0 - flastprocesstime;
            }
            bool isescorting = false;
            if (fdelta < 0.05) {
                fdelta = 0.0f;
            }
            if (pRef->pCurrentProcess->GetCurrentPackage()) {
                if (pRef->pCurrentProcess->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ESCORT) {
                    isescorting = true;
                }
            }
            if ((!isescorting && !pRef->IsFollowing()) || pRef->pCurrentProcess->GetTarget() != pPlayer) {
                pRef->Process(fdelta);
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
// left is flagged to change process later. A list item that is no mobile object stalls the walk, deliberately.
// The lock and AddReference go through the engine's ReferenceProcessLists, not this.
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
            (!pCPLRef->IsTalkingActivator() || Interface::InDialogWith() == pCPLRef)) {
            // The death time must not be equal to 0 (NaN goes on); the hours are converted as unsigned. The hours
            // passed must be strictly greater.
            bool bRespawn = false;
            if (pActor &&
                pActor->IsDead(false) &&
                !pActor->GetEssential() &&
                pActor->GetRespawn() &&
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
                ReferenceProcessLists.AddReference(pCPLRef, PROCESS_TYPE_HIGH, false, false, nullptr);
            }
            else if (
                (!pCPLRef->GetDelete() || pCPLRef->GetFile(-1)) &&
                (!pCPLRef->IsTalkingActivator() || pCPLRef->IsTalking())
            ) {
                if ((pCPLRef->GetDelete() || pCPLRef->GetDisabled()) &&
                    pCPLRef->GetCurrentAIProcess() &&
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
// mob too.
void ProcessLists::UpdateTempEffects(float afDeltaTime) {
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
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
            BSTempEffect* pEffect = rspItem;
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
            BSTempEffect* pEffect = rspItem;
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
    int32_t iCount = GlobalTempEffectList.ItemsInList();

    iCount += MagicEffectList.ItemsInList();
    if (!iCount) {
        return;
    }

    alignas(BSScrapArray<EffectTaskData>) uint8_t aArrayStorage[sizeof(BSScrapArray<EffectTaskData>)];
    BSScrapArray<EffectTaskData>& kTaskArray = *::new (aArrayStorage) BSScrapArray<EffectTaskData>;
    for (BSSimpleList<NiPointer<BSTempEffect>>* pIter = &GlobalTempEffectList; pIter; pIter = pIter->GetNext()) {
        NiPointer<BSTempEffect>& rspItem = pIter->GetItem();
        BSTempEffect* pEffect = rspItem;
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
        BSTempEffect* pEffect = rspItem;
        if (pEffect) {
            uint32_t uiIndex = kTaskArray.AddUninitialized();
            kTaskArray.GetAt(uiIndex).pTempEffect = pEffect;
            kTaskArray.GetAt(uiIndex).fDeltaTime = afDeltaTime;
            kTaskArray.GetAt(uiIndex).bGlobal = false;
            kTaskArray.GetAt(uiIndex).bRemove = false;
        }
    }

    if (kTaskArray.GetSize()) {
        BSParallelArrayTask kTask(&UpdateTempEffect, &kTaskArray, kTaskArray.GetSize());
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
            if (phitEffect &&
                NiObject::IsKindOf_0(&MagicHitEffect::ms_RTTI, phitEffect) &&
                phitEffect->pTarget == apEffectTarget) {
                phitEffect->ClearTarget();
                phitEffect->bFinished = true;
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
        GlobalCrimeListArray[i] = nullptr;
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
    bool bflag = false;
    if (abInterior) {
        fdistance = fHostileActorInteriorDistance.Float();
    }
    for (uint32_t i = 0; !bflag && i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = nullptr;
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
        }

        if (pactor && pactor != pPlayer && !pactor->IsDead(false) && !pactor->GetDisabled()) {
            if (pactor->pCurrentProcess) {
                if (pactor->pCurrentProcess->GetCommandingActor()) {
                    pactor = pactor->pCurrentProcess->GetCommandingActor();
                }
            }
            float fdist = pPlayer->GetDistanceFromReference(pactor, false, false);
            if (fdistance >= static_cast<double>(fdist)) {
                pactor->GetDisposition(pPlayer, nullptr);
                FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                if (pactor->IsInCombatWithActor(pPlayer)) {
                    bflag = true;
                }
                else if (pactor->GetShouldAttackActor(pPlayer, false, ereaction, false)) {
                    bflag = true;
                }
            }
        }
    }
    return bflag;
}

bool ProcessLists::IsPlayerInRadiationArea() {
    return bPlayerInRadiatedWater || bPlayerInRadiationArea;
}

void ProcessLists::ResetPlayerActionsCommentTimer() {
    fPlayerActionCommentTimer = static_cast<float>(fPlayerActionCommentTimer + (-0.5));
}

void ProcessLists::SetSystemTimeClock(float afvalue) {
    ProcessLists::fSystemTimeClock = afvalue;
    if (afvalue > 100000.0) {
        ProcessLists::fSystemTimeClock = 0.0f;
    }
    if (std::isnan(ProcessLists::fSystemTimeClock) || !std::isfinite(ProcessLists::fSystemTimeClock)) {
        ProcessLists::fSystemTimeClock = 0.0f;
    }
}

int32_t ProcessLists::GetCrimeNumber() {
    uint32_t uiCrimeNumber = iCrimeNumber;
    iCrimeNumber = uiCrimeNumber + 1;
    return static_cast<int32_t>(uiCrimeNumber);
}

void ProcessLists::RemoveTempEffectsForCell(TESObjectCELL* apCell) {
    if (!GlobalTempEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pelist = &GlobalTempEffectList;
        BSSimpleList<NiPointer<BSTempEffect>>* plastlist = nullptr;
        while (pelist && !pelist->IsEmpty()) {
            BSTempEffect* peffect = pelist->m_item.m_pObject;
            if (peffect->pCell == apCell) {
                peffect->pCell = nullptr;
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
        BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList;
        BSSimpleList<NiPointer<BSTempEffect>>* plastlist = nullptr;
        while (pelist && !pelist->IsEmpty()) {
            BSTempEffect* peffect = pelist->m_item.m_pObject;
            if (peffect->pCell == apCell) {
                peffect->pCell = nullptr;
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

BSSimpleList<TESObjectREFR*>* ProcessLists::GetActorTargetinganObject(uint32_t aId, Actor* apActorPickup) {
    BSSimpleList<TESObjectREFR*>* pRefList = nullptr;
    TESObjectREFR* pref = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(aId));
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetSize(); ++uiIndex) {
        MobileObject* pMob = MobProcessArray.GetItem(uiIndex);
        Actor* pactor = nullptr;
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
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
                TESObjectREFR* pTarget = pactor->GetCurrentTarget();
                if (pTarget->iFormID == aId && apActorPickup != pactor) {
                    if (!pRefList) {
                        pRefList = new BSSimpleList<TESObjectREFR*>;
                    }
                    pRefList->AddHead(pactor);
                }
            }
        }
    }
    return pRefList;
}

Actor* ProcessLists::FindOwnerSellsObject(TESObjectREFR* pRef) {
    Actor* pOwner = nullptr;
    for (uint32_t uiIndex = 0; !pOwner && uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        Actor* pObject = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        Actor* pActor = nullptr;
        if (pObject && pObject->IsActor()) {
            pActor = pObject;
        }

        while (pActor) {
            if (pRef->IsAnOwner(pActor, true)) {
                TESObject* pBaseObject = pRef->GetObjectReference();
                if (pActor->GetActorBase()->SellBuysItem(pBaseObject)) {
                    pOwner = pActor;
                }
            }
            pActor = nullptr;
        }
    }
    return pOwner;
}

void ProcessLists::RemoveAllCrimesforAnActor(Actor* apActor) {
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[i];
        BSSimpleList<Crime*>* pitem = pGlobalCrimeList;
        while (pitem && pitem->m_item) {
            Crime* pcrime = pitem->m_item;
            if (pcrime->pcriminal == apActor) {
                for (BSSimpleList<TESFaction*>* pfactlist = pcrime->pListofEnemyFactions;
                     pfactlist && !pfactlist->IsEmpty();
                     pfactlist = pfactlist->m_pkNext) {
                    TESFaction* pfac = pfactlist->m_item;
                    if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, pcrime)) {
                        pfac->SetPCAnEnemy(false);
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

// The bound deliberately lets crime type 5 through, one past GlobalCrimeListArray's five entries: its
// read there is the word after the array, GlobalTempEffectList's head item, which this index reads alike.
void ProcessLists::RemoveCrimeGlobalCrimeList(Crime* apCrime) {
    if (apCrime && apCrime->ecrime > PackageNS::CRIME_TYPE_NONE && apCrime->ecrime <= PackageNS::CRIME_TYPE_COUNT) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[apCrime->ecrime];
        if (pGlobalCrimeList) {
            pGlobalCrimeList->Remove(apCrime);
        }
    }
}

void ProcessLists::UpdateRadiationList() {
    NiTMapItem<uint32_t, TESObjectREFR*>* iter;
    bool bclearRadiation = false;
    float fheaviestPlayerRadiation = 0.0f;
    LoadedReferenceMap* pmap = &pLoadedReferenceCollection->RadiationMap;
    if (pmap) {
        iter = pmap->GetFirstPos();
        if (iter) {
            bRadiationExists = true;
        }
        else if (bRadiationExists) {
            bRadiationExists = false;
            bclearRadiation = true;
        }
        while (iter) {
            uint32_t referenceID = 0;
            TESObjectREFR* preference = nullptr;
            pmap->GetNext(iter, referenceID, preference);
            ExtraDataList* pextra = preference ? &preference->m_Extra : nullptr;
            if (pextra) {
                float fradius = pextra->GetRadius();
                float fradiation = pextra->GetRadiation();
                for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
                    MobileObject* pMob = MobProcessArray.GetItem(i);
                    Actor* pactor = nullptr;
                    if (pMob && pMob->IsActor()) {
                        pactor = static_cast<Actor*>(pMob);
                    }

                    if (pactor) {
                        HighProcess* pProcess = static_cast<HighProcess*>(pactor->pCurrentProcess);
                        double dRadReEvaluationTimer = pProcess->GetRadReEvaluationTimer();
                        if (fRadiationReevaluationRate.Float() < dRadReEvaluationTimer) {
                            float fdistance = pactor->GetDistanceFromReference(preference, false, true);
                            float fadjustedRadiation = GamePlayFormulas::GetRadiationLevel(
                                fradiation,
                                fradius,
                                fdistance
                            );
                            float fresistance = pactor->GetRadiationResistanceMult();
                            fadjustedRadiation = fadjustedRadiation * fresistance;
                            if (fRadiationAvoidThreshold.Float() < static_cast<double>(fadjustedRadiation) &&
                                static_cast<double>(fadjustedRadiation) > pProcess->GetCurrentRadiation()) {
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
                                        NiPoint3 kLocation = preference->GetLocationOnReference();
                                        pProcess->AddAvoidPathingArea(
                                            pactor,
                                            kLocation,
                                            fradius,
                                            std::numeric_limits<float>::max(),
                                            fadjustedRadiation,
                                            preference,
                                            nullptr
                                        );
                                    }
                                }
                                pProcess->SetCurrentRadiation(fadjustedRadiation);
                            }
                            pProcess->SetRadiationDelta(pProcess->GetCurrentRadiation());
                        }
                    }
                }
                HighProcess* pPlayerProcess = static_cast<HighProcess*>(pPlayer->pCurrentProcess);
                float fdistance = pPlayer->GetDistanceFromReference(preference, false, true);
                float fadjustedRadiation = GamePlayFormulas::GetRadiationLevel(fradiation, fradius, fdistance);
                if (fadjustedRadiation > 0.0) {
                    float fresistance = pPlayer->GetRadiationResistanceMult();
                    fadjustedRadiation = fadjustedRadiation * fresistance;
                }
                if (pPlayer &&
                    fadjustedRadiation > 0.0 &&
                    static_cast<double>(fadjustedRadiation) > pPlayerProcess->GetCurrentRadiation()) {
                    pPlayerProcess->SetCurrentRadiation(fadjustedRadiation);
                }

                if (fadjustedRadiation > static_cast<double>(fheaviestPlayerRadiation)) {
                    fheaviestPlayerRadiation = fadjustedRadiation;
                }
                pPlayerProcess->SetRadiationDelta(pPlayerProcess->GetCurrentRadiation());
            }
        }
    }
    bPlayerInRadiatedWater = fheaviestPlayerRadiation > 0.0;
    if (bclearRadiation) {
        for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
            Actor* pactor;
            MobileObject* pMob = MobProcessArray.GetItem(i);
            if (pMob && pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            else {
                pactor = nullptr;
            }
            if (pactor) {
                HighProcess* pProcess = static_cast<HighProcess*>(pactor->pCurrentProcess);
                if (pProcess) {
                    pProcess->SetRadiationDelta(0.0f);
                }
            }
        }
        HighProcess* pPlayerProcess = static_cast<HighProcess*>(pPlayer->pCurrentProcess);
        if (pPlayerProcess) {
            pPlayerProcess->SetRadiationDelta(0.0f);
        }
    }
}

bool ProcessLists::IsActorDetected(Actor* apActor) {
    bool bDetected = false;
    Actor* pActor = nullptr;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH) && !bDetected;
         ++uiIndex) {
        TESForm* pForm = MobProcessArray.GetItem(uiIndex);
        if (pForm && pForm->IsActor()) {
            pActor = static_cast<Actor*>(pForm);
        }

        if (pActor && pActor->IsNpc() && pActor->GetDetectionLevelValue(apActor) == DETECTION_SEEN) {
            if (!pActor->IsInCombat() ||
                !pActor->GetCombatController() ||
                pActor->GetCombatController()->GetTarget() != apActor ||
                !static_cast<uint8_t>(pActor->GetCombatController()->IsNotDetectingTarget())) {
                bDetected = true;
            }
        }
    }
    return bDetected;
}

void ProcessLists::NullOutActorsPackage(TESPackage* apPackage) {
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = parray->GetProcessLevelHead(PROCESS_TYPE_HIGH); i < parray->GetProcessLevelTail(PROCESS_TYPE_LOW);
         ++i) {
        MobileObject* pmob = parray->GetItem(i);
        Actor* pactor = nullptr;
        if (pmob && pmob->IsActor() && !pmob->IsStillLoading()) {
            pactor = static_cast<Actor*>(pmob);
        }

        while (pactor) {
            TESPackage* pPackageRunning = nullptr;
            if (pactor->pCurrentProcess) {
                pPackageRunning = pactor->pCurrentProcess->GetPackageThatIsRunning();
            }
            if (pactor->pCurrentProcess) {
                if (pPackageRunning != pactor->pCurrentProcess->GetCurrentPackage()) {
                    MiddleHighProcess* pProcess = nullptr;
                    if (pactor->pCurrentProcess) {
                        PROCESS_TYPE eLevel = pactor->pCurrentProcess->eLevel;
                        if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                            pProcess = static_cast<MiddleHighProcess*>(pactor->pCurrentProcess);
                        }
                    }
                    if (pPackageRunning == apPackage && pProcess) {
                        pProcess->ClearRunOncePackage();
                    }
                }
            }
            if (pactor->pCurrentProcess) {
                if (pactor->pCurrentProcess->GetCurrentPackage() == apPackage) {
                    pactor->pCurrentProcess->ClearCurrentPackage();
                    if (!pSaveLoadGame->GetSaveGameLoading()) {
                        pactor->pCurrentProcess->CheckforNewPackage(pactor, true);
                    }
                }
            }
            if (pactor->m_Extra.GetPackageExtra() == apPackage) {
                pactor->m_Extra.SetPackageExtra(nullptr, 0, nullptr, false, false, false);
            }
            pactor = nullptr;
        }
    }
}

void ProcessLists::StopCombatandAlarmonActor(Actor* apActor, bool abCombatOnly) {
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = parray->GetProcessLevelHead(PROCESS_TYPE_HIGH); i < parray->GetProcessLevelTail(PROCESS_TYPE_LOW);
         ++i) {
        MobileObject* pmob = parray->GetItem(i);
        Actor* pactor = nullptr;
        if (pmob && pmob->IsActor()) {
            pactor = static_cast<Actor*>(pmob);
        }

        if (pactor) {
            if (pactor->IsInCombatWithActor(apActor)) {
                pactor->StopCombat(apActor);
            }
            else if (
                pactor && !abCombatOnly && pactor->IsAlarmed() && pactor->GetCurrentPackage()->GetPackageTarget()
            ) {
                if (pactor->GetCurrentPackage()->GetPackageTarget()->GetTargReference() == apActor || !apActor) {
                    pactor->EndInterruptPackage(false);
                    pactor->pCurrentProcess->ClearActionHeadTrackTarget(true);
                }
            }
            if (apActor == pPlayer) {
                pactor->RemoveEnemyFlagFromAllFactions();
                pactor->m_Extra.RemovePlayerCrimeListExtra(pactor);
                if (pactor->IsCreature()) {
                    TESFaction* pmyFactionArray[128];
                    BSSimpleList<FACTION_RANK*>* pmyFactionList = &pactor->GetActorBase()->listFactions;
                    ExtraFactionChanges* pmyFactionChanges = pactor->m_Extra.GetFactionChangesExtra();
                    BSSimpleList<FACTION_RANK*>* pmyChangesList = pmyFactionChanges ? pmyFactionChanges->pFactionChanges
                                                                                    : nullptr;
                    uint32_t
                        imyCount = pactor->IntegrateFactionLists(pmyFactionArray, 128u, pmyFactionList, pmyChangesList);
                    for (uint32_t uiFaction = 0; uiFaction < imyCount; ++uiFaction) {
                        if (pmyFactionArray[uiFaction] && pmyFactionArray[uiFaction]->GetPCAnEnemy()) {
                            pmyFactionArray[uiFaction]->SetPCAnEnemy(false);
                        }
                    }
                }
            }
        }
    }
}

void ProcessLists::UpdateTheCrimeList() {
    for (int32_t i = 0; i < 5; ++i) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[i];
        BSSimpleList<Crime*>* pitem = pGlobalCrimeList;
        while (pitem && pitem->m_item) {
            Crime* pcrime = pitem->m_item;
            if (pcrime->ShouldCrimeBeRemoved()) {
                pcrime->ClearActorKnowList();
                pGlobalCrimeList->Remove(pcrime);
                if (pcrime) {
                    delete pcrime;
                }
                pitem = pGlobalCrimeList;
            }
            else {
                pitem = pitem->m_pkNext;
            }
        }
    }
    fCrimeUpdateTimer = static_cast<float>(static_cast<int32_t>(iNumberHighActors) + 10.0);
}

void ProcessLists::SendGrenadeMineAlert(TESObjectREFR* apShooter, Projectile* apProjectile) {
    ;
}

void ProcessLists::UpdateMiddleHighList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* pTempList;
    MobileObject* pmob = nullptr;
    pTempList = new BSSimpleList<MobileObject*>;
    uint32_t ilow = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
    for (uint32_t i = ilow; i < ihigh; ++i) {
        pmob = MobProcessArray.GetItem(i);
        if (pmob && !pmob->IsStillLoading()) {
            if (pmob->GetDisabled() || pmob->GetDelete() || pmob->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_HIGH) {
                pTempList->AddHead(pmob);
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
                            fhour < static_cast<double>(flastprocesstime) ||
                            afDelta != 0.0) {
                            if (pmob->pCurrentProcess->CheckCombatDelayTimer()) {
                                pmob->Process(afDelta);
                                if (Script::GetProcessScripts()) {
                                    pmob->RunScript();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for (BSSimpleList<MobileObject*>* plisttemp = pTempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_MIDDLE_HIGH);
        if (pmob->IsTransient()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            if (pmob) {
                delete pmob;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        else if (pmob->GetDisabled()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            BaseProcess* pproc = pmob->pCurrentProcess;
            if (pproc) {
                delete pproc;
            }
            pmob->SetCurrentAIProcess(nullptr);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    pTempList->RemoveAll();
    if (pTempList) {
        delete pTempList;
    }
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::SendActorsYellAlarm(Crime* apCrime, BSSimpleList<TESObjectREFR*>* apDoorList) {
    BSSimpleList<TESObjectREFR*>* preflist = new BSSimpleList<TESObjectREFR*>;
    Actor* apCriminal = apCrime->pcriminal;
    TESObjectCELL* pcrimcell = apCriminal->GetParentCell();
    TESWorldSpace* pcrimworld = apCriminal->GetWorldSpace();
    apCrime->breported = true;
    if (!apCriminal) {
        return preflist;
    }
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = nullptr;
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
        }

        BSSimpleList<Actor*>* ptemp = nullptr;
        while (pactor) {
            float fdistance = std::numeric_limits<float>::max();
            if (!pactor->IsDead(false) && !pactor->GetDisabled() && !pactor->GetDelete() && !pactor->IsDead(false)) {
                TESObjectCELL* pactorcell = pactor->GetParentCell();
                TESWorldSpace* pactorworld = pactor->GetWorldSpace();
                AlarmPackage* palarm = nullptr;
                TESPackage* pbasepack = pactor->GetCurrentPackage();
                if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_ALARM) {
                    palarm = static_cast<AlarmPackage*>(pbasepack);
                }

                if (!palarm || !palarm->IsCrimeInList(apCrime)) {
                    if ((pactorcell && pactorcell == pcrimcell) ||
                        (pcrimworld == pactorworld &&
                         ((pactorcell && !pactorcell->GetInterior()) || (pcrimcell && !pcrimcell->GetInterior())))) {
                        fdistance = pactor->GetDistanceFromReference(apCriminal, false, false);
                        double dRecDistance = static_cast<double>(iCrimeAlarmRecDistance.Int());
                        if (fdistance <= dRecDistance) {
                            preflist->AddHead(pactor);
                        }
                    }
                    else {
                        for (TESObjectREFR* pref : apDoorList->ItemsUntilNull()) {
                            if (pref) {
                                DoorTeleportData* pTel = pref->GetTeleport();
                                TESObjectREFR* pDoorRef = nullptr;
                                if (pTel) {
                                    TESWorldSpace* pDoorWorld;
                                    pDoorRef = pTel->pLinkedDoor;
                                    TESObjectCELL* pDoorCell = pDoorRef->GetParentCell();
                                    if (pDoorCell == pactorcell ||
                                        (!pDoorRef->GetParentCell() &&
                                         (pDoorWorld = pDoorRef->GetWorldSpace(),
                                          pDoorWorld == pactor->GetWorldSpace()))) {
                                        fdistance = pactor->GetDistanceFromReference(pDoorRef, false, false);
                                        double dRecDistance = static_cast<double>(iCrimeAlarmRecDistance.Int());
                                        if (fdistance <= dRecDistance) {
                                            preflist->AddHead(pactor);
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
                pactor = nullptr;
                if (ptemp->m_item->IsActor()) {
                    pactor = ptemp->m_item;
                }
                ptemp = ptemp->m_pkNext;
            }
            else {
                pactor = nullptr;
            }
        }
    }
    if (preflist) {
        preflist->Sort(&ProcessLists::DistanceListCompareFn);
    }
    return preflist;
}

void ProcessLists::UpdateLowList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* pTempList;
    _LARGE_INTEGER icurrentTime;
    _LARGE_INTEGER imaxTime;
    MobileObject* pmob = nullptr;
    pTempList = new BSSimpleList<MobileObject*>;
    QueryPerformanceCounter(&imaxTime);
    uint32_t uiMilliseconds = iLowProcessingMilliseconds.Int();
    imaxTime.QuadPart += uiMilliseconds * ProcessLists::ulTimerFrequency.QuadPart / 1000;
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
    if (!iArrayNumbStopped || iArrayNumbStopped >= ihigh) {
        iArrayNumbStopped = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_LOW);
    }
    int32_t iamount = ihigh;
    if (abSleeping) {
        iamount = ihigh;
    }
    float fcurrenthour = appCalendar.GetHour();
    bool bstopflag = false;
    for (uint32_t i = iArrayNumbStopped; i < iamount && !bstopflag; ++i) {
        pmob = MobProcessArray.GetItem(i);
        if (!abSleeping && !(i % 5)) {
            QueryPerformanceCounter(&icurrentTime);
            if (icurrentTime.QuadPart >= imaxTime.QuadPart) {
                bstopflag = true;
            }
        }
        if (pmob) {
            BaseProcess* pproc = pmob->pCurrentProcess;
            if (pproc && !pmob->IsStillLoading()) {
                if (pmob->GetDisabled() || pmob->GetDelete() || pproc->eLevel != PROCESS_TYPE_LOW) {
                    pTempList->AddHead(pmob);
                }
                else {
                    if (!ProcessLists::bIsForciblyUpdated) {
                        if (!pmob->GetRunsInLow() || pproc->fHourLastProcessed == -1.0) {
                            static_cast<LowProcess*>(pproc)->LowProcess::ComputeLastTimeProcessed();
                        }
                        else {
                            float flastprocesstime = pproc->fHourLastProcessed;
                            float fhour = fcurrenthour - flastprocesstime;
                            if ((fhour >= 1.0 || fhour < 0.0 || afDelta != 0.0) && pproc->CheckCombatDelayTimer()) {
                                pmob->Process(afDelta);
                                if (Script::GetProcessScripts()) {
                                    pmob->RunScript();
                                }
                            }
                        }
                    }
                }
            }
        }
        iArrayNumbStopped = i + 1;
    }
    for (BSSimpleList<MobileObject*>* plisttemp = pTempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_LOW);
        if (pmob->GetDisabled()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            BaseProcess* pproc = pmob->pCurrentProcess;
            if (pproc) {
                delete pproc;
            }
            pmob->SetCurrentAIProcess(nullptr);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    pTempList->RemoveAll();
    if (pTempList) {
        delete pTempList;
    }
}

void ProcessLists::UpdateMiddleLowList(float afDelta, bool abSleeping) {
    BSSimpleList<MobileObject*>* pTempList;
    MobileObject* pmob = nullptr;
    pTempList = new BSSimpleList<MobileObject*>;
    uint32_t ilow = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_LOW);
    uint32_t ihigh = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_LOW);
    for (uint32_t i = ilow; i < ihigh; ++i) {
        pmob = MobProcessArray.GetItem(i);
        Actor* pact = nullptr;
        if (pmob && !pmob->IsStillLoading()) {
            if (pmob->IsActor()) {
                pact = static_cast<Actor*>(pmob);
            }
            if (pmob->GetDisabled() || pmob->GetDelete() || pmob->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_LOW) {
                pTempList->AddHead(pmob);
            }
            else {
                if (!ProcessLists::bIsForciblyUpdated) {
                    if (pmob->GetRunsInLow() || !pact || pact->IsInCombat()) {
                        float flastprocesstime = pmob->pCurrentProcess->fHourLastProcessed;
                        float fhour = appCalendar.GetHour();
                        if (flastprocesstime == -1.0) {
                            pmob->InitLastTimeProcessed();
                        }
                        else {
                            if (fhour - flastprocesstime >= 0.30000001192092896 ||
                                fhour < static_cast<double>(flastprocesstime) ||
                                afDelta != 0.0) {
                                if (pmob->pCurrentProcess->CheckCombatDelayTimer()) {
                                    pmob->Process(afDelta);
                                    if (Script::GetProcessScripts()) {
                                        pmob->RunScript();
                                    }
                                }
                            }
                        }
                    }
                    else {
                        pmob->pCurrentProcess->ComputeLastTimeProcessed();
                    }
                }
            }
        }
    }
    for (BSSimpleList<MobileObject*>* plisttemp = pTempList; plisttemp && plisttemp->m_item;
         plisttemp = plisttemp->m_pkNext) {
        pmob = plisttemp->m_item;
        MobProcessArray.RemoveActor(pmob, PROCESS_TYPE_MIDDLE_LOW);
        if (pmob->GetDisabled()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            BaseProcess* pproc = pmob->pCurrentProcess;
            if (pproc) {
                delete pproc;
            }
            pmob->SetCurrentAIProcess(nullptr);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    pTempList->RemoveAll();
    if (pTempList) {
        delete pTempList;
    }
}

void ProcessLists::ProcessHighActorsForFastTravel() {
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        PlayerCharacter* pActor = static_cast<PlayerCharacter*>(MobProcessArray.GetItem(uiIndex));
        if (pActor &&
            pActor->IsActor() &&
            !pActor->IsStillLoading() &&
            (pActor->GetParentCell() || pActor->GetWorldSpace()) &&
            pActor != pPlayer &&
            pActor->GetCurrentAIProcess() &&
            !pActor->GetDelete() &&
            !pActor->GetDisabled() &&
            !pActor->IsInCombat()) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            float fLastTimeProcessed = pProcess->GetLastTimeProcessed();
            float fCurrentHour = appCalendar.GetHour();
            float fDelta = fCurrentHour - fLastTimeProcessed;
            if (fLastTimeProcessed > static_cast<double>(fCurrentHour)) {
                fDelta = fCurrentHour + 24.0 - fLastTimeProcessed;
            }
            bool bIsEscort = false;
            if (fDelta < 0.05) {
                fDelta = 0.0;
            }
            if (pActor->GetCurrentAIProcess()) {
                pProcess = pActor->GetCurrentAIProcess();
                if (pProcess->GetCurrentPackage()) {
                    pProcess = pActor->GetCurrentAIProcess();
                    if (pProcess->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ESCORT) {
                        bIsEscort = true;
                    }
                }
            }
            if ((!bIsEscort && !pActor->IsFollowing()) || pActor->GetCurrentAIProcess()->GetTarget() != pPlayer) {

                pActor->Process(fDelta);
            }
        }
    }
}

Actor* ProcessLists::GetActorRefInHigh(TESFaction* apFaction, bool abDetection, bool abClosetoPlayer) {
    Actor* pFound = nullptr;
    float fClosestDistance = 10000.0f;
    Actor* pClosest = nullptr;
    PlayerCharacter* pCreature = nullptr;
    Actor* pClosestCreature = nullptr;
    for (uint32_t uiIndex = 0;
         (!pFound || !abClosetoPlayer) && uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++uiIndex) {
        PlayerCharacter* pActor = nullptr;
        Actor* pObject = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        if (pObject && pObject->IsActor()) {
            pActor = static_cast<PlayerCharacter*>(pObject);
        }

        if (pActor &&
            !pActor->IsDead(false) &&
            !pActor->IsUnConscious() &&
            pActor->GetActorBase()->GetFactionRank(apFaction, pActor == pPlayer) > -1) {
            if (!abDetection || pActor->pCurrentProcess->GetDetectionActor(pPlayer, 0) > 0) {
                pFound = pActor;
                if (abClosetoPlayer) {
                    float fDistance = pActor->GetDistanceFromReference(pPlayer, false, false);
                    if (fClosestDistance > static_cast<double>(fDistance)) {
                        pClosest = pActor;
                        if (pActor->IsCreature()) {
                            pClosestCreature = pActor;
                            pClosest = nullptr;
                        }
                        fClosestDistance = fDistance;
                    }
                }
                if (pActor->IsCreature()) {
                    pCreature = pActor;
                    pFound = nullptr;
                }
            }
        }
    }
    if (pFound || !pCreature) {
        if (abClosetoPlayer && pClosest) {
            return pClosest;
        }
    }
    else {
        pFound = pCreature;
        if (abClosetoPlayer) {
            return pClosestCreature;
        }
    }
    return pFound;
}

BSSimpleList<TESObjectREFR*>* ProcessLists::SendCrimetoHighList(Crime* apCrime) {
    BSSimpleList<TESObjectREFR*>* pRefList = nullptr;
    Actor* pcriminal = apCrime->pcriminal;
    if (pcriminal && (pcriminal->GetDisabled() || pcriminal->GetDelete() || pcriminal->IsDead(true))) {
        return nullptr;
    }
    int32_t iguards = 0;
    if (pcriminal) {
        iguards = pcriminal->pCurrentProcess->GetNumberGuardsArresting();
    }
    if (iguards > 0) {
        BSSimpleList<TESObjectREFR*>* pPersuelist = IsActorTargetingREFinPackage(pcriminal, PACKAGE_TYPE_ALARM, false);
        if (pPersuelist) {
            iguards = pPersuelist->ItemsInList();
        }
        else {
            iguards = 0;
        }
        pcriminal->pCurrentProcess->SetNumberGuardsArresting(iguards);
    }
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        Actor* pactor = nullptr;
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
        }

        if (pactor) {
            bool blinesight;
            if (!pactor->IsUnConscious() &&
                !pactor->IsParalyzed() &&
                pactor != pcriminal &&
                !pactor->IsDead(false) &&
                !pactor->GetDisabled() &&
                pactor->GetDetectionLevelAgainstActor(false, pcriminal, blinesight, false, false, false, nullptr) > 0) {
                if (!pRefList) {
                    pRefList = new BSSimpleList<TESObjectREFR*>;
                }
                pRefList->AddHead(pactor);
            }
        }
    }
    if (pRefList) {
        pRefList->Sort(&ProcessLists::DistanceListCompareFn);
    }
    return pRefList;
}

BSSimpleList<TESObjectREFR*>*
ProcessLists::SendResistArresttoGuards(Actor* apActor, BSSimpleList<TESObjectREFR*>* apDoorList) {
    BSSimpleList<TESObjectREFR*>* preflist = new BSSimpleList<TESObjectREFR*>;
    TESObjectCELL* pcrimcell = apActor->GetParentCell();
    TESWorldSpace* pcrimworld = apActor->GetWorldSpace();
    if (!apActor) {
        return preflist;
    }
    int32_t iamount = pPlayer->pCurrentProcess->GetNumberGuardsArresting();
    pPlayer->pCurrentProcess->ModNumberGuardsArresting(1 - iamount);
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            Actor* pactor = nullptr;
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            float fdistance = std::numeric_limits<float>::max();
            BSSimpleList<Actor*>* ptemp = nullptr;
            while (pactor) {
                TESObjectCELL* pactorcell = pactor->GetParentCell();
                TESWorldSpace* pactorworld = pactor->GetWorldSpace();
                if (!pactor->IsInCombat() && pactor->IsActoraGuard()) {
                    if ((pactorcell && pactorcell->GetInterior() && pactorcell == pcrimcell) ||
                        (pcrimworld == pactorworld &&
                         ((pactorcell && !pactorcell->GetInterior()) || (pcrimcell && !pcrimcell->GetInterior())))) {
                        fdistance = pactor->GetDistanceFromReference(apActor, false, false);
                        double dRecDistance = static_cast<double>(iCrimeAlarmRecDistance.Int());
                        if (fdistance <= dRecDistance) {
                            pactor->pCurrentProcess->EnterCombat(
                                pactor,
                                apActor,
                                false,
                                false,
                                nullptr,
                                false,
                                false,
                                false,
                                0,
                                false,
                                false,
                                true,
                                nullptr
                            );
                            preflist->AddHead(pactor);
                        }
                    }
                    else {
                        for (TESObjectREFR* pref : apDoorList->ItemsUntilNull()) {
                            if (pref) {
                                DoorTeleportData* pTel = pref->GetTeleport();
                                TESObjectREFR* pDoorRef = nullptr;
                                if (pTel) {
                                    TESWorldSpace* pDoorWorld;
                                    pDoorRef = pTel->pLinkedDoor;
                                    TESObjectCELL* pDoorCell = pDoorRef->GetParentCell();
                                    if (pDoorCell == pactorcell ||
                                        (!pDoorRef->GetParentCell() &&
                                         (pDoorWorld = pDoorRef->GetWorldSpace(),
                                          pDoorWorld == pactor->GetWorldSpace()))) {
                                        fdistance = pactor->GetDistanceFromReference(pDoorRef, false, false);
                                        double dRecDistance = static_cast<double>(iCrimeAlarmRecDistance.Int());
                                        if (fdistance <= dRecDistance) {
                                            pactor->pCurrentProcess->EnterCombat(
                                                pactor,
                                                apActor,
                                                false,
                                                false,
                                                nullptr,
                                                false,
                                                false,
                                                false,
                                                0,
                                                false,
                                                false,
                                                true,
                                                nullptr
                                            );
                                            preflist->AddHead(pactor);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (ptemp && ptemp->m_item) {
                    pactor = nullptr;
                    if (ptemp->m_item->IsActor()) {
                        pactor = ptemp->m_item;
                    }
                    ptemp = ptemp->m_pkNext;
                }
                else {
                    pactor = nullptr;
                }
            }
        }
    }
    return preflist;
}

BSSimpleList<TESObjectREFR*>* ProcessLists::GetActorsProcessCrimeAlarm(Crime* apCrime, bool bdelete) {
    BSSimpleList<TESObjectREFR*>* pRefList = nullptr;
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        if (pMob) {
            Actor* pactor = nullptr;
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            if (pactor) {
                AlarmPackage* palarm = nullptr;
                TESPackage* pbasepack = pactor->GetCurrentPackage();
                if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_ALARM) {
                    palarm = static_cast<AlarmPackage*>(pbasepack);
                }

                if (palarm && palarm->IsCrimeInList(apCrime)) {
                    if (!pRefList) {
                        pRefList = new BSSimpleList<TESObjectREFR*>;
                    }
                    if (bdelete) {
                        palarm->RemoveCrime(apCrime, pactor);
                    }
                    else {
                        pRefList->AddHead(pactor);
                    }
                }
            }
        }
    }
    if (bdelete) {
        if (pRefList) {
            delete pRefList;
        }
        return nullptr;
    }
    return pRefList;
}

void ProcessLists::TurnOffMovementinHigh() {
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        Actor* pActor = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
        if (pActor) {
            pActor->SetMovementComplete(true);
            pActor->PickAnimations(1.0, 1.0);
        }
    }
}

BSSimpleList<Crime*>* ProcessLists::RemoveCrimesForYieldCheck(float afTime) {
    BSSimpleList<Crime*>* pReturnList;
    BSSimpleList<Crime*>* plist = GlobalCrimeListArray[3];
    pReturnList = new BSSimpleList<Crime*>;
    bool bflag = false;
    BSSimpleList<Crime*>* plastlist = nullptr;
    while (plist && !plist->IsEmpty() && !bflag) {
        Crime* pcrime = plist->m_item;
        if (!(static_cast<double>(afTime) >= pcrime->GetTimeStamp().ElapsedTime())) {
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            for (BSSimpleList<TESFaction*>* pfactlist = pcrime->pListofEnemyFactions;
                 pfactlist && !pfactlist->IsEmpty();
                 pfactlist = pfactlist->m_pkNext) {
                TESFaction* pfac = pfactlist->m_item;
                if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, pcrime)) {
                    pfac->SetPCAnEnemy(false);
                }
            }
            Actor* pact = static_cast<Actor*>(pcrime->pcrimetarget);
            if (pact && pact->IsActor()) {
                BSSimpleList<Crime*>* pcrimelist = pact->m_Extra.GetPlayerCrimeList();
                if (pcrimelist) {
                    pcrimelist->Remove(pcrime);
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
        return nullptr;
    }
    return pReturnList;
}

void ProcessLists::AddtoGlobalCrimeList(Crime* apCrime) {
    if (apCrime && apCrime->ecrime > PackageNS::CRIME_TYPE_NONE && apCrime->ecrime < PackageNS::CRIME_TYPE_COUNT) {
        BSSimpleList<Crime*>* pGlobalCrimeList = GlobalCrimeListArray[apCrime->ecrime];
        if (!pGlobalCrimeList) {
            pGlobalCrimeList = new BSSimpleList<Crime*>;
        }
        if (!pGlobalCrimeList->IsInList(apCrime)) {
            pGlobalCrimeList->AddHead(apCrime);
        }
        GlobalCrimeListArray[apCrime->ecrime] = pGlobalCrimeList;
    }
}

bool ProcessLists::IsEnemyFactionofOtherCrime(TESFaction* apFaction, Crime* apCrime) {
    BSSimpleList<Crime*>* plist = GlobalCrimeListArray[3];
    bool bflag = false;
    while (plist && !plist->IsEmpty() && !bflag) {
        Crime* pcrime = plist->m_item;
        if (pcrime != apCrime) {
            if (pcrime->pListofEnemyFactions) {
                bflag = pcrime->pListofEnemyFactions->IsInList(apFaction);
            }
        }
        plist = plist->m_pkNext;
    }
    if (!bflag) {
        plist = GlobalCrimeListArray[4];
        bool bflagSecond = false;
        while (plist && !plist->IsEmpty() && !bflagSecond) {
            Crime* pcrime = plist->m_item;
            if (pcrime != apCrime) {
                if (pcrime->pListofEnemyFactions) {
                    bflagSecond = pcrime->pListofEnemyFactions->IsInList(apFaction);
                }
            }
            plist = plist->m_pkNext;
        }
    }
    return bflag;
}

void ProcessLists::BuildFollowerListRecursive(Actor* apActor, BSSimpleList<Actor*>& aReturnList) {
    ExtraFollower* pfollow = apActor->m_Extra.GetFollowerExtra();
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
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        Actor* pObject = static_cast<Actor*>(MobProcessArray.GetItem(i));
        Actor* pactor = nullptr;
        if (pObject && pObject->IsActor()) {
            pactor = pObject;
            if (pObject->GetCurrentPackage()) {
                if ((pactor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ESCORT ||
                     pactor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ACCOMPANY) &&
                    pactor->GetCurrentPackageTarget() == apActor &&
                    !aReturnList.IsInList(pactor)) {
                    aReturnList.AddHead(pactor);
                    BuildFollowerListRecursive(pactor, aReturnList);
                }
            }
        }
    }
}

int32_t ProcessLists::GetHighestDetectionLevelForActor(Actor* apActor, bool abIncludeFollowers) {
    int32_t ilevel = -100;
    int32_t itotallevel = 0x7FFFFFFF;
    pPlayer->SetActorInSneakRange(false);
    for (uint32_t i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = MobProcessArray.GetItem(i);
        Actor* pactor = nullptr;
        if (pMob && pMob->IsActor()) {
            pactor = static_cast<Actor*>(pMob);
        }

        bool bskipThisOne = false;
        if (!abIncludeFollowers && pactor) {
            BSSimpleList<Actor*> followerList;
            BuildFollowerListRecursive(apActor, followerList);
            if (followerList.IsInList(pactor) || (apActor == pPlayer && pactor->bPlayerTeammate)) {
                bskipThisOne = true;
            }
        }

        if (!bskipThisOne && pactor && !pactor->IsDead(false) && !pactor->GetDisabled() && !pactor->IsUnConscious()) {
            FIGHT_REACTION ffight;
            if (apActor == pPlayer && !pPlayer->GetActorInSneakRange()) {
                float fdistance = apActor->GetDistanceFromReference(pactor, false, false);
                if (fdistance <= static_cast<double>(iSneakSkillUseDistance.Int())) {
                    pPlayer->SetActorInSneakRange(true);
                }
            }
            if (pactor->IsInCombat() || pactor->IsAlarmed()) {
                if (pactor->IsInCombat() &&
                    pactor->GetCombatController() &&
                    pactor->GetCombatController()->GetTarget() == apActor) {
                    CombatController* pcombatController = pactor->GetCombatController();
                    if (!static_cast<uint8_t>(pcombatController->IsNotDetectingTarget())) {
                        return 100;
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
            if (ilevel > 0 && pactor->GetShouldAttackActor(pPlayer, false, ffight, false)) {
                pPlayer->SetHostileDetection(true);
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
            Actor* pActor = nullptr;
            if (pObject && pObject->IsActor()) {
                pActor = static_cast<Actor*>(pObject);
            }

            if (pActor &&
                !pActor->IsDead(false) &&
                (pPlayer->IsActorFollowingEscortingPlayer(pActor, false) || pActor->IsPlayerTeammate())) {
                pActor->EndInterruptPackage(false);
                pActor->MoveActorAndFollowers(apMarker, false, false);
            }
        }
    }
}

void ProcessLists::RemoveParticleTempEffectsFor3D(NiAVObject* apFollow3D) {
    if (apFollow3D && !GlobalTempEffectList.IsEmpty()) {
        BSSimpleList<NiPointer<BSTempEffect>>* pelist = &GlobalTempEffectList;
        BSSimpleList<NiPointer<BSTempEffect>>* plastlist = nullptr;
        while (pelist && !pelist->IsEmpty()) {
            BSTempEffectParticle* ppart;
            BSTempEffect* peffect = pelist->m_item.m_pObject;
            if (peffect->GetType() == TEMP_EFFECT_PARTICLE &&
                (ppart = dynamic_cast<BSTempEffectParticle*>(peffect)) != nullptr &&
                ppart->GetFollowObject() == apFollow3D) {
                NiAVObject* ppartObj = ppart->GetParticleObject();
                if (ppartObj) {
                    ppartObj->SetAppCulled(true);
                }
                ppart->pCell = nullptr;
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

MagicShaderHitEffect* ProcessLists::FindActiveShaderHitEffect(TESObjectREFR* apEffectTarget) {
    MagicShaderHitEffect* pactiveEffect = nullptr;
    float effectAge = std::numeric_limits<float>::max();
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect &&
                NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                pshaderEffect->pTarget == apEffectTarget &&
                static_cast<double>(effectAge) > pshaderEffect->BSTempEffect::fAge &&
                !pshaderEffect->bWeaponEnchantment) {
                pactiveEffect = pshaderEffect;
                effectAge = pshaderEffect->BSTempEffect::fAge;
            }
        }
    }
    return pactiveEffect;
}

MagicShaderHitEffect*
ProcessLists::FindAndCleanupWeaponShaderHitEffect(TESObjectREFR* apEffectTarget, TESEffectShader* apShader) {
    MagicShaderHitEffect* pactiveEffect = nullptr;
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect &&
                NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                pshaderEffect->pTarget == apEffectTarget &&
                pshaderEffect->bWeaponEnchantment &&
                !pshaderEffect->bFinished) {
                if (pshaderEffect->pEffectData != apShader || pactiveEffect) {
                    pshaderEffect->SetFinished(true);
                }
                else {
                    pactiveEffect = pshaderEffect;
                }
            }
        }
    }
    return pactiveEffect;
}

uint32_t ProcessLists::EndAllNonPlayerConversations() {
    uint32_t uiTail;
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        uiTail = MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (uiIndex >= uiTail) {
            break;
        }
        TESForm* pForm = MobProcessArray.GetItem(uiIndex);
        MobileObject* pMob = nullptr;
        if (pForm && pForm->IsActor()) {
            pMob = static_cast<MobileObject*>(pForm);
        }

        if (pMob && pMob->IsinDialogue() && pMob->GetCurrentTarget() != pPlayer) {
            pMob->EndDialogue();
        }
    }
    return uiTail;
}

// Ends every high-process magic and arrow projectile (virtual SetDelete(true)), kills the other projectiles without an
// alt-trigger or detonating explosion, then ends the middle-high arrows.
int32_t ProcessLists::GetActiveMagicProjectileCount() {
    int32_t icount;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        MobileObject* pMob = MobProcessArray.GetItem(uiIndex);
        MagicProjectile* pmagicProjectile = dynamic_cast<MagicProjectile*>(pMob);
        if (pmagicProjectile) {
            pmagicProjectile->SetDelete(true);
        }
        else {
            ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(pMob);
            if (parrow) {
                parrow->SetDelete(true);
            }
            else {
                Projectile* pProjectile = dynamic_cast<Projectile*>(pMob);
                if (pProjectile &&
                    (!pProjectile->GetProjectileBase() ||
                     (!pProjectile->GetProjectileBase()->GetHasExplosionAltTrigger() &&
                      !pProjectile->GetProjectileBase()->GetHasExplosionDetonates()))) {
                    pProjectile->Kill();
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
        BSSimpleList<Crime*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            Crime* pcrime = plist->m_item;
            bool badvanced = false;
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
                badvanced = true;
            }
            else if (apRef->IsActor()) {
                pcrime->RemoveActorKnowList(static_cast<Actor*>(apRef));
            }
            if (!badvanced) {
                plastlist = plist;
                plist = plist->m_pkNext;
            }
        }
    }
}

int16_t ProcessLists::GetCrimesSaveSize() {
    int16_t sSize = 4;
    for (BSSimpleList<Crime*>* pNode : GlobalCrimeListArray) {
        sSize += 2;
        while (pNode && !pNode->IsEmpty()) {
            sSize += pNode->m_item->GetSaveSize();
            pNode = pNode->m_pkNext;
        }
    }
    return sSize;
}

void ProcessLists::SaveGame_Crimes() {
    pSaveLoadGameOLD->SaveGameDataOLD(&ProcessLists::fSystemTimeClock, sizeof(ProcessLists::fSystemTimeClock));
    for (int32_t i = 0; i < 5; ++i) {
        uint16_t usCount = 0;
        uint16_t* pusCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
        for (BSSimpleList<Crime*>* pNode = GlobalCrimeListArray[i]; pNode && !pNode->IsEmpty();
             pNode = pNode->m_pkNext) {
            pNode->m_item->SaveGame();
            ++usCount;
        }
        *pusCount = usCount;
    }
}

void ProcessLists::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&fPlayerActionCommentTimer), sizeof(fPlayerActionCommentTimer), 0);
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&fPlayerKnockObjectCommentTimer),
        sizeof(fPlayerKnockObjectCommentTimer),
        0
    );
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCrimeUpdateTimer), sizeof(fCrimeUpdateTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCrimeNumber), sizeof(iCrimeNumber), 0);
    for (uint32_t i = 0; i < 5; ++i) {
        uint32_t aiVariableValue = 0;
        uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
        for (BSSimpleList<Crime*>* pNode = GlobalCrimeListArray[i]; pNode; pNode = pNode->m_pkNext) {
            Crime* pCrime = pNode->m_item;
            if (pCrime) {
                pCrime->Save(apSaveGameBuffer);
                ++aiVariableValue;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    }
}

void ProcessLists::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPlayerActionCommentTimer), sizeof(fPlayerActionCommentTimer));
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&fPlayerKnockObjectCommentTimer),
            sizeof(fPlayerKnockObjectCommentTimer)
        );
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCrimeUpdateTimer), sizeof(fCrimeUpdateTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCrimeNumber), sizeof(iCrimeNumber));
    for (uint32_t i = 0; i < 5; ++i) {
        uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
        if (uiCount) {
            if (GlobalCrimeListArray[i]) {
                GlobalCrimeListArray[i]->RemoveAll();
            }
            else {
                BSSimpleList<Crime*>* pCrimeList = new BSSimpleList<Crime*>;
                GlobalCrimeListArray[i] = pCrimeList;
            }
            for (uint32_t j = 0; j < uiCount; ++j) {
                Crime* pCrime = new Crime;
                pCrime->LoadGame(apLoadGameBuffer);
                GlobalCrimeListArray[i]->AddHead(pCrime);
            }
        }
    }
}

void ProcessLists::FlushNonPersistActors(int32_t aeMode) {
    BSSimpleList<void*>* pFarActors;
    BSSimpleList<void*>* pCloseActors;
    BSSimpleList<void*>* pInner;
    pCloseActors = new BSSimpleList<void*>;
    pFarActors = new BSSimpleList<void*>;
    TESObjectREFR* pActor = nullptr;
    TESObjectREFR* pOther = nullptr;
    MobileObject* pMobileObject = nullptr;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        pMobileObject = MobProcessArray.GetItem(uiIndex);
        pActor = nullptr;
        if (pMobileObject && pMobileObject->IsActor()) {
            pActor = pMobileObject;
        }

        if (pActor && !pActor->GetPersistFlagValue()) {
            if (aeMode == 2) {
                ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                if (pActor) {
                    delete pActor;
                }
                ReferenceProcessLists.kUpdateLock.Unlock();
                uiIndex = 0;
            }
            else if (!(pActor->GetDistanceFromReference(pPlayer, false, false) > 3000.0)) {
                pCloseActors->AddHead(pActor);
            }
            else {
                pFarActors->AddHead(pActor);
            }
        }
    }
    if (aeMode == 1) {
        pInner = nullptr;
        BSSimpleList<void*>* pIter = nullptr;
        bool bOnlyClose = false;
        if (pFarActors->IsEmpty()) {
            pIter = pCloseActors;
            bOnlyClose = true;
        }
        else {
            pIter = pFarActors;
        }
        while (pIter && pIter->m_item) {
            pActor = static_cast<TESObjectREFR*>(pIter->m_item);
            if (pActor->IsCreature() && !bOnlyClose) {
                pInner = pCloseActors;
                while (pInner && pInner->m_item) {
                    pOther = static_cast<TESObjectREFR*>(pInner->m_item);
                    const char* pModel = pActor->GetModel();
                    const char* pOtherModel = pOther->GetModel();
                    if (BSstricmp(pOtherModel, pModel)) {
                        pInner = pInner->m_pkNext;
                    }
                    else {
                        pCloseActors->Remove(pOther);
                        pInner->Remove(pOther);
                        pInner = pCloseActors;
                        ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                        if (pOther) {
                            delete pOther;
                        }
                        ReferenceProcessLists.kUpdateLock.Unlock();
                    }
                }
            }
            pIter->RemoveHead();
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            if (pActor) {
                delete pActor;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    else if (!aeMode) {
        while (pFarActors && pFarActors->m_item) {
            pActor = static_cast<TESObjectREFR*>(pFarActors->m_item);
            pFarActors->RemoveHead();
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            if (pActor) {
                delete pActor;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
    }
    pFarActors->RemoveAll();
    if (pFarActors) {
        delete pFarActors;
    }
    pCloseActors->RemoveAll();
}

void ProcessLists::ReferDestructorNullOut(TESObjectREFR* apActor) {
    Actor* pFollower;
    uint32_t i;
    pCombatManager->ClearReferencePointers(apActor);
    if (apActor->IsActor()) {
        if (pPlayer->m_Extra.GetFollowerExtra() && pPlayer->m_Extra.GetFollowerExtra()->pActorlist) {
            pFollower = static_cast<Actor*>(apActor);
            pPlayer->m_Extra.GetFollowerExtra()->pActorlist->Remove(pFollower);
        }

        if (pPlayer->pCurrentProcess) {
            pPlayer->pCurrentProcess->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
        }
        if (pPlayer->GetDialogueItemTarget() == apActor) {
            pPlayer->SetDialogueItemTarget(nullptr);
        }
    }
    TESObjectREFR* pMob = nullptr;
    for (i = 0; i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW); ++i) {
        pMob = MobProcessArray.GetItem(i);
        if (pMob && !pMob->IsStillLoading()) {
            Actor* pactor = nullptr;
            if (pMob->IsActor()) {
                pactor = static_cast<Actor*>(pMob);
            }
            if (pactor) {
                BaseProcess* pproc = pactor->pCurrentProcess;
                if (pactor->GetDialogueItemTarget() == apActor) {
                    pactor->SetDialogueItemTarget(nullptr);
                    if (pproc) {
                        pproc->SetGreetingFlag(false);
                        pproc->ClearMovementStoped();
                    }
                }
                if (pproc) {
                    TESPackage* pCurrentrunning = pactor->pCurrentProcess->GetPackageThatIsRunning();
                    TESPackage* pcurrentpack = pactor->GetPackageSetAsPcurrent();
                    if (pCurrentrunning &&
                        pCurrentrunning->GetIsCreated() &&
                        pCurrentrunning->GetPackType() != PACKAGE_TYPE_COMBAT) {
                        if (pCurrentrunning->GetPackageTarget() &&
                            pCurrentrunning->GetPackageTarget()->GetTargReference() == apActor) {
                            pCurrentrunning->SetPackageTarget(nullptr);
                        }

                        if (pCurrentrunning->GetLocationReference(pactor) == apActor) {
                            pCurrentrunning->SetPackageLocation(nullptr);
                        }
                    }

                    if (pcurrentpack &&
                        pcurrentpack->GetIsCreated() &&
                        pcurrentpack->GetPackType() != PACKAGE_TYPE_COMBAT) {
                        if (pcurrentpack->GetPackageTarget() &&
                            pcurrentpack->GetPackageTarget()->GetTargReference() == apActor) {
                            pcurrentpack->SetPackageTarget(nullptr);
                        }

                        if (pcurrentpack->GetLocationReference(pactor) == apActor) {
                            pcurrentpack->SetPackageLocation(nullptr);
                        }
                    }

                    if (i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH)) {
                        pproc->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
                        if (pactor->GetCurrentPackageTarget() == apActor ||
                            (pproc->GetRunOncePackage() &&
                             pproc->GetRunOncePackage()->GetPackageTarget() &&
                             pproc->GetRunOncePackage()->GetPackageTarget()->GetTargReference() == apActor)) {
                            if (pactor->IsinDialogue()) {
                                pactor->EndDialogue();
                            }
                            else if (pactor->GetCurrentPackage() && pactor->GetCurrentPackage()->GetIsCreated()) {
                                pactor->EndInterruptPackage(false);
                            }
                            else {
                                pproc->ClearCurrentPackage();
                                pproc->ClearRunOncePackage();
                            }
                        }
                        MagicCaster* pcast = apActor->GetMagicCaster();
                        pactor->CheckDeletedCaster(pcast);
                        if (i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
                            pproc->RemoveHeadTrackTarget(apActor);
                        }
                    }
                    pproc->ClearObjectFromAcquireList(apActor);
                    pproc->ClearPathLookTarget(apActor);
                    ExtraPackage* pextrapack = static_cast<ExtraPackage*>(pactor->m_Extra.GetExtraData(25));
                    if (pextrapack && pextrapack->pTarg == apActor) {
                        pextrapack->pTarg = nullptr;
                    }

                    if (pactor->pMyKiller == apActor) {
                        pactor->pMyKiller = nullptr;
                    }
                    if (pactor->IsFleeing(false)) {
                        TESPackage* pbasepack = pactor->GetCurrentPackage();
                        TESPackage* pfleePack = nullptr;
                        if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                            pfleePack = pbasepack;
                        }

                        if (pfleePack && static_cast<FleePackage*>(pfleePack)->pRefFleeTo == apActor) {
                            static_cast<FleePackage*>(pfleePack)->SetRefFleeTo(nullptr);
                        }

                        pactor->RemoveFleeTarget(apActor);
                    }
                    if (pactor->IsInCombat()) {
                        CombatController* pcombat = pactor->GetCombatController();
                        if (pcombat) {
                            pcombat->RemoveTarget(static_cast<Actor*>(apActor));
                        }
                    }
                    pactor->m_Extra.RemoveFollower(static_cast<Actor*>(apActor));
                    if (pproc->GetTarget() == apActor) {
                        pproc->SetTarget(nullptr);
                    }
                    if (pactor->GetCurrentTarget() == apActor) {
                        pactor->SetCurrentTarget(nullptr);
                    }
                }
            }
            else if (pMob->IsProjectile()) {
                Projectile* pprojectile = static_cast<Projectile*>(pMob);
                pprojectile->Projectile::CheckRefDeleted(apActor);
                if (pprojectile->GetDisabled()) {
                    if (static_cast<Projectile*>(MobProcessArray.GetItem(i)) != pprojectile) {
                        --i;
                    }
                }
            }
            else if (pMob->IsExplosion()) {
                static_cast<Explosion*>(pMob)->Explosion::CheckRefDeleted(apActor);
            }
        }
    }
    for (BSSimpleList<void*>* plist = &TempShouldMoveList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor = nullptr;
        if (static_cast<TESObjectREFR*>(plist->m_item)->IsActor()) {
            pactor = static_cast<Actor*>(static_cast<TESObjectREFR*>(plist->m_item));
        }
        if (!pactor || pactor->IsStillLoading()) {
            if (pMob->IsProjectile()) {
                static_cast<Projectile*>(pMob)->Projectile::CheckRefDeleted(apActor);
            }
            else if (pMob->IsExplosion()) {
                static_cast<Explosion*>(pMob)->Explosion::CheckRefDeleted(apActor);
            }
        }
        else {
            BaseProcess* pproc = pactor->pCurrentProcess;
            if (pactor->GetDialogueItemTarget() == apActor) {
                pactor->SetDialogueItemTarget(nullptr);
                if (pproc) {
                    pproc->SetGreetingFlag(false);
                    pproc->ClearMovementStoped();
                }
            }
            if (pproc) {
                TESPackage* pCurrentrunning = pactor->pCurrentProcess->GetPackageThatIsRunning();
                TESPackage* pcurrentpack = pactor->GetPackageSetAsPcurrent();
                if (pCurrentrunning &&
                    pCurrentrunning->GetIsCreated() &&
                    pCurrentrunning->GetPackType() != PACKAGE_TYPE_COMBAT) {
                    if (pCurrentrunning->GetPackageTarget() &&
                        pCurrentrunning->GetPackageTarget()->GetTargReference() == apActor) {
                        pCurrentrunning->SetPackageTarget(nullptr);
                    }

                    if (pCurrentrunning->GetLocationReference(pactor) == apActor) {
                        pCurrentrunning->SetPackageLocation(nullptr);
                    }
                }

                if (pcurrentpack &&
                    pcurrentpack->GetIsCreated() &&
                    pcurrentpack->GetPackType() != PACKAGE_TYPE_COMBAT) {
                    if (pcurrentpack->GetPackageTarget() &&
                        pcurrentpack->GetPackageTarget()->GetTargReference() == apActor) {
                        pcurrentpack->SetPackageTarget(nullptr);
                    }

                    if (pcurrentpack->GetLocationReference(pactor) == apActor) {
                        pcurrentpack->SetPackageLocation(nullptr);
                    }
                }

                if (i < 2) {
                    pproc->RemoveDetectionActor(static_cast<Actor*>(apActor), 3);
                    if (pactor->GetCurrentPackageTarget() == apActor ||
                        (pproc->GetRunOncePackage() &&
                         pproc->GetRunOncePackage()->GetPackageTarget() &&
                         pproc->GetRunOncePackage()->GetPackageTarget()->GetTargReference() == apActor)) {
                        if (pactor->IsinDialogue()) {
                            pactor->EndDialogue();
                        }
                        else if (pactor->GetCurrentPackage()->GetIsCreated()) {
                            pactor->EndInterruptPackage(false);
                        }
                        else {
                            pproc->ClearCurrentPackage();
                            pproc->ClearRunOncePackage();
                        }
                    }
                    MagicCaster* pcast = apActor->GetMagicCaster();
                    pactor->CheckDeletedCaster(pcast);
                    if (!i) {
                        pproc->RemoveHeadTrackTarget(apActor);
                    }
                }
                pproc->ClearObjectFromAcquireList(apActor);
                ExtraPackage* pextrapack = static_cast<ExtraPackage*>(pactor->m_Extra.GetExtraData(25));
                if (pextrapack && pextrapack->pTarg == apActor) {
                    pextrapack->pTarg = nullptr;
                }

                if (pactor->pMyKiller == apActor) {
                    pactor->pMyKiller = nullptr;
                }
                if (pactor->IsFleeing(false)) {
                    TESPackage* pbasepack = pactor->GetCurrentPackage();
                    TESPackage* pfleePack = nullptr;
                    if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
                        pfleePack = pbasepack;
                    }

                    if (pfleePack && static_cast<FleePackage*>(pfleePack)->pRefFleeTo == apActor) {
                        static_cast<FleePackage*>(pfleePack)->SetRefFleeTo(nullptr);
                    }

                    pactor->RemoveFleeTarget(apActor);
                }
                if (pactor->IsInCombat()) {
                    CombatController* pcombat = pactor->GetCombatController();
                    if (pcombat) {
                        pcombat->RemoveTarget(static_cast<Actor*>(apActor));
                    }
                }
                pactor->m_Extra.RemoveFollower(static_cast<Actor*>(apActor));
                if (pproc->GetTarget() == apActor) {
                    pproc->SetTarget(nullptr);
                }
                if (pactor->GetCurrentTarget() == apActor) {
                    pactor->SetCurrentTarget(nullptr);
                }
            }
        }
    }
    RemoveActorCloseToPlayer(static_cast<Actor*>(apActor));
}

void ProcessLists::DisableExcessDeadActors(float afDeltaTime) {
    fRemoveExcessDeadTimer = fRemoveExcessDeadTimer - afDeltaTime;
    if (fRemoveExcessDeadTimer <= 0.0) {
        int32_t* piMaxDead;
        int32_t iactorsTotal;
        float fTimer;
        uint32_t uiDeadCount = 0;
        uint32_t uiActorCount = 0;
        BSSimpleList<Actor*> deadActors;
        for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
            Actor* pactor_item;
            Actor* pactor = static_cast<Actor*>(MobProcessArray.GetItem(uiIndex));
            pactor_item = nullptr;
            if (pactor && pactor->IsActor() && !pactor->IsStillLoading()) {
                ++uiActorCount;
                pactor_item = pactor;
                if (pactor->GetLifeState() == ACTOR_LIFE_STATE_DEAD) {
                    BaseProcess* pprocess = pactor_item->pCurrentProcess;
                    if (pprocess &&
                        !pprocess->eLevel &&
                        pprocess->GetFadeState() == BaseProcess::FADE_NORMAL &&
                        !pactor_item->IsQuestObject() &&
                        !pactor_item->HasQuestObject() &&
                        !pactor_item->TESObjectREFR::IsDead(false) &&
                        pprocess->GetGreetingTimer() < 0.0 &&
                        (!pactor_item->GetEnableStateChildList() ||
                         pactor_item->GetEnableStateChildList()->IsEmpty())) {
                        ++uiDeadCount;
                        deadActors.AddTail(pactor_item);
                    }
                }
            }
        }
        bool bcomplex = Main::GetSceneIsComplex();
        if (uiActorCount < 25) {
            bcomplex = false;
        }
        if (bcomplex) {
            fTimer = fRemoveExcessDeadComplexTime.Float();
        }
        else {
            fTimer = fRemoveExcessDeadTime.Float();
        }
        fRemoveExcessDeadTimer = fTimer;
        if (bcomplex) {
            piMaxDead = &iRemoveExcessDeadComplexCount.Int();
        }
        else {
            piMaxDead = &iRemoveExcessDeadCount.Int();
        }
        uint32_t uiMaxDead = *piMaxDead;
        if (bcomplex) {
            iactorsTotal = iRemoveExcessDeadComplexTotalActorCount.Int();
        }
        else {
            iactorsTotal = iRemoveExcessDeadTotalActorCount.Int();
        }
        uint32_t uiMinActors = iactorsTotal;
        if (uiDeadCount >= uiMaxDead && uiActorCount >= uiMinActors) {
            if (bcomplex) {
                for (BSSimpleList<Actor*>* plist = &deadActors; plist && !plist->IsEmpty() && uiDeadCount >= uiMaxDead;
                     plist = plist->m_pkNext) {
                    Actor* pactor = plist->m_item;
                    if (pactor) {
                        static_cast<HighProcess*>(pactor->pCurrentProcess)->HighProcess::FadeAndDisable(pactor);
                        --uiDeadCount;
                    }
                }
            }
            else {
                Actor* poldestDeadActor = nullptr;
                float foldestDeadTime = std::numeric_limits<float>::max();
                for (BSSimpleList<Actor*>* plist = &deadActors; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
                    Actor* pactor = plist->m_item;
                    if (pactor) {
                        if (!poldestDeadActor ||
                            static_cast<double>(foldestDeadTime) > pactor->pCurrentProcess->GetGreetingTimer()) {
                            poldestDeadActor = pactor;
                            foldestDeadTime = pactor->pCurrentProcess->GetGreetingTimer();
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
        TESForm* pForm = MobProcessArray.GetItem(uiIndex);
        Actor* pActor = nullptr;
        if (pForm && pForm->IsActor()) {
            pActor = static_cast<Actor*>(pForm);
        }

        if (pActor) {
            pActor->UnloadPermanentMagic();
        }
    }
    for (uint32_t i = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
         i < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         ++i) {
        TESForm* pForm = MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pForm && pForm->IsActor()) {
            pActor = static_cast<Actor*>(pForm);
        }

        if (pActor) {
            pActor->UnloadPermanentMagic();
        }
    }
    if (pPlayer) {
        pPlayer->UnloadPermanentMagic();
    }
}

void ProcessLists::RebuildActorsCloseToPlayer() {
    kNearbyActorLock.Lock(nullptr);
    iActorsCloseToPlayer = 0;
    fillchar(pActorsCloseToPlayerA, 0, 200u);
    kNearbyActorLock.Unlock();
    Actor* pActor = nullptr;
    for (uint32_t uiIndex = 0; uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        TESForm* pForm = MobProcessArray.GetItem(uiIndex);
        if (pForm && pForm->IsActor()) {
            pActor = static_cast<Actor*>(pForm);
        }

        if (pActor) {
            float fDistance = pActor->GetDistanceFromReference(pPlayer, false, false);
            if (fShaderShadowUpdateDistance.Float() > static_cast<double>(fDistance)) {
                ReferenceProcessLists.InsertActorCloseToPlayer(pActor);
            }
        }
    }
    ReferenceProcessLists.SortActorsCloseToPlayer();
}

void ProcessLists::UpdateHighListAnimation(float afDelta, bool abSleeping) {
    for (uint32_t uiIndex = MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         uiIndex < MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++uiIndex) {
        MobileObject* pMob = MobProcessArray.GetItem(uiIndex);
        if (pMob && pMob->IsActor()) {
            pMob->UpdateAnimation(afDelta);
        }
    }
}

void ProcessLists::RemoveReferenceMuzzleFlash(TESObjectREFR* apParentRef) {
    MuzzleFlash* pflash;
    if (!ReferenceMuzzleFlashList.IsEmpty()) {
        BSSimpleList<void*>* plist = &ReferenceMuzzleFlashList;
        bool bfirst = true;
        while (plist) {
            pflash = static_cast<MuzzleFlash*>(plist->m_item);
            if (!bfirst) {
                plist = plist->m_pkNext;
            }
            bool bremove = false;
            if (pflash && pflash->pParentReference == apParentRef) {
                ReferenceMuzzleFlashList.Remove(*reinterpret_cast<void**>(&pflash));
                if (pflash) {
                    delete pflash;
                }
                bremove = true;
            }

            if (!bremove && bfirst) {
                plist = plist->m_pkNext;
                bfirst = false;
            }
        }
    }
}

void ProcessLists::SpawnReferenceMuzzleFlash(Projectile* apProjectile, TESObjectREFR* apParentRef) {
    MuzzleFlash* pcurrentFlash;
    if (apParentRef && apProjectile) {
        pcurrentFlash = nullptr;
        for (BSSimpleList<void*>* plist = &ReferenceMuzzleFlashList; !pcurrentFlash && plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            MuzzleFlash* pflash = static_cast<MuzzleFlash*>(plist->m_item);
            if (pflash) {
                BGSProjectile* pFlashProjectile = pflash->pProjectile;
                if (pFlashProjectile == apProjectile->GetProjectileBase()) {
                    if (pflash->pWeaponSource == apProjectile->pWeaponSource &&
                        pflash->pParentReference == apParentRef) {
                        pcurrentFlash = pflash;
                    }
                }
            }
        }
        if (!pcurrentFlash) {
            pcurrentFlash = new MuzzleFlash(apProjectile, apParentRef);
            apParentRef->SetTargeted(true);
            ReferenceMuzzleFlashList.AddHead(pcurrentFlash);
        }
        if (pcurrentFlash) {
            pcurrentFlash->Enable();
        }
    }
}

// aeType 5 (CRIME_TYPE_COUNT), one past the lists, is deliberately accepted: it then reads the
// pointer after GlobalCrimeListArray, the head of GlobalTempEffectList.
Crime* ProcessLists::GetCrimeinGlobalCrimeList(PackageNS::CRIME_TYPE aeType, int32_t aiCrime) {
    if (aeType <= PackageNS::CRIME_TYPE_NONE || aeType > PackageNS::CRIME_TYPE_COUNT) {
        return nullptr;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    Crime* preturncrime = nullptr;
    while (pitem && pitem->m_item && !preturncrime) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->iCrimeNumber == aiCrime) {
            preturncrime = pcrime;
        }
    }
    return preturncrime;
}

// Accepts aeType 5, one past the lists, as the other overload does; this is deliberate.
Crime*
ProcessLists::GetCrimeinGlobalCrimeList(Actor* apCriminal, TESObjectREFR* apVictim, PackageNS::CRIME_TYPE aeType) {
    if (aeType <= PackageNS::CRIME_TYPE_NONE || aeType > PackageNS::CRIME_TYPE_COUNT) {
        return nullptr;
    }
    BSSimpleList<Crime*>* pitem = GlobalCrimeListArray[aeType];
    Crime* preturncrime = nullptr;
    while (pitem && pitem->m_item && !preturncrime) {
        Crime* pcrime = pitem->m_item;
        pitem = pitem->m_pkNext;
        if (pcrime->pcriminal == apCriminal && pcrime->pcrimetarget == apVictim) {
            preturncrime = pcrime;
        }
    }
    return preturncrime;
}

uint16_t ProcessLists::GetCrimeIndex(PackageNS::CRIME_TYPE aeType, Crime* apCrime) {
    uint16_t usIndex = 0;
    for (BSSimpleList<Crime*>* pNode = GlobalCrimeListArray[aeType]; pNode && !pNode->IsEmpty();
         pNode = pNode->m_pkNext) {
        if (apCrime == pNode->m_item) {
            return usIndex;
        }
        ++usIndex;
    }
    Warning("AI: When trying to get a crime index, the crime was not found in the crime lists.");
    return 0;
}

void ProcessLists::FinishMagicShaderHitEffectAll(TESEffectShader* apShader) {
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicShaderHitEffect* pshaderEffect = static_cast<MagicShaderHitEffect*>(pelist->m_item.m_pObject);
            if (pshaderEffect &&
                NiObject::IsKindOf_0(&MagicShaderHitEffect::ms_RTTI, pshaderEffect) &&
                pshaderEffect->pEffectData == apShader) {
                pshaderEffect->SetFinished(true);
            }
        }
    }
}

void ProcessLists::FinishMagicModelHitEffect(TESObjectREFR* apEffectTarget, char* apFilename) {
    if (!MagicEffectList.IsEmpty()) {
        for (BSSimpleList<NiPointer<BSTempEffect>>* pelist = &MagicEffectList; pelist; pelist = pelist->m_pkNext) {
            MagicModelHitEffect* psmodelEffect = static_cast<MagicModelHitEffect*>(pelist->m_item.m_pObject);
            if (psmodelEffect &&
                NiObject::IsKindOf_0(&MagicModelHitEffect::ms_RTTI, psmodelEffect) &&
                psmodelEffect->pTarget == apEffectTarget &&
                !BSstrcmp(psmodelEffect->pFilename, apFilename)) {
                psmodelEffect->bFinished = true;
            }
        }
    }
}

void ProcessLists::CommentOnPlayerAction(PLAYER_ACTION eaction, TESObjectREFR* apObject, bool abMakecomment) {
    if (eaction) {
        int32_t iTopicIndex = 0;
        bool bDetected;
        if (fPlayerActionCommentTimer <= 0.0 || abMakecomment) {
            Actor* pTalk = nullptr;
            bDetected = false;
            int32_t idistance = 350;
            if (eaction == PLAYER_ACTION_IRON_SITES) {
                iTopicIndex = 8;
                pTalk = pPlayer->GetCombatTarget();
                if (pTalk) {
                    if (pTalk->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                        !pTalk->CanSpeak() ||
                        pTalk->IsDead(false) ||
                        pTalk->IsInCombat() ||
                        (pTalk->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS) &&
                         !pTalk->bPlayerTeammate) ||
                        350.0 < pTalk->GetDistanceFromReference(pPlayer, false, false)) {
                        pTalk = nullptr;
                    }
                    else if (pTalk->GetIsTalking()) {
                        BaseProcess* pBaseProcess = pTalk->pCurrentProcess;
                        if (pBaseProcess->GetGreetingFlag() &&
                            pTalk->GetDetectionLevelAgainstActor(
                                false,
                                pPlayer,
                                bDetected,
                                false,
                                false,
                                false,
                                nullptr
                            ) <= 0) {
                            pTalk = nullptr;
                        }
                    }
                }
            }
            else {
                kNearbyActorLock.Lock(nullptr);
                int32_t icurActor = 0;
                while (icurActor < iActorsCloseToPlayer && !pTalk) {
                    bool bisTeammate;
                    Actor* pact = pActorsCloseToPlayerA[icurActor];
                    bool bsetactor = true;
                    if (pact) {
                        bisTeammate = pact->bPlayerTeammate;
                    }
                    else {
                        bisTeammate = false;
                    }
                    if (pact &&
                        pact->pCurrentProcess &&
                        pact->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING &&
                        pact->CanSpeak() &&
                        !pact->GetIsTalking()) {
                        if (!pact->pCurrentProcess->GetGreetingFlag() &&
                            pact->GetDetectionLevelAgainstActor(
                                false,
                                pPlayer,
                                bDetected,
                                false,
                                false,
                                false,
                                nullptr
                            ) > 0 &&
                            (!pact->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS) ||
                             bisTeammate) &&
                            static_cast<double>(idistance) > pact->GetDistanceFromReference(pPlayer, false, false) &&
                            !pact->IsDead(false) &&
                            !pact->IsInCombat()) {
                            if (eaction == PLAYER_ACTION_KNOCKING_OVER_OBJECTS ||
                                eaction == PLAYER_ACTION_Z_KEY_OBJECT) {
                                if (apObject &&
                                    apObject->IsAnOwner(pact, true) &&
                                    !(fPlayerKnockObjectCommentTimer > 0.0)) {
                                    fPlayerKnockObjectCommentTimer = fCommentOnPlayerKnockingThings.Float();
                                }
                                else {
                                    bsetactor = false;
                                }
                            }
                            if (bsetactor) {
                                pTalk = pact;
                            }
                        }
                    }

                    if (++icurActor >= iActorsCloseToPlayer && idistance < 1000 && !pTalk) {
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
                        pTalk = nullptr;
                    }
                    break;
                }
                case PLAYER_ACTION_THROW_GRANADE: {
                    iTopicIndex = 2;
                    if (pPlayer->IsInCombat()) {
                        pTalk = nullptr;
                    }
                    break;
                }
                case PLAYER_ACTION_FIRE_WEAPON: {
                    iTopicIndex = 3;
                    if (pPlayer->IsInCombat() || (pTalk && pTalk->bPlayerTeammate)) {
                        pTalk = nullptr;
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
    ExtraFollower* pfollower = pPlayer->m_Extra.GetFollowerExtra();
    if (pfollower) {
        BSSimpleList<Actor*>* pHead;
        BSSimpleList<Actor*>* pTempList = new BSSimpleList<Actor*>;
        pHead = new BSSimpleList<Actor*>;
        BSSimpleList<Actor*>* plist = pHead;
        for (BSSimpleList<Actor*>* ptemp = pfollower->pActorlist; ptemp && ptemp->m_item; ptemp = ptemp->m_pkNext) {
            pHead->AddTail(ptemp->m_item);
        }
        while (plist && plist->m_item) {
            ptarget = plist->m_item;
            if (ptarget && ptarget != pPlayer && ptarget->pCurrentProcess) {
                TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                TESPackage* pEditorpack = ptarget->m_Extra.GetPackageExtra();
                if (pEditorpack) {
                    ptargpack = pEditorpack;
                }
                if (ptargpack &&
                    (ptargpack->GetPackType() == PACKAGE_TYPE_FOLLOW ||
                     ptargpack->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
                    if (ptarget->GetCurrentProcessType()) {
                        ptarget->StopCombat(nullptr);
                        ptarget->EndInterruptPackage(false);
                        ptarget->Process(3600.0f);
                    }
                }
                else {
                    pTempList->AddHead(ptarget);
                }
            }

            plist = plist->m_pkNext;
        }
        for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
            pPlayer->m_Extra.RemoveFollower(plist->m_item);
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
            pPlayer->m_Extra.RemoveFollowerExtra();
        }
    }
}

int32_t ProcessLists::DistanceListCompareFn(TESObjectREFR* apCompare1, TESObjectREFR* apCompare2) {
    float fDistance1 = apCompare1->GetDistanceFromReference(pPlayer, false, false);
    float fDistance2 = apCompare2->GetDistanceFromReference(pPlayer, false, false);
    if (!(fDistance2 > static_cast<double>(fDistance1))) {
        return fDistance2 < static_cast<double>(fDistance1);
    }

    return -1;
}

void ProcessLists::DeleteAttachedArrows(
    TESBoundObject* apBase,
    int32_t aiMax,
    Actor* apRef,
    uint8_t abNow,
    uint8_t abInv
) {
    int32_t icount = 0;
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = 0; icount < aiMax && i < parray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(parray->GetItem(i));
        if (parrow &&
            parrow->GetAttachedRef() == apRef &&
            parrow->GetObjectReference() == apBase &&
            (!abInv || parrow->GetAddedToInventory())) {
            if (abNow) {
                delete parrow;
            }
            else {
                parrow->SetState(ArrowProjectile::STATE_DELETING);
            }
            ++icount;
        }
    }
    for (uint32_t i = parray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         icount < aiMax && i < parray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         ++i) {
        ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(parray->GetItem(i));
        if (parrow &&
            parrow->GetAttachedRef() == apRef &&
            parrow->GetObjectReference() == apBase &&
            (!abInv || parrow->GetAddedToInventory())) {
            delete parrow;
            ++icount;
        }
    }
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
