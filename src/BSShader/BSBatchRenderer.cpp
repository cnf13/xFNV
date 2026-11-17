#include "BSShader/BSBatchRenderer.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSInstanceRendering.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGeometryGroupManager.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiUnsharedGeometryGroup.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

NiRTTI BSBatchRenderer::ms_RTTI("BSBatchRenderer", &NiObject::ms_RTTI);

void PersistentPassList_Sort(
    PersistentPassList* apList,
    int32_t (*apCompare)(BSShaderProperty::RenderPass**, BSShaderProperty::RenderPass**)
) {
    PersistentPassList* v2 = apList;
    apList->CleanUnusedNodes();
    if (v2->m_pkHead) {
        int32_t v13 = 1;
        while (true) {
            PersistentPassList::ListItem* i;
            PersistentPassList::ListItem* m_pkHead = v2->m_pkHead;
            uint32_t v4 = v13;
            PersistentPassList::ListItem* v5 = nullptr;
            v2->m_pkHead = nullptr;
            PersistentPassList::ListItem* v12 = nullptr;
            int32_t v14 = 0;
            if (!m_pkHead) {
                goto LABEL_24;
            }
            do {
                ++v14;
                int32_t v6 = 0;
                uint32_t v7 = 0;
                for (i = m_pkHead; v7 < v4; ++v7) {
                    i = i->m_pkNext;
                    ++v6;
                    if (!i) {
                        break;
                    }
                }
                int32_t v9 = v4;
                while (true) {
                    PersistentPassList::ListItem* v10;
                    int32_t v11;
                    if (v6 > 0) {
                        goto LABEL_13;
                    }
                    if (v9 <= 0 || !i) {
                        break;
                    }
                    if (!v6) {
                        v10 = i;
                        i = i->m_pkNext;
                        --v9;
                        goto LABEL_18;
                    }
                LABEL_13:;
                    if (v9 && (i && (v11 = apCompare(&m_pkHead->m_element, &i->m_element), (v5 = v12, v11 > 0)))) {
                        v10 = i;
                        i = i->m_pkNext;
                        --v9;
                    }
                    else {
                        v10 = m_pkHead;
                        m_pkHead = m_pkHead->m_pkNext;
                        --v6;
                    }
                LABEL_18:;
                    if (v5) {
                        v4 = v13;
                        v5->m_pkNext = v10;
                        v10->m_pkPrev = v5;
                    }
                    else {
                        apList->m_pkHead = v10;
                        v4 = v13;
                        v10->m_pkPrev = nullptr;
                    }
                    v12 = v10;
                    v5 = v10;
                }
                m_pkHead = i;
            }
            while (i);
            v2 = apList;
        LABEL_24:;
            v5->m_pkNext = nullptr;
            v13 = 2 * v4;
            if (v14 == 1) {
                v2->m_pkTail = v5;
                return;
            }
        }
    }
}

// The alpha passes' depth order, the compare PersistentPassList_Sort is given. Each pass's depth is its geometry's
// world bound center along the sort camera's direction (the first column of its world rotation), less the bound's
// radius when bSortByClosestPoint: 0 when the depths are equal, 1 when the second pass is deeper, else -1 (unordered
// included).
int32_t CompareRenderPass(BSShaderProperty::RenderPass** apPass1, BSShaderProperty::RenderPass** apPass2) {
    NiGeometry* pGeometry2 = (*apPass2)->pGeometry;
    const NiCamera* pCamera = BSBatchRenderer::pSortCamera;
    const float fDirX = pCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
    NiGeometry* pGeometry1 = (*apPass1)->pGeometry;
    const float fDirY = pCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
    const float fDirZ = pCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
    float fDepth1;
    float fDepth2;
    // EXE-INLINED 0x43D450: each bound is NiAVObject::GetWorldBound inlined; under bSortByClosestPoint the exe
    // inlines it twice per geometry, once for the center and once for the radius.
    if (BSBatchRenderer::bSortByClosestPoint) {
        const NiBound* pCenter1 = pGeometry1->m_kWorldBound ? pGeometry1->m_kWorldBound : &NiAVObject::NullBoundS;
        const NiBound* pRadius1 = pGeometry1->m_kWorldBound ? pGeometry1->m_kWorldBound : &NiAVObject::NullBoundS;
        fDepth1 = pCenter1->m_kCenter.x * fDirX + pCenter1->m_kCenter.y * fDirY + pCenter1->m_kCenter.z * fDirZ;
        fDepth1 = fDepth1 - pRadius1->m_fRadius;
        const NiBound* pCenter2 = pGeometry2->m_kWorldBound ? pGeometry2->m_kWorldBound : &NiAVObject::NullBoundS;
        const NiBound* pRadius2 = pGeometry2->m_kWorldBound ? pGeometry2->m_kWorldBound : &NiAVObject::NullBoundS;
        fDepth2 = pCenter2->m_kCenter.x * fDirX + pCenter2->m_kCenter.y * fDirY + pCenter2->m_kCenter.z * fDirZ;
        fDepth2 = fDepth2 - pRadius2->m_fRadius;
    }
    else {
        const NiBound* pBound1 = pGeometry1->m_kWorldBound ? pGeometry1->m_kWorldBound : &NiAVObject::NullBoundS;
        fDepth1 = pBound1->m_kCenter.x * fDirX + pBound1->m_kCenter.y * fDirY + pBound1->m_kCenter.z * fDirZ;
        const NiBound* pBound2 = pGeometry2->m_kWorldBound ? pGeometry2->m_kWorldBound : &NiAVObject::NullBoundS;
        fDepth2 = pBound2->m_kCenter.x * fDirX + pBound2->m_kCenter.y * fDirY + pBound2->m_kCenter.z * fDirZ;
    }
    if (fDepth1 == fDepth2) {
        return 0;
    }
    return fDepth2 > fDepth1 ? 1 : -1;
}

int32_t ComparePasses(uint16_t pass1, uint16_t pass2) {
    return pass1 < pass2 ? -1 : 1;
}

PersistentPassList::PersistentPassList() : m_pkHead(nullptr), m_pkTail(nullptr), pNextFreeItem(nullptr) {}

// Moves the used nodes to the free chain and frees every node.
PersistentPassList::~PersistentPassList() {
    CleanUnusedNodes();
    pNextFreeItem = m_pkHead;
    m_pkHead = nullptr;
    m_pkTail = nullptr;
    Allocator.uiCount = 0;
    CleanUnusedNodes();
}

void PersistentPassList::CleanUnusedNodes() {
    PersistentPassList::ListItem* pNextFreeItem = this->pNextFreeItem;
    if (pNextFreeItem) {
        NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
        do {
            // A freed list item's memory is reused as a pool node (NiTAbstractPoolAllocator's node holds the free-list
            // link and a data pointer), so the item is read as one.
            NiTAbstractPoolAllocator<uint32_t>::AllocNode* v3 =
                reinterpret_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(pNextFreeItem);
            pNextFreeItem = pNextFreeItem->m_pkNext;
            NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
            v3->m_pkData = nullptr;
            v3->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
            bool v4 = NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount-- == 1;
            NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = v3;
            if (v4) {
                NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
            }
        }
        while (pNextFreeItem);
        if (!--NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount) {
            NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
        }
    }
    PersistentPassList::ListItem* m_pkTail = this->m_pkTail;
    this->pNextFreeItem = nullptr;
    if (m_pkTail) {
        m_pkTail->m_pkNext = nullptr;
    }
}

void PersistentPassList::AddFront(BSShaderProperty::RenderPass** apRenderPass) {
    ListItem* pItem = pNextFreeItem;
    if (pItem) {
        pNextFreeItem = pItem->m_pkNext;
    }
    else {
        pItem = static_cast<ListItem*>(Allocator.Allocate());
    }
    pItem->m_element = *apRenderPass;
    pItem->m_pkNext = m_pkHead;
    pItem->m_pkPrev = nullptr;
    ListItem* pHead = m_pkHead;
    if (pHead) {
        pHead->m_pkPrev = pItem;
        ++Allocator.uiCount;
    }
    else {
        ++Allocator.uiCount;
        m_pkTail = pItem;
    }
    m_pkHead = pItem;
}

void PersistentPassList::AddBack(BSShaderProperty::RenderPass** apRenderPass) {
    PersistentPassList::ListItem* pNextFreeItem = this->pNextFreeItem;
    if (pNextFreeItem) {
        this->pNextFreeItem = pNextFreeItem->m_pkNext;
    }
    else {
        pNextFreeItem = static_cast<PersistentPassList::ListItem*>(Allocator.Allocate());
    }
    pNextFreeItem->m_element = *apRenderPass;
    pNextFreeItem->m_pkNext = nullptr;
    pNextFreeItem->m_pkPrev = this->m_pkTail;
    PersistentPassList::ListItem* m_pkTail = this->m_pkTail;
    if (m_pkTail) {
        m_pkTail->m_pkNext = pNextFreeItem;
        ++Allocator.uiCount;
    }
    else {
        ++Allocator.uiCount;
        m_pkHead = pNextFreeItem;
    }
    this->m_pkTail = pNextFreeItem;
}

