#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorHLSL.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/string.h"

NiD3DShaderProgramCreatorHLSL::NiD3DShaderProgramCreatorHLSL() : m_uiShaderCreationFlags(0) {}

NiD3DShaderProgramCreatorHLSL::~NiD3DShaderProgramCreatorHLSL() {}

NiD3DVertexShader* NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DHLSLVertexShader* pkVertexShader = new NiD3DHLSLVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkVertexShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    ID3DXConstantTable* pkConstantTable = nullptr;
    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    if (!pcShaderTarget) {
        pcShaderTarget = D3DXGetVertexShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }
    if (!LoadShaderCodeFromFile(
            pcFileName,
            pcEntryPoint,
            pcShaderTarget,
            pvCode,
            uiCodeSize,
            pkConstantTable,
            bRecoverable
        )) {
        delete pkVertexShader;
        return nullptr;
    }
    uint8_t v12 = bSoftwareVP;
    IDirect3DVertexShader9* hShader = CreateVertexShader(pvCode, hDecl, uiUsage, nullptr, 0, bSoftwareVP);
    if (hShader) {
        pkVertexShader->SetName(pcShaderName);
        pkVertexShader->SetShaderProgramName(pcFileName);
        pkVertexShader->SetCode(uiCodeSize, pvCode);
        pkVertexShader->SetCreator(this);
        pkVertexShader->SetShaderHandle(hShader);
        pkVertexShader->SetUsage(uiUsage);
        pkVertexShader->SetSoftwareVertexProcessing(v12);
        pkVertexShader->SetEntryPoint(pcEntryPoint);
        pkVertexShader->SetShaderTarget(pcShaderTarget);
        pkVertexShader->SetConstantTable(pkConstantTable);
        if (pkConstantTable) {
            pkConstantTable->Release();
        }
        return pkVertexShader;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
    delete pkVertexShader;
    return nullptr;
}

NiD3DVertexShader* NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromBuffer(
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
    NiD3DHLSLVertexShader* pkVertexShader = new NiD3DHLSLVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkVertexShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    ID3DXConstantTable* pkConstantTable = nullptr;
    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    if (!pcShaderTarget) {
        pcShaderTarget = D3DXGetVertexShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }
    if (!LoadShaderCodeFromBuffer(
            pvBuffer,
            uiBufferSize,
            pcEntryPoint,
            pcShaderTarget,
            pvCode,
            uiCodeSize,
            pkConstantTable,
            bRecoverable
        )) {
        delete pkVertexShader;
        return nullptr;
    }
    uint8_t v13 = bSoftwareVP;
    IDirect3DVertexShader9* hShader = CreateVertexShader(pvCode, hDecl, uiUsage, nullptr, 0, bSoftwareVP);
    if (hShader) {
        pkVertexShader->SetName(pcShaderName);
        pkVertexShader->SetShaderProgramName(nullptr);
        pkVertexShader->SetCode(uiCodeSize, pvCode);
        pkVertexShader->SetCreator(this);
        pkVertexShader->SetShaderHandle(hShader);
        pkVertexShader->SetUsage(uiUsage);
        pkVertexShader->SetSoftwareVertexProcessing(v13);
        pkVertexShader->SetEntryPoint(pcEntryPoint);
        pkVertexShader->SetShaderTarget(pcShaderTarget);
        pkVertexShader->SetConstantTable(pkConstantTable);
        if (pkConstantTable) {
            pkConstantTable->Release();
        }
        return pkVertexShader;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
    delete pkVertexShader;
    return nullptr;
}

NiD3DPixelShader* NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromFile(
    const char* pcFileName,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    IDirect3DPixelShader9* hShader;
    NiD3DHLSLPixelShader* pkPixelShader = new NiD3DHLSLPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkPixelShader) {
        return nullptr;
    }
    uint32_t uiCodeSize = 0;
    void* pvCode = nullptr;
    ID3DXConstantTable* pkConstantTable = nullptr;
    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    const char* v8 = pcShaderTarget;
    if (!pcShaderTarget) {
        v8 = D3DXGetPixelShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }
    if (LoadShaderCodeFromFile(pcFileName, pcEntryPoint, v8, pvCode, uiCodeSize, pkConstantTable, bRecoverable)) {
        hShader = CreatePixelShader(pvCode);
        if (hShader) {
            pkPixelShader->SetName(pcShaderName);
            pkPixelShader->SetShaderProgramName(pcFileName);
            pkPixelShader->SetCode(uiCodeSize, pvCode);
            pkPixelShader->SetCreator(this);
            pkPixelShader->SetShaderHandle(hShader);
            pkPixelShader->SetEntryPoint(pcEntryPoint);
            pkPixelShader->SetShaderTarget(v8);
            pkPixelShader->SetConstantTable(pkConstantTable);
            if (pkConstantTable) {
                pkConstantTable->Release();
            }
            return pkPixelShader;
        }
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
    }
    delete pkPixelShader;
    return nullptr;
}

