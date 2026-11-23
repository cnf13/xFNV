#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "SDK/DirectShow/CAsyncIo.h"
#include "SDK/DirectShow/CAsyncReader.h"
#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CEnumPins.h"
#include "SDK/DirectShow/CMediaSample.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include "SDK/DirectShow/wxutil.h"
#include "SDK/MSVC/stdlib.h"
#include <stdint.h>

GUID CLSID_IQualityControl{0x56A868A5, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IPin{0x56A86891, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};

struct AM_MEDIA_TYPE;
class CUnknown;
struct IUnknown;
class CBasePin;
class CMediaType;

// The SDK's CBasePin::NonDelegatingAddRef: the pin's references are the filter's (the SDK's debug-only count is gone).
// The exe swaps the filter's IBaseFilter (+0xC) into the this slot and tail-jumps to its AddRef (slot 1, stdcall).
uint32_t STDMETHODCALLTYPE CBasePin::NonDelegatingAddRef()
{
	return m_pFilter->AddRef();
}

// The SDK's CBasePin::NonDelegatingRelease: releases a reference on the filter (tail-jump to IBaseFilter slot 2).
uint32_t STDMETHODCALLTYPE CBasePin::NonDelegatingRelease()
{
	return m_pFilter->Release();
}

// The SDK's CBasePin::ReceiveConnection: under the pin's lock, accepts pConnector when the pin is free and the filter
// stopped (or reconnecting is allowed), checks the connection and the media type, then completes the connection;
// every failure after the AddRef releases the connected pin again. The SDK's CAutoLock destructor is written out as
// the exe inlines it, at each return. The exe's cleanup-only EH frame is left out.
HRESULT STDMETHODCALLTYPE CBasePin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	if (!pConnector || !pmt) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	_RTL_CRITICAL_SECTION* pLock = m_pLock;
	EnterCriticalSection(pLock);
	if (m_Connected) {
		LeaveCriticalSection(pLock);
		// VFW_E_ALREADY_CONNECTED
		return (HRESULT)0x80040204;
	}
	// EXE-INLINED none: CBasePin::IsStopped (the filter's m_State is State_Stopped, 0)
	if (m_pFilter->m_State != 0 && !m_bCanReconnectWhenActive) {
		LeaveCriticalSection(pLock);
		// VFW_E_NOT_STOPPED
		return (HRESULT)0x80040224;
	}
	HRESULT hr = CheckConnect(pConnector);
	if (hr < 0) {
		BreakConnect();
		LeaveCriticalSection(pLock);
		return hr;
	}
	hr = CheckMediaType(static_cast<const CMediaType*>(pmt));
	if (hr != 0) {
		BreakConnect();
		// E_FAIL and E_INVALIDARG, like any success other than S_OK, become VFW_E_TYPE_NOT_ACCEPTED.
		if (hr >= 0 || hr == (HRESULT)0x80004005 || hr == (HRESULT)0x80070057) {
			hr = (HRESULT)0x8004022A;
		}
		LeaveCriticalSection(pLock);
		return hr;
	}
	m_Connected = pConnector;
	// The exe adds the reference through pConnector, not through a reload of m_Connected.
	pConnector->AddRef();
	hr = SetMediaType(static_cast<const CMediaType*>(pmt));
	if (hr >= 0) {
		hr = CompleteConnect(pConnector);
		if (hr >= 0) {
			LeaveCriticalSection(pLock);
			return 0;
		}
	}
	m_Connected->Release();
	m_Connected = nullptr;
	BreakConnect();
	LeaveCriticalSection(pLock);
	return hr;
}