// The deleting destructor frees through the engine's heap.
void PersistentPassList::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}

BSBatchRenderer::PassGroup::PassGroup() : iSize(0) {}

void BSBatchRenderer::PassGroup::Clear(bool abReportNotEmpty) {
    for (uint32_t i = 0; i < 5; ++i) {
        PersistentPassList& rList = pListA[i];
        if (rList.Allocator.uiCount) {
            rList.CleanUnusedNodes();
            rList.pNextFreeItem = rList.m_pkHead;
            rList.m_pkHead = nullptr;
            rList.m_pkTail = nullptr;
            rList.Allocator.uiCount = 0;
        }
    }
    iSize = 0;
}

void BSBatchRenderer::PassGroup::AddHead(int32_t aiBucket, BSShaderProperty::RenderPass* apRenderPass) {
    PersistentPassList* v4 = &pListA[aiBucket];
    PersistentPassList::ListItem* pNextFreeItem = v4->pNextFreeItem;
    if (pNextFreeItem) {
        v4->pNextFreeItem = pNextFreeItem->m_pkNext;
    }
    else {
        pNextFreeItem = static_cast<PersistentPassList::ListItem*>(v4->Allocator.Allocate());
    }
    pNextFreeItem->m_element = apRenderPass;
    pNextFreeItem->m_pkNext = v4->m_pkHead;
    pNextFreeItem->m_pkPrev = nullptr;
    PersistentPassList::ListItem* m_pkHead = v4->m_pkHead;
    if (m_pkHead) {
        m_pkHead->m_pkPrev = pNextFreeItem;
    }
    else {
        v4->m_pkTail = pNextFreeItem;
    }
    v4->m_pkHead = pNextFreeItem;
    ++v4->Allocator.uiCount;
    ++iSize;
}

void BSBatchRenderer::PassGroup::AddTail(int32_t aiBucket, BSShaderProperty::RenderPass* apRenderPass) {
    PersistentPassList* v4 = &pListA[aiBucket];
    PersistentPassList::ListItem* pNextFreeItem = v4->pNextFreeItem;
    if (pNextFreeItem) {
        v4->pNextFreeItem = pNextFreeItem->m_pkNext;
    }
    else {
        pNextFreeItem = static_cast<PersistentPassList::ListItem*>(v4->Allocator.Allocate());
    }
    pNextFreeItem->m_element = apRenderPass;
    pNextFreeItem->m_pkNext = nullptr;
    pNextFreeItem->m_pkPrev = v4->m_pkTail;
    PersistentPassList::ListItem* m_pkTail = v4->m_pkTail;
    if (m_pkTail) {
        m_pkTail->m_pkNext = pNextFreeItem;
    }
    else {
        v4->m_pkHead = pNextFreeItem;
    }
    v4->m_pkTail = pNextFreeItem;
    ++v4->Allocator.uiCount;
    ++iSize;
}

// The pass list starts with its nodes moved to the free chain, as the game does.
BSBatchRenderer::GeometryGroup::GeometryGroup(uint32_t auiPassCount)
  : ucFlags(0), usCount(0), uiPassCount(auiPassCount), pBatchRenderer(nullptr) {
    PassList.CleanUnusedNodes();
    PersistentPassList::ListItem* pHead = PassList.m_pkHead;
    PassList.m_pkHead = nullptr;
    PassList.m_pkTail = nullptr;
    PassList.Allocator.uiCount = 0;
    PassList.pNextFreeItem = pHead;
}

// The batch renderer the group owns is deleted; the pass list frees its own nodes.
BSBatchRenderer::GeometryGroup::~GeometryGroup() {
    if (pBatchRenderer) {
        delete pBatchRenderer;
        pBatchRenderer = nullptr;
    }
}

// BSBatchRenderer::GeometryGroup::RegisterPass.
void BSBatchRenderer::GeometryGroup::RegisterPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t abAddToFront) {
    if ((ucFlags & 1) != 0) {
        if (static_cast<uint8_t>(abAddToFront)) {
            PassList.AddFront(&apRenderPass);
        }
        else {
            PassList.AddBack(&apRenderPass);
        }
    }
    else {
        if (!pBatchRenderer) {
            BSBatchRenderer* v5;
            BSBatchRenderer* v4 = static_cast<BSBatchRenderer*>(NiMemObject::operator new(sizeof(BSBatchRenderer)));
            if (v4) {
                v5 = ::new (v4) BSBatchRenderer(uiPassCount, 100);
            }
            else {
                v5 = nullptr;
            }
            pBatchRenderer = v5;
        }
        pBatchRenderer->RegisterPass(apRenderPass, apRenderPass->usPassEnum);
    }
    ++usCount;
}

void BSBatchRenderer::GeometryGroup::ClearPasses() {
    BSBatchRenderer* pBatchRenderer;
    PersistentPassList* p_kPassList = &PassList;
    (&PassList)->CleanUnusedNodes();
    p_kPassList->pNextFreeItem = p_kPassList->m_pkHead;
    p_kPassList->m_pkHead = nullptr;
    p_kPassList->m_pkTail = nullptr;
    p_kPassList->Allocator.uiCount = (uint32_t)0;
    pBatchRenderer = this->pBatchRenderer;
    if (pBatchRenderer) {
        pBatchRenderer->ClearRenderPasses(false);
    }
    usCount = (uint16_t)0;
}

void BSBatchRenderer::GeometryGroup::Render(int32_t abAlphaPass) {
    if ((ucFlags & 1) != 0) {
        BSBatchRenderer::RenderPersistentPassList(PassList, abAlphaPass);
    }
    else {
        BSBatchRenderer* pBatchRenderer = this->pBatchRenderer;
        if (pBatchRenderer) {
            pBatchRenderer->RenderBatches(
                BSShaderManager::BSSM_ZONLY, BSShaderManager::BSSM_BLOOD_SPLATTER, abAlphaPass
            );
        }
    }
    BSBatchRenderer* v4 = this->pBatchRenderer;
    if (v4) {
        v4->kActivePassIndexList.RemoveAll();
    }
    usCount = 0;
}

// The pass map with 37 buckets, the instance renderer and auiPassCount pass groups in the shader memory context,
// then the geometry groups of the a2 layout (99: all, 100: none, 101: the fading and alpha groups only).
BSBatchRenderer::BSBatchRenderer(uint32_t auiPassCount, int32_t a2) : RenderPassMap(37) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_SHADERS;
    pInstanceRenderer = new BSInstanceRendering();
    pRenderPassA = new PassGroup[auiPassCount];
    uiRenderPassCount = auiPassCount;
    iGroupingAlphas = 0;
    ClearRenderPasses(false);
    if (a2 == 99) {
        static const uint32_t kPassCounts[GG_COUNT] = {759, 759, 759, 759, 759, 759, 6, 1, 4, 759, 759, 6, 4};
        for (uint32_t i = 0; i < GG_COUNT; ++i) {
            pGeometryGroups[i] = new GeometryGroup(kPassCounts[i]);
        }
    }
    else if (a2 == 100 || a2 == 101) {
        for (GeometryGroup*& rpGroup : pGeometryGroups) {
            rpGroup = nullptr;
        }
        if (a2 == 100) {
            pAlphaGroups[0] = nullptr;
            pAlphaGroups[1] = nullptr;
        }
        else {
            pGeometryGroups[GG_FADING] = new GeometryGroup(759);
        }
    }
    if (a2 == 99 || a2 == 101) {
        pAlphaGroups[0] = new GeometryGroup(759);
        pAlphaGroups[1] = new GeometryGroup(759);
        pAlphaGroups[0]->ucFlags |= 1;
        pAlphaGroups[1]->ucFlags |= 1;
        kAlphaGroupLists[0].RemoveAll();
        kAlphaGroupLists[1].RemoveAll();
    }
    pTLS->eMemContext = eMemContext;
}

// The pass groups, the instance renderer and the geometry groups are owned; the lists and the map free their own.
BSBatchRenderer::~BSBatchRenderer() {
    ClearRenderPasses(false);
    delete[] pRenderPassA;
    uiRenderPassCount = 0;
    if (pInstanceRenderer) {
        delete pInstanceRenderer;
        pInstanceRenderer = nullptr;
    }
    for (GeometryGroup* pGroup : pGeometryGroups) {
        delete pGroup;
    }
    delete pAlphaGroups[0];
    delete pAlphaGroups[1];
}

