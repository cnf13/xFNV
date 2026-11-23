#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMovie/CAsyncStream.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class BSFile;

// An async stream over a file the game opened through its archives (a movie in a BSA), guarded by a critical section.
class CBSAStream : public CAsyncStream {
public:
    CBSAStream(char* apPath);
    ~CBSAStream() override;

    int32_t Seek(int64_t aiPosition) override;
    int32_t Read(uint8_t* apBuffer, uint32_t auiLength, int32_t abAligned, uint32_t* apBytesRead) override;
    int64_t GetFileSize(int64_t* apSizeAvailable) override;
    uint32_t Alignment() override;
    void Lock() override;
    void Unlock() override;

    // True while pFile is open (not null).
    bool IsOpen();

    static CBSAStream* Construct(void* apMemory, char* apPath);

    char cPath[260];
    BSFile* pFile;
    // Construct and Destruct run BSCriticalSection's constructor and destructor on kLock, and the 0x18 bytes before the
    // 8-aligned ullFileSize hold no more. Lock and Unlock are bare Enter/LeaveCriticalSection calls. NiCriticalSection
    // (0x80 bytes) does not fit here.
    BSCriticalSection kLock;
    // No function of the class reads or writes it.
    uint8_t padding124[4];
    // The file's size: GetFileSize stores pFile->GetFileSize() into it and Seek compares the position against it as a
    // 64-bit value.
    _LARGE_INTEGER ullFileSize;
    // The bytes read so far: Construct clears it, Seek stores the position in it and Read advances it by the length it
    // clamps.
    _LARGE_INTEGER ullBytesRead;
};

// A one-pointer lock guard on the caller's stack: CBSAStream::Read builds one on its kLock. The constructor stores
// apLock and enters it; the destructor leaves it.
struct BSCriticalSectionGuard {
    BSCriticalSectionGuard(BSCriticalSection* apLock);
    ~BSCriticalSectionGuard();

    BSCriticalSection* pLock;
};
