#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "SDK/DirectShow/vfwmsgs.h"

// SDK media format descriptor; CopyFrom copies 0x48 bytes before duplicating pbFormat and AddRef'ing pUnk.
struct AM_MEDIA_TYPE {
    GUID majortype;
    GUID subtype;
    int32_t bFixedSizeSamples;
    int32_t bTemporalCompression;
    uint32_t lSampleSize;
    GUID formattype;
    IUnknown* pUnk;
    uint32_t cbFormat;
    uint8_t* pbFormat;
};

// Not named in the PDB or IDA (IDA calls it AM_MEDIA_TYPE::~AM_MEDIA_TYPE); the DirectShow base classes' name
// (mtype.cpp). Frees mt's format block (CoTaskMemFree) and releases pUnk, clearing cbFormat, pbFormat and pUnk
// (WINAPI: stdcall, ret 4, 0x9F9C20).
void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);

// Not named in the PDB or IDA; the DirectShow base classes' name (mtype.cpp). Copies pmtSource's 0x48 bytes into
// pmtTarget, then a CoTaskMemAlloc'd copy of its format block and an AddRef of pUnk; E_OUTOFMEMORY when the format
// block cannot be had (WINAPI: stdcall, ret 8, 0x9F9BB0).
HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource);

// Not named in the PDB or IDA; the DirectShow base classes' name (mtype.cpp). A CoTaskMemAlloc'd copy of pSrc
// (CopyMediaType), or null (WINAPI: stdcall, ret 4, 0x9F9CD0).
AM_MEDIA_TYPE* WINAPI CreateMediaType(const AM_MEDIA_TYPE* pSrc);

// IDA name; not in the PDB. The PDB has no source path; placed with the DirectShow base classes.
class CMediaType : public AM_MEDIA_TYPE {
public:
    CMediaType();
    ~CMediaType();

public:
    // Copies the AM_MEDIA_TYPE a1 into this one through Set.
    uint32_t* CopyFrom(const AM_MEDIA_TYPE* a1);
    // Not named in the PDB or IDA (IDA calls it AM_MEDIA_TYPE's constructor); the DirectShow
    // base classes' name (mtype.cpp). Zeroes the 0x48 bytes, then sets lSampleSize and bFixedSizeSamples to 1
    // (thiscall, ret 0, 0x9F9A30; eax keeps the 1, which no caller reads). The constructor and
    // CBasePin::ConnectionMediaType call it.
    void InitMediaType();

public:
    int32_t IsPartiallySpecified();
    // Not named in the PDB or IDA; the DirectShow base classes' name (mtype.cpp). TRUE when every GUID ppartial
    // gives (not GUID_NULL) and its format block, when it names a format type, match this one's (thiscall, ret 4).
    int32_t MatchesPartial(const CMediaType* ppartial) const;
    // Not named in the PDB or IDA; the DirectShow base classes' name (mtype.cpp). Frees this type's format block and
    // copies rt into it (CopyMediaType) unless rt is this one; E_OUTOFMEMORY when the copy fails (thiscall, ret 4).
    // The SDK's two overloads have the same code; the exe's callers (0x9F9D10, 0x9F9D50) pass an AM_MEDIA_TYPE.
    HRESULT Set(const AM_MEDIA_TYPE& rt);
    // Set(rt): the SDK's assignment from an AM_MEDIA_TYPE.
    CMediaType& operator=(const AM_MEDIA_TYPE& rt);
};

// guiddef.h's IsEqualGUID, which the DirectShow base classes compile out of line: 1 when the two GUIDs' 16 bytes are
// equal.
int32_t IsEqualGUID(REFGUID rguid1, REFGUID rguid2);

// Inferred type: pv: AM_MEDIA_TYPE* from 0x9fa91d (CMediaSample::m_pMediaType (this of CMediaSample::SetMediaType))
void WINAPI DeleteMediaType(AM_MEDIA_TYPE* pv);
