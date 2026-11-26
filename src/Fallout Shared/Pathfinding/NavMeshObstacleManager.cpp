#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSThread.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSSystem/BSTasklet.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/Win32/BSWin32TaskletManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <limits>

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> bBackgroundNavmeshUpdate("bBackgroundNavmeshUpdate:Pathfinding", true);
SettingT<INISettingCollection> fObstacleUpdateDeltaWhenMoving("fObstacleUpdateDeltaWhenMoving:Pathfinding", 1.0f);
SettingT<INISettingCollection> fObstacleUpdateDeltaWhenUnknown("fObstacleUpdateDeltaWhenUnknown:Pathfinding", 5.0f);
SettingT<INISettingCollection> bCutDoors("bCutDoors:Pathfinding", true);

bhkObstacleDeactivationListener::bhkObstacleDeactivationListener() {}

bhkObstacleDeactivationListener::~bhkObstacleDeactivationListener() {}

void bhkObstacleDeactivationListener::entityDeactivatedCallback(hkpEntity* entity) {
    bhkEntity* pEntity = bhkEntity::Getbhk(entity);
    if (pEntity) {
        NavMeshObstacleManager::GetInstance()->RemoveObstacleFromUpdateList(static_cast<bhkRigidBody*>(pEntity));
    }
}

void bhkObstacleDeactivationListener::entityActivatedCallback(hkpEntity* entity) {
    bhkEntity* pEntity = bhkEntity::Getbhk(entity);
    if (pEntity) {
        NavMeshObstacleManager::GetInstance()->AddObstacleToUpdateList(static_cast<bhkRigidBody*>(pEntity));
    }
}

void bhkObstacleDeactivationListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bhkObstacleRemovalListener::bhkObstacleRemovalListener() {}

bhkObstacleRemovalListener::~bhkObstacleRemovalListener() {}

void bhkObstacleRemovalListener::entityRemovedCallback(hkpEntity* entity) {
    bhkEntity* pEntity = bhkEntity::Getbhk(entity);
    if (pEntity) {
        // The listener is added to obstacle rigid bodies only (AddActivationListener).
        NavMeshObstacleManager::GetInstance()->OnObstacleLost3D(static_cast<bhkRigidBody*>(pEntity));
    }
}

void bhkObstacleRemovalListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ReferenceObstacleArray::ReferenceObstacleArray() : iFormID(0) {}

ReferenceObstacleArray::~ReferenceObstacleArray() {
    iFormID = 0;
}

ObstacleData::ObstacleData() : pRigidBody(nullptr), bActive(true), sp3DNode(nullptr) {}

ObstacleData::~ObstacleData() {}

NavMeshObstacleManager::NavMeshObstacleManager()
  : bUpdateAllObstacles(false), FormIDMap(37u), ObstaclesToAddToUpdateList(7u, 8u),
    ObstaclesToRemoveFromUpdateList(7u, 8u), RigidBodyToObstacleMap(37u), OpenDoorMap(37u), ClosedDoorMap(37u),
    pListener(nullptr), pRemoveListener(nullptr), CurrentNavMeshTaskMap(37u),
    eState(OBSTACLE_MANAGER_BACKGROUND_STATE_PROCESSING_OBSTACLES), fTimeToNextSwap(0.5f), bDrawObstacles(false),
    spObstacleRootNode(nullptr) {
    // This memory context is opened around the field stores; nothing in it allocates.
    AutoMemContext kMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshObstacleManager.cpp",
        184
    );
}

NavMeshObstacleManager::~NavMeshObstacleManager() {
    ClearAllObstacles();
    delete pListener;
    delete pRemoveListener;
}

void NavMeshObstacleManager::RemoveObstacleFromUpdateList(bhkRigidBody* apRigidBody) {
    ObstaclesToRemoveFromUpdateList.Push(apRigidBody);
}

void NavMeshObstacleManager::AddObstacleToUpdateList(bhkRigidBody* apRigidBody) {
    ObstaclesToAddToUpdateList.Push(apRigidBody);
}

// The obstacle of apRigidBody leaves with every other obstacle of its reference.
void NavMeshObstacleManager::OnObstacleLost3D(bhkRigidBody* apRigidBody) {
    NiPointer<ObstacleData> spObstacle(nullptr);
    if (RigidBodyToObstacleMap.GetAt(apRigidBody, spObstacle)) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(
            TESForm::GetFormByNumericID(spObstacle->spParentArray->iFormID)
        );
        if (pRef) {
            RemoveObstacleForReference(pRef);
        }
    }
}

void NavMeshObstacleManager::Update() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshObstacleManager.cpp",
        1288
    );
    if (bUseObstacleAvoidance.Bool()) {
        UpdateCriticalSection.Enter("NavMeshObstacleManager::Update()");
        bool bupdatedObstacles = false;
        fTimeToNextSwap = fTimeToNextSwap - appTimer.fDelta;
        NavMeshObstacleManager::OBSTACLE_MANAGER_BACKGROUND_STATE eState = this->eState;
        if (eState) {
            if (eState == NavMeshObstacleManager::OBSTACLE_MANAGER_BACKGROUND_STATE_WAITING_FOR_PATH_MANAGER_PAUSE) {
                if (PathManager::QInstance()->QPaused()) {
                    GetProcessedTaskCount();
                    if (GetBackgroundTaskCount()) {
                        ReconnectPortals();
                        bupdatedObstacles = SwapNavmeshes();
                        DeleteCurrentTasks();
                    }
                    QueueTasks();
                    PathManager::QInstance()->Resume();
                    this->eState = NavMeshObstacleManager::OBSTACLE_MANAGER_BACKGROUND_STATE_PROCESSING_OBSTACLES;
                    fTimeToNextSwap = 0.5f;
                    if (bupdatedObstacles && NavMeshRender::GetInstance()->GetDraw()) {
                        NavMeshRender::GetInstance()->UpdateRenderOnlyNavMeshes();
                    }
                }
            }
        }
        else {
            ProcessRemovedReferences();
            ProcessNewReferences();
            ProcessRemovedDoors();
            ProcessNewDoors();
            ProcessDisabledNavMeshes();
            ProcessEnabledNavMeshes();
            if (bUpdateAllObstacles) {
                ProcessAllObstacles();
                bUpdateAllObstacles = false;
            }
            else {
                ProcessDeactivatedObstacles();
                ProcessActivatedObstacles();
            }
            ProcessMovingReferences();
            uint32_t iProcessedTaskCount = GetProcessedTaskCount();
            uint32_t iBackgroundTaskCount = GetBackgroundTaskCount();
            bool bneedsToQueueNewTasks = false;
            if (CurrentNavMeshTaskMap.m_kAllocator.m_uiCount) {
                if (iBackgroundTaskCount) {
                    if (iBackgroundTaskCount == iProcessedTaskCount) {
                        bneedsToQueueNewTasks = fTimeToNextSwap <= 0.0;
                    }
                }
                else {
                    bneedsToQueueNewTasks = fTimeToNextSwap <= 0.0;
                }
            }
            else if (iBackgroundTaskCount && iBackgroundTaskCount == iProcessedTaskCount) {
                bneedsToQueueNewTasks = fTimeToNextSwap <= 0.0;
            }
            if (bneedsToQueueNewTasks) {
                PathManager::QInstance()->Pause();
                this->eState = NavMeshObstacleManager::OBSTACLE_MANAGER_BACKGROUND_STATE_WAITING_FOR_PATH_MANAGER_PAUSE;
            }
        }
        UpdateCriticalSection.Leave();
    }
}

bool NavMeshObstacleManager::Initialize() {
    if (!BSTaskletManager::QInstance().CreateTaskGroup(ObstacleTaskletGroup)) {
        return false;
    }
    if (!ObstacleTaskletGroup.Open()) {
        return false;
    }
    pListener = new bhkObstacleDeactivationListener;
    pRemoveListener = new bhkObstacleRemovalListener;
    return true;
}

void NavMeshObstacleManager::AddObstacleForReference(TESObjectREFR* apRef) {
    if (bUseObstacleAvoidance.Bool() && !QueuedListOfRefsToAdd.IsInList(apRef)) {
        QueuedListOfRefsToAdd.AddHead(apRef);
    }
}

// Drops apRef from the queue of references to add and, when it has obstacles and is not already queued for removal,
// stops listening to their rigid bodies and queues its form ID for removal.
void NavMeshObstacleManager::RemoveObstacleForReference(TESObjectREFR* apRef) {
    if (!bUseObstacleAvoidance.Bool()) {
        return;
    }
    QueuedListOfRefsToAdd.Remove(apRef);
    uint32_t uiFormID = apRef->GetFormID();
    if (QueuedListOfRefsToRemove.IsInList(uiFormID)) {
        return;
    }
    NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
    if (FormIDMap.GetAt(apRef->GetFormID(), spObstacleArray)) {
        for (NiPointer<ObstacleData>& spObstacle : spObstacleArray->Obstacles) {
            RemoveActivationListener(spObstacle->pRigidBody);
        }
        uint32_t uiRemoveID = apRef->GetFormID();
        QueuedListOfRefsToRemove.AddTail(uiRemoveID);
    }
}

