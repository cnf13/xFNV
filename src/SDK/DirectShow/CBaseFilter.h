#pragma once

#include "SDK/DirectShow/CUnknown.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
class CBaseObject;
class NiPoint4;

class CBasePin;
struct IFilterGraph;
struct IEnumPins;
struct IEnumFilters;
struct IPin;
struct AMOVIESETUP_FILTER;
struct IFilterMapper;

extern _GUID rclsid;
extern GUID CLSID_IFilterMapper;
extern GUID CLSID_IMediaEventSink;

// The graph's reference clock (the SDK's IReferenceClock): the current time and the events scheduled from it. The
// PDB has no class; IDA's IReferenceClockVtbl gives the methods and CAsyncReader::StreamTime (0x9F9DB0) calls
// GetTime through the vtable at +0xC, while the CAsyncReader destructor (0x9FB550) releases it through +0x8.
struct IReferenceClock : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE GetTime(int64_t* pTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE AdviseTime(int64_t rtBaseTime, int64_t rtStreamTime, void* hEvent,
		uint32_t* pdwAdviseCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE AdvisePeriodic(int64_t rtStartTime, int64_t rtPeriodTime, void* hSemaphore,
		uint32_t* pdwAdviseCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unadvise(uint32_t dwAdviseCookie) = 0;
};

struct IPersist : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE GetClassID(GUID* pClassID) = 0;
};
struct IMediaFilter : IPersist {
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
	virtual HRESULT STDMETHODCALLTYPE Pause() = 0;
	virtual HRESULT STDMETHODCALLTYPE Run(int64_t tStart) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetState(uint32_t dwMSecs, int32_t* pState) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock* pClock) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock** ppClock) = 0;
};

// What QueryFilterInfo fills in (dshow.h's FILTER_INFO, IDA's _FilterInfo): the filter's name and the graph it joined.
// The exe writes the name as 0x80 wide characters at +0 (0x9F9E0E push 0x80) and the graph pointer at +0x100
// (0x9F9E24 mov [edi+0x100],eax), sizeof 0x104.
struct FILTER_INFO {
	uint16_t achName[128];
	IFilterGraph* pGraph;
};

struct IBaseFilter : IMediaFilter {
	virtual HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins** ppEnum) = 0;
	virtual HRESULT STDMETHODCALLTYPE FindPin(const uint16_t* Id, IPin** ppPin) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO* pInfo) = 0;
	virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph* pGraph, const uint16_t* pName) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(uint16_t** pVendorInfo) = 0;
};
struct IAMovieSetup : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Register() = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister() = 0;
};

// The graph a filter joins (dshow.h's IFilterGraph). IDA's IFilterGraphVtbl gives the methods and their order; the exe
// calls AddFilter through slot +0xC (0x830B30 FalloutAudioMedia::ThreadProc: 0x831027 mov eax,[ecx+0xC]; call eax) and
// QueryInterface through slot 0 (0x831107 mov eax,[ecx]; call eax). The wide strings are the SDK's LPCWSTR.
struct IFilterGraph : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter* pFilter, const uint16_t* pName) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter* pFilter) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters** ppEnum) = 0;
	virtual HRESULT STDMETHODCALLTYPE FindFilterByName(const uint16_t* pName, IBaseFilter** ppFilter) = 0;
	virtual HRESULT STDMETHODCALLTYPE ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt) = 0;
	virtual HRESULT STDMETHODCALLTYPE Reconnect(IPin* pPin) = 0;
	virtual HRESULT STDMETHODCALLTYPE Disconnect(IPin* pPin) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDefaultSyncSource() = 0;
};

