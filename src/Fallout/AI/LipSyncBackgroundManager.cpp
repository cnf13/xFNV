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
	AutoMemContext apMemory(MEM_CONTEXT_DIALOGUE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\LipSyncBackgroundManager.cpp", 21);
	BSFile* apFile = GetFile(ARCHIVE_TYPE_INDEX_VOICES, ARCHIVE_TYPE_VOICES);
	if (!apFile) {
		BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
		CurrentAIProcess->SetWaitingforLipFile(0);
		BaseProcess* v1 = pActor->pCurrentProcess;
		v1->SetLipFileFailed(1);
	}
	pLipSynicAnim = LipSynchAnim::LoadFromFile(apFile);
}

void LipTask::Finish() {
	BaseProcess* v1;
	BaseProcess* v2;
	BaseProcess* currentAIProcess;
	if (pActor->MobileObject::GetCurrentAIProcess()) {
		currentAIProcess = pActor->MobileObject::GetCurrentAIProcess();
		currentAIProcess->SetLipAnim(pLipSynicAnim);
		v2 = pActor->MobileObject::GetCurrentAIProcess();
		v2->SetWaitingforLipFile((bool)0);
		if (!pLipSynicAnim) {
			v1 = pActor->MobileObject::GetCurrentAIProcess();
			v1->SetLipFileFailed((bool)1);
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
	return (uint8_t)3;
}

LipSyncBackgroundManager::LipSyncBackgroundManager() : LipMap(10, 37, 12) {}

LipSyncBackgroundManager::~LipSyncBackgroundManager() {
	LockFreeMap<MobileObject*, NiPointer<LipTask>>::LockFreeMapIterator iterator;
	while (!iterator.GetFinished()) {
		MobileObject* pactor = nullptr;
		NiPointer<LipTask> sptask(nullptr);
		if (LipMap.GetNext(iterator, pactor, sptask, true)) {
			RemoveTask(pactor);
			pIOManager->CancelTask(sptask.m_pObject, 0);
		}
	}
}

void LipSyncBackgroundManager::RemoveTask(MobileObject* apActor) {
	LipMap.RemoveAt(apActor);
}

void LipSyncBackgroundManager::CancelLipFile(MobileObject* apActor) {
	FNVD_INLINED_LOCAL(NiPointer<LipTask>, sptask, std::nullopt, std::nullopt);
	::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
	if (LipMap.GetAt(apActor, sptask)) {
		RemoveTask(apActor);
		LipTask* v2 = sptask.m_pObject;
		pIOManager->CancelTask(v2, 0);
	}
	sptask.~NiPointer<LipTask>();
}

void LipSyncBackgroundManager::BuildLipSynicFile(MobileObject* apActor, const char* apFilename) {
	LipTask* pnewtask;
	FNVD_INLINED_LOCAL(NiPointer<LipTask>, v6, std::nullopt, std::nullopt);
	AutoMemContext tempMemContext(MEM_CONTEXT_DIALOGUE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\LipSyncBackgroundManager.cpp", 97);
	CancelLipFile(apActor);
	LipTask* v7 = static_cast<LipTask*>(ourMalloc(sizeof(LipTask)));
	if (v7) {
		pnewtask = ::new (v7) LipTask(this, apActor, apFilename);
	} else {
		pnewtask = 0;
	}
	::new (static_cast<void*>(&v6)) NiPointer<QueuedFile>(static_cast<QueuedFile*>(pnewtask));
	bool bresult = LipMap.SetAt(apActor, v6, 0);
	v6.~NiPointer<LipTask>();
	BaseProcess* pProcess = apActor->pCurrentProcess;
	pProcess->SetLipFileFailed(0);
	pIOManager->AddTask(pnewtask);
}
