#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9VertexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

#include <cstdint>

NiDX9ShaderDeclaration::NiDX9ShaderDeclaration()
  : NiD3DShaderDeclaration(), m_pkElements(nullptr), m_hVertexDecl(nullptr), m_bSoftwareVB(false) {}

// Frees the element array and releases the vertex declaration; the base does the rest.
NiDX9ShaderDeclaration::~NiDX9ShaderDeclaration() {
    MemoryManager::Instance().Deallocate(m_pkElements);
    if (m_hVertexDecl) {
        m_hVertexDecl->Release();
        m_hVertexDecl = nullptr;
    }
}

// Maps the Gamebryo parameter to its D3D usage and usage index and sets the entry with the default tesselator.
// Parameters past SHADERPARAM_NI_BINORMAL (and SHADERPARAM_INVALID) fail.
bool NiDX9ShaderDeclaration::SetEntry(
    uint32_t auiEntry,
    uint32_t auiRegister,
    NiShaderDeclaration::ShaderParameter aeInput,
    NiShaderDeclaration::ShaderParameterType aeType,
    uint32_t auiStream
) {
    uint32_t uiIndex = 0;
    ShaderParameterUsage eUsage;
    switch (aeInput) {
        case SHADERPARAM_NI_POSITION: eUsage = SPUSAGE_POSITION; break;
        case SHADERPARAM_NI_BLENDWEIGHT: eUsage = SPUSAGE_BLENDWEIGHT; break;
        case SHADERPARAM_NI_BLENDINDICES: eUsage = SPUSAGE_BLENDINDICES; break;
        case SHADERPARAM_NI_NORMAL: eUsage = SPUSAGE_NORMAL; break;
        case SHADERPARAM_NI_COLOR: eUsage = SPUSAGE_COLOR; break;
        case SHADERPARAM_NI_TEXCOORD0: eUsage = SPUSAGE_TEXCOORD; break;
        case SHADERPARAM_NI_TEXCOORD1:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 1;
            break;
        case SHADERPARAM_NI_TEXCOORD2:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 2;
            break;
        case SHADERPARAM_NI_TEXCOORD3:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 3;
            break;
        case SHADERPARAM_NI_TEXCOORD4:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 4;
            break;
        case SHADERPARAM_NI_TEXCOORD5:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 5;
            break;
        case SHADERPARAM_NI_TEXCOORD6:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 6;
            break;
        case SHADERPARAM_NI_TEXCOORD7:
            eUsage = SPUSAGE_TEXCOORD;
            uiIndex = 7;
            break;
        case SHADERPARAM_NI_TANGENT: eUsage = SPUSAGE_TANGENT; break;
        case SHADERPARAM_NI_BINORMAL: eUsage = SPUSAGE_BINORMAL; break;
        default: return false;
    }
    return SetEntry(auiStream, auiEntry, auiRegister, aeInput, aeType, eUsage, uiIndex, SPTESS_DEFAULT);
}

// Marks the entry unused (SHADERPARAM_INVALID, SPTYPE_NONE).
bool NiDX9ShaderDeclaration::RemoveEntry(uint32_t auiEntry, uint32_t auiStream) {
    if (auiEntry >= m_uiMaxStreamEntryCount || auiStream >= m_uiStreamCount) {
        return false;
    }

    ShaderRegisterEntry* pkEntry = &m_pkStreamEntries[auiStream].m_pkEntries[auiEntry];
    pkEntry->m_eInput = SHADERPARAM_INVALID;
    pkEntry->m_eType = SPTYPE_NONE;
    m_bModified = true;
    if (m_hVertexDecl) {
        m_hVertexDecl->Release();
        m_hVertexDecl = nullptr;
    }
    return true;
}