void BSBatchRenderer::RegisterPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t auiPassEnum) {
    if (apRenderPass) {
        NiTriStrips* pGeometry = static_cast<NiTriStrips*>(apRenderPass->pGeometry);
        if (apRenderPass->pGeometry) {
            int32_t rbt;
            uint32_t uiindex;
            BSBatchRenderer* v11;
            bool v4 = false;
            BSShaderPPLightingProperty* m_pObject =
                static_cast<BSShaderPPLightingProperty*>(pGeometry->m_PropertyState.m_aspProps[3].m_pObject);
            if (pGeometry->m_PropertyState.m_aspProps[4].m_pObject) {
                // The stencil property's draw mode is DRAW_BOTH: both mask bits set.
                NiStencilProperty* pStencil =
                    static_cast<NiStencilProperty*>(pGeometry->m_PropertyState.m_aspProps[4].m_pObject);
                v4 = (pStencil->m_uFlags & NiStencilProperty::DRAWMODE_MASK) == NiStencilProperty::DRAWMODE_MASK;
            }
            NiAlphaProperty* v6 = static_cast<NiAlphaProperty*>(pGeometry->m_PropertyState.m_aspProps[0].m_pObject);
            char v7 = 0;
            if ((v6) &&
                ((v6->m_uFlags & 512) == 0 ||
                 (!apRenderPass->bFirstPass ||
                  (((v7 = 1, m_pObject->fAlpha < 1.0f) || m_pObject->fFadeAlpha < 1.0f) &&
                   ((v6->m_uFlags & 1) == 0 && auiPassEnum != 2))))) {
                v7 = 0;
            }

            if (auiPassEnum == 14 ||
                (auiPassEnum == 15 || (auiPassEnum == 4 || (auiPassEnum == 5 || auiPassEnum == 570)))) {
                v7 = 1;
            }
            if ((m_pObject->ulFlags[1] & BSS2_No_Transparency_Multisampling) != 0) {
                rbt = 4;
            }
            else {
                if (v7) {
                    rbt = !v4 + 2;
                }
                else {
                    rbt = v4;
                }
            }
            uint32_t uipass = apRenderPass->usPassEnum;
            if (auiPassEnum) {
                uipass = auiPassEnum;
            }
            if (RenderPassMap.GetAt(uipass, auiPassEnum)) {
                uiindex = auiPassEnum;
                v11 = this;
            }
            else {
                uiindex = RenderPassMap.m_kAllocator.m_uiCount;
                RenderPassMap.SetAt(uipass, uiindex);
                v11 = this;
                pRenderPassA[uiindex].Clear(false);
            }
            if (!v11->pRenderPassA[uiindex].iSize) {
                v11->kActivePassIndexList.Insert(uipass, &ComparePasses);
            }
            if (apRenderPass->bFirstPass &&
                (uipass < BSShaderManager::BSSM_SKYBASEPRE || uipass > BSShaderManager::BSSM_SKY_CLOUDS_UNDERWATER)) {
                v11->pRenderPassA[uiindex].AddHead(rbt, apRenderPass);
            }
            else {
                v11->pRenderPassA[uiindex].AddTail(rbt, apRenderPass);
            }
        }
    }
}

// Renders, for every active pass in the range (all passes when aePass is BSSM_EMPTY), each non-empty bucket of its
// pass group, then empties the group.
void BSBatchRenderer::RenderBatches(
    BSShaderManager::eBSSMPassEnum aePass,
    BSShaderManager::eBSSMPassEnum aePassEnd,
    int32_t abAlphaPass
) {
    BSShaderManager::eBSSMPassEnum uistart = aePass;
    BSShaderManager::eBSSMPassEnum uiend = aePassEnd;
    if (aePass == BSShaderManager::BSSM_EMPTY) {
        uistart = BSShaderManager::BSSM_ZONLY;
        uiend = BSShaderManager::BSSM_BLOOD_SPLATTER;
    }
    if (BSShaderManager::bUse30Lighting) {
        Lighting30Shader::ResetPerFrameConstantData();
    }
    if (kActivePassIndexList.GetNext() || kActivePassIndexList.m_item) {
        BSSimpleList<uint16_t>* pNode = &kActivePassIndexList;
        while (pNode) {
            uint32_t uiPass = pNode->m_item;
            if (uiPass <= uiend && uiPass >= uistart) {
                uint32_t uiIndex = 0;
                RenderPassMap.GetAt(uiPass, uiIndex);
                if (BSShaderAccumulator::IsShadowPass(uiPass) &&
                    (BSShaderManager::GetAccumulator()->bWaterReflection ||
                     BSShaderManager::GetAccumulator()->bWaterRefraction)) {
                    pRenderPassA[uiIndex].Clear(false);
                }
                else {
                    for (int32_t iBucket = 0; iBucket < 5; ++iBucket) {
                        if (!pRenderPassA[uiIndex].pListA[iBucket].m_pkHead) {
                            continue;
                        }
                        // The alpha-test flag passed to RenderPassImmediately is aePassEnd's value with its low byte
                        // cleared, then set, as the game does.
                        uint32_t uiAlphaTest = static_cast<uint32_t>(aePassEnd) & 0xFFFFFF00u;
                        switch (iBucket) {
                            case 0: {
                                BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_NOLOCK);
                                BSRenderState::SetAlphaTestEnable(false, BSRS_NOLOCK);
                                if (BSShaderManager::bTransparencyMultisampling) {
                                    BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
                                }
                                break;
                            }
                            case 1: {
                                BSRenderState::SetCullMode(NiStencilProperty::DRAW_BOTH, BSRS_LOCK);
                                BSRenderState::SetAlphaTestEnable(false, BSRS_NOLOCK);
                                if (BSShaderManager::bTransparencyMultisampling) {
                                    BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
                                }
                                break;
                            }
                            case 2: {
                                BSRenderState::SetCullMode(NiStencilProperty::DRAW_BOTH, BSRS_LOCK);
                                BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
                                uiAlphaTest |= 1u;
                                if (BSShaderManager::bTransparencyMultisampling) {
                                    BSRenderState::SetAlphaToCoverageEnable(true, BSRS_LOCK);
                                }
                                break;
                            }
                            case 3: {
                                BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_LOCK);
                                BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
                                uiAlphaTest |= 1u;
                                if (BSShaderManager::bTransparencyMultisampling) {
                                    BSRenderState::SetAlphaToCoverageEnable(true, BSRS_LOCK);
                                }
                                break;
                            }
                            case 4: {
                                BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_LOCK);
                                BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
                                uiAlphaTest |= 1u;
                                if (BSShaderManager::bTransparencyMultisampling) {
                                    BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
                                }
                                break;
                            }
                        }
                        PersistentPassList::ListItem* pItem = pRenderPassA[uiIndex].pListA[iBucket].m_pkHead;
                        while (pItem) {
                            BSShaderProperty::RenderPass* pPass = pItem->m_element;
                            pItem = pItem->m_pkNext;
                            BSBatchRenderer::RenderPassImmediately(
                                pPass, uiPass, static_cast<int32_t>(uiAlphaTest), abAlphaPass, 0
                            );
                        }
                        if (BSShaderManager::bTransparencyMultisampling) {
                            --BSRenderState::iLock[20];
                            BSRenderState::SetAlphaToCoverageEnable(false, BSRS_NOLOCK);
                        }
                        BSRenderState::SetAlphaToCoverageEnable(false, BSRS_NOLOCK);
                        if (BSRenderState::iLock[19]) {
                            --BSRenderState::iLock[19];
                        }
                        if (BSRenderState::iLock[4]) {
                            --BSRenderState::iLock[4];
                        }
                    }
                    pRenderPassA[uiIndex].Clear(false);
                }
            }
            pNode = pNode->GetNext();
        }
    }
    BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_NOLOCK);
    BSRenderState::SetAlphaTestEnable(false, BSRS_NOLOCK);
    BSBatchRenderer::EndPass();
}

bool BSBatchRenderer::PrepareNextRenderPass_SetNextBucket(uint32_t auiIndex, int32_t& aiBucket) {
    const PassGroup& kGroup = pRenderPassA[auiIndex];
    for (uint32_t i = 0; i < 5; ++i) {
        if (kGroup.pListA[i].m_pkHead) {
            aiBucket = static_cast<int32_t>(i);
            return true;
        }
    }
    return false;
}

// The parameter is not read.
void BSBatchRenderer::ClearRenderPasses(bool /*abUnused*/) {
    NiTMapItem<uint32_t, uint32_t>* pPos = RenderPassMap.GetFirstPos();
    while (pPos) {
        uint32_t uiPass;
        uint32_t uiGroup;
        RenderPassMap.GetNext(pPos, uiPass, uiGroup);

        PassGroup& kGroup = pRenderPassA[uiGroup];
        for (uint32_t i = 0; i < 5; ++i) {
            PersistentPassList& kList = kGroup.pListA[i];
            if (kList.Allocator.uiCount) {
                kList.CleanUnusedNodes();
                // Every node goes back to the free chain.
                kList.pNextFreeItem = kList.m_pkHead;
                kList.m_pkHead = nullptr;
                kList.m_pkTail = nullptr;
                kList.Allocator.uiCount = 0;
            }
        }
        kGroup.iSize = 0;
    }
    kActivePassIndexList.RemoveAll();
}

