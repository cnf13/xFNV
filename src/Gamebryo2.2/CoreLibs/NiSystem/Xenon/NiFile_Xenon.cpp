#include "Gamebryo2.2/CoreLibs/NiSystem/Xenon/NiFile_Xenon.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSList.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/ehvec.h"
#include "platform/Paths.h"

#include <filesystem>
#include <system_error>

using ReadErrorFunc = void (*)();
static ReadErrorFunc spReadErrorFunc{};

namespace OEI {
    uint32_t OEI::IO::fread(void* the_buffer, uint32_t the_block_count, uint32_t the_size, std::FILE* the_file) {
        OEI::IO::IO_Request new_request;
        new_request.m_wait_handle = nullptr;
        new_request.m_eType = OEI::IO::IRT_READ;
        new_request.m_pFile = the_file;
        new_request.m_pMemory = the_buffer;
        new_request.m_uiSize = the_size;
        OEI::IO::add_work_item_to_queue_and_wait(new_request);
        return new_request.result.result_unsigned_int;
    }

    uint32_t OEI::IO::fwrite(const void* the_buffer, uint32_t the_block_count, uint32_t the_size, std::FILE* the_file) {
        OEI::IO::IO_Request new_request;
        new_request.m_wait_handle = nullptr;
        new_request.m_eType = OEI::IO::IRT_WRITE;
        new_request.m_pFile = the_file;
        // The request's buffer serves reads too, while fwrite takes it as const, hence the const_cast.
        new_request.m_pMemory = const_cast<void*>(the_buffer);
        new_request.m_uiSize = the_size;
        OEI::IO::add_work_item_to_queue_and_wait(new_request);
        return new_request.result.result_unsigned_int;
    }

    int32_t OEI::IO::fseek(std::FILE* the_file, int32_t the_offset, OEI::IO::Seek_Whence_Type the_whence) {
        OEI::IO::IO_Request new_request;
        new_request.m_pMemory = nullptr;
        new_request.m_wait_handle = nullptr;
        new_request.m_eType = OEI::IO::IRT_SEEK;
        new_request.m_pFile = the_file;
        new_request.m_eWhence = the_whence;
        new_request.m_uiSize = the_offset;
        OEI::IO::add_work_item_to_queue_and_wait(new_request);
        return new_request.result.result_int;
    }

    // Takes the lock, links the request into the work item queue and signals the worker thread's semaphore.
    void OEI::IO::add_work_item_to_queue(OEI::IO::IO_Request& new_request) {
        EnterCriticalSection(&s_queue_lock->CriticalSection);
        BSList<OEI::IO::IO_Request*>* s_work_item_queue = ::s_work_item_queue;
        BSListItem<OEI::IO::IO_Request*>* apItem = ::s_work_item_queue->NewItem();
        apItem->m_element = &new_request;
        s_work_item_queue->AddNodeTail(apItem);
        LeaveCriticalSection(&s_queue_lock->CriticalSection);
        BSSemaphore* lpAddend = s_event_sema;
        s_event_sema->hSemaphore->Release();
        InterlockedIncrementInt32(&lpAddend->iCount);
    }

    // Queues the request, then waits on the request's semaphore until the worker thread signals it.
    void OEI::IO::add_work_item_to_queue_and_wait(OEI::IO::IO_Request& new_request) {
        // EXE-INLINED 0x4538E0
        EnterCriticalSection(&s_queue_lock->CriticalSection);
        new_request.m_wait_handle = s_wait_handle_queue->RemoveHead();
        BSList<OEI::IO::IO_Request*>* s_work_item_queue = ::s_work_item_queue;
        BSListItem<OEI::IO::IO_Request*>* apItem = ::s_work_item_queue->NewItem();
        apItem->m_element = &new_request;
        s_work_item_queue->AddNodeTail(apItem);
        // EXE-INLINED 0x82F1F0
        LeaveCriticalSection(&s_queue_lock->CriticalSection);
        // EXE-INLINED 0x442550
        BSSemaphore* lpAddend = s_event_sema;
        s_event_sema->hSemaphore->Release();
        InterlockedIncrementInt32(&lpAddend->iCount);
        // EXE-INLINED 0x4424E0
        BSSemaphore* m_wait_handle = new_request.m_wait_handle;
        // INFINITE: no timeout.
        m_wait_handle->hSemaphore->Wait();
        InterlockedDecrementInt32(&m_wait_handle->iCount);
        // EXE-INLINED 0x4538E0
        EnterCriticalSection(&s_queue_lock->CriticalSection);
        BSList<BSSemaphore*>* s_wait_handle_queue = ::s_wait_handle_queue;
        BSListItem<BSSemaphore*>* pkItem = ::s_wait_handle_queue->NewItem();
        pkItem->m_element = new_request.m_wait_handle;
        s_wait_handle_queue->AddNodeHead(pkItem);
        // EXE-INLINED 0x82F1F0
        LeaveCriticalSection(&s_queue_lock->CriticalSection);
    }
} // namespace OEI

