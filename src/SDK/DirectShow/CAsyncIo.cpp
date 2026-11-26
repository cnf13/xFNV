#include "SDK/DirectShow/CAsyncIo.h"
#include "BSCore/BSSystemObjects.h"
#include "BSCore/NewOverloads.h"
#include "BSMovie/CAsyncStream.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/DirectShow/CAMEvent.h"
#include "SDK/DirectShow/CMediaSample.h"
#include "SDK/DirectShow/CRequestList.h"

// The DirectShow async reader's bodies (the SDK sample's asyncio.cpp and the base classes' wxlist.cpp). Neither the
// PDB nor IDA names them; the names are the SDK's, chosen here. The exe's CAutoLock EH frames (cleanup-only) are left
// out: the lock and unlock calls stay where the exe makes them.

HRESULT CAsyncIo::SyncReadAligned(int64_t llPos, int32_t lLength, uint8_t* pBuffer, int32_t* pcbActual,
	void* pvContext)
{
	if (!pcbActual)
		// E_POINTER
		return static_cast<HRESULT>(0x80004003);
	// EXE-INLINED none: CAsyncIo::IsAligned (each test reads the alignment again)
	if ((static_cast<uint32_t>(llPos) & (this->m_pStream->Alignment() - 1)) != 0
		|| (static_cast<uint32_t>(lLength) & (this->m_pStream->Alignment() - 1)) != 0
		|| (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pBuffer)) & (this->m_pStream->Alignment() - 1)) != 0)
		// VFW_E_BADALIGN
		return static_cast<HRESULT>(0x8004020E);
	CAsyncRequest request;
	HRESULT hr = request.Request(this, this->m_pStream, llPos, lLength, 1, pBuffer, pvContext, 0);
	if (hr >= 0) {
		hr = request.Complete();
		// EXE-INLINED none: CAsyncRequest::GetActualLength
		*pcbActual = request.m_lLength;
	}
	return hr;
}

HRESULT CAsyncIo::SyncRead(int64_t llPos, int32_t lLength, uint8_t* pBuffer)
{
	// EXE-INLINED none: CAsyncIo::IsAligned (each test reads the alignment again)
	if ((static_cast<uint32_t>(llPos) & (this->m_pStream->Alignment() - 1)) == 0
		&& (static_cast<uint32_t>(lLength) & (this->m_pStream->Alignment() - 1)) == 0
		&& (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pBuffer)) & (this->m_pStream->Alignment() - 1)) == 0) {
		// The exe passes pBuffer's own argument slot (dead once pBuffer is in a register) as the unused length
		// out-parameter; src uses a local, so the length lands in this frame instead of the caller's argument area.
		int32_t cbUnused;
		return this->SyncReadAligned(llPos, lLength, pBuffer, &cbUnused, nullptr);
	}
	CAsyncRequest request;
	HRESULT hr = request.Request(this, this->m_pStream, llPos, lLength, 0, pBuffer, nullptr, 0);
	if (hr < 0)
		return hr;
	return request.Complete();
}

CAsyncRequest* CAsyncIo::GetWorkItem()
{
	EnterCriticalSection(&this->m_csLists);
	CAsyncRequest* pRequest = static_cast<CAsyncRequest*>(this->m_listWork.RemoveHeadI());
	// EXE-INLINED none: CBaseList::GetCount
	if (this->m_listWork.m_Count == 0)
		this->m_evWork.m_event->Reset();
	LeaveCriticalSection(&this->m_csLists);
	return pRequest;
}

CAsyncRequest* CAsyncIo::GetDoneItem()
{
	EnterCriticalSection(&this->m_csLists);
	CAsyncRequest* pRequest = static_cast<CAsyncRequest*>(this->m_listDone.RemoveHeadI());
	// EXE-INLINED none: CBaseList::GetCount
	if (this->m_listDone.m_Count == 0 && (!this->m_bFlushing || this->m_bWaiting))
		this->m_evDone.m_event->Reset();
	LeaveCriticalSection(&this->m_csLists);
	return pRequest;
}

