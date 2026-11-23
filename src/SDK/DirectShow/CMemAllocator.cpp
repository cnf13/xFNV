#include "SDK/DirectShow/CMemAllocator.h"
#include "BSCore/MemoryManager.h"
#include "BSMovie/CBaseObject.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/DirectShow/CMediaSample.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include "SDK/DirectShow/wxutil.h"
#include "BSCore/BSSystemObjects.h"
#include "platform/Memory.h"
#include <stdint.h>

class CBaseObject;
class CUnknown;
struct IUnknown;

CCritSec::CCritSec() {
	InitializeCriticalSection(&m_CritSec);
}

CCritSec::~CCritSec() {
	DeleteCriticalSection(&m_CritSec);
}

// The SDK's ~CBaseAllocator: closes the semaphore and releases the notify callback.
CBaseAllocator::~CBaseAllocator() {
	delete m_hSem;
	if (m_pNotify) {
		m_pNotify->Release();
	}
}

// The SDK's CBaseAllocator::Alloc: VFW_E_SIZENOTSET until the count, size and alignment are set; S_FALSE when
// nothing changed since the last allocation, else S_OK.
HRESULT CBaseAllocator::Alloc() {
	if (m_lCount <= 0 || m_lSize <= 0 || m_lAlignment <= 0) {
		return VFW_E_SIZENOTSET;
	}
	if (m_bChanged == 0) {
		return S_FALSE;
	}
	return S_OK;
}

// The SDK's DECLARE_IUNKNOWN on CBaseAllocator, in the IMemAllocatorCallbackTemp vtable (+0xC): delegates to the owner
// (m_pUnknown, this-8 from that subobject). The exe swaps the owner into the this slot and tail-jumps to its stdcall
// QueryInterface (slot 0).
HRESULT STDMETHODCALLTYPE CBaseAllocator::QueryInterface(REFIID riid, void** ppv)
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->QueryInterface(riid, ppv);
}

// DECLARE_IUNKNOWN's AddRef: the owner's AddRef (slot 1, tail-jump).
uint32_t STDMETHODCALLTYPE CBaseAllocator::AddRef()
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->AddRef();
}

// DECLARE_IUNKNOWN's Release: the owner's Release (slot 2, tail-jump).
uint32_t STDMETHODCALLTYPE CBaseAllocator::Release()
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->Release();
}

// The SDK's CBaseAllocator::NonDelegatingQueryInterface (CMemAllocator's primary table, slot 0): GetInterface on the
// IMemAllocatorCallbackTemp subobject (null when this is) for IID_IMemAllocator, and for
// IID_IMemAllocatorCallbackTemp when m_fEnableReleaseCallback is set; CUnknown's otherwise.
HRESULT STDMETHODCALLTYPE CBaseAllocator::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
	if (!IsEqualGUID(riid, CLSID_IMemAllocator)
		&& (!IsEqualGUID(riid, CLSID_IMemAllocatorCallbackTemp) || !m_fEnableReleaseCallback)) {
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
	return GetInterface(static_cast<IMemAllocatorCallbackTemp*>(this), ppv);
}

// The IMemAllocatorCallbackTemp methods below are the SDK's CBaseAllocator ones; the exe's entries (CMemAllocator's
// table at +0xC) take that subobject as this.

