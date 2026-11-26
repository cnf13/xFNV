#pragma once

#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/CRequestList.h"

class CBaseFilter;
struct IPin;

struct IEnumPins : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Next(uint32_t cPins, IPin** ppPins, uint32_t* pcFetched) = 0;
    virtual HRESULT STDMETHODCALLTYPE Skip(uint32_t cPins) = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset() = 0;
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumPins** ppEnum) = 0;
};

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// The constructor initializes the six leading words and the 0x18-byte pin cache at +0x18.
class CEnumPins : public IEnumPins {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    uint32_t STDMETHODCALLTYPE AddRef() override;
    uint32_t STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE Next(uint32_t cPins, IPin** ppPins, uint32_t* pcFetched) override;
    HRESULT STDMETHODCALLTYPE Skip(uint32_t cPins) override;
    HRESULT STDMETHODCALLTYPE Reset() override;
    HRESULT STDMETHODCALLTYPE Clone(IEnumPins** ppEnum) override;
    virtual ~CEnumPins();
    CEnumPins(CBaseFilter* apFilter, CEnumPins* apEnumPins);
    static void operator delete(void* apMemory);

    int32_t m_Position;
    int32_t m_PinCount;
    CBaseFilter* m_pFilter;
    int32_t m_Version;
    int32_t m_cRef;
    CRequestList m_PinCache;

public:
    // Takes the filter's pin version and count again and starts over.
    HRESULT Refresh();
};
