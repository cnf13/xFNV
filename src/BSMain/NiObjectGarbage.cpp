#include "BSMain/NiObjectGarbage.h"

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

NiTObjectArray<NiPointer<NiRefObject>> NiObjectGarbage::ObjectGarbageA(0, 1000);
BSSpinLock NiObjectGarbage::kCritSec;

void SetGarbageGrowBy(uint16_t a1) {
	NiObjectGarbage::ObjectGarbageA.m_usGrowBy = a1;
}

void NiObjectGarbage::ClearAll(bool abTryLock) {
	bool bLocked = 1;
	if (abTryLock) {
		bLocked = NiObjectGarbage::kCritSec.TryLock();
	} else {
		NiObjectGarbage::kCritSec.Lock(0);
	}
	if (!bLocked || TryEnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection)) {
		if (bLocked) {
			NiObjectGarbage::ObjectGarbageA.RemoveAll();
			NiObjectGarbage::ResetSize();
			LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
			if (!--NiObjectGarbage::kCritSec.uiLockCount) {
				NiObjectGarbage::kCritSec.uiOwningThread = 0;
			}
		}
	} else {
		if (!--NiObjectGarbage::kCritSec.uiLockCount) {
			NiObjectGarbage::kCritSec.uiOwningThread = 0;
		}
	}
}

void NiObjectGarbage::Add(NiRefObject* apObject) {
	NiObjectGarbage::kCritSec.Lock(0);
	{
		NiPointer<NiRefObject> a2 = apObject;
		NiObjectGarbage::ObjectGarbageA.Add(a2);
	}
	if (!--NiObjectGarbage::kCritSec.uiLockCount) {
		NiObjectGarbage::kCritSec.uiOwningThread = 0;
	}
}

void NiObjectGarbage::Init(uint32_t aiTime) {
	NiObjectGarbage::iMaxTime = (int32_t)aiTime;
}

void NiObjectGarbage::ResetSize() {
	if (ObjectGarbageA.m_usSize > 100) {
		ObjectGarbageA.SetSize(100);
	}
}

void NiObjectGarbage::IncreaseBuffer(uint32_t auBuffer) {
	if (ObjectGarbageA.m_usSize < auBuffer) {
		ObjectGarbageA.SetSize(auBuffer);
	}
}

uint32_t NiObjectGarbage::GetCount() {
	return (uint32_t)NiObjectGarbage::ObjectGarbageA.m_usESize;
}

void NiObjectGarbage::Update() {
	NiObjectGarbage::kCritSec.Lock(0);
	uint32_t inextTime = NiObjectGarbage::iMaxTime + GetTickCount();
	uint32_t itime = 0;
	uint32_t inumObjects = NiObjectGarbage::ObjectGarbageA.m_usESize;
	if (NiObjectGarbage::ObjectGarbageA.m_usESize > 200u) {
		inextTime += 1000;
	}
	while (inumObjects && itime < inextTime) {
		{
			NiPointer<NiRefObject> result = (NiObjectGarbage::ObjectGarbageA.RemoveAt(inumObjects - 1));
		}
		itime = GetTickCount();
		--inumObjects;
	}
	if (!inumObjects) {
		NiObjectGarbage::ResetSize();
	}
	if (!--NiObjectGarbage::kCritSec.uiLockCount) {
		NiObjectGarbage::kCritSec.uiOwningThread = 0;
	}
}

bool NiObjectGarbage::IsEmpty() {
	return (NiObjectGarbage::ObjectGarbageA.m_usESize == 0);
}
