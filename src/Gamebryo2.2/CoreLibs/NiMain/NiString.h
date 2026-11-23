#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

// A reference-counted mutable string; the allocation header precedes its character handle.
class NiString : public NiMemObject {
public:
    NiString(const char* pcString);
    ~NiString();

    bool ToFloat(float& f);

    // The allocation size for a request of astSize bytes.
    static size_t GetBestBufferSize(size_t uiReqSize);
    // A fresh handle for a string of stStrLength characters (at least one): refcount 1, length 0.
    static char* Allocate(size_t stStrLength);
    // A fresh handle holding a copy of pcString (refcount 1, length set), or null for a null pcString.
    static char* AllocateAndCopy(const char* pcString);

    char* m_kHandle;
};
