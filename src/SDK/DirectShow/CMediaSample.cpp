#include "SDK/DirectShow/CMediaSample.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CMemAllocator.h"
#include "SDK/DirectShow/CUnknown.h"
#include <stdint.h>

GUID IID_IMediaSample2{0x36B73884, 49864, 4559, {139, 70, 0, 128, 95, 108, 239, 96}};
GUID IID_IMediaSample{0x56A8689A, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};

struct AM_MEDIA_TYPE;

// DirectShow's CMediaSample::QueryInterface (amfilter.cpp); the exe leaves *ppv alone on E_NOINTERFACE.
HRESULT STDMETHODCALLTYPE CMediaSample::QueryInterface(REFIID riid, void** ppv) {
	if (IsEqualGUID(riid, IID_IMediaSample) || IsEqualGUID(riid, IID_IMediaSample2)
		|| IsEqualGUID(riid, CLSID_IUnknown)) {
		return GetInterface(this, ppv);
	} else {
		// E_NOINTERFACE
		return static_cast<HRESULT>(0x80004002);
	}
}

uint32_t STDMETHODCALLTYPE CMediaSample::Release() {
	uint32_t v1;
	if (m_cRef == 1) {
		v1 = 0;
		m_cRef = 0;
	} else {
		uint32_t result = InterlockedDecrementInt32(&m_cRef);
		v1 = result;
		if (result) {
			return result;
		}
	}
	if ((m_dwFlags & 8) != 0) {
		SetMediaType(0);
	}
	IMemAllocatorCallbackTemp* v3 = static_cast<IMemAllocatorCallbackTemp*>(m_pAllocator);
	m_dwFlags = 0;
	m_dwTypeSpecificFlags = 0;
	m_dwStreamId = 0;
	v3->ReleaseBuffer(this);
	return v1;
}

HRESULT STDMETHODCALLTYPE CMediaSample::GetPointer(uint8_t** ppBuffer) {
	*ppBuffer = m_pBuffer;
	return 0;
}

int32_t STDMETHODCALLTYPE CMediaSample::GetSize() {
	return m_cbBuffer;
}

HRESULT STDMETHODCALLTYPE CMediaSample::GetTime(int64_t* pStart, int64_t* pEnd) {
	uint32_t m_dwFlags = this->m_dwFlags;
	if ((m_dwFlags & 256) != 0) {
		*pStart = m_Start;
		*pEnd = m_End;
		return 0;
	} else {
		if ((m_dwFlags & 16) != 0) {
			*pStart = m_Start;
			*pEnd = m_Start + 1;
			return VFW_S_NO_STOP_TIME;
		} else {
			return VFW_E_SAMPLE_TIME_NOT_SET;
		}
	}
}

// DirectShow's CMediaSample::SetTime (amfilter.cpp).
HRESULT STDMETHODCALLTYPE CMediaSample::SetTime(int64_t* pStart, int64_t* pEnd) {
	if (pStart) {
		if (pEnd) {
			m_Start = *pStart;
			m_End = *pEnd;
			m_dwFlags |= Sample_TimeValid | Sample_StopValid;
		} else {
			m_Start = *pStart;
			m_dwFlags = (m_dwFlags & ~(Sample_TimeValid | Sample_StopValid)) | Sample_TimeValid;
		}
		return 0;
	} else {
		m_dwFlags &= ~(Sample_TimeValid | Sample_StopValid);
		return 0;
	}
}

HRESULT STDMETHODCALLTYPE CMediaSample::GetMediaTime(int64_t* pStart, int64_t* pEnd) {
	if ((m_dwFlags & 32) == 0) {
		return VFW_E_MEDIA_TIME_NOT_SET;
	}
	*pStart = m_MediaStart;
	*pEnd = m_MediaStart + m_MediaEnd;
	return 0;
}

// DirectShow's CMediaSample::SetMediaTime (amfilter.cpp): pEnd is not tested, and the length is the low dword of the
// difference.
HRESULT STDMETHODCALLTYPE CMediaSample::SetMediaTime(int64_t* pStart, int64_t* pEnd) {
	if (pStart) {
		m_MediaStart = *pStart;
		m_MediaEnd = static_cast<int32_t>(*pEnd - *pStart);
		m_dwFlags |= Sample_MediaTimeValid;
	} else {
		m_dwFlags &= ~Sample_MediaTimeValid;
	}
	return 0;
}