// The SDK's CBasePin::Connect: under the pin's lock, refuses a second connection and a running filter that may not
// reconnect, then agrees a media type with pReceivePin (AgreeMediaType, 0x9FC0B0), breaking the connection when that
// fails. The SDK's CAutoLock destructor is written out as the exe inlines it, at each return. The exe's cleanup-only
// EH frame is left out. Not in a vtable: CAsyncOutputPin overrides Connect and its override (0x831F40) calls this
// one directly, with the IPin subobject as its stack this.
HRESULT STDMETHODCALLTYPE CBasePin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	if (!pReceivePin) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	_RTL_CRITICAL_SECTION* pLock = m_pLock;
	EnterCriticalSection(pLock);
	if (m_Connected) {
		LeaveCriticalSection(pLock);
		// VFW_E_ALREADY_CONNECTED
		return (HRESULT)0x80040204;
	}
	// EXE-INLINED none: CBasePin::IsStopped (the filter's m_State is State_Stopped, 0)
	if (m_pFilter->m_State != 0 && !m_bCanReconnectWhenActive) {
		LeaveCriticalSection(pLock);
		// VFW_E_NOT_STOPPED
		return (HRESULT)0x80040224;
	}
	// The SDK casts the const away: (CMediaType*)pmt.
	HRESULT hr = AgreeMediaType(pReceivePin, const_cast<CMediaType*>(static_cast<const CMediaType*>(pmt)));
	if (hr < 0) {
		BreakConnect();
		LeaveCriticalSection(pLock);
		return hr;
	}
	LeaveCriticalSection(pLock);
	return 0;
}

// CBasePin's IPin and IQualityControl methods the exe reaches through CAsyncOutputPin's
// tables for those subobjects (no adjustor thunks): each takes the subobject as its stack this.
// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBasePin::Disconnect() {
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	if (m_pFilter->m_State) {
		LeaveCriticalSection(m_pLock);
		return VFW_E_NOT_STOPPED;
	} else {
		int32_t v3 = DisconnectInternal();
		LeaveCriticalSection(m_pLock);
		return v3;
	}
}

