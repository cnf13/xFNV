#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLVertexShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorHLSL.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

NiRTTI NiD3DHLSLVertexShader::ms_RTTI("NiD3DHLSLVertexShader", &NiD3DVertexShader::ms_RTTI);

NiD3DHLSLVertexShader::~NiD3DHLSLVertexShader() {
    _NiFree(m_pcEntryPoint);
    _NiFree(m_pcShaderTarget);
    if (m_pkConstantTable) {
        m_pkConstantTable->Release();
        m_pkConstantTable = nullptr;
    }
}

const NiRTTI* NiD3DHLSLVertexShader::GetRTTI() const {
    return &NiD3DHLSLVertexShader::ms_RTTI;
}

uint32_t NiD3DHLSLVertexShader::GetVariableCount() {
    _D3DXCONSTANTTABLE_DESC v3;
    if (!m_pkConstantTable) {
        return 0;
    }
    int32_t v2 = m_pkConstantTable->GetDesc(&v3);
    return v2 < 0 ? 0 : v3.Constants;
}

const char* NiD3DHLSLVertexShader::GetVariableName(uint32_t uiIndex) {
    ID3DXConstantTable* m_pkConstantTable;
    _D3DXCONSTANT_DESC kDesc;
    const char* hConstant = this->m_pkConstantTable->GetConstant(nullptr, uiIndex);
    if (hConstant &&
        ((m_pkConstantTable = this->m_pkConstantTable,
          (uiIndex = 1, m_pkConstantTable->GetConstantDesc(hConstant, &kDesc, &uiIndex) >= 0)) &&
         uiIndex)) {
        return kDesc.Name;
    }

    return nullptr;
}

