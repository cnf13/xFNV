#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9VertexBufferManager.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "SDK/MSVC/string.h"

#include <cmath>

NiDX9VertexBufferManager::NiDX9VertexBufferManager(IDirect3DDevice9* pkD3DDevice9)
  : m_pkD3DDevice9(pkD3DDevice9), m_kDefaultResources(37), m_kManagedResources(37), m_kSystemMemResources(37),
    m_uiFrameID(0), m_pvTempBuffer(nullptr), m_uiTempBufferSize(0), m_pvLockedBuffer(nullptr), m_uiLockedBufferSize(0) {
    m_pkD3DDevice9->AddRef();
    m_kDefaultResources.RemoveAll();
    m_kManagedResources.RemoveAll();
    m_kSystemMemResources.RemoveAll();
}

// The maps' and the critical section's own destructors free their tables and delete the section.
NiDX9VertexBufferManager::~NiDX9VertexBufferManager() {
    NiTPointerMap<uint32_t, NiDX9VBInfo*>* apkResourceMaps[3] =
        {&m_kDefaultResources, &m_kManagedResources, &m_kSystemMemResources};
    for (NiTPointerMap<uint32_t, NiDX9VBInfo*>* pkResourceMap : apkResourceMaps) {
        NiTMapItem<uint32_t, NiDX9VBInfo*>* pkPos = pkResourceMap->GetFirstPos();
        while (pkPos) {
            uint32_t uiKey;
            NiDX9VBInfo* pkVBInfo;
            pkResourceMap->GetNext(pkPos, uiKey, pkVBInfo);
            if (pkVBInfo) {
                for (NiDX9VBResource* pkVBRes : pkVBInfo->m_pkVBRes) {
                    if (pkVBRes) {
                        if (pkVBRes->m_pkVB) {
                            pkVBRes->m_pkVB->Release();
                        }
                        delete pkVBRes;
                    }
                }
                delete pkVBInfo;
            }
        }
    }
    m_kDefaultResources.RemoveAll();
    m_kManagedResources.RemoveAll();
    m_kSystemMemResources.RemoveAll();
    m_pkD3DDevice9->Release();
    m_pkD3DDevice9 = nullptr;
    _NiFree(m_pvTempBuffer);
}

void NiDX9VertexBufferManager::EndFrame() {
    ++m_uiFrameID;
}

// The geometry group releases the stream's chip and makes a new one, which is stored when the stream exists.
bool NiDX9VertexBufferManager::AllocateBufferSpace(NiGeometryBufferData* apBuffData, uint32_t auiStream) {
    NiGeometryGroup* pkGroup = apBuffData->m_pkGeometryGroup;
    pkGroup->ReleaseChip(apBuffData, auiStream);
    NiVBChip* pkChip = pkGroup->CreateChip(apBuffData, auiStream);
    if (!pkChip) {
        return false;
    }
    if (auiStream < apBuffData->m_uiStreamCount) {
        apBuffData->m_ppkVBChip[auiStream] = pkChip;
    }
    return true;
}

// The temporary buffer grows (freed and reallocated) to the lock size.
void* NiDX9VertexBufferManager::LockVB(
    IDirect3DVertexBuffer9* apVB,
    uint32_t auiOffset,
    uint32_t auiSize,
    uint32_t auiLockFlags
) {
    void* pvLocked = nullptr;
    if (apVB->Lock(auiOffset, auiSize, &pvLocked, auiLockFlags) < 0) {
        return nullptr;
    }

    m_kVBCriticalSection.Lock();
    m_pvLockedBuffer = pvLocked;
    m_uiLockedBufferSize = auiSize;
    if (m_uiTempBufferSize < auiSize) {
        _NiFree(m_pvTempBuffer);
        m_pvTempBuffer = _NiMalloc(auiSize);
        m_uiTempBufferSize = auiSize;
    }
    if (!(auiLockFlags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE))) {
        rt::memcpy(m_pvTempBuffer, pvLocked, auiSize);
    }
    return m_pvTempBuffer;
}

bool NiDX9VertexBufferManager::UnlockVB(IDirect3DVertexBuffer9* apVB) {
    rt::memcpy(m_pvLockedBuffer, m_pvTempBuffer, m_uiLockedBufferSize);
    m_pvLockedBuffer = nullptr;
    m_uiLockedBufferSize = 0;
    m_kVBCriticalSection.Unlock();
    return apVB->Unlock() >= 0;
}

