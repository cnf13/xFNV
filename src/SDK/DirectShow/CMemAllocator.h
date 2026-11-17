#pragma once

#include "SDK/DirectShow/CUnknown.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
class BSSystemSemaphore;
class CBaseObject;

class CMediaSample;
struct IMemAllocatorNotifyCallbackTemp;

struct ALLOCATOR_PROPERTIES {
	int32_t cBuffers;
	int32_t cbBuffer;
	int32_t cbAlign;
	int32_t cbPrefix;
};
struct IMemAllocator : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetProperties(ALLOCATOR_PROPERTIES* pProps) = 0;
	virtual HRESULT STDMETHODCALLTYPE Commit() = 0;
	virtual HRESULT STDMETHODCALLTYPE Decommit() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(CMediaSample** ppBuffer, int64_t* pStart, int64_t* pEnd,
		uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(CMediaSample* pBuffer) = 0;
};
struct IMemAllocatorCallbackTemp : IMemAllocator {
	virtual HRESULT STDMETHODCALLTYPE SetNotify(IMemAllocatorNotifyCallbackTemp* pNotify) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFreeCount(int32_t* pCount) = 0;
};
// The SDK's allocator release callback.
struct IMemAllocatorNotifyCallbackTemp : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE NotifyRelease() = 0;
};

// SDK critical-section wrapper. RTTI places it at +0x10 in CBaseAllocator.
class CCritSec {
public:
	// The SDK's CCritSec: the hybrid constructors and destructors of its users inline these.
	CCritSec();
	~CCritSec();
	_RTL_CRITICAL_SECTION m_CritSec;
};

// IDA name; not in the PDB. The PDB has no source path; placed with its concrete DirectShow allocator.
class CBaseAllocator : public CUnknown, public IMemAllocatorCallbackTemp, public CCritSec {
public:
	struct CSampleList {
		CMediaSample* m_List;
		int32_t m_nOnList;
	};

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE AddRef() override;
	uint32_t STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
	~CBaseAllocator() override;
	virtual void Free() = 0;
	// The SDK's base Alloc: VFW_E_SIZENOTSET until the count, size and alignment are set,
	// else S_FALSE when nothing changed. CMemAllocator::Alloc calls it directly.
	virtual HRESULT Alloc();
	// Pure: the exe has no CBaseAllocator vtable (the SDK's abstract base, AM_NOVTABLE) and no code for the SDK's
	// base SetProperties; its one concrete allocator, CMemAllocator, overrides it (0x9FB050) without calling it.
	HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual) override = 0;
	HRESULT STDMETHODCALLTYPE GetProperties(ALLOCATOR_PROPERTIES* pProps) override;
	HRESULT STDMETHODCALLTYPE Commit() override;
	HRESULT STDMETHODCALLTYPE Decommit() override;
	HRESULT STDMETHODCALLTYPE GetBuffer(CMediaSample** ppBuffer, int64_t* pStart, int64_t* pEnd,
		uint32_t dwFlags) override;
	HRESULT STDMETHODCALLTYPE ReleaseBuffer(CMediaSample* pBuffer) override;
	HRESULT STDMETHODCALLTYPE SetNotify(IMemAllocatorNotifyCallbackTemp* pNotify) override;
	HRESULT STDMETHODCALLTYPE GetFreeCount(int32_t* pCount) override;
	CBaseAllocator(const char* pName, IUnknown* pUnk, HRESULT* phr, int32_t bEvent, int32_t fEnableReleaseCallback);

	CSampleList m_lFree;
	// HANDLE m_hSem in the SDK; the exe's semaphore handle, here the standard-C++ object in its slot.
	BSSystemSemaphore* m_hSem;
	int32_t m_lWaiting;
	int32_t m_lCount;
	int32_t m_lAllocated;
	int32_t m_lSize;
	int32_t m_lAlignment;
	int32_t m_lPrefix;
	int32_t m_bChanged;
	int32_t m_bCommitted;
	int32_t m_bDecommitInProgress;
	IMemAllocatorNotifyCallbackTemp* m_pNotify;
	int32_t m_fEnableReleaseCallback;
public:
	// Not in the PDB; the DirectShow base classes' name. NotifySample (thiscall, ret 0,
	// 0x9FAE70): when m_lWaiting (+0x34) is set, ReleaseSemaphore(m_hSem (+0x30), m_lWaiting, 0) and m_lWaiting
	// cleared. Decommit (0x9FAF50) and ReleaseBuffer (0x9FC480) call it on the allocator; neither reads eax after.
	void NotifySample();
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// The constructor clears m_pBuffer at +0x60 after constructing CBaseAllocator.
class CMemAllocator : public CBaseAllocator {
public:
	~CMemAllocator() override;
	HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual) override;
	void Free() override {}
	HRESULT Alloc() override;
	uint8_t* m_pBuffer;
public:
	// The DirectShow base classes' name (amfilter.cpp); not in the PDB or IDA. Deletes every free sample, clears
	// m_lAllocated and releases m_pBuffer (thiscall, ret 0, 0x9FB170). Its callers, the destructor and Alloc, do not
	// read eax after it.
	void ReallyFree();
	CMemAllocator(const char* pName, IUnknown* pUnk, HRESULT* phr);
	static void operator delete(void* apMemory);
};
