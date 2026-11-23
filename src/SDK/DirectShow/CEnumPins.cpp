#include "SDK/DirectShow/CEnumPins.h"
#include "BSCore/MemoryManager.h"
#include "BSMovie/CRequestList.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include <stdint.h>

struct IUnknown;
class CBasePin;
class CEnumPins;

HRESULT STDMETHODCALLTYPE CEnumPins::QueryInterface(REFIID riid, void** ppv) {
	if (!ppv) {
		return E_POINTER;
	}
	if (IsEqualGUID(riid, CLSID_IEnumPins) || IsEqualGUID(riid, CLSID_IUnknown)) {
		return GetInterface(this, ppv);
	}
	*ppv = nullptr;
	return E_NOINTERFACE;
}

uint32_t STDMETHODCALLTYPE CEnumPins::Release() {
	uint32_t result = InterlockedDecrementInt32(&m_cRef);
	if (!result) {
		if (this) {
			delete this;
			return 0;
		}
	}
	return result;
}

HRESULT STDMETHODCALLTYPE CEnumPins::Reset() {
	int32_t v1 = this->m_pFilter->GetPinVersion();
	CBaseFilter* m_pFilter = this->m_pFilter;
	m_Version = v1;
	m_PinCount = m_pFilter->GetPinCount();
	m_Position = 0;
	m_PinCache.RemoveAll();
	return 0;
}

// The exe's destructor (Destruct, 0x9F9F70): the filter's reference.
CEnumPins::~CEnumPins() {
	static_cast<IUnknown*>(static_cast<IBaseFilter*>(m_pFilter))->Release();
}

void CEnumPins::operator delete(void* apMemory) {
	ourFree(apMemory);
}

HRESULT STDMETHODCALLTYPE CEnumPins::Next(uint32_t cPins, IPin** ppPins, uint32_t* pcFetched) {
	if (!ppPins) {
		return E_POINTER;
	}
	if (pcFetched) {
		*pcFetched = 0;
	} else {
		if (cPins > 1) {
			return E_INVALIDARG;
		}
	}
	uint32_t v10 = 0;
	if (this->m_pFilter->GetPinVersion() != m_Version) {
		Refresh();
	}
	int32_t v5 = m_PinCount - this->m_Position;
	// The SDK's min(cPins, (ULONG)(m_PinCount - m_Position)) compiled to a signed compare (jl at 0x9FBB9E).
	if (v5 >= static_cast<int32_t>(cPins)) {
		v5 = cPins;
	}
	if (!v5) {
		return S_FALSE;
	}
	do {
		int32_t m_Position = this->m_Position;
		if (m_PinCount == m_Position) {
			break;
		}
		CBaseFilter* m_pFilter = this->m_pFilter;
		this->m_Position = m_Position + 1;
		CBasePin* pPin = m_pFilter->GetPin(m_Position);
		if (!pPin) {
			return VFW_E_ENUM_OUT_OF_SYNC;
		}
		if (!m_PinCache.FindI(pPin)) {
			IPin* pIPin = pPin;
			*ppPins = pIPin;
			pIPin->AddRef();
			++v10;
			++ppPins;
			m_PinCache.AddTailI(pPin);
			--v5;
		}
	} while (v5);
	if (pcFetched) {
		*pcFetched = v10;
	}
	return cPins != v10;
}

HRESULT STDMETHODCALLTYPE CEnumPins::Skip(uint32_t cPins) {
	if (m_pFilter->GetPinVersion() != m_Version) {
		return VFW_E_ENUM_OUT_OF_SYNC;
	}
	int32_t m_Position = this->m_Position;
	if (cPins > m_PinCount - m_Position) {
		return 1;
	}
	this->m_Position = cPins + m_Position;
	return 0;
}

// The SDK's CEnumPins::Clone: a copy of this enumerator, VFW_E_ENUM_OUT_OF_SYNC when the filter's pins changed,
// E_OUTOFMEMORY when the allocation fails. The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CEnumPins::Clone(IEnumPins** ppEnum) {
	HRESULT hr = S_OK;
	if (!ppEnum) {
		return E_POINTER;
	}
	if (m_pFilter->GetPinVersion() == m_Version) {
		CEnumPins* pMemory = static_cast<CEnumPins*>(ourMalloc(sizeof(CEnumPins)));
		CEnumPins* pEnum;
		if (pMemory) {
			pEnum = ::new (pMemory) CEnumPins(m_pFilter, this);
		} else {
			pEnum = nullptr;
		}
		*ppEnum = pEnum;
		if (!pEnum) {
			hr = E_OUTOFMEMORY;
		}
	} else {
		*ppEnum = nullptr;
		hr = VFW_E_ENUM_OUT_OF_SYNC;
	}
	return hr;
}

CEnumPins::CEnumPins(CBaseFilter* apFilter, CEnumPins* apEnumPins)
	: m_Position(0), m_PinCount(0), m_pFilter(apFilter), m_cRef(1), m_PinCache(nullptr) {
	static_cast<IUnknown*>(static_cast<IBaseFilter*>(m_pFilter))->AddRef();
	if (apEnumPins) {
		m_Position = apEnumPins->m_Position;
		m_PinCount = apEnumPins->m_PinCount;
		m_Version = apEnumPins->m_Version;
		m_PinCache.AddTail(&apEnumPins->m_PinCache);
	} else {
		m_Version = m_pFilter->GetPinVersion();
		m_PinCount = m_pFilter->GetPinCount();
	}
}

uint32_t STDMETHODCALLTYPE CEnumPins::AddRef() {
	return InterlockedIncrementInt32(&m_cRef);
}

HRESULT CEnumPins::Refresh() {
	m_Version = m_pFilter->GetPinVersion();
	m_PinCount = m_pFilter->GetPinCount();
	m_Position = 0;
	return S_OK;
}
