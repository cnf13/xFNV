#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSList.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "SDK/MSVC/stdio.h"

NiFile::NiFile(const char* pcName, NiFile::OpenMode eMode, uint32_t uiBufferSize)
  : m_uiBufferAllocSize(uiBufferSize), m_uiBufferReadSize(0), m_uiPos(0), m_uiCurrentFilePos(0), m_eMode(eMode) {
    NiFile::SetEndianSwap(false);
    const char* pcMode;
    if (eMode) {
        pcMode = "wb";
        if (eMode != NiFile::WRITE_ONLY) {
            pcMode = "ab";
        }
    }
    else {
        pcMode = "rb";
    }
    m_bGood = !rt::fopen_s(&m_pFile, pcName, pcMode) && m_pFile;
    if (m_bGood && uiBufferSize) {
        m_pBuffer = static_cast<char*>(_NiMalloc(uiBufferSize));
    }
    else {
        m_pBuffer = nullptr;
    }
}

NiFile::~NiFile() {
    if (m_bGood && m_pFile) {
        Flush();
        std::fclose(m_pFile);
    }
    _NiFree(m_pBuffer);
}

// Seeks relative to the current position through the two-argument virtual.
void NiFile::Seek(int32_t iNumBytes) {
    Seek(iNumBytes, ms_iSeekCur);
}

// Picks the swapping or plain read and write callbacks.
void NiFile::SetEndianSwap(bool bDoSwap) {
    if (bDoSwap) {
        m_pfnRead = &NiFile::ReadAndSwap;
        m_pfnWrite = &NiFile::WriteAndSwap;
    }
    else {
        m_pfnRead = &NiFile::ReadNoSwap;
        m_pfnWrite = &NiFile::WriteNoSwap;
    }
}

// Seeks within the buffered file when possible, otherwise flushes and seeks the underlying stream.
void NiFile::Seek(int32_t iOffset, int32_t iWhence) {
    if (!m_bGood) {
        return;
    }

    if (iWhence == 1) {
        const uint32_t uiPos = m_uiPos;
        const int32_t iNewPos = static_cast<int32_t>(uiPos + static_cast<uint32_t>(iOffset));
        if (iNewPos >= 0 && iNewPos < static_cast<int32_t>(m_uiBufferReadSize)) {
            m_uiAbsoluteCurrentPos += static_cast<uint32_t>(iOffset);
            m_uiPos = static_cast<uint32_t>(iNewPos);
            return;
        }
        if (m_eMode == READ_ONLY) {
            iOffset = static_cast<int32_t>(static_cast<uint32_t>(iOffset) + (uiPos - m_uiBufferReadSize));
        }
    }

    Flush();
    m_bGood = OEI::IO::fseek(m_pFile, iOffset, static_cast<OEI::IO::Seek_Whence_Type>(iWhence)) == 0;
    if (m_bGood) {
        const int32_t iFilePos = std::ftell(m_pFile);
        m_uiCurrentFilePos = static_cast<uint32_t>(iFilePos);
        m_uiAbsoluteCurrentPos = static_cast<uint32_t>(iFilePos);
    }
}

// Measures the stream by seeking to its end and back; 0 when the position or size can't be read.
uint32_t NiFile::GetFileSize() const {
    int32_t iCurrent = std::ftell(m_pFile);
    if (iCurrent < 0) {
        return 0;
    }
    std::fseek(m_pFile, 0, 2);
    int32_t iSize = std::ftell(m_pFile);
    std::fseek(m_pFile, iCurrent, 0);
    return iSize < 0 ? 0 : static_cast<uint32_t>(iSize);
}

// Returns the currently installed file-creation callback's result.
NiFile* NiFile::GetFile(const char* pcName, OpenMode eMode, uint32_t uiSize) {
    return ms_pfnFileCreateFunc(pcName, eMode, uiSize);
}

// Returns the currently installed file-access callback's result.
bool NiFile::Access(const char* pcName, OpenMode eMode) {
    return ms_pfnFileAccessFunc(pcName, eMode);
}

// Restores the default access callback before installing a non-null replacement.
void NiFile::SetFileAccessFunc(FileAccessFunc pfnFunc) {
    ms_pfnFileAccessFunc = &NiFile::DefaultFileAccessFunc;
    if (pfnFunc) {
        ms_pfnFileAccessFunc = pfnFunc;
    }
}

// Restores the default create callback before installing a non-null replacement.
void NiFile::SetFileCreateFunc(FileCreateFunc pfnFunc) {
    ms_pfnFileCreateFunc = &NiFile::DefaultFileCreateFunc;
    if (pfnFunc) {
        ms_pfnFileCreateFunc = pfnFunc;
    }
}

NiFile* NiFile::DefaultFileCreateFunc(const char* pcName, OpenMode eMode, uint32_t uiSize) {
    return new NiFile(pcName, eMode, uiSize);
}

bool NiFile::DirectoryExists(const char* pcDirName) {
    return ms_pfnDirExistsFunc(pcDirName);
}

