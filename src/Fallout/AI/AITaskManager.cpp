#include "Fallout/AI/AITaskManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSThread.h"
#include "BSCore/BSTimer.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
AITaskThreadManager AITaskThreadManager_kInstance;

MobileObjectMessage::MobileObjectMessage() : eMsgType(MobileObjectMessage::MT_END), uiCount(0), pMob(nullptr) {}

MobileObjectMessage::MobileObjectMessage(uint32_t a1)
	: eMsgType(MobileObjectMessage::MT_COUNT), uiCount(a1), pMob(nullptr) {}

MobileObjectMessage::MobileObjectMessage(MobileObject* a2) : eMsgType(MobileObjectMessage::MT_MOB), pMob(a2) {}

MobileObjectTaskletData::MobileObjectTaskletData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: pmoblist(nullptr), msgqueue(apPool), uiHighActorCount(0), eType(0) {}

MobileObjectTaskletData::~MobileObjectTaskletData() {}

void MobileObjectTaskletData::Process() {
	MobileObjectMessage msg;
	msg.~MobileObjectMessage();
	::new (&msg) MobileObjectMessage;
	while (msgqueue.TryPop(&msg)) {
		if (!HandleMessage(msg, eType)) {
			return;
		}
	}
	TaskYield();
}

bool MobileObjectTaskletData::HandleMessage(const MobileObjectMessage& aMessage, uint32_t aiThread) {
	bool bTrue = 1;
	MobileObjectMessage::MsgType eMsgType = aMessage.eMsgType;
	if (aMessage.eMsgType) {
		if (eMsgType == MobileObjectMessage::MT_MOB) {
			HandleMobMessage(aMessage.pMob);
		} else {
			if (eMsgType == MobileObjectMessage::MT_END) {
				HandleEndMessage(aiThread);
				return 0;
			}
		}
	} else {
		uiHighActorCount = aMessage.uiCount;
		HandleCountMessage(aMessage.uiCount);
	}
	return bTrue;
}

void MobileObjectTaskletData::RunToCompletion(uint32_t aiThread) {
	MobileObjectMessage kTask;
	eType = aiThread;
	kTask.~MobileObjectMessage();
	::new (&kTask) MobileObjectMessage;
	while (msgqueue.Pop(&kTask) && HandleMessage(kTask, aiThread)) {
		;
	}
}

bool MobileObjectTaskletData::SendMessageTo(const MobileObjectMessage& aMessage) {
	return msgqueue.Push(&aMessage);
}

bool MobileObjectTaskletData::SendEndMessage() {
	MobileObjectMessage kMessage;
	MobileObjectMessage* pMessage = FNVD_RECONSTRUCT(MobileObjectMessage)(&kMessage);
	return msgqueue.Push(pMessage);
}

ActorUpdateTaskData::ActorUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool), iUpdated(0) {}

ActorUpdateTaskData::~ActorUpdateTaskData() {}

void ActorUpdateTaskData::HandleCountMessage(uint32_t aiCount) {
	iUpdated = 0;
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Enter(nullptr);
}

void ActorUpdateTaskData::HandleMobMessage(MobileObject* apMob) {
	if (apMob) {
		if (apMob->IsActor()) {
			if (static_cast<Actor*>(apMob)->bProcessMe) {
				apMob->Update(0.0f);
			}
			++iUpdated;
		}
	}
}

void ActorUpdateTaskData::HandleEndMessage(uint32_t aiThread) {
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Leave();
}

MovementTaskData::MovementTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool), iUpdated(0) {}

MovementTaskData::~MovementTaskData() {}

void MovementTaskData::HandleCountMessage(uint32_t auiCount) {
	iUpdated = 0;
	(&MobileObjectTaskletData::kGeneralLock)->Enter(nullptr);
	(&MobileObjectTaskletData::kPackageUpdateLock)->Enter(nullptr);
}