uint32_t NiDX9VertexBufferManager::GetFrameID() {
    return m_uiFrameID;
}

// The partition's vertices through its vertex map, with the blend weights after the position: all but the last
// bone's weight, plus the last for one or three bones. A weight set that does not sum to 1 (within 1e-5) is
// formatted into a local message that is never shown. The skin instance is not used. The chip choice is
// PackUntransformedVB's.
NiVBChip* NiDX9VertexBufferManager::PackSkinnedVB(
    NiGeometryData* apData,
    NiSkinInstance* apSkin,
    NiSkinPartition::Partition* apPartition,
    uint32_t auiDirtyFlags,
    NiVBChip* apOldVBChip,
    void* apvLockedBuff
) {
    const NiPoint3* pkVertex = apData->m_pkVertex;
    const NiPoint3* pkNormal = apData->m_pkNormal;
    const NiColorA* pkColor = apData->m_pkColor;
    uint32_t uiTextureSets = apData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
    if (uiTextureSets > NiD3DPass::ms_uiMaxTextureBlendStages) {
        uiTextureSets = NiD3DPass::ms_uiMaxTextureBlendStages;
    }
    const uint16_t usBones = apPartition->m_usBones;
    const uint16_t usVerts = apPartition->m_usVertices;
    const uint16_t* pusVertexMap = apPartition->m_pusVertexMap;

    uint32_t uiFVF = 0;
    uint32_t uiVertexStride = 0;
    uint32_t uiPosOffset;
    uint32_t uiWeightOffset;
    uint32_t uiNormOffset;
    uint32_t uiColorOffset;
    uint32_t uiTexOffset;
    GenerateVertexParameters(
        apData,
        apPartition,
        uiFVF,
        uiVertexStride,
        uiPosOffset,
        uiWeightOffset,
        uiNormOffset,
        uiColorOffset,
        uiTexOffset
    );

    NiGeometryBufferData* pkBuffData = apPartition->m_pkBuffData;
    pkBuffData->SetFVF(uiFVF);
    if (pkBuffData->m_uiStreamCount > 0) {
        pkBuffData->m_puiVertexStride[0] = uiVertexStride;
    }

    NiVBChip* pkVBChip;
    uint8_t* pucBuffer;
    bool bUnlock = true;
    if (apvLockedBuff) {
        pkVBChip = apOldVBChip;
        bUnlock = false;
        pucBuffer = static_cast<uint8_t*>(apvLockedBuff);
    }
    else {
        bool bReuse = false;
        if (apOldVBChip && apOldVBChip->m_pkVB) {
            // GetDesc's result is not checked; it fills the description.
            D3DVERTEXBUFFER_DESC kDesc;
            apOldVBChip->m_pkVB->GetDesc(&kDesc);
            bReuse = kDesc.Format == D3DFMT_VERTEXDATA && kDesc.FVF == uiFVF && kDesc.Size >= usVerts * uiVertexStride;
        }
        if (bReuse) {
            pkVBChip = apOldVBChip;
        }
        else {
            if (!AllocateBufferSpace(pkBuffData, 0)) {
                return nullptr;
            }
            pkVBChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
            auiDirtyFlags = NiGeometryData::DIRTY_MASK;
        }
        pucBuffer = static_cast<uint8_t*>(
            LockVB(pkVBChip->m_pkVB, pkVBChip->m_uiOffset, pkVBChip->m_uiSize, pkVBChip->m_uiLockFlags)
        );
    }

    if (pkVertex && (auiDirtyFlags & NiGeometryData::VERTEX_MASK)) {
        uint8_t* pucDest = pucBuffer + uiPosOffset;
        for (uint32_t i = 0; i < usVerts; ++i) {
            float* pfDest = reinterpret_cast<float*>(pucDest);
            const NiPoint3& rkSrc = pkVertex[pusVertexMap[i]];
            pfDest[0] = rkSrc.x;
            pfDest[1] = rkSrc.y;
            pfDest[2] = rkSrc.z;
            pucDest += uiVertexStride;
        }

        const float* pfWeights = apPartition->m_pfWeights;
        uint8_t* pucWeights = pucBuffer + uiWeightOffset;
        for (uint32_t i = 0; i < usVerts; ++i) {
            float fRemaining = 1.0f;
            for (uint32_t j = 0; j < usBones; ++j) {
                fRemaining -= pfWeights[j];
            }
            const float fError = std::fabs(fRemaining);
            if (fError > 1e-05f) {
                char acMessage[256];
                NiSprintf(
                    acMessage,
                    sizeof(acMessage),
                    "ILLEGAL WEIGHTS! Vertex %4d - Weight Sum %8.5f  (%8.5f)\n",
                    i,
                    1.0 - fRemaining,
                    static_cast<double>(fRemaining)
                );
            }
            float* pfDest = reinterpret_cast<float*>(pucWeights);
            uint16_t k = 0;
            for (; k < usBones - 1; ++k) {
                pfDest[k] = pfWeights[k];
            }
            if (usBones == 3 || usBones == 1) {
                pfDest[k] = pfWeights[k];
            }
            pucWeights += uiVertexStride;
            pfWeights += usBones;
        }
    }

    if (pkNormal && (auiDirtyFlags & NiGeometryData::NORMAL_MASK)) {
        uint8_t* pucDest = pucBuffer + uiNormOffset;
        for (uint32_t i = 0; i < usVerts; ++i) {
            float* pfDest = reinterpret_cast<float*>(pucDest);
            const NiPoint3& rkSrc = pkNormal[pusVertexMap[i]];
            pfDest[0] = rkSrc.x;
            pfDest[1] = rkSrc.y;
            pfDest[2] = rkSrc.z;
            pucDest += uiVertexStride;
        }
    }

    if (pkColor && (auiDirtyFlags & NiGeometryData::COLOR_MASK)) {
        uint8_t* pucDest = pucBuffer + uiColorOffset;
        for (uint32_t i = 0; i < usVerts; ++i) {
            const NiColorA& rkColor = pkColor[pusVertexMap[i]];
            const uint32_t uiA = NiFastFloatToInt(static_cast<float>(rkColor.a * 255.0));
            const uint32_t uiR = NiFastFloatToInt(static_cast<float>(rkColor.r * 255.0));
            const uint32_t uiG = NiFastFloatToInt(static_cast<float>(rkColor.g * 255.0));
            const uint32_t uiB = NiFastFloatToInt(static_cast<float>(rkColor.b * 255.0));
            *reinterpret_cast<uint32_t*>(pucDest) = ((uiA << 8 | uiR) << 8 | uiG) << 8 | uiB;
            pucDest += uiVertexStride;
        }
    }

    if (auiDirtyFlags & NiGeometryData::TEXTURE_MASK) {
        uint8_t* pucSet = pucBuffer + uiTexOffset;
        for (uint32_t uiSet = 0; uiSet < uiTextureSets; ++uiSet) {
            const NiPoint2* pkTexture = apData->GetTextureSet(static_cast<uint16_t>(uiSet));
            uint8_t* pucDest = pucSet;
            for (uint32_t i = 0; i < usVerts; ++i) {
                float* pfDest = reinterpret_cast<float*>(pucDest);
                const NiPoint2& rkSrc = pkTexture[pusVertexMap[i]];
                pfDest[0] = rkSrc.x;
                pfDest[1] = rkSrc.y;
                pucDest += uiVertexStride;
            }
            pucSet += sizeof(NiPoint2);
        }
    }

    if (bUnlock && !UnlockVB(pkVBChip->m_pkVB)) {
        return nullptr;
    }
    return pkVBChip;
}

