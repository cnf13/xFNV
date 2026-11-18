#include "BSShader/TallGrassShaderProperty.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/GrassLoadTask.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/TallGrassGroup.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

NiRTTI TallGrassShaderProperty::ms_RTTI("TallGrassShaderProperty", &BSShaderLightingProperty::ms_RTTI);

TallGrassShaderProperty::GrassParam::GrassParam()
  : pcGeomFilename(nullptr), fPositionRange(0.0f), fHeightRange(0.0f), fColorRange(0.0f), fWaveOffsetRange(0.0f),
    fWavePeriod(1.0f) {}

TallGrassShaderProperty::CachedGeometry::CachedGeometry() {}

// The members release themselves (the load task through its own count).
TallGrassShaderProperty::CachedGeometry::~CachedGeometry() {}

// The terrain chunk map has 101 buckets.
TallGrassShaderProperty::CachedGrass::CachedGrass() : pLowTerrainChunkMap(101) {}

void TallGrassShaderProperty::CachedGrass::Free() {
    spCachedLowGeometry = nullptr;
    pLowTerrainChunkMap.RemoveAll();
    NiTListItem<TallGrassGroup*>* m_pkHead = pLowGrassGroupList.m_pkHead;
    while (m_pkHead) {
        TallGrassGroup* m_element = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (m_element) {
            m_element->~TallGrassGroup();
            MemoryManager_Singleton.Deallocate(m_element);
        }
    }
    NiTPointerList<TallGrassGroup*>* p_pLowGrassGroupList = &pLowGrassGroupList;
    pLowGrassGroupList.RemoveAll();
    spCachedLowGeometry = nullptr;
    pLowTerrainChunkMap.RemoveAll();
    TallGrassShaderProperty::CachedGeometry* v7 = spCachedLowGeometry.m_pObject;
    if (v7) {
        // EXE-INLINED none: NiDecRefCount
        NiDecRefCount(v7);
    }
    p_pLowGrassGroupList->RemoveAll();
}

TallGrassShaderProperty::TallGrassShaderProperty() : pGrassGroup(nullptr) {
    iShaderPropertyType = 4;
}

// The terrain chunk, the texture filename and the base texture release themselves.
TallGrassShaderProperty::~TallGrassShaderProperty() {
    pGrassGroup = nullptr;
}

// Builds the clone with the grass heap context selected.
NiObject* TallGrassShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_GRASS;
    TallGrassShaderProperty* pClone = nullptr;
    void* pMemory = NiMemObject::operator new(sizeof(TallGrassShaderProperty));
    if (pMemory) {
        pClone = ::new (pMemory) TallGrassShaderProperty;
    }
    this->CopyMembers(pClone, arCloneData);
    GetThreadMemContext() = iOldMemContext;
    return pClone;
}

void TallGrassShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderLightingProperty::LoadBinary(kStream);
    char* pcName = nullptr;
    kStream.LoadCString(pcName);
    TextureFilename.Set(pcName, 0);
    _NiFree(pcName);
}

void TallGrassShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderLightingProperty::LinkObject(kStream);
}

bool TallGrassShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderLightingProperty::RegisterStreamables(kStream);
}

void TallGrassShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderLightingProperty::SaveBinary(kStream);
    kStream.SaveCString(TextureFilename.pString);
}

void TallGrassShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderLightingProperty::GetViewerStrings(apStrings);
}

// BSShaderLightingProperty has no PostLinkObject of its own, so this reaches BSShaderProperty's.
void TallGrassShaderProperty::PostLinkObject(NiStream& kStream) {
    BSShaderProperty::PostLinkObject(kStream);
    if (TextureFilename.pString) {
        BSShaderManager::GetTexture(TextureFilename.pString, true, spBaseTexture, true, false);
    }
}

// The light iterator is a plain list position (void*), zeroed before the light queries.
BSShaderProperty::RenderPassArray* TallGrassShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    TLSData* v8 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v8->eMemContext;
    TLSData* v24 = v8;
    v8->eMemContext = MEM_CONTEXT_GRASS;
    uint32_t uiLastRenderPassState = this->iLastRenderPassState;
    if (uiLastRenderPassState != aiEnabledPasses) {
        uint16_t inumlights;
        NiGeometry* v15;
        BSShaderProperty::RenderPassArray* pRenderPassArray;
        uint16_t v17;
        CheckCreateRenderPassArray(2);
        this->pRenderPassList->iSize = 0;
        void* kIter = nullptr;
        if (TallGrassShaderProperty::bPointLighting) {
            inumlights = GetNumberofActiveNonShadowLights();
        }
        else {
            inumlights = 0;
        }
        bool v12 = (ulFlags[1] & 1024) != 0;
        bool bFitSlope = (ulFlags[1] & BSS2_Fit_Slope) != 0;
        bool bBillboard = (ulFlags[1] & BSS2_Billboard_and_Envmap_Light_Fade) != 0;
        bool v13 = false;
        if ((!BSShaderManager::bInterior) && (BSShaderManager::pProjectedShadowTexture.m_pObject) &&
            ((BSShaderManager::iEnabledPasses & 32) != 0 && BSShaderManager::eShaderLevel >= 2) &&
            (static_cast<int32_t>(static_cast<int64_t>(ShadowLightShader::kShadowProjTransform.w)) != 0)) {
            v13 = true;
        }

        int32_t v14 = (inumlights != 0 ? 20 : 0) + (BSShaderManager::bHDR ? 582 : 0);
        if (v12) {
            v15 = apGeom;
            pRenderPassArray = this->pRenderPassList;
            if (bFitSlope) {
                if (v13) {
                    v17 = v14 + 33;
                }
                else {
                    v17 = v14 + 25;
                }
            }
            else {
                if (v13) {
                    v17 = v14 + 31;
                }
                else {
                    v17 = v14 + 23;
                }
            }
        }
        else {
            if (bFitSlope) {
                if (!bBillboard) {
                    v15 = apGeom;
                    pRenderPassArray = this->pRenderPassList;
                    if (v13) {
                        v17 = v14 + 32;
                    }
                    else {
                        v17 = v14 + 24;
                    }
                    goto LABEL_32;
                }
                v17 = v14 + 39;
            }
            else {
                if (bBillboard) {
                    v17 = v14 + 38;
                }
                else {
                    if (v13) {
                        v17 = v14 + 30;
                    }
                    else {
                        v17 = v14 + 22;
                    }
                }
            }
            pRenderPassArray = this->pRenderPassList;
            v15 = apGeom;
        }
    LABEL_32:;
        pRenderPassArray->Add(v15, v17, 1, 0, nullptr, nullptr, nullptr, nullptr);
        if (inumlights) {
            this->pRenderPassList->pPassA.m_pBase[this->pRenderPassList->iSize - 1]->SetLights(
                1, GetFirstActiveNonShadowLight(kIter)
            );
        }
        if (BSShaderManager::eShaderLevel >= 3 && (BSShaderManager::iEnabledPasses & 16) != 0) {
            for (ShadowSceneLight* psslight = GetFirstActiveLight(kIter); psslight;
                 psslight = GetNextActiveLight(kIter)) {
                if (psslight->bCastShadow) {
                    BSShaderProperty::RenderPassArray* v19 = this->pRenderPassList;
                    if (bFitSlope) {
                        if (bBillboard) {
                            v19->Add(v15, 647, 0, 1, psslight, nullptr, nullptr, nullptr);
                        }
                        else {
                            v19->Add(v15, 645, 0, 1, psslight, nullptr, nullptr, nullptr);
                        }
                    }
                    else {
                        if (bBillboard) {
                            v19->Add(v15, 646, 0, 1, psslight, nullptr, nullptr, nullptr);
                        }
                        else {
                            v19->Add(v15, 644, 0, 1, psslight, nullptr, nullptr, nullptr);
                        }
                    }
                }
            }
        }
        this->iLastRenderPassState = aiEnabledPasses | auiRenderMode << 8;
        v8 = v24;
    }
    BSShaderProperty::RenderPassArray* result = this->pRenderPassList;
    v8->eMemContext = eMemContext;
    return result;
}