bool BSBatchRenderer::PrepareNextRenderPass_SetNextPass(uint32_t& auiPass, int32_t& aiBucket) {
    BSSimpleList<uint16_t>* pNode = &kActivePassIndexList;
    if (!pNode->m_pkNext && !pNode->m_item) {
        return false;
    }

    uint32_t uiPass = pNode->m_item;
    auiPass = uiPass;
    while (uiPass < uiCurrentFirstPass) {
        pNode = pNode->m_pkNext;
        if (!pNode) {
            return false;
        }
        uiPass = pNode->m_item;
        auiPass = uiPass;
    }
    if (uiPass > uiCurrentLastPass) {
        return false;
    }

    kActivePassIndexList.RemoveHead();
    uint32_t uiGroup = 0;
    RenderPassMap.GetAt(auiPass, uiGroup);
    return PrepareNextRenderPass_SetNextBucket(uiGroup, aiBucket);
}

bool BSBatchRenderer::PrepareNextRenderPass(uint32_t& auiPass, int32_t& aiBucket, bool& abFirstPass) {
    bool bResult = true;
    uint32_t uiGroup = 0;
    uint32_t uiPass = auiPass;
    bool bFound = false;
    if (uiPass) {
        RenderPassMap.GetAt(uiPass, uiGroup);
        bFound = PrepareNextRenderPass_SetNextBucket(uiGroup, aiBucket);
    }
    if (!bFound) {
        bResult = PrepareNextRenderPass_SetNextPass(auiPass, aiBucket);
        if (!bResult) {
            return bResult;
        }
    }

    RenderPassMap.GetAt(auiPass, uiGroup);
    // Copies the pass's first-pass byte as it is.
    reinterpret_cast<uint8_t&>(abFirstPass) = pRenderPassA[uiGroup].pListA[aiBucket].m_pkHead->m_element->bFirstPass;
    return bResult;
}

bool BSBatchRenderer::RenderNextRenderPass(
    uint32_t& auiPass,
    int32_t& aiBucket,
    bool& abFirstPass,
    int32_t abAlphaPass
) {
    uint32_t uiGroup = 0;
    RenderPassMap.GetAt(auiPass, uiGroup);
    // Keeps the first pass's byte as it is and compares each pass's byte with it.
    uint8_t ucFirstPass = pRenderPassA[uiGroup].pListA[aiBucket].m_pkHead->m_element->bFirstPass;
    // The alpha-test flag passed to RenderPassImmediately is uiGroup with its low byte cleared, then set, so the upper
    // bits of uiGroup are deliberately passed along with it, as the game does.
    uint32_t uiAlphaTest = uiGroup & 0xFFFFFF00u;

    // The asm pushes the bucket (0) as the lock count of the first SetCullMode.
    if (aiBucket == 0) {
        BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_NOLOCK);
        BSRenderState::SetAlphaTestEnable(false, BSRS_LOCK);
        BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
    }
    if (aiBucket == 1) {
        BSRenderState::SetCullMode(NiStencilProperty::DRAW_BOTH, BSRS_LOCK);
        BSRenderState::SetAlphaTestEnable(false, BSRS_LOCK);
        BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
    }
    if (aiBucket == 2) {
        BSRenderState::SetCullMode(NiStencilProperty::DRAW_BOTH, BSRS_LOCK);
        BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
        uiAlphaTest |= 1u;
        if (BSShaderManager::bTransparencyMultisampling) {
            BSRenderState::SetAlphaToCoverageEnable(true, BSRS_LOCK);
        }
    }
    if (aiBucket == 3) {
        BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_LOCK);
        BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
        uiAlphaTest |= 1u;
        if (BSShaderManager::bTransparencyMultisampling) {
            BSRenderState::SetAlphaToCoverageEnable(true, BSRS_LOCK);
        }
    }
    if (aiBucket == 4) {
        BSRenderState::SetCullMode(NiStencilProperty::DRAW_CCW, BSRS_LOCK);
        BSRenderState::SetAlphaTestEnable(true, BSRS_LOCK);
        uiAlphaTest |= 1u;
        if (BSShaderManager::bTransparencyMultisampling) {
            BSRenderState::SetAlphaToCoverageEnable(false, BSRS_LOCK);
        }
    }

    bool bResult = false;
    BSShaderProperty::RenderPass* pPass = pRenderPassA[uiGroup].pListA[aiBucket].m_pkHead->m_element;
    while (pPass) {
        if (pPass->bFirstPass != ucFirstPass) {
            reinterpret_cast<uint8_t&>(abFirstPass) = pPass->bFirstPass;
            bResult = true;
            break;
        }

        RenderPassImmediately(pPass, auiPass, static_cast<int32_t>(uiAlphaTest), abAlphaPass, false);

        // Removes the list head as BSTPersistentList::RemoveHead does: the head node moves to the free chain.
        PassGroup& kGroup = pRenderPassA[uiGroup];
        PersistentPassList& kList = kGroup.pListA[aiBucket];
        PersistentPassList::ListItem* pHead = kList.m_pkHead;
        if (pHead) {
            if (pHead == kList.m_pkTail) {
                kList.m_pkTail = nullptr;
            }
            PersistentPassList::ListItem* pNext = nullptr;
            if (pHead->m_pkNext) {
                pNext = pHead->m_pkNext;
                pNext->m_pkPrev = nullptr;
            }
            if (kList.pNextFreeItem) {
                kList.pNextFreeItem->m_pkPrev = kList.m_pkHead;
            }
            kList.m_pkHead->m_pkNext = kList.pNextFreeItem;
            kList.pNextFreeItem = kList.m_pkHead;
            --kList.Allocator.uiCount;
            kList.m_pkHead = pNext;
        }
        --kGroup.iSize;

        PersistentPassList::ListItem* pNewHead = pRenderPassA[uiGroup].pListA[aiBucket].m_pkHead;
        if (!pNewHead) {
            break;
        }
        pPass = pNewHead->m_element;
    }

    if (BSRenderState::iLock[19]) {
        --BSRenderState::iLock[19];
    }
    if (BSRenderState::iLock[4]) {
        --BSRenderState::iLock[4];
    }
    if (BSShaderManager::bTransparencyMultisampling) {
        --BSRenderState::iLock[20];
        BSRenderState::SetAlphaToCoverageEnable(false, BSRS_NOLOCK);
    }
    BSRenderState::SetAlphaToCoverageEnable(false, BSRS_NOLOCK);

    if (bResult) {
        return true;
    }
    return PrepareNextRenderPass(auiPass, aiBucket, abFirstPass);
}

bool BSBatchRenderer::QWaterRefractionPassWithinRange(
    BSShaderManager::eBSSMPassEnum aeStartRange,
    BSShaderManager::eBSSMPassEnum aeEndRange
) {
    BSSimpleList<uint16_t>* pkList = &kActivePassIndexList;
    if (!pkList) {
        return false;
    }
    while (true) {
        uint32_t m_item = pkList->m_item;
        if (m_item <= aeEndRange &&
            (m_item >= aeStartRange &&
             (m_item == 650 ||
              (m_item == 653 ||
               (m_item == 654 ||
                (m_item == 657 ||
                 (m_item == 658 ||
                  (m_item == 661 ||
                   (m_item == 662 ||
                    (m_item == 665 ||
                     (m_item == 684 ||
                      (m_item == 687 ||
                       (m_item == 692 ||
                        (m_item == 693 ||
                         (m_item == 688 ||
                          (m_item == 691 ||
                           (m_item == 696 ||
                            (m_item == 699 ||
                             (m_item == 667 ||
                              (m_item == 668 ||
                               (m_item == 671 ||
                                (m_item == 672 ||
                                 (m_item == 675 ||
                                  (m_item == 676 ||
                                   (m_item == 679 ||
                                    (m_item == 680 ||
                                     (m_item == 702 ||
                                      (m_item == 705 ||
                                       (m_item == 710 ||
                                        (m_item == 711 ||
                                         (m_item == 706 ||
                                          (m_item == 709 ||
                                           (m_item == 714 ||
                                            (m_item == 717 || m_item == 701)))))))))))))))))))))))))))))))))) {
            break;
        }
        pkList = pkList->GetNext();
        if (!pkList) {
            return false;
        }
    }
    return true;
}

void BSBatchRenderer::RegisterAlphaPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t aiIndex) {
    if (iGroupingAlphas > 0) {
        pCurrentAlphaGroup[aiIndex]->RegisterPass(apRenderPass, 0);
    }
    else {
        pAlphaGroups[aiIndex]->RegisterPass(apRenderPass, 0);
    }
}

// abAddToFront is int32_t so that the caller's whole value is passed on; a bool would be zero-extended.
void BSBatchRenderer::RegisterPassGeometryGroup(
    BSShaderProperty::RenderPass* apRenderPass,
    GEOMETRY_GROUP_ENUM aeGroup,
    int32_t abAddToFront
) {
    pGeometryGroups[aeGroup]->RegisterPass(apRenderPass, abAddToFront);
}

void BSBatchRenderer::SortAlphaPasses(const NiCamera* apCamera, bool abGetSortByClosestPoint) {
    bSortByClosestPoint = abGetSortByClosestPoint;
    pSortCamera = apCamera;

    PersistentPassList_Sort(&pAlphaGroups[0]->PassList, &CompareRenderPass);
    PersistentPassList_Sort(&pAlphaGroups[1]->PassList, &CompareRenderPass);
}

