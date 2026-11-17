#pragma once

#include "SDK/Windows/windef.h"

// The Windows SDK v6.0A <objbase.h> names of a COM method's calling convention: stdcall with `this` pushed last on
// 32-bit Windows, nothing elsewhere. Every method of a COM interface, the SDK's or one the game implements, is
// declared with them.
#if defined(_WIN32) && defined(_M_IX86)
#define STDMETHODCALLTYPE __stdcall
#else
#define STDMETHODCALLTYPE
#endif
#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type, method) virtual type STDMETHODCALLTYPE method
#define STDMETHODIMP HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
