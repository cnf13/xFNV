#include "BSShader/BSShaderData.h"

#include "BSCore/BSStringT.h"
#include "BSShader/BSMaterialEmittanceMultController.h"
#include "BSShader/BSRefractionFirePeriodController.h"
#include "BSShader/BSRefractionStrengthController.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/NoFaderShaderProperty.h"
#include "BSShader/SkyShaderProperty.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSShader/TileShaderProperty.h"
#include "BSShader/VolumetricFogShaderProperty.h"
#include "BSShader/WaterShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAlphaController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMaterialColorController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

// The default settings: standard lighting, black colours, opaque, depth-tested and -written, wrapped textures.
BSShaderData::BSShaderData() {
    FalloffData.r = 0.0f;
    FalloffData.g = 0.0f;
    FalloffData.b = 0.0f;
    FalloffData.a = 0.0f;
    eLightingShader = BSShaderData::LS_STANDARD;
    SpecularColor = NiColor_BLACK;
    EmittanceColor = NiColor_BLACK;
    fShininess = 10.0f;
    fAlpha = 1.0f;
    fEmittanceMult = 1.0f;
    pSpecularController = nullptr;
    pEmittanceController = nullptr;
    fRefractionPower = 0.0f;
    pAlphaController = nullptr;
    pEmittanceMultController = nullptr;
    fParallaxOcclusionHieghtScale = 1.0f;
    bAlphaBlend = false;
    eAlphaSrcBlendMode = NiAlphaProperty::ALPHA_SRCALPHA;
    fParallaxOcclusionMaxPasses = 4.0f;
    eAlphaDestBlendMode = NiAlphaProperty::ALPHA_INVSRCALPHA;
    bAlphaTest = false;
    eAlphaTestFunction = NiAlphaProperty::TEST_GREATER;
    ucAlphaTestRef = 0;
    bVertexAlpha = false;
    bModelSpaceNormals = false;
    bRemappableTextures = false;
    bDecal = false;
    bExternalEmittance = false;
    bHideSecret = false;
    bNoFade = false;
    bLightFade = false;
    bZBufferWrite = true;
    bZBufferTest = true;
    bRefraction = false;
    bRefractionFire = false;
    iRefractionFirePeriod = 0;
    pRefractionStrengthController = nullptr;
    pRefractionFirePeriodController = nullptr;
    bParallax = false;
    bParallaxOcclusion = false;
    VolumetricFogColor = NiColor_BLACK;
    fVolumetricFogDensity = 10.0f;
    fEnvironmentMappingMaskScale = 1.0f;
    bEnvironmentMapping = false;
    bEnvironmentMappingEye = false;
    bEnvironmentMappingWindow = false;
    pTextureSet = nullptr;
    for (int32_t i = 0; i < 6; ++i) {
        pbTextureHasAlpha[i] = false;
    }
    eClampMode = NiTexturingProperty::WRAP_S_WRAP_T;
}

void BSShaderData::Convert(NiAVObject* apTarget, uint32_t auiProperties) {
    if ((auiProperties & 1) != 0) {
        AddShaderProperty(apTarget);
    }
    if ((auiProperties & 2) != 0) {
        AddMaterialProperty(apTarget);
    }
    if ((auiProperties & 4) != 0) {
        AddAlphaProperty(apTarget);
    }
}

