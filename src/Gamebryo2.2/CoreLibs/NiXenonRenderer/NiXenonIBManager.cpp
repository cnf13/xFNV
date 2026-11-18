#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Resource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiDX9IndexBufferManager::NiDX9IndexBufferManager(IDirect3DDevice9* apkD3DDevice9)
  : m_pkD3DDevice9(apkD3DDevice9), m_pkTempPointConnIB(nullptr), m_uiTempPointConnLen(0), m_pkTempLineConnIB(nullptr),
    m_uiTempLineConnLen(0), m_kDefaultResources(37), m_kManagedResources(37), m_kSystemMemResources(37) {
    apkD3DDevice9->AddRef();
    m_kDefaultResources.RemoveAll();
    m_kManagedResources.RemoveAll();
    m_kSystemMemResources.RemoveAll();
}

// Frees every tracked resource and its info, then releases the shared buffers and the device; the maps go with their
// own destructors.
NiDX9IndexBufferManager::~NiDX9IndexBufferManager() {
    NiTPointerMap<uint32_t, NiDX9IBInfo*>* apkMaps[3] = {
        &m_kDefaultResources, &m_kManagedResources, &m_kSystemMemResources
    };
    for (NiTPointerMap<uint32_t, NiDX9IBInfo*>* pkMap : apkMaps) {
        NiTMapItem<uint32_t, NiDX9IBInfo*>* pkPos = pkMap->GetFirstPos();
        while (pkPos) {
            uint32_t uiIBID;
            NiDX9IBInfo* pkIBInfo;
            pkMap->GetNext(pkPos, uiIBID, pkIBInfo);
            if (pkIBInfo) {
                for (uint32_t j = 0; j < 5; ++j) {
                    NiDX9IBResource* pkIBRes = pkIBInfo->m_apkIBRes[j];
                    if (pkIBRes) {
                        if (pkIBRes->m_pkIB) {
                            pkIBRes->m_pkIB->Release();
                        }
                        delete pkIBRes;
                    }
                }
                delete pkIBInfo;
            }
        }
    }
    m_kDefaultResources.RemoveAll();
    m_kManagedResources.RemoveAll();
    m_kSystemMemResources.RemoveAll();
    m_pkD3DDevice9->SetIndices(nullptr);
    if (m_pkTempPointConnIB) {
        m_pkTempPointConnIB->Release();
    }
    if (m_pkTempLineConnIB) {
        m_pkTempLineConnIB->Release();
    }
    if (m_pkD3DDevice9) {
        m_pkD3DDevice9->Release();
        m_pkD3DDevice9 = nullptr;
    }
}

IDirect3DIndexBuffer9*
NiDX9IndexBufferManager::GetIndexBuffer(uint32_t auiLength, uint32_t auiUsage, D3DFORMAT aeFormat, D3DPOOL aePool) {
    if (!m_pkD3DDevice9) {
        return nullptr;
    }
    IDirect3DIndexBuffer9* pkD3DIB9 = nullptr;
    NiDX9IndexBufferManager::NiDX9IBInfo* IBInfo = GetIBInfo(auiUsage, aeFormat, aePool);
    if (IBInfo) {
        uint32_t v10;
        uint8_t m_ucIndex = IBInfo->m_ucIndex;
        NiDX9IndexBufferManager::NiDX9IBResource* v9 = IBInfo->m_apkIBRes[m_ucIndex];
        if (v9) {
            v10 = auiLength;
            if (v9->m_uiLength > auiLength) {
                v9->m_pkIB->Release();
                v9->m_pkIB = nullptr;
            }
            else {
                pkD3DIB9 = v9->m_pkIB;
                IBInfo->m_ucIndex = m_ucIndex + 1;
            }
        }
        else {
            NiDX9IndexBufferManager::NiDX9IBResource* v11 = static_cast<NiDX9IndexBufferManager::NiDX9IBResource*>(
                NiMemObject::operator new(sizeof(NiDX9IndexBufferManager::NiDX9IBResource))
            );
            v9 = v11;
            if (v11) {
                ::new (v11) NiDX9IndexBufferManager::NiDX9IBResource;
            }
            else {
                v9 = nullptr;
            }
            v9->m_ePool = aePool;
            v10 = auiLength;
            v9->m_uiUsage = auiUsage;
            v9->m_eType = NiDX9Resource::TYPE_INDEXBUFFER;
            v9->m_uiLength = auiLength;
            v9->m_eFormat = aeFormat;
            v9->m_uiRevID = 0;
            v9->m_pkIB = nullptr;
        }
        if (!v9->m_pkIB) {
            IDirect3DIndexBuffer9* IndexBuffer = CreateIndexBuffer(v10, auiUsage, aeFormat, aePool, &v9->m_kIBDesc);
            v9->m_pkIB = IndexBuffer;
            v9->m_uiLength = v10;
            v9->m_eFormat = aeFormat;
            v9->m_uiRevID = 0;
            ++IBInfo->m_ucIndex;
            pkD3DIB9 = IndexBuffer;
        }
        if (IBInfo->m_ucIndex == 5) {
            IBInfo->m_ucIndex = 0;
        }
    }
    return pkD3DIB9;
}