// A closed swinging door cuts the navmesh again: its open-door obstacles are dropped, a pending removal of its open
// state is cancelled, and the door is queued as closed unless it already is.
void NavMeshObstacleManager::OnDoorClose(TESObjectREFR* apDoorRef) {
    TESBoundObject* pBase = apDoorRef->GetObjectReference();
    if (pBase->GetFormType() == DOOR_ID && !static_cast<TESObjectDOOR*>(pBase)->IsSlidingDoor()) {
        RemoveObstacleForReference(apDoorRef);
    }
    NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
    if (!OpenDoorMap.GetAt(apDoorRef->GetFormID(), spObstacleArray)) {
        uint32_t uiFormID = apDoorRef->GetFormID();
        if (QueuedListOfDoorsToRemove.IsInList(uiFormID)) {
            uint32_t uiRemoveID = apDoorRef->GetFormID();
            QueuedListOfDoorsToRemove.Remove(uiRemoveID);
        }
    }
    if (!ClosedDoorMap.GetAt(apDoorRef->GetFormID(), spObstacleArray) && !QueuedListOfDoorsToAdd.IsInList(apDoorRef)) {
        QueuedListOfDoorsToAdd.AddHead(apDoorRef);
    }
}

void NavMeshObstacleManager::OnDoorOpen(TESObjectREFR* apDoorRef) {
    TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(apDoorRef->GetObjectReference());
    if (pdoor->GetFormType() == DOOR_ID) {
        TESObjectDOOR* pDoorForm = pdoor;
        if (!pDoorForm->IsSlidingDoor()) {
            AddObstacleForReference(apDoorRef);
        }
    }
    NiPointer<ReferenceObstacleArray> spdummy(static_cast<ReferenceObstacleArray*>(nullptr));
    uint32_t uiFormID = apDoorRef->iFormID;
    if (!ClosedDoorMap.GetAt(uiFormID, spdummy) && QueuedListOfDoorsToAdd.IsInList(apDoorRef)) {
        QueuedListOfDoorsToAdd.Remove(apDoorRef);
    }
    uiFormID = apDoorRef->iFormID;
    if (!OpenDoorMap.GetAt(uiFormID, spdummy)) {
        uiFormID = apDoorRef->iFormID;
        if (!QueuedListOfDoorsToRemove.IsInList(uiFormID)) {
            uint32_t uiAddID = apDoorRef->iFormID;
            QueuedListOfDoorsToRemove.AddHead(uiAddID);
        }
    }
}

void NavMeshObstacleManager::OnDoorRemove(TESObjectREFR* apDoorRef) {
    TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(apDoorRef->GetObjectReference());
    if (pdoor->GetFormType() == DOOR_ID) {
        TESObjectDOOR* pDoorForm = pdoor;
        if (!pDoorForm->IsSlidingDoor()) {
            RemoveObstacleForReference(apDoorRef);
        }
    }
    QueuedListOfRefsToAdd.Remove(apDoorRef);
    if (QueuedListOfDoorsToAdd.IsInList(apDoorRef)) {
        QueuedListOfDoorsToAdd.Remove(apDoorRef);
    }
    else {
        uint32_t uiFormID = apDoorRef->iFormID;
        if (!QueuedListOfDoorsToRemove.IsInList(uiFormID)) {
            uint32_t uiAddID = apDoorRef->iFormID;
            QueuedListOfDoorsToRemove.AddHead(uiAddID);
        }
    }
}

void NavMeshObstacleManager::QueueUpdateAllObstacle() {
    bUpdateAllObstacles = true;
}

// Brings every obstacle up to date at once: waits for the background tasks, pauses pathing, swaps the finished
// navmeshes in, runs a full update cycle and waits for it, then resumes pathing. The lock name is an old name of this
// function, kept as the game has it.
void NavMeshObstacleManager::ForceUpdateAllObstacles() {
    UpdateCriticalSection.Enter("NavMeshObstacleManager::ForceUpdate()");
    uint32_t uiProcessed = GetProcessedTaskCount();
    uint32_t uiBackground = GetBackgroundTaskCount();
    while (uiProcessed != uiBackground) {
        BSSleep(0);
        uiProcessed = GetProcessedTaskCount();
        uiBackground = GetBackgroundTaskCount();
    }
    PathManager::QInstance()->Pause();
    while (!PathManager::QInstance()->QPaused()) {
        PathManager::QInstance()->Update();
    }
    ReconnectPortals();
    SwapNavmeshes();
    DeleteCurrentTasks();
    QueueTasks();
    uiProcessed = GetProcessedTaskCount();
    uiBackground = GetBackgroundTaskCount();
    while (uiProcessed != uiBackground) {
        BSSleep(0);
        uiProcessed = GetProcessedTaskCount();
        uiBackground = GetBackgroundTaskCount();
    }
    ReconnectPortals();
    SwapNavmeshes();
    DeleteCurrentTasks();
    ProcessRemovedReferences();
    ProcessNewReferences();
    ProcessAllObstacles();
    ProcessMovingReferences();
    ProcessRemovedDoors();
    ProcessNewDoors();
    QueueTasks();
    uiProcessed = GetProcessedTaskCount();
    uiBackground = GetBackgroundTaskCount();
    while (uiProcessed != uiBackground) {
        BSSleep(0);
        uiProcessed = GetProcessedTaskCount();
        uiBackground = GetBackgroundTaskCount();
    }
    ReconnectPortals();
    SwapNavmeshes();
    DeleteCurrentTasks();
    eState = OBSTACLE_MANAGER_BACKGROUND_STATE_PROCESSING_OBSTACLES;
    fTimeToNextSwap = 0.5f;
    if (NavMeshRender::GetInstance()->GetDraw()) {
        NavMeshRender::GetInstance()->UpdateRenderOnlyNavMeshes();
    }
    PathManager::QInstance()->Resume();
    UpdateCriticalSection.Leave();
}

void NavMeshObstacleManager::AddActivationListener(bhkRigidBody* apRigidBody) {
    apRigidBody->bhkEntity::AddEntityActivationListener(pListener);
    apRigidBody->bhkEntity::AddEntityListener(pRemoveListener);
}

void NavMeshObstacleManager::EnableNavMesh(uint32_t aiNavMeshID) {
    QueuedListOfNavmeshesToEnable.AddHead(aiNavMeshID);
}

void NavMeshObstacleManager::DisableNavMesh(uint32_t aiNavMeshID) {
    QueuedListOfNavmeshesToDisable.AddHead(aiNavMeshID);
}

void NavMeshObstacleManager::AddBoundingBox3DNode(const NiPointer<ObstacleData>& aBox) {
    if (aBox->sp3DNode.m_pObject) {
        spObstacleRootNode->DetachChild(aBox->sp3DNode.m_pObject);
    }
    NiPointer<NiTriShape> spnode(nullptr);
    CreateBoundingBox3DNode(aBox, spnode);
    spObstacleRootNode->AttachChild(spnode.m_pObject, 1);
    spObstacleRootNode->UpdateProperties();
    NiUpdateData UpdateData(0.0f, false, false);
    spObstacleRootNode->Update(UpdateData);
}

// Detaches the obstacle listeners AddActivationListener attached.
void NavMeshObstacleManager::RemoveActivationListener(bhkRigidBody* apRigidBody) {
    apRigidBody->RemoveEntityActivationListener(pListener);
    apRigidBody->RemoveEntityListener(pRemoveListener);
}

uint32_t NavMeshObstacleManager::GetProcessedTaskCount() {
    ProcessedTaskLock.Lock("NavMeshObstacleManager::GetProcessedTaskCount()");
    uint32_t uiCount = ProcessedTasks.GetSize();
    ProcessedTaskLock.Unlock();
    return uiCount;
}

// Takes the processed-task lock, though it reads the background tasks.
uint32_t NavMeshObstacleManager::GetBackgroundTaskCount() {
    ProcessedTaskLock.Lock("NavMeshObstacleManager::GetBackgroundTaskCount()");
    uint32_t uiCount = BackgroundTasks.GetSize();
    ProcessedTaskLock.Unlock();
    return uiCount;
}

// Starts every pending navmesh task: on the tasklet threads when bBackgroundNavmeshUpdate is set, otherwise it is
// recorded as a background task and processed at once. The map is emptied afterwards.
void NavMeshObstacleManager::QueueTasks() {
    NiTMapItem<uint32_t, ObstacleTaskData*>* pPos = CurrentNavMeshTaskMap.GetFirstPos();
    while (pPos) {
        uint32_t uiNavMeshID;
        ObstacleTaskData* pTask;
        CurrentNavMeshTaskMap.GetNext(pPos, uiNavMeshID, pTask);
        if (bBackgroundNavmeshUpdate.Bool()) {
            PushTask(pTask);
        }
        else {
            ProcessedTaskLock.Lock(nullptr);
            BackgroundTasks.Add(pTask);
            ProcessedTaskLock.Unlock();
            pTask->Process();
        }
    }
    CurrentNavMeshTaskMap.RemoveAll();
}

// Lets each processed task swap its new navmesh in; true when any did.
bool NavMeshObstacleManager::SwapNavmeshes() {
    bool bSwapped = false;
    uint32_t uiCount = ProcessedTasks.GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ObstacleTaskData* pTask = ProcessedTasks.GetAt(i);
        if (pTask->ReplaceNavMesh()) {
            bSwapped = true;
        }
    }
    return bSwapped;
}

