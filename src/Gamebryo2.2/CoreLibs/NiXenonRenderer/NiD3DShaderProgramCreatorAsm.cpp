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

void NiD3DShaderProgramCreatorAsm::SetShaderCreationFlags(uint32_t uiFlags) {
    m_uiShaderCreationFlags = uiFlags;
}

NiD3DVertexShader* NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DVertexShader* pkVertexShader = new NiD3DVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkVertexShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    uint32_t uiConstantSize = 0;
    void* pvConstantData = nullptr;
    if (!this->LoadShaderCodeFromFile(pcFileName, pvCode, uiCodeSize, pvConstantData, uiConstantSize, bRecoverable)) {
        delete pkVertexShader;
        return nullptr;
    }
    IDirect3DVertexShader9*
        hShader = CreateVertexShader(pvCode, hDecl, uiUsage, pvConstantData, uiConstantSize, bSoftwareVP);
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

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
    delete pkVertexShader;
    return nullptr;
}

NiD3DVertexShader* NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DVertexShader* pkVertexShader = new NiD3DVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkVertexShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    uint32_t uiConstantSize = 0;
    void* pvConstantData = nullptr;
    if (!this->LoadShaderCodeFromBuffer(
            pvBuffer,
            uiBufferSize,
            pvCode,
            uiCodeSize,
            pvConstantData,
            uiConstantSize,
            bRecoverable
        )) {
        delete pkVertexShader;
        return nullptr;
    }
    IDirect3DVertexShader9*
        hShader = CreateVertexShader(pvCode, hDecl, uiUsage, pvConstantData, uiConstantSize, bSoftwareVP);
    if (hShader) {
        pkVertexShader->SetName(pcShaderName);
        pkVertexShader->SetShaderProgramName(nullptr);
        pkVertexShader->SetCode(uiCodeSize, pvCode);
        pkVertexShader->SetCreator(this);
        pkVertexShader->SetShaderHandle(hShader);
        pkVertexShader->SetUsage(uiUsage);
        pkVertexShader->SetVertexDeclaration(hDecl);
        // The setter stores only the low byte of bSoftwareVP.
        pkVertexShader->SetSoftwareVertexProcessing(static_cast<uint8_t>(bSoftwareVP));
        return pkVertexShader;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
    delete pkVertexShader;
    return nullptr;
}

NiD3DPixelShader* NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    NiD3DPixelShader* pkPixelShader = new NiD3DPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkPixelShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    uint32_t pvConstantData_2 = 0;
    void* pvConstantData = nullptr;
    if (!this->LoadShaderCodeFromFile(pcFileName, pvCode, uiCodeSize, pvConstantData, pvConstantData_2, bRecoverable)) {
        delete pkPixelShader;
        return nullptr;
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

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
    delete pkPixelShader;
    return nullptr;
}

NiD3DPixelShader* NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    NiD3DPixelShader* pkPixelShader = new NiD3DPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkPixelShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    uint32_t pvConstantData_2 = 0;
    void* pvConstantData = nullptr;
    if (!this->LoadShaderCodeFromBuffer(
            pvBuffer,
            uiBufferSize,
            pvCode,
            uiCodeSize,
            pvConstantData,
            pvConstantData_2,
            bRecoverable
        )) {
        delete pkPixelShader;
        return nullptr;
    }
    IDirect3DPixelShader9* hShader = CreatePixelShader(pvCode);
    if (hShader) {
        pkPixelShader->SetName(pcShaderName);
        pkPixelShader->SetShaderProgramName(nullptr);
        pkPixelShader->SetCode(uiCodeSize, pvCode);
        pkPixelShader->SetCreator(this);
        pkPixelShader->SetShaderHandle(hShader);
        return pkPixelShader;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
    delete pkPixelShader;
    return nullptr;
}

bool NiD3DShaderProgramCreatorAsm::RecreateVertexShader(NiD3DVertexShader* pkVertexShader) {
    IDirect3DVertexDeclaration9* v8;
    void* pvCode = pkVertexShader->GetCode();
    if (!pvCode) {
        return false;
    }
    v8 = pkVertexShader->GetVertexDeclaration();
    uint32_t v6 = pkVertexShader->GetUsage();
    IDirect3DVertexShader9* hShader = CreateVertexShader(pvCode, v8, v6, nullptr, 0, 0);
    if (!hShader) {
        return false;
    }
    pkVertexShader->SetShaderHandle(hShader);
    return true;
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
    pvCode = nullptr;
    uiCodeSize = 0;
    pvConstantData = nullptr;
    uiConstantSize = 0;
    if (!pcFileName || !*pcFileName) {
        NiShaderFactory::ReportError(1u, 0, "Invalid shader file name\n");
        return false;
    }
    if (!NiD3DShaderProgramFactory::ResolveShaderFileName(pcFileName, pcTrueName, 260u)) {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed to find shader program file %s\n", pcFileName);
        return false;
    }
    if (D3DXAssembleShaderFromFileA_Thunk(
            pcTrueName,
            nullptr,
            nullptr,
            (static_cast<uint8_t>(NiD3DShaderProgramCreator::ms_uiUniversalShaderCreationFlags) |
             static_cast<uint8_t>(m_uiShaderCreationFlags)) &
                3,
            &pkCode,
            &pkErrors
        ) < 0) {
        char* pcErr = nullptr;
        if (pkErrors) {
            void* pvBuff = pkErrors->GetBufferPointer();
            if (pvBuff) {
                uint32_t uiLen = pkErrors->GetBufferSize();
                pcErr = static_cast<char*>(_NiMalloc(uiLen));
                NiStrcpy(pcErr, uiLen, static_cast<char*>(pvBuff));
                NiShaderFactory::ReportError(
                    1u,
                    bRecoverable,
                    "Failed to assemble shader %s\nError: %s\n",
                    pcFileName,
                    pcErr
                );
            }
            pkErrors->Release();
        }
        else {
            NiShaderFactory::ReportError(
                1u,
                bRecoverable,
                "Failed to assemble shader %s\nError: NONE REPORTED\n",
                pcFileName
            );
        }
        _NiFree(pcErr);
        if (pkCode) {
            pkCode->Release();
        }
        return false;
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
    return true;
}

void NiD3DShaderProgramCreatorAsm::_SDMInit() {
    NiD3DShaderProgramCreatorAsm* v0 = new NiD3DShaderProgramCreatorAsm;
    NiD3DShaderProgramCreatorAsm::ms_pkCreator = v0;
    NiD3DShaderProgramFactory::RegisterShaderCreator("vsh", v0);
    NiD3DShaderProgramFactory::RegisterShaderCreator("psh", NiD3DShaderProgramCreatorAsm::ms_pkCreator);
}