NiFile::NiFile() : m_uiCurrentFilePos(0), m_pBuffer(nullptr), m_pFile(nullptr) {}

// Copies what fits into the write buffer, flushes, and writes the rest straight to the file when it fills a whole
// buffer.
size_t NiFile::FileWrite(const void* pBuffer, uint32_t uiBytes) {
    if (!m_bGood) {
        return 0;
    }
    const uint32_t uiAvailBufferBytes = m_uiBufferAllocSize - m_uiPos;
    uint32_t uiWritten = 0;
    if (uiBytes > uiAvailBufferBytes) {
        if (uiAvailBufferBytes) {
            rt::memcpy(m_pBuffer + m_uiPos, pBuffer, uiAvailBufferBytes);
            pBuffer = static_cast<const char*>(pBuffer) + uiAvailBufferBytes;
            uiBytes -= uiAvailBufferBytes;
            uiWritten = uiAvailBufferBytes;
            m_uiPos = m_uiBufferAllocSize;
        }
        if (!Flush()) {
            return 0;
        }
        if (uiBytes >= m_uiBufferAllocSize) {
            const uint32_t uiBytesWritten = OEI::IO::fwrite(pBuffer, 1, uiBytes, m_pFile);
            return static_cast<size_t>(uiWritten + uiBytesWritten);
        }
    }
    rt::memcpy(m_pBuffer + m_uiPos, pBuffer, uiBytes);
    m_uiPos += uiBytes;
    return static_cast<size_t>(uiWritten + uiBytes);
}

// Reads bytes from the underlying stream and advances its file position.
uint32_t NiFile::DiskRead(void* pBuffer, uint32_t uiBytes) {
    const uint32_t uiBytesRead = OEI::IO::fread(pBuffer, 1, uiBytes, m_pFile);
    m_uiCurrentFilePos += uiBytesRead;
    return uiBytesRead;
}

// Flushes buffered writes, or clears the read buffer for a read-only stream.
bool NiFile::Flush() {
    if (m_eMode == READ_ONLY) {
        m_uiBufferReadSize = 0;
        m_uiPos = 0;
        return true;
    }

    if (m_uiPos && OEI::IO::fwrite(m_pBuffer, 1, m_uiPos, m_pFile) != m_uiPos) {
        m_bGood = false;
        return false;
    }
    m_uiPos = 0;
    return true;
}

// Copies buffered bytes first, then reads directly or refills the file buffer as needed.
size_t NiFile::FileRead(void* pBuffer, uint32_t uiBytes) {
    if (!m_bGood) {
        return 0;
    }

    uint32_t uiRemaining = m_uiBufferReadSize - m_uiPos;
    uint32_t uiCopied = 0;
    char* pDestination = static_cast<char*>(pBuffer);
    if (uiBytes > uiRemaining) {
        if (uiRemaining) {
            rt::memcpy(pDestination, m_pBuffer + m_uiPos, uiRemaining);
            pDestination += uiRemaining;
            uiBytes -= uiRemaining;
            uiCopied = uiRemaining;
        }

        Flush();
        const uint32_t uiBufferAllocSize = m_uiBufferAllocSize;
        std::FILE* pFile = m_pFile;
        if (uiBytes > uiBufferAllocSize) {
            const uint32_t uiBytesRead = OEI::IO::fread(pDestination, 1, uiBytes, pFile);
            m_uiCurrentFilePos += uiBytesRead;
            return uiCopied + uiBytesRead;
        }

        const uint32_t uiBytesRead = OEI::IO::fread(m_pBuffer, 1, uiBufferAllocSize, pFile);
        m_uiCurrentFilePos += uiBytesRead;
        m_uiBufferReadSize = uiBytesRead;
        if (uiBytesRead < uiBytes) {
            uiBytes = uiBytesRead;
        }
    }

    rt::memcpy(pDestination, m_pBuffer + m_uiPos, uiBytes);
    m_uiPos += uiBytes;
    return uiCopied + uiBytes;
}

// GetFileAttributesA's FILE_ATTRIBUTE_DIRECTORY: false when the path is missing (INVALID_FILE_ATTRIBUTES) or no
// folder.
bool NiFile::DefaultDirectoryExistsFunc(const char* pcDirName) {
    std::error_code kError;
    return std::filesystem::is_directory(platform::NativePath(pcDirName), kError);
}

// CreateDirectoryA(pcDirName, NULL) != 0: false when the folder exists already or cannot be made.
bool NiFile::DefaultCreateDirectoryFunc(const char* pcDirName) {
    std::error_code kError;
    return std::filesystem::create_directory(platform::NativePath(pcDirName), kError);
}