void CAsyncIo::ProcessRequests()
{
	EnterCriticalSection(&this->m_csLists);
	CAsyncRequest* pRequest = this->GetWorkItem();
	while (pRequest) {
		++this->m_cItemsOut;
		LeaveCriticalSection(&this->m_csLists);
		pRequest->Complete();
		EnterCriticalSection(&this->m_csLists);
		this->PutDoneItem(pRequest);
		if (--this->m_cItemsOut == 0 && this->m_bWaiting)
			this->m_evAllDone.m_event->Set();
		LeaveCriticalSection(&this->m_csLists);
		EnterCriticalSection(&this->m_csLists);
		pRequest = this->GetWorkItem();
	}
	LeaveCriticalSection(&this->m_csLists);
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
int32_t ThreadProc(CAsyncIo* apThis, [[maybe_unused]] void* apEdx) {
	// WaitForMultipleObjects on the two events, stop first: 1 is the work event.
	while (BSSystemEvent::WaitAny(*apThis->m_evStop.m_event, *apThis->m_evWork.m_event) == 1) {
		apThis->ProcessRequests();
	}
	return 0;
}

uint32_t InitialThreadProc(void* lpThreadParameter) {
	return static_cast<uint32_t>(::ThreadProc(static_cast<CAsyncIo*>(lpThreadParameter), nullptr));
}

HRESULT CAsyncIo::BeginFlush()
{
	EnterCriticalSection(&this->m_csLists);
	this->m_bFlushing = 1;
	CAsyncRequest* pRequest;
	while ((pRequest = this->GetWorkItem()) != nullptr)
		this->PutDoneItem(pRequest);
	if (this->m_cItemsOut > 0) {
		this->m_bWaiting = 1;
		LeaveCriticalSection(&this->m_csLists);
		this->m_evAllDone.m_event->Wait();
		EnterCriticalSection(&this->m_csLists);
		while (this->m_cItemsOut != 0) {
			LeaveCriticalSection(&this->m_csLists);
			this->m_evAllDone.m_event->Wait();
			EnterCriticalSection(&this->m_csLists);
		}
		this->m_bWaiting = 0;
		this->m_evDone.m_event->Set();
		LeaveCriticalSection(&this->m_csLists);
	} else {
		this->m_evDone.m_event->Set();
		LeaveCriticalSection(&this->m_csLists);
	}
	return 0;
}

HRESULT CAsyncIo::PutWorkItem(CAsyncRequest* pRequest)
{
	HRESULT hr;
	EnterCriticalSection(&this->m_csLists);
	if (this->m_bFlushing) {
		// VFW_E_WRONG_STATE
		hr = static_cast<HRESULT>(0x80040227);
	} else if (this->m_listWork.AddTailI(pRequest)) {
		this->m_evWork.m_event->Set();
		hr = this->StartThread();
	} else {
		// E_OUTOFMEMORY
		hr = static_cast<HRESULT>(0x8007000E);
	}
	LeaveCriticalSection(&this->m_csLists);
	return hr;
}

CAsyncIo::CAsyncIo(CAsyncStream* a1)
	: m_pStream(a1), m_bFlushing(0), m_listWork(nullptr), m_listDone(nullptr), m_evWork(1), m_evDone(1), m_cItemsOut(0),
	  m_bWaiting(0), m_evAllDone(0), m_evStop(1), m_evStop_(0) {
	InitializeCriticalSection(&m_csReader);
	InitializeCriticalSection(&m_csLists);
}

int32_t CAsyncIo::EndFlushIo() {
	_RTL_CRITICAL_SECTION* p_m_csLists = &m_csLists;
	EnterCriticalSection(&m_csLists);
	bool v3 = m_listDone.m_Count <= static_cast<int>(0u);
	m_bFlushing = 0;
	if (v3) {
		m_evDone.m_event->Reset();
	} else {
		m_evDone.m_event->Set();
	}
	LeaveCriticalSection(p_m_csLists);
	return 0;
}

int32_t CAsyncIo::WaitForNextRequest(uint32_t dwMilliseconds, void** a2, uintptr_t* a3, int32_t* a4) {
	CAsyncRequest* DoneItem;
	CAsyncRequest* v8;
	if (!a2 || (!a3 || !a4)) {
		return E_POINTER;
	}
	*a2 = 0;
	if (m_evDone.m_event->Wait(dwMilliseconds)) {
		return VFW_E_TIMEOUT;
	}
	while (1) {
		DoneItem = GetDoneItem();
		v8 = DoneItem;
		if (DoneItem) {
			break;
		}
		EnterCriticalSection(&m_csLists);
		if (m_bFlushing && !m_bWaiting) {
			LeaveCriticalSection(&m_csLists);
			return VFW_E_WRONG_STATE;
		}
		LeaveCriticalSection(&m_csLists);
		if (m_evDone.m_event->Wait(dwMilliseconds)) {
			return VFW_E_TIMEOUT;
		}
	}
	int32_t m_hr = DoneItem->m_hr;
	if (m_hr == 1) {
		int32_t m_lLength = DoneItem->m_lLength;
		uint32_t m_llPos = DoneItem->m_llPos;
		uint32_t m_llPos_high = static_cast<uint32_t>(DoneItem->m_llPos >> 32);
		if (static_cast<uint64_t>((static_cast<uint64_t>(static_cast<uint32_t>(m_llPos_high)) << 32) | static_cast<uint32_t>(m_llPos)) + m_lLength == m_pStream->GetFileSize(0)) {
			m_hr = 0;
		} else {
			m_hr = E_FAIL;
		}
	}
	*a4 = v8->m_lLength;
	*a2 = v8->m_pContext;
	*a3 = v8->m_dwUser;
	ourFree(v8);
	return m_hr;
}

int32_t CAsyncIo::IsAligned(int32_t l) {
	return (l & m_pStream->Alignment() - 1) == 0;
}

HRESULT CAsyncRequest::Request(CAsyncIo* apIo, CAsyncStream* apStream, int64_t allPos, int32_t alLength, int32_t abAligned, uint8_t* apBuffer, void* apContext, uintptr_t adwUser) {
	m_pIo = apIo;
	m_llPos = static_cast<int64_t>((static_cast<uint64_t>(m_llPos) & 0xFFFFFFFF00000000ull) | (static_cast<uint64_t>(static_cast<uint32_t>(allPos))));
	m_pStream = apStream;
	m_lLength = alLength;
	m_llPos = static_cast<int64_t>((static_cast<uint64_t>(m_llPos) & 0xFFFFFFFFull) | (static_cast<uint64_t>(static_cast<uint32_t>(static_cast<uint32_t>(allPos >> 32))) << 32));
	m_pBuffer = apBuffer;
	m_bAligned = abAligned;
	m_dwUser = adwUser;
	m_pContext = apContext;
	m_hr = VFW_E_TIMEOUT;
	return 0;
}

// KEEP: an uninitialised read. CBSAStream::Read (0x82F3C0) returns S_FALSE without writing the byte count when its
// file is missing or bad; the success branch then compares dwActual, which the exe leaves as the ecx it pushed to
// make the slot, and may store it into m_lLength.
HRESULT CAsyncRequest::Complete() {
	this->m_pStream->Lock();
	m_hr = this->m_pStream->Seek(m_llPos);
	// S_OK
	if (m_hr == 0) {
		uint32_t dwActual;
		m_hr = this->m_pStream->Read(m_pBuffer, static_cast<uint32_t>(m_lLength), m_bAligned, &dwActual);
		// OLE_S_FIRST: the read went back to the start, so the sample is a discontinuity.
		if (m_hr == 0x40000) {
			if (m_pContext) {
				// The SDK's cast: the context of a queued request is its IMediaSample.
				static_cast<IMediaSample*>(m_pContext)->SetDiscontinuity(1);
				m_hr = 0;
			}
		}
		if (m_hr >= 0) {
			if (dwActual != static_cast<uint32_t>(m_lLength)) {
				m_lLength = static_cast<int32_t>(dwActual);
				// S_FALSE
				m_hr = 1;
			} else {
				m_hr = 0;
			}
		}
	}
	this->m_pStream->Unlock();
	return m_hr;
}

HRESULT CAsyncIo::Length(int64_t* pTotal, int64_t* pAvailable) {
	if (!pTotal) {
		// E_POINTER
		return static_cast<HRESULT>(0x80004003);
	}
	*pTotal = m_pStream->GetFileSize(pAvailable);
	return 0;
}

HRESULT CAsyncIo::Alignment(int32_t* pAlignment) {
	if (!pAlignment) {
		return E_POINTER;
	}
	*pAlignment = m_pStream->Alignment();
	return 0;
}

HRESULT CAsyncIo::PutDoneItem(CAsyncRequest* pRequest) {
	if (!m_listDone.AddTailI(pRequest)) {
		return E_OUTOFMEMORY;
	}
	m_evDone.m_event->Set();
	return 0;
}

// CreateThread's failure path (HRESULT_FROM_WIN32(GetLastError())) has no standard form: std::thread throws
// instead of returning no thread.
HRESULT CAsyncIo::StartThread() {
	if (m_evStop_)
		return 0;
	m_evStop.m_event->Reset();
	m_evStop_ = new BSSystemThread(&InitialThreadProc, this, false);
	return 0;
}

HRESULT CAsyncIo::Request(int64_t llPos, int32_t lLength, int32_t bAligned, uint8_t* pBuffer, void* pContext, uintptr_t dwUser) {
	// The DirectShow source tests the buffer's address against the stream's alignment.
	if (bAligned && ((m_pStream->Alignment() - 1 & static_cast<uint32_t>(llPos)) != 0 || ((m_pStream->Alignment() - 1 & lLength) != 0 || (m_pStream->Alignment() - 1 & reinterpret_cast<uintptr_t>(pBuffer)) != 0))) {
		return VFW_E_BADALIGN;
	}
	CAsyncRequest* v9 = static_cast<CAsyncRequest*>(ourMalloc(sizeof(CAsyncRequest)));
	if (!v9) {
		return E_OUTOFMEMORY;
	}
	int32_t v10 = v9->Request(this, m_pStream, llPos, lLength, bAligned, pBuffer, pContext, dwUser);
	if (v10 < 0 || (v10 = PutWorkItem(v9), v10 < 0)) {
		ourFree(v9);
	}
	return v10;
}

CAsyncIo::~CAsyncIo() {
	BeginFlush();
	CloseThread();
	// The SDK frees every completed request; CAsyncRequest has no destructor, so its delete is the free alone.
	CRequestList::CNode* pPos = m_listDone.m_pFirst;
	while (pPos) {
		ourFree(m_listDone.GetNextI(pPos));
	}
	m_listDone.RemoveAll();
	DeleteCriticalSection(&m_csLists);
	DeleteCriticalSection(&m_csReader);
}

int32_t CAsyncIo::CloseThread() {
	this->m_evStop.m_event->Set();
	BSSystemThread* pThread = m_evStop_;
	if (pThread) {
		pThread->Wait();
		delete m_evStop_;
		m_evStop_ = 0;
	}
	return 0;
}
