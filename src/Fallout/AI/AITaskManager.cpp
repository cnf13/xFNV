#include "Fallout/AI/AITaskManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/BSThread.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/AITaskThread.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"

#include <bit>

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bRunDetectionDuringRendering("bRunDetectionDuringRendering:General", true);
SettingT<INISettingCollection> bRunActorMovementDuringRendering("bRunActorMovementDuringRendering:General", true);
SettingT<INISettingCollection> bUseThreadedAI("bUseThreadedAI:General", false);
BSCriticalSection MobileObjectTaskletData::kActorUpdateTaskLock;
BSCriticalSection MobileObjectTaskletData::kPackageUpdateLock;
BSCriticalSection MobileObjectTaskletData::kGeneralLock;
AITaskQueue AITaskQueue::kInstance;
AITaskThreadManager AITaskThreadManager::kInstance;

MobileObjectMessage::MobileObjectMessage() : eMsgType(MobileObjectMessage::MT_END), uiCount(0), pMob(nullptr) {}

MobileObjectMessage::MobileObjectMessage(uint32_t auiCount)
  : eMsgType(MobileObjectMessage::MT_COUNT), uiCount(auiCount), pMob(nullptr) {}

MobileObjectMessage::MobileObjectMessage(MobileObject* apMob) : eMsgType(MobileObjectMessage::MT_MOB), pMob(apMob) {}

MobileObjectTaskletData::MobileObjectTaskletData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : pmoblist(nullptr), msgqueue(apPool), uiHighActorCount(0), eType(0) {}

MobileObjectTaskletData::~MobileObjectTaskletData() {}

void MobileObjectTaskletData::Process() {
    MobileObjectMessage msg;
    while (msgqueue.TryPop(&msg)) {
        if (!HandleMessage(msg, eType)) {
            return;
        }
    }
    TaskYield();
}

bool MobileObjectTaskletData::HandleMessage(const MobileObjectMessage& aMessage, uint32_t aiThread) {
    bool bTrue = true;
    MobileObjectMessage::MsgType eMsgType = aMessage.eMsgType;
    if (aMessage.eMsgType) {
        if (eMsgType == MobileObjectMessage::MT_MOB) {
            HandleMobMessage(aMessage.pMob);
        }
        else {
            if (eMsgType == MobileObjectMessage::MT_END) {
                HandleEndMessage(aiThread);
                return false;
            }
        }
    }
    else {
        uiHighActorCount = aMessage.uiCount;
        HandleCountMessage(aMessage.uiCount);
    }
    return bTrue;
}

void MobileObjectTaskletData::RunToCompletion(uint32_t aiThread) {
    eType = aiThread;
    MobileObjectMessage kTask;
    while (msgqueue.Pop(&kTask) && HandleMessage(kTask, aiThread)) {
        ;
    }
}

bool MobileObjectTaskletData::SendMessageTo(const MobileObjectMessage& aMessage) {
    return msgqueue.Push(&aMessage);
}

bool MobileObjectTaskletData::SendEndMessage() {
    MobileObjectMessage kMessage;
    return msgqueue.Push(&kMessage);
}

ActorUpdateTaskData::ActorUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool), iUpdated(0) {}

ActorUpdateTaskData::~ActorUpdateTaskData() {}

void ActorUpdateTaskData::HandleCountMessage(uint32_t aiCount) {
    iUpdated = 0;
    MobileObjectTaskletData::kActorUpdateTaskLock.Enter(nullptr);
}

void ActorUpdateTaskData::HandleMobMessage(MobileObject* apMob) {
    if (apMob && apMob->IsActor()) {
        if (static_cast<Actor*>(apMob)->bProcessMe) {
            apMob->Update(0.0f);
        }
        ++iUpdated;
    }
}

void ActorUpdateTaskData::HandleEndMessage(uint32_t aiThread) {
    MobileObjectTaskletData::kActorUpdateTaskLock.Leave();
}

