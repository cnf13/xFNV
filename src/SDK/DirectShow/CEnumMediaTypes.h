#pragma once

#include "SDK/DirectShow/CMediaType.h"

class CBasePin;

struct IEnumMediaTypes : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Next(uint32_t cTypes, AM_MEDIA_TYPE** ppTypes, uint32_t* pcFetched) = 0;
	virtual HRESULT STDMETHODCALLTYPE Skip(uint32_t cTypes) = 0;
	virtual HRESULT STDMETHODCALLTYPE Reset() = 0;
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes** ppEnum) = 0;
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// Constructor stores: position +4, pin +8, version +0xC, refcount +0x10.
class CEnumMediaTypes : public IEnumMediaTypes {
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE AddRef() override;
	uint32_t STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE Next(uint32_t cTypes, AM_MEDIA_TYPE** ppTypes, uint32_t* pcFetched) override;
	HRESULT STDMETHODCALLTYPE Skip(uint32_t cTypes) override;
	HRESULT STDMETHODCALLTYPE Reset() override;
	HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes** ppEnum) override;
	virtual ~CEnumMediaTypes();

	int32_t m_Position;
	CBasePin* m_pPin;
	int32_t m_Version;
	int32_t m_cRef;
public:
	CEnumMediaTypes(CBasePin* apPin, CEnumMediaTypes* apEnumMediaTypes);
	static void operator delete(void* apMemory);
};