HRESULT STDMETHODCALLTYPE CBasePin::ConnectedTo(IPin** ppPin) {
	if (!ppPin) {
		return E_POINTER;
	}
	IPin* m_Connected = this->m_Connected;
	*ppPin = m_Connected;
	if (!m_Connected) {
		return VFW_E_NOT_CONNECTED;
	}
	m_Connected->AddRef();
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBasePin::ConnectionMediaType(AM_MEDIA_TYPE* pType) {
	if (!pType) {
		return E_POINTER;
	}
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	if (m_Connected) {
		CopyMediaType(pType, &m_mt);
		LeaveCriticalSection(m_pLock);
		return 0;
	} else {
		static_cast<CMediaType*>(pType)->CMediaType::InitMediaType();
		LeaveCriticalSection(m_pLock);
		return VFW_E_NOT_CONNECTED;
	}
}

HRESULT STDMETHODCALLTYPE CBasePin::QueryPinInfo(PIN_INFO* pInfo) {
	IBaseFilter* v4;
	if (!pInfo) {
		return E_POINTER;
	}
	CBaseFilter* m_pFilter = this->m_pFilter;
	if (m_pFilter) {
		v4 = static_cast<IBaseFilter*>(m_pFilter);
	} else {
		v4 = 0;
	}
	pInfo->pFilter = v4;
	if (this->m_pFilter) {
		static_cast<IUnknown*>(static_cast<IBaseFilter*>(this->m_pFilter))->AddRef();
	}
	uint16_t* m_pName = this->m_pName;
	if (m_pName) {
		lstrcpynWInternal(pInfo->achName, reinterpret_cast<int16_t*>(m_pName), 128);
	} else {
		pInfo->achName[0u] = 0;
	}
	pInfo->dir = static_cast<PIN_DIRECTION>(m_dir);
	return 0;
}

HRESULT STDMETHODCALLTYPE CBasePin::QueryDirection(int32_t* pDirection) {
	if (!pDirection) {
		return E_POINTER;
	}
	*pDirection = m_dir;
	return 0;
}

// The exe tail-jumps to AMGetWideString (0x9FC45B), as the BaseClasses source calls it.
HRESULT STDMETHODCALLTYPE CBasePin::QueryId(uint16_t** pId) {
	return AMGetWideString(m_pName, pId);
}

// The SDK's CBasePin::QueryAccept: the media type is checked as the SDK's CMediaType (the source's cast there), and
// a failure becomes S_FALSE.
HRESULT STDMETHODCALLTYPE CBasePin::QueryAccept(const AM_MEDIA_TYPE* pType) {
	if (!pType) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	HRESULT hr = CheckMediaType(static_cast<const CMediaType*>(pType));
	if (hr < 0) {
		return 1;
	}
	return hr;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBasePin::EnumMediaTypes(IEnumMediaTypes** ppEnum) {
	CEnumMediaTypes* v4;
	if (!ppEnum) {
		return E_POINTER;
	}
	CEnumMediaTypes* v3 = static_cast<CEnumMediaTypes*>(ourMalloc(sizeof(CEnumMediaTypes)));
	if (v3) {
		v4 = ::new (v3) CEnumMediaTypes(this, nullptr);
	} else {
		v4 = 0;
	}
	*ppEnum = v4;
	return v4 != 0 ? 0 : E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE CBasePin::QueryInternalConnections(IPin** ppPins, uint32_t* pcPins) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBasePin::NewSegment(int64_t tStart, int64_t tStop, double dRate) {
	m_tStart = tStart;
	m_tStop = tStop;
	m_dRate = dRate;
	return 0;
}

HRESULT STDMETHODCALLTYPE CBasePin::Notify(IBaseFilter* pSender, Quality q) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBasePin::SetSink(IQualityControl* pSink) {
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	m_pQSink = pSink;
	LeaveCriticalSection(m_pLock);
	return 0;
}

CAsyncOutputPin::~CAsyncOutputPin() {}

// The SDK's DECLARE_IUNKNOWN on CAsyncOutputPin: delegates to the owner (the CBasePin's m_pUnknown at +0xC). The exe
// swaps the owner into the this slot and tail-jumps to its stdcall QueryInterface (slot 0).
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::QueryInterface(REFIID riid, void** ppv)
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->QueryInterface(riid, ppv);
}

// DECLARE_IUNKNOWN's AddRef: the owner's AddRef (slot 1, tail-jump).
uint32_t STDMETHODCALLTYPE CAsyncOutputPin::AddRef()
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->AddRef();
}

// DECLARE_IUNKNOWN's Release: the owner's Release (slot 2, tail-jump).
uint32_t STDMETHODCALLTYPE CAsyncOutputPin::Release()
{
	// EXE-INLINED none: CUnknown::GetOwner
	return m_pUnknown->Release();
}

// The SDK's RequestAllocator: fixes the requested alignment to the stream's, then uses pPreferred when it accepts
// the properties with a suitable alignment, else an allocator of its own. pPreferred is checked for null up front,
// so the SDK's `if (pPreferred)` is gone in the exe.
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps,
	IMemAllocator** ppActual)
{
	if (!pPreferred) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	if (!pProps || !ppActual) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	// EXE-INLINED 0x9FD3A0
	if (!pProps->cbAlign || ((m_pIo->m_pStream->Alignment() - 1) & pProps->cbAlign)) {
		m_pIo->Alignment(&pProps->cbAlign);
	}
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pPreferred->SetProperties(pProps, &Actual);
	if (hr >= 0 && m_pIo->IsAligned(Actual.cbAlign)) {
		pPreferred->AddRef();
		*ppActual = pPreferred;
		return 0;
	}
	IMemAllocator* pAlloc;
	hr = InitAllocator(&pAlloc);
	if (hr < 0) {
		return hr;
	}
	hr = pAlloc->SetProperties(pProps, &Actual);
	if (hr >= 0 && m_pIo->IsAligned(Actual.cbAlign)) {
		*ppActual = pAlloc;
		return 0;
	}
	pAlloc->Release();
	if (hr >= 0) {
		// VFW_E_BADALIGN
		hr = (HRESULT)0x8004020E;
	}
	return hr;
}