// Stores the entry's input, type, usage, usage index and tesselator (not the register) when any differs; a blend
// weight that is not SPTYPE_FLOAT16_4 becomes SPTYPE_FLOAT3. A change marks the declaration modified and releases
// the D3D declaration.
bool NiDX9ShaderDeclaration::SetEntry(
    uint32_t auiStream,
    uint32_t auiEntry,
    uint32_t auiRegister,
    NiShaderDeclaration::ShaderParameter aeInput,
    NiShaderDeclaration::ShaderParameterType aeType,
    NiShaderDeclaration::ShaderParameterUsage aeUsage,
    uint32_t auiIndex,
    NiD3DShaderDeclaration::ShaderParameterTesselator aeTess
) {
    if (auiEntry >= m_uiMaxStreamEntryCount || auiStream >= m_uiStreamCount) {
        return false;
    }

    ShaderRegisterEntry* pkEntry = &m_pkStreamEntries[auiStream].m_pkEntries[auiEntry];
    if (pkEntry->m_eInput == aeInput &&
        pkEntry->m_eType == aeType &&
        pkEntry->m_eUsage == aeUsage &&
        pkEntry->m_uiUsageIndex == auiIndex &&
        pkEntry->m_eTesselator == aeTess) {
        return true;
    }

    pkEntry->m_eInput = aeInput;
    pkEntry->m_eType = aeType;
    pkEntry->m_eUsage = aeUsage;
    pkEntry->m_uiUsageIndex = auiIndex;
    pkEntry->m_eTesselator = aeTess;
    if (aeInput == SHADERPARAM_NI_BLENDWEIGHT && aeType != SPTYPE_FLOAT16_4) {
        pkEntry->m_eType = SPTYPE_FLOAT3;
    }
    m_bModified = true;
    if (m_hVertexDecl) {
        m_hVertexDecl->Release();
        m_hVertexDecl = nullptr;
    }
    return true;
}

// Marks every entry of the stream unused.
bool NiDX9ShaderDeclaration::ClearAllEntries(uint32_t auiStream) {
    if (auiStream >= m_uiStreamCount) {
        return false;
    }

    ShaderRegisterStream* pkStream = &m_pkStreamEntries[auiStream];
    for (uint32_t i = 0; i < m_uiMaxStreamEntryCount; i++) {
        pkStream->m_pkEntries[i].m_eInput = SHADERPARAM_INVALID;
        pkStream->m_pkEntries[i].m_eType = SPTYPE_NONE;
    }
    m_bModified = true;
    if (m_hVertexDecl) {
        m_hVertexDecl->Release();
        m_hVertexDecl = nullptr;
    }
    return true;
}

// Rebuilds the declaration first (which sets the strides), then returns the stream's stride.
uint32_t NiDX9ShaderDeclaration::GetVertexStride(uint32_t auiStream) {
    if (auiStream >= m_uiStreamCount) {
        return 0;
    }
    GetD3DDeclaration();
    return m_pkStreamEntries[auiStream].m_uiStride;
}