// D3DTADDRESS_CLAMP (3) selects clamp, anything else wrap.
void TallGrassShaderProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode =
        (aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T : NiTexturingProperty::WRAP_S_WRAP_T;
}

// Copies this property's members into pDest while cloning.
void TallGrassShaderProperty::CopyMembers(TallGrassShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
    pDest->spTerrainChunk = spTerrainChunk;
    pDest->spBaseTexture = spBaseTexture;
    pDest->TextureClampMode = TextureClampMode;
    pDest->pGrassParam = pGrassParam;
    pDest->pGrassGroup = pGrassGroup;
}

NiTPointerMap<uint32_t, TallGrassShaderProperty::CachedGrass*>* TallGrassShaderProperty::GetCachedGrassMap() {
    static pmGrassMap_t pmGrassMap;
    return &pmGrassMap;
}

// Looks the grass up in the cached grass map, then the terrain chunk in that grass's own chunk map.
bool TallGrassShaderProperty::ContainsChunk(GrassParam* apGrassParam, int32_t alTerrainChunkSeed) {
    CachedGrass* pCachedGrass = nullptr;
    NiTPointerMap<uint32_t, CachedGrass*>* pGrassMap = GetCachedGrassMap();
    if (!pGrassMap->GetAt(apGrassParam->iGrassID, pCachedGrass)) {
        return false;
    }
    int32_t iChunkIndex;
    return pCachedGrass->pLowTerrainChunkMap.GetAt(alTerrainChunkSeed, iChunkIndex);
}

void TallGrassShaderProperty::SetUseBackgroundLoading(bool abUseBackgroundLoading) {
    TallGrassShaderProperty::bUseBackgroundLoading = (bool)((uint8_t)abUseBackgroundLoading);
}

void TallGrassShaderProperty::ClearStaticMaps() {
    NiTMapItem<const char*, NiPointer<TallGrassShaderProperty::CachedGeometry>>* kiter;
    TallGrassShaderProperty::RemoveAllGrass();
    NiTStringPointerMap<NiPointer<TallGrassShaderProperty::CachedGeometry>>* CachedGeometryMap =
        TallGrassShaderProperty::GetCachedGeometryMap();
    uint32_t m_uiHashSize = CachedGeometryMap->m_uiHashSize;
    uint32_t v2 = 0;
    if (m_uiHashSize) {
        NiTMapItem<const char*, NiPointer<TallGrassShaderProperty::CachedGeometry>>** m_ppkHashTable =
            CachedGeometryMap->m_ppkHashTable;
        NiTMapItem<const char*, NiPointer<TallGrassShaderProperty::CachedGeometry>>** v4 = m_ppkHashTable;
        while (!*v4) {
            ++v2;
            ++v4;
            if (v2 >= m_uiHashSize) {
                goto LABEL_5;
            }
        }
        kiter = m_ppkHashTable[v2];
    }
    else {
    LABEL_5:;
        kiter = nullptr;
    }
    NiTMapItem<const char*, NiPointer<TallGrassShaderProperty::CachedGeometry>>* kiter_2 = kiter;
    // The last geometry GetNext handed out is released when spCache goes out of scope.
    NiPointer<TallGrassShaderProperty::CachedGeometry> spCache;
    if (TallGrassShaderProperty::GetCachedGeometryMap()->m_kAllocator.m_uiCount) {
        if (kiter) {
            do {
                const char* pckey;
                TallGrassShaderProperty::GetCachedGeometryMap()->GetNext(kiter_2, pckey, spCache);
                TallGrassShaderProperty::CachedGeometry* m_pObject = spCache.m_pObject;
                if ((m_pObject) && (!m_pObject->bQueued)) {
                    m_pObject->spTriGeomData = nullptr;
                    MemoryManager_Singleton.Deallocate(m_pObject->pInstanceIndex);
                    m_pObject->pInstanceIndex = nullptr;
                    m_pObject->spBaseTexture = nullptr;
                    m_pObject->spAlphaNoBlend = nullptr;
                    m_pObject->spAlphaBlend = nullptr;
                    m_pObject->spStencil = nullptr;
                }
            }
            while (kiter_2);
        }
        TallGrassShaderProperty::GetCachedGeometryMap()->RemoveAll();
    }
}

int32_t TallGrassShaderProperty::Hash_2DPosition(NiPoint3& arPos) {
    int32_t v2 = (int32_t)FloatFloor((&arPos)->x);
    return ((uint16_t)((int32_t)FloatFloor((&arPos)->y))) | (v2 << 16);
}

void TallGrassShaderProperty::SetFadeParameters(
    float afLowMinFade,
    float afLowMin,
    float afLowMax,
    float afLowMaxFade
) {
    TallGrassShaderProperty::kFadeParams.x = afLowMinFade;
    TallGrassShaderProperty::kFadeParams.y = (afLowMin - afLowMinFade);
    TallGrassShaderProperty::kFadeParams.z = afLowMax;
    TallGrassShaderProperty::kFadeParams.w = (afLowMaxFade - afLowMax);
}

// The stream's factory: a property built in freshly allocated memory, or null.
NiObject* TallGrassShaderProperty::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(TallGrassShaderProperty));
    if (pMemory) {
        return ::new (pMemory) TallGrassShaderProperty;
    }
    return nullptr;
}

