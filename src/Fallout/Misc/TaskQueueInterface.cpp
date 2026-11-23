#include "Fallout/Misc/TaskQueueInterface.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSParallelTaskManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/BSPackedTask.h"
#include "BSSystem/BSPackedTaskQueue.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Havok/BGSWaterCollisionManager.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

#include <cstdarg>
#include <cstdint>
#include <xmmintrin.h>

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bTaskletActorHavokSync("bTaskletActorHavokSync:General", true);
SettingT<INISettingCollection> bTaskletCellTransformsUpdate("bTaskletCellTransformsUpdate:General", true);
HavokSyncObjects_t DeferredActorHavokSyncArray;
CellTransformsUpdateList_t CellTransformsUpdateList;

void CellUpdateParallelArrayTaskFunc(void* apData, uint32_t aiIndex) {
    BSSimpleArray<NiPointer<NiAVObject>, 1024>* parray = static_cast<BSSimpleArray<NiPointer<NiAVObject>, 1024>*>(
        apData
    );
    NiAVObject* pobj = (*parray)[aiIndex];
    if (pobj) {
        NiUpdateData updateData(0.0f, false, false);
        pobj->UpdateTransformAndBounds(updateData);
    }
}

void ActorHavokSyncParallelArrayTaskFunc(void* apData, uint32_t aiIndex) {
    BSSimpleArray<ActorHavokSyncData, 1024>* parray = static_cast<BSSimpleArray<ActorHavokSyncData, 1024>*>(apData);
    ActorHavokSyncData& actorHavokSyncData = (*parray)[aiIndex];
    if (actorHavokSyncData.pActor &&
        actorHavokSyncData.pActor->pRagdollController &&
        actorHavokSyncData.pActor->pRagdollController->GetRagdollAnimEnable() &&
        !static_cast<uint8_t>(actorHavokSyncData.pObj->GetNotVisible()) &&
        !actorHavokSyncData.pActor->IsInDialoguewithPlayer()) {
        bhkNiCollisionObject::Synchronize(actorHavokSyncData.pObj, bhkNiCollisionObject::SYNC_FROMHAVOK);
    }
}

void CreateDeferredCellNodeArray(BSSimpleArray<NiPointer<NiAVObject>, 1024>* apArray) {
    uint32_t inumDeferredCells = CellTransformsUpdateList.iSize;
    for (uint32_t icurCell = 0; icurCell < inumDeferredCells; ++icurCell) {
        NiNode* pNode = CellTransformsUpdateList[icurCell].m_pObject;
        uint32_t inumChildren = pNode->GetArrayCount();
        if (inumChildren <= 10) {
            NiPointer<NiAVObject> spNode(pNode);
            apArray->Add(spNode);
        }
        else {
            for (uint32_t i = 0; i < inumChildren; ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                if (pChild) {
                    NiPointer<NiAVObject> spChild(pChild);
                    apArray->Add(spChild);
                }
            }
        }
    }
    CellTransformsUpdateList.Clear(false);
}

TaskQueueInterface::TaskQueueInterface(BSPackedTaskQueue& apQueue, BSPackedTaskQueue& apSecondaryQueue)
  : pQueue(&apQueue), pSecondaryQueue(&apSecondaryQueue), pActiveQueue(&apQueue), iProcessingThread(0) {}

