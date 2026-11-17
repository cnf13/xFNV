#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorAsm.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/string.h"

NiD3DShaderProgramCreatorAsm::NiD3DShaderProgramCreatorAsm()
  : m_uiShaderCreationFlags(0), m_pvConstantBuffer(nullptr), m_uiConstantBufferSize(0) {}

// Frees the constant buffer.
NiD3DShaderProgramCreatorAsm::~NiD3DShaderProgramCreatorAsm() {
    _NiFree(m_pvConstantBuffer);
}

void NiD3DShaderProgramCreatorAsm::SetShaderCreationFlags(uint32_t a2) {
    m_uiShaderCreationFlags = a2;
}

NiD3DVertexShader* NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* a3,
    const char* a4,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DVertexShader* pkVertexShader =
        static_cast<NiD3DVertexShader*>(NiMemObject::operator new(sizeof(NiD3DVertexShader)));
    if (!pkVertexShader) {
        return 0;
    }
    ::new (pkVertexShader) NiD3DVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    uint32_t uiCodeSize = 0;
    void* pvCode = 0;
    uint32_t uiConstantSize = 0;
    void* pvConstantData = 0;
    if (!this->LoadShaderCodeFromFile(pcFileName, pvCode, uiCodeSize, pvConstantData, uiConstantSize, bRecoverable)) {
        delete pkVertexShader;
        return 0;
    }
    IDirect3DVertexShader9* hShader =
        CreateVertexShader(pvCode, hDecl, uiUsage, pvConstantData, uiConstantSize, bSoftwareVP);
    if (hShader) {
        pkVertexShader->SetName(pcShaderName);
        pkVertexShader->SetShaderProgramName(pcFileName);
        pkVertexShader->SetCode(uiCodeSize, pvCode);
        pkVertexShader->SetCreator(this);
        pkVertexShader->SetShaderHandle(hShader);
        pkVertexShader->SetUsage(uiUsage);
        pkVertexShader->SetVertexDeclaration(hDecl);
        pkVertexShader->SetSoftwareVertexProcessing(bSoftwareVP);
        return pkVertexShader;
    }
    else {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
        delete pkVertexShader;
        return 0;
    }
}

NiD3DVertexShader* NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* a4,
    const char* a5,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DVertexShader* pkVertexShader =
        static_cast<NiD3DVertexShader*>(NiMemObject::operator new(sizeof(NiD3DVertexShader)));
    if (!pkVertexShader) {
        return 0;
    }
    ::new (pkVertexShader) NiD3DVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    uint32_t uiCodeSize = 0;
    void* pvCode = 0;
    uint32_t uiConstantSize = 0;
    void* pvConstantData = 0;
    if (!this->LoadShaderCodeFromBuffer(
            pvBuffer, uiBufferSize, pvCode, uiCodeSize, pvConstantData, uiConstantSize, bRecoverable
        )) {
        delete pkVertexShader;
        return 0;
    }
    IDirect3DVertexShader9* hShader =
        CreateVertexShader(pvCode, hDecl, uiUsage, pvConstantData, uiConstantSize, bSoftwareVP);
    if (hShader) {
        pkVertexShader->SetName(pcShaderName);
        pkVertexShader->SetShaderProgramName(0);
        pkVertexShader->SetCode(uiCodeSize, pvCode);
        pkVertexShader->SetCreator(this);
        pkVertexShader->SetShaderHandle(hShader);
        pkVertexShader->SetUsage(uiUsage);
        pkVertexShader->SetVertexDeclaration(hDecl);
        // The setter stores only the low byte of bSoftwareVP.
        pkVertexShader->SetSoftwareVertexProcessing(static_cast<uint8_t>(bSoftwareVP));
        return pkVertexShader;
    }
    else {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
        delete pkVertexShader;
        return 0;
    }
}

NiD3DPixelShader* NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* a3,
    const char* a4,
    int32_t bRecoverable
) {
    NiD3DPixelShader* pkPixelShader =
        static_cast<NiD3DPixelShader*>(NiMemObject::operator new(sizeof(NiD3DPixelShader)));
    if (!pkPixelShader) {
        return 0;
    }
    ::new (pkPixelShader) NiD3DPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    uint32_t uiCodeSize = 0;
    void* pvCode = 0;
    uint32_t pvConstantData_2 = 0;
    void* pvConstantData = 0;
    if (!this->LoadShaderCodeFromFile(pcFileName, pvCode, uiCodeSize, pvConstantData, pvConstantData_2, bRecoverable)) {
        delete pkPixelShader;
        return 0;
    }
    IDirect3DPixelShader9* hShader = CreatePixelShader(pvCode);
    if (hShader) {
        pkPixelShader->SetName(pcShaderName);
        pkPixelShader->SetShaderProgramName(pcFileName);
        pkPixelShader->SetCode(uiCodeSize, pvCode);
        pkPixelShader->SetCreator(this);
        pkPixelShader->SetShaderHandle(hShader);
        return pkPixelShader;
    }
    else {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
        delete pkPixelShader;
        return 0;
    }
}