MovementTaskData::MovementTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool), iUpdated(0) {}

MovementTaskData::~MovementTaskData() {}

void MovementTaskData::HandleCountMessage(uint32_t auiCount) {
    iUpdated = 0;
    MobileObjectTaskletData::kGeneralLock.Enter(nullptr);
    MobileObjectTaskletData::kPackageUpdateLock.Enter(nullptr);
}

void MovementTaskData::HandleMobMessage(MobileObject* apMob) {
    if (apMob && apMob->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apMob);
        if (pActor->bProcessMe) {
            pActor->UpdateMovement(0.0f, pPlayer->IsSleepingorResting());
        }
    }
    else if (apMob->IsProjectile()) {
        static_cast<Projectile*>(apMob)->Projectile::UpdateIfNotInArrowsList(0.0f);
    }
    else if (apMob->IsExplosion()) {
        static_cast<Explosion*>(apMob)->Explosion::ProcessMovement(0.0f);
    }
    ++iUpdated;
}

void MovementTaskData::HandleEndMessage(uint32_t aiThread) {
    MobileObjectTaskletData::kPackageUpdateLock.Leave();
    MobileObjectTaskletData::kGeneralLock.Leave();
}

PackageUpdateTaskData::PackageUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool), iUpdated(0), iNotUpdated(0) {}

PackageUpdateTaskData::~PackageUpdateTaskData() {}

void PackageUpdateTaskData::HandleCountMessage(uint32_t uiCount) {
    iUpdated = 0;
    iNotUpdated = 0;
    HighProcess::ClearWanderStarted();
    pPlayer->SetHostileDetection(false);
    MobileObjectTaskletData::kPackageUpdateLock.Enter(nullptr);
}

void PackageUpdateTaskData::HandleMobMessage(MobileObject* apMob) {
    if (apMob &&
        !apMob->IsStillLoading() &&
        !apMob->GetDelete() &&
        !apMob->GetDisabled() &&
        apMob->pCurrentProcess &&
        apMob->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        Actor* pAct = nullptr;
        bool bUpdateFollower = true;
        if (apMob && apMob->IsActor()) {
            pAct = static_cast<Actor*>(apMob);
        }

        if (!pAct || pAct->bProcessMe) {
            if (pAct && pAct->CanSkipActorUpdate()) {
                pAct->pCurrentProcess->CheckforNewPackage(pAct, false);
                bUpdateFollower = false;
                ++iNotUpdated;
                static_cast<HighProcess*>(pAct->pCurrentProcess)->HighProcess::SetSkippedLastUpdate(true);
            }
            else {
                apMob->Process(0.0f);
                ++iUpdated;
            }
            if (pAct && pPlayer->IsSleepingorResting()) {
                pAct->RestoreHealth(2.0f);
                pAct->RestoreFatigue(2.0f);
            }

            if (apMob->GetCurrentProcessType()) {
                pmoblist = nullptr;
            }
            if (bUpdateFollower) {
                // The address &apMob->m_Extra is never null.
                ExtraFollower* pFollow = apMob->m_Extra.GetFollowerExtra();
                if (pFollow) {
                    BSSimpleList<Actor*>* pTempList = new BSSimpleList<Actor*>;
                    BSSimpleList<Actor*>* pHead = new BSSimpleList<Actor*>;
                    BSSimpleList<Actor*>* pList = pHead;
                    for (BSSimpleList<Actor*>* pIter = pFollow->pActorlist; pIter && pIter->m_item;
                         pIter = pIter->m_pkNext) {
                        pHead->AddTail(pIter->m_item);
                    }
                    while (pList && pList->m_item) {
                        Actor* pTarget = pList->m_item;
                        if (pTarget && pTarget != pPlayer && !pTarget->IsFollowing(pAct)) {
                            pTempList->AddHead(pTarget);
                        }

                        pList = pList->m_pkNext;
                    }
                    for (pList = pTempList; pList && pList->m_item; pList = pList->m_pkNext) {
                        apMob->m_Extra.RemoveFollower(pList->m_item);
                    }
                    pTempList->RemoveAllPointerList();
                    if (pTempList) {
                        delete pTempList;
                    }
                    pHead->RemoveAllPointerList();
                    if (pHead) {
                        delete pHead;
                    }
                    if (pFollow->pActorlist->IsEmpty()) {
                        apMob->m_Extra.RemoveFollowerExtra();
                    }
                }
            }
        }
    }
}

