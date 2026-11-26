#pragma once

#include <stdint.h>

#include "BSMovie/CAsyncStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/DirectShow/CAMEvent.h"
#include "SDK/DirectShow/CRequestList.h"
#include "SDK/DirectShow/CUnknown.h"

class BSSystemThread;
class CAsyncIo;

// One read request.
class CAsyncRequest {
public:
    // Fills the request; m_hr starts as VFW_E_TIMEOUT.
    // Inferred type: return: HRESULT: signed uses from 0x9fe186
    HRESULT Request(
        CAsyncIo* apIo,
        CAsyncStream* apStream,
        int64_t allPos,
        int32_t alLength,
        int32_t abAligned,
        uint8_t* apBuffer,
        void* apContext,
        uintptr_t adwUser
    );
    // Runs the read.
    HRESULT Complete();

    CAsyncIo* m_pIo;
    CAsyncStream* m_pStream;
    int64_t m_llPos;
    int32_t m_bAligned;
    int32_t m_lLength;
    uint8_t* m_pBuffer;
    void* m_pContext;
    uintptr_t m_dwUser;
    HRESULT m_hr;
};

// DirectShow's asynchronous request queues and worker synchronization.
// The PDB has no source path; its location was chosen with the DirectShow base classes.
class CAsyncIo {
public:
    // Reads at an aligned position and length into an
    // aligned buffer.
    HRESULT SyncReadAligned(int64_t llPos, int32_t lLength, uint8_t* pBuffer, int32_t* pcbActual, void* pvContext);
    // Reads, aligned or not.
    HRESULT SyncRead(int64_t llPos, int32_t lLength, uint8_t* pBuffer);
    // Takes the next queued request, or null.
    CAsyncRequest* GetWorkItem();
    // Takes the next completed request, or null.
    CAsyncRequest* GetDoneItem();
    // Queues a completed request and signals m_evDone.
    HRESULT PutDoneItem(CAsyncRequest* pRequest);
    // The worker thread's loop: completes every queued
    // request.
    void ProcessRequests();
    // Moves the queued requests to the done list and
    // waits for the ones in progress.
    HRESULT BeginFlush();
    // Starts the worker thread when it is not running.
    HRESULT StartThread();
    // Queues a request and wakes the worker.
    HRESULT PutWorkItem(CAsyncRequest* pRequest);
    // Writes the stream's alignment to pAlignment;
    // E_POINTER without one.
    HRESULT Alignment(int32_t* pAlignment);
    // Whether l is a multiple of the stream's alignment.
    int32_t IsAligned(int32_t l);
    // The stream's length into pTotal and, through the
    // stream's Size, the bytes available from its current position into pAvailable; E_POINTER without pTotal.
    HRESULT Length(int64_t* pTotal, int64_t* pAvailable);
    // Queues a read of lLength bytes at llPos into pBuffer;
    // pContext is the requesting sample and dwUser its user data.
    HRESULT
    Request(int64_t llPos, int32_t lLength, int32_t bAligned, uint8_t* pBuffer, void* pContext, uintptr_t dwUser);

    _RTL_CRITICAL_SECTION m_csReader;
    CAsyncStream* m_pStream;
    // CRITICAL_SECTION aliases RTL_CRITICAL_SECTION in the PC SDK.
    _RTL_CRITICAL_SECTION m_csLists;
    int32_t m_bFlushing;
    CRequestList m_listWork;
    CRequestList m_listDone;
    CAMEvent m_evWork;
    CAMEvent m_evDone;
    // LONG is 32-bit in the PC SDK.
    int32_t m_cItemsOut;
    int32_t m_bWaiting;
    CAMEvent m_evAllDone;
    CAMEvent m_evStop;
    // IDA name; not in the PDB. HANDLE m_hThread in the SDK; the exe's worker thread handle, here the standard-C++
    // thread in its slot.
    BSSystemThread* m_evStop_;

public:
    CAsyncIo(CAsyncStream* a1);
    ~CAsyncIo();
    int32_t EndFlushIo();
    // The DirectShow base classes' CAsyncIo::WaitForNext: a3 is the
    // SDK's DWORD_PTR* pUser, which receives the request's m_dwUser (uintptr_t).
    // Inferred type: return: int32_t: signed uses from 0x9fdb32
    int32_t WaitForNextRequest(uint32_t dwMilliseconds, void** a2, uintptr_t* a3, int32_t* a4);

public:
public:
    int32_t CloseThread();
};

// The worker thread's start
// routine: runs ThreadProc on the CAsyncIo it is given.
uint32_t InitialThreadProc(void* lpThreadParameter);

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
int32_t ThreadProc(CAsyncIo* apThis, [[maybe_unused]] void* apEdx);