// Deletes the processed tasks and empties both task arrays.
bool NavMeshObstacleManager::DeleteCurrentTasks() {
    uint32_t uiCount = ProcessedTasks.GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ObstacleTaskData* pTask = ProcessedTasks.GetAt(i);
        delete pTask;
    }
    ProcessedTasks.Clear(true);
    BackgroundTasks.Clear(true);
    return true;
}

// The pending task of apNavMesh, created (0x58 bytes, ourMalloc) and entered in the map under the navmesh's ID when
// there is none yet.
ObstacleTaskData* NavMeshObstacleManager::GetTaskForNavMesh(NavMeshInfo* apNavMesh) {
    ObstacleTaskData* pTask = nullptr;
    if (CurrentNavMeshTaskMap.GetAt(apNavMesh->GetNavMeshID(), pTask)) {
        return pTask;
    }
    ObstacleTaskData* pNewTask = new ObstacleTaskData(apNavMesh);
    pTask = pNewTask;
    CurrentNavMeshTaskMap.SetAt(apNavMesh->GetNavMeshID(), pTask);
    return pTask;
}

// Records apTaskData as a background task and hands it to the tasklet group, retrying with a zero sleep up to 100
// times; when the group never takes it, it is dropped from the background tasks again. The tasklet lives on the stack.
bool NavMeshObstacleManager::PushTask(ObstacleTaskData* apTaskData) {
    alignas(BSTasklet) uint8_t aTaskStorage[sizeof(BSTasklet)];
    BSTasklet& kTask = *::new (aTaskStorage) BSTasklet;
    kTask.SetTaskData(apTaskData);
    ProcessedTaskLock.Lock(nullptr);
    BackgroundTasks.Add(apTaskData);
    ProcessedTaskLock.Unlock();
    const uint32_t uiMaxAttempts = 100;
    uint32_t uiAttempt = 0;
    while (!ObstacleTaskletGroup.AttachTask(kTask, false)) {
        if (++uiAttempt >= uiMaxAttempts) {
            ProcessedTaskLock.Lock(nullptr);
            int32_t iIndex = BackgroundTasks
                                 .Find(apTaskData, 0, &BSSimpleArray<ObstacleTaskData*>::DefaultEqualFunctionByAddress);
            if (iIndex != -1) {
                BackgroundTasks.Remove(iIndex, true);
            }
            ProcessedTaskLock.Unlock();
            kTask.~BSTasklet();
            return false;
        }
        BSSleep(0);
    }
    kTask.~BSTasklet();
    return true;
}

// Handles the references queued for adding: a reference with obstacles already has them rebuilt; one whose 3D holds an
// obstacle body that is in a Havok world gets its obstacles built. Either way it leaves the queue; the others stay
// queued. The processed-reference array and the probe obstacle live on the stack.
void NavMeshObstacleManager::ProcessNewReferences() {
    BSSimpleArray<TESObjectREFR*> kProcessedRefs;
    BSSimpleList<TESObjectREFR*>* pIter = &QueuedListOfRefsToAdd;
    while (pIter && !pIter->IsEmpty()) {
        TESObjectREFR* pRef = pIter->GetItem();
        pIter = pIter->GetNext();
        NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
        if (FormIDMap.GetAt(pRef->GetFormID(), spObstacleArray)) {
            UpdateObstaclesForReference(pRef->GetFormID(), pRef);
            kProcessedRefs.Add(pRef);
        }
        else if (pRef->Get3D()) {
            ObstacleData kObstacle;
            if (FindFirstObstacleFor3D(pRef->Get3D(), &kObstacle)) {
                bhkRigidBody* pRigidBody = kObstacle.pRigidBody;
                if (pRigidBody->GethkWorld()) {
                    ReallyAddObstaclesForReference(pRef);
                    kProcessedRefs.Add(pRef);
                }
            }
        }
    }
    for (TESObjectREFR* pRef : kProcessedRefs) {
        QueuedListOfRefsToAdd.Remove(pRef);
    }
}

// Handles the form IDs queued for removal: their obstacles leave the update list and are dropped, then the queue is
// emptied.
void NavMeshObstacleManager::ProcessRemovedReferences() {
    BSSimpleList<uint32_t>* pIter = &QueuedListOfRefsToRemove;
    while (pIter && !pIter->IsEmpty()) {
        uint32_t uiFormID = pIter->GetItem();
        pIter = pIter->GetNext();
        NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
        if (FormIDMap.GetAt(uiFormID, spObstacleArray)) {
            for (const NiPointer<ObstacleData>& spObstacle : spObstacleArray->Obstacles) {
                ObstaclesUpdateList.Remove(spObstacle);
            }
        }
        ReallyRemoveObstaclesForReference(uiFormID);
    }
    QueuedListOfRefsToRemove.RemoveAll();
}

// Enables all queued navmeshes and then clears their IDs.
bool NavMeshObstacleManager::ProcessEnabledNavMeshes() {
    BSSimpleList<uint32_t>* pIter = &QueuedListOfNavmeshesToEnable;
    while (pIter && !pIter->IsEmpty()) {
        uint32_t uiNavMeshID = pIter->GetItem();
        NavMesh::SetDisableNavMesh(uiNavMeshID, false);
        pIter = pIter->GetNext();
    }
    QueuedListOfNavmeshesToEnable.RemoveAll();
    return true;
}

// Disables all queued navmeshes and then clears their IDs.
bool NavMeshObstacleManager::ProcessDisabledNavMeshes() {
    BSSimpleList<uint32_t>* pIter = &QueuedListOfNavmeshesToDisable;
    while (pIter && !pIter->IsEmpty()) {
        uint32_t uiNavMeshID = pIter->GetItem();
        NavMesh::SetDisableNavMesh(uiNavMeshID, true);
        pIter = pIter->GetNext();
    }
    QueuedListOfNavmeshesToDisable.RemoveAll();
    return true;
}

// Re-cuts every obstacle of every reference.
void NavMeshObstacleManager::ProcessAllObstacles() {
    NiTMapItem<uint32_t, NiPointer<ReferenceObstacleArray>>* pPos = FormIDMap.GetFirstPos();
    while (pPos) {
        NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
        uint32_t uiFormID;
        FormIDMap.GetNext(pPos, uiFormID, spObstacleArray);
        for (uint32_t i = 0; i < spObstacleArray->Obstacles.GetSize(); ++i) {
            NiPointer<ObstacleData> spObstacle(spObstacleArray->Obstacles.GetAt(i));
            UpdateObstacle(spObstacle);
        }
    }
}

// Re-cuts each obstacle on the update list that has moved more than 5 units (squared distance 25) from its centre once
// fObstacleUpdateDeltaWhenMoving seconds have passed since its last update. The delay in milliseconds is truncated
// through a 64-bit integer and the low half is kept.
void NavMeshObstacleManager::ProcessMovingReferences() {
    uint32_t uiUpdateDelta = static_cast<uint32_t>(
        static_cast<int64_t>(fObstacleUpdateDeltaWhenMoving.Float() * 1000.0)
    );
    BSSimpleList<NiPointer<ObstacleData>>* pIter = &ObstaclesUpdateList;
    while (pIter) {
        bool bEmpty;
        bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        alignas(NiPointer<ObstacleData>) uint8_t aObstacleStorage[sizeof(NiPointer<ObstacleData>)];
        NiPointer<ObstacleData>& spObstacle = *reinterpret_cast<NiPointer<ObstacleData>*>(aObstacleStorage);
        spObstacle.SetPointer(&pIter->GetItem());
        pIter = pIter->GetNext();
        hkpRigidBody* phkRigidBody = spObstacle->pRigidBody->Gethk();
        hkVector4 kHavokPos(phkRigidBody->getTranslation());
        NiPoint3 kPos;
        HK2NI(kPos, kHavokPos);
        if ((kPos - spObstacle->Center).SqrLength() > 25.0f) {
            uint32_t uiTime = appTimer.GetTime();
            if (uiTime - spObstacle->iLastUpdateTime >= uiUpdateDelta) {
                UpdateObstacle(spObstacle);
            }
        }
        spObstacle.~NiPointer<ObstacleData>();
    }
}

void NavMeshObstacleManager::UpdateObstacle(NiPointer<ObstacleData>& spObstacle) {
    if (spObstacle->bActive) {
        for (NavMeshInfo* pNavMeshInfo : spObstacle->Navmeshes) {
            ObstacleTaskData* pTask = GetTaskForNavMesh(pNavMeshInfo);
            pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE);
        }
        spObstacle->Navmeshes.Clear(true);
    }
    NavMeshObstacleManager::UpdateObstacleData(spObstacle.m_pObject);
    NavMeshObstacleManager::UpdateObstacleActiveStatus(spObstacle);
    if (bDrawObstacles) {
        AddBoundingBox3DNode(spObstacle);
    }
    if (spObstacle->bActive) {
        using navmeshes_t = BSScrapArray<NavMeshPtr, 1024>;
        navmeshes_t navmeshes;
        if (NavMeshObstacleManager::GetNavMeshListForObstacle(spObstacle, &navmeshes)) {
            for (uint32_t i = 0; i < navmeshes.iSize; ++i) {
                NavMeshInfo* pInfo = navmeshes.GetAt(i)->GetNavMeshInfo();
                ObstacleTaskData* pTask = GetTaskForNavMesh(pInfo);
                pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE);
                NavMeshInfo* pAddInfo = navmeshes.GetAt(i)->GetNavMeshInfo();
                spObstacle->Navmeshes.Add(pAddInfo);
            }
        }
    }
}