// The SDK's UNITS: reference-time ticks per second. The exe pushes it as an immediate (0x989680 at 0x9FD7F0), so it
// stays a constant here.
static constexpr int64_t kUnits = 10000000;

// The SDK's Request: the sample's start time becomes the file position and its duration the length, both in seconds;
// a span that runs past the stream is cut back to the stream's alignment, which also shortens the sample's stop
// time. The read is then queued for the buffer the sample holds.
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::Request(IMediaSample* pSample, uintptr_t dwUser)
{
	if (!pSample) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	int64_t llStart, llStop;
	HRESULT hr = pSample->GetTime(&llStart, &llStop);
	if (hr < 0) {
		return hr;
	}
	int64_t llPos = llStart / kUnits;
	int32_t lLength = static_cast<int32_t>((llStop - llStart) / kUnits);
	// The exe zeroes both before the call (0x9FD873), so a Length that fails leaves them 0.
	int64_t llTotal = 0;
	int64_t llAvailable = 0;
	m_pIo->Length(&llTotal, &llAvailable);
	if (llPos + lLength > llTotal) {
		int32_t lAlignment;
		m_pIo->Alignment(&lAlignment);
		llTotal = (llTotal + lAlignment - 1) & ~static_cast<int64_t>(lAlignment - 1);
		if (llPos + lLength > llTotal) {
			lLength = static_cast<int32_t>(llTotal - llPos);
			llStop = llTotal * kUnits;
			pSample->SetTime(&llStart, &llStop);
		}
	}
	uint8_t* pBuffer;
	hr = pSample->GetPointer(&pBuffer);
	if (hr < 0) {
		return hr;
	}
	return m_pIo->Request(llPos, lLength, 1, pBuffer, pSample, dwUser);
}

// The SDK's SyncReadAligned: as Request, but the read is done on the spot and the bytes read are stored back into
// the sample's actual length.
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::SyncReadAligned(IMediaSample* pSample)
{
	if (!pSample) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	int64_t llStart, llStop;
	HRESULT hr = pSample->GetTime(&llStart, &llStop);
	if (hr < 0) {
		return hr;
	}
	int64_t llPos = llStart / kUnits;
	int32_t lLength = static_cast<int32_t>((llStop - llStart) / kUnits);
	int64_t llTotal, llAvailable;
	m_pIo->Length(&llTotal, &llAvailable);
	if (llPos + lLength > llTotal) {
		int32_t lAlignment;
		m_pIo->Alignment(&lAlignment);
		llTotal = (llTotal + lAlignment - 1) & ~static_cast<int64_t>(lAlignment - 1);
		if (llPos + lLength > llTotal) {
			lLength = static_cast<int32_t>(llTotal - llPos);
			llStop = llTotal * kUnits;
			pSample->SetTime(&llStart, &llStop);
		}
	}
	uint8_t* pBuffer;
	hr = pSample->GetPointer(&pBuffer);
	if (hr < 0) {
		return hr;
	}
	int32_t lActual;
	hr = m_pIo->SyncReadAligned(llPos, lLength, pBuffer, &lActual, pSample);
	pSample->SetActualDataLength(lActual);
	return hr;
}

// The SDK's CBasePin::SetMediaType: copies pType into the pin's m_mt; a failure's HRESULT, else S_OK.
HRESULT CBasePin::SetMediaType(const CMediaType* pType) {
	HRESULT hr = SetMediaTypeFrom(&m_mt, nullptr, pType);
	return hr >= 0 ? S_OK : hr;
}

// The SDK's CBasePin::Inactive: clears the run-time error flag.
HRESULT CBasePin::Inactive() {
	m_bRunTimeError = 0;
	return 0;
}