NiD3DPixelShader* NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    IDirect3DPixelShader9* hShader;
    void* pvCode;
    ID3DXConstantTable* pkConstantTable;
    uint32_t uiCodeSize;
    NiD3DHLSLPixelShader* pkPixelShader = new NiD3DHLSLPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkPixelShader) {
        return nullptr;
    }
    uiCodeSize = 0;
    pvCode = nullptr;
    pkConstantTable = nullptr;
    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    const char* v9 = pcShaderTarget;
    if (!pcShaderTarget) {
        v9 = D3DXGetPixelShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }
    if (LoadShaderCodeFromBuffer(
            pvBuffer,
            uiBufferSize,
            pcEntryPoint,
            v9,
            pvCode,
            uiCodeSize,
            pkConstantTable,
            bRecoverable
        )) {
        hShader = CreatePixelShader(pvCode);
        if (hShader) {
            pkPixelShader->SetName(pcShaderName);
            pkPixelShader->SetShaderProgramName(nullptr);
            pkPixelShader->SetCode(uiCodeSize, pvCode);
            pkPixelShader->SetCreator(this);
            pkPixelShader->SetShaderHandle(hShader);
            pkPixelShader->SetEntryPoint(pcEntryPoint);
            pkPixelShader->SetShaderTarget(v9);
            pkPixelShader->SetConstantTable(pkConstantTable);
            if (pkConstantTable) {
                pkConstantTable->Release();
            }
            return pkPixelShader;
        }
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
    }
    delete pkPixelShader;
    return nullptr;
}

// Recreates the device shader from the kept code and sets its constant table again.
bool NiD3DShaderProgramCreatorHLSL::RecreateVertexShader(NiD3DVertexShader* pkVertexShader) {
    void* pvCode = pkVertexShader->GetCode();
    if (!pvCode) {
        return false;
    }

    IDirect3DVertexDeclaration9* hDecl = pkVertexShader->GetVertexDeclaration();
    IDirect3DVertexShader9* hShader = CreateVertexShader(pvCode, hDecl, pkVertexShader->GetUsage(), nullptr, 0, 0);
    if (!hShader) {
        return false;
    }

    pkVertexShader->SetShaderHandle(hShader);

    // pkConstantTable deliberately starts out holding pkVertexShader's address, so that value is what
    // remains when D3DX fails without writing the table.
    ID3DXConstantTable* pkConstantTable = reinterpret_cast<ID3DXConstantTable*>(pkVertexShader);
    // The reference source casts the shader to its HLSL class here.
    NiD3DHLSLVertexShader* pkHLSLVS = static_cast<NiD3DHLSLVertexShader*>(pkVertexShader);
    if (D3DXGetShaderConstantTable_Thunk(static_cast<const uint32_t*>(pvCode), &pkConstantTable) >= 0) {
        pkHLSLVS->SetConstantTable(pkConstantTable);
    }

    if (pkConstantTable) {
        pkConstantTable->Release();
    }

    return true;
}