void NavMeshObstacleManager::UpdateObstaclesForReference(uint32_t aiRefID, TESObjectREFR* apRef) {
    NiPointer<ReferenceObstacleArray> spArray(static_cast<ReferenceObstacleArray*>(nullptr));
    if (FormIDMap.GetAt(aiRefID, spArray)) {
        for (uint32_t i = 0; i < spArray->Obstacles.iSize; ++i) {
            NiPointer<ObstacleData> spObstacle(spArray->Obstacles.GetAt(static_cast<int32_t>(i)));
            if (spObstacle->bActive) {
                for (NavMeshInfo* pNavMeshInfo : spObstacle->Navmeshes) {
                    ObstacleTaskData* pTask = GetTaskForNavMesh(pNavMeshInfo);
                    pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE);
                }
                spObstacle->Navmeshes.Clear(true);
            }
            if (bDrawObstacles) {
                RemoveBoundingBox3DNode(spObstacle);
            }
            RemoveActivationListener(spObstacle->pRigidBody.m_pObject);
            RigidBodyToObstacleMap.RemoveAt(spObstacle->pRigidBody.m_pObject);
            ObstaclesUpdateList.Remove(spObstacle);
        }
        NavMeshObstacleManager::UpdateObstacleDataForReference(apRef, spArray);
        for (uint32_t i = 0; i < spArray->Obstacles.iSize; ++i) {
            NiPointer<ObstacleData> spObstacle(spArray->Obstacles.GetAt(static_cast<int32_t>(i)));
            NavMeshObstacleManager::UpdateObstacleActiveStatus(spObstacle);
            if (spObstacle->bActive) {
                using navmeshes_t = BSScrapArray<NavMeshPtr, 1024>;
                navmeshes_t navmeshes;
                if (NavMeshObstacleManager::GetNavMeshListForObstacle(spObstacle, &navmeshes)) {
                    for (uint32_t j = 0; j < navmeshes.iSize; ++j) {
                        NavMeshInfo* pInfo = navmeshes.GetAt(static_cast<int32_t>(j))->GetNavMeshInfo();
                        ObstacleTaskData* pTask = GetTaskForNavMesh(pInfo);
                        pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE);
                        NavMeshInfo* pAddInfo = navmeshes.GetAt(static_cast<int32_t>(j))->GetNavMeshInfo();
                        spObstacle->Navmeshes.Add(pAddInfo);
                    }
                }
            }
            if (bDrawObstacles) {
                AddBoundingBox3DNode(spObstacle);
            }
            AddActivationListener(spObstacle->pRigidBody.m_pObject);
        }
    }
}

void NavMeshObstacleManager::ReallyAddObstaclesForReference(TESObjectREFR* apRef) {
    NiPointer<ReferenceObstacleArray> spArray(static_cast<ReferenceObstacleArray*>(nullptr));
    if (!FormIDMap.GetAt(apRef->iFormID, spArray)) {
        ReferenceObstacleArray* pArray = new ReferenceObstacleArray;
        spArray = pArray;
        NavMeshObstacleManager::GetObstacleDataForReference(apRef, spArray);
        FormIDMap.SetAt(apRef->iFormID, spArray);
    }
    for (uint32_t uiIndex = 0; uiIndex < spArray->Obstacles.iSize; ++uiIndex) {
        NiPointer<ObstacleData> spObstacle(spArray->Obstacles.GetAt(static_cast<int32_t>(uiIndex)));
        RigidBodyToObstacleMap.SetAt(spObstacle->pRigidBody.m_pObject, spObstacle);
        AddActivationListener(spObstacle->pRigidBody.m_pObject);
        if (spObstacle->pRigidBody->IsActive()) {
            AddObstacleToUpdateList(spObstacle->pRigidBody.m_pObject);
        }
        NavMeshObstacleManager::UpdateObstacleActiveStatus(spObstacle);
        if (bDrawObstacles) {
            AddBoundingBox3DNode(spObstacle);
        }
        if (spObstacle->bActive) {
            using navmeshes_t = BSScrapArray<NavMeshPtr, 1024>;
            navmeshes_t navmeshes;
            if (NavMeshObstacleManager::GetNavMeshListForObstacle(spObstacle, &navmeshes)) {
                for (uint32_t i = 0; i < navmeshes.iSize; ++i) {
                    NavMeshInfo* pInfo = navmeshes.GetAt(static_cast<int32_t>(i))->GetNavMeshInfo();
                    ObstacleTaskData* pTask = GetTaskForNavMesh(pInfo);
                    pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_ADD_OBSTACLE);
                    NavMeshInfo* pAddInfo = navmeshes.GetAt(static_cast<int32_t>(i))->GetNavMeshInfo();
                    spObstacle->Navmeshes.Add(pAddInfo);
                }
            }
        }
    }
}

void NavMeshObstacleManager::ReallyRemoveObstaclesForReference(uint32_t aiFormID) {
    NiPointer<ReferenceObstacleArray> spArray(static_cast<ReferenceObstacleArray*>(nullptr));
    if (FormIDMap.GetAt(aiFormID, spArray)) {
        FormIDMap.RemoveAt(aiFormID);
        for (uint32_t i = 0; i < spArray->Obstacles.iSize; ++i) {
            NiPointer<ObstacleData> spObstacle(spArray->Obstacles.GetAt(static_cast<int32_t>(i)));
            RigidBodyToObstacleMap.RemoveAt(spObstacle->pRigidBody.m_pObject);
            ObstaclesUpdateList.Remove(spObstacle);
            if (bDrawObstacles) {
                RemoveBoundingBox3DNode(spObstacle);
            }
            if (!spObstacle->Navmeshes.IsEmpty()) {
                for (NavMeshInfo* pNavMeshInfo : spObstacle->Navmeshes) {
                    ObstacleTaskData* pTask = GetTaskForNavMesh(pNavMeshInfo);
                    pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE);
                }
                spObstacle->Navmeshes.Clear(true);
            }
        }
    }
}