void TaskQueueInterface::QueueActorHitMe(Actor* apTarget, HitData* apHitData) {
    if (apHitData) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 17;
        task.pTaskDataA[1].pData = apTarget;
        task.pTaskDataA[2].pData = apHitData;
        apHitData->IncRefCount();
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueActorCreateBlood(Actor* apTarget, HitData* apHitData) {
    if (apHitData) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 11;
        task.pTaskDataA[1].pData = apTarget;
        task.pTaskDataA[2].pData = apHitData;
        apHitData->IncRefCount();
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueParallelTaskProcessing() {
    BSPackedTask arTask;
    arTask.pTaskDataA[0].iData = 4;
    QueueIfActive(arTask);
}

void TaskQueueInterface::QueueAttachWeapon(Actor* apActor, TESObjectWEAP* apWeapon) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 21;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].pData = apWeapon;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueHavokDetach(TESObjectREFR* apRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 42;
    task.pTaskDataA[1].iData = static_cast<int32_t>(apRef->TESForm::GetFormID());
    QueueIfActive(task);
}

void TaskQueueInterface::QueueNodeDetach(NiAVObject* apObject) {
    if (apObject) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 8;
        task.pTaskDataA[1].pData = apObject;
        apObject->IncRefCount();
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueActorMeleeAttack(Actor* apActor, bool abPowerAttack, bool abInCombatAttack) {
    if (apActor) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 15;
        task.pTaskDataA[1].pData = apActor;
        task.pTaskDataA[2].bData = abPowerAttack;
        task.pTaskDataA[3].bData = abInCombatAttack;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueActorKnockExplosion(
    Actor* apActor,
    float afLocationX,
    float afLocationY,
    float afLocationZ,
    float afMagnitude
) {
    if (apActor) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 16;
        task.pTaskDataA[1].pData = apActor;
        task.pTaskDataA[2].fData = afLocationX;
        task.pTaskDataA[3].fData = afLocationY;
        task.pTaskDataA[4].fData = afLocationZ;
        task.pTaskDataA[5].fData = afMagnitude;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueDeleteActiveEffect(ActiveEffect* apEffect) {
    BSPackedTask arTask;
    arTask.pTaskDataA[0].iData = 46;
    arTask.pTaskDataA[1].pData = apEffect;
    QueueIfActive(arTask);
}

void TaskQueueInterface::QueueCellTransformChildTask(NiNode* apNode) {
    if (bTaskletCellTransformsUpdate.Bool()) {
        NiPointer<NiNode> spNode(apNode);
        CellTransformsUpdateList.Add(spNode);
    }
}

void TaskQueueInterface::QueueMorphControllerUpdate(NiNode* apNode) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 6;
    task.pTaskDataA[1].pData = apNode;
    apNode->IncRefCount();
    QueueIfActive(task);
}

void TaskQueueInterface::QueueHavokMove(TESObjectREFR* apRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 41;
    task.pTaskDataA[1].pData = apRef;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueHavokInit(TESObjectREFR* apRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 45;
    task.pTaskDataA[1].pData = apRef;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueHavokAdd(NiAVObject* ap3D) {
    if (ap3D) {
        BSPackedTask task;
        ap3D->IncRefCount();
        task.pTaskDataA[0].iData = 19;
        task.pTaskDataA[1].pData = ap3D;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueHavokMotionChange(NiAVObject* ap3D, int32_t aiType) {
    if (ap3D) {
        BSPackedTask task;
        ap3D->IncRefCount();
        task.pTaskDataA[0].iData = 34;
        task.pTaskDataA[1].pData = ap3D;
        task.pTaskDataA[2].iData = aiType;
        QueueIfActive(task);
    }
}

// Only the task's first three words are filled; the rest stay undefined.
void TaskQueueInterface::QueueSSNRemoveObject(NiAVObject* apObject, int32_t aiIndex) {
    BSPackedTask task;
    if (apObject) {
        task.pTaskDataA[0].iData = 9;
        task.pTaskDataA[1].pData = apObject;
        task.pTaskDataA[2].iData = aiIndex;
        apObject->IncRefCount();
        QueueIfActive(task);
    }
}

// Only the task's first two words are filled; the rest stay undefined.
void TaskQueueInterface::QueueHavokRemove(NiAVObject* ap3D) {
    BSPackedTask task;
    if (ap3D) {
        ap3D->IncRefCount();
        task.pTaskDataA[0].iData = 20;
        task.pTaskDataA[1].pData = ap3D;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueIfActive(const BSPackedTask& arTask) {
    if (TaskQueueInterface::bActive) {
        // AddTask takes a non-const pointer, but the task is passed here as const.
        if (!pActiveQueue->AddTask(const_cast<BSPackedTask*>(&arTask))) {
            platform::DebuggerBreak();
        }
    }
    else {
        TaskQueueInterface::TaskUnpackFunc(arTask);
    }
}

void TaskQueueInterface::QueueActorKill(Actor* apActor, float* afDamage, DISMEMBER_DATA* apDismemberData) {
    if (apActor) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 14;
        task.pTaskDataA[1].pData = apActor;
        task.pTaskDataA[2].pData = afDamage;
        task.pTaskDataA[3].pData = apDismemberData;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueSet3DNull(TESObjectREFR* apRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 23;
    task.pTaskDataA[1].pData = apRef;
    QueueIfActive(task);
}

void TaskQueueInterface::ThreadBeginInput() {
    pQueue->ThreadBeginInput();
}

void TaskQueueInterface::ThreadEndInput() {
    pQueue->ThreadEndInput();
}

void TaskQueueInterface::ThreadBeginSecondaryInput() {
    pSecondaryQueue->ThreadBeginInput();
}

void TaskQueueInterface::ThreadEndSecondaryInput() {
    pSecondaryQueue->ThreadEndInput();
}

void TaskQueueInterface::ProcessQueue() {
    pActiveQueue = pSecondaryQueue;
    iProcessingThread = BSGetCurrentThreadID();
    pQueue->Process();
    iProcessingThread = 0;
}

void TaskQueueInterface::ProcessSecondaryQueue() {
    iProcessingThread = BSGetCurrentThreadID();
    pSecondaryQueue->Process();
    iProcessingThread = 0;
    pActiveQueue = pQueue;
}

void TaskQueueInterface::QueueActorApplyCriticalStage(Actor* apActor) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 39;
    task.pTaskDataA[1].pData = apActor;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueActorResetLoadedAnims(Actor* apActor) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 40;
    task.pTaskDataA[1].pData = apActor;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueBiped3DDetach(NiAVObject* apObject) {
    if (apObject) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 47;
        task.pTaskDataA[1].pData = apObject;
        apObject->IncRefCount();
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueActorSetPosition(Actor* apActor, const NiPoint3& aLocation) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 33;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].fData = aLocation.x;
    task.pTaskDataA[3].fData = aLocation.y;
    task.pTaskDataA[4].fData = aLocation.z;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueActorMoveCharController(Actor* apActor) {
    if (apActor) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 44;
        task.pTaskDataA[1].pData = apActor;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueHavokAdjustForceAddCount(
    NiAVObject* ap3D,
    bool abIncrement,
    bool abRecurse,
    bool abForce
) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 49;
    task.pTaskDataA[1].pData = ap3D;
    task.pTaskDataA[2].bData = abIncrement;
    task.pTaskDataA[3].bData = abRecurse;
    task.pTaskDataA[4].bData = abForce;
    ap3D->IncRefCount();
    QueueIfActive(task);
}

void TaskQueueInterface::QueuePickedUpObjectRemoval(TESObjectREFR* apPickUpRef, bool abDelete) {
    if (apPickUpRef) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 48;
        task.pTaskDataA[1].pData = apPickUpRef;
        task.pTaskDataA[2].bData = abDelete;
        QueueIfActive(task);
    }
}

// Packs the call into a task: its kind (31), the function's index and apRefObject's form ID, then one word per
// argument, read by the function's parameter types: integers and axes as they are, floats as the double C passes stored
// as a float, references and actors as their form ID (0 for none). Other parameter types read no argument and fill no
// word; arguments past the task's eighth word are dropped. The words the call does not fill are deliberately left
// undefined.
void TaskQueueInterface::QueueScriptFunctionCall(uint32_t aeFunc, TESObjectREFR* apRefObject, ...) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 31;
    task.pTaskDataA[1].iData = static_cast<int32_t>(aeFunc);
    if (apRefObject) {
        task.pTaskDataA[2].iData = static_cast<int32_t>(apRefObject->GetFormID());
    }
    else {
        task.pTaskDataA[2].iData = 0;
    }
    va_list argList;
    va_start(argList, apRefObject);
    int32_t iWord = 3;
    // The scripts' function table starts at index 0x1000.
    int32_t ifunc = static_cast<int32_t>(aeFunc - 0x1000);
    // Zero-extends the 16-bit count.
    uint16_t usParamCount = static_cast<uint16_t>(scriptFunctions[ifunc].sParamCount);
    for (int32_t i = 0; i < usParamCount; ++i) {
        if (iWord >= BSPackedTask::PACKED_TASK_SIZE) {
            continue;
        }
        switch (scriptFunctions[ifunc].pParameters[i].eParamType) {
            case SCRIPT_PARAM_FLOAT:
                task.pTaskDataA[iWord].fData = static_cast<float>(va_arg(argList, double));
                ++iWord;
                break;
            case SCRIPT_PARAM_INT:
            case SCRIPT_PARAM_AXIS:
                task.pTaskDataA[iWord].iData = va_arg(argList, int32_t);
                ++iWord;
                break;
            case SCRIPT_PARAM_OBJECTREF:
            case SCRIPT_PARAM_ACTOR: {
                TESForm* pForm = va_arg(argList, TESForm*);
                if (pForm) {
                    task.pTaskDataA[iWord].iData = static_cast<int32_t>(pForm->GetFormID());
                }
                else {
                    task.pTaskDataA[iWord].iData = 0;
                }
                ++iWord;
                break;
            }
            default: break;
        }
    }
    va_end(argList);
    QueueIfActive(task);
}

bool TaskQueueInterface::QQueueFurtherTasks() const {
    return !pQueue->IsEmpty() && !pQueue->bProcessing;
}

void TaskQueueInterface::QueueCharacterReset3D(Character* apActor) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 29;
    task.pTaskDataA[1].pData = apActor;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueActorHavokSyncTask(Actor* apActor, NiAVObject* apObj) {
    if (bTaskletActorHavokSync.Bool()) {
        uint32_t iindex = DeferredActorHavokSyncArray.AddUninitialized();
        DeferredActorHavokSyncArray.GetAt(static_cast<int32_t>(iindex)).pActor = apActor;
        DeferredActorHavokSyncArray.GetAt(static_cast<int32_t>(iindex)).pObj = apObj;
    }
}

void TaskQueueInterface::QueueNodeAttach(
    NiAVObject* apChild,
    NiNode* apParent,
    bool abFirstAvailable,
    bool abUpdateProperties
) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 7;
    task.pTaskDataA[1].pData = apChild;
    task.pTaskDataA[2].pData = apParent;
    task.pTaskDataA[3].bData = abFirstAvailable;
    task.pTaskDataA[4].bData = abUpdateProperties;
    apChild->IncRefCount();
    apParent->IncRefCount();
    QueueIfActive(task);
}

void TaskQueueInterface::QueueObjectUpdate(NiRefObject* apObject) {
    if (apObject) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 10;
        task.pTaskDataA[1].pData = apObject;
        apObject->IncRefCount();
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueAddRipple(float afSize, const NiPoint3& aLocation) {
    BSPackedTask arTask;
    arTask.pTaskDataA[0].iData = 37;
    arTask.pTaskDataA[1].fData = afSize;
    arTask.pTaskDataA[2].fData = aLocation.x;
    arTask.pTaskDataA[3].fData = aLocation.y;
    arTask.pTaskDataA[4].fData = aLocation.z;
    QueueIfActive(arTask);
}

void TaskQueueInterface::QueueWeaponFire(TESObjectWEAP* apWeapon, TESObjectREFR* apRefObject) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 32;
    task.pTaskDataA[1].pData = apWeapon;
    task.pTaskDataA[2].pData = apRefObject;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueMobileObjectInitialize(MobileObject* apTarget) {
    if (apTarget && apTarget->IsExplosion()) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 12;
        task.pTaskDataA[1].pData = apTarget;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueMHEInstantiateHitEffects(ActiveEffect* apTarget) {
    if (apTarget) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 18;
        if (apTarget->pTarget->GetTargetAsActor()) {
            Actor* pActor = apTarget->pTarget->GetTargetAsActor();
            if (pActor->Get3D()) {
                task.pTaskDataA[1].pData = apTarget;
                QueueIfActive(task);
            }
        }
        else {
            task.pTaskDataA[1].pData = apTarget;
            QueueIfActive(task);
        }
    }
}

void TaskQueueInterface::QueueUpdateDestructibleObject(TESObjectREFR* apObject, float afDamage, bool abSelfDamage) {
    if (apObject) {
        BSPackedTask task;
        task.pTaskDataA[0].iData = 13;
        task.pTaskDataA[1].pData = apObject;
        task.pTaskDataA[2].fData = afDamage;
        task.pTaskDataA[3].bData = abSelfDamage;
        QueueIfActive(task);
    }
}

void TaskQueueInterface::QueueRemoveWeapon(Actor* apActor, TESBoundObject* apWeapon) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 22;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].pData = apWeapon;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueDismemberLimb(
    Actor* apActor,
    int32_t aeDismemberLocation,
    int32_t aeLimb,
    bool abExplode,
    bool abBodyBloodSpray
) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 24;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].iData = aeDismemberLocation;
    task.pTaskDataA[3].iData = aeLimb;
    task.pTaskDataA[4].bData = abExplode;
    task.pTaskDataA[5].bData = abBodyBloodSpray;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueWaterObjectForAdd(
    TESObjectREFR* apWaterRef,
    TESObjectREFR* apObjectRef,
    float afWaterHeight
) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 25;
    task.pTaskDataA[1].pData = apWaterRef;
    task.pTaskDataA[2].pData = apObjectRef;
    task.pTaskDataA[3].fData = afWaterHeight;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueWaterObjectForRemove(
    TESObjectREFR* apWaterRef,
    TESObjectREFR* apObjectRef,
    float afWaterHeight
) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 26;
    task.pTaskDataA[1].pData = apWaterRef;
    task.pTaskDataA[2].pData = apObjectRef;
    task.pTaskDataA[3].fData = afWaterHeight;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueObstacleDoorOpen(TESObjectREFR* apDoorRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 27;
    task.pTaskDataA[1].pData = apDoorRef;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueObstacleDoorClose(TESObjectREFR* apDoorRef) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 28;
    task.pTaskDataA[1].pData = apDoorRef;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueSetOpenState(TESObjectREFR* apRef, bool abOpen, bool abSnap) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 30;
    task.pTaskDataA[1].pData = apRef;
    task.pTaskDataA[2].bData = abOpen;
    task.pTaskDataA[3].bData = abSnap;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueActorSetHavokWeapon(Actor* apActor, bool abConstrain) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 35;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].bData = abConstrain;
    QueueIfActive(task);
}

void TaskQueueInterface::QueueWaterSplashEffects(float afSplashingBodySize, const NiPoint3& aSplashLocation) {
    BSPackedTask arTask;
    arTask.pTaskDataA[0].iData = 36;
    arTask.pTaskDataA[1].fData = afSplashingBodySize;
    arTask.pTaskDataA[2].fData = aSplashLocation.x;
    arTask.pTaskDataA[3].fData = aSplashLocation.y;
    arTask.pTaskDataA[4].fData = aSplashLocation.z;
    QueueIfActive(arTask);
}

void TaskQueueInterface::QueueActorTriggerPain(Actor* apActor, bool abVoice, bool abBlur) {
    BSPackedTask task;
    task.pTaskDataA[0].iData = 38;
    task.pTaskDataA[1].pData = apActor;
    task.pTaskDataA[2].bData = abVoice;
    task.pTaskDataA[3].bData = abBlur;
    QueueIfActive(task);
}

TaskQueueInterface* TaskQueueInterface::GetInstance() {
    return TaskQueueInterface::pInstance;
}

void TaskQueueInterface::SetActive(bool abActive) {
    TaskQueueInterface::bActive = abActive;
}

void TaskQueueInterface::Instantiate(BSPackedTaskQueue& apQueue, BSPackedTaskQueue& apSecondayQueue) {
    TaskQueueInterface::pInstance = new TaskQueueInterface(apQueue, apSecondayQueue);
}

void TaskQueueInterface::Shutdown() {
    if (TaskQueueInterface::pInstance) {
        delete TaskQueueInterface::pInstance;
    }
    TaskQueueInterface::pInstance = nullptr;
}

// Unpacks one task and runs it, by the kind in its first word. Kind 12 deliberately falls into kind 13.
void TaskQueueInterface::TaskUnpackFunc(const BSPackedTask& arTask) {
    switch (arTask.pTaskDataA[0].iData) {
        case 0: {
            Warning("Float %.2f", arTask.pTaskDataA[1].fData);
            return;
        }
        case 1: {
            Warning("Int %d", arTask.pTaskDataA[1].iData);
            return;
        }
        case 2: {
            NiNode* pnode = static_cast<NiNode*>(arTask.pTaskDataA[1].pData);
            NiColor emittanceColor(arTask.pTaskDataA[2].fData, arTask.pTaskDataA[3].fData, arTask.pTaskDataA[4].fData);
            BSShaderManager::SetEmittanceRecurse(pnode, emittanceColor);
            return;
        }
        case 3: {
            SyncFromHavok(static_cast<NiNode*>(arTask.pTaskDataA[1].pData));
            return;
        }
        case 4: {
            if (DeferredActorHavokSyncArray.iSize) {
                uint32_t uiCount = DeferredActorHavokSyncArray.iSize;
                BSParallelArrayTask actorHavokSyncTask(
                    &ActorHavokSyncParallelArrayTaskFunc,
                    &DeferredActorHavokSyncArray,
                    uiCount
                );
                pParallelTaskManager->RunTask(&actorHavokSyncTask);
                DeferredActorHavokSyncArray.Clear(false);
            }
            if (CellTransformsUpdateList.iSize) {
                using cellNodeArray_t = BSScrapArray<NiPointer<NiAVObject>, 1024>;
                cellNodeArray_t cellNodeArray;
                CreateDeferredCellNodeArray(&cellNodeArray);
                uint32_t uiCount = cellNodeArray.iSize;
                BSParallelArrayTask cellUpdateTask(&CellUpdateParallelArrayTaskFunc, &cellNodeArray, uiCount);
                pParallelTaskManager->RunTask(&cellUpdateTask);
            }
            if (!bTaskletCellTransformsUpdate.Bool()) {
                TES::pInstance->UpdateDeferredCellAnimationTransforms();
            }
            return;
        }
        case 5: {
            NiNode* pnode = static_cast<NiNode*>(arTask.pTaskDataA[1].pData);
            bool bNoRadius = pnode->GetWorldBoundRadius() == 0.0f;
            TESObjectCELL::UpdateParticlesOnly(pnode);
            if (bNoRadius && pnode->GetWorldBoundRadius() > 0.0f) {
                TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pnode);
                TES::pInstance->GetCurrentCell()->TestObjectAgainstMultibounds(pRef, false, false);
            }
            pnode->DecRefCount();
            return;
        }
        case 6: {
            NiNode* pnode = static_cast<NiNode*>(arTask.pTaskDataA[1].pData);
            TESObjectCELL::UpdateMorphControllersOnly(pnode);
            pnode->DecRefCount();
            return;
        }
        case 7: {
            NiAVObject* pchild = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            NiNode* pparent = static_cast<NiNode*>(arTask.pTaskDataA[2].pData);
            bool bfirstAvailable = arTask.pTaskDataA[3].bData;
            bool bupdateProperties = arTask.pTaskDataA[4].bData;
            pparent->AttachChild(pchild, bfirstAvailable);
            pchild->PostAttachUpdate();
            if (bupdateProperties) {
                pchild->UpdateProperties();
            }
            pchild->DecRefCount();
            pparent->DecRefCount();
            return;
        }
        case 8: {
            NiAVObject* pobject = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            NiNode* pparent = pobject->m_pkParent;
            if (pparent) {
                pparent->DetachChild(pobject);
            }
            pobject->DecRefCount();
            return;
        }
        case 9: {
            NiAVObject* pobject = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            ShadowSceneNode* pssn = BSShaderManager::GetShadowSceneNode(arTask.pTaskDataA[2].iData);
            if (pssn) {
                pssn->RemoveObject(pobject);
            }
            pobject->DecRefCount();
            return;
        }
        case 10: {
            NiAVObject* pobject = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            NiUpdateData kUpdateData(0.0f, false, false);
            pobject->Update(kUpdateData);
            pobject->DecRefCount();
            return;
        }
        case 11: {
            Actor* pactor = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            HitData* phitData = static_cast<HitData*>(arTask.pTaskDataA[2].pData);
            pactor->CreateBlood(phitData->fHealthDamage, phitData);
            if (phitData->pAggressor) {
                phitData->pAggressor->CreateBloodOnWeapon(phitData->fHealthDamage, phitData, pactor);
            }
            phitData->DecRefCount();
            return;
        }
        case 12: {
            Explosion* pexplosion = static_cast<Explosion*>(arTask.pTaskDataA[1].pData);
            if (pexplosion->IsExplosion()) {
                pexplosion->Initialize();
            }
            [[fallthrough]];
        }
        case 13: {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData);
            float fData = arTask.pTaskDataA[2].fData;
            bool bData = arTask.pTaskDataA[3].bData;
            bool bFound = false;
            DestructibleRefsTakingDamageMap* pDestructibleObjects = BGSDestructibleObjectForm::GetDestructibleObjects();
            NiTMapItem<TESObjectREFR*, uint32_t>* pPos = pDestructibleObjects->GetFirstPos();
            uint32_t uiValue = 0;
            while (pPos && !bFound) {
                TESObjectREFR* pKey = nullptr;
                pDestructibleObjects->GetNext(pPos, pKey, uiValue);
                if (pKey == pref) {
                    bFound = true;
                }
            }
            if (bFound) {
                if (pref->IsActor()) {
                    static_cast<Actor*>(pref)->DoDamage(fData, 0.0f, nullptr);
                }
                else {
                    pref->DamageObject(fData, bData);
                }
            }
            return;
        }
        case 14: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            float* fdamage = static_cast<float*>(arTask.pTaskDataA[2].pData);
            pact->Kill(pact->pMyKiller, *fdamage);
            DISMEMBER_DATA* pdisdata = static_cast<DISMEMBER_DATA*>(arTask.pTaskDataA[3].pData);
            if (pdisdata) {
                // DISMEMBER_DATA keeps the cause and the limb as int.
                pact->ScriptDismember(
                    static_cast<CAUSE_OF_DEATH>(pdisdata->iCause),
                    static_cast<BGSBodyPart::LIMB_ENUM>(pdisdata->iLimb),
                    pdisdata->pKiller
                );
                ourFree(pdisdata);
            }
            ourFree(fdamage);
            return;
        }
        case 15: {
            static_cast<Actor*>(arTask.pTaskDataA[1].pData)
                ->MeleeAttack(arTask.pTaskDataA[2].bData, arTask.pTaskDataA[3].bData);
            return;
        }
        case 16: {
            Actor* pactor = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            NiPoint3 sourceLocation(arTask.pTaskDataA[2].fData, arTask.pTaskDataA[3].fData, arTask.pTaskDataA[4].fData);
            float fmagnitude = arTask.pTaskDataA[5].fData;
            if (pactor && pactor->pCurrentProcess) {
                pactor->pCurrentProcess->KnockExplosion(pactor, sourceLocation, fmagnitude);
            }

            return;
        }
        case 17: {
            Actor* pactor = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            HitData* phitData = static_cast<HitData*>(arTask.pTaskDataA[2].pData);
            if (pactor) {
                pactor->HitMe(phitData, 0);
            }
            phitData->DecRefCount();
            return;
        }
        case 18: {
            ActiveEffect* phitEffect = static_cast<ActiveEffect*>(arTask.pTaskDataA[1].pData);
            if (phitEffect) {
                phitEffect->pHitEffects = MagicHitEffect::InstantiateHitEffects(phitEffect);
            }
            return;
        }
        case 19: {
            NiAVObject* p3D = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            if (p3D) {
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(p3D);
                TESObjectCELL* pcell = pref ? pref->GetParentCell() : nullptr;
                bhkWorld* pworld = pcell ? pcell->GetbhkWorld() : nullptr;
                if (pworld) {
                    pworld->AddObjects(p3D, true, true, 0, false);
                }
                p3D->DecRefCount();
            }
            return;
        }
        case 20: {
            NiAVObject* p3D = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            if (p3D) {
                bhkWorld::Activate(p3D, true, true, false);
                bhkWorld::RemoveObjects(p3D, true, false);
                p3D->DecRefCount();
            }
            return;
        }
        case 21: {
            static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData)
                ->AttachWeapon(static_cast<TESObjectWEAP*>(arTask.pTaskDataA[2].pData));
            return;
        }
        case 22: {
            Actor* pactor = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            TESBoundObject* pWeapon = static_cast<TESBoundObject*>(arTask.pTaskDataA[2].pData);
            if (pactor->pCurrentProcess) {
                if (pactor->pCurrentProcess->GetCurrentWeapon()) {
                    ItemChange* pitem = pactor->pCurrentProcess->GetCurrentWeapon();
                    if (pitem->pContainerObj == pWeapon) {
                        pactor->RemoveWeapon();
                        if (pactor->pCurrentProcess) {
                            pactor->pCurrentProcess->SetCurrentWeapon(nullptr, nullptr, false);
                        }
                    }
                }
            }
            return;
        }
        case 23: {
            static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData)->Set3D(nullptr, false);
            return;
        }
        case 24: {
            // abExplode is read from the limb's word and abBodyBloodSpray from the explode word, one word earlier than
            // QueueDismemberLimb packs them.
            static_cast<Actor*>(arTask.pTaskDataA[1].pData)
                ->DismemberLimb(
                    static_cast<ActorValue::Index>(arTask.pTaskDataA[2].iData),
                    static_cast<BGSBodyPart::LIMB_ENUM>(arTask.pTaskDataA[3].iData),
                    arTask.pTaskDataA[3].bData,
                    arTask.pTaskDataA[4].bData,
                    true,
                    false
                );
            return;
        }
        case 25: {
            TES::pInstance->GetWaterSystem()->AddTESObjectToWaterGroup(
                static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData),
                static_cast<TESObjectREFR*>(arTask.pTaskDataA[2].pData),
                arTask.pTaskDataA[3].fData
            );
            return;
        }
        case 26: {
            TES::pInstance->GetWaterSystem()->RemoveTESObjectFromWaterGroup(
                static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData),
                static_cast<TESObjectREFR*>(arTask.pTaskDataA[2].pData),
                arTask.pTaskDataA[3].fData
            );
            return;
        }
        case 27: {
            TESObjectREFR* pDoorRef = static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData);
            NavMeshObstacleManager::GetInstance()->OnDoorOpen(pDoorRef);
            return;
        }
        case 28: {
            TESObjectREFR* pDoorRef = static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData);
            NavMeshObstacleManager::GetInstance()->OnDoorClose(pDoorRef);
            return;
        }
        case 29: {
            Character* pactor = static_cast<Character*>(arTask.pTaskDataA[1].pData);
            if (pactor) {
                pactor->Reset3D();
            }
            return;
        }
        case 30: {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData);
            bool bopen = arTask.pTaskDataA[2].bData;
            bool bsnap = arTask.pTaskDataA[3].bData;
            if (pref) {
                BGSOpenCloseForm::SetOpenState(pref, bopen, bsnap);
            }
            return;
        }
        case 31: {
            // The script function's index (QueueScriptFunctionCall; the table starts at 0x1000): 0x1007 SetPos, 0x1009
            // SetAngle, 0x109E MoveTo, 0x113C SetScale, 0x1156 SetRigidBodyMass, 0x116C DamageObject, 0x11DF
            // MatchFaceGeometry, 0x124F MoveTo with fade.
            uint32_t iformid = arTask.pTaskDataA[2].iData;
            Actor* pactor = static_cast<Actor*>(TESForm::GetFormByNumericID(iformid));
            if (!pactor) {
                return;
            }
            BSPackedTask::PackedTaskVariantData kFunction = arTask.pTaskDataA[1];
            if (kFunction.iData > 0x1156) {
                if (kFunction.iData == 0x116C) {
                    Script::DamageObjectBase(pactor, arTask.pTaskDataA[3].fData);
                    return;
                }
                if (kFunction.iData == 0x11DF) {
                    iformid = arTask.pTaskDataA[3].iData;
                    TESObjectREFR* ptargetref = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iformid));
                    Script::MatchFaceGeometryBase(pactor, ptargetref, arTask.pTaskDataA[4].iData);
                    return;
                }
                if (kFunction.iData != 0x124F) {
                    return;
                }
            }
            else {
                if (kFunction.iData == 0x1156) {
                    Script::SetRigidBodyMassBase(pactor, arTask.pTaskDataA[3].fData);
                    return;
                }
                if (kFunction.iData > 0x109E) {
                    if (kFunction.iData == 0x113C) {
                        Script::SetScaleBase(pactor, arTask.pTaskDataA[3].fData);
                    }
                    return;
                }
                if (kFunction.iData != 0x109E) {
                    // The axis is the word's low byte.
                    if (kFunction.iData == 0x1007) {
                        Script::SetPosBase(
                            pactor,
                            static_cast<char>(arTask.pTaskDataA[3].iData),
                            arTask.pTaskDataA[4].fData
                        );
                    }
                    else if (kFunction.iData == 0x1009) {
                        Script::SetAngleBase(
                            pactor,
                            static_cast<char>(arTask.pTaskDataA[3].iData),
                            arTask.pTaskDataA[4].fData
                        );
                    }
                    return;
                }
            }
            iformid = arTask.pTaskDataA[3].iData;
            TESObjectREFR* ptargetref = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iformid));
            float fX = arTask.pTaskDataA[4].fData;
            float fY = arTask.pTaskDataA[5].fData;
            float fZ = arTask.pTaskDataA[6].fData;
            int32_t iFade = arTask.pTaskDataA[1].iData == 0x124F;
            if (!pactor->IsActor() ||
                pactor == pPlayer ||
                !pactor->pCurrentProcess ||
                pactor->GetCurrentProcessType() ||
                !(pactor->GetAlpha() > 0.0) ||
                !(static_cast<double>(iFade) > 0.0)) {
                Script::MoveToFunctionBase(pactor, ptargetref, fX, fY, fZ);
            }
            else {
                static_cast<HighProcess*>(pactor->pCurrentProcess)
                    ->HighProcess::FadeOutAndMove(pactor, ptargetref, fX, fY, fZ);
            }
            return;
        }
        case 32: {
            static_cast<TESObjectWEAP*>(arTask.pTaskDataA[1].pData)
                ->Fire(static_cast<TESObjectREFR*>(arTask.pTaskDataA[2].pData));
            return;
        }
        case 33: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            NiPoint3 loc(arTask.pTaskDataA[2].fData, arTask.pTaskDataA[3].fData, arTask.pTaskDataA[4].fData);
            if (pact) {
                pact->SetPosition(loc);
            }
            return;
        }
        case 34: {
            NiAVObject* p3D = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            hkpMotion::MotionType emotionType = static_cast<hkpMotion::MotionType>(arTask.pTaskDataA[2].iData);
            if (p3D) {
                bhkWorld::SetMotion(p3D, emotionType, true, false, true);
                p3D->DecRefCount();
            }
            return;
        }
        case 35: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            bool bconstraint = arTask.pTaskDataA[2].bData;
            if (pact) {
                pact->SetHavokWeapon(bconstraint);
            }
            return;
        }
        case 36: {
            float afSplashingBodySize = arTask.pTaskDataA[1].fData;
            NiPoint3 location(arTask.pTaskDataA[2].fData, arTask.pTaskDataA[3].fData, arTask.pTaskDataA[4].fData);
            BGSWaterCollisionManager::PlaySplashEffects(afSplashingBodySize, location);
            return;
        }
        case 37: {
            float fsize = arTask.pTaskDataA[1].fData;
            NiPoint3 location(arTask.pTaskDataA[2].fData, arTask.pTaskDataA[3].fData, arTask.pTaskDataA[4].fData);
            TES::pInstance->GetWaterSystem()->AddRipple(location, fsize);
            return;
        }
        case 38: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            if (pact) {
                pact->TriggerPain(arTask.pTaskDataA[2].bData, arTask.pTaskDataA[3].bData);
            }
            return;
        }
        case 39: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            if (pact) {
                pact->ApplyCriticalStage();
            }
            return;
        }
        case 40: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            if (pact) {
                pact->ResetLoadedAnimations();
            }
            return;
        }
        case 41: {
            static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData)->MoveHavok(false);
            return;
        }
        case 42: {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(
                TESForm::GetFormByNumericID(static_cast<uint32_t>(arTask.pTaskDataA[1].iData))
            );
            if (pref && pref->IsReference()) {
                pref->DetachHavok();
            }

            return;
        }
        case 44: {
            Actor* pact = static_cast<Actor*>(arTask.pTaskDataA[1].pData);
            bhkCharacterController* pcon = pact->GetCharController();
            if (pcon) {
                pcon->SetPosition(pact->GetLocationOnReference());
                pcon->SetLinearVelocity(hkVector4::getZero());
                pcon->QFallStartHeight();
                pcon->QHitDamage();
                pcon->ClearManifold();
            }
            return;
        }
        case 45: {
            static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData)->InitHavok();
            return;
        }
        case 46: {
            ActiveEffect* peffect = static_cast<ActiveEffect*>(arTask.pTaskDataA[1].pData);
            if (peffect) {
                delete peffect;
            }
            return;
        }
        case 47: {
            NiAVObject* pobject = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            BipedAnim::RunBiped3DDetach(pobject);
            pobject->DecRefCount();
            return;
        }
        case 48: {
            Actor::RemovePickedUpObjectFromWorld(
                static_cast<TESObjectREFR*>(arTask.pTaskDataA[1].pData),
                arTask.pTaskDataA[2].bData
            );
            return;
        }
        case 49: {
            NiAVObject* p3D = static_cast<NiAVObject*>(arTask.pTaskDataA[1].pData);
            bhkWorld::AdjustForceAddCount(
                p3D,
                arTask.pTaskDataA[2].bData,
                arTask.pTaskDataA[3].bData,
                arTask.pTaskDataA[4].bData
            );
            p3D->DecRefCount();
            return;
        }
        default: {
            return;
        }
    }
}

BSParallelArrayTask::BSParallelArrayTask(TaskFunc_t apTaskFunc, void* apData, uint32_t aiSize)
  : pTaskFunc(apTaskFunc), pData(apData), iSize(aiSize), iIndex(0) {}

bool BSPackedTaskQueue::IsEmpty() {
    return Queue.IsEmpty();
}

// Runs the queued tasks until none is left and the semaphore holds no count. The loop leaves only when TryPop failed,
// so the result is always false.
bool BSPackedTaskQueue::Process() {
    bool bResult;
    BSPackedTask task;
    bProcessing = true;
    bResult = true;
    while (bResult || Queue.semaphore.iCount > 0) {
        bResult = static_cast<BSTMessageQueue*>(&Queue)->TryPop(&task);
        if (bResult) {
            pUnpackFunc(&task);
        }
    }
    bProcessing = false;
    return bResult;
}

float NiAVObject::GetWorldBoundRadius() {
    return GetWorldBound().m_fRadius;
}