// Builds the guarded static pmCachedGeometryMap on first use.
NiTStringPointerMap<NiPointer<TallGrassShaderProperty::CachedGeometry>>*
TallGrassShaderProperty::GetCachedGeometryMap() {
    static pmCachedGeometryMap_t pmCachedGeometryMap;
    return &pmCachedGeometryMap;
}

// The cached geometry of apGrassParam's model, loading it on first use: one dynamic geometry holding as many copies of
// the model's first geometry as one draw can index, with each copy's instance number per vertex. Null while the model's
// background load runs. Smart-pointer locals are released before the thread's memory context is restored.
// KEEP: on a cache miss the exe copies SetAt's by-value NiPointer argument (InterlockedIncrement at 0xB61AA2) before
// calling GetCachedGeometryMap (0xB61AB7); C++17 evaluates the object expression of the call first, so src cannot
// express the exe's order.
TallGrassShaderProperty::CachedGeometry* TallGrassShaderProperty::GetCachedGeometry(GrassParam* apGrassParam) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_GRASS;
    if (!BSShaderManager::eShaderLevel || !apGrassParam->pcGeomFilename) {
        pTLSData->eMemContext = eMemContext;
        return nullptr;
    }
    XFNV_INLINED_LOCAL(NiPointer<CachedGeometry>, spCachedGeom, std::nullopt, std::nullopt);
    spCachedGeom.m_pObject = nullptr;
    if (!GetCachedGeometryMap()->GetAt(apGrassParam->pcGeomFilename, spCachedGeom)) {
        void* pMemory = NiMemObject::operator new(sizeof(CachedGeometry));
        CachedGeometry* pNewGeom = pMemory ? ::new (pMemory) CachedGeometry : nullptr;
        spCachedGeom.SetNew(pNewGeom);
        CachedGeometry* pCachedGeom = spCachedGeom.m_pObject;
        pCachedGeom->bQueued = true;
        pCachedGeom->spGrassLoadTask = nullptr;
        // The map gets its own reference: a copy of the smart pointer, passed by value.
        GetCachedGeometryMap()->SetAt(apGrassParam->pcGeomFilename, spCachedGeom);
        if (bUseBackgroundLoading) {
            pMemory = MemoryManager_Singleton.Allocate(sizeof(GrassLoadTask));
            GrassLoadTask* pTask = pMemory ? ::new (pMemory) GrassLoadTask(apGrassParam->pcGeomFilename) : nullptr;
            pCachedGeom->spGrassLoadTask = pTask;
            pIOManager->AddTask(pCachedGeom->spGrassLoadTask.m_pObject);
            spCachedGeom.~NiPointer<CachedGeometry>();
            pTLSData->eMemContext = eMemContext;
            return nullptr;
        }
    }
    CachedGeometry* pCachedGeom = spCachedGeom.m_pObject;
    if (pCachedGeom->bQueued) {
        XFNV_INLINED_LOCAL(NiPointer<NiObject>, spObject, std::nullopt, std::nullopt);
        spObject.m_pObject = nullptr;
        if (bUseBackgroundLoading && !pCachedGeom->spGrassLoadTask.m_pObject) {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(GrassLoadTask));
            GrassLoadTask* pTask = pMemory ? ::new (pMemory) GrassLoadTask(apGrassParam->pcGeomFilename) : nullptr;
            pCachedGeom->spGrassLoadTask = pTask;
            pIOManager->AddTask(pCachedGeom->spGrassLoadTask.m_pObject);
            spObject.~NiPointer<NiObject>();
            spCachedGeom.~NiPointer<CachedGeometry>();
            pTLSData->eMemContext = eMemContext;
            return nullptr;
        }
        GrassLoadTask* pTask = pCachedGeom->spGrassLoadTask.m_pObject;
        if (pTask) {
            // The background load has not finished yet.
            if (pTask->eState < BS_TASK_STATE_FINISHED) {
                spObject.~NiPointer<NiObject>();
                spCachedGeom.~NiPointer<CachedGeometry>();
                pTLSData->eMemContext = eMemContext;
                return nullptr;
            }
            spObject = pTask->GetObject();
            pCachedGeom->spGrassLoadTask = nullptr;
        }
        else {
            char acPath[260];
            StandardizeModelPath(apGrassParam->pcGeomFilename, acPath, sizeof(acPath));
            BSStream kStream;
            kStream.BSStream::Load(acPath, nullptr);
            if (kStream.m_kTopObjects.m_uiSize) {
                spObject = kStream.m_kTopObjects.m_pBase[0].m_pObject;
            }
        }
        if (!spObject.m_pObject) {
            spObject.~NiPointer<NiObject>();
            spCachedGeom.~NiPointer<CachedGeometry>();
            pTLSData->eMemContext = eMemContext;
            return nullptr;
        }
        pCachedGeom->bQueued = false;
        NiAVObject* pGrassObject =
            static_cast<NiAVObject*>(spObject.m_pObject->DynamicCastMember(&NiAVObject::ms_RTTI));
        NiNode* pGrassNode = pGrassObject->IsNode();
        // The root's first geometry child; the root keeps being scanned while it has none.
        NiGeometry* pGrassGeom = nullptr;
        while (pGrassNode) {
            for (uint32_t i = 0; i < pGrassNode->m_kChildren.m_usSize; ++i) {
                NiAVObject* pChild = pGrassNode->m_kChildren.m_pBase[i].m_pObject;
                if (pChild) {
                    pGrassGeom = pChild->IsGeometry();
                    if (pGrassGeom) {
                        break;
                    }
                }
            }
            if (pGrassGeom) {
                break;
            }
        }
        NiTriStrips* pTriStrips = pGrassGeom->IsTriStrips();
        uint16_t usMaxInstanceCount = BSShaderManager::eShaderLevel < 2 ? 80 : 228;
        bool bIsStrips;
        int32_t iVertices;
        NiPoint3* pkVertex;
        NiPoint3* pkNormal;
        NiColorA* pkColor;
        NiPoint2* pkTexture;
        float* pfInstanceIndex;
        uint16_t usInstanceTriCount;
        NiTriBasedGeomData* pTriData;
        if (!pTriStrips) {
            // One copy of the shape per instance, its triangle list offset to that copy's vertices.
            NiTriShapeData* pShapeData = static_cast<NiTriShapeData*>(pGrassGeom->m_spModelData.m_pObject);
            iVertices = pShapeData->m_usVertices;
            const NiPoint3* pkSrcVertex = pShapeData->m_pkVertex;
            uint32_t uiTriListLength = pShapeData->m_uiTriListLength;
            bIsStrips = false;
            const uint16_t* pusTriList = pShapeData->GetTriList();
            // The vertex and index counts of all copies stay within 16 bits.
            if (usMaxInstanceCount >= static_cast<uint16_t>(0xFFFF / iVertices)) {
                usMaxInstanceCount = static_cast<uint16_t>(0xFFFF / iVertices);
            }
            int32_t iTriListLength = static_cast<uint16_t>(uiTriListLength);
            if (usMaxInstanceCount >= static_cast<uint16_t>(0xFFFF / iTriListLength)) {
                usMaxInstanceCount = static_cast<uint16_t>(0xFFFF / iTriListLength);
            }
            uint32_t uiTotalVertices = usMaxInstanceCount * iVertices;
            // The array allocations ask for 0xFFFFFFFF bytes where the size overflows 32 bits.
            uint64_t ullBytes = uint64_t(uiTotalVertices) * sizeof(NiPoint3);
            pkVertex = static_cast<NiPoint3*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            pkNormal = static_cast<NiPoint3*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            ullBytes = uint64_t(uiTotalVertices) * sizeof(NiColorA);
            void* pColorMemory =
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            if (pColorMemory) {
                rt::eh_vector_constructor_iterator(
                    pColorMemory, sizeof(NiColorA), uiTotalVertices, &xfnv::constructElement<NiColorA>
                );
                pkColor = static_cast<NiColorA*>(pColorMemory);
            }
            else {
                pkColor = nullptr;
            }
            ullBytes = uint64_t(uiTotalVertices) * sizeof(NiPoint2);
            pkTexture = static_cast<NiPoint2*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            ullBytes = uint64_t(uiTotalVertices) * sizeof(float);
            pfInstanceIndex = static_cast<float*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            ullBytes = uint64_t(uint32_t(iTriListLength * usMaxInstanceCount)) * sizeof(uint16_t);
            uint16_t* pusIndices = static_cast<uint16_t*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            const NiColorA* pkSrcColor = pShapeData->m_pkColor;
            const NiPoint2* pkSrcTexture = pShapeData->m_pkTexture;
            const NiPoint3* pkSrcNormal = pShapeData->m_pkNormal;
            int32_t iIndex = 0;
            for (int32_t k = 0; k < usMaxInstanceCount; ++k) {
                if (iVertices > 0) {
                    float fInstance = static_cast<float>(k);
                    int32_t iBase = k * iVertices;
                    for (int32_t j = 0; j < iVertices; ++j) {
                        pkVertex[iBase + j] = pkSrcVertex[j];
                        pkNormal[iBase + j] = pkSrcNormal[j];
                        pkColor[iBase + j] = pkSrcColor[j];
                        pkTexture[iBase + j] = pkSrcTexture[j];
                        pfInstanceIndex[iBase + j] = fInstance;
                    }
                }
                for (int32_t i = 0; i < iTriListLength; ++i) {
                    pusIndices[iIndex++] = static_cast<uint16_t>(pusTriList[i] + k * iVertices);
                }
            }
            usInstanceTriCount = static_cast<uint16_t>(iTriListLength / 3);
            void* pDataMemory = NiMemObject::operator new(sizeof(NiTriShapeDynamicData));
            pTriData = pDataMemory ? ::new (pDataMemory) NiTriShapeDynamicData(
                                         usMaxInstanceCount * iVertices, pkVertex, pkNormal, pkColor, pkTexture, 1,
                                         NiGeometryData::NBT_METHOD_NONE, usMaxInstanceCount * (iTriListLength / 3),
                                         pusIndices, usMaxInstanceCount * iVertices, 0
                                     )
                                   : nullptr;
        }
        else {
            // One copy of the (single) strip per instance, each joined to the next by degenerate triangles: the
            // copy's last index and the next copy's first, that one twice after an odd-length strip so that the
            // next copy keeps its winding.
            NiTriStripsData* pStripsData = static_cast<NiTriStripsData*>(pGrassGeom->m_spModelData.m_pObject);
            iVertices = pStripsData->m_usVertices;
            const uint16_t* pusStripLengths = pStripsData->m_pusStripLengths;
            const NiPoint3* pkSrcVertex = pStripsData->m_pkVertex;
            bIsStrips = true;
            const uint16_t* pusStripLists = pStripsData->GetStripLists();
            // The vertex and index counts of all copies stay within 16 bits.
            if (usMaxInstanceCount >= static_cast<uint16_t>(0xFFFF / iVertices)) {
                usMaxInstanceCount = static_cast<uint16_t>(0xFFFF / iVertices);
            }
            int32_t iJoinedLength = pusStripLengths[0] + (pusStripLengths[0] % 2 != 0) + 2;
            if (usMaxInstanceCount >= static_cast<uint16_t>(0xFFFF / iJoinedLength)) {
                usMaxInstanceCount = static_cast<uint16_t>(0xFFFF / iJoinedLength);
            }
            uint32_t uiTotalVertices = usMaxInstanceCount * iVertices;
            // The array allocations ask for 0xFFFFFFFF bytes where the size overflows 32 bits.
            uint64_t ullBytes = uint64_t(uiTotalVertices) * sizeof(NiPoint3);
            pkVertex = static_cast<NiPoint3*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            pkNormal = static_cast<NiPoint3*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            ullBytes = uint64_t(uiTotalVertices) * sizeof(NiColorA);
            void* pColorMemory =
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            if (pColorMemory) {
                rt::eh_vector_constructor_iterator(
                    pColorMemory, sizeof(NiColorA), uiTotalVertices, &xfnv::constructElement<NiColorA>
                );
                pkColor = static_cast<NiColorA*>(pColorMemory);
            }
            else {
                pkColor = nullptr;
            }
            ullBytes = uint64_t(uiTotalVertices) * sizeof(NiPoint2);
            pkTexture = static_cast<NiPoint2*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            ullBytes = uint64_t(uiTotalVertices) * sizeof(float);
            pfInstanceIndex = static_cast<float*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            uint16_t usLength = pusStripLengths[0];
            uint16_t usJoinedLength = static_cast<uint16_t>(usLength + (usLength % 2 != 0) + 2);
            uint32_t uiIndexCount = static_cast<uint32_t>((usMaxInstanceCount - 1) * usJoinedLength + usLength);
            ullBytes = uint64_t(uiIndexCount) * sizeof(uint16_t);
            uint16_t* pusIndices = static_cast<uint16_t*>(
                MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
            );
            const NiPoint3* pkSrcNormal = pStripsData->m_pkNormal;
            const NiColorA* pkSrcColor = pStripsData->m_pkColor;
            const NiPoint2* pkSrcTexture = pStripsData->m_pkTexture;
            int32_t iIndex = 0;
            for (int32_t k = 0; k < usMaxInstanceCount; ++k) {
                rt::memcpy(pkVertex + k * iVertices, pkSrcVertex, iVertices * sizeof(NiPoint3));
                rt::memcpy(pkNormal + k * iVertices, pkSrcNormal, iVertices * sizeof(NiPoint3));
                rt::memcpy(pkColor + k * iVertices, pkSrcColor, iVertices * sizeof(NiColorA));
                rt::memcpy(pkTexture + k * iVertices, pkSrcTexture, iVertices * sizeof(NiPoint2));
                if (iVertices > 0) {
                    float fInstance = static_cast<float>(k);
                    for (int32_t j = 0; j < iVertices; ++j) {
                        pfInstanceIndex[k * iVertices + j] = fInstance;
                    }
                }
                for (int32_t i = 0; i < pusStripLengths[0]; ++i) {
                    pusIndices[iIndex++] = static_cast<uint16_t>(pusStripLists[i] + k * iVertices);
                }
                if (k < usMaxInstanceCount - 1) {
                    uint16_t usInstance = static_cast<uint16_t>(k);
                    pusIndices[iIndex++] =
                        static_cast<uint16_t>(pusStripLists[pusStripLengths[0] - 1] + usInstance * iVertices);
                    pusIndices[iIndex++] = static_cast<uint16_t>(pusStripLists[0] + (usInstance + 1) * iVertices);
                    if (pusStripLengths[0] % 2 == 1) {
                        pusIndices[iIndex++] = static_cast<uint16_t>(pusStripLists[0] + (usInstance + 1) * iVertices);
                    }
                }
            }
            uint16_t* pusOutStripLengths = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(sizeof(uint16_t)));
            *pusOutStripLengths = static_cast<uint16_t>(iIndex);
            int32_t iLength = pusStripLengths[0];
            usInstanceTriCount =
                iLength % 2 == 1 ? static_cast<uint16_t>(iLength + 3) : static_cast<uint16_t>(iLength + 2);
            void* pDataMemory = NiMemObject::operator new(sizeof(NiTriStripsDynamicData));
            pTriData = pDataMemory
                ? ::new (pDataMemory) NiTriStripsDynamicData(
                      usMaxInstanceCount * iVertices, pkVertex, pkNormal, pkColor, pkTexture, 1, 0, iIndex - 2, 1,
                      pusOutStripLengths, pusIndices, usMaxInstanceCount * iVertices, 0
                  )
                : nullptr;
        }
        NiTexture* pBaseTexture = nullptr;
        // A tall grass shader property (shader property type 4) gives the base texture.
        if (pGrassGeom->GetProperty(NiProperty::SHADE) &&
            static_cast<NiShadeProperty*>(pGrassGeom->GetProperty(NiProperty::SHADE))->iShaderPropertyType == 4) {
            TallGrassShaderProperty* pShader =
                static_cast<TallGrassShaderProperty*>(pGrassGeom->GetProperty(NiProperty::SHADE));
            if (pShader) {
                NiTexture* pTexture = pShader->GetBaseTexture();
                if (pTexture) {
                    pBaseTexture = pTexture;
                    NiIncRefCount(pTexture);
                }
            }
        }
        // The blended property is the model's own alpha property, or a new default one; both test against the model's
        // reference value (0 without one).
        NiAlphaProperty* pAlphaBlend = static_cast<NiAlphaProperty*>(pGrassGeom->GetProperty(NiProperty::ALPHA));
        uint8_t ucTestRef = 0;
        if (pAlphaBlend) {
            ucTestRef = pAlphaBlend->m_ucAlphaTestRef;
        }
        else {
            void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
            if (pAlphaMemory) {
                // EXE-INLINED 0x4391C0
                pAlphaBlend = ::new (pAlphaMemory) NiAlphaProperty;
            }
        }
        // Equivalent to SetAlphaBlending(true), SetAlphaTesting(true), SetTestMode(TEST_GREATER) and SetTestRef.
        pAlphaBlend->m_ucAlphaTestRef = ucTestRef;
        pAlphaBlend->m_uFlags = static_cast<uint16_t>(
            (pAlphaBlend->m_uFlags &
             ~(NiAlphaProperty::ALPHA_BLEND_MASK | NiAlphaProperty::TEST_ENABLE_MASK |
               NiAlphaProperty::TEST_FUNC_MASK)) |
            NiAlphaProperty::ALPHA_BLEND_MASK | NiAlphaProperty::TEST_ENABLE_MASK |
            (NiAlphaProperty::TEST_GREATER << NiAlphaProperty::TEST_FUNC_POS)
        );
        void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
        NiAlphaProperty* pAlphaNoBlend = nullptr;
        if (pAlphaMemory) {
            // EXE-INLINED 0x4391C0
            pAlphaNoBlend = ::new (pAlphaMemory) NiAlphaProperty;
        }
        // Equivalent to SetAlphaBlending(false), SetAlphaTesting(true), SetTestMode(TEST_GREATER), SetTestRef.
        pAlphaNoBlend->m_ucAlphaTestRef = ucTestRef;
        pAlphaNoBlend->m_uFlags = static_cast<uint16_t>(
            (pAlphaNoBlend->m_uFlags &
             ~(NiAlphaProperty::ALPHA_BLEND_MASK | NiAlphaProperty::TEST_ENABLE_MASK |
               NiAlphaProperty::TEST_FUNC_MASK)) |
            NiAlphaProperty::TEST_ENABLE_MASK | (NiAlphaProperty::TEST_GREATER << NiAlphaProperty::TEST_FUNC_POS)
        );
        NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(pGrassGeom->GetProperty(NiProperty::STENCIL));
        float fBoundRadius = pGrassGeom->m_spModelData.m_pObject->m_kBound.m_fRadius;
        pCachedGeom->bTristrip = bIsStrips;
        pCachedGeom->spTriGeomData = pTriData;
        pCachedGeom->pInstanceIndex = pfInstanceIndex;
        pCachedGeom->iVertices = iVertices;
        pCachedGeom->spBaseTexture = pBaseTexture;
        pCachedGeom->spAlphaNoBlend = pAlphaNoBlend;
        pCachedGeom->spAlphaBlend = pAlphaBlend;
        pCachedGeom->spStencil = pStencil;
        pCachedGeom->fBoundRadius = fBoundRadius;
        pCachedGeom->iMaxInstanceCount = usMaxInstanceCount;
        pCachedGeom->usInstanceTriCount = usInstanceTriCount;
        pCachedGeom->bBillboard = false;
        if (pGrassNode) {
            // NiIsKindOf(NiBillboardNode, pGrassNode).
            bool bIsBillboard = false;
            for (const NiRTTI* pRTTI = pGrassNode->GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
                if (pRTTI == &NiBillboardNode::ms_RTTI) {
                    bIsBillboard = true;
                    break;
                }
            }
            if (bIsBillboard) {
                pCachedGeom->bBillboard = true;
            }
        }
        if (pGeometryLoadCallback) {
            pGeometryLoadCallback(apGrassParam->pcGeomFilename);
        }
        if (pBaseTexture) {
            NiDecRefCount(pBaseTexture);
        }
        NiDecRefCount(spObject.m_pObject);
    }
    // The map holds a reference, so the geometry outlives the local's release.
    pCachedGeom = spCachedGeom.m_pObject;
    NiDecRefCount(pCachedGeom);
    pTLSData->eMemContext = eMemContext;
    return pCachedGeom;
}

