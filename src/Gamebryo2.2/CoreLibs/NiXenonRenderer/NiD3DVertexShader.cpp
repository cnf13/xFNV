#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"

NiRTTI NiD3DVertexShader::ms_RTTI("NiD3DVertexShader", &NiD3DShaderProgram::ms_RTTI);

NiD3DVertexShader::NiD3DVertexShader(NiDX9Renderer* apRenderer)
  : NiD3DShaderProgram(apRenderer, NiGPUProgram::PROGRAM_VERTEX), m_bSoftwareVP(0), m_uiUsage(0), m_hShader(nullptr),
    m_hDecl(nullptr) {}

// Takes the shader out of the program factory's map and purges its device shader.
NiD3DVertexShader::~NiD3DVertexShader() {
    NiD3DShaderProgramFactory::ReleaseVertexShader(this);
    if (m_hShader) {
        m_pkD3DRenderer->PurgeVertexShader(this);
    }
}

// Like the pixel shader's, but a defined entry with its own data takes a copy of the source (m_uiDataSize bytes)
// instead of a device call.
bool NiD3DVertexShader::SetShaderConstant(
    NiShaderConstantMapEntry* pkEntry,
    const void* pvDataSource,
    uint32_t uiRegisterCount
) {
    if (!pvDataSource) {
        pvDataSource = pkEntry->m_pvDataSource;
    }
    if (!uiRegisterCount) {
        uiRegisterCount = pkEntry->m_uiRegisterCount;
        if (!uiRegisterCount) {
            uiRegisterCount = 1;
        }
    }
    uint32_t uiFlags = pkEntry->m_uiFlags;
    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType[uiFlags & 0xFF];
    if ((uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) == NiShaderConstantMapEntry::SCME_MAP_DEFINED &&
        pkEntry->m_pvDataSource) {
        rt::memcpy(pkEntry->m_pvDataSource, pvDataSource, pkEntry->m_uiDataSize);
        return true;
    }
    IDirect3DDevice9* pkDevice = m_pkD3DRenderState->m_pkD3DDevice;
    if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_BOOL) {
        return pkDevice->SetVertexShaderConstantB(
                   pkEntry->m_uiShaderRegister, static_cast<const int32_t*>(pvDataSource), uiRegisterCount
               ) >= 0;
    }
    if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
        return pkDevice->SetVertexShaderConstantI(
                   pkEntry->m_uiShaderRegister, static_cast<const int32_t*>(pvDataSource), uiRegisterCount
               ) >= 0;
    }
    return pkDevice->SetVertexShaderConstantF(
               pkEntry->m_uiShaderRegister, static_cast<const float*>(pvDataSource), uiRegisterCount
           ) >= 0;
}

// Matrix entries only: entry i comes from source element apusReorderArray[i]. A defined entry with its own data
// copies it to element i of that data; otherwise it goes to the next auiRegistersPerEntry float registers.
bool NiD3DVertexShader::SetShaderConstantArray(
    NiShaderConstantMapEntry* pkEntry,
    const void* pvDataSource,
    uint32_t uiNumEntries,
    uint32_t uiRegistersPerEntry,
    uint16_t* pusReorderArray
) {
    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF];
    if (eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3 && eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) {
        return false;
    }

    uint32_t uiRegister = pkEntry->m_uiShaderRegister;
    uint32_t uiEntryBytes = uiRegistersPerEntry << 4;
    for (uint32_t i = 0; i < uiNumEntries; i++) {
        const uint8_t* pucSource =
            static_cast<const uint8_t*>(pvDataSource) + ((pusReorderArray[i] * uiRegistersPerEntry) << 4);
        if ((pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
                NiShaderConstantMapEntry::SCME_MAP_DEFINED &&
            pkEntry->m_pvDataSource) {
            rt::memcpy(
                static_cast<uint8_t*>(pkEntry->m_pvDataSource) + i * uiEntryBytes, pucSource, pkEntry->m_uiDataSize
            );
        }
        else {
            m_pkD3DRenderState->m_pkD3DDevice->SetVertexShaderConstantF(
                uiRegister, reinterpret_cast<const float*>(pucSource), uiRegistersPerEntry
            );
        }
        uiRegister += uiRegistersPerEntry;
    }
    return true;
}

// The renderer releases the shader handle, when there is one.
void NiD3DVertexShader::DestroyRendererData() {
    if (m_hShader) {
        m_pkD3DRenderer->PurgeVertexShader(this);
    }
}

// The program factory recreates the shader through its creator unless it still has a handle.
void NiD3DVertexShader::RecreateRendererData() {
    NiD3DShaderProgramFactory::RecreateVertexShader(this);
}