void BSShaderData::AddShaderProperty(NiAVObject* apTarget) {
    BSShaderProperty* pShaderProp = GetShaderProperty();
    if (pShaderProp) {
        CheckTextureAlphas();
        SetupShaderFlags(pShaderProp, apTarget);
        int32_t iShaderPropertyType = pShaderProp->iShaderPropertyType;
        BSShaderPPLightingProperty* pPPLightingProp =
            iShaderPropertyType >= BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_FIRST &&
                iShaderPropertyType <= BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_LAST
            ? static_cast<BSShaderPPLightingProperty*>(pShaderProp)
            : nullptr;
        if (pPPLightingProp) {
            SetupTexturesPPLightingShader(pPPLightingProp);
            SetupDataLightingShader(pPPLightingProp);
        }
        BSShaderNoLightingProperty* pNoLightingProp =
            pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_NOLIGHTING
            ? static_cast<BSShaderNoLightingProperty*>(pShaderProp)
            : nullptr;
        if (pNoLightingProp) {
            SetupTexturesNoLightingShader(pNoLightingProp);
        }
        VolumetricFogShaderProperty* pFogProp =
            pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_VOLUMETRIC_FOG
            ? static_cast<VolumetricFogShaderProperty*>(pShaderProp)
            : nullptr;
        if (pFogProp) {
            pFogProp->FogProps.r = VolumetricFogColor.r;
            pFogProp->FogProps.g = VolumetricFogColor.g;
            pFogProp->FogProps.b = VolumetricFogColor.b;
            pFogProp->FogProps.a = fVolumetricFogDensity;
        }
        TallGrassShaderProperty* pTallGrassProp =
            pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_TALLGRASS
            ? static_cast<TallGrassShaderProperty*>(pShaderProp)
            : nullptr;
        if (pTallGrassProp) {
            SetupTexturesTallGrassShader(pTallGrassProp);
        }
        SkyShaderProperty* pSkyProp = pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_SKY
            ? static_cast<SkyShaderProperty*>(pShaderProp)
            : nullptr;
        if (pSkyProp) {
            SetupTexturesSkyShader(pSkyProp);
            pSkyProp->soType = static_cast<uint32_t>(iSkyTYpe);
        }
        TileShaderProperty* pTileProp =
            pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_TILESHADER
            ? static_cast<TileShaderProperty*>(pShaderProp)
            : nullptr;
        if (pTileProp) {
            SetupTexturesTileShader(pTileProp);
        }
        BSShaderPPLightingProperty* pNoFaderProp =
            pShaderProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_NOFADER
            ? static_cast<BSShaderPPLightingProperty*>(pShaderProp)
            : nullptr;
        if (pNoFaderProp) {
            SetupTexturesPPLightingShader(pNoFaderProp);
            SetupDataLightingShader(pNoFaderProp);
        }
        pShaderProp->iShader = BSShaderData::uiShaderPropertyToShader[eLightingShader];
        apTarget->AttachProperty(pShaderProp);
    }
}

void BSShaderData::AddAlphaProperty(NiAVObject* apTarget) {
    if ((bAlphaBlend || bAlphaTest) && !bRefraction) {
        NiAlphaProperty* palphaprop_2;
        uint16_t v8;
        NiObjectNET* v3 = static_cast<NiObjectNET*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
        NiAlphaProperty* palphaprop = static_cast<NiAlphaProperty*>(v3);
        if (v3) {
            ::new (v3) NiAlphaProperty;
            palphaprop_2 = palphaprop;
        }
        else {
            palphaprop_2 = nullptr;
        }
        if (bAlphaBlend) {
            palphaprop_2->m_uFlags |= 1u;
        }
        else {
            palphaprop_2->m_uFlags &= ~1u;
        }
        uint16_t v6 = (palphaprop_2->m_uFlags & ~NiAlphaProperty::SRC_BLEND_MASK) |
            2 * static_cast<uint16_t>(static_cast<uint32_t>(eAlphaSrcBlendMode));
        palphaprop_2->m_uFlags = v6;
        uint16_t v7 = (v6 & ~NiAlphaProperty::DEST_BLEND_MASK) |
            32 * static_cast<uint16_t>(static_cast<uint32_t>(eAlphaDestBlendMode));
        palphaprop_2->m_uFlags = v7;
        if (bAlphaTest) {
            v8 = v7 | 512;
        }
        else {
            v8 = v7 & ~NiAlphaProperty::TEST_ENABLE_MASK;
        }
        palphaprop_2->m_uFlags = v8;
        palphaprop_2->m_ucAlphaTestRef = ucAlphaTestRef;
        palphaprop_2->m_uFlags = (palphaprop_2->m_uFlags & ~NiAlphaProperty::TEST_FUNC_MASK) |
            static_cast<uint16_t>(static_cast<uint32_t>(eAlphaTestFunction)) << 10;
        apTarget->AttachProperty(palphaprop_2);
    }
}