// The vertex buffer is the locked one given, the old chip when its buffer still has this format and room, or a new
// chip (which then takes every component). A skinned mesh deforms its positions and normals into the buffer;
// colors pack as A8R8G8B8.
NiVBChip* NiDX9VertexBufferManager::PackUntransformedVB(
    NiGeometryData* apData,
    NiSkinInstance* apSkin,
    uint32_t auiDirtyFlags,
    NiVBChip* apOldVBChip,
    void* apvLockedBuff
) {
    const uint16_t usVerts = apData->GetActiveVertexCount();
    if (usVerts == 0) {
        return apOldVBChip;
    }

    const NiPoint3* pkNormal = apData->m_pkNormal;
    const NiPoint3* pkVertex = apData->m_pkVertex;
    uint32_t uiTextureSets = apData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
    const NiColorA* pkColor = apData->m_pkColor;
    if (uiTextureSets > NiD3DPass::ms_uiMaxTextureBlendStages) {
        uiTextureSets = NiD3DPass::ms_uiMaxTextureBlendStages;
    }

    uint32_t uiFVF = 0;
    uint32_t uiVertexStride = 0;
    uint32_t uiPosOffset;
    uint32_t uiWeightOffset;
    uint32_t uiNormOffset;
    uint32_t uiColorOffset;
    uint32_t uiTexOffset;
    GenerateVertexParameters(
        apData,
        nullptr,
        uiFVF,
        uiVertexStride,
        uiPosOffset,
        uiWeightOffset,
        uiNormOffset,
        uiColorOffset,
        uiTexOffset
    );

    NiGeometryBufferData* pkBuffData = apData->m_pkBuffData;
    pkBuffData->SetFVF(uiFVF);
    if (pkBuffData->m_uiStreamCount > 0) {
        pkBuffData->m_puiVertexStride[0] = uiVertexStride;
    }

    NiVBChip* pkVBChip;
    uint8_t* pucBuffer;
    bool bUnlock = true;
    if (apvLockedBuff) {
        pkVBChip = apOldVBChip;
        bUnlock = false;
        pucBuffer = static_cast<uint8_t*>(apvLockedBuff);
    }
    else {
        bool bReuse = false;
        if (apOldVBChip && apOldVBChip->m_pkVB) {
            // GetDesc's result is not checked; it fills the description.
            D3DVERTEXBUFFER_DESC kDesc;
            apOldVBChip->m_pkVB->GetDesc(&kDesc);
            bReuse = kDesc.Format == D3DFMT_VERTEXDATA && kDesc.FVF == uiFVF && kDesc.Size >= usVerts * uiVertexStride;
        }
        if (bReuse) {
            pkVBChip = apOldVBChip;
        }
        else {
            if (!AllocateBufferSpace(pkBuffData, 0)) {
                return nullptr;
            }
            pkVBChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
            auiDirtyFlags = NiGeometryData::DIRTY_MASK;
        }
        pucBuffer = static_cast<uint8_t*>(
            LockVB(pkVBChip->m_pkVB, pkVBChip->m_uiOffset, pkVBChip->m_uiSize, pkVBChip->m_uiLockFlags)
        );
    }

    if (apSkin) {
        if (pkVertex && (auiDirtyFlags & NiGeometryData::VERTEX_MASK)) {
            NiPoint3* pkDstNormal = pkNormal ? reinterpret_cast<NiPoint3*>(pucBuffer + uiNormOffset) : nullptr;
            apSkin->Deform(
                apData,
                reinterpret_cast<NiPoint3*>(pucBuffer + uiPosOffset),
                pkDstNormal,
                nullptr,
                nullptr,
                uiVertexStride
            );
        }
    }
    else {
        if (pkVertex && (auiDirtyFlags & NiGeometryData::VERTEX_MASK)) {
            PackVB(
                pucBuffer + uiPosOffset,
                reinterpret_cast<const uint8_t*>(pkVertex),
                uiVertexStride,
                sizeof(NiPoint3),
                usVerts
            );
        }
        if (pkNormal && (auiDirtyFlags & NiGeometryData::NORMAL_MASK)) {
            PackVB(
                pucBuffer + uiNormOffset,
                reinterpret_cast<const uint8_t*>(pkNormal),
                uiVertexStride,
                sizeof(NiPoint3),
                usVerts
            );
        }
    }

    if (pkColor && (auiDirtyFlags & NiGeometryData::COLOR_MASK)) {
        uint8_t* pucDest = pucBuffer + uiColorOffset;
        for (uint32_t i = 0; i < usVerts; ++i) {
            const uint32_t uiA = NiFastFloatToInt(static_cast<float>(pkColor->a * 255.0));
            const uint32_t uiR = NiFastFloatToInt(static_cast<float>(pkColor->r * 255.0));
            const uint32_t uiG = NiFastFloatToInt(static_cast<float>(pkColor->g * 255.0));
            const uint32_t uiB = NiFastFloatToInt(static_cast<float>(pkColor->b * 255.0));
            *reinterpret_cast<uint32_t*>(pucDest) = ((uiA << 8 | uiR) << 8 | uiG) << 8 | uiB;
            ++pkColor;
            pucDest += uiVertexStride;
        }
    }

    if (auiDirtyFlags & NiGeometryData::TEXTURE_MASK) {
        uint8_t* pucSet = pucBuffer + uiTexOffset;
        for (uint32_t uiSet = 0; uiSet < uiTextureSets; ++uiSet) {
            const NiPoint2* pkTexture = apData->GetTextureSet(static_cast<uint16_t>(uiSet));
            uint8_t* pucDest = pucSet;
            for (uint32_t i = 0; i < usVerts; ++i) {
                float* pfDest = reinterpret_cast<float*>(pucDest);
                pfDest[0] = pkTexture->x;
                pfDest[1] = pkTexture->y;
                ++pkTexture;
                pucDest += uiVertexStride;
            }
            pucSet += sizeof(NiPoint2);
        }
    }

    if (bUnlock && !UnlockVB(pkVBChip->m_pkVB)) {
        return nullptr;
    }
    return pkVBChip;
}

