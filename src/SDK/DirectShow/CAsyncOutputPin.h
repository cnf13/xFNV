#pragma once

#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CEnumMediaTypes.h"
#include "SDK/DirectShow/CMediaSample.h"
#include "SDK/DirectShow/CMemAllocator.h"
class CMediaType;

class CAsyncIo;
class CAsyncReader;

// A pin's direction (dshow.h's PIN_DIRECTION).
enum PIN_DIRECTION : int32_t {
	PINDIR_INPUT = 0,
	PINDIR_OUTPUT = 1,
};

// What QueryPinInfo fills in (dshow.h's PIN_INFO, IDA's _PinInfo): the owning filter, the direction and the pin's
// name. The exe writes the filter at +0 (0x9FA425 mov [edi],eax), the direction at +4 (0x9FA456 mov [edi+4],ecx) and
// the name as 0x80 wide characters at +8 (0x9FA443 push 0x80; 0x9FA449 lea eax,[edi+8]), sizeof 0x108.
struct PIN_INFO {
	IBaseFilter* pFilter;
	PIN_DIRECTION dir;
	uint16_t achName[128];
};

struct IPin : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Connect(IPin* pPin, const AM_MEDIA_TYPE* pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin* pPin, const AM_MEDIA_TYPE* pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE Disconnect() = 0;
	virtual HRESULT STDMETHODCALLTYPE ConnectedTo(IPin** ppPin) = 0;
	virtual HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE* pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO* pInfo) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryDirection(int32_t* pDirection) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryId(uint16_t** pId) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE* pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes** ppEnum) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin** ppPins, uint32_t* pcPins) = 0;
	virtual HRESULT STDMETHODCALLTYPE EndOfStream() = 0;
	virtual HRESULT STDMETHODCALLTYPE BeginFlush() = 0;
	virtual HRESULT STDMETHODCALLTYPE EndFlush() = 0;
	virtual HRESULT STDMETHODCALLTYPE NewSegment(int64_t tStart, int64_t tStop, double dRate) = 0;
};
struct Quality {
	int32_t Type;
	int32_t Proportion;
	int64_t Late;
	int64_t TimeStamp;
};
struct IQualityControl : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Notify(IBaseFilter* pSender, Quality q) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSink(IQualityControl* pSink) = 0;
};
struct IAsyncReader : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps,
		IMemAllocator** ppActual) = 0;
	virtual HRESULT STDMETHODCALLTYPE Request(IMediaSample* pSample, uintptr_t dwUser) = 0;
	virtual HRESULT STDMETHODCALLTYPE WaitForNext(uint32_t dwTimeout, IMediaSample** ppSample, uintptr_t* pdwUser) = 0;
	virtual HRESULT STDMETHODCALLTYPE SyncReadAligned(IMediaSample* pSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SyncRead(int64_t llPosition, int32_t lLength, uint8_t* pBuffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE Length(int64_t* pTotal, int64_t* pAvailable) = 0;
	virtual HRESULT STDMETHODCALLTYPE BeginFlush() = 0;
	virtual HRESULT STDMETHODCALLTYPE EndFlush() = 0;
};

// IDA name; not in the PDB. The PDB has no source path; placed with its DirectShow output-pin subclass.
// The SDK member order matches constructor 0x9FC890's stores, including the media type at +0x34.
class CBasePin : public CUnknown, public IPin, public IQualityControl {
public:
	// The SDK's DECLARE_IUNKNOWN forwards these to the owner; the exe has no vtable of CBasePin's own (it is abstract)
	// and every pin it builds overrides them, so it has no copy of them.
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override = 0;
	uint32_t STDMETHODCALLTYPE AddRef() override = 0;
	uint32_t STDMETHODCALLTYPE Release() override = 0;
	// The SDK's CBasePin::NonDelegatingQueryInterface, which the exe also calls directly (stdcall, ret 12, 0x9FA260).
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE NonDelegatingAddRef() override;
	uint32_t STDMETHODCALLTYPE NonDelegatingRelease() override;
	~CBasePin() override;
	virtual int32_t GetMediaTypeVersion() { return m_TypeVersion; }
	virtual HRESULT Active();
	virtual HRESULT Inactive();
	virtual HRESULT Run(int64_t tStart);
	virtual HRESULT CheckMediaType(const CMediaType* pType) = 0;
	virtual HRESULT SetMediaType(const CMediaType* pType);
	// The base classes' CBasePin::CheckConnect: pPin's QueryDirection (slot 9),
	// VFW_E_INVALID_DIRECTION (0x80040208) when it equals m_dir, else 0. CAsyncOutputPin::CheckConnect
	// calls it.
	virtual HRESULT CheckConnect(IPin* pPin);
	// The base classes' S_OK: the exe's 0xE7CD40 (a fold, thiscall, ret 0), which CAsyncOutputPin::BreakConnect calls.
	virtual HRESULT BreakConnect();
	// The base classes' S_OK: the exe's 0xA38DE0 (a fold, thiscall, ret 4), which CAsyncOutputPin::CompleteConnect
	// calls.
	virtual HRESULT CompleteConnect(IPin* pPin);
	// The SDK's returns E_UNEXPECTED; the exe has no vtable of CBasePin's own and no copy of it: every pin it builds
	// overrides it.
	virtual HRESULT GetMediaType(int32_t iPosition, CMediaType* pType) = 0;

	HRESULT STDMETHODCALLTYPE Connect(IPin* pPin, const AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin* pPin, const AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE Disconnect() override;
	HRESULT STDMETHODCALLTYPE ConnectedTo(IPin** ppPin) override;
	HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO* pInfo) override;
	HRESULT STDMETHODCALLTYPE QueryDirection(int32_t* pDirection) override;
	HRESULT STDMETHODCALLTYPE QueryId(uint16_t** pId) override;
	HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes** ppEnum) override;
	HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin** ppPins, uint32_t* pcPins) override;
	HRESULT STDMETHODCALLTYPE EndOfStream() override;
	HRESULT STDMETHODCALLTYPE NewSegment(int64_t tStart, int64_t tStop, double dRate) override;
	HRESULT STDMETHODCALLTYPE Notify(IBaseFilter* pSender, Quality q) override;
	HRESULT STDMETHODCALLTYPE SetSink(IQualityControl* pSink) override;

	uint16_t* m_pName;
	IPin* m_Connected;
	int32_t m_dir;
	_RTL_CRITICAL_SECTION* m_pLock;
	bool m_bRunTimeError;
	bool m_bCanReconnectWhenActive;
	bool m_bTryMyTypesFirst;
	uint8_t padding27;
	CBaseFilter* m_pFilter;
	IQualityControl* m_pQSink;
	int32_t m_TypeVersion;
	CMediaType m_mt;
	int64_t m_tStart;
	int64_t m_tStop;
	double m_dRate;
