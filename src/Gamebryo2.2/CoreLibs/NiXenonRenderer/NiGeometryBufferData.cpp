#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

NiGeometryBufferData::NiGeometryBufferData()
  : m_uiFlags(0), m_pkGeometryGroup(nullptr), m_uiFVF(0), m_hDeclaration(nullptr), m_bSoftwareVP(false),
    m_uiVertCount(0), m_uiMaxVertCount(0), m_uiStreamCount(0), m_puiVertexStride(nullptr), m_ppkVBChip(nullptr),
    m_uiIndexCount(0), m_uiIBSize(0), m_pkIB(nullptr), m_uiBaseVertexIndex(0), m_eType(D3DPT_TRIANGLELIST),
    m_uiTriCount(0), m_uiMaxTriCount(0), m_uiNumArrays(0), m_pusArrayLengths(nullptr), m_pusIndexArray(nullptr) {}

// Gives each stream's chip back to the group, releases the index buffer and the declaration and frees the chip and
// stride arrays.
NiGeometryBufferData::~NiGeometryBufferData() {
    if (m_pkGeometryGroup) {
        for (uint32_t i = 0; i < m_uiStreamCount; ++i) {
            m_pkGeometryGroup->ReleaseChip(this, i);
        }
    }
    if (m_pkIB) {
        m_pkIB->Release();
        m_pkIB = nullptr;
    }
    m_uiIBSize = 0;
    _NiFree(m_ppkVBChip);
    _NiFree(m_puiVertexStride);
    if (m_hDeclaration) {
        m_hDeclaration->Release();
    }
}

// The flags' top byte is the texture coordinate set count. Other parameters are never reported.
bool NiGeometryBufferData::ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) {
    switch (eParameter) {
        case NiShaderDeclaration::SHADERPARAM_NI_NORMAL:
            return (m_uiFlags & NiDX9Renderer::VERTEX_FLAG_DX_HASNORMALS) != 0;
        case NiShaderDeclaration::SHADERPARAM_NI_COLOR:
            return (m_uiFlags & NiDX9Renderer::VERTEX_FLAG_DX_HASCOLORS) != 0;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD1:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD2:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD3:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD4:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD5:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD6:
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD7:
            return eParameter - NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0 <
                (m_uiFlags >> NiDX9Renderer::VERTEX_FLAG_DX_TEXTURESHIFT);
        case NiShaderDeclaration::SHADERPARAM_NI_TANGENT:
        case NiShaderDeclaration::SHADERPARAM_NI_BINORMAL:
            return (m_uiFlags & NiDX9Renderer::VERTEX_FLAG_DX_HASBINORMALSTANGENTS) != 0;
        default: return false;
    }
}

// Resizes both stream arrays, releasing chips removed by a shrink before freeing their old array.
void NiGeometryBufferData::SetStreamCount(uint32_t uiStreamCount) {
    if (uiStreamCount != m_uiStreamCount) {
        NiVBChip** ppkNewVBChip = static_cast<NiVBChip**>(_NiMalloc(sizeof(NiVBChip*) * uiStreamCount));
        uint32_t* puiNewVertexStride = static_cast<uint32_t*>(_NiMalloc(sizeof(uint32_t) * uiStreamCount));
        if (uiStreamCount >= m_uiStreamCount) {
            rt::memcpy(ppkNewVBChip, m_ppkVBChip, sizeof(NiVBChip*) * m_uiStreamCount);
            rt::memcpy(puiNewVertexStride, m_puiVertexStride, sizeof(uint32_t) * m_uiStreamCount);
            std::memset(&ppkNewVBChip[m_uiStreamCount], false, sizeof(NiVBChip*) * (uiStreamCount - m_uiStreamCount));
            std::memset(
                &puiNewVertexStride[m_uiStreamCount],
                false,
                sizeof(uint32_t) * (uiStreamCount - m_uiStreamCount)
            );
        }
        else {
            rt::memcpy(ppkNewVBChip, m_ppkVBChip, sizeof(NiVBChip*) * uiStreamCount);
            rt::memcpy(puiNewVertexStride, m_puiVertexStride, sizeof(uint32_t) * uiStreamCount);
            if (m_pkGeometryGroup) {
                for (uint32_t uiStream = uiStreamCount; uiStream < m_uiStreamCount; ++uiStream) {
                    m_pkGeometryGroup->ReleaseChip(this, uiStream);
                }
            }
        }
        MemoryManager::Instance().Deallocate(m_ppkVBChip);
        _NiFree(m_puiVertexStride);
        m_ppkVBChip = ppkNewVBChip;
        m_puiVertexStride = puiNewVertexStride;
        m_uiStreamCount = uiStreamCount;
    }
}

// An FVF replaces any vertex declaration.
void NiGeometryBufferData::SetFVF(uint32_t uiFVF) {
    m_uiFVF = uiFVF;
    if (m_hDeclaration) {
        m_hDeclaration->Release();
        m_hDeclaration = nullptr;
    }
}

void NiGeometryBufferData::SetVertexDeclaration(IDirect3DVertexDeclaration9* hDecl) {
    IDirect3DVertexDeclaration9* pkOldDeclaration = m_hDeclaration;
    if (hDecl != pkOldDeclaration) {
        if (pkOldDeclaration) {
            pkOldDeclaration->Release();
        }
        m_hDeclaration = hDecl;
        if (hDecl) {
            hDecl->AddRef();
        }
    }
    m_uiFVF = 0;
}

void NiGeometryBufferData::RemoveIB() {
    if (m_pkIB) {
        m_pkIB->Release();
        m_pkIB = nullptr;
    }
    m_uiIBSize = 0;
}
