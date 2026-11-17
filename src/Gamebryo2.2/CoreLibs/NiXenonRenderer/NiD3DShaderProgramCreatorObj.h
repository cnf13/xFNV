#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorAsm.h"

// The shader program creator for precompiled shader objects (.vso and .pso files); registered with
// NiD3DShaderProgramFactory by _SDMInit. It adds no members to NiD3DShaderProgramCreatorAsm and overrides the two
// LoadShaderCode virtuals.
class NiD3DShaderProgramCreatorObj : public NiD3DShaderProgramCreatorAsm {
public:
    NiD3DShaderProgramCreatorObj();
    ~NiD3DShaderProgramCreatorObj() override;

    // bRecoverable is int32_t rather than bool because the caller's whole value is passed on; a bool would be
    // zero-extended.
    bool LoadShaderCodeFromFile(
        const char* pcFileName,
        void*& pvCode,
        uint32_t& uiCodeSize,
        void*& pvConstantData,
        uint32_t& uiConstantSize,
        int32_t bRecoverable
    ) override;
    bool LoadShaderCodeFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        void*& pvCode,
        uint32_t& uiCodeSize,
        void*& pvConstantData,
        uint32_t& uiConstantSize,
        int32_t bRecoverable
    ) override;

    // Creates the creator instance and registers it for "vso" and "pso".
    static void _SDMInit();
};