void PackageUpdateTaskData::HandleEndMessage(uint32_t aiThread) {
    float fSeconds = appTimer.GetSeconds();
    ReferenceProcessLists.DisableExcessDeadActors(fSeconds);
    MobileObjectTaskletData::kPackageUpdateLock.Leave();
}

ActorsScriptTaskData::ActorsScriptTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool), iUpdated(0) {}

ActorsScriptTaskData::~ActorsScriptTaskData() {}

void ActorsScriptTaskData::HandleCountMessage(uint32_t auiCount) {
    iUpdated = 0;
    MobileObjectTaskletData::kActorUpdateTaskLock.Enter(nullptr);
    MobileObjectTaskletData::kGeneralLock.Enter(nullptr);
    MobileObjectTaskletData::kPackageUpdateLock.Enter(nullptr);
}

void ActorsScriptTaskData::HandleMobMessage(MobileObject* apMob) {
    if (apMob && apMob->IsActor() && Script::GetProcessScripts()) {
        apMob->RunScript();
        ++iUpdated;
    }
}

void ActorsScriptTaskData::HandleEndMessage(uint32_t aiThread) {
    MobileObjectTaskletData::kPackageUpdateLock.Leave();
    MobileObjectTaskletData::kGeneralLock.Leave();
    MobileObjectTaskletData::kActorUpdateTaskLock.Leave();
}

DetectionTaskData::DetectionTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool), fNumber(0.0f), iUpdated(0) {}

DetectionTaskData::~DetectionTaskData() {}

void DetectionTaskData::HandleCountMessage(uint32_t auiCount) {
    fNumber = 0.0f;
    iUpdated = 0;
    MobileObjectTaskletData::kGeneralLock.Enter(nullptr);
    MobileObjectTaskletData::kPackageUpdateLock.Enter(nullptr);
}