// Each group of the grass that has the chunk drops the chunk; a group left without chunks leaves the group list (the
// head and the tail through RemoveHead and RemoveTail, a middle node unlinked and returned to the node pool), then is
// destroyed and freed.
void TallGrassShaderProperty::RemoveFromTerrainChunk(int32_t alTerrainChunkSeed) {
    NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>* v8;
    NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>* pos;
    int32_t iChunkIndex;
    TLSData* v1 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v1->eMemContext;
    TLSData* v22 = v1;
    MEM_CONTEXT v23 = eMemContext;
    v1->eMemContext = MEM_CONTEXT_GRASS;
    NiTPointerMap<uint32_t, TallGrassShaderProperty::CachedGrass*>* CachedGrassMap =
        TallGrassShaderProperty::GetCachedGrassMap();
    uint32_t m_uiHashSize = CachedGrassMap->m_uiHashSize;
    uint32_t v5 = 0;
    if (m_uiHashSize) {
        NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>** m_ppkHashTable = CachedGrassMap->m_ppkHashTable;
        NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>** v7 = m_ppkHashTable;
        while (!*v7) {
            ++v5;
            ++v7;
            if (v5 >= m_uiHashSize) {
                goto LABEL_5;
            }
        }
        v8 = m_ppkHashTable[v5];
    }
    else {
    LABEL_5:;
        v8 = nullptr;
    }
    pos = v8;
    TallGrassShaderProperty::CachedGrass* pCachedGrass = nullptr;
    if ((TallGrassShaderProperty::GetCachedGrassMap()->m_kAllocator.m_uiCount) && (v8)) {
        do {
            uint32_t v24;
            TallGrassShaderProperty::GetCachedGrassMap()->GetNext(pos, v24, pCachedGrass);
            TallGrassShaderProperty::CachedGrass* v9 = pCachedGrass;
            if (pCachedGrass) {
                NiTMap<int32_t, int32_t>* p_pLowTerrainChunkMap = &pCachedGrass->pLowTerrainChunkMap;
                NiTMap<int32_t, int32_t>* v21 = &pCachedGrass->pLowTerrainChunkMap;
                if (pCachedGrass->pLowTerrainChunkMap.GetAt(alTerrainChunkSeed, iChunkIndex)) {
                    NiTListItem<TallGrassGroup*>* m_pkHead = v9->pLowGrassGroupList.m_pkHead;
                    NiTPointerList<TallGrassGroup*>* p_pLowGrassGroupList = &v9->pLowGrassGroupList;
                    if (m_pkHead) {
                        do {
                            TallGrassGroup* m_element = m_pkHead->m_element;
                            TallGrassGroup** p_m_element = &m_pkHead->m_element;
                            NiTListItem<TallGrassGroup*>* v14 = m_pkHead;
                            m_pkHead = m_pkHead->m_pkNext;
                            if ((m_element) && (!m_element->RemoveTerrainChunk(alTerrainChunkSeed))) {
                                if (v14 == p_pLowGrassGroupList->m_pkHead) {
                                    p_pLowGrassGroupList->RemoveHead();
                                }
                                else {
                                    if (v14 == p_pLowGrassGroupList->m_pkTail) {
                                        p_pLowGrassGroupList->RemoveTail();
                                    }
                                    else {
                                        NiTListItem<TallGrassGroup*>* pkPrev = v14->m_pkPrev;
                                        NiTListItem<TallGrassGroup*>* m_pkNext = v14->m_pkNext;
                                        if (pkPrev) {
                                            pkPrev->m_pkNext = m_pkNext;
                                        }
                                        if (m_pkNext) {
                                            m_pkNext->m_pkPrev = pkPrev;
                                        }
                                        *p_m_element = nullptr;
                                        // EXE-INLINED 0x45CEE0
                                        using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
                                        PoolAllocator::ms_kSpinLock.Lock(nullptr);
                                        // The pool treats a freed list item as untyped node storage, as the
                                        // allocator's own Deallocate (NiTPointerAllocator) casts it.
                                        PoolAllocator::AllocNode* pkFreed =
                                            reinterpret_cast<PoolAllocator::AllocNode*>(v14);
                                        pkFreed->m_pkData = nullptr;
                                        pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
                                        bool v17 = PoolAllocator::ms_kSpinLock.uiLockCount-- == 1;
                                        PoolAllocator::ms_pkFreeMem = pkFreed;
                                        if (v17) {
                                            PoolAllocator::ms_kSpinLock.uiOwningThread = 0;
                                        }
                                        --p_pLowGrassGroupList->m_uiCount;
                                    }
                                }
                                m_element->~TallGrassGroup();
                                MemoryManager_Singleton.Deallocate(m_element);
                                --uiGrassGeometryCount;
                            }
                        }
                        while (m_pkHead);
                        p_pLowTerrainChunkMap = v21;
                        v1 = v22;
                    }
                    p_pLowTerrainChunkMap->RemoveAt_00E91FD0(alTerrainChunkSeed);
                    eMemContext = v23;
                }
            }
        }
        while (pos);
    }

    v1->eMemContext = eMemContext;
}

