#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

class NiFile;
class NiPixelData;
class NiPixelFormat;

// Base of the image file readers: checks a file, reads its header and reads it into NiPixelData under a lock.
class NiImageReader : public NiMemObject {
public:
    NiImageReader();
    virtual ~NiImageReader();

    virtual bool CanReadFile(const char*) const = 0;
    virtual NiPixelData* ReadFile(NiFile&, NiPixelData*) = 0;
    virtual bool ReadHeader(NiFile&, uint32_t&, uint32_t&, NiPixelFormat&, bool&, uint32_t&) = 0;

    // The deleting destructor frees through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);

    NiCriticalSection m_kReadCriticalSection;
};
