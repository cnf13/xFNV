#include "SDK/DirectShow/CBaseFilter.h"
#include "BSCore/NewOverloads.h"
#include "BSMovie/CBaseObject.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CEnumPins.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include "SDK/DirectShow/strmif.h"
#include "SDK/DirectShow/wxutil.h"
#include "SDK/MSVC/string.h"
#include <stdint.h>

_GUID rclsid{0xE436EBB2, 21071, 4558, {159, 83, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IMediaEventSink{0x56A868A2, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IFilterMapper{0x56A868A3, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};

class CBaseObject;
class CUnknown;
struct IUnknown;

CUnknown::~CUnknown() {}

// The SDK's DECLARE_IUNKNOWN on CBaseFilter: delegates to the owner (m_pUnknown) with its QueryInterface (slot 0).
// The IAMovieSetup slot (0x831FE0, the subobject at +0x10) is a stdcall adjustor thunk (sub [esp+4], 4; jmp
// 0x831EB0), not ported; the body is the IBaseFilter slot's (CAsyncReader's and CBSAReader's tables at +0xC,
// slot 0): this - 0xC, the owner through the one-field getter 0x726070, then the owner's QueryInterface.
HRESULT STDMETHODCALLTYPE CBaseFilter::QueryInterface(REFIID riid, void** ppv)
{
	return m_pUnknown->QueryInterface(riid, ppv);
}

// DECLARE_IUNKNOWN's AddRef: the owner's AddRef (slot 1). The IBaseFilter slot's body (slot 1): this - 0xC, the
// owner through the one-field getter, then the owner's AddRef; the IAMovieSetup slot 0x832000 is an adjustor thunk
// to it, not ported.
uint32_t STDMETHODCALLTYPE CBaseFilter::AddRef()
{
	return m_pUnknown->AddRef();
}

// DECLARE_IUNKNOWN's Release: the owner's Release (slot 2). The IBaseFilter slot's body (slot 2): this - 0xC, the
// owner through the one-field getter, then the owner's Release; the IAMovieSetup slot 0x831FF0 is an adjustor thunk
// to it, not ported.
uint32_t STDMETHODCALLTYPE CBaseFilter::Release()
{
	return m_pUnknown->Release();
}

// The IBaseFilter methods below are the SDK's CBaseFilter ones (the exe has no CBaseFilter table: they are the
// entries CAsyncReader's and CBSAReader's tables at +0xC share); the exe's entries take that subobject as this.

HRESULT STDMETHODCALLTYPE CBaseFilter::GetClassID(GUID* pClassID) {
	if (!pClassID) {
		return E_POINTER;
	}
	*pClassID = m_clsid;
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::Stop() {
	IBaseFilter* v1 = static_cast<IBaseFilter*>(this);
	_RTL_CRITICAL_SECTION* lpCriticalSection = m_pLock;
	EnterCriticalSection(lpCriticalSection);
	int32_t v2 = 0;
	int32_t v3 = 0;
	if (m_State) {
		int32_t v4 = GetPinCount();
		if (v4 > 0) {
			do {
				CBasePin* v5 = GetPin(v2);
				if (v5->m_Connected) {
					int32_t v6 = v5->Inactive();
					if (v6 < 0 && v3 >= 0) {
						v3 = v6;
					}
				}
				++v2;
			} while (v2 < v4);
		}
		v1 = static_cast<IBaseFilter*>(this);
	}
	this->m_State = 0;
	LeaveCriticalSection(lpCriticalSection);
	return v3;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::Pause() {
	int32_t v3;
	CBasePin* v4;
	IBaseFilter* v1 = static_cast<IBaseFilter*>(this);
	_RTL_CRITICAL_SECTION* lpCriticalSection = m_pLock;
	EnterCriticalSection(lpCriticalSection);
	int32_t v2 = 0;
	if (m_State || (v3 = GetPinCount(), v3 <= 0)) {
	LABEL_7:;
		this->m_State = 1;
		LeaveCriticalSection(lpCriticalSection);
		return 0;
	}
	while (1) {
		v4 = GetPin(v2);
		if (v4->m_Connected) {
			break;
		}
	LABEL_6:;
		if (++v2 >= v3) {
			goto LABEL_7;
		}
	}
	int32_t v5 = v4->Active();
	if (v5 >= 0) {
		v1 = static_cast<IBaseFilter*>(this);
		goto LABEL_6;
	}
	LeaveCriticalSection(lpCriticalSection);
	return v5;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::Run(int64_t tStart) {
	int32_t v5;
	_RTL_CRITICAL_SECTION* lpCriticalSection = m_pLock;
	_RTL_CRITICAL_SECTION* v3 = lpCriticalSection;
	EnterCriticalSection(lpCriticalSection);
	int32_t v4 = 0;
	m_tStart = tStart;
	if (m_State || (v5 = static_cast<IBaseFilter*>(this)->Pause(), v5 >= 0)) {
		if (m_State != 2) {
			int32_t v6 = GetPinCount();
			int32_t v9 = v6;
			if (v6 > 0) {
				do {
					CBasePin* v7 = GetPin(v4);
					if (v7->m_Connected) {
						v5 = v7->Run(tStart);
						if (v5 < 0) {
							goto LABEL_11;
						}
						v6 = v9;
					}
					++v4;
				} while (v4 < v6);
			}
			v3 = lpCriticalSection;
		}
		m_State = 2;
		LeaveCriticalSection(v3);
		return 0;
	} else {
	LABEL_11:;
		LeaveCriticalSection(lpCriticalSection);
		return v5;
	}
}

HRESULT STDMETHODCALLTYPE CBaseFilter::GetState(uint32_t dwMSecs, int32_t* pState) {
	if (!pState) {
		return E_POINTER;
	}
	*pState = m_State;
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::SetSyncSource(IReferenceClock* pClock) {
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	if (pClock) {
		pClock->AddRef();
	}
	IReferenceClock* m_pClock = this->m_pClock;
	if (m_pClock) {
		this->m_pClock->Release();
	}
	this->m_pClock = pClock;
	LeaveCriticalSection(m_pLock);
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::GetSyncSource(IReferenceClock** ppClock) {
	if (!ppClock) {
		return E_POINTER;
	}
	_RTL_CRITICAL_SECTION* m_pLock = this->m_pLock;
	EnterCriticalSection(m_pLock);
	if (m_pClock) {
		m_pClock->AddRef();
	}
	*ppClock = m_pClock;
	LeaveCriticalSection(m_pLock);
	return 0;
}

// The SDK's CBaseFilter::EnumPins: a new CEnumPins over this filter, E_POINTER without ppEnum, E_OUTOFMEMORY when
// the allocation fails. The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::EnumPins(IEnumPins** ppEnum) {
	if (!ppEnum) {
		return E_POINTER;
	}
	CEnumPins* pMemory = static_cast<CEnumPins*>(ourMalloc(sizeof(CEnumPins)));
	CEnumPins* pEnum;
	if (pMemory) {
		pEnum = ::new (pMemory) CEnumPins(this, nullptr);
	} else {
		pEnum = nullptr;
	}
	*ppEnum = pEnum;
	return pEnum ? S_OK : E_OUTOFMEMORY;
}

// The SDK's CBaseFilter::FindPin: the pin whose name is Id, AddRef'd, else VFW_E_NOT_FOUND (0x80040216); E_POINTER
// (0x80004003) without ppPin. The exe's cleanup-only C++ EH frame (the SDK's CAutoLock) is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::FindPin(const uint16_t* Id, IPin** ppPin) {
	int32_t i = 0;
	if (!ppPin) {
		return static_cast<HRESULT>(0x80004003);
	}
	_RTL_CRITICAL_SECTION* pLock = m_pLock;
	EnterCriticalSection(pLock);
	int32_t iCount = GetPinCount();
	if (iCount > 0) {
		do {
			CBasePin* pPin = GetPin(i);
			if (!lstrcmpWInternal(pPin->m_pName, Id)) {
				// The pin was just dereferenced: the exe adds the IPin base's offset without a null test.
				IPin* pIPin = &static_cast<IPin&>(*pPin);
				*ppPin = pIPin;
				pIPin->AddRef();
				LeaveCriticalSection(pLock);
				return 0;
			}
		} while (++i < iCount);
	}
	*ppPin = nullptr;
	LeaveCriticalSection(pLock);
	return static_cast<HRESULT>(0x80040216);
}

HRESULT STDMETHODCALLTYPE CBaseFilter::QueryFilterInfo(FILTER_INFO* pInfo) {
	if (!pInfo) {
		return E_POINTER;
	}
	uint16_t* m_pName = this->m_pName;
	if (m_pName) {
		lstrcpynWInternal(pInfo->achName, reinterpret_cast<int16_t*>(m_pName), 128);
	} else {
		pInfo->achName[0] = 0;
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CBaseFilter::JoinFilterGraph(IFilterGraph* pGraph, const uint16_t* pName) {
	_RTL_CRITICAL_SECTION* lpCriticalSection = m_pLock;
	EnterCriticalSection(lpCriticalSection);
	m_pGraph = pGraph;
	if (pGraph) {
		if (pGraph->QueryInterface(CLSID_IMediaEventSink, reinterpret_cast<void**>(&m_pSink)) >= 0) {
			m_pSink->Release();
		}
	} else {
		m_pSink = 0;
	}
	if (m_pName) {
		ourFree(m_pName);
		m_pName = 0;
	}
	if (pName) {
		uint32_t v4 = strlen_w(pName) + 1;
		uint16_t* v5 =
			static_cast<uint16_t*>(ourMalloc(v4 <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * v4 : SIZE_MAX));
		m_pName = v5;
		if (v5) {
			rt::memcpy(v5, pName, sizeof(uint16_t) * v4);
		}
	}
	LeaveCriticalSection(lpCriticalSection);
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseFilter::QueryVendorInfo(uint16_t** pVendorInfo) {
	return E_NOTIMPL;
}

// The DirectShow SDK's AMovieSetupRegisterFilter: the filter's registration removed, then, when bRegister is set, the
// filter, each of its pins and each pin's media types registered with the mapper, the first failure ending it.
// 0x80070002 (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): the filter was not registered) counts as success.
HRESULT WINAPI AMovieSetupRegisterFilterWithMapper(const AMOVIESETUP_FILTER* psetupdata, IFilterMapper* pIFM, int32_t bRegister) {
	if (psetupdata == nullptr) {
		// S_FALSE.
		return 1;
	}
	HRESULT hr = pIFM->UnregisterFilter(*psetupdata->clsID);
	if (bRegister) {
		hr = pIFM->RegisterFilter(*psetupdata->clsID, psetupdata->strName, psetupdata->dwMerit);
		if (hr >= 0) {
			for (uint32_t m1 = 0; m1 < psetupdata->nPins; ++m1) {
				hr = pIFM->RegisterPin(*psetupdata->clsID, psetupdata->lpPin[m1].strName,
					psetupdata->lpPin[m1].bRendered, psetupdata->lpPin[m1].bOutput, psetupdata->lpPin[m1].bZero,
					psetupdata->lpPin[m1].bMany, *psetupdata->lpPin[m1].clsConnectsToFilter,
					psetupdata->lpPin[m1].strConnectsToPin);
				if (hr < 0) {
					break;
				}
				for (uint32_t m2 = 0; m2 < psetupdata->lpPin[m1].nMediaTypes; ++m2) {
					hr = pIFM->RegisterPinType(*psetupdata->clsID, psetupdata->lpPin[m1].strName,
						*psetupdata->lpPin[m1].lpMediaType[m2].clsMajorType,
						*psetupdata->lpPin[m1].lpMediaType[m2].clsMinorType);
					if (hr < 0) {
						break;
					}
				}
				if (hr < 0) {
					break;
				}
			}
		}
	}
	if (hr == static_cast<HRESULT>(0x80070002)) {
		return 0;
	}
	return hr;
}

// The SDK's CBaseFilter::Register (IAMovieSetup), reached through CAsyncReader's table for the IAMovieSetup
// subobject at +0x10 (no adjustor thunk): it takes the subobject as its stack this. S_FALSE (1) without setup data;
// else the filter mapper (CLSID_FilterMapper) registers the setup data, and Register returns S_OK whatever that did.
HRESULT STDMETHODCALLTYPE CBaseFilter::Register() {
	const AMOVIESETUP_FILTER* pSetupData = GetSetupData();
	if (!pSetupData) {
		return 1;
	}
	CoInitialize(nullptr);
	IFilterMapper* pFilterMapper;
	// The SDK's (void**)&pIFM: CoCreateInstance writes the interface pointer through an untyped out-parameter.
	if (CoCreateInstance(&rclsid, nullptr, 1, &CLSID_IFilterMapper, reinterpret_cast<void**>(&pFilterMapper)) >= 0) {
		AMovieSetupRegisterFilterWithMapper(pSetupData, pFilterMapper, 1);
		pFilterMapper->Release();
	}
	CoFreeUnusedLibraries();
	CoUninitialize();
	return 0;
}

// The SDK's CBaseFilter::Unregister, reached the same way: as Register with bRegister 0, returning the result, with
// HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) (0x80070002, a filter never registered) taken as S_OK.
HRESULT STDMETHODCALLTYPE CBaseFilter::Unregister() {
	const AMOVIESETUP_FILTER* pSetupData = GetSetupData();
	if (!pSetupData) {
		return 1;
	}
	CoInitialize(nullptr);
	IFilterMapper* pFilterMapper;
	// The SDK's (void**)&pIFM: CoCreateInstance writes the interface pointer through an untyped out-parameter.
	HRESULT hr = CoCreateInstance(&rclsid, nullptr, 1, &CLSID_IFilterMapper, reinterpret_cast<void**>(&pFilterMapper));
	if (hr >= 0) {
		hr = AMovieSetupRegisterFilterWithMapper(pSetupData, pFilterMapper, 0);
		pFilterMapper->Release();
	}
	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr != static_cast<HRESULT>(0x80070002) ? hr : 0;
}

// The SDK's CBaseFilter constructor; phr is unused.
CBaseFilter::CBaseFilter(const char* apName, IUnknown* apUnk, _RTL_CRITICAL_SECTION* apLock, const GUID& arClsid,
	[[maybe_unused]] HRESULT* apHr)
	: CUnknown(apName, apUnk), m_State(0), m_pClock(nullptr), m_tStart(0), m_clsid(arClsid), m_pLock(apLock),
	  m_pName(nullptr), m_pGraph(nullptr), m_pSink(nullptr), m_PinVersion(1) {}

// The SDK's CBaseFilter destructor (the exe's out-of-line copy is CAsyncReader::sub_9FB550): the name, then the
// clock's reference.
CBaseFilter::~CBaseFilter() {
	ourFree(m_pName);
	if (m_pClock) {
		m_pClock->Release();
		m_pClock = nullptr;
	}
}

// DirectShow's CUnknown(LPCTSTR pName, LPUNKNOWN pUnk): the owner (pUnk, else the object's own nondelegating
// interface) and a zero reference count.
// DirectShow's source has this cast: the two interfaces are unrelated C++ types sharing their first three slots.
CUnknown::CUnknown(const char* apName, IUnknown* apUnk)
	: CBaseObject(apName),
	  m_pUnknown(apUnk ? apUnk : reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this))), m_cRef(0) {}

uint32_t STDMETHODCALLTYPE CUnknown::NonDelegatingAddRef() {
	InterlockedIncrementInt32(&m_cRef);
	uint32_t result = m_cRef;
	if (result <= 1) {
		return 1;
	}
	return result;
}

// The SDK's CUnknown::NonDelegatingRelease: the last release deletes the object (the deleting destructor, slot 3,
// flag 1) after a guard increment; otherwise the count, at least 1. The SDK writes max(ULONG(m_cRef), 1ul): the
// unsigned compare (ja) is its cast.
uint32_t STDMETHODCALLTYPE CUnknown::NonDelegatingRelease() {
	if (!InterlockedDecrementInt32(&m_cRef)) {
		++m_cRef;
		delete this;
		return 0;
	}
	uint32_t uiRef = static_cast<uint32_t>(m_cRef);
	if (uiRef <= 1) {
		return 1;
	}
	return uiRef;
}

HRESULT WINAPI GetInterface(IUnknown* pUnk, void** ppv) {
	if (!ppv) {
		return E_POINTER;
	}
	*ppv = pUnk;
	pUnk->AddRef();
	return 0;
}

// The SDK's CUnknown::NonDelegatingQueryInterface: IUnknown answers with the nondelegating table; anything else
// E_NOINTERFACE. The exe reaches it only by direct calls from its derived classes' overrides.
HRESULT STDMETHODCALLTYPE CUnknown::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
	if (!ppv) {
		return E_POINTER;
	}
	if (IsEqualGUID(riid, CLSID_IUnknown)) {
		// The SDK's GetInterface((LPUNKNOWN)(PNDUNKNOWN)this, ppv): the nondelegating table handed out as IUnknown.
		GetInterface(reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this)), ppv);
		return 0;
	} else {
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
}