NiD3DPixelShader* NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* a4,
    const char* a5,
    int32_t bRecoverable
) {
    NiD3DPixelShader* pkPixelShader =
        static_cast<NiD3DPixelShader*>(NiMemObject::operator new(sizeof(NiD3DPixelShader)));
    if (!pkPixelShader) {
        return 0;
    }
    ::new (pkPixelShader) NiD3DPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    uint32_t uiCodeSize = 0;
    void* pvCode = 0;
    uint32_t pvConstantData_2 = 0;
    void* pvConstantData = 0;
    if (!this->LoadShaderCodeFromBuffer(
            pvBuffer, uiBufferSize, pvCode, uiCodeSize, pvConstantData, pvConstantData_2, bRecoverable
        )) {
        delete pkPixelShader;
        return 0;
    }
    IDirect3DPixelShader9* hShader = CreatePixelShader(pvCode);
    if (hShader) {
        pkPixelShader->SetName(pcShaderName);
        pkPixelShader->SetShaderProgramName(0);
        pkPixelShader->SetCode(uiCodeSize, pvCode);
        pkPixelShader->SetCreator(this);
        pkPixelShader->SetShaderHandle(hShader);
        return pkPixelShader;
    }
    else {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
        delete pkPixelShader;
        return 0;
    }
}

bool NiD3DShaderProgramCreatorAsm::RecreateVertexShader(NiD3DVertexShader* a1) {
    IDirect3DVertexDeclaration9* v8;
    void* pvCode = a1->GetCode();
    if (!pvCode) {
        return 0;
    }
    v8 = a1->GetVertexDeclaration();
    uint32_t v6 = a1->GetUsage();
    IDirect3DVertexShader9* hShader = CreateVertexShader(pvCode, v8, v6, 0, 0, 0);
    if (!hShader) {
        return 0;
    }
    a1->SetShaderHandle(hShader);
    return 1;
}

// Recreates the device shader from the program's kept code and hands it to the program.
bool NiD3DShaderProgramCreatorAsm::RecreatePixelShader(NiD3DPixelShader* pkPixelShader) {
    void* pvCode = pkPixelShader->GetCode();
    if (!pvCode) {
        return false;
    }
    IDirect3DPixelShader9* hShader = CreatePixelShader(pvCode);
    if (!hShader) {
        return false;
    }
    pkPixelShader->SetShaderHandle(hShader);
    return true;
}

bool NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromFile(
    const char* pcFileName,
    void*& pvCode,
    uint32_t& uiCodeSize,
    void*& pvConstantData,
    uint32_t& uiConstantSize,
    int32_t bRecoverable
) {
    ID3DXBuffer* pkErrors;
    ID3DXBuffer* pkCode;
    char pcTrueName[260];
    pvCode = 0;
    uiCodeSize = 0;
    pvConstantData = 0;
    uiConstantSize = 0;
    if (!pcFileName || !*pcFileName) {
        NiShaderFactory::ReportError(1u, 0, "Invalid shader file name\n");
        return 0;
    }
    if (!NiD3DShaderProgramFactory::ResolveShaderFileName(pcFileName, pcTrueName, 260u)) {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed to find shader program file %s\n", pcFileName);
        return 0;
    }
    if (D3DXAssembleShaderFromFileA_Thunk(
            pcTrueName, 0, 0,
            (static_cast<uint8_t>(NiD3DShaderProgramCreator::ms_uiUniversalShaderCreationFlags) |
             static_cast<uint8_t>(m_uiShaderCreationFlags)) &
                3,
            &pkCode, &pkErrors
        ) < 0) {
        char* pcErr = 0;
        if (pkErrors) {
            void* pvBuff = pkErrors->GetBufferPointer();
            if (pvBuff) {
                uint32_t uiLen = pkErrors->GetBufferSize();
                pcErr = static_cast<char*>(_NiMalloc(uiLen));
                NiStrcpy(pcErr, uiLen, static_cast<char*>(pvBuff));
                NiShaderFactory::ReportError(
                    1u, bRecoverable, "Failed to assemble shader %s\nError: %s\n", pcFileName, pcErr
                );
            }
            pkErrors->Release();
        }
        else {
            NiShaderFactory::ReportError(
                1u, bRecoverable, "Failed to assemble shader %s\nError: NONE REPORTED\n", pcFileName
            );
        }
        _NiFree(pcErr);
        if (pkCode) {
            pkCode->Release();
        }
        return 0;
    }
    uint32_t uiLen_2 = pkCode->GetBufferSize();
    uiCodeSize = uiLen_2;
    pvCode = _NiMalloc(uiLen_2);
    uint32_t v12 = uiCodeSize;
    const void* v13 = pkCode->GetBufferPointer();
    rt::memcpy(pvCode, v13, v12);
    pkCode->Release();
    if (pkErrors) {
        pkErrors->Release();
    }
    return 1;
}

void NiD3DShaderProgramCreatorAsm::_SDMInit() {
    NiD3DShaderProgramCreatorAsm* v0 =
        static_cast<NiD3DShaderProgramCreatorAsm*>(NiMemObject::operator new(sizeof(NiD3DShaderProgramCreatorAsm)));
    if (v0) {
        ::new (v0) NiD3DShaderProgramCreatorAsm;
    }
    else {
        v0 = 0;
    }
    NiD3DShaderProgramCreatorAsm::ms_pkCreator = v0;
    NiD3DShaderProgramFactory::RegisterShaderCreator("vsh", v0);
    NiD3DShaderProgramFactory::RegisterShaderCreator("psh", NiD3DShaderProgramCreatorAsm::ms_pkCreator);
}
