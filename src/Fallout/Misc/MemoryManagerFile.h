#pragma once

#include "BSCore/IMemoryManagerFile.h"
#include "BSSystem/BSSystemFile.h"

// The memory manager's debug output file, over an OS file.
class MemoryManagerFile : public IMemoryManagerFile {
public:
    // Implicit constructor, for a local constructed later in place (File starts closed).
    MemoryManagerFile() = default;
    MemoryManagerFile(const char* apFileName);
    ~MemoryManagerFile() override;

    int32_t Read(void* apData, uint32_t aSize) override;
    int32_t Write(const void* apData, uint32_t aSize) override;
    int32_t Size() override;
    void Seek(int32_t aiPos) override;

    static void operator delete(void* apMemory);

    BSSystemFile File;
};