// Hooks the entry up to its constant table variable on first use, then sets the constant by attribute type. The warning
// goes to NiRenderer::Error, which is empty. GetConstantDesc's result is not checked: for the handle GetConstantByName
// returned, D3DX fills kDesc.
bool NiD3DHLSLVertexShader::SetShaderConstant(
    NiShaderConstantMapEntry* pkEntry,
    const void* pvDataSource,
    uint32_t __formal
) {
    XFNV_INLINED_LOCAL(NiFixedString, kVariableName, std::nullopt, std::nullopt);
    if (!m_pkConstantTable || !m_pkD3DDevice) {
        return false;
    }
    if (!pvDataSource) {
        pvDataSource = pkEntry->m_pvDataSource;
    }
    if (!pkEntry->m_bVariableHookupValid) {
        NiGlobalStringTable::AcquireHandle(pkEntry->m_kVariableName.GetHandle());
        const char* kHandle = pkEntry->m_kVariableName.GetHandle();
        kVariableName.m_kHandle = kHandle;
        if (kHandle == NiGlobalStringTable::NULL_STRING) {
            kVariableName.Assign(pkEntry->m_kKey);
            pkEntry->SetVariableName(kVariableName);
            kHandle = kVariableName.GetHandle();
        }
        D3DXHANDLE pkVariable = m_pkConstantTable->GetConstantByName(nullptr, kHandle);
        if (!pkVariable) {
            NiRenderer::Error(
                "NiD3DHLSLVertexShader::SetShaderConstant - %s - variable %s not found\n", m_pszName, kHandle
            );
            // Releases kVariableName, as its destructor would.
            NiGlobalStringTable::ReleaseHandle(kHandle);
            return false;
        }
        D3DXCONSTANT_DESC kDesc;
        uint32_t uiCount = 1;
        m_pkConstantTable->GetConstantDesc(pkVariable, &kDesc, &uiCount);
        pkEntry->m_uiShaderRegister = kDesc.RegisterIndex;
        pkEntry->m_uiRegisterCount = kDesc.RegisterCount;
        pkEntry->m_bVariableHookupValid = true;
        // Releases kVariableName, as its destructor would.
        NiGlobalStringTable::ReleaseHandle(kHandle);
    }
    bool bReturn = false;
    switch (NiShaderConstantMapEntry::ms_aeAttribType[static_cast<uint8_t>(pkEntry->m_uiFlags)]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3:
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4: {
            alignas(16) D3DXMATRIX kMat;
            D3DXMatrixTranspose_Thunk(&kMat, static_cast<const D3DXMATRIX*>(pvDataSource));
            if (m_pkD3DRenderState->m_pkD3DDevice->SetVertexShaderConstantF(
                    pkEntry->m_uiShaderRegister, &kMat._11, pkEntry->m_uiRegisterCount
                ) >= 0) {
                bReturn = true;
            }
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            if (m_pkD3DRenderState->SetVertexShaderConstantF(
                    pkEntry->m_uiShaderRegister, static_cast<const float*>(pvDataSource), pkEntry->m_uiRegisterCount,
                    false
                )) {
                bReturn = true;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
            if (m_pkD3DRenderState->SetVertexShaderConstantI(
                    pkEntry->m_uiShaderRegister, static_cast<const int32_t*>(pvDataSource), pkEntry->m_uiRegisterCount,
                    false
                )) {
                bReturn = true;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
            if (m_pkD3DRenderState->SetVertexShaderConstantB(
                    pkEntry->m_uiShaderRegister, static_cast<const int32_t*>(pvDataSource), pkEntry->m_uiRegisterCount,
                    false
                )) {
                bReturn = true;
            }
            break;
        default: break;
    }
    return bReturn;
}

// Sets an array of matrices (matrix entries only), hooking the entry up to its constant table variable on first use.
// Bone matrices are stored as the shader wants them and go out as they are; other matrices are transposed one by one.
// With a reorder array, entry i is the data's pusReorderArray[i]th. Unlike Gamebryo 2.2, the per-entry sets call the
// device directly, and false comes back as soon as one fails. As in SetShaderConstant, the warning goes to
// NiRenderer::Error, which is empty.
bool NiD3DHLSLVertexShader::SetShaderConstantArray(
    NiShaderConstantMapEntry* pkEntry,
    const void* pvDataSource,
    uint32_t uiNumEntries,
    uint32_t uiRegistersPerEntry,
    uint16_t* pusReorderArray
) {
    XFNV_INLINED_LOCAL(NiFixedString, kVariableName, std::nullopt, std::nullopt);
    if (!m_pkConstantTable || !m_pkD3DDevice) {
        return false;
    }
    NiShaderAttributeDesc::AttributeType eType =
        NiShaderConstantMapEntry::ms_aeAttribType[static_cast<uint8_t>(pkEntry->m_uiFlags)];
    if (eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3 && eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) {
        return false;
    }
    if (!pvDataSource) {
        pvDataSource = pkEntry->m_pvDataSource;
    }
    bool bBoneMatrix = false;
    uint32_t uiInternal = pkEntry->m_uiInternal;
    if (uiInternal == NiShaderConstantMap::SCM_DEF_BONE_MATRIX_3 ||
        uiInternal == NiShaderConstantMap::SCM_DEF_SKINBONE_MATRIX_3 ||
        uiInternal == NiShaderConstantMap::SCM_DEF_BONE_MATRIX_4 ||
        uiInternal == NiShaderConstantMap::SCM_DEF_SKINBONE_MATRIX_4) {
        bBoneMatrix = true;
    }
    if (!pkEntry->m_bVariableHookupValid) {
        NiGlobalStringTable::AcquireHandle(pkEntry->m_kVariableName.GetHandle());
        const char* kHandle = pkEntry->m_kVariableName.GetHandle();
        kVariableName.m_kHandle = kHandle;
        if (kHandle == NiGlobalStringTable::NULL_STRING) {
            kVariableName.Assign(pkEntry->m_kKey);
            pkEntry->SetVariableName(kVariableName);
            kHandle = kVariableName.GetHandle();
        }
        D3DXHANDLE pkVariable = m_pkConstantTable->GetConstantByName(nullptr, kHandle);
        if (!pkVariable) {
            NiRenderer::Error(
                "NiD3DHLSLVertexShader::SetShaderConstantArray - %s - variable %s not found\n", m_pszName, kHandle
            );
            // Releases kVariableName, as its destructor would.
            NiGlobalStringTable::ReleaseHandle(kHandle);
            return false;
        }
        D3DXCONSTANT_DESC kDesc;
        uint32_t uiCount = 1;
        m_pkConstantTable->GetConstantDesc(pkVariable, &kDesc, &uiCount);
        pkEntry->m_uiShaderRegister = kDesc.RegisterIndex;
        pkEntry->m_uiRegisterCount = kDesc.RegisterCount;
        pkEntry->m_bVariableHookupValid = true;
        // Releases kVariableName, as its destructor would.
        NiGlobalStringTable::ReleaseHandle(kHandle);
    }
    bool bReturn = false;
    if (pusReorderArray) {
        uint32_t uiBaseRegister = pkEntry->m_uiShaderRegister;
        alignas(16) D3DXMATRIX kMat;
        for (uint32_t i = 0; i < uiNumEntries; i++) {
            uint16_t usNewIndex = pusReorderArray[i];
            bReturn = false;
            const float* pfData = static_cast<const float*>(pvDataSource) + usNewIndex * uiRegistersPerEntry * 4;
            if (!bBoneMatrix) {
                D3DXMatrixTranspose_Thunk(&kMat, (const D3DXMATRIX*)pfData);
                pfData = &kMat._11;
            }
            if (m_pkD3DRenderState->m_pkD3DDevice->SetVertexShaderConstantF(
                    uiBaseRegister, pfData, uiRegistersPerEntry
                ) < 0) {
                break;
            }
            uiBaseRegister += uiRegistersPerEntry;
            bReturn = true;
        }
    }
    else if (bBoneMatrix) {
        if (m_pkD3DRenderState->SetVertexShaderConstantF(
                pkEntry->m_uiShaderRegister, static_cast<const float*>(pvDataSource), pkEntry->m_uiRegisterCount, false
            )) {
            bReturn = true;
        }
    }
    else {
        uint32_t uiBaseRegister = pkEntry->m_uiShaderRegister;
        const float* pfDataSourceIterator = static_cast<const float*>(pvDataSource);
        alignas(16) D3DXMATRIX kMat;
        for (uint32_t i = 0; i < uiNumEntries; i++) {
            D3DXMatrixTranspose_Thunk(&kMat, (const D3DXMATRIX*)pfDataSourceIterator);
            bReturn = false;
            if (m_pkD3DRenderState->m_pkD3DDevice->SetVertexShaderConstantF(
                    uiBaseRegister, &kMat._11, uiRegistersPerEntry
                ) < 0) {
                break;
            }
            uiBaseRegister += uiRegistersPerEntry;
            pfDataSourceIterator += uiRegistersPerEntry * 4;
            bReturn = true;
        }
    }
    return bReturn;
}

void NiD3DHLSLVertexShader::DestroyRendererData() {
    if (m_pkConstantTable) {
        m_pkConstantTable->Release();
        m_pkConstantTable = nullptr;
    }
    NiD3DVertexShader::DestroyRendererData();
}

void NiD3DHLSLVertexShader::RecreateRendererData() {
    NiD3DVertexShader::RecreateRendererData();
}

// Replaces the entry point name with a copy of pcEntryPoint; an empty name clears it.
void NiD3DHLSLVertexShader::SetEntryPoint(const char* pcEntryPoint) {
    if (m_pcEntryPoint == pcEntryPoint) {
        return;
    }
    _NiFree(m_pcEntryPoint);
    if (pcEntryPoint && *pcEntryPoint) {
        // EXE-INLINED 0xEC6130
        size_t uiLength = 0;
        while (pcEntryPoint[uiLength]) {
            ++uiLength;
        }
        size_t uiSize = uiLength + 1;
        char* pcCopy = static_cast<char*>(_NiMalloc(uiSize));
        strcpy_s(pcCopy, uiSize, pcEntryPoint);
        m_pcEntryPoint = pcCopy;
    }
    else {
        m_pcEntryPoint = nullptr;
    }
}

const char* NiD3DHLSLVertexShader::GetShaderTarget() const {
    return m_pcShaderTarget;
}

// Replaces the shader target with a copy of pcShaderTarget. Unlike SetEntryPoint, an empty target leaves the freed
// pointer in place, deliberately, as the game does.
void NiD3DHLSLVertexShader::SetShaderTarget(const char* pcShaderTarget) {
    if (m_pcShaderTarget == pcShaderTarget) {
        return;
    }
    _NiFree(m_pcShaderTarget);
    if (pcShaderTarget && *pcShaderTarget) {
        // EXE-INLINED 0xEC6130
        size_t uiLength = 0;
        while (pcShaderTarget[uiLength]) {
            ++uiLength;
        }
        size_t uiSize = uiLength + 1;
        char* pcCopy = static_cast<char*>(_NiMalloc(uiSize));
        strcpy_s(pcCopy, uiSize, pcShaderTarget);
        m_pcShaderTarget = pcCopy;
    }
}

ID3DXConstantTable* NiD3DHLSLVertexShader::GetConstantTable() const {
    return m_pkConstantTable;
}

// Swaps in pkTable: releases the old table and holds a reference to the new one.
void NiD3DHLSLVertexShader::SetConstantTable(ID3DXConstantTable* pkTable) {
    if (m_pkConstantTable == pkTable) {
        return;
    }
    if (m_pkConstantTable) {
        m_pkConstantTable->Release();
    }
    m_pkConstantTable = pkTable;
    if (pkTable) {
        pkTable->AddRef();
    }
}

// Compiles HLSL source into shader code through D3DX; D3DX writes both of its output buffers on every path, so they are
// left unset before the call, and the caller's out-parameters take the copied code and its size. The whole of
// bRecoverable is passed on to ReportError.
bool NiD3DShaderProgramCreatorHLSL::LoadShaderCodeFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    void*& pvCode,
    uint32_t& uiCodeSize,
    ID3DXConstantTable*& pkConstantTable,
    int32_t bRecoverable
) {
    pvCode = nullptr;
    uiCodeSize = 0;
    if (!pvBuffer || !uiBufferSize) {
        NiShaderFactory::ReportError(NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable, "Invalid shader buffer\n");
        return false;
    }
    ID3DXBuffer* pkCode;
    ID3DXBuffer* pkErrors;
    uint32_t uiFlags = m_uiShaderCreationFlags | NiD3DShaderProgramCreator::ms_uiUniversalShaderCreationFlags;
    if (D3DXCompileShader_Thunk(
            static_cast<const char*>(pvBuffer), uiBufferSize, nullptr, nullptr, pcEntryPoint, pcShaderTarget, uiFlags,
            &pkCode, &pkErrors, &pkConstantTable
        ) >= 0) {
        uiCodeSize = pkCode->GetBufferSize();
        pvCode = _NiMalloc(uiCodeSize);
        rt::memcpy(pvCode, pkCode->GetBufferPointer(), uiCodeSize);
        pkCode->Release();
        if (pkErrors) {
            pkErrors->Release();
        }
        return true;
    }
    char* pcErr = nullptr;
    if (pkErrors) {
        void* pvBuff = pkErrors->GetBufferPointer();
        if (pvBuff) {
            uint32_t uiLen = pkErrors->GetBufferSize();
            pcErr = static_cast<char*>(_NiMalloc(uiLen));
            NiStrcpy(pcErr, uiLen, static_cast<const char*>(pvBuff));
            NiShaderFactory::ReportError(
                NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable,
                "Failed to assemble shader %s from memory\nError: %s\n", pcEntryPoint, pcErr
            );
        }
        pkErrors->Release();
    }
    else {
        NiShaderFactory::ReportError(
            NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable,
            "Failed to assemble shader %s from memory\nError: NONE REPORTED\n", pcEntryPoint
        );
    }
    _NiFree(pcErr);
    if (pkCode) {
        pkCode->Release();
    }
    return false;
}