// Screen-space vertices: x and y from the positions, z 0 and rhw 1; then the optional A8R8G8B8 color and one texture
// set. The chip choice is PackUntransformedVB's, but a new chip keeps the dirty flags (and texture coordinates go
// with dirty bit 2).
NiVBChip* NiDX9VertexBufferManager::PackTransformedVB(
    NiGeometryBufferData* apBuffData,
    uint16_t ausVerts,
    const NiPoint2* apkPositions,
    const NiColorA* apkColors,
    const NiPoint2* apkTexCoords,
    uint32_t auiDirtyFlags,
    NiVBChip* apOldVBChip,
    void* apvLockedBuff
) {
    if (ausVerts == 0 || !apkPositions) {
        return apOldVBChip;
    }

    uint32_t uiFVF = D3DFVF_XYZRHW;
    uint32_t uiVertexStride = 4 * sizeof(float);
    uint32_t uiColorOffset = 0;
    if (apkColors) {
        uiColorOffset = uiVertexStride;
        uiFVF = static_cast<uint32_t>(D3DFVF_XYZRHW) | D3DFVF_DIFFUSE;
        uiVertexStride = 4 * sizeof(float) + sizeof(uint32_t);
    }
    uint32_t uiTexOffset = 0;
    if (apkTexCoords) {
        uiFVF |= D3DFVF_TEX1;
        uiTexOffset = uiVertexStride;
        uiVertexStride += sizeof(NiPoint2);
    }

    apBuffData->SetFVF(uiFVF);
    if (apBuffData->m_uiStreamCount > 0) {
        apBuffData->m_puiVertexStride[0] = uiVertexStride;
    }

    NiVBChip* pkVBChip;
    uint8_t* pucBuffer;
    bool bUnlock = true;
    if (apvLockedBuff) {
        pkVBChip = apOldVBChip;
        bUnlock = false;
        pucBuffer = static_cast<uint8_t*>(apvLockedBuff);
    }
    else {
        bool bReuse = false;
        if (apOldVBChip && apOldVBChip->m_pkVB) {
            // GetDesc's result is not checked; it fills the description.
            D3DVERTEXBUFFER_DESC kDesc;
            apOldVBChip->m_pkVB->GetDesc(&kDesc);
            bReuse = kDesc.Format == D3DFMT_VERTEXDATA && kDesc.FVF == uiFVF && kDesc.Size >= ausVerts * uiVertexStride;
        }
        if (bReuse) {
            pkVBChip = apOldVBChip;
        }
        else {
            if (!AllocateBufferSpace(apBuffData, 0)) {
                return nullptr;
            }
            pkVBChip = apBuffData->GetVBChip(0);
        }
        pucBuffer = static_cast<uint8_t*>(
            LockVB(pkVBChip->m_pkVB, pkVBChip->m_uiOffset, pkVBChip->m_uiSize, pkVBChip->m_uiLockFlags)
        );
    }

    if (auiDirtyFlags & NiGeometryData::VERTEX_MASK) {
        uint8_t* pucDest = pucBuffer;
        for (uint16_t i = 0; i < ausVerts; ++i) {
            float* pfDest = reinterpret_cast<float*>(pucDest);
            pfDest[0] = apkPositions[i].x;
            pfDest[1] = apkPositions[i].y;
            pfDest[2] = 0.0f;
            pfDest[3] = 1.0f;
            pucDest += uiVertexStride;
        }
    }

    if (apkColors && (auiDirtyFlags & NiGeometryData::COLOR_MASK)) {
        uint8_t* pucDest = pucBuffer + uiColorOffset;
        for (uint16_t i = 0; i < ausVerts; ++i) {
            const NiColorA& rkColor = apkColors[i];
            const uint32_t uiA = NiFastFloatToInt(static_cast<float>(rkColor.a * 255.0));
            const uint32_t uiR = NiFastFloatToInt(static_cast<float>(rkColor.r * 255.0));
            const uint32_t uiG = NiFastFloatToInt(static_cast<float>(rkColor.g * 255.0));
            const uint32_t uiB = NiFastFloatToInt(static_cast<float>(rkColor.b * 255.0));
            *reinterpret_cast<uint32_t*>(pucDest) = ((uiA << 8 | uiR) << 8 | uiG) << 8 | uiB;
            pucDest += uiVertexStride;
        }
    }

    if (apkTexCoords && (auiDirtyFlags & 2)) {
        uint8_t* pucDest = pucBuffer + uiTexOffset;
        for (uint16_t i = 0; i < ausVerts; ++i) {
            float* pfDest = reinterpret_cast<float*>(pucDest);
            pfDest[0] = apkTexCoords[i].x;
            pfDest[1] = apkTexCoords[i].y;
            pucDest += uiVertexStride;
        }
    }

    if (bUnlock && !UnlockVB(pkVBChip->m_pkVB)) {
        return nullptr;
    }
    return pkVBChip;
}

