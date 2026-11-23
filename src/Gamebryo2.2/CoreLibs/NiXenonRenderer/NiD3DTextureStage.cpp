#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "SDK/MSVC/string.h"

// The stage starts with no texture: passes such as ShadowLightShader's BSSM_AD3 never set some of their stages'
// textures, which ApplyTextureStages applies when not null, so an uninitialised texture would be applied.
NiD3DTextureStage::NiD3DTextureStage()
  : m_uiStage(0), m_spTexture(nullptr), m_uiTextureFlags(0), m_uiTextureTransformFlags(0), m_pkGlobalEntry(nullptr),
    m_usObjTextureFlags(0), m_bTextureTransform(false), m_bRendererOwned(false), m_uiRefCount(0), bApply(true) {
    m_pkTextureStageGroup = NiD3DTextureStageGroup::GetFreeTextureStageGroup();
}

// Releases the global constant the stage uses, then gives its stage group back.
NiD3DTextureStage::~NiD3DTextureStage() {
    if (m_pkGlobalEntry) {
        NiShaderFactory::ReleaseGlobalShaderConstant(m_pkGlobalEntry->m_kKey);
    }
    NiD3DTextureStageGroup::ReleaseTextureStageGroup(m_pkTextureStageGroup);
}

// Sets the stage's texture coordinate index, texture and sampler states. Also caps the anisotropy here, rather than
// leaving it to ModifyFilterMode. The mip filter comes from the texture manager's S0 mipmap flag. The three flags
// PrepareTextureForRendering fills are read only on the path where it returned a texture.
void NiD3DTextureStage::ApplyTexture() {
    if (m_uiStage < NiD3DPass::ms_uiMaxTextureBlendStages) {
        uint32_t uiValue = 0;
        bool bSave = false;
        if (!m_pkTextureStageGroup->GetStageState(D3DTSS_TEXCOORDINDEX, uiValue, bSave)) {
            uiValue = m_uiStage;
        }
        ms_pkD3DRenderState->SetTextureStageState(m_uiStage, D3DTSS_TEXCOORDINDEX, uiValue, false);
    }
    bool bS0Mipmap = false;
    bool bNonPow2 = false;
    IDirect3DBaseTexture9* pkD3DTexture = nullptr;
    if (m_spTexture) {
        bool bChanged;
        pkD3DTexture = ms_pkD3DRenderer->m_pkTextureManager
                           ->PrepareTextureForRendering(m_spTexture, bChanged, bS0Mipmap, bNonPow2);
    }
    ms_pkD3DRenderState->SetTexture(m_uiStage, pkD3DTexture);
    if (pkD3DTexture) {
        if (!bS0Mipmap) {
            ms_pkD3DRenderState->SetSamplerState(m_uiStage, D3DSAMP_MIPFILTER, 0, false);
        }
        ModifyClampMode(bNonPow2);
        ApplyTextureTransform();
    }
    if (m_eFilter == NiTexturingProperty::FILTER_ANISO) {
        ms_pkD3DDevice
            ->SetSamplerState(m_uiStage, D3DSAMP_MAXANISOTROPY, static_cast<uint32_t>(NiRenderer::iMaxAnisotropy));
    }
}

void NiD3DTextureStage::ApplyTextureTransform() {
    if (m_uiStage >= NiD3DPass::ms_uiMaxTextureBlendStages) {
        return;
    }
    if (!m_bTextureTransform) {
        NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(m_uiStage, 24u, 0, false);
        return;
    }
    switch (m_uiTextureTransformFlags & NiD3DTextureStage::TSTTF_SOURCE_MASK) {
        case NiD3DTextureStage::TSTTF_IGNORE: {
            NiD3DTextureStage::ms_pkD3DDevice->SetTransform(m_uiStage + 16, &m_kTextureTransformation);
            break;
        }
        case NiD3DTextureStage::TSTTF_GLOBAL: {
            if ((m_uiTextureTransformFlags & ~NiD3DTextureStage::TSTTF_SOURCE_MASK) == 0) {
                NiD3DTextureStage::ms_pkD3DDevice->SetTransform(
                    m_uiStage + 16,
                    static_cast<_D3DMATRIX*>(m_pkGlobalEntry->m_pvDataSource)
                );
            }
            else {
                int32_t iCalculationMethod = m_uiTextureTransformFlags & ~NiD3DTextureStage::TSTTF_SOURCE_MASK;
                _D3DMATRIX* pkD3DMatSource = static_cast<_D3DMATRIX*>(m_pkGlobalEntry->m_pvDataSource);
                if (NiD3DTextureStage::CalculateTransformationMatrix(iCalculationMethod, pkD3DMatSource)) {
                    NiD3DTextureStage::ms_pkD3DDevice->SetTransform(
                        m_uiStage + 16,
                        &NiD3DTextureStage::ms_kCalculatedTextureTransformation
                    );
                }
            }
            break;
        }
        case NiD3DTextureStage::TSTTF_CONSTANT: {
            if ((m_uiTextureTransformFlags & ~NiD3DTextureStage::TSTTF_SOURCE_MASK) == 0) {
                NiD3DTextureStage::ms_pkD3DDevice->SetTransform(m_uiStage + 16, &m_kTextureTransformation);
            }
            else {
                int32_t iCalculationMethod = m_uiTextureTransformFlags & ~NiD3DTextureStage::TSTTF_SOURCE_MASK;
                _D3DMATRIX* pkD3DMatSource = &m_kTextureTransformation;
                if (NiD3DTextureStage::CalculateTransformationMatrix(iCalculationMethod, pkD3DMatSource)) {
                    NiD3DTextureStage::ms_pkD3DDevice->SetTransform(
                        m_uiStage + 16,
                        &NiD3DTextureStage::ms_kCalculatedTextureTransformation
                    );
                }
            }
            break;
        }
    }
}