// The depth of the side's next alpha group: the farther of the open group's first pass (when it has passes) and the
// head of the side's sorted group list; -FLT_MAX when both are empty.
float BSBatchRenderer::GetNextAlphaDepth(uint32_t aiIndex, const NiCamera* apCamera, bool abGetSortByClosestPoint) {
    float fDirX = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
    float fDirY = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
    BSBatchRenderer::GeometryGroup* pGroup = pAlphaGroups[aiIndex];
    float fDirZ = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
    NiTPointerList<BSBatchRenderer::GeometryGroup*>& kList = kAlphaGroupLists[aiIndex];
    if (!pGroup->usCount) {
        if (!kList.m_uiCount) {
            return -3.4028235e+38f;
        }
        return kList.m_pkHead->m_element->fDepth;
    }
    const NiBound* pBound = pGroup->PassList.m_pkHead->m_element->pGeometry->m_kWorldBound;
    if (!pBound) {
        pBound = &NiAVObject::NullBoundS;
    }
    float fDepth;
    if (abGetSortByClosestPoint) {
        float fCenterDepth = pBound->m_kCenter.y * fDirY + pBound->m_kCenter.x * fDirX + pBound->m_kCenter.z * fDirZ;
        fDepth = fCenterDepth - pBound->m_fRadius;
    }
    else {
        fDepth = pBound->m_kCenter.y * fDirY + pBound->m_kCenter.x * fDirX + pBound->m_kCenter.z * fDirZ;
    }
    if (!kList.m_uiCount) {
        return fDepth;
    }
    return BSMAX(kList.m_pkHead->m_element->fDepth, fDepth);
}

// Opens a new alpha group on each side at the bound's depth along the camera direction and inserts it into the side's
// list, which stays sorted farthest first; nested calls only count.
void BSBatchRenderer::StartGroupingAlphas(
    const NiBound& aBound,
    bool abZPrePass,
    const NiCamera* apCamera,
    bool abGetSortByClosestPoint
) {
    if (iGroupingAlphas || !apCamera) {
        ++iGroupingAlphas;
        return;
    }
    for (int32_t iSide = 0; iSide < 2; ++iSide) {
        NiTPointerList<BSBatchRenderer::GeometryGroup*>& kList = kAlphaGroupLists[iSide];
        BSBatchRenderer::GeometryGroup* pGroup = static_cast<BSBatchRenderer::GeometryGroup*>(
            MemoryManager_Singleton.Allocate(sizeof(BSBatchRenderer::GeometryGroup))
        );
        if (pGroup) {
            pGroup = ::new (pGroup) BSBatchRenderer::GeometryGroup(759);
        }
        else {
            pGroup = nullptr;
        }
        // Clears the group's passes, as GeometryGroup::ClearPasses does.
        pGroup->PassList.CleanUnusedNodes();
        pGroup->PassList.pNextFreeItem = pGroup->PassList.m_pkHead;
        pGroup->PassList.m_pkHead = nullptr;
        pGroup->PassList.m_pkTail = nullptr;
        pGroup->PassList.Allocator.uiCount = 0;
        BSBatchRenderer* pBatchRenderer = pGroup->pBatchRenderer;
        if (pBatchRenderer) {
            pBatchRenderer->ClearRenderPasses(false);
        }
        pGroup->usCount = 0;
        float fDirX = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
        float fDirY = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
        float fDirZ = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
        if (abGetSortByClosestPoint) {
            float fCenterDepth = aBound.m_kCenter.x * fDirX + aBound.m_kCenter.y * fDirY + aBound.m_kCenter.z * fDirZ;
            pGroup->fDepth = fCenterDepth - aBound.m_fRadius;
        }
        else {
            pGroup->fDepth = aBound.m_kCenter.x * fDirX + aBound.m_kCenter.y * fDirY + aBound.m_kCenter.z * fDirZ;
        }
        if (abZPrePass) {
            pGroup->ucFlags &= ~1u;
        }
        else {
            pGroup->ucFlags |= 1u;
        }

        // Inserts before the first group the new one is farther than.
        bool bInserted = false;
        NiTListItem<BSBatchRenderer::GeometryGroup*>* pPos = kList.m_pkHead;
        while (pPos) {
            if (pGroup->fDepth > pPos->m_element->fDepth) {
                NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
                NiTAbstractPoolAllocator<uint32_t>::AllocNode* pFree = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
                if (!NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem) {
                    NiTAbstractPoolAllocator<uint32_t>::CreateFreeMem();
                    pFree = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
                }
                NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pFree->m_pkNext;
                pFree->m_element = 0;
                pFree->m_pkNext = nullptr;
                pFree->m_pkData = nullptr;
                if (!--NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount) {
                    NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
                }
                // The pool's node is the list's item (the same 12-byte node the allocator hands out).
                void* pNodeMemory = pFree;
                NiTListItem<BSBatchRenderer::GeometryGroup*>* pNode =
                    static_cast<NiTListItem<BSBatchRenderer::GeometryGroup*>*>(pNodeMemory);
                pNode->m_element = pGroup;
                pNode->m_pkNext = pPos;
                pNode->m_pkPrev = pPos->m_pkPrev;
                if (pPos->m_pkPrev) {
                    pPos->m_pkPrev->m_pkNext = pNode;
                }
                else {
                    kList.m_pkHead = pNode;
                }
                pPos->m_pkPrev = pNode;
                ++kList.m_uiCount;
                bInserted = true;
            }
            pPos = pPos->m_pkNext;
            if (bInserted) {
                break;
            }
        }
        if (!bInserted) {
            // NiTList::AddTail. The list's allocator is folded into the item count field (NiTPointerAllocator.h), so
            // the count's address is used as the allocator's this.
            NiTListItem<BSBatchRenderer::GeometryGroup*>* pNode =
                static_cast<NiTListItem<BSBatchRenderer::GeometryGroup*>*>(
                    reinterpret_cast<NiTPointerAllocator<uint32_t>*>(&kList.m_uiCount)->Allocate()
                );
            pNode->m_element = pGroup;
            pNode->m_pkNext = nullptr;
            pNode->m_pkPrev = kList.m_pkTail;
            if (kList.m_pkTail) {
                kList.m_pkTail->m_pkNext = pNode;
            }
            else {
                kList.m_pkHead = pNode;
            }
            ++kList.m_uiCount;
            kList.m_pkTail = pNode;
        }
        pCurrentAlphaGroup[iSide] = pGroup;
    }
    ++iGroupingAlphas;
    bAlphaGroupZPrePass = abZPrePass;
}

// Ends one level of alpha grouping; at the last, each side's current group is dropped from its list and deleted
// when it holds no passes, and both are cleared. The count stops at 0.
void BSBatchRenderer::StopGroupingAlphas() {
    int32_t iCount = iGroupingAlphas - 1;
    if (iCount < 0) {
        iCount = 0;
    }
    iGroupingAlphas = iCount;
    if (iCount > 0) {
        return;
    }
    for (int32_t iSide = 0; iSide < 2; ++iSide) {
        if (pCurrentAlphaGroup[iSide]->usCount == 0) {
            kAlphaGroupLists[iSide].Remove(pCurrentAlphaGroup[iSide]);
            GeometryGroup* pGroup = pCurrentAlphaGroup[iSide];
            if (pGroup) {
                pGroup->~GeometryGroup();
                MemoryManager_Singleton.Deallocate(pGroup);
            }
        }
        pCurrentAlphaGroup[iSide] = nullptr;
    }
    bAlphaGroupZPrePass = false;
}

bool BSBatchRenderer::SkipNextRenderPass(uint32_t& auiPass, int32_t& aiBucket, bool& abFirstPass, bool abAlphaPass) {
    uint32_t v7 = auiPass;
    uint32_t iTestAlpha = 0;
    RenderPassMap.GetAt(v7, iTestAlpha);
    pRenderPassA[iTestAlpha].Clear(false);
    return PrepareNextRenderPass(auiPass, aiBucket, abFirstPass);
}

