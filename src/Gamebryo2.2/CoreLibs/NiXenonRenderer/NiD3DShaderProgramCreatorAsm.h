#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"

class NiD3DShaderProgramCreatorAsm;

// The shader program creator for assembled shader code; registered with NiD3DShaderProgramFactory by _SDMInit.
class NiD3DShaderProgramCreatorAsm : public NiD3DShaderProgramCreator {
public:
    NiD3DShaderProgramCreatorAsm();
    ~NiD3DShaderProgramCreatorAsm() override;

    uint32_t GetShaderCreationFlags() const override {
        return m_uiShaderCreationFlags;
    }
    void SetShaderCreationFlags(uint32_t) override;
    // bSoftwareVP and bRecoverable are int32_t rather than bool so that the caller's whole 32-bit value is passed on; a
    // bool would be zero-extended.
    NiD3DVertexShader* CreateVertexShaderFromFile(
        const char*,
        const char*,
        const char*,
        const char*,
        IDirect3DVertexDeclaration9*,
        uint32_t,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    ) override;
    // bSoftwareVP and bRecoverable are int32_t rather than bool so that the caller's whole 32-bit value is passed on; a
    // bool would be zero-extended.
    NiD3DVertexShader* CreateVertexShaderFromBuffer(
        const void*,
        uint32_t,
        const char*,
        const char*,
        const char*,
        IDirect3DVertexDeclaration9*,
        uint32_t,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    ) override;
    // bRecoverable is an int32_t rather than a bool so the caller's whole 32-bit value is passed on; a bool would be
    // zero-extended.
    NiD3DPixelShader*
    CreatePixelShaderFromFile(const char*, const char*, const char*, const char*, int32_t bRecoverable) override;
    // bRecoverable is an int32_t rather than a bool so the caller's whole 32-bit value is passed on; a bool would be
    // zero-extended.
    NiD3DPixelShader* CreatePixelShaderFromBuffer(
        const void*,
        uint32_t,
        const char*,
        const char*,
        const char*,
        int32_t bRecoverable
    ) override;
    bool RecreateVertexShader(NiD3DVertexShader*) override;
    bool RecreatePixelShader(NiD3DPixelShader*) override;
    // bRecoverable is an int32_t rather than a bool so the caller's whole 32-bit value is passed on; a bool would be
    // zero-extended.
    virtual bool LoadShaderCodeFromFile(
        const char* pcFileName,
        void*& pvCode,
        uint32_t& uiCodeSize,
        void*& pvConstantData,
        uint32_t& uiConstantSize,
        int32_t bRecoverable
    );
    // bRecoverable is int32_t rather than bool because the caller's whole value is passed on to ReportError; a bool
    // would be zero-extended.
    virtual bool LoadShaderCodeFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        void*& pvCode,
        uint32_t& uiCodeSize,
        void*& pvConstantData,
        uint32_t& uiConstantSize,
        int32_t bRecoverable
    );

    static void _SDMInit();

    uint32_t m_uiShaderCreationFlags;
    void* m_pvConstantBuffer;
    uint32_t m_uiConstantBufferSize;

    static inline NiD3DShaderProgramCreatorAsm* ms_pkCreator;
};