void MovementTaskData::HandleMobMessage(MobileObject* apMob) {
	if (apMob && apMob->IsActor()) {
		Actor* pActor = static_cast<Actor*>(apMob);
		if (pActor->bProcessMe) {
			pActor->UpdateMovement(0.0f, pPlayer->IsSleepingorResting());
		}
	} else {
		if (apMob->IsProjectile()) {
			static_cast<Projectile*>(apMob)->Projectile::UpdateIfNotInArrowsList(0.0f);
		} else {
			if (apMob->IsExplosion()) {
				static_cast<Explosion*>(apMob)->Explosion::ProcessMovement(0.0f);
			}
		}
	}
	++iUpdated;
}

void MovementTaskData::HandleEndMessage(uint32_t aiThread) {
	(&MobileObjectTaskletData::kPackageUpdateLock)->Leave();
	(&MobileObjectTaskletData::kGeneralLock)->Leave();
}

PackageUpdateTaskData::PackageUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool), iUpdated(0), iNotUpdated(0) {}

PackageUpdateTaskData::~PackageUpdateTaskData() {}

void PackageUpdateTaskData::HandleCountMessage(uint32_t uiCount) {
	iUpdated = 0;
	iNotUpdated = 0;
	HighProcess::ClearWanderStarted();
	pPlayer->SetHostileDetection(0);
	(&MobileObjectTaskletData::kPackageUpdateLock)->Enter(nullptr);
}

void PackageUpdateTaskData::HandleMobMessage(MobileObject* apMob) {
	Actor* ptarget;
	if (apMob) {
		if (!apMob->IsStillLoading() && !apMob->GetDelete() && !apMob->GetDisabled()) {
			if (apMob->pCurrentProcess) {
				if (apMob->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
					Actor* pact = 0;
					bool updatefollower = 1;
					if (apMob) {
						if (apMob->IsActor()) {
							pact = static_cast<Actor*>(apMob);
						}
					}
					if (!pact || pact->bProcessMe) {
						if (pact && pact->CanSkipActorUpdate()) {
							BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
							CurrentAIProcess->CheckforNewPackage(pact, 0);
							updatefollower = 0;
							++iNotUpdated;
							static_cast<HighProcess*>(pact->pCurrentProcess)->HighProcess::SetSkippedLastUpdate(1);
						} else {
							apMob->Process(0.0f);
							++iUpdated;
						}
						if (pact) {
							if (pPlayer->IsSleepingorResting()) {
								pact->RestoreHealth(2.0f);
								pact->RestoreFatigue(2.0f);
							}
						}
						if (apMob->GetCurrentProcessType()) {
							pmoblist = 0;
						}
						if (updatefollower) {
							ExtraFollower* pfollow;
							// The address &apMob->m_Extra is never null.
							pfollow = (&apMob->m_Extra)->GetFollowerExtra();
							if (pfollow) {
								BSSimpleList<Actor*>* pTempList;
								void* pTempMemory = ourMalloc(sizeof(BSSimpleList<Actor*>));
								if (pTempMemory) {
									pTempList = ::new (pTempMemory) BSSimpleList<Actor*>;
								} else {
									pTempList = 0;
								}
								BSSimpleList<Actor*>* pHead;
								void* pHeadMemory = ourMalloc(sizeof(BSSimpleList<Actor*>));
								if (pHeadMemory) {
									pHead = ::new (pHeadMemory) BSSimpleList<Actor*>;
								} else {
									pHead = 0;
								}
								BSSimpleList<Actor*>* plist = pHead;
								for (BSSimpleList<Actor*>* ptemp = pfollow->pActorlist; ptemp && ptemp->m_item;
									ptemp = ptemp->m_pkNext) {
									pHead->AddTail(ptemp->m_item);
								}
								while (plist && plist->m_item) {
									ptarget = plist->m_item;
									if (ptarget) {
										if (ptarget != pPlayer && !ptarget->IsFollowing(pact)) {
											pTempList->AddHead(ptarget);
										}
									}
									plist = plist->m_pkNext;
								}
								for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
									Actor* m_item = plist->m_item;
									(&apMob->m_Extra)->RemoveFollower(m_item);
								}
								pTempList->RemoveAllPointerList();
								if (pTempList) {
									delete pTempList;
								}
								pHead->RemoveAllPointerList();
								if (pHead) {
									delete pHead;
								}
								if (pfollow->pActorlist->IsEmpty()) {
									(&apMob->m_Extra)->RemoveFollowerExtra();
								}
							}
						}
					}
				}
			}
		}
	}
}

