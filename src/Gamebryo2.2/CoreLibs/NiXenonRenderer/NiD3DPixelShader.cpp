#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"

NiRTTI NiD3DPixelShader::ms_RTTI("NiD3DPixelShader", &NiD3DShaderProgram::ms_RTTI);

NiD3DPixelShader::NiD3DPixelShader(NiDX9Renderer* apRenderer)
  : NiD3DShaderProgram(apRenderer, NiGPUProgram::PROGRAM_PIXEL), m_hShader(nullptr) {}

NiD3DPixelShader::~NiD3DPixelShader() {
    NiD3DShaderProgramFactory::ReleasePixelShader(this);
    if (m_hShader) {
        m_pkD3DRenderer->PurgePixelShader(this);
    }
    m_hShader = nullptr;
}

// Sets the entry's registers from apvDataSource (or the entry's data), auiRegisterCount registers (or the entry's
// count, at least 1), as bool, int or float constants by the entry's attribute type.
bool NiD3DPixelShader::SetShaderConstant(
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
    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType
        [pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_ATTRIB_TYPE_MASK];
    IDirect3DDevice9* pkDevice = m_pkD3DRenderState->m_pkD3DDevice;
    if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_BOOL) {
        return pkDevice->SetPixelShaderConstantB(
                   pkEntry->m_uiShaderRegister,
                   static_cast<const int32_t*>(pvDataSource),
                   uiRegisterCount
               ) >= 0;
    }
    if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
        return pkDevice->SetPixelShaderConstantI(
                   pkEntry->m_uiShaderRegister,
                   static_cast<const int32_t*>(pvDataSource),
                   uiRegisterCount
               ) >= 0;
    }
    return pkDevice->SetPixelShaderConstantF(
               pkEntry->m_uiShaderRegister,
               static_cast<const float*>(pvDataSource),
               uiRegisterCount
           ) >= 0;
}

// Matrix entries only: entry i of the array comes from source element apusReorderArray[i] and goes to the next
// auiRegistersPerEntry float registers. The device is re-read for every entry.
bool NiD3DPixelShader::SetShaderConstantArray(
    NiShaderConstantMapEntry* pkEntry,
    const void* pvDataSource,
    uint32_t uiNumEntries,
    uint32_t uiRegistersPerEntry,
    uint16_t* pusReorderArray
) {
    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType
        [pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_ATTRIB_TYPE_MASK];
    if (eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3 && eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) {
        return false;
    }

    uint32_t uiRegister = pkEntry->m_uiShaderRegister;
    for (uint32_t i = 0; i < uiNumEntries; i++) {
        const uint8_t* pucSource = static_cast<const uint8_t*>(pvDataSource) +
            ((pusReorderArray[i] * uiRegistersPerEntry) << 4);
        m_pkD3DRenderState->m_pkD3DDevice
            ->SetPixelShaderConstantF(uiRegister, reinterpret_cast<const float*>(pucSource), uiRegistersPerEntry);
        uiRegister += uiRegistersPerEntry;
    }
    return true;
}

// The renderer releases the shader handle, when there is one.
void NiD3DPixelShader::DestroyRendererData() {
    if (m_hShader) {
        m_pkD3DRenderer->PurgePixelShader(this);
    }
}

// The program factory recreates the shader through its creator unless it still has a handle.
void NiD3DPixelShader::RecreateRendererData() {
    NiD3DShaderProgramFactory::RecreatePixelShader(this);
}