// Recreates the device shader from the kept code and sets its constant table again.
bool NiD3DShaderProgramCreatorHLSL::RecreatePixelShader(NiD3DPixelShader* pkPixelShader) {
    void* pvCode = pkPixelShader->GetCode();
    if (!pvCode) {
        return false;
    }

    IDirect3DPixelShader9* hShader = CreatePixelShader(pvCode);
    if (!hShader) {
        return false;
    }

    pkPixelShader->SetShaderHandle(hShader);

    // pkConstantTable deliberately starts out holding the shader's address, because pkPixelShader's
    // argument is passed as the constant table's out pointer; that value is what remains when D3DX fails without
    // writing it.
    ID3DXConstantTable* pkConstantTable = reinterpret_cast<ID3DXConstantTable*>(pkPixelShader);
    // The reference source casts the shader to its HLSL class here.
    NiD3DHLSLPixelShader* pkHLSLPS = static_cast<NiD3DHLSLPixelShader*>(pkPixelShader);
    if (D3DXGetShaderConstantTable_Thunk(static_cast<const uint32_t*>(pvCode), &pkConstantTable) >= 0) {
        pkHLSLPS->SetConstantTable(pkConstantTable);
    }

    if (pkConstantTable) {
        pkConstantTable->Release();
    }

    return true;
}

// Wraps the compiled code of pkBuffer in a new NiD3DHLSLVertexShader; null on failure. Deliberately leaks the shader
// when the constant table cannot be read, and stores the handle before the usage.
NiD3DVertexShader* NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromD3DXBuffer(
    ID3DXBuffer* pkBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    NiD3DHLSLVertexShader* pkShader = new NiD3DHLSLVertexShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkShader) {
        return nullptr;
    }

    ID3DXConstantTable* pkConstantTable = nullptr;
    if (D3DXGetShaderConstantTable_Thunk(static_cast<const uint32_t*>(pkBuffer->GetBufferPointer()), &pkConstantTable) <
        0) {
        return nullptr;
    }

    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    if (!pcShaderTarget) {
        pcShaderTarget = D3DXGetVertexShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }

    IDirect3DVertexShader9*
        hShader = CreateVertexShader(pkBuffer->GetBufferPointer(), hDecl, uiUsage, nullptr, 0, bSoftwareVP);
    if (!hShader) {
        NiShaderFactory::ReportError(1, bRecoverable, "Failed CreateVertexShader call on %s\n", pcShaderName);
        delete pkShader;
        return nullptr;
    }

    void* pvCode = _NiMalloc(uiBufferSize);
    uint32_t uiSize = pkBuffer->GetBufferSize();
    rt::memcpy(pvCode, pkBuffer->GetBufferPointer(), uiSize);

    pkShader->SetName(pcShaderName);
    pkShader->SetShaderProgramName(nullptr);
    pkShader->SetCode(uiBufferSize, pvCode);
    pkShader->SetCreator(this);
    pkShader->SetShaderHandle(hShader);
    pkShader->SetUsage(uiUsage);
    pkShader->SetSoftwareVertexProcessing(static_cast<uint8_t>(bSoftwareVP));
    pkShader->SetEntryPoint(pcEntryPoint);
    pkShader->SetShaderTarget(pcShaderTarget);
    pkShader->SetConstantTable(pkConstantTable);
    if (pkConstantTable) {
        pkConstantTable->Release();
    }
    return pkShader;
}

// Wraps the compiled code of pkBuffer in a new NiD3DHLSLPixelShader; null on failure. Leaks the shader when the
// constant table cannot be read.
NiD3DPixelShader* NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromD3DXBuffer(
    ID3DXBuffer* pkBuffer,
    uint32_t uiBufferSize,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    NiD3DHLSLPixelShader* pkShader = new NiD3DHLSLPixelShader(NiD3DShaderProgramCreator::ms_pkD3DRenderer);
    if (!pkShader) {
        return nullptr;
    }

    ID3DXConstantTable* pkConstantTable = nullptr;
    if (D3DXGetShaderConstantTable_Thunk(static_cast<const uint32_t*>(pkBuffer->GetBufferPointer()), &pkConstantTable) <
        0) {
        return nullptr;
    }

    if (!pcEntryPoint) {
        pcEntryPoint = "main";
    }
    if (!pcShaderTarget) {
        pcShaderTarget = D3DXGetPixelShaderProfile_Thunk(NiD3DShaderProgramCreator::ms_pkD3DDevice);
    }

    IDirect3DPixelShader9* hShader = CreatePixelShader(pkBuffer->GetBufferPointer());
    if (!hShader) {
        NiShaderFactory::ReportError(1, bRecoverable, "Failed CreatePixelShader call on %s\n", pcShaderName);
        delete pkShader;
        return nullptr;
    }

    void* pvCode = _NiMalloc(uiBufferSize);
    uint32_t uiSize = pkBuffer->GetBufferSize();
    rt::memcpy(pvCode, pkBuffer->GetBufferPointer(), uiSize);

    pkShader->SetName(pcShaderName);
    pkShader->SetShaderProgramName(nullptr);
    pkShader->SetCode(uiBufferSize, pvCode);
    pkShader->SetCreator(this);
    pkShader->SetShaderHandle(hShader);
    pkShader->SetEntryPoint(pcEntryPoint);
    pkShader->SetShaderTarget(pcShaderTarget);
    pkShader->SetConstantTable(pkConstantTable);
    if (pkConstantTable) {
        pkConstantTable->Release();
    }
    return pkShader;
}