HRESULT STDMETHODCALLTYPE CMediaSample::IsSyncPoint() {
	return (m_dwFlags & 1) == 0;
}

HRESULT STDMETHODCALLTYPE CMediaSample::SetSyncPoint(int32_t bSync) {
	if (bSync) {
		m_dwFlags |= 1u;
	} else {
		m_dwFlags &= ~1u;
	}
	return 0;
}

HRESULT STDMETHODCALLTYPE CMediaSample::IsDiscontinuity() {
	return static_cast<uint8_t>(~static_cast<uint8_t>(m_dwFlags)) >> 2 & 1;
}

HRESULT STDMETHODCALLTYPE CMediaSample::SetDiscontinuity(int32_t bDiscontinuity) {
	if (bDiscontinuity) {
		m_dwFlags |= 4u;
	} else {
		m_dwFlags &= ~4u;
	}
	return 0;
}

HRESULT STDMETHODCALLTYPE CMediaSample::IsPreroll() {
	return static_cast<uint8_t>(~static_cast<uint8_t>(m_dwFlags)) >> 1 & 1;
}

HRESULT STDMETHODCALLTYPE CMediaSample::SetPreroll(int32_t bPreroll) {
	if (bPreroll) {
		m_dwFlags |= 2u;
	} else {
		m_dwFlags &= ~2u;
	}
	return 0;
}

int32_t STDMETHODCALLTYPE CMediaSample::GetActualDataLength() {
	return m_lActual;
}

HRESULT STDMETHODCALLTYPE CMediaSample::SetActualDataLength(int32_t lActual) {
	if (lActual > m_cbBuffer) {
		return VFW_E_BUFFER_OVERFLOW;
	}
	m_lActual = lActual;
	return 0;
}

HRESULT STDMETHODCALLTYPE CMediaSample::GetMediaType(AM_MEDIA_TYPE** ppType) {
	if ((m_dwFlags & 8) != 0) {
		AM_MEDIA_TYPE* v3 = CreateMediaType(m_pMediaType);
		*ppType = v3;
		return v3 != 0 ? 0 : E_OUTOFMEMORY;
	} else {
		*ppType = 0;
		return 1;
	}
}

// A null pType returns its own value, 0 (S_OK), as the exe does.
HRESULT STDMETHODCALLTYPE CMediaSample::SetMediaType(AM_MEDIA_TYPE* pType) {
	if (m_pMediaType) {
		DeleteMediaType(m_pMediaType);
		m_pMediaType = 0;
	}
	if (pType) {
		AM_MEDIA_TYPE* v3 = CreateMediaType(pType);
		m_pMediaType = v3;
		if (v3) {
			m_dwFlags |= 8u;
			return 0;
		} else {
			m_dwFlags &= ~8u;
			return E_OUTOFMEMORY;
		}
	} else {
		m_dwFlags &= ~8u;
	}
	return 0;
}

// DirectShow's CMediaSample::GetProperties (amfilter.cpp): the whole AM_SAMPLE2_PROPERTIES is built on the stack and
// its first cbData bytes copied out.
HRESULT STDMETHODCALLTYPE CMediaSample::GetProperties(uint32_t cbProperties, uint8_t* pbProperties) {
	if (cbProperties) {
		if (!pbProperties) {
			// E_POINTER
			return static_cast<HRESULT>(0x80004003);
		}
		AM_SAMPLE2_PROPERTIES Props;
		Props.cbData = cbProperties < sizeof(Props) ? cbProperties : sizeof(Props);
		Props.dwSampleFlags = m_dwFlags & ~Sample_MediaTimeValid;
		Props.dwTypeSpecificFlags = m_dwTypeSpecificFlags;
		Props.pbBuffer = m_pBuffer;
		Props.cbBuffer = m_cbBuffer;
		Props.lActual = m_lActual;
		Props.tStart = m_Start;
		Props.tStop = m_End;
		Props.dwStreamId = m_dwStreamId;
		if (m_dwFlags & Sample_TypeChanged) {
			Props.pMediaType = m_pMediaType;
		} else {
			Props.pMediaType = nullptr;
		}
		rt::memcpy(pbProperties, &Props, Props.cbData);
	}
	return 0;
}