void DetectionTaskData::HandleMobMessage(MobileObject* apMob) {
    double dHighActorCount = static_cast<double>(this->uiHighActorCount);
    float fTime = appTimer.fDelta * dHighActorCount;
    fTime = BSMIN(fTime, 5.0f);
    fTime = RandomFloat(0.0f, fTime);
    if (apMob && apMob->IsActor()) {
        Actor* pMob = static_cast<Actor*>(apMob);
        Actor* pDetectAct = static_cast<Actor*>(apMob);
        if (pDetectAct->bProcessMe) {
            TESActorBaseData* pActorBase = static_cast<TESActorBaseData*>(pDetectAct->GetActorBase());
            if (!pActorBase->GetIsGhost()) {
                float fDistanceFromPlayer = pDetectAct->GetDistanceFromReference(pPlayer, false, false);
                if (pDetectAct != pPlayer && !pSaveLoadGameOLD->GetLoading()) {
                    if (!(pMob->GetDetectionTimer() < 0.0) || pDetectAct->IsUnConscious() || pMob->IsDead(false)) {
                        if (pMob->GetDetectionTimer() < 0.0) {
                            pPlayer->RemovePerceivedActor(pMob);
                        }
                        pMob->ReduceDetectionTimer();
                    }
                    else if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) <= 0.0) {
                        pMob->EvaluateDetection(fTime);
                        if (pDetectAct->pCurrentProcess->ShouldRunPlayerDetection(pDetectAct)) {
                            bool bLineSight = false;
                            bool bCombat = pMob->IsInCombat();
                            bool b360LineofSight = false;
                            int32_t iPlayerDetect = pPlayer->GetDetectionLevelAgainstActor(
                                true,
                                pMob,
                                bLineSight,
                                bCombat,
                                false,
                                false,
                                &b360LineofSight
                            );
                            bool bAttack = false;
                            FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
                            if (pMob->GetShouldAttackActor(pPlayer, false, eReaction, false) ||
                                pMob->IsInCombatWithActor(pPlayer)) {
                                bAttack = true;
                            }
                            else if (
                                (eReaction == FIGHT_REACTION_NEUTRAL || eReaction == FIGHT_REACTION_ENEMY) &&
                                pMob->GetActorBase()->AIData.bAggroRadius &&
                                pMob->GetActorBase()->AIData.uiAggroRadius
                            ) {
                                bAttack = true;
                            }
                            if (pMob) {
                                if (fShaderShadowUpdateDistance.Float() > static_cast<double>(fDistanceFromPlayer)) {
                                    ReferenceProcessLists.InsertActorCloseToPlayer(pDetectAct);
                                }
                            }
                            double dPlayerDetect = static_cast<double>(iPlayerDetect);
                            if (!(fPlayerDetectActorValue.Float() <= dPlayerDetect)) {
                                if (b360LineofSight || bLineSight) {
                                    pPlayer->InsertPerceivedActor(pMob, bAttack, true);
                                }
                                else {
                                    pPlayer->RemovePerceivedActor(pMob);
                                }
                            }
                            else {
                                pPlayer->InsertPerceivedActor(pMob, bAttack, false);
                            }
                        }
                    }
                }
            }
        }
        ++iUpdated;
    }
}

void DetectionTaskData::HandleEndMessage(uint32_t aiThread) {
    pPlayer->UpdateAutoAimActor();
    MobileObjectTaskletData::kGeneralLock.Leave();
    MobileObjectTaskletData::kPackageUpdateLock.Leave();
}

AnimationTaskData::AnimationTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
  : MobileObjectTaskletData(apPool) {}

AnimationTaskData::~AnimationTaskData() {}

void AnimationTaskData::HandleCountMessage(uint32_t uiCount) {
    MobileObjectTaskletData::kActorUpdateTaskLock.Enter(nullptr);
    MobileObjectTaskletData::kGeneralLock.Enter(nullptr);
}

void AnimationTaskData::HandleMobMessage(MobileObject* apMob) {
    if (apMob && apMob->IsActor()) {
        if (static_cast<Actor*>(apMob)->bProcessMe) {
            apMob->UpdateAnimation(0.0f);
        }
    }
    else if (apMob && apMob->IsTalkingActivator()) {
        apMob->UpdateAnimation(0.0f);
    }
}

void AnimationTaskData::HandleEndMessage(uint32_t aiThread) {
    MobileObjectTaskletData::kGeneralLock.Leave();
    MobileObjectTaskletData::kActorUpdateTaskLock.Leave();
}

AITaskQueue::AITaskQueue() : iNumTasks(0), iFirstTask(0) {}

AITaskQueue::~AITaskQueue() {}

void AITaskQueue::Enqueue(MobileObjectTaskletData* pTaskData) {
    if (iNumTasks < 10) {
        int32_t iIndex = static_cast<int32_t>(static_cast<uint32_t>(iNumTasks) + static_cast<uint32_t>(iFirstTask));
        if (iIndex >= 10) {
            iIndex -= 10;
        }
        TaskDataArray[iIndex] = pTaskData;
        ++iNumTasks;
    }
}

MobileObjectTaskletData* AITaskQueue::Dequeue() {
    UsingQueue.Enter(nullptr);
    MobileObjectTaskletData* pTaskData = nullptr;
    if (iNumTasks > 0) {
        pTaskData = TaskDataArray[static_cast<uint32_t>(iFirstTask)];
        --iNumTasks;
        if (++iFirstTask >= 10) {
            iFirstTask = 0;
        }
    }
    UsingQueue.Leave();
    return pTaskData;
}