void BSShaderData::AddMaterialProperty(NiAVObject* apTarget) {
    NiMaterialProperty* v4;
    NiMaterialProperty* v3 = static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
    if (v3) {
        v4 = ::new (v3) NiMaterialProperty;
    }
    else {
        v4 = nullptr;
    }
    v4->m_spec.r = SpecularColor.r;
    v4->m_spec.g = SpecularColor.g;
    float b = SpecularColor.b;
    uint32_t v6 = ++v4->m_uiRevID;
    v4->m_spec.b = b;
    v4->m_emit.r = EmittanceColor.r;
    v4->m_emit.g = EmittanceColor.g;
    float v7 = EmittanceColor.b;
    v4->m_uiRevID = ++v6;
    v4->m_emit.b = v7;
    ++v6;
    v4->m_fShine = fShininess;
    v4->m_uiRevID = v6++;
    v4->m_fAlpha = fAlpha;
    v4->m_uiRevID = v6;
    v4->m_fEmitMult = fEmittanceMult;
    v4->m_uiRevID = v6 + 1;
    if (pSpecularController) {
        pSpecularController->SetTarget(v4);
    }
    if (pEmittanceController) {
        pEmittanceController->SetTarget(v4);
    }
    if (pAlphaController) {
        pAlphaController->SetTarget(v4);
    }
    if (pEmittanceMultController) {
        pEmittanceMultController->SetTarget(v4);
    }
    apTarget->AttachProperty(v4);
}

BSShaderProperty* BSShaderData::GetShaderProperty() {
    void* pMemory;
    switch (eLightingShader) {
        case LS_STANDARD:
        case LS_SKIN:
        case LS_HAIR:
            pMemory = NiMemObject::operator new(sizeof(BSShaderPPLightingProperty));
            return pMemory ? ::new (pMemory) BSShaderPPLightingProperty : nullptr;
        case LS_WATER:
            pMemory = NiMemObject::operator new(sizeof(WaterShaderProperty));
            return pMemory ? ::new (pMemory) WaterShaderProperty : nullptr;
        case LS_NOLIGHTING:
            pMemory = NiMemObject::operator new(sizeof(BSShaderNoLightingProperty));
            return pMemory ? ::new (pMemory) BSShaderNoLightingProperty : nullptr;
        case LS_VFOG:
            pMemory = NiMemObject::operator new(sizeof(VolumetricFogShaderProperty));
            return pMemory ? ::new (pMemory) VolumetricFogShaderProperty : nullptr;
        case LS_SKY:
            pMemory = NiMemObject::operator new(sizeof(SkyShaderProperty));
            return pMemory ? ::new (pMemory) SkyShaderProperty : nullptr;
        case LS_TALLGRASS:
            pMemory = NiMemObject::operator new(sizeof(TallGrassShaderProperty));
            return pMemory ? ::new (pMemory) TallGrassShaderProperty : nullptr;
        case LS_TILE:
            pMemory = NiMemObject::operator new(sizeof(TileShaderProperty));
            return pMemory ? ::new (pMemory) TileShaderProperty(false) : nullptr;
        case LS_NOFADER:
            pMemory = NiMemObject::operator new(sizeof(NoFaderShaderProperty));
            return pMemory ? ::new (pMemory) NoFaderShaderProperty : nullptr;
        default: return nullptr;
    }
}

void BSShaderData::SetupTexturesTileShader(TileShaderProperty* pTileShaderProp) {
    if (pTextureSet) {
        const char* pcFilename = pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE);
        // EXE-INLINED none: NiFixedString::NiFixedString(const char*)
        NiGlobalStringTable::GlobalStringHandle kFilename =
            pcFilename ? NiGlobalStringTable::AddString(pcFilename) : NiGlobalStringTable::NULL_STRING;
        pTileShaderProp->TextureFilename.Set(kFilename, 0);
        // EXE-INLINED none: NiFixedString::~NiFixedString (the refcount sits 8 bytes before the characters)
        if (kFilename != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<uint32_t*>(const_cast<char*>(kFilename - 8)));
        }
        pTileShaderProp->TexClampMode = eClampMode;
    }
}