// Removes every cached grass group of cell (aiCellX, aiCellY), under the grass memory context: each group's terrain
// chunks leave its grass's chunk map, its node leaves the grass's group list (NiTPointerListBase::RemovePos), then the
// group is destroyed and freed.
void TallGrassShaderProperty::RemoveFromCell(int32_t aiCellX, int32_t aiCellY) {
    using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eOldContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_GRASS;
    NiTMapItem<uint32_t, CachedGrass*>* pPos = GetCachedGrassMap()->GetFirstPos();
    CachedGrass* pCachedGrass = nullptr;
    if (GetCachedGrassMap()->m_kAllocator.m_uiCount && pPos) {
        do {
            uint32_t uiGrassID;
            GetCachedGrassMap()->GetNext(pPos, uiGrassID, pCachedGrass);
            if (!pCachedGrass) {
                continue;
            }
            NiTListItem<TallGrassGroup*>* pNode = pCachedGrass->pLowGrassGroupList.m_pkHead;
            while (pNode) {
                NiTListItem<TallGrassGroup*>* pNext = pNode->m_pkNext;
                TallGrassGroup* pGroup = pNode->m_element;
                if (pGroup && aiCellX == pGroup->iCellCoord[0] && aiCellY == pGroup->iCellCoord[1]) {
                    int32_t iInstanceCount = pGroup->iInstanceCount;
                    int32_t* plTerrainChunkSeed = pGroup->plTerrainChunkSeed;
                    for (int32_t i = 0; i < iInstanceCount; ++i) {
                        pCachedGrass->pLowTerrainChunkMap.RemoveAt_00E91FD0(plTerrainChunkSeed[i]);
                    }
                    // NiTPointerListBase::RemovePos: the head and a middle node are unlinked and returned to the node
                    // pool here, the tail goes through RemoveTail.
                    NiTPointerList<TallGrassGroup*>& kList = pCachedGrass->pLowGrassGroupList;
                    if (pNode != kList.m_pkHead && pNode == kList.m_pkTail) {
                        kList.RemoveTail();
                    }
                    else {
                        if (pNode == kList.m_pkHead) {
                            kList.m_pkHead = pNode->m_pkNext;
                            if (kList.m_pkHead) {
                                kList.m_pkHead->m_pkPrev = nullptr;
                            }
                            else {
                                kList.m_pkTail = nullptr;
                            }
                        }
                        else {
                            NiTListItem<TallGrassGroup*>* pPrevItem = pNode->m_pkPrev;
                            NiTListItem<TallGrassGroup*>* pNextItem = pNode->m_pkNext;
                            if (pPrevItem) {
                                pPrevItem->m_pkNext = pNextItem;
                            }
                            if (pNextItem) {
                                pNextItem->m_pkPrev = pPrevItem;
                            }
                        }
                        pNode->m_element = nullptr;
                        PoolAllocator::Lock();
                        // (the freed node is the pool's untyped memory again, linked through the pool's own fields).
                        PoolAllocator::AllocNode* pFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pNode);
                        pFreed->m_pkData = nullptr;
                        pFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
                        PoolAllocator::ms_pkFreeMem = pFreed;
                        PoolAllocator::Unlock();
                        kList.m_uiCount--;
                    }
                    pGroup->~TallGrassGroup();
                    MemoryManager_Singleton.Deallocate(pGroup);
                    --uiGrassGeometryCount;
                }
                pNode = pNext;
            }
        }
        while (pPos);
    }
    pTLSData->eMemContext = eOldContext;
}