// Joins the edges the processed tasks cut free. Each new navmesh's potential child edge is matched first against the
// other new navmeshes' child edges, then through the source navmesh's potential parent edges against the navmesh they
// led to (another task's new navmesh, else the loaded one), or, when that navmesh is not loaded, is pointed one way at
// the old portal's triangle. Two edges match when each one's start lies within 50 units (squared 2500, in x and y) of
// the other's end. A parent-edge match connects the child edge, so the portal uses the child edge's
// handle.
bool NavMeshObstacleManager::ReconnectPortals() {
    uint32_t uiTaskCount = ProcessedTasks.GetSize();
    for (uint32_t i = 0; i < uiTaskCount; ++i) {
        ObstacleTaskData* pTask = ProcessedTasks.GetAt(i);
        NavMesh* pNewNavMesh = static_cast<NavMesh*>(pTask->spNewNavMesh);
        if (!pNewNavMesh) {
            continue;
        }
        TaskPortalSwap* pPortalSwaps = &pTask->PortalSwaps;
        for (uint32_t uiChild = 0; uiChild < pPortalSwaps->PotentialChildren.GetSize(); ++uiChild) {
            alignas(NavMeshEdgeHandle) uint8_t aEdgeStorage[sizeof(NavMeshEdgeHandle)];
            NavMeshEdgeHandle& kEdge = *::new (aEdgeStorage)
                                           NavMeshEdgeHandle(pPortalSwaps->PotentialChildren.GetAt(uiChild));
            if (kEdge.usTriangle >= pTask->spNewNavMesh->GetTriangleCount()) {
                Warning("PATHFINDING: (NavMesh Obstacle Manager) Bad triangle index while trying to reconnect portals");
                continue;
            }
            NiPoint3 kEdgeStart = pTask->spNewNavMesh->GetEdgeVertices(kEdge.usTriangle, kEdge.iEdgeIndex)
                                      .Vertices[0]
                                      ->Location;
            NiPoint3 kEdgeEnd = pTask->spNewNavMesh->GetEdgeVertices(kEdge.usTriangle, kEdge.iEdgeIndex)
                                    .Vertices[1]
                                    ->Location;
            bool bConnected = false;
            for (uint32_t j = 0; j < uiTaskCount; ++j) {
                if (j == i) {
                    continue;
                }
                ObstacleTaskData* pOtherTask = ProcessedTasks.GetAt(j);
                NavMesh* pOtherNavMesh = static_cast<NavMesh*>(pOtherTask->spNewNavMesh);
                if (!pOtherNavMesh) {
                    continue;
                }
                TaskPortalSwap* pOtherSwaps = &pOtherTask->PortalSwaps;
                for (uint32_t k = 0; k < pOtherSwaps->PotentialChildren.GetSize(); ++k) {
                    alignas(NavMeshEdgeHandle) uint8_t aOtherStorage[sizeof(NavMeshEdgeHandle)];
                    NavMeshEdgeHandle& kOtherEdge = *::new (aOtherStorage)
                                                        NavMeshEdgeHandle(pOtherSwaps->PotentialChildren.GetAt(k));
                    if (kOtherEdge.usTriangle >= pOtherTask->spNewNavMesh->GetTriangleCount()) {
                        Warning(
                            "PATHFINDING: (NavMesh Obstacle Manager) Bad triangle index while trying to "
                            "reconnect portals"
                        );
                        continue;
                    }
                    NiPoint3 kOtherStart = pOtherTask->spNewNavMesh
                                               ->GetEdgeVertices(kOtherEdge.usTriangle, kOtherEdge.iEdgeIndex)
                                               .Vertices[0]
                                               ->Location;
                    NiPoint3 kOtherEnd = pOtherTask->spNewNavMesh
                                             ->GetEdgeVertices(kOtherEdge.usTriangle, kOtherEdge.iEdgeIndex)
                                             .Vertices[1]
                                             ->Location;
                    if ((kEdgeEnd - kOtherStart).SqrXYLength() < 2500.0f &&
                        (kEdgeStart - kOtherEnd).SqrXYLength() < 2500.0f) {
                        CreatePortal(pTask->spNewNavMesh, kEdge, pOtherTask->spNewNavMesh, kOtherEdge);
                        pTask->bPortalModified = true;
                        pOtherTask->bPortalModified = true;
                        pPortalSwaps->PotentialChildren.RemoveFast(uiChild, 1);
                        --uiChild;
                        pOtherSwaps->PotentialChildren.RemoveFast(k, 1);
                        --k;
                        bConnected = true;
                        break;
                    }
                }
                if (bConnected) {
                    break;
                }
            }
            if (bConnected) {
                continue;
            }
            for (uint32_t uiParent = 0; uiParent < pPortalSwaps->PotentialParents.GetSize(); ++uiParent) {
                alignas(NavMeshEdgeHandle) uint8_t aParentStorage[sizeof(NavMeshEdgeHandle)];
                NavMeshEdgeHandle& kParentEdge = *::new (aParentStorage)
                                                     NavMeshEdgeHandle(pPortalSwaps->PotentialParents.GetAt(uiParent));
                alignas(NavMeshEdgeHandle) uint8_t aMatchStorage[sizeof(NavMeshEdgeHandle)];
                NavMeshEdgeHandle& kMatchEdge = *::new (aMatchStorage) NavMeshEdgeHandle;
                if (!pTask->spSrcMesh->GetMatchingEdge(kParentEdge, &kMatchEdge)) {
                    continue;
                }
                if (kParentEdge.pNavMeshInfo == kMatchEdge.pNavMeshInfo) {
                    continue;
                }
                alignas(NavMeshPtr) uint8_t aMatchMeshStorage[sizeof(NavMeshPtr)];
                NavMeshPtr& spMatchMesh = *reinterpret_cast<NavMeshPtr*>(aMatchMeshStorage);
                ::new (aMatchMeshStorage) NavMeshPtr;
                ObstacleTaskData* pMatchTask = nullptr;
                for (uint32_t m = 0; m < uiTaskCount; ++m) {
                    if (ProcessedTasks.GetAt(m)->pInfo == kMatchEdge.pNavMeshInfo) {
                        ObstacleTaskData* pFoundTask = ProcessedTasks.GetAt(m);
                        spMatchMesh = pFoundTask->spNewNavMesh;
                        pMatchTask = ProcessedTasks.GetAt(m);
                        break;
                    }
                }
                if (!spMatchMesh) {
                    kMatchEdge.pNavMeshInfo->GetNavMesh(spMatchMesh);
                    pMatchTask = nullptr;
                }
                if (!spMatchMesh) {
                    alignas(NavMeshEdgeHandle) uint8_t aOldStorage[sizeof(NavMeshEdgeHandle)];
                    NavMeshEdgeHandle& kOldEdge = *::new (aOldStorage)
                                                      NavMeshEdgeHandle(pPortalSwaps->PotentialParents.GetAt(uiParent));
                    EdgeExtraInfo* pExtraInfo = pTask->spSrcMesh->GetEdgeExtraInfo(
                        kOldEdge.usTriangle,
                        kOldEdge.iEdgeIndex
                    );
                    if (pExtraInfo) {
                        alignas(NavMeshTriHandle) uint8_t aTriStorage[sizeof(NavMeshTriHandle)];
                        NavMeshTriHandle& kOldTri = *::new (
                            aTriStorage
                        ) NavMeshTriHandle(pExtraInfo->Portal.pOtherMesh, pExtraInfo->Portal.sTriangle);
                        CreateOneWayPortal(pTask->spNewNavMesh, kEdge, kOldTri);
                        pPortalSwaps->PotentialChildren.RemoveFast(uiChild, 1);
                        --uiChild;
                        bConnected = true;
                        spMatchMesh = nullptr;
                        break;
                    }
                    spMatchMesh = nullptr;
                    continue;
                }
                if (kMatchEdge.usTriangle >= spMatchMesh->GetTriangleCount()) {
                    spMatchMesh = nullptr;
                    continue;
                }
                if (spMatchMesh->GetTriangle(kMatchEdge.usTriangle)->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                    spMatchMesh = nullptr;
                    continue;
                }
                NiPoint3 kMatchStart = spMatchMesh->GetEdgeVertices(kMatchEdge.usTriangle, kMatchEdge.iEdgeIndex)
                                           .Vertices[0]
                                           ->Location;
                NiPoint3 kMatchEnd = spMatchMesh->GetEdgeVertices(kMatchEdge.usTriangle, kMatchEdge.iEdgeIndex)
                                         .Vertices[1]
                                         ->Location;
                if ((kEdgeEnd - kMatchStart).SqrXYLength() < 2500.0f &&
                    (kEdgeStart - kMatchEnd).SqrXYLength() < 2500.0f) {
                    CreatePortal(pTask->spNewNavMesh, kEdge, spMatchMesh, kMatchEdge);
                    pTask->bPortalModified = true;
                    if (pMatchTask) {
                        pMatchTask->bPortalModified = true;
                    }
                    pPortalSwaps->PotentialChildren.RemoveFast(uiChild, 1);
                    --uiChild;
                    bConnected = true;
                    spMatchMesh = nullptr;
                    break;
                }
                spMatchMesh = nullptr;
            }
        }
    }
    return true;
}

// Each info leaves the two bytes after Portal.sTriangle unset, but AddExtraEdgeInfo copies it field by field,
// so they never reach the navmesh.
bool NavMeshObstacleManager::CreatePortal(
    NavMeshPtr& aspNavMesh1,
    const NavMeshEdgeHandle& aEdge1,
    NavMeshPtr& aspNavMesh2,
    const NavMeshEdgeHandle& aEdge2
) {
    alignas(EdgeExtraInfo) uint8_t aInfo1Storage[sizeof(EdgeExtraInfo)];
    EdgeExtraInfo& kInfo1 = *::new (aInfo1Storage) EdgeExtraInfo;
    kInfo1.Portal.pOtherMesh = aEdge2.pNavMeshInfo;
    kInfo1.Portal.sTriangle = aEdge2.usTriangle;
    uint16_t usInfo1 = aspNavMesh1->AddExtraEdgeInfo(kInfo1);
    aspNavMesh1->GetTriangle(aEdge1.usTriangle)->SetEdgeExtraInfo(aEdge1.iEdgeIndex, usInfo1);

    alignas(EdgeExtraInfo) uint8_t aInfo2Storage[sizeof(EdgeExtraInfo)];
    EdgeExtraInfo& kInfo2 = *::new (aInfo2Storage) EdgeExtraInfo;
    kInfo2.Portal.pOtherMesh = aEdge1.pNavMeshInfo;
    kInfo2.Portal.sTriangle = aEdge1.usTriangle;
    uint16_t usInfo2 = aspNavMesh2->AddExtraEdgeInfo(kInfo2);
    aspNavMesh2->GetTriangle(aEdge2.usTriangle)->SetEdgeExtraInfo(aEdge2.iEdgeIndex, usInfo2);
    return true;
}

// Points edge aEdge1 of aspNavMesh1 at triangle aTri2 through a new extra info.
bool NavMeshObstacleManager::CreateOneWayPortal(
    NavMeshPtr& aspNavMesh1,
    const NavMeshEdgeHandle& aEdge1,
    const NavMeshTriHandle& aTri2
) {
    alignas(EdgeExtraInfo) uint8_t aInfoStorage[sizeof(EdgeExtraInfo)];
    EdgeExtraInfo& kInfo = *::new (aInfoStorage) EdgeExtraInfo;
    kInfo.Portal.pOtherMesh = aTri2.pNavMeshInfo;
    kInfo.Portal.sTriangle = aTri2.usTriangle;
    uint16_t usInfo = aspNavMesh1->AddExtraEdgeInfo(kInfo);
    aspNavMesh1->GetTriangle(aEdge1.usTriangle)->SetEdgeExtraInfo(aEdge1.iEdgeIndex, usInfo);
    return true;
}

// Reopens every door queued for removal: each navmesh under each of the door's closed obstacles gets a remove-door
// operation, and the door's obstacle array moves from ClosedDoorMap to OpenDoorMap. Returns false when the queue is
// empty.
bool NavMeshObstacleManager::ProcessRemovedDoors() {
    if (QueuedListOfDoorsToRemove.IsEmpty()) {
        return false;
    }
    BSSimpleList<uint32_t>* plist = &this->QueuedListOfDoorsToRemove;
    while (plist) {
        uint32_t doorId = plist->GetItem();
        plist = plist->GetNext();
        NiPointer<ReferenceObstacleArray> spArray(static_cast<ReferenceObstacleArray*>(nullptr));
        if (this->ClosedDoorMap.GetAt(doorId, spArray)) {
            for (NiPointer<ObstacleData>& spObstacle : spArray->Obstacles) {
                for (NavMeshInfo* pNavMesh : spObstacle->Navmeshes) {
                    ObstacleTaskData* pTask = this->GetTaskForNavMesh(pNavMesh);
                    pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_REMOVE_DOOR);
                }
            }
            this->ClosedDoorMap.RemoveAt(doorId);
            // The map takes its value by value: spArray is copied into the argument and the callee releases it.
            this->OpenDoorMap.SetAt(doorId, spArray);
        }
    }
    this->QueuedListOfDoorsToRemove.RemoveAll();
    return true;
}