// Packs the stream's entries of the geometry (or skin partition) into its vertex chip: the locked buffer given, the
// old chip when it is still large enough, or a newly allocated chip (then every entry is dirty). Only dirty entries
// are packed; for software skinning (a skin instance without a partition), position, normal, binormal and tangent
// are deformed into the buffer by the skin instance instead. Returns the chip, apOldVBChip when there are no
// vertices, or null on failure.
NiVBChip* NiDX9ShaderDeclaration::PackUntransformedVB(
    NiGeometryData* apData,
    NiSkinInstance* apSkin,
    NiSkinPartition::Partition* apPartition,
    uint16_t ausDirtyFlags,
    NiVBChip* apOldVBChip,
    uint32_t auiStream,
    void* apvLockedBuff,
    void* apv8,
    void* apv9
) {
    uint16_t usVertices = apPartition ? apPartition->m_usVertices : apData->GetActiveVertexCount();
    if (!usVertices) {
        return apOldVBChip;
    }

    IDirect3DVertexDeclaration9* hDeclaration = nullptr;
    uint32_t uiNumStreams = 0;
    if (!GenerateVertexParameters(&hDeclaration, &uiNumStreams)) {
        return nullptr;
    }

    NiGeometryBufferData* pkBuffData = apPartition ? apPartition->m_pkBuffData : apData->m_pkBuffData;
    pkBuffData->SetVertexDeclaration(hDeclaration);
    if (auiStream < pkBuffData->m_uiStreamCount) {
        pkBuffData->m_puiVertexStride[auiStream] = m_pkStreamEntries[auiStream].m_uiStride;
    }
    if (m_pkRenderer->m_bSWVertexCapable) {
        pkBuffData->m_bSoftwareVP = GetSoftwareVertexProcessing();
    }

    NiVBChip* pkVBChip;
    uint8_t* pucLockedBuff;
    bool bUnlock = true;
    if (apvLockedBuff) {
        pkVBChip = apOldVBChip;
        bUnlock = false;
        pucLockedBuff = static_cast<uint8_t*>(apvLockedBuff);
    }
    else {
        bool bReuse = false;
        if (apOldVBChip && apOldVBChip->m_pkVB) {
            D3DVERTEXBUFFER_DESC kDesc;
            apOldVBChip->m_pkVB->GetDesc(&kDesc);
            if (kDesc.Format == D3DFMT_VERTEXDATA && kDesc.Size >= GetVertexStride(auiStream) * usVertices) {
                bReuse = true;
            }
        }
        if (bReuse) {
            pkVBChip = apOldVBChip;
        }
        else {
            if (!m_pkVBManager->AllocateBufferSpace(pkBuffData, auiStream)) {
                return nullptr;
            }
            pkVBChip = pkBuffData->GetVBChip(auiStream);
            ausDirtyFlags = NiGeometryData::DIRTY_MASK;
        }
        pucLockedBuff = static_cast<uint8_t*>(
            m_pkVBManager->LockVB(pkVBChip->m_pkVB, pkVBChip->m_uiOffset, pkVBChip->m_uiSize, pkVBChip->m_uiLockFlags)
        );
    }

    // software skinning: a hardware-skinned partition is packed like unskinned data
    bool bSkinned = apSkin && !apPartition;
    ShaderRegisterStream* pkStream = &m_pkStreamEntries[auiStream];
    NiPoint3* pkDstVertex = nullptr;
    NiPoint3* pkDstNormal = nullptr;
    NiPoint3* pkDstBinormal = nullptr;
    NiPoint3* pkDstTangent = nullptr;
    if (pkStream) {
        for (uint32_t i = 0; i < m_uiMaxStreamEntryCount; i++) {
            ShaderRegisterEntry* pkEntry = &pkStream->m_pkEntries[i];
            uint8_t* pucDest = pucLockedBuff + pkEntry->m_uiPackingOffset;
            switch (pkEntry->m_eInput) {
                case SHADERPARAM_NI_POSITION:
                    if (!(ausDirtyFlags & NiGeometryData::VERTEX_MASK)) {
                        continue;
                    }
                    if (bSkinned) {
                        pkDstVertex = reinterpret_cast<NiPoint3*>(pucDest);
                        continue;
                    }
                    break;
                case SHADERPARAM_NI_NORMAL:
                    if (!(ausDirtyFlags & NiGeometryData::NORMAL_MASK)) {
                        continue;
                    }
                    if (bSkinned) {
                        pkDstNormal = reinterpret_cast<NiPoint3*>(pucDest);
                        continue;
                    }
                    break;
                case SHADERPARAM_NI_TANGENT:
                    if (!(ausDirtyFlags & NiGeometryData::NORMAL_MASK)) {
                        continue;
                    }
                    if (bSkinned) {
                        pkDstTangent = reinterpret_cast<NiPoint3*>(pucDest);
                        continue;
                    }
                    break;
                case SHADERPARAM_NI_BINORMAL:
                    if (!(ausDirtyFlags & NiGeometryData::NORMAL_MASK)) {
                        continue;
                    }
                    if (bSkinned) {
                        pkDstBinormal = reinterpret_cast<NiPoint3*>(pucDest);
                        continue;
                    }
                    break;
                case SHADERPARAM_NI_COLOR:
                    if (!(ausDirtyFlags & NiGeometryData::COLOR_MASK)) {
                        continue;
                    }
                    break;
                case SHADERPARAM_NI_TEXCOORD0:
                case SHADERPARAM_NI_TEXCOORD1:
                case SHADERPARAM_NI_TEXCOORD2:
                case SHADERPARAM_NI_TEXCOORD3:
                case SHADERPARAM_NI_TEXCOORD4:
                case SHADERPARAM_NI_TEXCOORD5:
                case SHADERPARAM_NI_TEXCOORD6:
                case SHADERPARAM_NI_TEXCOORD7:
                    if (!(ausDirtyFlags & NiGeometryData::TEXTURE_MASK)) {
                        continue;
                    }
                    break;
                default: break;
            }
            PackEntry(usVertices, apData, apSkin, apPartition, pkEntry, pkStream->m_uiStride, pucDest, apv8, apv9);
        }
    }

    if (bSkinned) {
        apSkin->Deform(
            apData->m_pkVertex,
            apData->m_pkNormal,
            apData->m_usVertices,
            pkDstVertex,
            pkDstNormal,
            pkDstBinormal,
            pkDstTangent,
            pkStream->m_uiStride
        );
    }
    if (bUnlock && !m_pkVBManager->UnlockVB(pkVBChip->m_pkVB)) {
        return nullptr;
    }
    return pkVBChip;
}

