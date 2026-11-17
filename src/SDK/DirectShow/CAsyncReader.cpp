#include "SDK/DirectShow/CAsyncReader.h"
#include "BSCore/MemoryManager.h"
#include "BSMovie/CAsyncStream.h"
#include "BSMovie/CBaseObject.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"
#include "SDK/DirectShow/CAsyncIo.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include <stdint.h>

GUID rdata_1093D70{0xC553F2C0, 0x1529, 0x11D0, {0xB4, 0xD1, 0x00, 0x80, 0x5F, 0x6C, 0xBB,
	0xEA}};

struct AM_MEDIA_TYPE;
class CBaseObject;
struct IUnknown;

// The SDK's CAsyncReader::Connect: the output pin's base CBasePin::Connect, called directly.
HRESULT CAsyncReader::Func0009(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) {
	return m_OutputPin.CBasePin::Connect(pReceivePin, pmt);
}

// The SDK's CBaseFilter::StreamTime: VFW_E_NO_CLOCK (0x80040213) without a clock, else the clock's time less the
// start time, or the clock's failure.
HRESULT CAsyncReader::StreamTime(int64_t& rtStream) {
	if (!m_pClock) {
		return static_cast<HRESULT>(0x80040213);
	}
	HRESULT hr = m_pClock->GetTime(&rtStream);
	if (hr < 0) {
		return hr;
	}
	rtStream -= m_tStart;
	return 0;
}

// The SDK's: IBaseFilter, IMediaFilter and IPersist through the IBaseFilter subobject, IAMovieSetup through its own
// (a null this stays null), anything else through CUnknown's NonDelegatingQueryInterface.
HRESULT STDMETHODCALLTYPE CAsyncReader::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
	if (IsEqualGUID(riid, CLSID_IBaseFilter) || IsEqualGUID(riid, CLSID_IMediaFilter)
		|| IsEqualGUID(riid, CLSID_IPersist)) {
		return GetInterface(static_cast<IBaseFilter*>(this), ppv);
	}
	if (IsEqualGUID(riid, CLSID_IAMovieSetup)) {
		return GetInterface(static_cast<IAMovieSetup*>(this), ppv);
	}
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

CAsyncReader::~CAsyncReader() {
	DeleteCriticalSection(&m_csFilter);
}

// The SDK's CBaseFilter::GetSetupData.
const AMOVIESETUP_FILTER* CAsyncReader::GetSetupData()
{
	return nullptr;
}

// The SDK's CAsyncReader::GetPinCount: the
// one output pin.
int32_t CAsyncReader::GetPinCount()
{
	return 1;
}

CBasePin* CAsyncReader::GetPin(int32_t n) {
	// The exe also tests the output pin's address (0x9FDCC5), which is never null.
	if (GetPinCount() <= 0 || n) {
		return 0;
	} else {
		return &static_cast<CBasePin&>(m_OutputPin);
	}
}

CAsyncReader::CAsyncReader(const char* apName, IUnknown* apUnk, CAsyncStream* a3, HRESULT* a4)
	: CBaseFilter(apName, apUnk, &m_csFilter, rdata_1093D70, nullptr), m_Io(a3),
	  m_OutputPin(a4, this, &m_Io, &m_csFilter), m_mt() {
	InitializeCriticalSection(&m_csFilter);
}

void CAsyncReader::operator delete(void* apMemory) {
	ourFree(apMemory);
}