bool NavMeshObstacleManager::ProcessNewDoors() {
    NavMeshInfo* pNavMeshInfo;
    TESObjectREFR* pdoor;
    if (QueuedListOfDoorsToAdd.IsEmpty()) {
        return false;
    }
    bool bupdated = false;
    BSSimpleList<TESObjectREFR*>* plist = &this->QueuedListOfDoorsToAdd;
    while (plist && !plist->IsEmpty()) {
        pdoor = plist->m_item;
        if (pdoor->Get3D()) {
            ReferenceObstacleArray* pNewArray = new ReferenceObstacleArray;
            NiPointer<ReferenceObstacleArray> spArray(pNewArray);
            if (NavMeshObstacleManager::GetObstacleDataForReference(pdoor, spArray)) {
                for (NiPointer<ObstacleData>& spObstacle : spArray->Obstacles) {
                    using navmeshes_t = BSScrapArray<NavMeshPtr, 1024>;
                    navmeshes_t navmeshes;
                    if (NavMeshObstacleManager::GetNavMeshListForObstacle(spObstacle, &navmeshes)) {
                        for (const NavMeshPtr& spNavMesh : navmeshes) {
                            NavMeshInfo* pInfo = spNavMesh->GetNavMeshInfo();
                            ObstacleTaskData* pTask = this->GetTaskForNavMesh(pInfo);
                            pTask->AddOperation(spObstacle, OBSTACLE_TASK_TYPE_ADD_DOOR);
                            pNavMeshInfo = spNavMesh->GetNavMeshInfo();
                            spObstacle->Navmeshes.Add(pNavMeshInfo);
                        }
                    }
                }
                this->OpenDoorMap.RemoveAt(pdoor->iFormID);
                // The map takes its value by value: spArray is copied into the argument and the callee releases it.
                this->ClosedDoorMap.SetAt(pdoor->iFormID, spArray);
                bupdated = true;
            }
            this->QueuedListOfDoorsToAdd.Remove(pdoor);
            plist = &this->QueuedListOfDoorsToAdd;
        }
        else {
            plist = plist->m_pkNext;
        }
    }
    return bupdated;
}

// Stores apTaskData in the processed array while holding its lock.
void NavMeshObstacleManager::AddProcessedTask(ObstacleTaskData* apTaskData) {
    ProcessedTaskLock.Lock("NavMeshObstacleManager::AddProcessedTask()");
    ProcessedTasks.Add(apTaskData);
    ProcessedTaskLock.Unlock();
}

void NavMeshObstacleManager::Release() {
    ClearAllObstacles();
    if (pListener) {
        delete pListener;
        pListener = nullptr;
    }
    if (pRemoveListener) {
        delete pRemoveListener;
        pRemoveListener = nullptr;
    }
    ObstacleTaskletGroup.Close();
    BSTaskletManager::QInstance().DestroyTaskGroup(ObstacleTaskletGroup);
}

void NavMeshObstacleManager::ClearAllObstacles() {
    NiPointer<ReferenceObstacleArray> spObstacleArray(nullptr);
    NiTMapItem<uint32_t, NiPointer<ReferenceObstacleArray>>* pPos = FormIDMap.GetFirstPos();
    while (pPos) {
        uint32_t uiFormID;
        FormIDMap.GetNext(pPos, uiFormID, spObstacleArray);
        for (uint32_t i = 0; i < spObstacleArray->Obstacles.GetSize(); ++i) {
            NiPointer<ObstacleData> spObstacle(spObstacleArray->Obstacles.GetAt(static_cast<int32_t>(i)));
            if (!QueuedListOfRefsToRemove.IsInList(uiFormID)) {
                RemoveActivationListener(spObstacle->pRigidBody);
            }
            for (uint32_t j = 0; j < spObstacle->Navmeshes.GetSize(); ++j) {
                NavMeshPtr spNavMesh;
                NavMeshInfo* pNavMeshInfo = spObstacle->Navmeshes.GetAt(static_cast<int32_t>(j));
                if (pNavMeshInfo->GetNavMeshEvenDisabled(spNavMesh)) {
                    spNavMesh->NavMesh::ClearAllObstacles();
                }
            }
        }
    }
    FormIDMap.RemoveAll();
    OpenDoorMap.RemoveAll();
    ClosedDoorMap.RemoveAll();
    RigidBodyToObstacleMap.RemoveAll();
    QueuedListOfDoorsToAdd.RemoveAll();
    QueuedListOfDoorsToRemove.RemoveAll();
    QueuedListOfRefsToAdd.RemoveAll();
    QueuedListOfRefsToRemove.RemoveAll();
    ObstaclesUpdateList.RemoveAll();
    ObstaclesToAddToUpdateList.RemoveAll(false);
    ObstaclesToRemoveFromUpdateList.RemoveAll(false);
    if (bDrawObstacles) {
        DrawObstacles(false);
    }
}

// Moves each rigid body queued for activation whose obstacle is known onto the update list, unless it is there already.
// The queue's count is its virtual GetCount, called through a pointer.
void NavMeshObstacleManager::ProcessActivatedObstacles() {
    LockFreeQueue<bhkRigidBody*>* pQueue = &ObstaclesToAddToUpdateList;
    uint32_t iobstacleToAddCount = pQueue->GetCount();
    for (uint32_t i = 0; i < iobstacleToAddCount; ++i) {
        bhkRigidBody* prb;
        ObstaclesToAddToUpdateList.Pop(prb);
        NiPointer<ObstacleData> spObstacle(static_cast<ObstacleData*>(nullptr));
        if (RigidBodyToObstacleMap.GetAt(prb, spObstacle) && !ObstaclesUpdateList.IsInList(spObstacle)) {
            ObstaclesUpdateList.AddHead(spObstacle);
        }
    }
}

// Takes each rigid body queued for deactivation whose obstacle is known off the update list, first re-cutting the
// obstacle when the body has moved more than 5 units (squared distance 25) from its centre. The queue's count is its
// virtual GetCount, called through a pointer.
void NavMeshObstacleManager::ProcessDeactivatedObstacles() {
    LockFreeQueue<bhkRigidBody*>* pQueue = &ObstaclesToRemoveFromUpdateList;
    uint32_t iobstacleToRemoveCount = pQueue->GetCount();
    for (uint32_t i = 0; i < iobstacleToRemoveCount; ++i) {
        bhkRigidBody* prb;
        this->ObstaclesToRemoveFromUpdateList.Pop(prb);
        NiPointer<ObstacleData> spObstacle(static_cast<ObstacleData*>(nullptr));
        if (RigidBodyToObstacleMap.GetAt(prb, spObstacle)) {
            NiPoint3 pos;
            hkVector4 hkPos(prb->Gethk()->getTranslation());
            HK2NI(pos, hkPos);
            if ((pos - spObstacle->Center).SqrLength() > 25.0f) {
                this->UpdateObstacle(spObstacle);
            }
            ObstaclesUpdateList.Remove(spObstacle);
        }
    }
}

void NavMeshObstacleManager::DrawObstacles(bool abDraw) {
    NiTMapItem<uint32_t, NiPointer<ReferenceObstacleArray>>* it;
    if (abDraw) {
        if (spObstacleRootNode.m_pObject) {
            spObstacleRootNode->RemoveAllChildren();
        }
        else {
            NiNode* pRootNode = new NiNode(0);
            spObstacleRootNode = pRootNode;
            void* pAlphaProp = new NiAlphaProperty;
            NiPointer<NiAlphaProperty> spAlphaProp(static_cast<NiAlphaProperty*>(pAlphaProp));
            spAlphaProp->SetAlphaBlending(true);
            void* pMaterial = new NiMaterialProperty;
            NiPointer<NiMaterialProperty> spmat(static_cast<NiMaterialProperty*>(pMaterial));
            spmat->SetEmittance(NiColor(1.0f, 0.0f, 1.0f));
            spmat->SetAlpha(0.25f);
            spObstacleRootNode->AttachProperty(spAlphaProp.m_pObject);
            spObstacleRootNode->AttachProperty(spmat.m_pObject);
            NiNode* pObjRoot = TES::pInstance->pObjRoot;
            pObjRoot->AttachChild(spObstacleRootNode.m_pObject, 1);
            NiUpdateData UpdateData(0.0f, false, false);
            TES::pInstance->pObjRoot->Update(UpdateData);
        }
        it = FormIDMap.GetFirstPos();
        while (it) {
            uint32_t formID;
            NiPointer<ReferenceObstacleArray> spArray(static_cast<ReferenceObstacleArray*>(nullptr));
            FormIDMap.GetNext(it, formID, spArray);
            for (uint32_t i = 0; i < spArray->Obstacles.iSize; ++i) {
                NiPointer<ObstacleData> spObstacle(spArray->Obstacles.GetAt(static_cast<int32_t>(i)));
                NiPointer<NiTriShape> spobsNode(static_cast<NiTriShape*>(nullptr));
                CreateBoundingBox3DNode(spObstacle, spobsNode);
                spObstacleRootNode->AttachChild(spobsNode.m_pObject, 1);
            }
        }
        spObstacleRootNode->UpdateProperties();
        NiUpdateData UpdateData(0.0f, false, false);
        spObstacleRootNode->Update(UpdateData);
        bDrawObstacles = true;
    }
    else {
        if (spObstacleRootNode.m_pObject) {
            spObstacleRootNode->RemoveAllChildren();
        }
        bDrawObstacles = false;
    }
}

