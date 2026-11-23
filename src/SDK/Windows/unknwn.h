#pragma once

#include "SDK/Windows/objbase.h"
#include "SDK/Windows/winnt.h"

// The Windows SDK v6.0A COM basics the exe's interfaces build on (guiddef.h's GUID, winerror.h's generic codes,
// unknwn.h's IUnknown; HRESULT is winnt.h's), with the SDK's names and 32-bit layouts, so no Windows header is needed.
// Global, as Windows declares them.

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;
typedef const GUID& REFIID;
typedef const GUID& REFGUID;

// winerror.h's generic COM codes.
constexpr HRESULT S_OK = 0;
constexpr HRESULT S_FALSE = 1;
constexpr HRESULT E_NOTIMPL = static_cast<HRESULT>(0x80004001);
constexpr HRESULT E_NOINTERFACE = static_cast<HRESULT>(0x80004002);
constexpr HRESULT E_POINTER = static_cast<HRESULT>(0x80004003);
constexpr HRESULT E_FAIL = static_cast<HRESULT>(0x80004005);
constexpr HRESULT E_UNEXPECTED = static_cast<HRESULT>(0x8000FFFF);
constexpr HRESULT E_OUTOFMEMORY = static_cast<HRESULT>(0x8007000E);
constexpr HRESULT E_INVALIDARG = static_cast<HRESULT>(0x80070057);

struct IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
    virtual uint32_t STDMETHODCALLTYPE AddRef() = 0;
    virtual uint32_t STDMETHODCALLTYPE Release() = 0;
};