// Keeps the old buffer when it holds auiPoints sprites, but creates and fills auiMaxPoints. The error texts are the
// reference source's.
IDirect3DIndexBuffer9* NiDX9IndexBufferManager::ResizeTempPointConnectivity(
    uint32_t auiPoints,
    uint32_t auiMaxPoints,
    bool abRefill,
    uint32_t auiUsage,
    D3DPOOL aePool
) {
    if (!m_pkD3DDevice9 || !auiPoints) {
        return nullptr;
    }
    IDirect3DIndexBuffer9* pkD3DIB9 = nullptr;
    uint32_t uiSize = auiMaxPoints * sizeof(uint16_t) * 6;
    IDirect3DIndexBuffer9* pkOldIB = m_pkTempPointConnIB;
    if (pkOldIB) {
        D3DINDEXBUFFER_DESC kIBDesc = {};
        if (pkOldIB->GetDesc(&kIBDesc) >= 0) {
            if (kIBDesc.Format == D3DFMT_INDEX16 && kIBDesc.Type == D3DRTYPE_INDEXBUFFER && kIBDesc.Usage == auiUsage &&
                kIBDesc.Pool == aePool && kIBDesc.Size >= auiPoints * sizeof(uint16_t) * 6) {
                pkD3DIB9 = pkOldIB;
                if (!abRefill) {
                    return pkD3DIB9;
                }
            }
            else {
                // EXE-INLINED none: NiD3DRenderer::ReleaseIBResource
                pkOldIB->Release();
            }
        }
    }
    if (!pkD3DIB9) {
        if (m_pkD3DDevice9->CreateIndexBuffer(uiSize, auiUsage, D3DFMT_INDEX16, aePool, &pkD3DIB9, nullptr) < 0) {
            NiRenderer::Error("NiDX9IndexBufferManager::CreateIndexBuffer> Failed to create index buffer\n");
            pkD3DIB9 = nullptr;
        }
        if (!pkD3DIB9) {
            NiRenderer::Error(
                "NiDX9IndexBufferManager::ResizeTempPointConnectivity> Failed to create index "
                "buffer\n"
            );
            return nullptr;
        }
    }
    void* pvData = nullptr;
    if (pkD3DIB9->Lock(0, uiSize, &pvData, 0) >= 0) {
        uint16_t* pusTemp = static_cast<uint16_t*>(pvData);
        uint32_t uiBase = 0;
        for (uint32_t ui = 0; ui < auiMaxPoints; ui++) {
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 0);
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 1);
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 2);
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 0);
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 2);
            *(pusTemp++) = static_cast<uint16_t>(uiBase + 3);
            uiBase += 4;
        }
        pkD3DIB9->Unlock();
    }
    else {
        NiRenderer::Error("NiDX9IndexBufferManager::PackBuffer> Failed to lock index buffer\n");
        pkD3DIB9->Unlock();
        // EXE-INLINED none: NiD3DRenderer::ReleaseIBResource
        pkD3DIB9->Release();
        pkD3DIB9 = nullptr;
    }
    m_pkTempPointConnIB = pkD3DIB9;
    m_uiTempPointConnLen = uiSize;
    return pkD3DIB9;
}

