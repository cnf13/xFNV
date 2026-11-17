#pragma once

#include "SDK/DirectShow/CAsyncReader.h"

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
// The constructor changes only vtables and the inherited media type; it adds no instance fields.
class CBSAReader : public CAsyncReader {
public:
    ~CBSAReader() override;
    // IAMovieSetup::Register through CBSAReader's table for the IAMovieSetup subobject at +0x10
    // (slot 3, no adjustor thunk; stdcall, ret 4, 0x831F70): takes the subobject as its stack this and returns S_OK.
    HRESULT STDMETHODCALLTYPE Register() override;
    HRESULT STDMETHODCALLTYPE Unregister() override;
    CBSAReader(CAsyncStream* a1, const AM_MEDIA_TYPE* a2, HRESULT* a3);
};