public:
	// Not named in the PDB or IDA; the DirectShow base classes' name (thiscall, ret 0xC, 0x9FBFC0).
	HRESULT TryMediaTypes(IPin* pReceivePin, CMediaType* pmt, IEnumMediaTypes* pEnum);
	// The base classes' CBasePin::DisconnectInternal (STDMETHODIMP: stdcall, `this` pushed,
	// ret 4; 0x9FA380): with m_Connected (+0x18) set, BreakConnect (slot 11), and on success m_Connected released and
	// cleared, 0; S_FALSE (1) when not connected. CAsyncOutputPin's Disconnect path (0x9FC300) calls it.
	HRESULT WINAPI DisconnectInternal();
public:
	CBasePin(const char* pObjectName, CBaseFilter* pFilter, _RTL_CRITICAL_SECTION* pLock, HRESULT* phr,
		const uint16_t* pName, PIN_DIRECTION dir);
public:
	// Not named in the PDB or IDA; the DirectShow base classes' name (thiscall, ret 8, 0x9FBF00).
	HRESULT AttemptConnection(IPin* pReceivePin, CMediaType* pmt);
	// Not named in the PDB or IDA; the DirectShow base classes' name (thiscall, ret 8, 0x9FC0B0).
	HRESULT AgreeMediaType(IPin* pReceivePin, CMediaType* pmt);
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// RTTI puts IAsyncReader at zero and CBasePin at +8. QueryInterface writes the queried flag at +0xA8.
class CAsyncOutputPin : public IAsyncReader, public CBasePin {
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE AddRef() override;
	uint32_t STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
	~CAsyncOutputPin() override;
	HRESULT CheckMediaType(const CMediaType* pType) override;
	HRESULT CheckConnect(IPin* pPin) override;
	HRESULT BreakConnect() override;
	HRESULT CompleteConnect(IPin* pPin) override;
	HRESULT GetMediaType(int32_t iPosition, CMediaType* pType) override;

	HRESULT STDMETHODCALLTYPE Connect(IPin* pPin, const AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE BeginFlush() override;
	HRESULT STDMETHODCALLTYPE EndFlush() override;
	HRESULT STDMETHODCALLTYPE RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps,
		IMemAllocator** ppActual) override;
	HRESULT STDMETHODCALLTYPE Request(IMediaSample* pSample, uintptr_t dwUser) override;
	HRESULT STDMETHODCALLTYPE WaitForNext(uint32_t dwTimeout, IMediaSample** ppSample, uintptr_t* pdwUser) override;
	HRESULT STDMETHODCALLTYPE SyncReadAligned(IMediaSample* pSample) override;
	HRESULT STDMETHODCALLTYPE SyncRead(int64_t llPosition, int32_t lLength, uint8_t* pBuffer) override;
	HRESULT STDMETHODCALLTYPE Length(int64_t* pTotal, int64_t* pAvailable) override;
	// Creates a CMemAllocator and returns its
	// IMemAllocator, referenced, in ppAlloc.
	HRESULT InitAllocator(IMemAllocator** ppAlloc);

	CAsyncReader* m_pReader;
	CAsyncIo* m_pIo;
	int32_t m_bQueriedForAsyncReader;
public:
	CAsyncOutputPin(HRESULT* phr, CAsyncReader* pReader, CAsyncIo* pIo, _RTL_CRITICAL_SECTION* pLock);
};

// The exe's vtables of CAsyncOutputPin: its own (IAsyncReader at +0) and those of its CBasePin (+8), IPin (+0x14) and
// IQualityControl (+0x18) subobjects.
inline constexpr fnvd::Vtable CAsyncOutputPin_vftable{};
inline constexpr fnvd::Vtable CAsyncOutputPin_CBasePin_vftable{};
inline constexpr fnvd::Vtable CAsyncOutputPin_IPin_vftable{};
inline constexpr fnvd::Vtable CAsyncOutputPin_IQualityControl_vftable{};

// Not named in the PDB or IDA; name chosen here. The DirectShow base classes' CMediaType::Set(const CMediaType&)
// (mtype.cpp): a jmp to the AM_MEDIA_TYPE overload, CMediaType::Set (0x9F9C80). The exe calls it thiscall
// (ret 4) on a CMediaType (CBasePin::SetMediaType 0x9FA330: ecx = the pin's m_mt) with the CMediaType to copy.
HRESULT SetMediaTypeFrom(CMediaType* apThis, [[maybe_unused]] void* apEdx, const CMediaType* a2);

extern GUID CLSID_IQualityControl;

extern GUID CLSID_IPin;