void PackageUpdateTaskData::HandleEndMessage(uint32_t aiThread) {
	float Seconds = appTimer.GetSeconds();
	ReferenceProcessLists.DisableExcessDeadActors(Seconds);
	MobileObjectTaskletData::kPackageUpdateLock.Leave();
}

ActorsScriptTaskData::ActorsScriptTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool), iUpdated(0) {}

ActorsScriptTaskData::~ActorsScriptTaskData() {}

void ActorsScriptTaskData::HandleCountMessage(uint32_t auiCount) {
	iUpdated = 0;
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Enter(nullptr);
	(&MobileObjectTaskletData::kGeneralLock)->Enter(nullptr);
	(&MobileObjectTaskletData::kPackageUpdateLock)->Enter(nullptr);
}

void ActorsScriptTaskData::HandleMobMessage(MobileObject* apMob) {
	if (apMob) {
		if (apMob->IsActor() && Script::GetProcessScripts()) {
			apMob->RunScript();
			++iUpdated;
		}
	}
}

void ActorsScriptTaskData::HandleEndMessage(uint32_t aiThread) {
	(&MobileObjectTaskletData::kPackageUpdateLock)->Leave();
	(&MobileObjectTaskletData::kGeneralLock)->Leave();
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Leave();
}

DetectionTaskData::DetectionTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool), fNumber(0.0f), iUpdated(0) {}

DetectionTaskData::~DetectionTaskData() {}

void DetectionTaskData::HandleCountMessage(uint32_t auiCount) {
	fNumber = (float)0.0;
	iUpdated = 0;
	(&MobileObjectTaskletData::kGeneralLock)->Enter(nullptr);
	(&MobileObjectTaskletData::kPackageUpdateLock)->Enter(nullptr);
}

