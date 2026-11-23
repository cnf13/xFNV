#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSPackedTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ActiveEffect;
class Actor;
class BSPackedTaskQueue;
class Character;
class HitData;
class MobileObject;
class TESBoundObject;
class TESObjectREFR;
class TESObjectWEAP;

void CellUpdateParallelArrayTaskFunc(void* apData, uint32_t aiIndex);
void ActorHavokSyncParallelArrayTaskFunc(void* apData, uint32_t aiIndex);
void CreateDeferredCellNodeArray(BSSimpleArray<NiPointer<NiAVObject>, 1024>* apArray);

// The dismember request QueueActorKill queues: the limb and the cause of death, and the killer.
struct DISMEMBER_DATA {
    int32_t iLimb;
    int32_t iCause;
    Actor* pKiller;
};

// An actor and the 3D whose Havok state the actor Havok sync tasks bring in line with it.
struct ActorHavokSyncData {
    Actor* pActor;
    NiAVObject* pObj;
};

// The queue of tasks (3D loads, weapon attaches, ...) handed to the background loading threads. Only some of its
// members are declared.
class TaskQueueInterface {
public:
    TaskQueueInterface(BSPackedTaskQueue& apQueue, BSPackedTaskQueue& apSecondaryQueue);

    // Queues apActor's HitMe of apData for the 3D task thread.
    void QueueActorHitMe(Actor* apTarget, HitData* apHitData);
    // Queues apTarget's blood for apHitData for the 3D task thread.
    void QueueActorCreateBlood(Actor* apTarget, HitData* apHitData);
    // Hands the queued parallel tasks to the worker threads.
    void QueueParallelTaskProcessing();
    // Queues attaching the weapon's 3D to the actor.
    void QueueAttachWeapon(Actor* apActor, TESObjectWEAP* apWeapon);
    // Queues detaching the reference's Havok bodies.
    void QueueHavokDetach(TESObjectREFR* apRef);
    // Queues detaching apObject from its parent.
    void QueueNodeDetach(NiAVObject* apObject);
    // Queues apActor's melee attack for the worker threads.
    void QueueActorMeleeAttack(Actor* apActor, bool abPowerAttack, bool abInCombatAttack);
    // Queues apActor's knock explosion from the given location for the worker threads.
    void QueueActorKnockExplosion(
        Actor* apActor,
        float afLocationX,
        float afLocationY,
        float afLocationZ,
        float afMagnitude
    );
    // Queues destroying apEffect on the worker threads.
    void QueueDeleteActiveEffect(ActiveEffect* apEffect);
    // Queue updating the cell node's child transforms and its morph controllers on the worker
    // threads.
    void QueueCellTransformChildTask(NiNode* apNode);
    void QueueMorphControllerUpdate(NiNode* apNode);
    // Queue moving and initialising the reference's Havok bodies.
    void QueueHavokMove(TESObjectREFR* apRef);
    void QueueHavokInit(TESObjectREFR* apRef);
    // Queues adding ap3D's Havok bodies to their world.
    void QueueHavokAdd(NiAVObject* ap3D);
    // Queues changing the motion type of ap3D's Havok bodies.
    void QueueHavokMotionChange(NiAVObject* ap3D, int32_t aiType);
    // Queues removing apObject's scene-graph node from its parent.
    void QueueSSNRemoveObject(NiAVObject* apObject, int32_t aiIndex);
    // Queues removing ap3D's Havok bodies from their world.
    void QueueHavokRemove(NiAVObject* ap3D);
    // Queues arTask on the active queue when the interface is active.
    void QueueIfActive(const BSPackedTask& arTask);
    // Queues killing apActor with the damage block apfDamage (which the task frees) and the
    // dismember data.
    void QueueActorKill(Actor* apActor, float* afDamage, DISMEMBER_DATA* apDismemberData);
    // Queues clearing the reference's 3D.
    void QueueSet3DNull(TESObjectREFR* apRef);
    void ThreadBeginInput();
    void ThreadEndInput();
    void ThreadBeginSecondaryInput();
    void ThreadEndSecondaryInput();
    void ProcessQueue();
    void ProcessSecondaryQueue();
    // Queues Actor::ApplyCriticalStage for the actor.
    void QueueActorApplyCriticalStage(Actor* apActor);
    // Queues Actor::ResetLoadedAnimations for the actor.
    void QueueActorResetLoadedAnims(Actor* apActor);
    // Queues detaching a biped part's 3D.
    void QueueBiped3DDetach(NiAVObject* apObject);
    // Queues moving the actor to aLocation.
    void QueueActorSetPosition(Actor* apActor, const NiPoint3& aLocation);
    // Queues moving the actor's character controller to its position.
    void QueueActorMoveCharController(Actor* apActor);
    // Queues bhkWorld::AdjustForceAddCount over ap3D.
    void QueueHavokAdjustForceAddCount(NiAVObject* ap3D, bool abIncrement, bool abRecurse, bool abForce);
    // Queues removing the picked-up reference apPickUpRef from the world, deleting it when
    // abDelete.
    void QueuePickedUpObjectRemoval(TESObjectREFR* apPickUpRef, bool abDelete);
    // Queues the script function aeFunc on apRefObject with the function's arguments.
    void QueueScriptFunctionCall(uint32_t aeFunc, TESObjectREFR* apRefObject, ...);
    bool QQueueFurtherTasks() const;
    void QueueCharacterReset3D(Character* apActor);
    void QueueActorHavokSyncTask(Actor* apActor, NiAVObject* apObj);
    void QueueNodeAttach(NiAVObject* apChild, NiNode* apParent, bool abFirstAvailable, bool abUpdateProperties);
    void QueueObjectUpdate(NiRefObject* apObject);
    void QueueAddRipple(float afSize, const NiPoint3& aLocation);
    void QueueWeaponFire(TESObjectWEAP* apWeapon, TESObjectREFR* apRefObject);
    void QueueMobileObjectInitialize(MobileObject* apTarget);
    void QueueMHEInstantiateHitEffects(ActiveEffect* apTarget);
    void QueueUpdateDestructibleObject(TESObjectREFR* apObject, float afDamage, bool abSelfDamage);
    void QueueRemoveWeapon(Actor* apActor, TESBoundObject* apWeapon);
    void QueueDismemberLimb(
        Actor* apActor,
        int32_t aeDismemberLocation,
        int32_t aeLimb,
        bool abExplode,
        bool abBodyBloodSpray
    );
    void QueueWaterObjectForAdd(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight);
    void QueueWaterObjectForRemove(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight);
    void QueueObstacleDoorOpen(TESObjectREFR* apDoorRef);
    void QueueObstacleDoorClose(TESObjectREFR* apDoorRef);
    void QueueSetOpenState(TESObjectREFR* apRef, bool abOpen, bool abSnap);
    void QueueActorSetHavokWeapon(Actor* apActor, bool abConstrain);
    void QueueWaterSplashEffects(float afSplashingBodySize, const NiPoint3& aSplashLocation);
    void QueueActorTriggerPain(Actor* apActor, bool abVoice, bool abBlur);

