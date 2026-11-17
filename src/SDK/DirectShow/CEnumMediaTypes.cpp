#include "SDK/DirectShow/CEnumMediaTypes.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CUnknown.h"
#include <stdint.h>

struct AM_MEDIA_TYPE;
struct IUnknown;

HRESULT STDMETHODCALLTYPE CEnumMediaTypes::QueryInterface(REFIID riid, void** ppv) {
	if (!ppv) {
		return E_POINTER;
	}
	if (IsEqualGUID(riid, CLSID_IEnumMediaTypes) || IsEqualGUID(riid, CLSID_IUnknown)) {
		return GetInterface(this, ppv);
	}
	*ppv = nullptr;
	return E_NOINTERFACE;
}

uint32_t STDMETHODCALLTYPE CEnumMediaTypes::Release() {
	uint32_t result = InterlockedDecrementInt32(&m_cRef);
	if (!result) {
		if (this) {
			delete this;
			return 0;
		}
	}
	return result;
}

HRESULT STDMETHODCALLTYPE CEnumMediaTypes::Reset() {
	CBasePin* m_pPin = this->m_pPin;
	m_Position = 0;
	m_Version = m_pPin->GetMediaTypeVersion();
	return 0;
}

// The exe's destructor (Destruct, 0x9FA100): the pin's reference.
CEnumMediaTypes::~CEnumMediaTypes() {
	static_cast<IUnknown*>(static_cast<IPin*>(m_pPin))->Release();
}

void CEnumMediaTypes::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CEnumMediaTypes::Next(uint32_t cTypes, AM_MEDIA_TYPE** ppTypes, uint32_t* pcFetched) {
	CMediaType apMemory;
	AM_MEDIA_TYPE** v4 = ppTypes;
	CEnumMediaTypes* v5 = this;
	AM_MEDIA_TYPE** v13 = ppTypes;
	if (!ppTypes) {
		return E_POINTER;
	}
	if (this->m_pPin->GetMediaTypeVersion() != m_Version) {
		return VFW_E_ENUM_OUT_OF_SYNC;
	}
	uint32_t v7 = cTypes;
	if (pcFetched) {
		*pcFetched = 0;
	} else {
		if (cTypes > 1) {
			return E_INVALIDARG;
		}
	}
	uint32_t v14 = 0;
	if (cTypes) {
		while (1) {
			apMemory.~CMediaType();
			::new (&apMemory) CMediaType;
			int32_t m_Position = v5->m_Position;
			v5->m_Position = m_Position + 1;
			CBasePin* m_pPin = v5->m_pPin;
			int32_t v12 = m_Position;
			if (m_pPin->GetMediaType(v12, &apMemory)) {
				break;
			}
			AM_MEDIA_TYPE* v11 = static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
			*v4 = v11;
			if (!v11) {
				goto LABEL_17;
			}
			++v13;
			++v14;
			// The SDK's *ppMediaTypes[n] = mt (a rep movsd of the whole AM_MEDIA_TYPE), then clears the local's
			// pbFormat, cbFormat and pUnk, in that order, so that its destructor frees nothing the copy now owns.
			*v11 = apMemory;
			apMemory.pbFormat = nullptr;
			apMemory.cbFormat = 0;
			apMemory.pUnk = nullptr;
			--v7;
			if (!v7) {
				goto LABEL_18;
			}
			v4 = v13;
			v5 = this;
		}
	LABEL_17:;
	}
LABEL_18:;
	if (pcFetched) {
		*pcFetched = v14;
	}
	return v7 != 0;
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CEnumMediaTypes::Skip(uint32_t cTypes) {
	if (!cTypes) {
		return 0;
	}
	if (this->m_pPin->GetMediaTypeVersion() != m_Version) {
		return VFW_E_ENUM_OUT_OF_SYNC;
	}
	m_Position += cTypes;
	FNVD_CONSTRUCTED_LOCAL(CMediaType, apMemory, &CMediaType::Destruct);
	CBasePin* m_pPin = this->m_pPin;
	int32_t v6 = m_Position - 1;
	int32_t v5 = m_pPin->GetMediaType(v6, &apMemory) != 0;
	return v5;
}

CEnumMediaTypes::CEnumMediaTypes(CBasePin* apPin, CEnumMediaTypes* apEnumMediaTypes)
	: m_Position(0), m_pPin(apPin), m_cRef(1) {
	static_cast<IUnknown*>(static_cast<IPin*>(apPin))->AddRef();
	if (apEnumMediaTypes) {
		m_Position = apEnumMediaTypes->m_Position;
		m_Version = apEnumMediaTypes->m_Version;
	} else {
		m_Version = m_pPin->GetMediaTypeVersion();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
HRESULT STDMETHODCALLTYPE CEnumMediaTypes::Clone(IEnumMediaTypes** ppEnum) {
	int32_t v2 = 0;
	if (!ppEnum) {
		return E_POINTER;
	}
	if (m_pPin->GetMediaTypeVersion() == m_Version) {
		CEnumMediaTypes* v5;
		CEnumMediaTypes* v4 = static_cast<CEnumMediaTypes*>(ourMalloc(sizeof(CEnumMediaTypes)));
		if (v4) {
			v5 = ::new (v4) CEnumMediaTypes(m_pPin, this);
		} else {
			v5 = 0;
		}
		*ppEnum = v5;
		if (!v5) {
			return E_OUTOFMEMORY;
		}
	} else {
		*ppEnum = 0;
		return VFW_E_ENUM_OUT_OF_SYNC;
	}
	return v2;
}

uint32_t STDMETHODCALLTYPE CEnumMediaTypes::AddRef() {
	return InterlockedIncrementInt32(&m_cRef);
}
