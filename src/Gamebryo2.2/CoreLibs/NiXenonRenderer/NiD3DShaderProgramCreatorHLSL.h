#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"

struct ID3DXBuffer;
struct ID3DXConstantTable;

// The shader program creator for HLSL source; registered with NiD3DShaderProgramFactory by _SDMInit.
class NiD3DShaderProgramCreatorHLSL : public NiD3DShaderProgramCreator {
public:
    NiD3DShaderProgramCreatorHLSL();
    ~NiD3DShaderProgramCreatorHLSL() override;

    uint32_t GetShaderCreationFlags() const override {
        return m_uiShaderCreationFlags;
    }
    void SetShaderCreationFlags(uint32_t uiFlags) override {
        m_uiShaderCreationFlags = uiFlags;
    }
    // bSoftwareVP and bRecoverable are int32_t rather than bool so the caller's whole 32-bit value is passed on; a bool
    // would be zero-extended.
    NiD3DVertexShader* CreateVertexShaderFromFile(
        const char* pcFileName,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        IDirect3DVertexDeclaration9* hDecl,
        uint32_t uiUsage,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    ) override;
    // bSoftwareVP and bRecoverable are int32_t rather than bool so the caller's whole 32-bit value is passed on; a bool
    // would be zero-extended.
    NiD3DVertexShader* CreateVertexShaderFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        IDirect3DVertexDeclaration9* hDecl,
        uint32_t uiUsage,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    ) override;
    // bRecoverable is an int32_t rather than a bool so that the caller's whole value is passed on; a bool would be
    // zero-extended.
    NiD3DPixelShader* CreatePixelShaderFromFile(
        const char* pcFileName,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        int32_t bRecoverable
    ) override;
    // bRecoverable is an int32_t rather than a bool so that the caller's whole value is passed on; a bool would be
    // zero-extended.
    NiD3DPixelShader* CreatePixelShaderFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        int32_t bRecoverable
    ) override;
    bool RecreateVertexShader(NiD3DVertexShader* pkVertexShader) override;
    bool RecreatePixelShader(NiD3DPixelShader* pkPixelShader) override;
    // bSoftwareVP and bRecoverable, here and below, are int32_t rather than bool because the caller's whole value is
    // passed on (to CreateVertexShader and NiShaderFactory::ReportError); a bool would be zero-extended.
    virtual NiD3DVertexShader* CreateVertexShaderFromD3DXBuffer(
        ID3DXBuffer* pkBuffer,
        uint32_t uiBufferSize,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        IDirect3DVertexDeclaration9* hDecl,
        uint32_t uiUsage,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    );
    virtual NiD3DPixelShader* CreatePixelShaderFromD3DXBuffer(
        ID3DXBuffer* pkBuffer,
        uint32_t uiBufferSize,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        int32_t bRecoverable
    );

    // Compiles HLSL source into shader code and its constant table; false on failure. bRecoverable is int32_t rather
    // than bool because the caller's whole value is passed on; a bool would be zero-extended.
    bool LoadShaderCodeFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        void*& pvCode,
        uint32_t& uiCodeSize,
        ID3DXConstantTable*& pkConstantTable,
        int32_t bRecoverable
    );
    // bRecoverable is an int32_t rather than a bool so that the caller's whole value is passed on; a bool would be
    // zero-extended.
    bool LoadShaderCodeFromFile(
        const char* pcFileName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        void*& pvCode,
        uint32_t& uiCodeSize,
        ID3DXConstantTable*& pkConstantTable,
        int32_t bRecoverable
    );

    static void _SDMInit();

    uint32_t m_uiShaderCreationFlags;

    // The creator _SDMInit publishes for the "hlsl" extension.
    static inline NiD3DShaderProgramCreatorHLSL* ms_pkCreator;
};