HRESULT CAsyncOutputPin::CheckConnect(IPin* pPin) {
	m_bQueriedForAsyncReader = 0;
	return static_cast<CBasePin*>(this)->CBasePin::CheckConnect(pPin);
}

// The SDK's CAsyncOutputPin::CompleteConnect: VFW_E_NO_TRANSPORT unless the peer asked for IAsyncReader, else the
// base class's (the exe tail-jumps to 0xA38DE0).
HRESULT CAsyncOutputPin::CompleteConnect(IPin* pPin) {
	if (m_bQueriedForAsyncReader) {
		return CBasePin::CompleteConnect(pPin);
	}
	// VFW_E_NO_TRANSPORT
	return (HRESULT)0x80040266;
}

// The SDK's CAsyncOutputPin::BreakConnect: clears the IAsyncReader flag, then the base class's (the exe tail-jumps
// to 0xE7CD40).
HRESULT CAsyncOutputPin::BreakConnect() {
	m_bQueriedForAsyncReader = 0;
	return CBasePin::BreakConnect();
}

// The SDK's CAsyncOutputPin::NonDelegatingQueryInterface: IAsyncReader answers with this pin (and records that the
// peer asked for it), anything else goes to CBasePin's.
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
	if (!ppv) {
		return E_POINTER;
	}
	if (!IsEqualGUID(riid, CLSID_IAsyncReader)) {
		return CBasePin::NonDelegatingQueryInterface(riid, ppv);
	}
	m_bQueriedForAsyncReader = 1;
	return GetInterface(static_cast<IAsyncReader*>(this), ppv);
}

HRESULT CAsyncOutputPin::GetMediaType(int32_t iPosition, CMediaType* pType) {
	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}
	if (!pType) {
		return E_POINTER;
	}
	CAsyncReader* m_pReader = this->m_pReader;
	if (!m_pReader) {
		return E_UNEXPECTED;
	}
	pType->CopyFrom(&m_pReader->m_mt);
	return 0;
}

HRESULT CAsyncOutputPin::CheckMediaType(const CMediaType* pType) {
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	CAsyncReader* m_pReader = this->m_pReader;
	if (IsEqualGUID(m_pReader->m_mt.majortype, pType->majortype)
		&& (IsEqualGUID(m_pReader->m_mt.subtype, GUID_NULL) || IsEqualGUID(m_pReader->m_mt.subtype, pType->subtype))) {
		LeaveCriticalSection(m_pLock);
		return 0;
	} else {
		LeaveCriticalSection(m_pLock);
		return 1;
	}
}

// The SDK's CAsyncOutputPin::WaitForNext: the next finished request's sample, with its byte count stored as the
// sample's actual length when the read succeeded.
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::WaitForNext(uint32_t dwTimeout, IMediaSample** ppSample, uintptr_t* pdwUser)
{
	if (!ppSample) {
		return E_POINTER;
	}
	int32_t cbActual;
	// The request's context is the sample Request queued it with.
	void* pContext = nullptr;
	HRESULT hr = m_pIo->WaitForNextRequest(dwTimeout, &pContext, pdwUser, &cbActual);
	IMediaSample* pSample = static_cast<IMediaSample*>(pContext);
	if (hr >= 0) {
		pSample->SetActualDataLength(cbActual);
	}
	*ppSample = pSample;
	return hr;
}

HRESULT STDMETHODCALLTYPE CAsyncOutputPin::SyncRead(int64_t llPosition, int32_t lLength, uint8_t* pBuffer) {
	return m_pIo->SyncRead(llPosition, lLength, pBuffer);
}

HRESULT STDMETHODCALLTYPE CAsyncOutputPin::Length(int64_t* pTotal, int64_t* pAvailable) {
	return m_pIo->Length(pTotal, pAvailable);
}