void AITaskThreadManager::WaitForThread(uint32_t aiIndex) {
    AITaskThreadArray[aiIndex]->WaitForCompletion();
}

void AITaskThreadManager::AddThread(uint32_t aiThread) {
    if (iTaskThreads < 3) {
        AITaskThread* pThread = new AITaskThread(aiThread);
        AITaskThreadArray[iTaskThreads] = pThread;
        AITaskThreadArray[iTaskThreads]->Initialize(BSThread::LARGE_STACK_SIZE, nullptr, false);
        AITaskThreadArray[iTaskThreads]->SetThreadProcessor(aiThread);
        AITaskThreadArray[iTaskThreads]->SetName("AI Task Thread");
        ++iTaskThreads;
    }
}

void AITaskThreadManager::StartThreads() {
    for (int32_t iThreadIdx = 0; iThreadIdx < iTaskThreads; ++iThreadIdx) {
        AITaskThreadArray[iThreadIdx]->StartProcessingTasks();
    }
}

void AITaskThreadManager::WaitForThreads() {
    for (int32_t iThreadIdx = 0; iThreadIdx < iTaskThreads; ++iThreadIdx) {
        AITaskThreadArray[iThreadIdx]->WaitForCompletion();
    }
}

void AITaskThreadManager::StartThread(uint32_t aiIndex) {
    AITaskThreadArray[aiIndex]->StartProcessingTasks();
}

AITaskThreadManager* AITaskThreadManager::Construct(void* apMemory) {
    AITaskThreadManager* pThis = static_cast<AITaskThreadManager*>(apMemory);
    pThis->iTaskThreads = 0;
    return pThis;
}

// The task data are built before MessagePool: they keep only its address.
AITaskManager::AITaskManager()
  : TaskDataDetection(&MessagePool), TaskAnimationUpdate(&MessagePool), TaskDataPackagesUpdate(&MessagePool),
    TaskDataActorUpdate(&MessagePool), TaskDataActorsScript(&MessagePool), TaskDataActorMovement(&MessagePool),
    bWaitForThreads(false) {}

AITaskManager::~AITaskManager() {}

