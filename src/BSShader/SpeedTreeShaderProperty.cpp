#include "BSShader/SpeedTreeShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

NiRTTI SpeedTreeShaderPPLightingProperty::ms_RTTI(
    "SpeedTreeShaderPPLightingProperty",
    &BSShaderPPLightingProperty::ms_RTTI
);
NiRTTI SpeedTreeShaderLightingProperty::ms_RTTI("SpeedTreeShaderLightingProperty", &BSShaderLightingProperty::ms_RTTI);

STSPData::STSPData() : pWindInfo(nullptr), sVertexCount(0) {}

STSPData::~STSPData() {
    if (pWindInfo && sVertexCount) {
        MemoryManager::Instance().Deallocate(pWindInfo);
    }
}

SpeedTreeShaderLightingProperty::SpeedTreeShaderLightingProperty(STSPData* apData)
  : spBaseTexture(nullptr), TextureClampMode(NiTexturingProperty::WRAP_S_WRAP_T), spData(apData) {}

SpeedTreeShaderLightingProperty::~SpeedTreeShaderLightingProperty() {
    spData = nullptr;
    spBaseTexture = nullptr;
}

void SpeedTreeShaderLightingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderLightingProperty::GetViewerStrings(apStrings);
}

BSShaderProperty* SpeedTreeShaderLightingProperty::ClarifyShader(NiGeometry*, bool, bool) {
    if (!spBaseTexture.m_pObject) {
        iShader = 0;
    }
    return nullptr;
}

// spBaseTexture's RTTI chain is walked against NiSourceTexture and, when that fails, NiRenderedTexture, and the results
// are dropped; the walks call the texture's GetRTTI virtual, so they are kept deliberately.
void SpeedTreeShaderLightingProperty::GetTextureUse(
    void (*apTextureUseCB)(NiTexture*, uint32_t, void*),
    void* apParam
) {
    NiTexture* pTexture = spBaseTexture.m_pObject;
    if (!pTexture) {
        return;
    }
    if (!pTexture->IsKindOf(&NiSourceTexture::ms_RTTI)) {
        NiObject::IsKindOf(&NiRenderedTexture::ms_RTTI, spBaseTexture.m_pObject);
    }
    apTextureUseCB(spBaseTexture.m_pObject, 0, apParam);
}

D3DXVECTOR4* SpeedTreeShaderLightingProperty::GetWindInfo() {
    return spData.m_pObject ? spData->pWindInfo : nullptr;
}

uint32_t SpeedTreeShaderLightingProperty::GetVertexCount() {
    return spData.m_pObject ? spData->sVertexCount : 0;
}

void SpeedTreeShaderLightingProperty::SetData(D3DXVECTOR4* apWindInfo, uint32_t aiWindInfoCount) {
    if (spData.m_pObject) {
        spData->pWindInfo = apWindInfo;
        spData->sVertexCount = static_cast<uint16_t>(aiWindInfoCount);
    }
}

void SpeedTreeShaderLightingProperty::SetBaseTexture(NiTexture* apBaseTexture) {
    spBaseTexture = apBaseTexture;
}

void SpeedTreeShaderLightingProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode = (aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T
                                                         : NiTexturingProperty::WRAP_S_WRAP_T;
}

SpeedTreeShaderPPLightingProperty::SpeedTreeShaderPPLightingProperty(STSPData* apData) : spData(apData) {}

SpeedTreeShaderPPLightingProperty::~SpeedTreeShaderPPLightingProperty() {
    spData = nullptr;
}

void SpeedTreeShaderPPLightingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderPPLightingProperty::GetViewerStrings(apStrings);
}

BSShaderProperty* SpeedTreeShaderPPLightingProperty::ClarifyShader(NiGeometry*, bool, bool) {
    if (!spTextures[0]->m_pObject || !spTextures[1]->m_pObject) {
        iShader = 0;
    }
    return nullptr;
}

D3DXVECTOR4* SpeedTreeShaderPPLightingProperty::GetWindInfo() {
    return spData.m_pObject ? spData->pWindInfo : nullptr;
}

uint16_t SpeedTreeShaderPPLightingProperty::GetVertexCount() {
    return spData.m_pObject ? spData->sVertexCount : 0;
}

void SpeedTreeShaderPPLightingProperty::SetData(D3DXVECTOR4* apWindInfo, uint32_t asVertexCount) {
    if (spData.m_pObject) {
        spData->pWindInfo = apWindInfo;
        spData->sVertexCount = static_cast<uint16_t>(asVertexCount);
    }
}
