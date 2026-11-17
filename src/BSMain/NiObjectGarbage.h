#pragma once

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

void SetGarbageGrowBy(uint16_t a1);

// The garbage array's out-of-line RemoveAt, defined in NiObjectGarbage.cpp.
using NiTArrayNiPointerNiRefObjectNew = NiTArray<NiPointer<NiRefObject>, NiTNewInterface<NiPointer<NiRefObject>>>;

// Defers the release of Gamebryo objects the game threads may still be using. Only what the reconstructed code uses
// is declared.
class NiObjectGarbage {
public:
	// Releases everything queued; with abTryLock only if the lock is free.
	static void ClearAll(bool abTryLock);
	// Queues apObject for release.
	static void Add(NiRefObject* apObject);
	static void Init(uint32_t aiTime);
	// Shrinks the queue back to 100 slots when it grew past them.
	static void ResetSize();
	// Grows the queue to auBuffer slots when it holds fewer.
	static void IncreaseBuffer(uint32_t auBuffer);
	static uint32_t GetCount();
	static void Update();
	// Whether the queue holds no object.
	static bool IsEmpty();

	static inline int32_t iMaxTime = 4;
	// The queued objects.
	static NiTObjectArray<NiPointer<NiRefObject>> ObjectGarbageA;
	static BSSpinLock kCritSec;
};