HRESULT STDMETHODCALLTYPE CAsyncOutputPin::BeginFlush() {
	return m_pIo->BeginFlush();
}

HRESULT STDMETHODCALLTYPE CAsyncOutputPin::EndFlush() {
	return m_pIo->EndFlushIo();
}

// The SDK's: the reader connects through its own Connect (vtable slot 9).
HRESULT STDMETHODCALLTYPE CAsyncOutputPin::Connect(IPin* pPin, const AM_MEDIA_TYPE* pType) {
	return m_pReader->Func0009(pPin, pType);
}

// The exe's L"Output" text, a WCHAR string that src types as uint16_t: raw characters, no src type.
CAsyncOutputPin::CAsyncOutputPin(HRESULT* phr, CAsyncReader* pReader, CAsyncIo* pIo, _RTL_CRITICAL_SECTION* pLock)
	: CBasePin(nullptr, pReader, pLock, phr, reinterpret_cast<const uint16_t*>(u"Output"), PINDIR_OUTPUT),
	  m_pReader(pReader), m_pIo(pIo) {}

// The SDK's InitAllocator: a new CMemAllocator, whose IMemAllocator is returned referenced in ppAlloc. The exe keeps
// hr in ppAlloc's argument slot. The exe's cleanup-only C++ EH frame is left out.
HRESULT CAsyncOutputPin::InitAllocator(IMemAllocator** ppAlloc) {
	if (!ppAlloc) {
		// E_POINTER
		return (HRESULT)0x80004003;
	}
	HRESULT hr = 0;
	*ppAlloc = nullptr;
	CMemAllocator* pMemObject = static_cast<CMemAllocator*>(ourMalloc(sizeof(CMemAllocator)));
	if (pMemObject) {
		pMemObject = ::new (pMemObject) CMemAllocator(nullptr, nullptr, &hr);
	}
	if (!pMemObject) {
		// E_OUTOFMEMORY
		return (HRESULT)0x8007000E;
	}
	if (hr < 0) {
		delete pMemObject;
		return hr;
	}
	// QueryInterface's out-parameter is COM's untyped void**; it stores the IMemAllocator there.
	hr = static_cast<IUnknown*>(pMemObject)->QueryInterface(CLSID_IMemAllocator, reinterpret_cast<void**>(ppAlloc));
	if (hr < 0) {
		delete pMemObject;
		// E_NOINTERFACE
		return (HRESULT)0x80004002;
	}
	return 0;
}

// The exe's destructor (0x9FA1F0) frees the pin's name buffer.
CBasePin::~CBasePin() {
	ourFree(m_pName);
}

// The SDK's CBasePin::BreakConnect;
// CAsyncOutputPin::BreakConnect calls it.
HRESULT CBasePin::BreakConnect()
{
	return 0;
}

// The SDK's
// CBasePin::CompleteConnect; CAsyncOutputPin::CompleteConnect calls it.
HRESULT CBasePin::CompleteConnect([[maybe_unused]] IPin* pPin)
{
	return 0;
}

// The SDK's CBasePin::Active.
HRESULT CBasePin::Active()
{
	return 0;
}

// The SDK's
// CBasePin::Run.
HRESULT CBasePin::Run([[maybe_unused]] int64_t tStart)
{
	return 0;
}

// The SDK's
// CBasePin::EndOfStream.
HRESULT STDMETHODCALLTYPE CBasePin::EndOfStream()
{
	return 0;
}

// The SDK's CBasePin::CheckConnect: a peer of the pin's own direction is refused with VFW_E_INVALID_DIRECTION. The
// exe keeps the peer's direction in pPin's argument slot.
HRESULT CBasePin::CheckConnect(IPin* pPin) {
	int32_t iDirection;
	pPin->QueryDirection(&iDirection);
	if (iDirection == m_dir) {
		return (HRESULT)0x80040208;
	}
	return 0;
}