void OEI::IO::terminate() {
    OEI::IO::IO_Request exit_request;
    exit_request.m_pFile = nullptr;
    exit_request.m_pMemory = nullptr;
    exit_request.m_wait_handle = nullptr;
    exit_request.m_eType = OEI::IO::IRT_QUIT;
    OEI::IO::add_work_item_to_queue(exit_request);
    BSSemaphore* lpAddend = s_worker_thread_done;
    s_worker_thread_done->hSemaphore->Wait();
    InterlockedDecrementInt32(&lpAddend->iCount);
    s_thread_0->Close();
    if (s_thread_0) {
        delete s_thread_0;
    }
    s_thread_0 = nullptr;
    if (s_wait_handles) {
        delete[] s_wait_handles;
    }
    s_wait_handles = nullptr;
    if (s_wait_handle_queue) {
        delete s_wait_handle_queue;
    }
    s_wait_handle_queue = nullptr;
    if (s_work_item_queue) {
        delete s_work_item_queue;
    }
    s_work_item_queue = nullptr;
    BSCriticalSection* lpCriticalSection = s_queue_lock;
    if (s_queue_lock) {
        DeleteCriticalSection(&s_queue_lock->CriticalSection);
        MemoryManager_Singleton.Deallocate(lpCriticalSection);
    }
    s_queue_lock = nullptr;
    BSSemaphore* Block = s_worker_thread_done;
    if (s_worker_thread_done) {
        delete s_worker_thread_done->hSemaphore;
        MemoryManager_Singleton.Deallocate(Block);
    }
    s_worker_thread_done = nullptr;
    BSSemaphore* apMem = s_event_sema;
    if (s_event_sema) {
        delete s_event_sema->hSemaphore;
        MemoryManager_Singleton.Deallocate(apMem);
    }
    s_event_sema = nullptr;
}

void OEI::IO::initialize() {
    OEI::IO::IO_Thread* v0;
    BSSemaphore* v1;
    BSList<BSSemaphore*>* v2;
    BSList<OEI::IO::IO_Request*>* v3;
    BSCriticalSection* v4;
    BSSemaphore* v5;
    BSSemaphore* v6;
    BSSemaphore* v21 = static_cast<BSSemaphore*>(MemoryManager_Singleton.Allocate(sizeof(BSSemaphore)));
    if (v21) {
        // EXE-INLINED 0x86C6A0
        v21->iCount = 0;
        v21->iMaxCount = 0xFFFF;
        v21->hSemaphore = new BSSystemSemaphore(v21->iCount, v21->iMaxCount);
        v6 = v21;
    }
    else {
        v6 = nullptr;
    }
    s_event_sema = v6;
    BSSemaphore* v19 = static_cast<BSSemaphore*>(MemoryManager_Singleton.Allocate(sizeof(BSSemaphore)));
    if (v19) {
        // EXE-INLINED 0x86C6A0
        v19->iCount = 0;
        v19->iMaxCount = 1;
        v19->hSemaphore = new BSSystemSemaphore(v19->iCount, v19->iMaxCount);
        v5 = v19;
    }
    else {
        v5 = nullptr;
    }
    s_worker_thread_done = v5;
    BSCriticalSection* lpCriticalSection =
        static_cast<BSCriticalSection*>(MemoryManager_Singleton.Allocate(sizeof(BSCriticalSection)));
    if (lpCriticalSection) {
        InitializeCriticalSection(&lpCriticalSection->CriticalSection);
        v4 = lpCriticalSection;
    }
    else {
        v4 = nullptr;
    }
    s_queue_lock = v4;
    BSList<OEI::IO::IO_Request*>* v15 = static_cast<BSList<OEI::IO::IO_Request*>*>(
        MemoryManager_Singleton.Allocate(sizeof(BSList<OEI::IO::IO_Request*>))
    );
    if (v15) {
        v3 = ::new (v15) BSList<OEI::IO::IO_Request*>;
    }
    else {
        v3 = nullptr;
    }
    s_work_item_queue = v3;
    BSList<BSSemaphore*>* v13 =
        static_cast<BSList<BSSemaphore*>*>(MemoryManager_Singleton.Allocate(sizeof(BSList<BSSemaphore*>)));
    if (v13) {
        v2 = ::new (v13) BSList<BSSemaphore*>;
    }
    else {
        v2 = nullptr;
    }
    s_wait_handle_queue = v2;
    s_wait_handles = new BSSemaphore[32];
    for (uint32_t i = 0; i < 32; ++i) {
        BSSemaphore* arElement = &s_wait_handles[i];
        s_wait_handle_queue->AddHead(arElement);
    }
    OEI::IO::IO_Thread* apMemory =
        static_cast<OEI::IO::IO_Thread*>(MemoryManager_Singleton.Allocate(sizeof(OEI::IO::IO_Thread)));
    if (apMemory) {
        v0 = ::new (apMemory) OEI::IO::IO_Thread;
    }
    else {
        v0 = nullptr;
    }
    s_thread_0 = v0;
    s_thread_0->Initialize(BSThread::SMALL_STACK_SIZE, "Serialized IO Thread", false);
}