// Builds a segment per flagged point, the last one closing the loop to index 0.
IDirect3DIndexBuffer9* NiDX9IndexBufferManager::ResizeTempLineConnectivity(
    uint32_t auiPoints,
    bool* apbFlags,
    uint32_t* apuiConn,
    uint32_t auiUsage,
    D3DPOOL aePool
) {
    if (!m_pkD3DDevice9) {
        return nullptr;
    }
    if (!auiPoints || !apbFlags) {
        return nullptr;
    }
    IDirect3DIndexBuffer9* pkD3DIB9 = nullptr;
    uint32_t uiSize = (auiPoints * 2 + 2) * sizeof(uint16_t);
    IDirect3DIndexBuffer9* pkOldIB = m_pkTempLineConnIB;
    if (pkOldIB) {
        D3DINDEXBUFFER_DESC kIBDesc = {};
        if (pkOldIB->GetDesc(&kIBDesc) >= 0) {
            if (kIBDesc.Format == D3DFMT_INDEX16 && kIBDesc.Type == D3DRTYPE_INDEXBUFFER && kIBDesc.Usage == auiUsage &&
                kIBDesc.Pool == aePool && kIBDesc.Size >= uiSize) {
                pkD3DIB9 = pkOldIB;
            }
            else {
                // EXE-INLINED none: NiD3DRenderer::ReleaseIBResource
                pkOldIB->Release();
            }
        }
    }
    if (!pkD3DIB9) {
        pkD3DIB9 = CreateIndexBuffer(uiSize, auiUsage, D3DFMT_INDEX16, aePool, nullptr);
        if (!pkD3DIB9) {
            NiRenderer::Error(
                "NiDX9IndexBufferManager::ResizeTempLineConnectivity> Failed to create index "
                "buffer\n"
            );
            return nullptr;
        }
    }
    void* pvData = nullptr;
    *apuiConn = 0;
    if (pkD3DIB9->Lock(0, uiSize, &pvData, 0) >= 0) {
        uint16_t* pusConnTmp = static_cast<uint16_t*>(pvData);
        uint32_t uiMax = auiPoints - 1;
        for (uint32_t i = 0; i < uiMax; i++) {
            if (*apbFlags) {
                pusConnTmp[0] = static_cast<uint16_t>(i);
                pusConnTmp[1] = static_cast<uint16_t>(i + 1);
                pusConnTmp += 2;
                *apuiConn += 2;
            }
            apbFlags++;
        }
        if (*apbFlags) {
            pusConnTmp[0] = static_cast<uint16_t>(uiMax);
            pusConnTmp[1] = 0;
            *apuiConn += 2;
        }
        pkD3DIB9->Unlock();
    }
    else {
        NiRenderer::Error("NiDX9IndexBufferManager::PackBuffer> Failed to lock index buffer\n");
        pkD3DIB9->Unlock();
        // EXE-INLINED none: NiD3DRenderer::ReleaseIBResource
        pkD3DIB9->Release();
        pkD3DIB9 = nullptr;
    }
    m_pkTempLineConnIB = pkD3DIB9;
    m_uiTempLineConnLen = uiSize;
    return pkD3DIB9;
}

// Each info's resources release their index buffer before the base destructor runs.
void NiDX9IndexBufferManager::DestroyDefaultBuffers() {
    NiTMapItem<uint32_t, NiDX9IBInfo*>* pkPos = m_kDefaultResources.GetFirstPos();
    while (pkPos) {
        uint32_t uiIBID;
        NiDX9IBInfo* pkIBInfo;
        m_kDefaultResources.GetNext(pkPos, uiIBID, pkIBInfo);
        if (pkIBInfo) {
            for (uint32_t i = 0; i < 5; ++i) {
                NiDX9IBResource* pkIBRes = pkIBInfo->m_apkIBRes[i];
                if (pkIBRes) {
                    if (pkIBRes->m_pkIB) {
                        pkIBRes->m_pkIB->Release();
                    }
                    pkIBRes->NiDX9Resource::~NiDX9Resource();
                    NiMemObject::operator delete(pkIBRes, sizeof(NiDX9IBResource));
                }
            }
            NiMemObject::operator delete(pkIBInfo, sizeof(NiDX9IBInfo));
        }
    }
    m_kDefaultResources.RemoveAll();
    if (m_pkTempPointConnIB) {
        m_pkTempPointConnIB->Release();
        m_pkTempPointConnIB = nullptr;
    }
    if (m_pkTempLineConnIB) {
        m_pkTempLineConnIB->Release();
        m_pkTempLineConnIB = nullptr;
    }
}

// The key packs the usage above the format; the pool picks the map. A new entry starts with no resources.
NiDX9IndexBufferManager::NiDX9IBInfo*
NiDX9IndexBufferManager::GetIBInfo(uint32_t uiUsage, D3DFORMAT eFormat, D3DPOOL ePool) {
    uint32_t uiKey = (uiUsage << 16) | eFormat;
    NiDX9IBInfo* pkIBInfo = nullptr;
    NiTPointerMap<uint32_t, NiDX9IBInfo*>* pkMap;
    switch (ePool) {
        case D3DPOOL_DEFAULT: pkMap = &m_kDefaultResources; break;
        case D3DPOOL_MANAGED: pkMap = &m_kManagedResources; break;
        case D3DPOOL_SYSTEMMEM: pkMap = &m_kSystemMemResources; break;
        default: return nullptr;
    }
    if (pkMap->GetAt(uiKey, pkIBInfo)) {
        return pkIBInfo;
    }
    NiDX9IBInfo* pkNewInfo = static_cast<NiDX9IBInfo*>(NiMemObject::operator new(sizeof(NiDX9IBInfo)));
    pkNewInfo->m_uiIBID = 0;
    pkNewInfo->m_ucIndex = 0;
    pkNewInfo->m_apkIBRes[0] = nullptr;
    pkNewInfo->m_apkIBRes[1] = nullptr;
    pkNewInfo->m_apkIBRes[2] = nullptr;
    pkNewInfo->m_apkIBRes[3] = nullptr;
    pkNewInfo->m_apkIBRes[4] = nullptr;
    pkNewInfo->m_uiIBID = uiKey;
    pkMap->SetAt(uiKey, pkNewInfo);
    return pkNewInfo;
}
