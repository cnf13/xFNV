#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

class ProcessArrays {
public:
	// The mobile objects being processed this frame (IDA: NiTPrimitiveArray_void kMobileObjects, 0x10 bytes: the
	// vtable, m_pBase, m_usMaxSize, m_usSize, m_usESize and m_usGrowBy).
	NiTPrimitiveArray<void*> kMobileObjects;
	uint32_t uiBeginOffsets[4];
	uint32_t uiEndOffsets[4];
	uint32_t uiStoredBeginOffsets[4];
};