bool NiD3DShaderProgramCreatorHLSL::LoadShaderCodeFromFile(
    const char* pcFileName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    void*& pvCode,
    uint32_t& uiCodeSize,
    ID3DXConstantTable*& pkConstantTable,
    int32_t bRecoverable
) {
    ID3DXBuffer* pkErrors;
    ID3DXBuffer* pkCode;
    char acShaderPath[260];
    pvCode = nullptr;
    const void* pvBuff = pcShaderTarget;
    ID3DXConstantTable** v18 = &pkConstantTable;
    uiCodeSize = 0;
    if (!pcFileName || !*pcFileName) {
        NiShaderFactory::ReportError(1u, bRecoverable, "Invalid shader file name\n");
        return false;
    }
    if (!NiD3DShaderProgramFactory::ResolveShaderFileName(pcFileName, acShaderPath, 260u)) {
        NiShaderFactory::ReportError(1u, bRecoverable, "Failed to find shader program file %s\n", pcFileName);
        return false;
    }
    if (D3DXCompileShaderFromFileA_Thunk(
            acShaderPath,
            nullptr,
            nullptr,
            pcEntryPoint,
            static_cast<const char*>(pvBuff),
            NiD3DShaderProgramCreator::ms_uiUniversalShaderCreationFlags | this->m_uiShaderCreationFlags,
            &pkCode,
            &pkErrors,
            v18
        ) < 0) {
        char* pcErr = nullptr;
        if (pkErrors) {
            void* pvBuff_2 = pkErrors->GetBufferPointer();
            if (pvBuff_2) {
                uint32_t uiLen = pkErrors->GetBufferSize();
                pcErr = static_cast<char*>(_NiMalloc(uiLen));
                NiStrcpy(pcErr, uiLen, static_cast<char*>(pvBuff_2));
                NiShaderFactory::ReportError(
                    1u,
                    bRecoverable,
                    "Failed to compile shader %s in file %s\nError: %s\n",
                    pcEntryPoint,
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
                "Failed to compile shader %s in file %s\nError: NONE REPORTED\n",
                pcEntryPoint,
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
    uint32_t uiDestSize = uiCodeSize;
    const void* v12 = pkCode->GetBufferPointer();
    rt::memcpy(pvCode, v12, uiDestSize);
    pkCode->Release();
    if (pkErrors) {
        pkErrors->Release();
    }
    return true;
}

void NiD3DShaderProgramCreatorHLSL::_SDMInit() {
    NiD3DShaderProgramCreatorHLSL* pkCreator = new NiD3DShaderProgramCreatorHLSL;
    NiD3DShaderProgramCreatorHLSL::ms_pkCreator = pkCreator;
    NiD3DShaderProgramFactory::RegisterShaderCreator("hlsl", pkCreator);
}

NiD3DHLSLPixelShader::NiD3DHLSLPixelShader(NiDX9Renderer* pkRenderer)
  : NiD3DPixelShader(pkRenderer), m_pcEntryPoint(nullptr), m_pcShaderTarget(nullptr), m_pkConstantTable(nullptr) {}

NiD3DHLSLVertexShader::NiD3DHLSLVertexShader(NiDX9Renderer* pkRenderer)
  : NiD3DVertexShader(pkRenderer), m_pcEntryPoint(nullptr), m_pcShaderTarget(nullptr), m_pkConstantTable(nullptr) {}
