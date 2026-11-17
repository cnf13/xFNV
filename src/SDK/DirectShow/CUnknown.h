#pragma once

#include "BSMovie/CBaseObject.h"
#include "SDK/DirectShow/CMediaType.h"

// vfwmsgs.h: an enumerator whose filter or pin changed its set since the enumerator was made.
constexpr HRESULT VFW_E_ENUM_OUT_OF_SYNC = static_cast<HRESULT>(0x80040203);

// SDK nondelegating IUnknown interface. These methods, unlike ordinary C++ virtuals, use stdcall.
struct INonDelegatingUnknown {
	virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) = 0;
	virtual uint32_t STDMETHODCALLTYPE NonDelegatingAddRef() = 0;
	virtual uint32_t STDMETHODCALLTYPE NonDelegatingRelease() = 0;
};

// DirectShow's CUnknown; the PDB has no source path for it, so it is placed with the DirectShow base classes.
// RTTI places the empty CBaseObject at +4, sharing the owner pointer's offset. The constructor (0x9FCEE0) stores
// the owner (pUnk, else this) at +4 and clears the reference count at +8.
class CUnknown : public INonDelegatingUnknown, public CBaseObject {
public:
	// The DirectShow base classes' name; not in the PDB or IDA (0x9FCF10, stdcall, ret 12). In no exe vtable: the
	// derived classes' overrides call it directly for anything they do not answer.
	HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
	uint32_t STDMETHODCALLTYPE NonDelegatingAddRef() override;
	uint32_t STDMETHODCALLTYPE NonDelegatingRelease() override;
	virtual ~CUnknown();

	CUnknown(const char* apName, IUnknown* apUnk);

	IUnknown* m_pUnknown;
	int32_t m_cRef;
};

// The DirectShow base classes' GetInterface: stores pUnk in *ppv
// and AddRefs it; E_POINTER without ppv.
HRESULT WINAPI GetInterface(IUnknown* pUnk, void** ppv);
