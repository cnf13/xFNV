#pragma once

#include "BSMain/QueuedFiles.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Animation;

// The load of a replacement animation's file for an Animation that is already loaded: it finishes when the loads of
// its QueuedReplacementKF children have been post-processed. The PDB's source module is ModelLoader.cpp.
class QueuedReplacementKFList : public QueuedFile {
public:
	QueuedReplacementKFList(Animation* apAnim, IO_TASK_PRIORITY aePriority);
	~QueuedReplacementKFList() override;

	void CheckFinished() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;

	void AddPostProcessingChild();
	void NotifyChildFinishedPostProcessing();

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	Animation* pAnim;
	uint32_t iPostProcessingChildCount;
	// The class is 8-byte aligned (BSTask<int64_t>'s Key), so the four bytes after this count are its tail padding:
	// the PDB has no member after it and no exe function touches +0x34 (IDA's dword34 is a gap filler).
	uint32_t iPostProcessedChildCount;
};

inline constexpr fnvd::Vtable QueuedReplacementKFList_vftable{};