void NiD3DTextureStage::ModifyClampMode(bool bNonPow2) {
    if (bNonPow2 &&
        (NiD3DTextureStage::ms_pkD3DRenderer->GetFlags() & 8) == 0 &&
        (NiD3DTextureStage::ms_pkD3DRenderer->GetFlags() & 4) != 0) {
        NiD3DTextureStage::ms_pkD3DRenderState->SetSamplerState(m_uiStage, 1u, 3u, false);
        NiD3DTextureStage::ms_pkD3DRenderState->SetSamplerState(m_uiStage, 2u, 3u, false);
        uint32_t v3 = 198;
        int32_t v4 = 8;
        do {
            NiD3DTextureStage::ms_pkD3DRenderState->SetRenderState(v3 - 70, 0, 0, false);
            NiD3DTextureStage::ms_pkD3DRenderState->SetRenderState(v3++, 0, 0, false);
            --v4;
        }
        while (v4);
    }
}

bool NiD3DTextureStage::ConfigureStage(bool abForceUpdateSamplerStates) {
    if (!abForceUpdateSamplerStates && m_spTexture == pShadowedTextures[m_uiStage]) {
        return true;
    }
    pShadowedTextures[m_uiStage] = m_spTexture;
    if (m_pkTextureStageGroup->SetTextureStage(m_uiStage)) {
        return false;
    }
    ApplyTexture();
    return true;
}

void NiD3DTextureStage::SetFilterMode(NiTexturingProperty::FilterMode eFilter) {
    m_pkTextureStageGroup
        ->SetSamplerState(D3DSAMP_MAGFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMag, false, false);
    m_pkTextureStageGroup
        ->SetSamplerState(D3DSAMP_MINFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMin, false, false);
    m_pkTextureStageGroup
        ->SetSamplerState(D3DSAMP_MIPFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMip, false, false);
    m_eFilter = eFilter;
}

void NiD3DTextureStage::SetClampMode(NiTexturingProperty::ClampMode eClamp) {
    m_pkTextureStageGroup
        ->SetSamplerState(D3DSAMP_ADDRESSU, NiDX9Renderer::ms_akD3DAddressMapping[eClamp].m_eU, false, false);
    m_pkTextureStageGroup
        ->SetSamplerState(D3DSAMP_ADDRESSV, NiDX9Renderer::ms_akD3DAddressMapping[eClamp].m_eV, false, false);
}

uint32_t NiD3DTextureStage::GetTexCoordIndex() {
    uint32_t uiValue = 0;
    bool bSave = false;
    if (!m_pkTextureStageGroup->GetStageState(D3DTSS_TEXCOORDINDEX, uiValue, bSave)) {
        return m_uiStage;
    }
    return uiValue;
}

bool NiD3DTextureStage::GetSaveTexCoordIndex() {
    NiD3DTextureStageGroup* m_pkTextureStageGroup = this->m_pkTextureStageGroup;
    uint32_t uiValue = 0;
    bool bSave = false;
    bool StageState = m_pkTextureStageGroup->GetStageState(11u, uiValue, bSave);
    return StageState && bSave;
}

