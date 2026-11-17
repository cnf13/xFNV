#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"

#include <stdint.h>

class AITaskThread;
class MobileObject;

// A message to an AI task thread: the number of mobile objects to expect, one mobile object, or the end of the run.
class MobileObjectMessage {
public:
    enum MsgType : int32_t {
        MT_COUNT = 0x0,
        MT_MOB = 0x1,
        MT_END = 0x2,
    };

    MobileObjectMessage();
    MobileObjectMessage(uint32_t a1);
    MobileObjectMessage(MobileObject* a2);

    MsgType eMsgType;
    uint32_t uiCount;
    MobileObject* pMob;

    // The copy constructor is the implicit one.
};

// Tasklet data that walks a list of mobile objects, fed through a message queue; the subclasses handle the count, each
// object and the end.
class MobileObjectTaskletData : public BSWin32TaskletData {
public:
    MobileObjectTaskletData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~MobileObjectTaskletData() override;

    void Process() override;
    virtual void HandleCountMessage(uint32_t) = 0;
    virtual void HandleMobMessage(MobileObject*) = 0;
    virtual void HandleEndMessage(uint32_t) = 0;

    bool HandleMessage(const MobileObjectMessage& aMessage, uint32_t aiThread);
    void RunToCompletion(uint32_t aiThread);
    bool SendMessageTo(const MobileObjectMessage& aMessage);
    bool SendEndMessage();

    BSSimpleList<MobileObject*>* pmoblist;
    BSTCommonLLMessageQueue<MobileObjectMessage> msgqueue;
    uint32_t uiHighActorCount;
    uint32_t eType;

    static BSCriticalSection kGeneralLock;
    static BSCriticalSection kPackageUpdateLock;
    static BSCriticalSection kActorUpdateTaskLock;
};

// The tasklet data of the actor update task.
class ActorUpdateTaskData : public MobileObjectTaskletData {
public:
    ActorUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~ActorUpdateTaskData() override;

    void HandleCountMessage(uint32_t aiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;

    int32_t iUpdated;
};

// The tasklet data of the movement task.
class MovementTaskData : public MobileObjectTaskletData {
public:
    MovementTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~MovementTaskData() override;

    void HandleCountMessage(uint32_t auiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;

    int32_t iUpdated;
};

// The tasklet data of the package update task.
class PackageUpdateTaskData : public MobileObjectTaskletData {
public:
    PackageUpdateTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~PackageUpdateTaskData() override;

    void HandleCountMessage(uint32_t uiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;

    BSSimpleList<MobileObject*>* pprevmoblist;
    int32_t iUpdated;
    int32_t iNotUpdated;
};

// The tasklet data of the actors' script task.
class ActorsScriptTaskData : public MobileObjectTaskletData {
public:
    ActorsScriptTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~ActorsScriptTaskData() override;

    void HandleCountMessage(uint32_t auiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;

    BSSimpleList<MobileObject*>* pprevmoblist;
    int32_t iUpdated;
};

// The tasklet data of the detection task.
class DetectionTaskData : public MobileObjectTaskletData {
public:
    DetectionTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~DetectionTaskData() override;

    void HandleCountMessage(uint32_t auiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;

    float fNumber;
    int32_t iUpdated;
};

// The tasklet data of the animation task.
class AnimationTaskData : public MobileObjectTaskletData {
public:
    AnimationTaskData(BSTStaticFreeList<MobileObjectMessage, 4096>* apPool);
    ~AnimationTaskData() override;

    void HandleCountMessage(uint32_t uiCount) override;
    void HandleMobMessage(MobileObject* apMob) override;
    void HandleEndMessage(uint32_t aiThread) override;
};

// A ring buffer of up to ten tasklet data blocks waiting for an AI task thread, guarded by UsingQueue.
class AITaskQueue {
public:
    AITaskQueue();
    ~AITaskQueue();

    void Enqueue(MobileObjectTaskletData* pTaskData);
    MobileObjectTaskletData* Dequeue();

    BSCriticalSection UsingQueue;
    MobileObjectTaskletData* TaskDataArray[10];
    int32_t iNumTasks;
    int32_t iFirstTask;

    static AITaskQueue kInstance;
};

// The AI task threads the task manager hands its queued tasks to.
class AITaskThreadManager {
public:
    void WaitForThread(uint32_t aiIndex);
    void AddThread(uint32_t aiThread);
    void StartThreads();
    void WaitForThreads();
    void StartThread(uint32_t aiIndex);

    // Constructs an AITaskThreadManager in apMemory, zeroing the thread count.
    static AITaskThreadManager* Construct(void* apMemory);

    AITaskThread* AITaskThreadArray[3];
    int32_t iTaskThreads;
};

// The queue of AI tasks run between frames (ProcessLists::TaskManagerAI): the task data of each task, and the pool
// their message queues take their elements from.
class AITaskManager {
public:
    AITaskManager();
    ~AITaskManager();

    // Runs the queued AI tasks.
    void UpdateTasks();
    void WaitForTasksDuringRendering();
    // Starts the AI tasks that run while the frame renders.
    void StartTasksDuringRendering();

    DetectionTaskData TaskDataDetection;
    AnimationTaskData TaskAnimationUpdate;
    PackageUpdateTaskData TaskDataPackagesUpdate;
    ActorUpdateTaskData TaskDataActorUpdate;
    ActorsScriptTaskData TaskDataActorsScript;
    MovementTaskData TaskDataActorMovement;
    // The constructor passes it to each task data's message queue.
    BSTStaticFreeList<MobileObjectMessage, 4096> MessagePool;
    bool bWaitForThreads;
    uint8_t padding10181[3];

    static inline bool bFirstTime = true;
};

// The AI task threads' manager.
extern AITaskThreadManager AITaskThreadManager_kInstance;

// The game settings AITaskManager.cpp defines.
extern SettingT<INISettingCollection> bUseThreadedAI;
extern SettingT<INISettingCollection> bRunActorMovementDuringRendering;
extern SettingT<INISettingCollection> bRunDetectionDuringRendering;