// Looks the grass up in the cached grass map, then the terrain chunk in that grass's own chunk map.
bool TallGrassShaderProperty::ContainsChunk_00B617B0(uint32_t aiGrassID, int32_t alTerrainChunkSeed) {
    CachedGrass* pCachedGrass = nullptr;
    if (!GetCachedGrassMap()->GetAt(aiGrassID, pCachedGrass)) {
        return false;
    }
    int32_t iChunkIndex;
    return pCachedGrass->pLowTerrainChunkMap.GetAt(alTerrainChunkSeed, iChunkIndex);
}

void TallGrassShaderProperty::RemoveAllGrass() {
    NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>* kiter;
    TallGrassShaderProperty::CachedGrass* pcgrass;
    NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>* kiter_2;
    uint32_t ikey;
    NiTPointerMap<uint32_t, TallGrassShaderProperty::CachedGrass*>* CachedGrassMap =
        TallGrassShaderProperty::GetCachedGrassMap();
    uint32_t m_uiHashSize = CachedGrassMap->m_uiHashSize;
    uint32_t v2 = 0;
    if (m_uiHashSize) {
        NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>** m_ppkHashTable = CachedGrassMap->m_ppkHashTable;
        NiTMapItem<uint32_t, TallGrassShaderProperty::CachedGrass*>** v4 = m_ppkHashTable;
        while (!*v4) {
            ++v2;
            ++v4;
            if (v2 >= m_uiHashSize) {
                goto LABEL_5;
            }
        }
        kiter = m_ppkHashTable[v2];
    }
    else {
    LABEL_5:;
        kiter = nullptr;
    }
    kiter_2 = kiter;
    pcgrass = nullptr;
    if ((TallGrassShaderProperty::GetCachedGrassMap()->m_kAllocator.m_uiCount) && (kiter)) {
        do {
            TallGrassShaderProperty::GetCachedGrassMap()->GetNext(kiter_2, ikey, pcgrass);
            TallGrassShaderProperty::CachedGrass* v7 = pcgrass;
            if (pcgrass) {
                pcgrass->Free();
                MemoryManager_Singleton.Deallocate(v7);
            }
        }
        while (kiter_2);
    }

    TallGrassShaderProperty::GetCachedGrassMap()->RemoveAll();
    uiGrassGeometryCount = 0;
    uiGrassInstanceCount = 0;
}