void NavMeshObstacleManager::CreateBoundingBox3DNode(
    const NiPointer<ObstacleData>& aBox,
    NiPointer<NiTriShape>& aspTriShape
) {
    NiPoint3 size;
    size = aBox->BoxMax - aBox->BoxMin;
    NiColorA colora(1.0f, 0.0f, 1.0f, 0.5f);
    NiPoint3 dfLeft(aBox->BoxMin.x, aBox->BoxMin.y, aBox->BoxMin.z);
    NiPoint3 dfRight(aBox->BoxMax.x, aBox->BoxMin.y, aBox->BoxMin.z);
    NiPoint3 ufRight(aBox->BoxMax.x, aBox->BoxMin.y, aBox->BoxMax.z);
    NiPoint3 ufLeft(aBox->BoxMin.x, aBox->BoxMin.y, aBox->BoxMax.z);
    NiPoint3 dbLeft(aBox->BoxMin.x, aBox->BoxMax.y, aBox->BoxMin.z);
    NiPoint3 dbRight(aBox->BoxMax.x, aBox->BoxMax.y, aBox->BoxMin.z);
    NiPoint3 ubRight(aBox->BoxMax.x, aBox->BoxMax.y, aBox->BoxMax.z);
    NiPoint3 ubLeft(aBox->BoxMin.x, aBox->BoxMax.y, aBox->BoxMax.z);
    NiPointer<NiTriShape> spBox(
        MakeQuadBox(ubLeft, ubRight, ufRight, ufLeft, dbLeft, dbRight, dfRight, dfLeft, colora, true)
    );
    spBox->SetTranslate(aBox->Center);
    spBox->SetRotate(aBox->Orientation);
    aBox->sp3DNode = spBox;
    aspTriShape = spBox;
}

void NavMeshObstacleManager::RemoveBoundingBox3DNode(const NiPointer<ObstacleData>& aBox) {
    if (spObstacleRootNode.m_pObject && aBox->sp3DNode.m_pObject) {
        spObstacleRootNode->DetachChild(aBox->sp3DNode.m_pObject);
        aBox->sp3DNode = static_cast<NiTriShape*>(nullptr);
    }
}

// The manager is GetInstance's function static `theManager`.
NavMeshObstacleManager* NavMeshObstacleManager::GetInstance() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshObstacleManager.cpp",
        210
    );
    static NavMeshObstacleManager theManager;
    return &theManager;
}

void NavMeshObstacleManager::UpdateObstacleActiveStatus(NiPointer<ObstacleData>& aspObstacle) {
    NiPoint3 kAABB;
    NiPoint3* kAABBMin = &aspObstacle->aabbMin;
    ObstacleData* pData = aspObstacle.m_pObject;
    kAABB = pData->aabbMax - *kAABBMin;
    aspObstacle->bActive = !(kAABB.z < 48.0f) &&
        (!(kAABB.z < 48.0f + 48.0f) || (!(kAABB.x < 16.0f) || !(kAABB.y < 16.0f)));
}