// The alpha group list's head is removed as NiTPointerList::RemoveHead does: the node is unlinked, its element cleared
// and the node handed back to the shared node pool under its spin lock, then the count drops.
void BSBatchRenderer::RenderNextAlpha(
    uint32_t aiIndex,
    const NiCamera* apCamera,
    bool abGetSortByClosestPoint,
    float afStopDepth
) {
    BSShaderProperty::RenderPass* pkItem;
    BSBatchRenderer::GeometryGroup* m_element;
    BSShaderProperty::RenderPass* pkItem_3;
    float kCamDir = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
    float kCamDir_4 = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
    uint32_t v5 = aiIndex;
    float kCamDir_8 = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
    PersistentPassList::ListItem* m_pkHead = pAlphaGroups[aiIndex]->PassList.m_pkHead;
    if (m_pkHead) {
        pkItem = m_pkHead->m_element;
        pkItem_3 = pkItem;
    }
    else {
        pkItem_3 = nullptr;
        pkItem = nullptr;
    }
    float fdepth_2 = 0.0f;
    double v9 = kCamDir_8;
    double v10 = kCamDir_4;
    double v11 = kCamDir;
    if (pkItem) {
        if (abGetSortByClosestPoint) {
            NiBound* m_kWorldBound = (NiBound*)pkItem->pGeometry->m_kWorldBound;
            if (!m_kWorldBound) {
                m_kWorldBound = &NiAVObject::NullBoundS;
            }
            NiBound* v13 = (NiBound*)pkItem->pGeometry->m_kWorldBound;
            if (!v13) {
                v13 = &NiAVObject::NullBoundS;
            }
            float v41 =
                v9 * m_kWorldBound->m_kCenter.z + (v10 * m_kWorldBound->m_kCenter.y + v11 * m_kWorldBound->m_kCenter.x);
            fdepth_2 = v41 - v13->m_fRadius;
        }
        else {
            NiBound* v14 = (NiBound*)pkItem->pGeometry->m_kWorldBound;
            if (!v14) {
                v14 = &NiAVObject::NullBoundS;
            }
            fdepth_2 = v9 * v14->m_kCenter.z + (v10 * v14->m_kCenter.y + v11 * v14->m_kCenter.x);
        }
    }
    NiTPointerList<BSBatchRenderer::GeometryGroup*>* pList = &kAlphaGroupLists[aiIndex];
    if (pList->m_uiCount) {
        m_element = pList->m_pkHead->m_element;
    }
    else {
        m_element = nullptr;
    }
    float afStopDeptha = afStopDepth - 0.0010000000474974513;
    do {
        while (true) {
            BSBatchRenderer* v17;
            NiTListItem<BSBatchRenderer::GeometryGroup*>* pNode;
            NiTListItem<BSBatchRenderer::GeometryGroup*>* pNext;
            void* pItemMemory;
            NiTAbstractPoolAllocator<uint32_t>::AllocNode* pFree;
            if (!m_element) {
                goto LABEL_36;
            }
            if (!(fdepth_2 < static_cast<double>(m_element->fDepth)) && pkItem) {
                break;
            }
            if (!(afStopDeptha <= static_cast<double>(m_element->fDepth))) {
                goto LABEL_36;
            }
            if ((m_element->ucFlags & 1) != 0) {
                BSBatchRenderer::RenderPersistentPassList(m_element->PassList, 1);
            }
            else {
                BSBatchRenderer* pBatchRenderer = m_element->pBatchRenderer;
                if (pBatchRenderer) {
                    pBatchRenderer->RenderBatches(BSShaderManager::BSSM_ZONLY, BSShaderManager::BSSM_BLOOD_SPLATTER, 1);
                }
            }
            v17 = m_element->pBatchRenderer;
            if (v17) {
                v17->kActivePassIndexList.RemoveAll();
            }
            m_element->usCount = 0;
            m_element->~GeometryGroup();
            MemoryManager_Singleton.Deallocate(m_element);
            // EXE-INLINED none: NiTPointerListBase::RemoveHead
            pNode = pList->m_pkHead;
            pNext = pNode->m_pkNext;
            pList->m_pkHead = pNext;
            if (pNext) {
                pNext->m_pkPrev = nullptr;
            }
            else {
                pList->m_pkTail = nullptr;
            }
            pNode->m_element = 0;
            // EXE-INLINED none: NiTAbstractPoolAllocator::Deallocate. As Gamebryo's DeleteItem, the node goes to the
            // pool allocator as untyped memory (void*), which takes it as its own 12-byte AllocNode.
            pItemMemory = pNode;
            pFree = static_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(pItemMemory);
            NiTAbstractPoolAllocator<uint32_t>::Lock();
            pFree->m_pkData = nullptr;
            pFree->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
            NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pFree;
            NiTAbstractPoolAllocator<uint32_t>::Unlock();
            --pList->m_uiCount;
            if (!pList->m_uiCount) {
                v5 = aiIndex;
                m_element = nullptr;
            LABEL_36:;
                if (!pkItem) {
                    goto LABEL_77;
                }
                break;
            }
            m_element = pList->m_pkHead->m_element;
            v5 = aiIndex;
        }
        if (afStopDeptha <= static_cast<double>(fdepth_2)) {
            double fdepth;
            BSShaderPPLightingProperty* m_pObject = static_cast<BSShaderPPLightingProperty*>(
                pkItem->pGeometry->m_PropertyState.m_aspProps[NiProperty::SHADE].m_pObject
            );
            if (m_pObject) {
                int32_t iShaderPropertyType;
                if (BSShaderManager::eRenderMode != BSShaderManager::BSSM_RENDER_LOCALMAP ||
                    ((iShaderPropertyType = m_pObject->iShaderPropertyType,
                      iShaderPropertyType != BSShaderManager::BSSM_PROPERTYTYPE_NOLIGHTING) &&
                     iShaderPropertyType != BSShaderManager::BSSM_PROPERTYTYPE_PARTICLE)) {
                    char v26;
                    uint32_t v25 = m_pObject->ulFlags[0];
                    if ((v25 & BSS1_Decal_Single_Pass) != 0 || (v25 & BSS1_Dynamic_Decal_Single_Pass) != 0) {
                        float fdepthBias = BSShaderProperty::fDepthBiasUnit;
                        v26 = 1;
                        if (!BSShaderManager::bUseDepthBias) {
                            fdepthBias = 0.0f;
                        }
                        BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
                        BSRenderState::SetDepthBias(fdepthBias, BSRS_LOCK);
                    }
                    else {
                        v26 = 0;
                    }
                    BSBatchRenderer::RenderPassImmediately(pkItem_3, pkItem_3->usPassEnum, 1, 1, 1);
                    if (v26) {
                        --BSRenderState::iLock[1];
                        BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
                        --BSRenderState::iLock[3];
                        BSRenderState::SetDepthBias(0.0f, BSRS_NOLOCK);
                    }
                    v5 = aiIndex;
                }
            }
            BSBatchRenderer::GeometryGroup* v27 = pAlphaGroups[v5];
            if (v27->PassList.m_pkHead) {
                --v27->usCount;
                PersistentPassList::ListItem* v28 = v27->PassList.m_pkHead;
                if (v28) {
                    if (v28 == v27->PassList.m_pkTail) {
                        v27->PassList.m_pkTail = nullptr;
                    }
                    PersistentPassList::ListItem* v29 = nullptr;
                    if (v28->m_pkNext) {
                        v29 = v28->m_pkNext;
                        v28->m_pkNext->m_pkPrev = nullptr;
                    }
                    PersistentPassList::ListItem* pNextFreeItem = v27->PassList.pNextFreeItem;
                    if (pNextFreeItem) {
                        pNextFreeItem->m_pkPrev = v27->PassList.m_pkHead;
                    }
                    v27->PassList.m_pkHead->m_pkNext = v27->PassList.pNextFreeItem;
                    PersistentPassList::ListItem* v31 = v27->PassList.m_pkHead;
                    --v27->PassList.Allocator.uiCount;
                    v27->PassList.pNextFreeItem = v31;
                    v27->PassList.m_pkHead = v29;
                }
            }
            PersistentPassList::ListItem* v32 = pAlphaGroups[v5]->PassList.m_pkHead;
            if (!v32) {
                pkItem_3 = nullptr;
                pkItem = nullptr;
                goto LABEL_77;
            }
            BSShaderProperty::RenderPass* pkItem_2 = v32->m_element;
            pkItem_3 = pkItem_2;
            if (!pkItem_2) {
                pkItem = nullptr;
                goto LABEL_77;
            }
            if (abGetSortByClosestPoint) {
                NiTriStrips* pGeometry = static_cast<NiTriStrips*>(pkItem_2->pGeometry);
                NiBound* v35 = (NiBound*)pkItem_2->pGeometry->m_kWorldBound;
                if (!v35) {
                    v35 = &NiAVObject::NullBoundS;
                }
                NiBound* v36 = (NiBound*)pGeometry->m_kWorldBound;
                if (!v36) {
                    v36 = &NiAVObject::NullBoundS;
                }
                float v43 = v35->m_kCenter.y * kCamDir_4 + v35->m_kCenter.x * kCamDir + v35->m_kCenter.z * kCamDir_8;
                fdepth = v43 - v36->m_fRadius;
            }
            else {
                NiBound* v38 = (NiBound*)pkItem_2->pGeometry->m_kWorldBound;
                if (!v38) {
                    v38 = &NiAVObject::NullBoundS;
                }
                fdepth = v38->m_kCenter.y * kCamDir_4 + v38->m_kCenter.x * kCamDir + v38->m_kCenter.z * kCamDir_8;
            }
            pkItem = pkItem_3;
            fdepth_2 = fdepth;
        }
        if (pkItem && afStopDeptha <= static_cast<double>(fdepth_2)) {
            continue;
        }
    LABEL_77:;
        if (!m_element || !(afStopDeptha <= static_cast<double>(m_element->fDepth))) {
            break;
        }
    }
    while (!std::isnan(fdepth_2));
}