uint16_t TallGrassShaderProperty::GetGrassGroup(
    TallGrassShaderProperty::GrassParam* apGrassParam,
    int32_t aiCellX,
    int32_t aiCellY,
    TallGrassGroup** apGrassGroup,
    NiNode* apGrassNode
) {
    TallGrassShaderProperty::CachedGrass* pgrass;
    TLSData* v5 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v5->eMemContext;
    v5->eMemContext = MEM_CONTEXT_GRASS;
    TallGrassGroup** v6 = apGrassGroup;
    *apGrassGroup = nullptr;
    TallGrassShaderProperty::CachedGeometry* pcachedGeom = TallGrassShaderProperty::GetCachedGeometry(apGrassParam);
    if (!pcachedGeom) {
        v5->eMemContext = eMemContext;
        return 0;
    }
    uint32_t iGrassID = apGrassParam->iGrassID;
    pgrass = nullptr;
    if (!TallGrassShaderProperty::GetCachedGrassMap()->GetAt(iGrassID, pgrass)) {
        TallGrassShaderProperty::CachedGrass* v8 = static_cast<TallGrassShaderProperty::CachedGrass*>(
            MemoryManager_Singleton.Allocate(sizeof(TallGrassShaderProperty::CachedGrass))
        );
        if (v8) {
            pgrass = ::new (v8) TallGrassShaderProperty::CachedGrass;
        }
        else {
            pgrass = nullptr;
        }
        pgrass->spCachedLowGeometry.SetNew(pcachedGeom);
        rt::memcpy(&pgrass->kGrassParam, apGrassParam, sizeof(TallGrassShaderProperty::GrassParam));
        TallGrassShaderProperty::GetCachedGrassMap()->SetAt(apGrassParam->iGrassID, pgrass);
    }
    NiTListItem<TallGrassGroup*>* m_pkHead = pgrass->pLowGrassGroupList.m_pkHead;
    NiTPointerList<TallGrassGroup*>* plist = &pgrass->pLowGrassGroupList;
    TallGrassGroup* v12 = *v6;
    if (!*v6) {
        while (m_pkHead) {
            TallGrassGroup* m_element = m_pkHead->m_element;
            m_pkHead = m_pkHead->m_pkNext;
            if (m_element->iInstanceCount != m_element->iMaxInstanceCount &&
                (aiCellX == m_element->iCellCoord[0u] && aiCellY == m_element->iCellCoord[1])) {
                *v6 = m_element;
                break;
            }
            if (v12) {
                break;
            }
        }
        v12 = *v6;
        if (!*v6) {
            goto LABEL_21;
        }
    }
    if (v12->iMaxInstanceCount == v12->iInstanceCount) {
        TallGrassGroup* v15;
        if (v12) {
            v12->~TallGrassGroup();
            MemoryManager_Singleton.Deallocate(v12);
        }
    LABEL_21:;
        TallGrassGroup* v14 = static_cast<TallGrassGroup*>(MemoryManager_Singleton.Allocate(sizeof(TallGrassGroup)));
        if (v14) {
            v15 = ::new (v14) TallGrassGroup(pcachedGeom, aiCellX, aiCellY, false);
        }
        else {
            v15 = nullptr;
        }
        *v6 = v15;
        ++uiGrassGeometryCount;
        (*v6)->SetGrassParam(apGrassParam);
        plist->AddTail(*v6);
        apGrassNode->AttachChild((*v6)->spMBGrassNode.m_pObject, 1);
    }
    uint16_t result = (*v6)->iMaxInstanceCount - (*v6)->iInstanceCount;
    v5->eMemContext = eMemContext;
    return result;
}

