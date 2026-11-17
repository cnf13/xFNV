#pragma once

#include "SDK/DirectShow/CMediaType.h"
#include <stdint.h>

class CBaseAllocator;

// DirectShow's IID_IMediaSample2 (strmif.h); the bytes at 0x1093BCC are {36b73884-c2c8-11cf-8b46-00805f6cef60}.
extern GUID IID_IMediaSample2;
// DirectShow's IID_IMediaSample (strmif.h); the bytes at 0x1093BDC are {56a8689a-0ad4-11ce-b03a-0020af0ba770}.
extern GUID IID_IMediaSample;

struct IMediaSample : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE GetPointer(uint8_t** ppBuffer) = 0;
	virtual int32_t STDMETHODCALLTYPE GetSize() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTime(int64_t* pStart, int64_t* pEnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTime(int64_t* pStart, int64_t* pEnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsSyncPoint() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSyncPoint(int32_t bSync) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsPreroll() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPreroll(int32_t bPreroll) = 0;
	virtual int32_t STDMETHODCALLTYPE GetActualDataLength() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetActualDataLength(int32_t lActual) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE** ppType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE* pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsDiscontinuity() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDiscontinuity(int32_t bDiscontinuity) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMediaTime(int64_t* pStart, int64_t* pEnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaTime(int64_t* pStart, int64_t* pEnd) = 0;
};
struct IMediaSample2 : IMediaSample {
	virtual HRESULT STDMETHODCALLTYPE GetProperties(uint32_t cbProperties, uint8_t* pbProperties) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetProperties(uint32_t cbProperties, const uint8_t* pbProperties) = 0;
};

// DirectShow's AM_SAMPLE2_PROPERTIES (strmif.h): the block IMediaSample2::GetProperties and SetProperties copy.
struct AM_SAMPLE2_PROPERTIES {
	uint32_t cbData;
	uint32_t dwTypeSpecificFlags;
	uint32_t dwSampleFlags;
	int32_t lActual;
	int64_t tStart;
	int64_t tStop;
	uint32_t dwStreamId;
	AM_MEDIA_TYPE* pMediaType;
	uint8_t* pbBuffer;
	int32_t cbBuffer;
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// The SDK member order agrees with the exe's constructor and sample-property accesses.
class CMediaSample : public IMediaSample2 {
public:
	// DirectShow's sample flags (amfilter.h), the bits of m_dwFlags.
	enum {
		Sample_SyncPoint = 0x01,
		Sample_Preroll = 0x02,
		Sample_Discontinuity = 0x04,
		Sample_TypeChanged = 0x08,
		Sample_TimeValid = 0x10,
		Sample_MediaTimeValid = 0x20,
		Sample_TimeDiscontinuity = 0x40,
		Sample_StopValid = 0x100,
		Sample_ValidFlags = 0x1FF
	};

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE AddRef() override;
	uint32_t STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE GetPointer(uint8_t** ppBuffer) override;
	int32_t STDMETHODCALLTYPE GetSize() override;
	HRESULT STDMETHODCALLTYPE GetTime(int64_t* pStart, int64_t* pEnd) override;
	HRESULT STDMETHODCALLTYPE SetTime(int64_t* pStart, int64_t* pEnd) override;
	HRESULT STDMETHODCALLTYPE IsSyncPoint() override;
	HRESULT STDMETHODCALLTYPE SetSyncPoint(int32_t bSync) override;
	HRESULT STDMETHODCALLTYPE IsPreroll() override;
	HRESULT STDMETHODCALLTYPE SetPreroll(int32_t bPreroll) override;
	int32_t STDMETHODCALLTYPE GetActualDataLength() override;
	HRESULT STDMETHODCALLTYPE SetActualDataLength(int32_t lActual) override;
	HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE** ppType) override;
	HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE* pType) override;
	HRESULT STDMETHODCALLTYPE IsDiscontinuity() override;
	HRESULT STDMETHODCALLTYPE SetDiscontinuity(int32_t bDiscontinuity) override;
	HRESULT STDMETHODCALLTYPE GetMediaTime(int64_t* pStart, int64_t* pEnd) override;
	HRESULT STDMETHODCALLTYPE SetMediaTime(int64_t* pStart, int64_t* pEnd) override;
	HRESULT STDMETHODCALLTYPE GetProperties(uint32_t cbProperties, uint8_t* pbProperties) override;
	HRESULT STDMETHODCALLTYPE SetProperties(uint32_t cbProperties, const uint8_t* pbProperties) override;
	virtual ~CMediaSample();

	uint32_t m_dwFlags;
	uint32_t m_dwTypeSpecificFlags;
	uint8_t* m_pBuffer;
	int32_t m_lActual;
	int32_t m_cbBuffer;
	CBaseAllocator* m_pAllocator;
	CMediaSample* m_pNext;
	int64_t m_Start;
	int64_t m_End;
	int64_t m_MediaStart;
	int32_t m_MediaEnd;
	AM_MEDIA_TYPE* m_pMediaType;
	uint32_t m_dwStreamId;
	int32_t m_cRef;
public:
	CMediaSample(const char* apName, CBaseAllocator* apAllocator, HRESULT* aphr, uint8_t* apBuffer, int32_t alLength);
	static void operator delete(void* apMemory);
};
