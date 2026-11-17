#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Reconstruction.hpp"

// The task that creates a temp effect (the PDB's class; its objects come from a NiTObjectPool, whose containers'
// arrays are freed through NiTNewInterface<BSTECreateTask>::Deallocate 0x682740).
class BSTECreateTask : public NiTask {
public:
	~BSTECreateTask() override;

	NiPointer<BSTempEffect> m_pEffect;

	static NiRTTI ms_RTTI;
	// The PDB's. Guards ms_kPool; the exe builds it with the critical section constructor it shares with
	// BSCriticalSection (0x44FAE0).
	static NiCriticalSection ms_kPoolCriticalSection;
	// The PDB's. The pool GetFreeObject takes the tasks from; the exe builds it with the pool constructor it shares
	// with NiTPrimitivePool<void *> (0x682510).
	static NiTObjectPool<BSTECreateTask> ms_kPool;
};
