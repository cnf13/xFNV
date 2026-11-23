#pragma once

#include "BSCore/BSList.h"
#include "BSCore/BSThread.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "SDK/MSVC/stdio.h"

#include <stdint.h>

class BSSemaphore;

// Buffered disk file stream over the C runtime's FILE.
class NiFile : public NiBinaryStream {
public:
    enum OpenMode : uint32_t {
        READ_ONLY = 0,
        WRITE_ONLY = 1,
        APPEND_ONLY = 2,
    };

    // File-system callbacks stored in the executable's global function-pointer slots.
    using FileCreateFunc = NiFile* (*)(const char*, OpenMode, uint32_t);
    using FileAccessFunc = bool (*)(const char*, OpenMode);
    // The installed directory-exists callback, called with one path.
    using DirExistsFunc = bool (*)(const char*);
    // The installed directory-creation callback, called with one path.
    using CreateDirFunc = bool (*)(const char*);

    NiFile();
    NiFile(const char* pcName, OpenMode eMode, uint32_t uiBufferSize);
    ~NiFile() override;

    operator bool() const override {
        return m_bGood;
    }
    void Seek(int32_t iNumBytes) override;
    void SetEndianSwap(bool bDoSwap) override;

    // iWhence is one of ms_iSeekSet/Cur/End
    virtual void Seek(int32_t iOffset, int32_t iWhence);
    virtual char* GetDevice() {
        return nullptr;
    }
    // the file's length in bytes, 0 on error
    virtual uint32_t GetFileSize() const;

    // Writes uiBytes bytes of pBuffer to the file and returns the count written.
    size_t FileWrite(const void* pBuffer, uint32_t uiBytes);
    uint32_t DiskRead(void* pBuffer, uint32_t uiBytes);
    bool Flush();
    size_t FileRead(void* pBuffer, uint32_t uiBytes);

    static NiFile* GetFile(const char* pcName, OpenMode eMode, uint32_t uiSize);
    static bool Access(const char* pcName, OpenMode eMode);
    static void SetFileAccessFunc(FileAccessFunc pfnFunc);
    static void SetFileCreateFunc(FileCreateFunc pfnFunc);
    // The default create callback: a new NiFile on pcName.
    static NiFile* DefaultFileCreateFunc(const char* pcName, OpenMode eMode, uint32_t uiSize);
    // Whether the directory exists, through the installed directory callback.
    static bool DirectoryExists(const char* pcDirName);
    // The default directory-exists callback: GetFileAttributesA's directory bit.
    static bool DefaultDirectoryExistsFunc(const char* pcDirName);
    // The default directory-creation callback: CreateDirectoryA(pcDirName, NULL) != 0.
    static bool DefaultCreateDirectoryFunc(const char* pcDirName);
    // Creates pcFullPath and every missing directory above it; true when it exists afterwards.
    static bool CreateDirectoryRecursive(const char* pcFullPath);
    // The swapping write function installed in m_pfnWrite.
    static uint32_t WriteAndSwap(
        NiBinaryStream* pkThis,
        const void* pvBuffer,
        uint32_t uiBytes,
        uint32_t* puiComponentSizes,
        uint32_t uiNumComponents
    );
    static uint32_t ReadNoSwap(
        NiBinaryStream* pkThis,
        void* pvBuffer,
        uint32_t uiBytes,
        uint32_t* puiComponentSizes,
        uint32_t uiNumComponents
    );
    static uint32_t WriteNoSwap(
        NiBinaryStream* pkThis,
        const void* pvBuffer,
        uint32_t uiBytes,
        uint32_t* puiComponentSizes,
        uint32_t uiNumComponents
    );
    static uint32_t ReadAndSwap(
        NiBinaryStream* pkThis,
        void* pvBuffer,
        uint32_t uiBytes,
        uint32_t* puiComponentSizes,
        uint32_t uiNumComponents
    );
    static bool DefaultFileAccessFunc(const char* pcName, NiFile::OpenMode eMode);