HRESULT WINAPI CBasePin::DisconnectInternal() {
	if (!m_Connected) {
		return 1;
	}
	int32_t result = BreakConnect();
	if (result >= 0) {
		m_Connected->Release();
		m_Connected = 0;
		return 0;
	}
	return result;
}

// The SDK's CBasePin::TryMediaTypes: attempts a connection with each type of pEnum that matches pmt (every type when
// pmt is null). The first failure other than E_FAIL, E_INVALIDARG and VFW_E_TYPE_NOT_ACCEPTED is returned when none
// connects, else VFW_E_NO_ACCEPTABLE_TYPES.
HRESULT CBasePin::TryMediaTypes(IPin* pReceivePin, CMediaType* pmt, IEnumMediaTypes* pEnum) {
	HRESULT hr = pEnum->Reset();
	if (hr < 0) {
		return hr;
	}
	AM_MEDIA_TYPE* pMediaType = nullptr;
	uint32_t ulMediaCount = 0;
	HRESULT hrFailure = 0;
	for (;;) {
		hr = pEnum->Next(1, &pMediaType, &ulMediaCount);
		if (hr != 0) {
			if (hrFailure == 0) {
				// VFW_E_NO_ACCEPTABLE_TYPES
				hrFailure = (HRESULT)0x80040207;
			}
			return hrFailure;
		}
		// The source's cast: the enumerator hands out CMediaTypes as AM_MEDIA_TYPEs.
		if (!pmt || static_cast<CMediaType*>(pMediaType)->MatchesPartial(pmt)) {
			hr = AttemptConnection(pReceivePin, static_cast<CMediaType*>(pMediaType));
			// E_FAIL, E_INVALIDARG and VFW_E_TYPE_NOT_ACCEPTED do not replace the failure kept.
			if (hr < 0 && hrFailure >= 0 && hr != (HRESULT)0x80004005 && hr != (HRESULT)0x80070057
				&& hr != (HRESULT)0x8004022A) {
				hrFailure = hr;
			}
		} else {
			// VFW_E_NO_ACCEPTABLE_TYPES
			hr = (HRESULT)0x80040207;
		}
		DeleteMediaType(pMediaType);
		if (hr == 0) {
			return 0;
		}
	}
}

// The DirectShow base classes' HRESULT* phr is not used here: CBasePin's constructor only passes it to the SDK's
// asserts. The pin's name is copied into a new buffer.
CBasePin::CBasePin(const char* pObjectName, CBaseFilter* pFilter, _RTL_CRITICAL_SECTION* pLock,
	[[maybe_unused]] HRESULT* phr, const uint16_t* pName, PIN_DIRECTION dir)
	: CUnknown(pObjectName, nullptr), m_pName(nullptr), m_Connected(nullptr), m_dir(dir), m_pLock(pLock),
	  m_bRunTimeError(false), m_bCanReconnectWhenActive(false), m_bTryMyTypesFirst(false), m_pFilter(pFilter),
	  m_pQSink(nullptr), m_TypeVersion(1), m_mt(), m_tStart(0),
	  // MAX_TIME
	  m_tStop(0x7FFFFFFFFFFFFFFFll), m_dRate(1.0) {
	if (pName) {
		uint32_t uiLength = strlen_w(pName) + 1;
		// new WCHAR[uiLength]: an overflowing size asks for 0xFFFFFFFF bytes.
		uint16_t* pCopy =
			static_cast<uint16_t*>(ourMalloc(uiLength <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiLength
				: SIZE_MAX));
		m_pName = pCopy;
		if (pCopy) {
			rt::memcpy(pCopy, pName, sizeof(uint16_t) * uiLength);
		}
	}
}