    // The singleton.
    static TaskQueueInterface* GetInstance();
    static void SetActive(bool abActive);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Instantiate(BSPackedTaskQueue& apQueue, BSPackedTaskQueue& apSecondayQueue);
    static void Shutdown();
    static void TaskUnpackFunc(const BSPackedTask& arTask);

    BSPackedTaskQueue* pQueue;
    BSPackedTaskQueue* pSecondaryQueue;
    BSPackedTaskQueue* pActiveQueue;
    uint32_t iProcessingThread;

    static inline TaskQueueInterface* pInstance;
    static inline bool bActive = true;
};

// A task that the worker threads run once per item of an array: each call gets the array and the next unclaimed index.
// Only some of the members are declared.
class BSParallelArrayTask {
public:
    // the function the worker threads run on every item
    using TaskFunc_t = void (*)(void* apData, uint32_t auiIndex);

    BSParallelArrayTask() = default;
    BSParallelArrayTask(TaskFunc_t apTaskFunc, void* apData, uint32_t aiSize);

    // 0x0
    TaskFunc_t pTaskFunc;
    // 0x4
    void* pData;
    // 0x8
    uint32_t iSize;
    // 0xC the next index a worker claims
    volatile uint32_t iIndex;
};

// The game settings TaskQueueInterface.cpp defines.
extern SettingT<INISettingCollection> bTaskletCellTransformsUpdate;
extern SettingT<INISettingCollection> bTaskletActorHavokSync;

using CellTransformsUpdateList_t = BSSimpleArray<NiPointer<NiNode>, 1024>;
extern CellTransformsUpdateList_t CellTransformsUpdateList;

using HavokSyncObjects_t = BSSimpleArray<ActorHavokSyncData, 1024>;
extern HavokSyncObjects_t DeferredActorHavokSyncArray;
