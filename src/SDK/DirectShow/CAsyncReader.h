#pragma once

#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CAsyncIo.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
class CAsyncStream;
class CBaseObject;

// The filter's CLSID: CAsyncReader's constructor passes it to CBaseFilter's, which copies
// its 16 bytes.
extern GUID rdata_1093D70;

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// Constructor 0x9FDDF0 places its lock at +0x50, IO at +0x68, pin at +0xF0 and media type at +0x1A0.
class CAsyncReader : public CBaseFilter {
public:
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
	~CAsyncReader() override;
	HRESULT StreamTime(int64_t& rtStream) override;
	int32_t GetPinVersion() override { return m_PinVersion; }
	const AMOVIESETUP_FILTER* GetSetupData() override;
	int32_t GetPinCount() override;
	CBasePin* GetPin(int32_t n) override;
	// The SDK's CAsyncReader::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);
	// CAsyncOutputPin::Connect calls it through the vtable.
	virtual HRESULT Func0009(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);

	_RTL_CRITICAL_SECTION m_csFilter;
	CAsyncIo m_Io;
	CAsyncOutputPin m_OutputPin;
	CMediaType m_mt;
public:
	CAsyncReader(const char* apName, IUnknown* apUnk, CAsyncStream* a3, HRESULT* a4);
	static void operator delete(void* apMemory);
};
