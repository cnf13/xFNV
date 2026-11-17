#pragma once

#include "Reconstruction.hpp"

// Steam's callback dispatch interface and registration state.
// The PDB has no source path; its location was chosen with the Steam SDK interfaces.
class CCallbackBase {
public:
    // IDA names this overload Run_2; not in the PDB. The Steam SDK's order: the MSVC ABI lays a group of overloads out
    // in reverse, so the vtable has the three-argument Run first, as steam_api.dll expects.
    virtual void Run(void* apData) = 0;
    virtual void Run(void* apData, bool abIOFailure, uint64_t aCall) = 0;
    virtual int32_t GetCallbackSizeBytes() = 0;

    uint8_t m_nCallbackFlags;
    uint8_t padding05[3];
    int32_t m_iCallback;

public:
    CCallbackBase();
    // Not named in the PDB or IDA; name chosen here. SteamAPI_UnregisterCallback(this), unconditionally. void: the
    // exe's callers never read eax after it.
    void Unregister();
};

// steam_api.dll's SteamAPI_RegisterCallback(pCallback, iCallback) (cdecl; the Steam SDK's steam_api.h, void), through
// the exe's import slot. CCallbackBase's registration helpers (0x6FFB20, 0x6FFB90, 0x6FFC20) call it.
using SteamAPI_RegisterCallback_t = void (*)(CCallbackBase* pCallback, int32_t iCallback);
FNVD_IMPORT(SteamAPI_RegisterCallback_t, SteamAPI_RegisterCallback, 0xFDF48C);

// steam_api.dll's SteamAPI_UnregisterCallback(pCallback) (cdecl; steam_api.h, void), through the exe's import slot;
// 0x6FFC00 calls it.
using SteamAPI_UnregisterCallback_t = void (*)(CCallbackBase* pCallback);
FNVD_IMPORT(SteamAPI_UnregisterCallback_t, SteamAPI_UnregisterCallback, 0xFDF47C);