// Walks the standardized copy and creates each missing directory level at every separator, then the full path. The scan
// deliberately runs over all 260 bytes of acFullPathCopy, past the copied string's terminator;
// strcpy_s leaves the rest of the stack buffer unwritten, so it reads uninitialised bytes (and acts on any separator
// byte in them).
bool NiFile::CreateDirectoryRecursive(const char* pcFullPath) {
    char acFullPathCopy[260];
    if (ms_pfnDirExistsFunc(pcFullPath)) {
        return true;
    }
    if (strlen(pcFullPath) > 260) {
        return false;
    }
    strcpy_s(acFullPathCopy, 260, pcFullPath);
    NiPath::Standardize(acFullPathCopy);
    uint32_t uiStart = 0;
    if (acFullPathCopy[1] == ':') {
        uiStart = 2;
    }
    do {
        char c = acFullPathCopy[uiStart];
        if (c != '\\' && c != '/') {
            break;
        }
        ++uiStart;
    }
    while (uiStart < 260);
    for (uint32_t i = uiStart; i < 260; ++i) {
        char c = acFullPathCopy[i];
        if (c == '/' || c == '\\') {
            acFullPathCopy[i] = '\0';
            if (!ms_pfnDirExistsFunc(acFullPathCopy) && !ms_pfnCreateDirFunc(acFullPathCopy)) {
                return false;
            }
            acFullPathCopy[i] = c;
        }
    }
    return ms_pfnDirExistsFunc(acFullPathCopy) || ms_pfnCreateDirFunc(acFullPathCopy);
}

// Swaps a temporary copy of pvBuffer and writes that.
uint32_t NiFile::WriteAndSwap(
    NiBinaryStream* pkThis,
    const void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (uiBytes == 0) {
        return 0;
    }
    void* pvSwapped = _NiMalloc(uiBytes);
    rt::memcpy(pvSwapped, pvBuffer, uiBytes);
    DoByteSwap(pvSwapped, uiBytes, puiComponentSizes, uiNumComponents);
    uint32_t uiWritten = static_cast<NiFile*>(pkThis)->FileWrite(pvSwapped, uiBytes);
    _NiFree(pvSwapped);
    return uiWritten;
}

uint32_t NiFile::ReadNoSwap(
    NiBinaryStream* pkThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    return static_cast<NiFile*>(pkThis)->NiFile::FileRead(pvBuffer, uiBytes);
}

uint32_t NiFile::WriteNoSwap(
    NiBinaryStream* pkThis,
    const void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    return static_cast<NiFile*>(pkThis)->NiFile::FileWrite(pvBuffer, uiBytes);
}

uint32_t NiFile::ReadAndSwap(
    NiBinaryStream* pkThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (!uiBytes) {
        return 0;
    }
    uint32_t uiBytesRead = static_cast<NiFile*>(pkThis)->NiFile::FileRead(pvBuffer, uiBytes);
    NiBinaryStream::DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
    return uiBytesRead;
}

bool NiFile::DefaultFileAccessFunc(const char* pcName, NiFile::OpenMode eMode) {
    NiFile kFile(pcName, eMode, 0);
    bool m_bGood = kFile.m_bGood;
    return m_bGood;
}

OEI::IO::IO_Thread::IO_Thread() {}

// The worker loop: waits for a request, takes it off the queue under the lock and runs its CRT call, then signals the
// request's semaphore; a quit request ends the loop and signals s_worker_thread_done.
uint32_t OEI::IO::IO_Thread::ThreadProc() {
    SetThreadProcessor(1u);
    while (true) {
        s_event_sema->Wait();
        s_queue_lock->Lock();
        OEI::IO::IO_Request* pRequest = s_work_item_queue->RemoveHead();
        s_queue_lock->Unlock();
        bool bNeedToQuit = false;
        switch (pRequest->m_eType) {
            case OEI::IO::IRT_SEEK: {
                // On PC the request keeps a seek's offset in its size field (OEI::IO::fseek stores it there); it is
                // passed as fseek's long offset and the whence as its int origin.
                pRequest->result.result_int = std::fseek(
                    pRequest->m_pFile,
                    static_cast<int32_t>(pRequest->m_uiSize),
                    pRequest->m_eWhence
                );
                break;
            }
            case OEI::IO::IRT_READ: {
                pRequest->result
                    .result_int = std::fread(pRequest->m_pMemory, 1u, pRequest->m_uiSize, pRequest->m_pFile);
                break;
            }
            case OEI::IO::IRT_WRITE: {
                pRequest->result
                    .result_int = std::fwrite(pRequest->m_pMemory, 1u, pRequest->m_uiSize, pRequest->m_pFile);
                break;
            }
            case OEI::IO::IRT_QUIT: {
                bNeedToQuit = true;
                break;
            }
            default: {
                break;
            }
        }
        if (bNeedToQuit) {
            break;
        }
        if (pRequest->m_wait_handle) {
            pRequest->m_wait_handle->Signal();
        }
    }
    s_worker_thread_done->Signal();
    return 0;
}