void BSShaderData::SetupTexturesTallGrassShader(TallGrassShaderProperty* pTGShaderProp) {
    if (pTextureSet) {
        const char* pcFilename = pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE);
        // EXE-INLINED none: NiFixedString::NiFixedString(const char*)
        NiGlobalStringTable::GlobalStringHandle kFilename =
            pcFilename ? NiGlobalStringTable::AddString(pcFilename) : NiGlobalStringTable::NULL_STRING;
        pTGShaderProp->TextureFilename.Set(kFilename, 0);
        // EXE-INLINED none: NiFixedString::~NiFixedString (the refcount sits 8 bytes before the characters)
        if (kFilename != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<uint32_t*>(const_cast<char*>(kFilename - 8)));
        }
    }
}

void BSShaderData::SetupTexturesSkyShader(SkyShaderProperty* pSkyShaderProp) {
    if (pTextureSet) {
        const char* pcFilename = pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE);
        // EXE-INLINED none: NiFixedString::NiFixedString(const char*)
        NiGlobalStringTable::GlobalStringHandle kFilename =
            pcFilename ? NiGlobalStringTable::AddString(pcFilename) : NiGlobalStringTable::NULL_STRING;
        pSkyShaderProp->TextureFilename.Set(kFilename, 0);
        // EXE-INLINED none: NiFixedString::~NiFixedString (the refcount sits 8 bytes before the characters)
        if (kFilename != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<uint32_t*>(const_cast<char*>(kFilename - 8)));
        }
        pSkyShaderProp->SetClampNI(eClampMode);
    }
}

void BSShaderData::SetupTexturesPPLightingShader(BSShaderPPLightingProperty* apShaderProp) {
    BSShaderTextureSet* pTextureSet;
    BSShaderTextureSet* m_pObject;
    pTextureSet = this->pTextureSet;
    m_pObject = apShaderProp->spTextureSet.m_pObject;
    if (m_pObject != pTextureSet) {
        if ((m_pObject) && (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount))) {
            m_pObject->DeleteThis();
        }

        apShaderProp->spTextureSet.m_pObject = pTextureSet;
        if (pTextureSet) {
            InterlockedIncrementUInt32(&pTextureSet->m_uiRefCount);
        }
    }
    apShaderProp->SetClampNI(eClampMode);
}

void BSShaderData::SetupDataLightingShader(BSShaderPPLightingProperty* a2) {
    a2->SetRefractionStrength(fRefractionPower);
    a2->iRefractionFirePeriod = iRefractionFirePeriod;
    a2->fParallaxOccMaxPasses = fParallaxOcclusionMaxPasses;
    a2->fParallaxOccScale = fParallaxOcclusionHieghtScale;
    if (pRefractionStrengthController) {
        pRefractionStrengthController->SetTarget(a2);
    }
    if (pRefractionFirePeriodController) {
        pRefractionFirePeriodController->SetTarget(a2);
    }
}

void BSShaderData::CheckTextureAlphas() {
    if (this->pTextureSet) {
        for (BSShaderProperty::TextureTypeEnum i = BSShaderProperty::TEXTYPE_DIFFUSE;
             i < BSShaderProperty::TEXTYPE_SPECULAR; i = static_cast<BSShaderProperty::TextureTypeEnum>(i + 1)) {
            FNVD_INLINED_LOCAL(NiPointer<NiTexture>, spTexture, std::nullopt, std::nullopt);
            spTexture.m_pObject = nullptr;
            this->pTextureSet->GetTexture(i, spTexture);
            NiTexture* pTexture = spTexture.m_pObject;
            if (pTexture) {
                NiPixelFormat* pPixelFormat;
                NiTexture::RendererData* pRendererData = pTexture->m_pkRendererData;
                if (pRendererData) {
                    pPixelFormat = &pRendererData->m_kPixelFormat;
                }
                else {
                    pPixelFormat = nullptr;
                }
                NiPixelFormat::Format eFormat = pPixelFormat->m_eFormat;
                if (eFormat == NiPixelFormat::FORMAT_DXT3 || eFormat == NiPixelFormat::FORMAT_DXT5) {
                    pbTextureHasAlpha[i] = true;
                }
            }
            if (pTexture) {
                NiDecRefCount(pTexture);
            }
        }
    }
}