    uint32_t m_uiBufferAllocSize;
    uint32_t m_uiBufferReadSize;
    uint32_t m_uiPos;
    uint32_t m_uiCurrentFilePos;
    char* m_pBuffer;
    std::FILE* m_pFile;
    OpenMode m_eMode;
    bool m_bGood;

    static inline int32_t ms_iSeekCur = 1;
    static inline int32_t ms_iSeekEnd = 2;
    // whence value of a seek from the file's start (SEEK_SET)
    static inline int32_t ms_iSeekSet;
    static inline FileCreateFunc ms_pfnFileCreateFunc{&NiFile::DefaultFileCreateFunc};
    static FileAccessFunc ms_pfnFileAccessFunc;
    static DirExistsFunc ms_pfnDirExistsFunc;
    static CreateDirFunc ms_pfnCreateDirFunc;
    static inline bool bsUseDiskError;
};

inline NiFile::FileAccessFunc NiFile::ms_pfnFileAccessFunc{&NiFile::DefaultFileAccessFunc};
inline NiFile::CreateDirFunc NiFile::ms_pfnCreateDirFunc{&NiFile::DefaultCreateDirectoryFunc};
inline NiFile::DirExistsFunc NiFile::ms_pfnDirExistsFunc{&NiFile::DefaultDirectoryExistsFunc};
inline BSSemaphore* s_event_sema;
inline BSSemaphore* s_worker_thread_done;
inline BSCriticalSection* s_queue_lock;

namespace OEI {
    // The background file I/O thread: requests queued for its worker thread.
    class IO {
    public:
        // What a request asks the worker thread for. The worker thread knows seek, read, write and quit as 1-4.
        enum IO_Request_Type : int32_t {
            IRT_INVALID = 0,
            IRT_SEEK = 1,
            IRT_READ = 2,
            IRT_WRITE = 3,
            IRT_QUIT = 4,
        };

        // The origin of a seek; the worker thread passes it to the CRT's fseek as SEEK_SET/CUR/END.
        enum Seek_Whence_Type : int32_t {
            SWT_SET = 0,
            SWT_CURRENT = 1,
            SWT_END = 2,
        };

        // One queued request: the file, the operation, the seek origin, the buffer, the offset or byte count, the
        // semaphore released when it is done and its result.
        struct IO_Request {
            std::FILE* m_pFile;
            IO_Request_Type m_eType;
            Seek_Whence_Type m_eWhence;
            void* m_pMemory;
            uint32_t m_uiSize;
            BSSemaphore* m_wait_handle;

            // The CRT call's result: fseek's int or fread's and fwrite's count.
            union {
                int32_t result_int;
                uint32_t result_unsigned_int;
            } result;
        };

        // The worker thread that runs the queued requests. Its destructor is BSThread's.
        class IO_Thread : public BSThread {
        public:
            IO_Thread();

            uint32_t ThreadProc() override;
        };

        // Returns the count read or written; NiFile::FileRead compares fread's result as unsigned.
        static uint32_t fread(void* the_buffer, uint32_t the_block_count, uint32_t the_size, std::FILE* the_file);
        static uint32_t
        fwrite(const void* the_buffer, uint32_t the_block_count, uint32_t the_size, std::FILE* the_file);
        static int32_t fseek(std::FILE* the_file, int32_t the_offset, OEI::IO::Seek_Whence_Type the_whence);
        // Queues the request for the I/O worker thread.
        static void add_work_item_to_queue(OEI::IO::IO_Request& new_request);
        // Queues the request and waits for the worker thread to finish it.
        static void add_work_item_to_queue_and_wait(OEI::IO::IO_Request& new_request);
        // Stops the I/O worker thread and frees its queues.
        static void terminate();
        // Starts the I/O worker thread and its queues.
        static void initialize();
    };
} // namespace OEI

inline BSList<OEI::IO::IO_Request*>* s_work_item_queue;