// Position first; three blend weights instead of the plain position flag when the partition has bones; then normal,
// diffuse color and the texture sets (one when the data has none, clamped to the blend stages otherwise; nothing
// more is written when the clamp leaves none). The weight offset is always set; the others only when present.
void NiDX9VertexBufferManager::GenerateVertexParameters(
    NiGeometryData* apData,
    const NiSkinPartition::Partition* apPartition,
    uint32_t& auiFVF,
    uint32_t& auiVertexStride,
    uint32_t& auiPosOffset,
    uint32_t& auiWeightOffset,
    uint32_t& auiNormOffset,
    uint32_t& auiColorOffset,
    uint32_t& auiTexOffset
) {
    auiFVF = 0;
    auiVertexStride = 0;
    const uint16_t usBones = apPartition ? apPartition->m_usBones : 0;
    auiPosOffset = 0;
    auiVertexStride += sizeof(NiPoint3);
    auiWeightOffset = auiVertexStride;
    if (usBones == 0) {
        auiFVF |= D3DFVF_XYZ;
    }
    else {
        auiFVF = D3DFVF_XYZB3;
        auiVertexStride += 3 * sizeof(float);
    }

    if (apData->m_pkNormal) {
        auiFVF |= D3DFVF_NORMAL;
        auiNormOffset = auiVertexStride;
        auiVertexStride += sizeof(NiPoint3);
    }
    if (apData->m_pkColor) {
        auiFVF |= D3DFVF_DIFFUSE;
        auiColorOffset = auiVertexStride;
        auiVertexStride += sizeof(uint32_t);
    }

    uint32_t uiTextureSets = apData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
    if (uiTextureSets == 0) {
        uiTextureSets = 1;
    }
    else {
        if (uiTextureSets > NiD3DPass::ms_uiMaxTextureBlendStages) {
            uiTextureSets = NiD3DPass::ms_uiMaxTextureBlendStages;
        }
        if (uiTextureSets == 0) {
            return;
        }
    }
    auiFVF |= uiTextureSets << D3DFVF_TEXCOUNT_SHIFT;
    auiTexOffset = auiVertexStride;
    auiVertexStride += uiTextureSets * sizeof(NiPoint2);
}

