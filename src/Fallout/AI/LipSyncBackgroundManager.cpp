#include "Fallout/AI/LipSyncBackgroundManager.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

LipTask::LipTask(LipSyncBackgroundManager* apOwner, MobileObject* apActor, const char* apFilename)
  : QueuedFileEntry(IO_TASK_PRIORITY_CRITICAL), pActor(apActor), pLipSynicAnim(nullptr), pOwner(apOwner) {
    SetFileName(apFilename);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_VOICES);
}

LipTask::~LipTask() {}

void LipTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_DIALOGUE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\LipSyncBackgroundManager.cpp",
        21
    );
    BSFile* pFile = GetFile(ARCHIVE_TYPE_INDEX_VOICES, ARCHIVE_TYPE_VOICES);
    if (!pFile) {
        pActor->pCurrentProcess->SetWaitingforLipFile(false);
        pActor->pCurrentProcess->SetLipFileFailed(true);
    }
    pLipSynicAnim = LipSynchAnim::LoadFromFile(pFile);
}

void LipTask::Finish() {
    if (pActor->MobileObject::GetCurrentAIProcess()) {
        pActor->MobileObject::GetCurrentAIProcess()->SetLipAnim(pLipSynicAnim);
        pActor->MobileObject::GetCurrentAIProcess()->SetWaitingforLipFile(false);
        if (!pLipSynicAnim) {
            pActor->MobileObject::GetCurrentAIProcess()->SetLipFileFailed(true);
        }
    }
    pOwner->RemoveTask(pActor);
}

void LipTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    if (aePreviousState != BS_TASK_STATE_COMPLETED) {
        pOwner->RemoveTask(pActor);
    }
}

bool LipTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    // The type description parameter is char*, though the callers pass constant text, hence the const_cast.
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("liptask"));
}

uint8_t LipTask::GetFileIndex() {
    return 3;
}

LipSyncBackgroundManager::LipSyncBackgroundManager() : LipMap(10, 37, 12) {}

LipSyncBackgroundManager::~LipSyncBackgroundManager() {
    LockFreeMap<MobileObject*, NiPointer<LipTask>>::LockFreeMapIterator iterator;
    while (!iterator.GetFinished()) {
        MobileObject* pActor = nullptr;
        NiPointer<LipTask> spTask(nullptr);
        if (LipMap.GetNext(iterator, pActor, spTask, true)) {
            RemoveTask(pActor);
            pIOManager->CancelTask(spTask.m_pObject, nullptr);
        }
    }
}

void LipSyncBackgroundManager::RemoveTask(MobileObject* apActor) {
    LipMap.RemoveAt(apActor);
}

void LipSyncBackgroundManager::CancelLipFile(MobileObject* apActor) {
    NiPointer<LipTask> spTask(static_cast<LipTask*>(nullptr));
    if (LipMap.GetAt(apActor, spTask)) {
        RemoveTask(apActor);
        pIOManager->CancelTask(spTask.m_pObject, nullptr);
    }
}

void LipSyncBackgroundManager::BuildLipSynicFile(MobileObject* apActor, const char* apFilename) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_DIALOGUE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\LipSyncBackgroundManager.cpp",
        97
    );
    CancelLipFile(apActor);
    LipTask* pNewTask = new LipTask(this, apActor, apFilename);
    LipMap.SetAt(apActor, NiPointer<LipTask>(pNewTask), false);
    BaseProcess* pProcess = apActor->pCurrentProcess;
    pProcess->SetLipFileFailed(false);
    pIOManager->AddTask(pNewTask);
}