void TallGrassShaderProperty::AddGrass(
    int32_t alTerrainChunkSeed,
    int32_t aiCellX,
    int32_t aiCellY,
    NiNode* apGrassNode,
    TallGrassShaderProperty::GrassParam* apGrassParam,
    NiPoint3* apPositions,
    float* afColors,
    uint16_t aiNumPositions
) {
    TallGrassGroup::GrassInstanceData grassinstance;
    TallGrassGroup* v29;
    TLSData* v8 = GetThreadTLSData();
    TLSData* pTLS = v8;
    MEM_CONTEXT eMemContext = v8->eMemContext;
    v8->eMemContext = MEM_CONTEXT_GRASS;
    uint16_t v9 = aiNumPositions;
    uiGrassInstanceCount += aiNumPositions;
    int32_t i = 0;
    int32_t v11 = 0;
    if (aiNumPositions) {
        while (true) {
            v29 = nullptr;
            uint16_t ilen = TallGrassShaderProperty::GetGrassGroup(apGrassParam, aiCellX, aiCellY, &v29, apGrassNode);
            if (!v29) {
                break;
            }
            int32_t ilen_2 = ilen;
            if (ilen) {
                NiPoint3* v13 = &apPositions[v11];
                do {
                    if (!v9) {
                        break;
                    }
                    int32_t rseed = TallGrassShaderProperty::Hash_2DPosition(*v13);
                    rt::srand(static_cast<uint32_t>(rseed));
                    grassinstance.fXPos = v13->x;
                    grassinstance.fYPos = v13->y;
                    grassinstance.fZPos = v13->z;
                    float fRandOffseta = static_cast<double>(rt::rand()) / 65534.0 - 1.0;
                    grassinstance.fColor =
                        (1.0 - apGrassParam->fColorRange + apGrassParam->fColorRange * 0.5 * fRandOffseta) *
                        afColors[v11];
                    if (!(grassinstance.fColor >= 1.0f)) {
                        if (grassinstance.fColor < 0.0f) {
                            grassinstance.fColor = 0.0;
                        }
                    }
                    else {
                        grassinstance.fColor = 0.99f;
                    }
                    int32_t v15 = rt::rand();
                    float fRandOffsetb = static_cast<double>(v15) / 32767.0 + static_cast<double>(v15) / 32767.0 - 1.0;
                    float fRandOffsetc = apGrassParam->fHeightRange * fRandOffsetb * 100.0;
                    double v16 = fRandOffsetc;
                    float fRandOffset = static_cast<float>(static_cast<int32_t>(fRandOffsetc));
                    double v17 = !(v16 - fRandOffset >= 0.0) ? fRandOffset - 1.0 : fRandOffset;
                    float fRandOffsetd = v17;
                    grassinstance.fColor = fRandOffsetd + grassinstance.fColor;
                    v29->Add(&grassinstance, alTerrainChunkSeed);
                    ++i;
                    --v9;
                    ++v11;
                    ++v13;
                }
                while (i < ilen_2);
            }
            v29->UpdateGroupBound();
            v8 = pTLS;
            i = 0;
            if (!v9) {
                goto LABEL_15;
            }
        }
        v8->eMemContext = eMemContext;
    }
    else {
    LABEL_15:;
        apGrassNode->UpdateWorldBound();
        uint32_t iGrassID = apGrassParam->iGrassID;
        TallGrassShaderProperty::CachedGrass* pCachedGrass = nullptr;
        TallGrassShaderProperty::GetCachedGrassMap()->GetAt(iGrassID, pCachedGrass);
        pCachedGrass->pLowTerrainChunkMap.SetAt_B57EA0(alTerrainChunkSeed, alTerrainChunkSeed);
        v8->eMemContext = eMemContext;
    }
}

// A high-priority load of the mesh apName names, looked up in the meshes archive.
GrassLoadTask::GrassLoadTask(const char* apName) : QueuedFileEntry(IO_TASK_PRIORITY_HIGH) {
    SetFileName(apName);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

// The stream and the base clean up after themselves.
GrassLoadTask::~GrassLoadTask() {}

void GrassLoadTask::Run() {
    char apStandardizedFileName[260];
    StandardizeModelPath(pFileName, apStandardizedFileName, 260u);
    m_kStream.Load(apStandardizedFileName, static_cast<NiFile*>(nullptr));
}

bool GrassLoadTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("grass load task"));
}

// Grass models load from the meshes archives.
uint8_t GrassLoadTask::GetFileIndex() {
    return IO_FILE_INDEX_MESHES;
}

NiPointer<NiObject> GrassLoadTask::GetObject() {
    NiObject* pObject = m_kStream.m_kTopObjects.m_uiSize ? m_kStream.m_kTopObjects.m_pBase[0].m_pObject : nullptr;
    NiPointer<NiObject> spResult;
    spResult.m_pObject = pObject;
    if (pObject) {
        static_cast<NiRefObject*>(pObject)->IncRefCount();
    }
    return spResult;
}