void BSShaderData::SetupShaderFlags(BSShaderProperty* apShaderProp, NiAVObject* apObject) {
    BSShaderPPLightingProperty* v4;
    if (apObject->IsGeometry() && static_cast<NiGeometry*>(apObject)->m_spSkinInstance) {
        v4 = static_cast<BSShaderPPLightingProperty*>(apShaderProp);
        static_cast<uint8_t>(apShaderProp->ulFlags[0]);
        if ((apShaderProp->ulFlags[0] & 2) == 0) {
            apShaderProp->iLastRenderPassState = 0;
        }
        apShaderProp->ulFlags[0] |= 2u;
    }
    else {
        v4 = static_cast<BSShaderPPLightingProperty*>(apShaderProp);
        static_cast<uint8_t>(apShaderProp->ulFlags[0]);
        if ((apShaderProp->ulFlags[0] & 2) != 0) {
            apShaderProp->iLastRenderPassState = 0;
        }
        apShaderProp->ulFlags[0] &= ~2u;
    }
    if (bVertexAlpha) {
        if ((v4->ulFlags[0] & 8) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 8u;
    }
    else {
        if ((v4->ulFlags[0] & 8) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~8u;
    }
    if (bEnvironmentMapping) {
        if (static_cast<int8_t>(v4->ulFlags[0]) >= 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 128u;
    }
    else {
        if (static_cast<int8_t>(v4->ulFlags[0]) < 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~128u;
    }
    if (pAlphaController) {
        if ((v4->ulFlags[0] & BSS1_Dynamic_Alpha) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Dynamic_Alpha;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Dynamic_Alpha) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Dynamic_Alpha;
    }
    if (pbTextureHasAlpha[0u]) {
        if ((v4->ulFlags[0] & 256) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 256u;
    }
    else {
        if ((v4->ulFlags[0] & 256) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~256u;
    }
    if (eLightingShader == BSShaderData::LS_SKIN) {
        if ((v4->ulFlags[0] & 1024) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 1024u;
    }
    else {
        if ((v4->ulFlags[0] & 1024) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~1024u;
    }
    if (bParallax) {
        if ((v4->ulFlags[0] & 2048) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 2048u;
    }
    else {
        if ((v4->ulFlags[0] & 2048) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~2048u;
    }
    if (bParallaxOcclusion) {
        if ((v4->ulFlags[0] & BSS1_Parallax_Occulsion) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Parallax_Occulsion;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Parallax_Occulsion) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Parallax_Occulsion;
    }
    if (bModelSpaceNormals) {
        if ((v4->ulFlags[0] & BSS1_Model_Space_Normals) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Model_Space_Normals;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Model_Space_Normals) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Model_Space_Normals;
    }
    if (bRefraction) {
        if ((v4->ulFlags[0] & BSS1_Refraction) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Refraction;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Refraction) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Refraction;
    }
    if (bRefractionFire) {
        if ((v4->ulFlags[0] & BSS1_Fire_Refraction) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Fire_Refraction;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Fire_Refraction) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Fire_Refraction;
    }
    if (eLightingShader == BSShaderData::LS_HAIR) {
        if ((v4->ulFlags[0] & BSS1_Hair) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Hair;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Hair) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Hair;
    }
    if (bRemappableTextures) {
        if ((v4->ulFlags[0] & BSS1_Remappable_Textures) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Remappable_Textures;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Remappable_Textures) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Remappable_Textures;
    }
    if (bDecal) {
        if ((v4->ulFlags[0] & BSS1_Decal_Single_Pass) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Decal_Single_Pass;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Decal_Single_Pass) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Decal_Single_Pass;
    }
    if (bDecal) {
        if ((v4->ulFlags[0] & BSS1_Dynamic_Decal_Single_Pass) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Dynamic_Decal_Single_Pass;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Dynamic_Decal_Single_Pass) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Dynamic_Decal_Single_Pass;
    }
    if (bZBufferTest) {
        if ((v4->ulFlags[0] & BSS1_ZBuffer_Test) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_ZBuffer_Test;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_ZBuffer_Test) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_ZBuffer_Test;
    }
    if (bZBufferWrite) {
        if ((v4->ulFlags[1] & 1) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] |= 1u;
    }
    else {
        if ((v4->ulFlags[1] & 1) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] &= ~1u;
    }
    if (bHideSecret) {
        if ((v4->ulFlags[0] & BSS1_Localmap_Hide_Secret) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Localmap_Hide_Secret;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Localmap_Hide_Secret) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Localmap_Hide_Secret;
    }
    if (bNoFade) {
        if ((v4->ulFlags[1] & 8) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] |= 8u;
    }
    else {
        if ((v4->ulFlags[1] & 8) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] &= ~8u;
    }
    if (bLightFade) {
        if ((v4->ulFlags[1] & BSS2_Envmap_Light_Fade) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] |= BSS2_Envmap_Light_Fade;
    }
    else {
        if ((v4->ulFlags[1] & BSS2_Envmap_Light_Fade) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[1] &= ~BSS2_Envmap_Light_Fade;
    }
    if (bFalloffEnabled) {
        if ((v4->ulFlags[0] & 64) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 64u;
    }
    else {
        if ((v4->ulFlags[0] & 64) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~64u;
    }
    if (pbTextureHasAlpha[1]) {
        if ((v4->ulFlags[0] & 1) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= 1u;
    }
    else {
        if ((v4->ulFlags[0] & 1) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~1u;
    }
    if (bEnvironmentMappingEye) {
        if ((v4->ulFlags[0] & BSS1_Eye_Environment_Mapping) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Eye_Environment_Mapping;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Eye_Environment_Mapping) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Eye_Environment_Mapping;
    }
    if (bEnvironmentMappingWindow) {
        if ((v4->ulFlags[0] & BSS1_Window_Environment_Mapping) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_Window_Environment_Mapping;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_Window_Environment_Mapping) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_Window_Environment_Mapping;
    }
    if (bExternalEmittance) {
        if ((v4->ulFlags[0] & BSS1_External_Emittance) == 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] |= BSS1_External_Emittance;
        v4->fEnvmapScale = fEnvironmentMappingMaskScale;
    }
    else {
        if ((v4->ulFlags[0] & BSS1_External_Emittance) != 0) {
            v4->iLastRenderPassState = 0;
        }
        v4->ulFlags[0] &= ~BSS1_External_Emittance;
        v4->fEnvmapScale = fEnvironmentMappingMaskScale;
    }
}

void BSShaderData::SetupTexturesNoLightingShader(BSShaderNoLightingProperty* pNLShaderProp) {
    if (pTextureSet) {
        const char* v4;
        const char* v3 = pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE);
        if (v3) {
            v4 = NiGlobalStringTable::AddString(v3);
        }
        else {
            v4 = NiGlobalStringTable::NULL_STRING;
        }
        pNLShaderProp->TextureFilename.Set(v4, 0u);
        NiGlobalStringTable::ReleaseHandle(v4);
        pNLShaderProp->SetClampNI(eClampMode);
    }
    if (-1.0f != pNLShaderProp->FalloffData.fFalloffStartAngle) {
        pNLShaderProp->FalloffData.fFalloffStartAngle = FalloffData.r;
    }
    if (-1.0f != pNLShaderProp->FalloffData.fFalloffStopAngle) {
        pNLShaderProp->FalloffData.fFalloffStopAngle = FalloffData.g;
    }
    if (-1.0f != pNLShaderProp->FalloffData.fFalloffStartOpacity) {
        pNLShaderProp->FalloffData.fFalloffStartOpacity = FalloffData.b;
    }
    if (-1.0f != pNLShaderProp->FalloffData.fFalloffStopOpacity) {
        pNLShaderProp->FalloffData.fFalloffStopOpacity = FalloffData.a;
    }
}