// Return the D3D declaration and the number of populated streams.
bool NiDX9ShaderDeclaration::GenerateVertexParameters(
    IDirect3DVertexDeclaration9** apDeclaration,
    uint32_t* apuiNumStreams
) {
    *apDeclaration = GetD3DDeclaration();
    *apuiNumStreams = m_uiStreamCount;
    return *apDeclaration != nullptr;
}

// Rebuilds the D3D declaration when the entries changed: counts the used entries (marking the streams that have
// any), regrows the element array when too small, sets each used entry's packing offset and each valid stream's
// stride, ends the array with D3DDECL_END and creates the declaration. Returns null without used entries or when the
// device fails.
IDirect3DVertexDeclaration9* NiDX9ShaderDeclaration::GetD3DDeclaration() {
    if (!m_bModified) {
        return m_hVertexDecl;
    }

    if (m_hVertexDecl) {
        m_hVertexDecl->Release();
        m_hVertexDecl = nullptr;
    }

    uint32_t uiEntryCount = 0;
    for (uint32_t i = 0; i < m_uiStreamCount; i++) {
        ShaderRegisterStream* pkStream = &m_pkStreamEntries[i];
        if (!pkStream) {
            continue;
        }
        pkStream->m_bValid = false;
        for (uint32_t j = 0; j < m_uiMaxStreamEntryCount; j++) {
            ShaderRegisterEntry* pkEntry = &pkStream->m_pkEntries[j];
            if (pkEntry->m_eInput != SHADERPARAM_INVALID && pkEntry->m_eType != SPTYPE_NONE) {
                uiEntryCount++;
                pkStream->m_bValid = true;
            }
        }
    }
    // one more for D3DDECL_END
    if (uiEntryCount > 0) {
        uiEntryCount++;
    }

    if (m_uiDeclSize < uiEntryCount) {
        MemoryManager::Instance().Deallocate(m_pkElements);
        m_pkElements = nullptr;
    }
    if (uiEntryCount == 0) {
        m_uiDeclSize = 0;
        return nullptr;
    }
    if (!m_pkElements) {
        m_uiDeclSize = uiEntryCount;
        // new D3DVERTEXELEMENT9[uiEntryCount]: the size saturates on overflow
        uint64_t ulSize = static_cast<uint64_t>(uiEntryCount) * sizeof(D3DVERTEXELEMENT9);
        m_pkElements = static_cast<D3DVERTEXELEMENT9*>(
            MemoryManager::Instance().Allocate(ulSize > SIZE_MAX ? SIZE_MAX : static_cast<size_t>(ulSize))
        );
    }

    m_uiCurrDeclIndex = 0;
    for (uint32_t i = 0; i < m_uiStreamCount; i++) {
        ShaderRegisterStream* pkStream = &m_pkStreamEntries[i];
        if (!pkStream->m_bValid) {
            continue;
        }
        uint32_t uiOffset = 0;
        for (uint32_t j = 0; j < m_uiMaxStreamEntryCount; j++) {
            ShaderRegisterEntry* pkEntry = &pkStream->m_pkEntries[j];
            if (pkEntry->m_eInput != SHADERPARAM_INVALID && pkEntry->m_eType != SPTYPE_NONE) {
                pkEntry->m_uiPackingOffset = uiOffset;
                uiOffset += AddDeclarationEntry(pkEntry, i);
            }
        }
        pkStream->m_uiStride = uiOffset;
    }

    m_pkElements[m_uiCurrDeclIndex].Stream = 0xFF;
    m_pkElements[m_uiCurrDeclIndex].Offset = 0;
    m_pkElements[m_uiCurrDeclIndex].Type = D3DDECLTYPE_UNUSED;
    m_pkElements[m_uiCurrDeclIndex].Method = 0;
    m_pkElements[m_uiCurrDeclIndex].Usage = 0;
    m_pkElements[m_uiCurrDeclIndex].UsageIndex = 0;

    if (m_pkD3DDevice->CreateVertexDeclaration(m_pkElements, &m_hVertexDecl) < 0) {
        return nullptr;
    }
    m_bModified = false;
    return m_hVertexDecl;
}