// The SDK's CBasePin::AttemptConnection: checks pReceivePin and the media type, then connects to it with that type.
// Any failure breaks the connection again and releases the connected pin.
HRESULT CBasePin::AttemptConnection(IPin* pReceivePin, CMediaType* pmt) {
	HRESULT hr = CheckConnect(pReceivePin);
	if (hr < 0) {
		BreakConnect();
		return hr;
	}
	hr = CheckMediaType(pmt);
	if (hr == 0) {
		m_Connected = pReceivePin;
		// The exe adds the reference through pReceivePin, not through a reload of m_Connected.
		pReceivePin->AddRef();
		hr = SetMediaType(pmt);
		if (hr >= 0) {
			hr = pReceivePin->ReceiveConnection(this, pmt);
			if (hr >= 0) {
				hr = CompleteConnect(pReceivePin);
				if (hr >= 0) {
					return hr;
				}
				pReceivePin->Disconnect();
			}
		}
	} else if (hr >= 0 || hr == (HRESULT)0x80004005 || hr == (HRESULT)0x80070057) {
		// Any success other than S_OK, E_FAIL and E_INVALIDARG become VFW_E_TYPE_NOT_ACCEPTED.
		hr = (HRESULT)0x8004022A;
	}
	BreakConnect();
	if (m_Connected) {
		m_Connected->Release();
		m_Connected = nullptr;
	}
	return hr;
}

// The SDK's CBasePin::AgreeMediaType: a fully specified pmt is attempted as it is; otherwise the types of both pins
// are tried, the peer's first unless m_bTryMyTypesFirst. The first failure other than E_FAIL, E_INVALIDARG and
// VFW_E_TYPE_NOT_ACCEPTED is returned when none connects, else VFW_E_NO_ACCEPTABLE_TYPES.
HRESULT CBasePin::AgreeMediaType(IPin* pReceivePin, CMediaType* pmt) {
	IEnumMediaTypes* pEnumMediaTypes = nullptr;
	if (pmt && !pmt->IsPartiallySpecified()) {
		return AttemptConnection(pReceivePin, pmt);
	}
	// VFW_E_NO_ACCEPTABLE_TYPES
	HRESULT hrFailure = (HRESULT)0x80040207;
	for (int32_t i = 0; i < 2; i++) {
		HRESULT hr;
		if (i == m_bTryMyTypesFirst) {
			hr = pReceivePin->EnumMediaTypes(&pEnumMediaTypes);
		} else {
			// The exe calls this pin's EnumMediaTypes through its IPin table.
			hr = static_cast<IPin*>(this)->EnumMediaTypes(&pEnumMediaTypes);
		}
		if (hr >= 0) {
			hr = TryMediaTypes(pReceivePin, pmt, pEnumMediaTypes);
			pEnumMediaTypes->Release();
			if (hr >= 0) {
				return 0;
			}
			if (hr != (HRESULT)0x80004005 && hr != (HRESULT)0x80070057 && hr != (HRESULT)0x8004022A) {
				hrFailure = hr;
			}
		}
	}
	return hrFailure;
}

// The SDK's CMediaType::Set(const CMediaType&): the exe jumps to the AM_MEDIA_TYPE overload.
HRESULT SetMediaTypeFrom(CMediaType* apThis, [[maybe_unused]] void* apEdx, const CMediaType* a2)
{
	return apThis->Set(*a2);
}

// The SDK's CBasePin::NonDelegatingQueryInterface: IPin and IQualityControl answer with the pin's subobjects at +0xC
// and +0x10 (the casts keep MSVC's null test of the pin), anything else goes to CUnknown's.
HRESULT STDMETHODCALLTYPE CBasePin::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
	CBasePin* pThis = this;
	if (IsEqualGUID(riid, CLSID_IPin)) {
		if (pThis) {
			return GetInterface(static_cast<IPin*>(pThis), ppv);
		}
		return GetInterface(nullptr, ppv);
	}
	if (!IsEqualGUID(riid, CLSID_IQualityControl)) {
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
	if (!pThis) {
		return GetInterface(nullptr, ppv);
	}
	return GetInterface(static_cast<IQualityControl*>(pThis), ppv);
}
