#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

// A block of memory shared by the objects of one group in old (pre-10.1.0.114) files.
class NiObjectGroup : public NiMemObject {
public:
	NiObjectGroup(uint32_t uiBytes);

	void FreeVertexBuffer();

	uint32_t m_uiSize;
	void* m_pvBuffer;
	void* m_pvFree;
	uint32_t m_uiRefCount;
};
