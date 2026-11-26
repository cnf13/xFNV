#pragma once

#include "BSCore/NewOverloads.h"

struct IDirect3DQuery9;

// A hardware occlusion query: counts the pixels that pass the depth test.
class BSOcclusionQuery {
public:
    BSOcclusionQuery();
    ~BSOcclusionQuery();

    // Reads the query's result into pResult; true when it is ready. Only the flag's byte is tested, and
    // D3DGETDATA_FLUSH is passed when it is set, so bool fits.
    bool GetResult(uint32_t* pResult, bool bFlush);
    bool Begin();
    void End();
    bool GetResultNoWait(uint32_t* pResult, bool bFlush);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    IDirect3DQuery9* pQuery;

    static inline char bQueryInProgress;
};