HRESULT STDMETHODCALLTYPE CBaseAllocator::GetProperties(ALLOCATOR_PROPERTIES* pProps) {
	if (!pProps) {
		return E_POINTER;
	}
	CAutoLock lck(this);
	pProps->cbBuffer = m_lSize;
	pProps->cBuffers = m_lCount;
	pProps->cbAlign = m_lAlignment;
	pProps->cbPrefix = m_lPrefix;
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseAllocator::GetBuffer(CMediaSample** ppBuffer, int64_t* pStart, int64_t* pEnd,
	uint32_t dwFlags) {
	*ppBuffer = 0;
	while (1) {
		CMediaSample* m_List;
		{
			CAutoLock lck(this);
			if (!m_bCommitted) {
				return VFW_E_NOT_COMMITTED;
			}
			m_List = m_lFree.m_List;
			if (m_List) {
				CMediaSample* m_pNext = m_List->m_pNext;
				--m_lFree.m_nOnList;
				m_lFree.m_List = m_pNext;
			} else {
				++m_lWaiting;
			}
		}
		if (m_List) {
			m_List->m_cRef = 1;
			*ppBuffer = m_List;
			return 0;
		}
		if ((dwFlags & 4) != 0) {
			break;
		}
		m_hSem->Wait();
	}
	return VFW_E_TIMEOUT;
}

HRESULT STDMETHODCALLTYPE CBaseAllocator::SetNotify(IMemAllocatorNotifyCallbackTemp* pNotify) {
	CAutoLock lck(this);
	if (pNotify) {
		pNotify->AddRef();
	}
	IMemAllocatorNotifyCallbackTemp* m_pNotify = this->m_pNotify;
	if (m_pNotify) {
		this->m_pNotify->Release();
	}
	this->m_pNotify = pNotify;
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseAllocator::GetFreeCount(int32_t* pCount) {
	CAutoLock lck(this);
	*pCount = m_lFree.m_nOnList + (m_lCount - m_lAllocated);
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseAllocator::Commit() {
	CAutoLock lck(this);
	if (!m_bCommitted) {
		bool v2 = m_bDecommitInProgress == 0;
		m_bCommitted = 1;
		if (v2) {
			int32_t v3 = Alloc();
			if (v3 < 0) {
				m_bCommitted = 0;
				return v3;
			}
			static_cast<IMemAllocatorCallbackTemp*>(this)->AddRef();
		} else {
			m_bDecommitInProgress = 0;
		}
	}
	return 0;
}

// Free is CBaseAllocator's vtable slot 4 (+0x10), a virtual call.
HRESULT STDMETHODCALLTYPE CBaseAllocator::Decommit() {
	int32_t v1 = 0;
	{
		CAutoLock lck(this);
		if (!m_bCommitted && !m_bDecommitInProgress) {
			return 0;
		}
		bool v3 = m_lFree.m_nOnList < m_lAllocated;
		m_bCommitted = 0;
		if (v3) {
			m_bDecommitInProgress = 1;
		} else {
			m_bDecommitInProgress = 0;
			Free();
			v1 = 1;
		}
		NotifySample();
	}
	if (v1) {
		static_cast<IMemAllocatorCallbackTemp*>(this)->Release();
	}
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseAllocator::ReleaseBuffer(CMediaSample* pBuffer) {
	if (!pBuffer) {
		return E_POINTER;
	}
	int32_t v4 = 1;
	{
		CAutoLock lck(this);
		pBuffer->m_pNext = m_lFree.m_List;
		m_lFree.m_List = pBuffer;
		++m_lFree.m_nOnList;
		if (m_lWaiting) {
			NotifySample();
		}
		if (m_bDecommitInProgress && m_lFree.m_nOnList == m_lAllocated) {
			Free();
			m_bDecommitInProgress = 0;
		} else {
			v4 = 0;
		}
	}
	IMemAllocatorNotifyCallbackTemp* m_pNotify = this->m_pNotify;
	if (m_pNotify) {
		this->m_pNotify->NotifyRelease();
	}
	if (v4) {
		static_cast<IMemAllocatorCallbackTemp*>(this)->Release();
	}
	return 0;
}

void CBaseAllocator::NotifySample() {
	int32_t m_lWaiting = this->m_lWaiting;
	if (m_lWaiting) {
		m_hSem->Release(m_lWaiting);
		this->m_lWaiting = 0;
	}
}

HRESULT STDMETHODCALLTYPE CMemAllocator::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual) {
	if (!pActual) {
		return E_POINTER;
	}
	CAutoLock lck(this);
	pActual->cBuffers = 0;
	pActual->cbBuffer = 0;
	pActual->cbAlign = 0;
	pActual->cbPrefix = 0;
	int32_t cbAlign = pRequest->cbAlign;
	// GetSystemInfo's dwAllocationGranularity.
	if (!cbAlign || (cbAlign - 1 & static_cast<int32_t>(platform::AllocationGranularity())) != 0) {
		return VFW_E_BADALIGN;
	} else {
		if (m_bCommitted == 1) {
			return VFW_E_ALREADY_COMMITTED;
		} else {
			if (m_lFree.m_nOnList >= m_lAllocated) {
				int32_t v7 = pRequest->cbPrefix + pRequest->cbBuffer;
				if (v7 % cbAlign) {
					v7 += cbAlign - v7 % cbAlign;
				}
				int32_t v8 = v7 - pRequest->cbPrefix;
				m_lSize = v8;
				pActual->cbBuffer = v8;
				int32_t cBuffers = pRequest->cBuffers;
				m_lCount = pRequest->cBuffers;
				pActual->cBuffers = cBuffers;
				int32_t v10 = pRequest->cbAlign;
				m_lAlignment = v10;
				pActual->cbAlign = v10;
				int32_t cbPrefix = pRequest->cbPrefix;
				m_lPrefix = cbPrefix;
				pActual->cbPrefix = cbPrefix;
				m_bChanged = 1;
				return 0;
			} else {
				return VFW_E_BUFFERS_OUTSTANDING;
			}
		}
	}
}

// The DirectShow base classes' CMemAllocator::ReallyFree (amfilter.cpp). Each sample is deleted through its
// deleting destructor (vtable slot 0x54, flag 1).
void CMemAllocator::ReallyFree() {
	while (1) {
		CMediaSample* pSample = m_lFree.m_List;
		if (!pSample) {
			break;
		}
		CMediaSample* pNext = pSample->m_pNext;
		--m_lFree.m_nOnList;
		m_lFree.m_List = pNext;
		delete pSample;
	}
	m_lAllocated = 0;
	if (m_pBuffer) {
		// VirtualFree(MEM_RELEASE).
		platform::Release(m_pBuffer);
		m_pBuffer = nullptr;
	}
}

CMemAllocator::~CMemAllocator() {
	CBaseAllocator::Decommit();
	ReallyFree();
}

void CMemAllocator::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The DirectShow base classes' CBaseAllocator constructor (amfilter.cpp).
CBaseAllocator::CBaseAllocator(const char* pName, IUnknown* pUnk, HRESULT* phr, int32_t bEvent,
	int32_t fEnableReleaseCallback)
	: CUnknown(pName, pUnk), CCritSec(), m_lFree{nullptr, 0}, m_hSem(nullptr), m_lWaiting(0), m_lCount(0),
	  m_lAllocated(0), m_lSize(0), m_lAlignment(0), m_lPrefix(0), m_bChanged(0), m_bCommitted(0),
	  m_bDecommitInProgress(0), m_pNotify(nullptr), m_fEnableReleaseCallback(fEnableReleaseCallback) {
	if (!bEvent) {
		return;
	}
	// CreateSemaphoreA(nullptr, 0, 0x7FFFFFFF, nullptr); its failure (E_OUTOFMEMORY) has no standard form: the
	// allocation throws instead.
	m_hSem = new BSSystemSemaphore(0, 0x7FFFFFFF);
}

CMemAllocator::CMemAllocator(const char* pName, IUnknown* pUnk, HRESULT* phr)
	: CBaseAllocator(pName, pUnk, phr, 1, 1), m_pBuffer(nullptr) {}

// The DirectShow base classes' CMemAllocator::Alloc (amfilter.cpp).
// The exe passes the CAutoLock's slot (it holds the lock pointer) as phr: MSVC shares the slot with the SDK's hr,
// whose value the exe keeps in a register. The CMediaSample constructor never touches *phr; src passes the SDK's hr.
HRESULT CMemAllocator::Alloc() {
	CAutoLock lck(this);
	HRESULT hr = CBaseAllocator::Alloc();
	if (hr < 0) {
		return hr;
	}
	if (hr != S_FALSE) {
		if (m_pBuffer) {
			ReallyFree();
		}
		int32_t lAlignedSize = m_lSize + m_lPrefix;
		if (m_lAlignment > 1) {
			int32_t lRemainder = lAlignedSize % m_lAlignment;
			if (lRemainder != 0) {
				lAlignedSize += m_lAlignment - lRemainder;
			}
		}
		// VirtualAlloc(MEM_COMMIT, PAGE_READWRITE): reserved and committed pages.
		const size_t uiBufferSize = static_cast<uint32_t>(m_lCount * lAlignedSize);
		m_pBuffer = static_cast<uint8_t*>(platform::Reserve(uiBufferSize));
		if (m_pBuffer && !platform::Commit(m_pBuffer, uiBufferSize)) {
			platform::Release(m_pBuffer);
			m_pBuffer = nullptr;
		}
		if (!m_pBuffer) {
			return E_OUTOFMEMORY;
		}
		for (uint8_t* pNext = m_pBuffer; m_lAllocated < m_lCount; pNext += lAlignedSize) {
			CMediaSample* pMemory = static_cast<CMediaSample*>(ourMalloc(sizeof(CMediaSample)));
			if (!pMemory) {
				return E_OUTOFMEMORY;
			}
			CMediaSample* pSample = ::new (pMemory) CMediaSample(nullptr, this, &hr, &pNext[m_lPrefix], m_lSize);
			if (!pSample) {
				return E_OUTOFMEMORY;
			}
			pSample->m_pNext = m_lFree.m_List;
			m_lFree.m_List = pSample;
			++m_lFree.m_nOnList;
			++m_lAllocated;
		}
		m_bChanged = 0;
	}
	return 0;
}