// Collects the navmeshes of the obstacle's cells whose bounds, raised by 128 units, overlap the obstacle's world
// bounds; always true.
bool NavMeshObstacleManager::GetNavMeshListForObstacle(
    const NiPointer<ObstacleData>& apData,
    BSSimpleArray<NavMeshPtr, 1024>* apReturnNavMeshArray
) {
    NiPoint3 navmeshMin;
    NiPoint3 navmeshMax;
    BSScrapArray<TESObjectCELL*> kCells(32u, 0u, nullptr);
    if (NavMeshObstacleManager::GetCellListForObstacle(apData, &kCells)) {
        for (TESObjectCELL* pCell : kCells) {
            NavMeshArray* parray = pCell->GetNavMeshArray();
            if (parray) {
                for (uint32_t j = 0; j < parray->GetNavMeshCount(); ++j) {
                    bool bhasBounds = parray->GetNavMeshByIndex(j)->NavMesh::GetBounds(navmeshMin, navmeshMax);
                    if (bhasBounds) {
                        navmeshMax.z = navmeshMax.z + 128.0;
                        if (Pathing::DoAABBsOverlap(navmeshMin, navmeshMax, apData->aabbMin, apData->aabbMax)) {
                            apReturnNavMeshArray->Add(parray->GetNavMeshByIndex(j));
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool NavMeshObstacleManager::FindFirstObstacleFor3D(NiAVObject* ap3D, ObstacleData* apRetObstacle) {
    CFilter* pFilter;
    CFilter kFilter;
    COL_LAYER layer;
    bhkRigidBody* prb;
    bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(ap3D);
    if (pcol &&
        ((prb = pcol->GetRigidBody()) != nullptr &&
         ((pFilter = prb->GetCollisionFilter(&kFilter), ((layer = pFilter->GetLayer()) == L_ANIMSTATIC)) ||
          (layer == L_STATIC ||
           (layer == L_CLUTTER ||
            (layer == L_PROPS || (layer == L_DEBRIS_LARGE || layer == L_TRANSPARENT_SMALL_ANIM))))))) {
        apRetObstacle->spParentArray = nullptr;
        apRetObstacle->pRigidBody = prb;
        NavMeshObstacleManager::UpdateObstacleData(apRetObstacle);
        return true;
    }

    NiNode* pnode = ap3D->IsNode();
    if (pnode) {
        for (uint32_t i = 0; i < pnode->GetChildCount(); ++i) {
            if (pnode->GetAt(i)) {
                NiAVObject* pchild = pnode->GetAt(i);
                if (NavMeshObstacleManager::FindFirstObstacleFor3D(pchild, apRetObstacle)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Adds an obstacle for every rigid body of a static, clutter, prop, large debris or small transparent animated layer in
// ap3D and below it. The filter is read into a local CFilter, whose address GetCollisionFilter returns.
bool NavMeshObstacleManager::AddObstaclesFor3DRecursive(
    NiAVObject* ap3D,
    NiPointer<ReferenceObstacleArray>& apRetObstacleArray
) {
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(ap3D);
    if (pCollisionObject) {
        bhkWorldObject* pWorldObject = pCollisionObject->GetWorldObject();
        if (pWorldObject) {
            CFilter kFilter;
            pWorldObject->GetCollisionFilter(&kFilter);
            COL_LAYER eLayer = kFilter.GetLayer();
            if (eLayer == L_ANIMSTATIC ||
                eLayer == L_CLUTTER ||
                eLayer == L_PROPS ||
                eLayer == L_DEBRIS_LARGE ||
                eLayer == L_TRANSPARENT_SMALL_ANIM) {
                ObstacleData* pObstacle = new ObstacleData;
                NiPointer<ObstacleData> spObstacleData(pObstacle);
                ReferenceObstacleArray* pParentArray = apRetObstacleArray;
                spObstacleData->spParentArray = pParentArray;
                spObstacleData->pRigidBody = static_cast<bhkRigidBody*>(pWorldObject);
                UpdateObstacleData(spObstacleData);
                apRetObstacleArray->Obstacles.Add(spObstacleData);
            }
        }
    }
    NiNode* pNode = ap3D->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            if (pNode->GetAt(i)) {
                AddObstaclesFor3DRecursive(pNode->GetAt(i), apRetObstacleArray);
            }
        }
    }
    return true;
}

// Fits apObstacle's box to its rigid body: the body's axis nearest the vertical becomes up (its column with the largest
// |z|, flipped to point up with the axis after it), then the box is turned to keep Z vertical and keep that axis's
// heading. BoxMin/BoxMax bound the shape's local bounds' corners in that frame and aabbMin/aabbMax their world
// positions. A world corner lying within 15 units of a 4096-unit cell line in X or Y is pushed 30 units along +X or +Y
// and its box corner recomputed from it. False without a Havok body.
bool NavMeshObstacleManager::UpdateObstacleData(ObstacleData* apObstacle) {
    bhkRigidBody* pRigidBody = apObstacle->pRigidBody;
    if (!pRigidBody) {
        return false;
    }
    hkpRigidBody* phkRigidBody = pRigidBody->Gethk();
    if (!phkRigidBody) {
        return false;
    }
    hkTransform kHavokTransform(phkRigidBody->getTransform());
    NiTransform kTransform;
    kTransform.m_fScale = 1.0f;
    HK2NI(kTransform, kHavokTransform);
    hkAabb kAabb;
    pRigidBody->GetAabbLocal(kAabb);
    // The angles are not used.
    NiPoint3 kAngles;
    kTransform.m_Rotate.ToEulerAnglesXYZ(kAngles.x, kAngles.y, kAngles.z);
    NiPoint3 kMin;
    NiPoint3 kMax;
    HK2NI(kMin, kAabb.m_min);
    HK2NI(kMax, kAabb.m_max);
    apObstacle->Center = kTransform.m_Translate;

    NiPoint3 akCols[3];
    for (uint32_t uiCol = 0; uiCol < 3; ++uiCol) {
        kTransform.m_Rotate.GetCol(uiCol, akCols[uiCol]);
    }
    NiPoint3 kZAxis;
    NiPoint3 kYAxis;
    NiPoint3 kXAxis;
    // Takes |col0.z| before the other column's, and each test fails on NaN.
    if (std::fabs(akCols[0].z) > std::fabs(akCols[1].z)) {
        if (std::fabs(akCols[0].z) > std::fabs(akCols[2].z)) {
            kZAxis = akCols[0];
            kXAxis = akCols[1];
            kYAxis = akCols[2];
        }
        else {
            kZAxis = akCols[2];
            kXAxis = akCols[0];
            kYAxis = akCols[1];
        }
    }
    else if (std::fabs(akCols[1].z) > std::fabs(akCols[2].z)) {
        kZAxis = akCols[1];
        kXAxis = akCols[2];
        kYAxis = akCols[0];
    }
    else {
        kZAxis = akCols[2];
        kXAxis = akCols[0];
        kYAxis = akCols[1];
    }
    if (kZAxis.z < 0.0) {
        kZAxis = -kZAxis;
        kXAxis = -kXAxis;
    }
    kZAxis = NiPoint3::UNIT_Z;
    kYAxis = kZAxis.UnitCross(kXAxis);
    kXAxis = -kYAxis.Cross(kZAxis);
    apObstacle->Orientation.SetCol(0, kXAxis);
    apObstacle->Orientation.SetCol(1, kYAxis);
    apObstacle->Orientation.SetCol(2, kZAxis);
    NiMatrix3 kLocalRotate = apObstacle->Orientation.Transpose() * kTransform.m_Rotate;

    NiPoint3 akBoxCorners[8];
    akBoxCorners[0] = kLocalRotate * NiPoint3(kMin.x, kMin.y, kMin.z);
    akBoxCorners[1] = kLocalRotate * NiPoint3(kMax.x, kMin.y, kMin.z);
    akBoxCorners[2] = kLocalRotate * NiPoint3(kMin.x, kMax.y, kMin.z);
    akBoxCorners[3] = kLocalRotate * NiPoint3(kMax.x, kMax.y, kMin.z);
    akBoxCorners[4] = kLocalRotate * NiPoint3(kMin.x, kMin.y, kMax.z);
    akBoxCorners[5] = kLocalRotate * NiPoint3(kMax.x, kMin.y, kMax.z);
    akBoxCorners[6] = kLocalRotate * NiPoint3(kMin.x, kMax.y, kMax.z);
    akBoxCorners[7] = kLocalRotate * NiPoint3(kMax.x, kMax.y, kMax.z);
    NiPoint3 akWorldCorners[8];
    for (uint32_t i = 0; i < 8; ++i) {
        akWorldCorners[i] = apObstacle->Orientation * akBoxCorners[i] + apObstacle->Center;
    }

    NiMatrix3 kInverse = apObstacle->Orientation.Transpose();
    for (uint32_t j = 0; j < 8; ++j) {
        if (std::fabs(std::fmod(akWorldCorners[j].x, 4096.0f)) < 15.0f) {
            akWorldCorners[j].x = 30.0f + akWorldCorners[j].x;
            akBoxCorners[j] = kInverse * (akWorldCorners[j] - apObstacle->Center);
        }
        if (std::fabs(std::fmod(akWorldCorners[j].y, 4096.0f)) < 15.0f) {
            akWorldCorners[j].y = 30.0f + akWorldCorners[j].y;
            akBoxCorners[j] = kInverse * (akWorldCorners[j] - apObstacle->Center);
        }
    }

    apObstacle->BoxMin = NiPoint3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    apObstacle->BoxMax = NiPoint3(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    );
    for (uint32_t k = 0; k < 8; ++k) {
        apObstacle->BoxMin.x = BSMIN(apObstacle->BoxMin.x, akBoxCorners[k].x);
        apObstacle->BoxMin.y = BSMIN(apObstacle->BoxMin.y, akBoxCorners[k].y);
        apObstacle->BoxMin.z = BSMIN(apObstacle->BoxMin.z, akBoxCorners[k].z);
        apObstacle->BoxMax.x = BSMAX(apObstacle->BoxMax.x, akBoxCorners[k].x);
        apObstacle->BoxMax.y = BSMAX(apObstacle->BoxMax.y, akBoxCorners[k].y);
        apObstacle->BoxMax.z = BSMAX(apObstacle->BoxMax.z, akBoxCorners[k].z);
    }
    apObstacle->aabbMin = NiPoint3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    apObstacle->aabbMax = NiPoint3(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    );
    for (uint32_t m = 0; m < 8; ++m) {
        apObstacle->aabbMin.x = BSMIN(apObstacle->aabbMin.x, akWorldCorners[m].x);
        apObstacle->aabbMin.y = BSMIN(apObstacle->aabbMin.y, akWorldCorners[m].y);
        apObstacle->aabbMin.z = BSMIN(apObstacle->aabbMin.z, akWorldCorners[m].z);
        apObstacle->aabbMax.x = BSMAX(apObstacle->aabbMax.x, akWorldCorners[m].x);
        apObstacle->aabbMax.y = BSMAX(apObstacle->aabbMax.y, akWorldCorners[m].y);
        apObstacle->aabbMax.z = BSMAX(apObstacle->aabbMax.z, akWorldCorners[m].z);
    }
    apObstacle->iLastUpdateTime = appTimer.GetTime();
    return true;
}

// Fills apRetObstacleArray with apRef's obstacles, tagged with its form ID.
bool NavMeshObstacleManager::GetObstacleDataForReference(
    TESObjectREFR* apRef,
    NiPointer<ReferenceObstacleArray>& apRetObstacleArray
) {
    uint32_t uiFormID = apRef->GetFormID();
    apRetObstacleArray->iFormID = uiFormID;
    return UpdateObstacleDataForReference(apRef, apRetObstacleArray);
}

// Rebuilds apRetObstacleArray's obstacles from apRef's 3D; false when apRef has no 3D.
bool NavMeshObstacleManager::UpdateObstacleDataForReference(
    TESObjectREFR* apRef,
    NiPointer<ReferenceObstacleArray>& apRetObstacleArray
) {
    apRetObstacleArray->Obstacles.Clear(true);
    if (!apRef->Get3D()) {
        return false;
    }
    return AddObstaclesFor3DRecursive(apRef->Get3D(), apRetObstacleArray);
}

bool NavMeshObstacleManager::GetCellListForObstacle(
    const NiPointer<ObstacleData>& apData,
    BSSimpleArray<TESObjectCELL*, 1024>* apReturnCellArray
) {
    apReturnCellArray->Clear(false);
    TESObjectCELL* pcell = TES::pInstance->GetCurrentCell();
    if (!pcell) {
        return false;
    }
    if (pcell->GetInterior()) {
        apReturnCellArray->Add(pcell);
    }
    else {
        int32_t cellx = static_cast<int32_t>(apData->Center.x) >> 12;
        int32_t celly = static_cast<int32_t>(apData->Center.y) >> 12;
        for (int32_t x = cellx - 1; x <= cellx + 1; ++x) {
            for (int32_t y = celly - 1; y <= celly + 1; ++y) {
                TESWorldSpace* pworld = pcell->GetWorldSpace();
                TESObjectCELL* pgridCell = pworld->GetCellFromCellCoord(x, y);
                if (pgridCell && pgridCell->GetAttached()) {
                    NiPoint3 gridcellMin(
                        static_cast<float>(x << 12),
                        static_cast<float>(y << 12),
                        -std::numeric_limits<float>::max()
                    );
                    float fmaxY = gridcellMin.y + 4096.0;
                    float fmaxX = gridcellMin.x + 4096.0;
                    NiPoint3 gridcellMax(fmaxX, fmaxY, std::numeric_limits<float>::max());
                    if (Pathing::DoAABBsOverlap(gridcellMin, gridcellMax, apData->aabbMin, apData->aabbMax)) {
                        apReturnCellArray->Add(pgridCell);
                    }
                }
            }
        }
    }
    return true;
}

bool NavMeshObstacleManager::IsPointInBox(
    const NiPoint3& aPoint,
    const NiPointer<ObstacleData>& aBox,
    float afMaxHeightAbove,
    float afMaxHeightBelow
) {
    NiPoint3 localPoint;
    NiMatrix3* pOrientation = &aBox->Orientation;
    localPoint = (aPoint - aBox->Center) * *pOrientation;
    if (localPoint.z < -afMaxHeightAbove || afMaxHeightBelow < static_cast<double>(localPoint.z)) {
        return false;
    }
    double dminX = aBox->BoxMin.x;
    if (dminX > localPoint.x) {
        return false;
    }
    double dmaxX = aBox->BoxMax.x;
    if (dmaxX < localPoint.x) {
        return false;
    }
    double dminY = aBox->BoxMin.y;
    if (!(dminY > localPoint.y)) {
        double dmaxY = aBox->BoxMax.y;
        if (!(dmaxY < localPoint.y)) {
            return true;
        }
    }
    return false;
}