// The filter graph manager's building interface (strmif.h's IGraphBuilder): IFilterGraph's methods, then the seven of
// IDA's IGraphBuilderVtbl (0x48 bytes, 18 slots). FalloutAudioMedia::ThreadProc (0x830B30) creates one and calls
// QueryInterface through slot 0 for its IMediaControl, IMediaEvent, IMediaPosition and IBasicAudio. The wide strings
// are the SDK's LPCWSTR; hFile is its DWORD_PTR.
struct IGraphBuilder : IFilterGraph {
	virtual HRESULT STDMETHODCALLTYPE Connect(IPin* pPinOut, IPin* pPinIn) = 0;
	virtual HRESULT STDMETHODCALLTYPE Render(IPin* pPinOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE RenderFile(const uint16_t* lpcwstrFile, const uint16_t* lpcwstrPlayList) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddSourceFilter(const uint16_t* lpcwstrFileName,
		const uint16_t* lpcwstrFilterName, IBaseFilter** ppFilter) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetLogFile(uintptr_t hFile) = 0;
	virtual HRESULT STDMETHODCALLTYPE Abort() = 0;
	virtual HRESULT STDMETHODCALLTYPE ShouldOperationContinue() = 0;
};

// The sink a filter graph reports its events to (evcode.h's IMediaEventSink): IDA's IMediaEventSinkVtbl has Notify
// after IUnknown's three methods, with the SDK's LONG_PTR event parameters. CBaseFilter holds the one its
// JoinFilterGraph obtains at +0x44.
struct IMediaEventSink : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Notify(int32_t EventCode, intptr_t EventParam1, intptr_t EventParam2) = 0;
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// The constructor stores the CLSID at +0x28 and the lock at +0x38; CAsyncReader starts its lock at +0x50.
class CBaseFilter : public CUnknown, public IBaseFilter, public IAMovieSetup {
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE AddRef() override;
	uint32_t STDMETHODCALLTYPE Release() override;
	// Pure here and in StreamTime, GetPinVersion and GetSetupData: the exe has no CBaseFilter vtable (the SDK's
	// abstract base, AM_NOVTABLE). Its one filter, CAsyncReader, has its own entries for all four (0x9FB480,
	// 0x9F9DB0, 0xA7B850, 0xACBB70), defined on CAsyncReader, and no code calls a CBaseFilter copy directly.
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override = 0;
	~CBaseFilter() override;
	virtual HRESULT StreamTime(int64_t& rtStream) = 0;
	virtual int32_t GetPinVersion() = 0;
	virtual int32_t GetPinCount() = 0;
	virtual CBasePin* GetPin(int32_t n) = 0;
	// The SDK's LPAMOVIESETUP_FILTER GetSetupData() (slot 8, +0x20): Register and Unregister pass its result on to
	// AMovieSetupRegisterFilterWithMapper, the SDK's AMovieSetupRegisterFilter.
	virtual const AMOVIESETUP_FILTER* GetSetupData() = 0;

	HRESULT STDMETHODCALLTYPE GetClassID(GUID* pClassID) override;
	HRESULT STDMETHODCALLTYPE Stop() override;
	HRESULT STDMETHODCALLTYPE Pause() override;
	HRESULT STDMETHODCALLTYPE Run(int64_t tStart) override;
	HRESULT STDMETHODCALLTYPE GetState(uint32_t dwMSecs, int32_t* pState) override;
	HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock* pClock) override;
	HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock** ppClock) override;
	HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins** ppEnum) override;
	HRESULT STDMETHODCALLTYPE FindPin(const uint16_t* Id, IPin** ppPin) override;
	HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO* pInfo) override;
	HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph* pGraph, const uint16_t* pName) override;
	HRESULT STDMETHODCALLTYPE QueryVendorInfo(uint16_t** pVendorInfo) override;
	HRESULT STDMETHODCALLTYPE Register() override;
	HRESULT STDMETHODCALLTYPE Unregister() override;

	int32_t m_State;
	IReferenceClock* m_pClock;
	int64_t m_tStart;
	GUID m_clsid;
	_RTL_CRITICAL_SECTION* m_pLock;
	uint16_t* m_pName;
	IFilterGraph* m_pGraph;
	IMediaEventSink* m_pSink;
	int32_t m_PinVersion;
public:
	CBaseFilter(const char* apName, IUnknown* apUnk, _RTL_CRITICAL_SECTION* apLock, const GUID& arClsid,
		HRESULT* apHr);
};

// Not named in the PDB or IDA; name chosen here. The DirectShow SDK's AMovieSetupRegisterFilter (dllsetup.cpp):
// stdcall, ret 0xC; IAMovieSetup's Register (0x9FBA40) and Unregister (0x9FBAB0) call it.
HRESULT WINAPI AMovieSetupRegisterFilterWithMapper(const AMOVIESETUP_FILTER* psetupdata, IFilterMapper* pIFM, int32_t bRegister);