// Restores the last shader's pass states and, after the passes that biased the mip level, resets the bias of their
// sampler on the device.
void BSBatchRenderer::EndPass() {
    if (BSBatchRenderer::pLastShader) {
        BSBatchRenderer::pLastShader->RestorePassRenderStates(static_cast<uint32_t>(BSBatchRenderer::uiLastPass));
        if (!BSBatchRenderer::bFirstPass) {
            BSBatchRenderer::pLastShader->RestoreGeometryNonFirstPass();
        }
        if (BSBatchRenderer::uiLastPass == BSShaderManager::BSSM_ADT_Mn ||
            BSBatchRenderer::uiLastPass == BSShaderManager::BSSM_ADT_MnShp) {
            static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)
                ->m_pkD3DDevice9->SetSamplerState(7, D3DSAMP_MIPMAPLODBIAS, 0);
        }
        else if (BSBatchRenderer::uiLastPass == BSShaderManager::BSSM_LANDLO_A) {
            static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)
                ->m_pkD3DDevice9->SetSamplerState(2, D3DSAMP_MIPMAPLODBIAS, 0);
        }
    }
    BSBatchRenderer::uiLastPass = 0;
    BSBatchRenderer::pLastShader = nullptr;
}

void BSBatchRenderer::RenderPassImmediately(
    BSShaderProperty::RenderPass* prenderpass,
    uint32_t uicurrentPass,
    int32_t balphaTest,
    int32_t abAlphaPass,
    int32_t abGeometryRenderStates
) {
    if (prenderpass->usPassEnum == uicurrentPass) {
        BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(uicurrentPass);
        BSShaderManager::pCurrentRenderPass = prenderpass;
        BSShader* m_spShader = prenderpass->pGeometry->GetShader();
        char v6 = 0;
        if (BSBatchRenderer::uiLastPass != uicurrentPass ||
            (uicurrentPass == 758 || m_spShader != BSBatchRenderer::pLastShader)) {
            v6 = 1;
            BSBatchRenderer::BeginPass(uicurrentPass, m_spShader);
        }
        if (BSShaderManager::bTransparencyMultisampling) {
            NiAlphaProperty* m_pObject =
                static_cast<NiAlphaProperty*>(prenderpass->pGeometry->m_PropertyState.m_aspProps[0].m_pObject);
            if (m_pObject && (m_pObject->m_uFlags & 512) != 0) {
                BSRenderState::SetAlphaToCoverageEnable(true, BSRS_NOLOCK);
            }
            else {
                BSRenderState::SetAlphaToCoverageEnable(false, BSRS_NOLOCK);
            }
        }
        if (BSShaderManager::pRenderer.m_pObject->IsHardwareSkinned(prenderpass->pGeometry, nullptr)) {
            BSBatchRenderer::RenderPassImmediately_Skinned(
                prenderpass, balphaTest, abAlphaPass, abGeometryRenderStates
            );
        }
        else {
            NiParticles* v8 = prenderpass->pGeometry->IsParticlesGeom();
            NiGeometry* pGeometry = prenderpass->pGeometry;
            if (v8) {
                pGeometry->RenderImmediate(NiRenderer::ms_pkRenderer);
            }
            else {
                if (pGeometry->IsLinesGeom()) {
                    prenderpass->pGeometry->RenderImmediate(NiRenderer::ms_pkRenderer);
                }
                else {
                    BSBatchRenderer::RenderPassImmediately_Standard(
                        prenderpass, balphaTest, abAlphaPass, abGeometryRenderStates
                    );
                }
            }
        }
        if ((v6) && (BSBatchRenderer::pLastShader)) {
            int32_t iShaderType = BSBatchRenderer::pLastShader->iShaderType;
            if (iShaderType < 1 || iShaderType > 5) {
                if (iShaderType == 17) {
                    static_cast<Lighting30Shader*>(BSBatchRenderer::pLastShader)
                        ->Lighting30Shader::DisableNonVolatileConstantsForPass(uicurrentPass);
                }
            }
            else {
                static_cast<ShadowLightShader*>(BSBatchRenderer::pLastShader)
                    ->ShadowLightShader::DisableNonVolatileConstantsForPass(uicurrentPass);
            }
        }
    }
}

void BSBatchRenderer::RenderPersistentPassList(PersistentPassList& arPassList, int32_t abAlphaPass) {
    if (arPassList.Allocator.uiCount == 0) {
        return;
    }
    EndPass();
    PersistentPassList::ListItem* pNode = arPassList.m_pkHead;
    if (pNode) {
        PersistentPassList::ListItem* pNext = pNode->m_pkNext;
        BSShaderProperty::RenderPass* pPass = pNode->m_element;
        while (pPass) {
            if (pPass->pGeometry) {
                RenderPassImmediately(pPass, pPass->usPassEnum, true, abAlphaPass, true);
            }
            if (!pNext) {
                break;
            }
            pPass = pNext->m_element;
            pNext = pNext->m_pkNext;
        }
    }
    arPassList.CleanUnusedNodes();
    arPassList.pNextFreeItem = arPassList.m_pkHead;
    arPassList.m_pkHead = nullptr;
    arPassList.m_pkTail = nullptr;
    arPassList.Allocator.uiCount = 0;
    EndPass();
}

// The flags are 32-bit ints of which only the low byte is tested: the header says why.
void BSBatchRenderer::RenderPassImmediately_Standard(
    BSShaderProperty::RenderPass* apRenderPass,
    int32_t aiAlphaTest,
    int32_t aiAlphaPass,
    int32_t aiGeometryRenderStates
) {
    const bool abAlphaTest = static_cast<uint8_t>(aiAlphaTest) != 0;
    const bool abAlphaPass = static_cast<uint8_t>(aiAlphaPass) != 0;
    const bool abGeometryRenderStates = static_cast<uint8_t>(aiGeometryRenderStates) != 0;
    NiGeometry* pgeometry = apRenderPass->pGeometry;
    NiGeometryData* pgeomData = apRenderPass->pGeometry->m_spModelData.m_pObject;
    BSShader* pshader = apRenderPass->pGeometry->GetShader();
    uint32_t uipass = apRenderPass->usPassEnum;
    NiDX9Renderer* v7 = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    NiGeometryGroup* v9 = v7->m_pkDefaultGroup;
    NiD3DGeometryGroupManager* v8 = v7->m_pkGeometryGroupManager;
    NiRenderer* prenderer = NiRenderer::ms_pkRenderer;
    NiRenderer::ms_pkRenderer->m_pkCurrProp = &apRenderPass->pGeometry->m_PropertyState;
    v7->m_pkCurrEffects = nullptr;
    v8->AddObjectToGroup(v9, pgeomData, nullptr, false, 0, 0);
    NiGeometryBufferData* pbuffData = pgeomData->m_pkBuffData;
    switch (uipass) {
        case 202u: {
            static_cast<ShadowLightShader*>(pshader)->ShadowLightShader::Setup_ADT4_Opt(&pgeometry->m_PropertyState);
            if (abAlphaTest) {
                pshader->SetupGeometryAlphaTesting(&pgeometry->m_PropertyState);
            }
            if (abAlphaPass) {
                pshader->SetupGeometryAlphaBlending(&pgeometry->m_PropertyState);
            }
            goto LABEL_32;
        }
        case 209u: {
            static_cast<ShadowLightShader*>(pshader)->ShadowLightShader::Setup_ADTS10_Opt(&pgeometry->m_PropertyState);
            if (abAlphaTest) {
                pshader->SetupGeometryAlphaTesting(&pgeometry->m_PropertyState);
            }
            if (abAlphaPass) {
                pshader->SetupGeometryAlphaBlending(&pgeometry->m_PropertyState);
            }
            if (abGeometryRenderStates) {
                goto LABEL_33;
            }
            break;
        }
        case 252u:
        case 253u: {
            static_cast<ShadowLightShader*>(pshader)->ShadowLightShader::Setup_ADT_Opt(&pgeometry->m_PropertyState);
            goto LABEL_3;
        }
        case 258u: {
            static_cast<ShadowLightShader*>(pshader)->ShadowLightShader::Setup_ADT_PxOpt(&pgeometry->m_PropertyState);
        LABEL_3:;
            if (abAlphaTest) {
                pshader->SetupGeometryAlphaTesting(&pgeometry->m_PropertyState);
            }
            if (abAlphaPass) {
                pshader->SetupGeometryAlphaBlending(&pgeometry->m_PropertyState);
            }
            break;
        }
        default: {
            pshader->SetupGeometryTextures(&pgeometry->m_PropertyState);
            pshader->SetupGeometryConstants(&pgeometry->m_PropertyState);
            if (apRenderPass->bFirstPass) {
                if (abAlphaPass && !BSShaderAccumulator::IsShadowPass(uipass)) {
                    pshader->SetupGeometryAlphaBlending(&pgeometry->m_PropertyState);
                }
                if (abAlphaTest && (uipass < 4 || uipass > 5) && (uipass < 14 || uipass > 15) && uipass != 570) {
                    pshader->SetupGeometryAlphaTesting(&pgeometry->m_PropertyState);
                }
            }
            else {
                if (BSBatchRenderer::bFirstPass && !BSShaderAccumulator::IsShadowPass(uipass)) {
                    pshader->SetupGeometryNonFirstPass(&pgeometry->m_PropertyState);
                    BSBatchRenderer::bFirstPass = false;
                }
            }
        LABEL_32:;
            if (abGeometryRenderStates) {
            LABEL_33:;
                // Passes the pass's whole byte: read as a bool, it is not normalised.
                pshader->SetupGeometryRenderStates(
                    &pgeometry->m_PropertyState, reinterpret_cast<const bool&>(apRenderPass->bFirstPass)
                );
            }
            break;
        }
    }
    pshader->PrepareGeometryForRendering(pgeometry, nullptr, pbuffData, &pgeometry->m_PropertyState);
    pgeometry->OnlyRenderImmediate(prenderer);
    pshader->PostGeometry(&pgeometry->m_PropertyState);
}