void NiDX9VertexBufferManager::PackVB(
    uint8_t* apucDest,
    const uint8_t* apucSrc,
    uint32_t auiVertexSize,
    uint32_t auiDataSize,
    uint32_t auiNumVerts
) {
    for (; auiNumVerts != 0; --auiNumVerts) {
        rt::memcpy(apucDest, apucSrc, auiDataSize);
        apucDest += auiVertexSize;
        apucSrc += auiDataSize;
    }
}

// Each default-pool resource releases its vertex buffer and is deleted, then its info; the map is emptied last. The
// info's resource pointers are not cleared.
void NiDX9VertexBufferManager::DestroyDefaultBuffers() {
    NiTMapItem<uint32_t, NiDX9VBInfo*>* pkPos = m_kDefaultResources.GetFirstPos();
    while (pkPos) {
        uint32_t uiVBID;
        NiDX9VBInfo* pkInfo;
        m_kDefaultResources.GetNext(pkPos, uiVBID, pkInfo);
        if (pkInfo) {
            for (NiDX9VBResource* pkRes : pkInfo->m_pkVBRes) {
                if (pkRes) {
                    if (pkRes->m_pkVB) {
                        pkRes->m_pkVB->Release();
                    }
                    delete pkRes;
                }
            }
            delete pkInfo;
        }
    }
    m_kDefaultResources.RemoveAll();
}