void AITaskManager::UpdateTasks() {
    if (AITaskManager::bFirstTime) {
        if (bUseThreadedAI.Bool()) {
            AITaskThreadManager::kInstance.AddThread(1u);
            AITaskThreadManager::kInstance.AddThread(3u);
            AITaskThreadManager::kInstance.AddThread(5u);
        }
        AITaskManager::bFirstTime = false;
    }
    if (ReferenceProcessLists.bRunScheduals &&
        ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
        uint32_t uiItems = ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (!bRunDetectionDuringRendering.Bool()) {
            MobileObjectMessage kOpenMsg(uiItems);
            TaskDataDetection.SendMessageTo(kOpenMsg);
        }
        MobileObjectMessage kPackagesMsg(uiItems);
        TaskDataPackagesUpdate.SendMessageTo(kPackagesMsg);
        if (iNumHWThreads.UInt() == 1) {
            MobileObjectMessage kActorUpdateMsg(uiItems);
            TaskDataActorUpdate.SendMessageTo(kActorUpdateMsg);
            MobileObjectMessage kAnimationMsg(uiItems);
            TaskAnimationUpdate.SendMessageTo(kAnimationMsg);
        }
        MobileObjectMessage kScriptMsg(uiItems);
        TaskDataActorsScript.SendMessageTo(kScriptMsg);
        if (!bRunActorMovementDuringRendering.Bool()) {
            MobileObjectMessage kMovementMsg(uiItems);
            TaskDataActorMovement.SendMessageTo(kMovementMsg);
        }
        int32_t i = 0;
        do {
            if (!bRunDetectionDuringRendering.Bool()) {
                MobileObjectMessage kObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskDataDetection.SendMessageTo(kObjMsg);
            }
            MobileObjectMessage kPackagesObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
            TaskDataPackagesUpdate.SendMessageTo(kPackagesObjMsg);
            if (iNumHWThreads.UInt() == 1) {
                MobileObjectMessage kActorUpdateObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskDataActorUpdate.SendMessageTo(kActorUpdateObjMsg);
                MobileObjectMessage kAnimationObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskAnimationUpdate.SendMessageTo(kAnimationObjMsg);
            }
            MobileObjectMessage kScriptObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
            TaskDataActorsScript.SendMessageTo(kScriptObjMsg);
            if (!bRunActorMovementDuringRendering.Bool()) {
                MobileObjectMessage kMovementObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskDataActorMovement.SendMessageTo(kMovementObjMsg);
            }
            ++i;
            uiItems = ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        }
        while (i < uiItems);
        if (!bRunDetectionDuringRendering.Bool()) {
            TaskDataDetection.SendEndMessage();
        }
        TaskDataPackagesUpdate.SendEndMessage();
        if (iNumHWThreads.UInt() == 1) {
            TaskAnimationUpdate.SendEndMessage();
            TaskDataActorUpdate.SendEndMessage();
        }
        TaskDataActorsScript.SendEndMessage();
        if (!bRunActorMovementDuringRendering.Bool()) {
            TaskDataActorMovement.SendEndMessage();
        }
        if (bUseThreadedAI.Bool()) {
            if (iNumHWThreads.UInt() == 1) {
                AITaskQueue::kInstance.Enqueue(&TaskDataActorUpdate);
            }
            AITaskQueue::kInstance.Enqueue(&TaskDataPackagesUpdate);
            AITaskQueue::kInstance.Enqueue(&TaskDataActorsScript);
            if (!bRunDetectionDuringRendering.Bool()) {
                AITaskQueue::kInstance.Enqueue(&TaskDataDetection);
            }
            if (!bRunActorMovementDuringRendering.Bool()) {
                AITaskQueue::kInstance.Enqueue(&TaskDataActorMovement);
                AITaskQueue::kInstance.Enqueue(&TaskAnimationUpdate);
            }
            AITaskThreadManager::kInstance.StartThreads();
            AITaskThreadManager::kInstance.WaitForThreads();
        }
        else {
            if (!bRunDetectionDuringRendering.Bool()) {
                TaskDataDetection.RunToCompletion(0);
            }
            TaskDataPackagesUpdate.RunToCompletion(0);
            TaskDataActorsScript.RunToCompletion(0);
            if (iNumHWThreads.UInt() == 1) {
                TaskAnimationUpdate.RunToCompletion(0);
                TaskDataActorUpdate.RunToCompletion(0);
            }
            if (!bRunActorMovementDuringRendering.Bool()) {
                TaskDataActorMovement.RunToCompletion(0);
            }
        }
    }

    if (iNumHWThreads.UInt() == 1) {
        Interface::SetPickLength(iActivatePickLength.Int());
        Interface::RefreshActivateREFR();
    }
}

void AITaskManager::WaitForTasksDuringRendering() {
    if (bWaitForThreads) {
        AITaskThreadManager::kInstance.WaitForThread(2);
        bWaitForThreads = false;
    }
}