void DetectionTaskData::HandleMobMessage(MobileObject* apMob) {
	bool b360LineofSight;
	double uiHighActorCount = static_cast<double>(this->uiHighActorCount);
	float ftime = appTimer.fDelta * uiHighActorCount;
	ftime = BSMIN(ftime, 5.0f);
	ftime = RandomFloat(0.0f, ftime);
	Actor* toDetect = 0;
	if (apMob) {
		if (apMob->IsActor()) {
			Actor* pmob = static_cast<Actor*>(apMob);
			Actor* pdetectact = static_cast<Actor*>(apMob);
			if (pdetectact->bProcessMe) {
				TESActorBaseData* v5 = static_cast<TESActorBaseData*>(pdetectact->GetActorBase());
				if (!v5->GetIsGhost()) {
					float fdistancefromplayer = pdetectact->GetDistanceFromReference(pPlayer, 0, 0);
					if (pdetectact != pPlayer && !pSaveLoadGameOLD->GetLoading()) {
						if (!(pmob->GetDetectionTimer() < 0.0) || (pdetectact->IsUnConscious() || pmob->IsDead(0))) {
							if (pmob->GetDetectionTimer() < 0.0) {
								pPlayer->RemovePerceivedActor(pmob);
							}
							pmob->ReduceDetectionTimer();
						} else {
							if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) <= 0.0) {
								pmob->EvaluateDetection(ftime);
								BaseProcess* CurrentAIProcess = pdetectact->pCurrentProcess;
								if (CurrentAIProcess->ShouldRunPlayerDetection(pdetectact)) {
									bool blinesight = 0;
									bool bcombat = pmob->IsInCombat();
									b360LineofSight = 0;
									int32_t iplayerdetect = pPlayer->GetDetectionLevelAgainstActor(1, pmob, blinesight,
										bcombat, 0, 0, &b360LineofSight);
									bool battack = 0;
									FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
									if (pmob->GetShouldAttackActor(pPlayer, 0, ereaction, 0) ||
										pmob->IsInCombatWithActor(pPlayer)) {
										battack = 1;
									} else {
										if (ereaction == FIGHT_REACTION_NEUTRAL || ereaction == FIGHT_REACTION_ENEMY) {
											if (pmob->GetActorBase()->AIData.bAggroRadius) {
												if (pmob->GetActorBase()->AIData.uiAggroRadius) {
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
									double v3 = static_cast<double>(iplayerdetect);
									if (!(fPlayerDetectActorValue.Float() <= v3)) {
										if (b360LineofSight || blinesight) {
											pPlayer->InsertPerceivedActor(pmob, battack, 1);
										} else {
											pPlayer->RemovePerceivedActor(pmob);
										}
									} else {
										pPlayer->InsertPerceivedActor(pmob, battack, 0);
									}
								}
							}
						}
					}
				}
			}
			++iUpdated;
		}
	}
}

void DetectionTaskData::HandleEndMessage(uint32_t aiThread) {
	pPlayer->UpdateAutoAimActor();
	(&MobileObjectTaskletData::kGeneralLock)->Leave();
	(&MobileObjectTaskletData::kPackageUpdateLock)->Leave();
}

AnimationTaskData::AnimationTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool)
	: MobileObjectTaskletData(apPool) {}

AnimationTaskData::~AnimationTaskData() {}

void AnimationTaskData::HandleCountMessage(uint32_t uiCount) {
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Enter(nullptr);
	(&MobileObjectTaskletData::kGeneralLock)->Enter(nullptr);
}

void AnimationTaskData::HandleMobMessage(MobileObject* apMob) {
	if (apMob && apMob->IsActor()) {
		if (static_cast<Actor*>(apMob)->bProcessMe) {
			apMob->UpdateAnimation(0.0f);
		}
	} else {
		if (apMob) {
			if (apMob->IsTalkingActivator()) {
				apMob->UpdateAnimation(0.0f);
			}
		}
	}
}

void AnimationTaskData::HandleEndMessage(uint32_t aiThread) {
	(&MobileObjectTaskletData::kGeneralLock)->Leave();
	(&MobileObjectTaskletData::kActorUpdateTaskLock)->Leave();
}

AITaskQueue::AITaskQueue() : iNumTasks(0), iFirstTask(0) {}

AITaskQueue::~AITaskQueue() {}

void AITaskQueue::Enqueue(MobileObjectTaskletData* pTaskData) {
	int32_t v2;
	if (iNumTasks < 10) {
		v2 = (int32_t)(((uint32_t)iNumTasks) + ((uint32_t)iFirstTask));
		if (v2 >= 10) {
			v2 -= 10;
		}
		TaskDataArray[v2] = pTaskData;
		++iNumTasks;
	}
}

MobileObjectTaskletData* AITaskQueue::Dequeue() {
	UsingQueue.Enter(nullptr);
	MobileObjectTaskletData* v3 = nullptr;
	if (iNumTasks > 0) {
		v3 = TaskDataArray[(uint32_t)iFirstTask];
		--iNumTasks;
		if ((++iFirstTask) >= 10) {
			iFirstTask = 0;
		}
	}
	UsingQueue.Leave();
	return v3;
}

void AITaskThreadManager::WaitForThread(uint32_t aiIndex) {
	AITaskThreadArray[aiIndex]->WaitForCompletion();
}

void AITaskThreadManager::AddThread(uint32_t aiThread) {
	if (iTaskThreads < 3) {
		AITaskThread* pThread;
		AITaskThread* apMemory = static_cast<AITaskThread*>(ourMalloc(sizeof(AITaskThread)));
		if (apMemory) {
			pThread = ::new (apMemory) AITaskThread(aiThread);
		} else {
			pThread = 0;
		}
		AITaskThreadArray[iTaskThreads] = pThread;
		AITaskThreadArray[iTaskThreads]->Initialize(BSThread::LARGE_STACK_SIZE, 0, 0);
		AITaskThreadArray[iTaskThreads]->SetThreadProcessor(aiThread);
		AITaskThreadArray[iTaskThreads]->SetName("AI Task Thread");
		++iTaskThreads;
	}
}

void AITaskThreadManager::StartThreads() {
	for (int32_t ithreadIdx = 0; ithreadIdx < iTaskThreads; ++ithreadIdx) {
		AITaskThreadArray[ithreadIdx]->StartProcessingTasks();
	}
}

void AITaskThreadManager::WaitForThreads() {
	for (int32_t ithreadIdx = 0; ithreadIdx < iTaskThreads; ++ithreadIdx) {
		AITaskThreadArray[ithreadIdx]->WaitForCompletion();
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

// The task data are built before MessagePool, as the game does: they keep only its address.
AITaskManager::AITaskManager()
	: TaskDataDetection(&MessagePool), TaskAnimationUpdate(&MessagePool), TaskDataPackagesUpdate(&MessagePool),
	  TaskDataActorUpdate(&MessagePool), TaskDataActorsScript(&MessagePool), TaskDataActorMovement(&MessagePool),
	  bWaitForThreads(false) {}

AITaskManager::~AITaskManager() {}

void AITaskManager::UpdateTasks() {
	MobileObjectMessage a2;
	MobileObjectMessage v5;
	MobileObjectMessage v6;
	MobileObjectMessage v7;
	MobileObjectMessage v8;
	MobileObjectMessage kObjMsg;
	MobileObjectMessage v10;
	MobileObjectMessage v11;
	MobileObjectMessage v12;
	MobileObjectMessage aMessage;
	MobileObjectMessage apMemory;
	MobileObjectMessage kOpenMsg;
	if (AITaskManager::bFirstTime) {
		if (bUseThreadedAI.Bool()) {
			AITaskThreadManager_kInstance.AddThread(1u);
			AITaskThreadManager_kInstance.AddThread(3u);
			AITaskThreadManager_kInstance.AddThread(5u);
		}
		AITaskManager::bFirstTime = 0;
	}
	if (ReferenceProcessLists.bRunScheduals) {
		if ((&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
			uint32_t uiItems_2;
			uint32_t uiItems = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
			if (!bRunDetectionDuringRendering.Bool()) {
				kOpenMsg.~MobileObjectMessage();
				::new (&kOpenMsg) MobileObjectMessage(uiItems);
				TaskDataDetection.SendMessageTo(kOpenMsg);
			}
			apMemory.~MobileObjectMessage();
			::new (&apMemory) MobileObjectMessage(uiItems);
			TaskDataPackagesUpdate.SendMessageTo(apMemory);
			if (iNumHWThreads.UInt() == 1) {
				aMessage.~MobileObjectMessage();
				::new (&aMessage) MobileObjectMessage(uiItems);
				TaskDataActorUpdate.SendMessageTo(aMessage);
				v12.~MobileObjectMessage();
				::new (&v12) MobileObjectMessage(uiItems);
				TaskAnimationUpdate.SendMessageTo(v12);
			}
			v11.~MobileObjectMessage();
			::new (&v11) MobileObjectMessage(uiItems);
			TaskDataActorsScript.SendMessageTo(v11);
			if (!bRunActorMovementDuringRendering.Bool()) {
				v10.~MobileObjectMessage();
				::new (&v10) MobileObjectMessage(uiItems);
				TaskDataActorMovement.SendMessageTo(v10);
			}
			int32_t i = 0;
			do {
				if (!bRunDetectionDuringRendering.Bool()) {
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&kObjMsg,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskDataDetection.SendMessageTo(kObjMsg);
				}
				FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&v8,
					(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
				TaskDataPackagesUpdate.SendMessageTo(v8);
				if (iNumHWThreads.UInt() == 1) {
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&v7,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskDataActorUpdate.SendMessageTo(v7);
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&v6,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskAnimationUpdate.SendMessageTo(v6);
				}
				FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&v5,
					(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
				TaskDataActorsScript.SendMessageTo(v5);
				if (!bRunActorMovementDuringRendering.Bool()) {
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&a2,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskDataActorMovement.SendMessageTo(a2);
				}
				++i;
				uiItems_2 = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
			} while (i < uiItems_2);
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
				AITaskThreadManager_kInstance.StartThreads();
				AITaskThreadManager_kInstance.WaitForThreads();
			} else {
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
	}
	if (iNumHWThreads.UInt() == 1) {
		int32_t* v2 = &iActivatePickLength.Int();
		Interface::SetPickLength(*v2);
		Interface::RefreshActivateREFR();
	}
}

void AITaskManager::WaitForTasksDuringRendering() {
	if (bWaitForThreads) {
		(&AITaskThreadManager_kInstance)->WaitForThread(2);
		bWaitForThreads = (bool)0;
	}
}

void AITaskManager::StartTasksDuringRendering() {
	MobileObjectMessage a2;
	MobileObjectMessage kObjMsg;
	MobileObjectMessage apMemory;
	MobileObjectMessage kOpenMsg;
	if (iNumHWThreads.UInt() == 1 && (bRunDetectionDuringRendering.Bool() || bRunActorMovementDuringRendering.Bool()) &&
		ReferenceProcessLists.bRunScheduals) {
		if ((&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
			uint32_t uiItems = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
			if (bRunDetectionDuringRendering.Bool()) {
				kOpenMsg.~MobileObjectMessage();
				::new (&kOpenMsg) MobileObjectMessage(uiItems);
				TaskDataDetection.SendMessageTo(kOpenMsg);
			}
			if (bRunActorMovementDuringRendering.Bool()) {
				apMemory.~MobileObjectMessage();
				::new (&apMemory) MobileObjectMessage(uiItems);
				TaskDataActorMovement.SendMessageTo(apMemory);
			}
			for (uint32_t i = 0; i < uiItems; ++i) {
				if (bRunDetectionDuringRendering.Bool()) {
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&kObjMsg,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskDataDetection.SendMessageTo(kObjMsg);
				}
				if (bRunActorMovementDuringRendering.Bool()) {
					FNVD_RECONSTRUCT_AS(Construct_008C9A50, MobileObjectMessage)(&a2,
						(&ReferenceProcessLists.MobProcessArray)->GetItem(i));
					TaskDataActorMovement.SendMessageTo(a2);
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
				AITaskThreadManager_kInstance.StartThread(2u);
				bWaitForThreads = 1;
			} else {
				if (bRunDetectionDuringRendering.Bool()) {
					TaskDataDetection.RunToCompletion(0);
				}
				if (bRunActorMovementDuringRendering.Bool()) {
					TaskDataActorMovement.RunToCompletion(0);
				}
			}
		}
	}
}

AITaskThread::AITaskThread(uint32_t aiThread)
	: StartProcessing(0, 10), FinishedProcessing(0, 10), bKillThread(false), iThread(aiThread) {}

AITaskThread::~AITaskThread() {}

uint32_t AITaskThread::ThreadProc() {
	OnStartup();
	while (1) {
		StartProcessing.Wait();
		if (bKillThread) {
			break;
		}
		while (1) {
			MobileObjectTaskletData* ptaskData = AITaskQueue::kInstance.Dequeue();
			if (!ptaskData) {
				break;
			}
			ptaskData->RunToCompletion(iThread);
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