uint32_t NiDX9ShaderDeclaration::AddDeclarationEntry(ShaderRegisterEntry* apEntry, uint32_t auiStream) {
    m_pkElements[m_uiCurrDeclIndex].Stream = static_cast<uint16_t>(auiStream);
    m_pkElements[m_uiCurrDeclIndex].Offset = static_cast<uint16_t>(apEntry->m_uiPackingOffset);
    m_pkElements[m_uiCurrDeclIndex].Type = static_cast<uint8_t>(ms_aeTypes[apEntry->m_eType]);
    m_pkElements[m_uiCurrDeclIndex].Method = static_cast<uint8_t>(ms_aeMethods[apEntry->m_eTesselator]);
    m_pkElements[m_uiCurrDeclIndex].Usage = static_cast<uint8_t>(ms_aeUsage[apEntry->m_eUsage]);
    m_pkElements[m_uiCurrDeclIndex].UsageIndex = static_cast<uint8_t>(apEntry->m_uiUsageIndex);
    m_uiCurrDeclIndex++;
    return GetTypeSize(apEntry->m_eType);
}

// Allocates a declaration for at most the renderer's stream count and initialises it; null when it cannot.
NiDX9ShaderDeclaration*
NiDX9ShaderDeclaration::Create(NiDX9Renderer* pkRenderer, uint32_t uiMaxStreamEntryCount, uint32_t uiStreamCount) {
    if (uiStreamCount > pkRenderer->m_uiMaxStreams) {
        return nullptr;
    }
    NiDX9ShaderDeclaration* pkShaderDecl = new NiDX9ShaderDeclaration;
    if (!pkShaderDecl) {
        return nullptr;
    }
    if (!pkShaderDecl->Initialize(pkRenderer, uiMaxStreamEntryCount, uiStreamCount)) {
        delete pkShaderDecl;
        return nullptr;
    }
    return pkShaderDecl;
}