void AITaskManager::StartTasksDuringRendering() {
    if (iNumHWThreads.UInt() == 1 &&
        (bRunDetectionDuringRendering.Bool() || bRunActorMovementDuringRendering.Bool()) &&
        ReferenceProcessLists.bRunScheduals &&
        ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
        uint32_t uiItems = ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (bRunDetectionDuringRendering.Bool()) {
            MobileObjectMessage kOpenMsg(uiItems);
            TaskDataDetection.SendMessageTo(kOpenMsg);
        }
        if (bRunActorMovementDuringRendering.Bool()) {
            MobileObjectMessage kMovementMsg(uiItems);
            TaskDataActorMovement.SendMessageTo(kMovementMsg);
        }
        for (uint32_t i = 0; i < uiItems; ++i) {
            if (bRunDetectionDuringRendering.Bool()) {
                MobileObjectMessage kObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskDataDetection.SendMessageTo(kObjMsg);
            }
            if (bRunActorMovementDuringRendering.Bool()) {
                MobileObjectMessage kMovementObjMsg(ReferenceProcessLists.MobProcessArray.GetItem(i));
                TaskDataActorMovement.SendMessageTo(kMovementObjMsg);
            }
        }
        if (bRunDetectionDuringRendering.Bool()) {
            TaskDataDetection.SendEndMessage();
        }
        if (bRunActorMovementDuringRendering.Bool()) {
            TaskDataActorMovement.SendEndMessage();
        }
        if (bUseThreadedAI.Bool()) {
            if (bRunDetectionDuringRendering.Bool()) {
                AITaskQueue::kInstance.Enqueue(&TaskDataDetection);
            }
            if (bRunActorMovementDuringRendering.Bool()) {
                AITaskQueue::kInstance.Enqueue(&TaskDataActorMovement);
            }
            AITaskThreadManager::kInstance.StartThread(2u);
            bWaitForThreads = true;
        }
        else {
            if (bRunDetectionDuringRendering.Bool()) {
                TaskDataDetection.RunToCompletion(0);
            }
            if (bRunActorMovementDuringRendering.Bool()) {
                TaskDataActorMovement.RunToCompletion(0);
            }
        }
    }
}

AITaskThread::AITaskThread(uint32_t aiThread)
  : StartProcessing(0, 10), FinishedProcessing(0, 10), bKillThread(false), iThread(aiThread) {}

AITaskThread::~AITaskThread() {}

uint32_t AITaskThread::ThreadProc() {
    OnStartup();
    while (true) {
        StartProcessing.Wait();
        if (bKillThread) {
            break;
        }
        while (true) {
            MobileObjectTaskletData* pTaskData = AITaskQueue::kInstance.Dequeue();
            if (!pTaskData) {
                break;
            }
            pTaskData->RunToCompletion(iThread);
        }
        FinishedProcessing.Signal();
    }
    OnShutdown();
    return 0;
}

// The router's temp and solver allocators are its heap; the stack allocator runs on a 0x16800-byte block from the heap.
void AITaskThread::OnStartup() {
    hkMemorySystem::getInstance().threadInit(ThreadMemory, "AITaskThread", hkMemorySystem::FLAG_ROUTER);
    hkBaseSystem::initThread(&ThreadMemory);
    ThreadMemory.setTemp(&ThreadMemory.heap());
    ThreadMemory.setSolver(&ThreadMemory.heap());
    pThreadStackBuffer = ThreadMemory.heap().blockAlloc(0x16800);
    ThreadMemory.m_stack.initMemory(pThreadStackBuffer, 0x16800);
    pDataHandler->CreateThreadSafeFiles();
}

// Frees OnStartup's stack block (the pointer is left as it is), then releases the thread's Havok state.
void AITaskThread::OnShutdown() {
    ThreadMemory.heap().blockFree(pThreadStackBuffer, 0x16800);
    hkBaseSystem::quitThread();
    hkMemorySystem::getInstance().threadQuit(ThreadMemory, hkMemorySystem::FLAG_ROUTER);
    ThreadMemory.m_stack.initMemory(nullptr, 0);
}

void AITaskThread::StartProcessingTasks() {
    StartProcessing.Signal();
}

void AITaskThread::WaitForCompletion() {
    FinishedProcessing.Wait();
}

// Stores whether this process skipped its last update.
void HighProcess::SetSkippedLastUpdate(bool abVal) {
    bSkippedUpdate = abVal;
}

// Clears the process-wide flag that marks a wander task as started.
void HighProcess::ClearWanderStarted() {
    bWanderStarted = 0;
}

void PlayerCharacter::SetHostileDetection(bool abflag) {
    bHostileDetection = abflag;
}