// DirectShow's CMediaSample::SetProperties (amfilter.cpp): every check first, then the assignments; a field counts
// when cbProperties covers it whole.
HRESULT STDMETHODCALLTYPE CMediaSample::SetProperties(uint32_t cbProperties, const uint8_t* pbProperties) {
	AM_MEDIA_TYPE* pMediaType = nullptr;
	if (cbProperties < offsetof(AM_SAMPLE2_PROPERTIES, cbData) + sizeof(uint32_t)) {
		return 0;
	}
	if (!pbProperties) {
		// E_POINTER
		return static_cast<HRESULT>(0x80004003);
	}
	// The SDK's own cast: the caller's property bytes are an AM_SAMPLE2_PROPERTIES.
	const AM_SAMPLE2_PROPERTIES* pProps = reinterpret_cast<const AM_SAMPLE2_PROPERTIES*>(pbProperties);
	if (pProps->cbData < cbProperties) {
		cbProperties = pProps->cbData;
	}
	if (cbProperties > sizeof(*pProps) || pProps->cbData > sizeof(*pProps)) {
		// E_INVALIDARG
		return static_cast<HRESULT>(0x80070057);
	}
	if (cbProperties >= 12) {
		if ((pProps->dwSampleFlags & (~Sample_ValidFlags | Sample_MediaTimeValid)) != 0
			|| ((pProps->dwSampleFlags & Sample_TimeValid) != 0 && (m_dwFlags & Sample_TimeValid) == 0
				&& cbProperties < 32)) {
			return static_cast<HRESULT>(0x80070057);
		}
	}
	if (cbProperties >= 44) {
		if (pProps->pbBuffer && pProps->pbBuffer != m_pBuffer) {
			return static_cast<HRESULT>(0x80070057);
		}
	}
	if (cbProperties >= 48) {
		int32_t cbBuffer = pProps->cbBuffer;
		if (cbBuffer && cbBuffer != m_cbBuffer) {
			return static_cast<HRESULT>(0x80070057);
		}
		if (cbBuffer < pProps->lActual) {
			return static_cast<HRESULT>(0x80070057);
		}
	}
	if (cbProperties >= 40 && (pProps->dwSampleFlags & Sample_TypeChanged) != 0) {
		if (!pProps->pMediaType) {
			return static_cast<HRESULT>(0x80004003);
		}
		pMediaType = CreateMediaType(pProps->pMediaType);
		if (!pMediaType) {
			// E_OUTOFMEMORY
			return static_cast<HRESULT>(0x8007000E);
		}
	}
	if (cbProperties >= 36) {
		m_dwStreamId = pProps->dwStreamId;
	}
	if (cbProperties < 12) {
		if (cbProperties >= 8) {
			m_dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
		}
	} else {
		m_dwFlags = pProps->dwSampleFlags | (m_dwFlags & Sample_MediaTimeValid);
		m_dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
	}
	if (cbProperties >= 16) {
		m_lActual = pProps->lActual;
	}
	if (cbProperties >= 32) {
		m_End = pProps->tStop;
	}
	if (cbProperties >= 24) {
		m_Start = pProps->tStart;
	}
	if (cbProperties >= 40 && (pProps->dwSampleFlags & Sample_TypeChanged) != 0) {
		if (m_pMediaType) {
			DeleteMediaType(m_pMediaType);
		}
		m_pMediaType = pMediaType;
	}
	if (m_pMediaType) {
		m_dwFlags |= Sample_TypeChanged;
		return 0;
	}
	m_dwFlags &= ~Sample_TypeChanged;
	return 0;
}

// The exe's destructor (Destruct, 0x9FA560): the sample's media type, when it has one.
CMediaSample::~CMediaSample() {
	if (m_pMediaType) {
		DeleteMediaType(m_pMediaType);
	}
}

void CMediaSample::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// DirectShow's CMediaSample constructor (amfilter.cpp): apName and aphr are unused; m_pNext and the times are left
// as they are.
CMediaSample::CMediaSample([[maybe_unused]] const char* apName, CBaseAllocator* apAllocator,
	[[maybe_unused]] HRESULT* aphr, uint8_t* apBuffer, int32_t alLength)
	: m_dwFlags(0), m_dwTypeSpecificFlags(0), m_pBuffer(apBuffer), m_lActual(alLength), m_cbBuffer(alLength),
	  m_pAllocator(apAllocator), m_pMediaType(nullptr), m_dwStreamId(0), m_cRef(0) {}

uint32_t STDMETHODCALLTYPE CMediaSample::AddRef() {
	return InterlockedIncrementInt32(&m_cRef);
}
