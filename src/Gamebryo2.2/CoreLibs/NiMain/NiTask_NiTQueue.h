#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

class NiTask;

// NiTPrimitiveQueue<NiTask*>. A ring buffer: m_uiHead is the next task out, m_uiTail the next slot in, and m_uiHead is
// -1 while the queue is empty.
class NiTask_NiTQueue : public NiMemObject {
public:
	NiTask* m_pBase;
	uint32_t m_uiAlloced;
	uint32_t m_uiHead;
	uint32_t m_uiTail;
};
