#include "SDK/DirectShow/CMediaType.h"
#include "BSAudio/DirectSound.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include <stdint.h>
#include <cstring>

struct AM_MEDIA_TYPE;
class CMediaType;
struct IUnknown;

void CMediaType::InitMediaType() {
	std::memset(this, 0, sizeof(*this));
	lSampleSize = 1;
	bFixedSizeSamples = 1;
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt) {
	if (mt.cbFormat) {
		CoTaskMemFree(mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = 0;
	}
	IUnknown* pUnk = mt.pUnk;
	if (pUnk) {
		mt.pUnk->Release();
		mt.pUnk = 0;
	}
}

uint32_t* CMediaType::CopyFrom(const AM_MEDIA_TYPE* a1) {
	*this = *a1;
	return &majortype.Data1;
}

CMediaType::~CMediaType() {
	FreeMediaType(*this);
}

CMediaType::CMediaType() {
	InitMediaType();
}

int32_t CMediaType::IsPartiallySpecified() {
	if (IsEqualGUID(majortype, GUID_NULL)) {
		return 1;
	}
	int result = IsEqualGUID(formattype, GUID_NULL);
	if (result) {
		return 1;
	}
	return result;
}

// The DirectShow base classes' CMediaType::MatchesPartial (mtype.cpp). The exe inlines memcmp's equality test as the
// CRT's intrinsic form (dwords, then the bytes left); only whether the blocks are equal is used, so src compares the
// bytes in order.
int32_t CMediaType::MatchesPartial(const CMediaType* ppartial) const {
	if (!IsEqualGUID(ppartial->majortype, GUID_NULL)
		&& !IsEqualGUID(majortype, ppartial->majortype)) {
		return 0;
	}
	if (!IsEqualGUID(ppartial->subtype, GUID_NULL)
		&& !IsEqualGUID(subtype, ppartial->subtype)) {
		return 0;
	}
	if (IsEqualGUID(ppartial->formattype, GUID_NULL)) {
		return 1;
	}
	if (!IsEqualGUID(formattype, ppartial->formattype)) {
		return 0;
	}
	uint32_t uiSize = cbFormat;
	if (uiSize != ppartial->cbFormat) {
		return 0;
	}
	if (!uiSize) {
		return 1;
	}
	// EXE-INLINED none: memcmp
	const uint8_t* pucFormat = pbFormat;
	const uint8_t* pucPartial = ppartial->pbFormat;
	for (uint32_t i = 0; i < uiSize; ++i) {
		if (pucFormat[i] != pucPartial[i]) {
			return 0;
		}
	}
	return 1;
}

// The DirectShow base classes' CMediaType::Set (mtype.cpp).
HRESULT CMediaType::Set(const AM_MEDIA_TYPE& rt) {
	if (&rt == this) {
		return 0;
	}
	FreeMediaType(*this);
	if (CopyMediaType(this, &rt) >= 0) {
		return 0;
	}
	return E_OUTOFMEMORY;
}

// The exe inlines memcmp's equality test; only equality reaches the result.
int32_t IsEqualGUID(REFGUID rguid1, REFGUID rguid2) {
	return std::memcmp(&rguid1, &rguid2, sizeof(GUID)) == 0;
}

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource) {
	rt::memcpy(pmtTarget, pmtSource, sizeof(AM_MEDIA_TYPE));
	if (pmtSource->cbFormat) {
		uint8_t* pbFormat = static_cast<uint8_t*>(CoTaskMemAlloc(pmtSource->cbFormat));
		pmtTarget->pbFormat = pbFormat;
		if (!pbFormat) {
			pmtTarget->cbFormat = 0;
			return E_OUTOFMEMORY;
		}
		rt::memcpy(pbFormat, pmtSource->pbFormat, pmtTarget->cbFormat);
	}
	IUnknown* pUnk = pmtTarget->pUnk;
	if (pUnk) {
		pUnk->AddRef();
	}
	return 0;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE* pv) {
	if (pv) {
		FreeMediaType(*pv);
		CoTaskMemFree(pv);
	}
}

AM_MEDIA_TYPE* WINAPI CreateMediaType(const AM_MEDIA_TYPE* pSrc) {
	void* v1 = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
	if (!v1) {
		return 0;
	}
	if (CopyMediaType(static_cast<AM_MEDIA_TYPE*>(v1), pSrc) < 0) {
		CoTaskMemFree(v1);
		return 0;
	}
	return static_cast<AM_MEDIA_TYPE*>(v1);
}

CMediaType& CMediaType::operator=(const AM_MEDIA_TYPE& rt) {
	Set(rt);
	return *this;
}
