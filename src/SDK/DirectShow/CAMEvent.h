#pragma once

#include <cstdint>

class BSSystemEvent;

// The DirectShow base classes' Win32 event wrapper.
// The PDB has no source path; its location was chosen with the DirectShow base classes.
class CAMEvent {
public:
    // IDA name; not in the PDB. HANDLE m_event; the exe's event handle, here the standard-C++ object in its slot.
    BSSystemEvent* m_event;

public:
    CAMEvent(int32_t bManualReset);
    ~CAMEvent();
};