// Only the low byte of each flag is tested: the header says why.
void BSBatchRenderer::RenderPassImmediately_Skinned(
    BSShaderProperty::RenderPass* apRenderPass,
    int32_t aiAlphaTest,
    int32_t aiAlphaPass,
    int32_t aiGeometryRenderStates
) {
    const bool abAlphaTest = static_cast<uint8_t>(aiAlphaTest) != 0;
    const bool abAlphaPass = static_cast<uint8_t>(aiAlphaPass) != 0;
    const bool abGeometryRenderStates = static_cast<uint8_t>(aiGeometryRenderStates) != 0;
    NiGeometry* pgeometry = apRenderPass->pGeometry;
    NiSkinInstance* pkSkinInstance = apRenderPass->pGeometry->m_spSkinInstance.m_pObject;
    NiSkinPartition* m_pObject = pkSkinInstance->m_spSkinPartition.m_pObject;
    BSShader* pshader = apRenderPass->pGeometry->GetShader();
    uint32_t uipass = apRenderPass->usPassEnum;
    NiDX9Renderer* v8 = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    NiPropertyState* ppropState = &apRenderPass->pGeometry->m_PropertyState;
    NiGeometryData* pgeomData = apRenderPass->pGeometry->m_spModelData.m_pObject;
    NiSkinPartition::Partition* pSkinPartitions = m_pObject->m_pkPartitions;
    NiGeometryGroup* v17 = v8->m_pkDefaultGroup;
    NiD3DGeometryGroupManager* v9 = v8->m_pkGeometryGroupManager;
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    NiRenderer::ms_pkRenderer->m_pkCurrProp = ppropState;
    v8->m_pkCurrEffects = nullptr;
    v9->AddObjectToGroup(v17, pgeomData, pkSkinInstance, true, 0, 0);
    uint32_t uiPartitionCount = m_pObject->m_uiPartitions;
    uint32_t v10 = 0;
    BSDismemberSkinInstance::PartitionData* pDismemberPartitions = nullptr;
    if (pkSkinInstance->GetRTTI() == &BSDismemberSkinInstance::ms_RTTI) {
        pDismemberPartitions = static_cast<BSDismemberSkinInstance*>(pkSkinInstance)->pPartitionDataArray;
    }
    bool bFirstDraw = true;
    uint32_t v22 = 0;
    if (uiPartitionCount) {
        do {
            if (!pDismemberPartitions || pDismemberPartitions[v10].bEnabled) {
                NiSkinPartition::Partition* pkCurrPartition = &pSkinPartitions[v10];
                NiGeometryData::RendererData* pkBuffData = (NiGeometryData::RendererData*)pkCurrPartition->m_pkBuffData;
                const NiBound* WorldBound = &pgeometry->GetWorldBound();
                pshader->SetupBoneMatrices(
                    pgeometry, pkSkinInstance, pkCurrPartition, pkBuffData, ppropState, nullptr, pgeometry->m_kWorld,
                    *WorldBound
                );
                if (bFirstDraw) {
                    pshader->SetupGeometryTextures(ppropState);
                    pshader->SetupGeometryConstants(ppropState);
                    if (!apRenderPass->bFirstPass || BSShaderAccumulator::IsShadowPass(uipass)) {
                        if (BSBatchRenderer::bFirstPass) {
                            pshader->SetupGeometryNonFirstPass(ppropState);
                            BSBatchRenderer::bFirstPass = false;
                        }
                    }
                    else {
                        if (abAlphaPass) {
                            pshader->SetupGeometryAlphaBlending(ppropState);
                        }
                        if (abAlphaTest && (uipass < 4 || uipass > 5) && (uipass < 14 || uipass > 15)) {
                            pshader->SetupGeometryAlphaTesting(ppropState);
                        }
                    }
                    if (abGeometryRenderStates) {
                        // Passes the pass's whole bFirstPass byte, as above.
                        pshader->SetupGeometryRenderStates(
                            ppropState, reinterpret_cast<const bool&>(apRenderPass->bFirstPass)
                        );
                    }
                    bFirstDraw = false;
                }
                NiGeometryBufferData* pkBuffData_2 = static_cast<NiGeometryBufferData*>(
                    pshader->PrepareGeometryForRendering(pgeometry, pkCurrPartition, pkBuffData, ppropState)
                );
                pRenderer->Do_DrawPartition(pkBuffData_2, pkCurrPartition, pgeomData);
                BSDismemberSkinInstance::PartitionData* v14 = pDismemberPartitions;
                v10 = v22;
                if (pDismemberPartitions) {
                    uint32_t v15 = v22 + 1;
                    while (v15 < uiPartitionCount) {
                        if (v14[v10 + 1].bStateChange) {
                            break;
                        }
                        ++v10;
                        ++v15;
                        ++pkCurrPartition;
                        uint32_t v29 = v15;
                        if (v14[v10].bEnabled) {
                            NiGeometryBufferData* pkBuffData_3 =
                                static_cast<NiGeometryBufferData*>(pshader->PrepareGeometryForRendering(
                                    pgeometry, pkCurrPartition, pkCurrPartition->m_pkBuffData, ppropState
                                ));
                            if (!pkBuffData_3) {
                                return;
                            }
                            pRenderer->Do_DrawPartition(pkBuffData_3, pkCurrPartition, pgeomData);
                            v14 = pDismemberPartitions;
                            v15 = v29;
                        }
                    }
                }
            }
            v22 = ++v10;
        }
        while (v10 < uiPartitionCount);
    }
    pgeometry->m_spModelData.m_pObject->m_usDirtyFlags &= 0xF000u;
    pshader->PostGeometry(ppropState);
}

void BSBatchRenderer::BeginPass(uint32_t auiPass, BSShader* apShader) {
    BSBatchRenderer::EndPass();
    BSShader::sShadowAlphaTestRef = -1;
    apShader->InitPass(auiPass);
    NiD3DTextureStage::pShadowedTextures[0] = nullptr;
    NiD3DTextureStage::pShadowedTextures[1] = nullptr;
    NiD3DTextureStage::pShadowedTextures[2] = nullptr;
    NiD3DTextureStage::pShadowedTextures[3] = nullptr;
    NiD3DTextureStage::pShadowedTextures[4] = nullptr;
    NiD3DTextureStage::pShadowedTextures[5] = nullptr;
    NiD3DTextureStage::pShadowedTextures[6] = nullptr;
    NiD3DTextureStage::pShadowedTextures[7] = nullptr;
    NiD3DTextureStage::pShadowedTextures[8] = nullptr;
    NiD3DTextureStage::pShadowedTextures[9] = nullptr;
    NiD3DTextureStage::pShadowedTextures[10] = nullptr;
    NiD3DTextureStage::pShadowedTextures[11] = nullptr;
    NiD3DTextureStage::pShadowedTextures[12] = nullptr;
    NiD3DTextureStage::pShadowedTextures[13] = nullptr;
    NiD3DTextureStage::pShadowedTextures[14] = nullptr;
    NiD3DTextureStage::pShadowedTextures[15] = nullptr;
    switch (auiPass) {
        case 202u:
        case 209u: {
            static_cast<ShadowLightShader*>(apShader)->ShadowLightShader::SetupPass_ADT4_Opt();
            break;
        }
        case 252u:
        case 253u: {
            static_cast<ShadowLightShader*>(apShader)->ShadowLightShader::SetupPass_ADT_Opt();
            BSRenderState::SetZEnable(true, BSRS_NOLOCK);
            break;
        }
        case 258u: {
            static_cast<ShadowLightShader*>(apShader)->ShadowLightShader::SetupPass_ADT_Opt();
            break;
        }
        default: {
            apShader->SetupPassConstants(auiPass);
            apShader->SetupPassTextures(auiPass);
            apShader->SetupPassRenderStates(auiPass);
            break;
        }
    }
    apShader->SetupPassShaders(auiPass);
    BSBatchRenderer::uiLastPass = auiPass;
    BSBatchRenderer::pLastShader = apShader;
    BSBatchRenderer::bFirstPass = true;
}