// A renderer-owned stage is cleared (texture, stage and sampler states, global constant, flags) and goes back to the
// pool; any other is deleted.
void NiD3DTextureStage::ReturnStageToPool() {
    if (m_bRendererOwned) {
        m_spTexture = nullptr;
        m_pkTextureStageGroup->RemoveAllStageStates();
        m_pkTextureStageGroup->RemoveAllSamplerStates();
        NiGlobalConstantEntry* pkGlobalEntry = m_pkGlobalEntry;
        if (pkGlobalEntry) {
            NiShaderFactory::ReleaseGlobalShaderConstant(NiFixedString(pkGlobalEntry->m_kKey));
        }
        m_pkGlobalEntry = nullptr;
        m_uiStage = 0;
        m_uiTextureFlags = 0;
        m_usObjTextureFlags = 0;
        m_uiTextureTransformFlags = 0;
        m_bTextureTransform = false;
        ms_pkTextureStagePool->ReleaseObject(this);
    }
    else {
        delete this;
    }
}

void NiD3DTextureStage::CreateNewStage(NiPointer<NiD3DTextureStage>& aspD3DTextureStage) {
    bool v7;
    uint32_t* p_m_uiUsed = &NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed;
    NiTObjectPool<NiD3DTextureStage>* v2 = NiD3DTextureStage::ms_pkTextureStagePool;
    if (!NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed) {
        uint32_t* p_m_uiCurrentSize = &NiD3DTextureStage::ms_pkTextureStagePool->m_uiCurrentSize;
        NiD3DTextureStage::ms_pkTextureStagePool->CreateNewObjects(
            NiD3DTextureStage::ms_pkTextureStagePool->m_uiCurrentSize
        );
        *p_m_uiCurrentSize *= 2;
    }
    NiD3DTextureStage** m_pBase = v2->m_kFreeObjects.m_pBase;
    NiD3DTextureStage* v5 = *v2->m_kFreeObjects.m_pBase;
    *m_pBase = m_pBase[--*p_m_uiUsed];
    if (v5) {
        NiIncRefCount(v5);
    }
    v5->m_bRendererOwned = true;
    NiD3DTextureStage* m_pObject = aspD3DTextureStage.m_pObject;
    if (aspD3DTextureStage.m_pObject != v5) {
        if (m_pObject) {
            v7 = m_pObject->m_uiRefCount-- == 1;
            if (v7) {
                m_pObject->ReturnStageToPool();
            }
        }
        aspD3DTextureStage.m_pObject = v5;
        NiIncRefCount(v5);
    }
    v7 = v5->m_uiRefCount-- == 1;
    if (v7) {
        v5->ReturnStageToPool();
    }
}

void NiD3DTextureStage::DisableStage(uint32_t uiStage) {
    uint32_t i;
    for (i = uiStage; i < NiD3DPass::ms_uiMaxTextureBlendStages; ++i) {
        NiD3DTextureStage::ms_pkD3DRenderState->SetTexture(i, nullptr);
        NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 1u, 1u, false);
        NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 4u, 1u, false);
        NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 24u, 0, false);
    }
    for (; i < NiD3DPass::ms_uiMaxSamplers; ++i) {
        NiD3DTextureStage::ms_pkD3DRenderState->SetTexture(i, nullptr);
    }
}

// NiNew NiTObjectPool<NiD3DTextureStage>: an empty free set, no containers and an initial size of 8.
void NiD3DTextureStage::InitializePools() {
    NiTObjectPool<NiD3DTextureStage>* pkPool = static_cast<NiTObjectPool<NiD3DTextureStage>*>(
        MemoryManager::Instance().Allocate(sizeof(NiTObjectPool<NiD3DTextureStage>))
    );
    if (pkPool) {
        pkPool->m_kFreeObjects.m_pBase = nullptr;
        pkPool->m_kFreeObjects.m_uiAlloced = 0;
        pkPool->m_kFreeObjects.m_uiUsed = 0;
        pkPool->m_pkContainers = nullptr;
        pkPool->m_uiCurrentSize = 8;
        pkPool->m_uiInitialSize = 8;
    }
    ms_pkTextureStagePool = pkPool;
}

void NiD3DTextureStage::ShutdownPools() {
    NiTObjectPool<NiD3DTextureStage>* v0 = NiD3DTextureStage::ms_pkTextureStagePool;
    if (NiD3DTextureStage::ms_pkTextureStagePool) {
        NiTPoolContainer<NiD3DTextureStage, NiTNewInterface<NiD3DTextureStage>>*
            m_pkContainers = NiD3DTextureStage::ms_pkTextureStagePool->m_pkContainers;
        NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed = 0;
        if (m_pkContainers) {
            if (m_pkContainers->m_pkObjectArray) {
                delete[] m_pkContainers->m_pkObjectArray;
            }
            NiTPoolContainer<NiD3DTextureStage, NiTNewInterface<NiD3DTextureStage>>* m_pkNext = m_pkContainers
                                                                                                    ->m_pkNext;
            if (m_pkNext) {
                delete m_pkNext;
            }
            MemoryManager::Instance().Deallocate(m_pkContainers);
        }
        _NiFree(v0->m_kFreeObjects.m_pBase);
        MemoryManager::Instance().Deallocate(v0);
    }
    NiD3DTextureStage::ms_pkTextureStagePool = nullptr;
}

// Holds the renderer and a reference on its device, and caches the renderer's render state; null drops both.
void NiD3DTextureStage::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    ms_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        IDirect3DDevice9* pkDevice = pkD3DRenderer->m_pkD3DDevice9;
        if (ms_pkD3DDevice) {
            ms_pkD3DDevice->Release();
        }
        ms_pkD3DDevice = pkDevice;
        if (pkDevice) {
            pkDevice->AddRef();
        }
        ms_pkD3DRenderState = ms_pkD3DRenderer->m_pkRenderState;
    }
    else {
        if (ms_pkD3DDevice) {
            ms_pkD3DDevice->Release();
        }
        ms_pkD3DDevice = nullptr;
        ms_pkD3DRenderState = nullptr;
    }
}

char NiD3DTextureStage::CalculateTransformationMatrix(int32_t uiCalculationMethod, _D3DMATRIX* pkD3DMatSource) {
    switch (uiCalculationMethod) {
        case 1: {
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._21 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._12 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_22 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._22 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._32 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._13 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 = pkD3DMatSource->_13 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._33 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._41 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * pkD3DMatSource->_31 + pkD3DMatSource->_41;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._42 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * pkD3DMatSource->_22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * pkD3DMatSource->_32 + pkD3DMatSource->_42;
            double v4 = pkD3DMatSource->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41;
            double v5 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * pkD3DMatSource->_23;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = v4 + v5 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * pkD3DMatSource->_33 + pkD3DMatSource->_43;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
            return 1;
        }
        case 2: {
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._21 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 = pkD3DMatSource->_22 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_12 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._22 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._32 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._13 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_13 +
                pkD3DMatSource->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 = pkD3DMatSource->_13 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._33 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * pkD3DMatSource->_31 + pkD3DMatSource->_41;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._42 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 + pkD3DMatSource->_42;
            double v4 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * pkD3DMatSource->_23;
            double v5 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * pkD3DMatSource->_13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = v4 + v5 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * pkD3DMatSource->_33 + pkD3DMatSource->_43;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
            return 1;
        }
        case 3: {
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._31 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._12 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._22 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._32 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._13 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._23 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_33;
            double v10 = pkD3DMatSource->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31;
            double v11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_23;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 = v10 + v11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = pkD3DMatSource->_41;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = pkD3DMatSource->_42;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = pkD3DMatSource->_43;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
            return 1;
        }
        case 4: {
            std::memset(
                &NiD3DTextureStage::ms_kCalculatedTextureTransformation,
                0,
                sizeof(NiD3DTextureStage::ms_kCalculatedTextureTransformation)
            );
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 = 0.5f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = 0.5f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = 0.5f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = 0.5f;
            return 1;
        }
        case 5: {
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._12 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_22 +
                pkD3DMatSource->_12 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 = pkD3DMatSource->_22 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_12 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 = pkD3DMatSource->_22 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 = pkD3DMatSource->_13 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._23 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_13 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_33;
            double v10 = pkD3DMatSource->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32;
            double v11 = pkD3DMatSource->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 = v10 + v11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = pkD3DMatSource->_41;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = pkD3DMatSource->_42;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = pkD3DMatSource->_43;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
            return 1;
        }
        case 6: {
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._21 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_21 +
                pkD3DMatSource->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = pkD3DMatSource->_21 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_31;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 = pkD3DMatSource->_22 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * pkD3DMatSource->_12 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._22 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_22 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * pkD3DMatSource->_12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_32;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation
                ._32 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_12 +
                pkD3DMatSource->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
                pkD3DMatSource->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 = pkD3DMatSource->_13 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
                pkD3DMatSource->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 = pkD3DMatSource->_13 *
                    NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * pkD3DMatSource->_23 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * pkD3DMatSource->_33;
            double v10 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * pkD3DMatSource->_23;
            double v11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * pkD3DMatSource->_13;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 = v10 + v11 +
                NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * pkD3DMatSource->_33;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = pkD3DMatSource->_41;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = pkD3DMatSource->_42;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = pkD3DMatSource->_43;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
            NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
            return 1;
        }
        default: {
            return 0;
        }
    }
}
