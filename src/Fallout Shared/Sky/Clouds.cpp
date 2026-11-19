#include "Fallout Shared/Sky/Clouds.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShader.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWeather.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their values, in initialization order.
SettingT<GameSettingCollection> fWeatherCloudSpeedMax("fWeatherCloudSpeedMax", 0.1f);

Clouds::Clouds() : usNumLayers(0) {
    ZeroGeo();
    ClearTransTextures();
    bForceUpdate = false;
}

Clouds::~Clouds() {
    ZeroGeo();
    ClearTransTextures();
}

void Clouds::Initialize(NiNode* apSkyRoot) {
    SkyObject::Initialize(apSkyRoot);
    BSStream apMemory_28;
    if (apMemory_28.BSStream::Load("Meshes\\Sky\\Clouds.nif", nullptr)) {
        Model v6("Meshes\\Sky\\Clouds.nif", &apMemory_28, false, false);
        NiNode* apMemory_20 = v6.GetModel();
        apSkyRoot->AttachChild(spRoot.m_pObject, 1);
        int32_t apMemory_16 = 0;
        int32_t apMemory_24 = apMemory_20->GetChildCount();
        for (int32_t i = 0; i < apMemory_24; ++i) {
            NiAVObject* v3 = apMemory_20->GetAt(static_cast<uint32_t>(i));
            if (v3->IsGeometry()) {
                spClouds[i] = static_cast<NiGeometry*>(apMemory_20->GetAt(static_cast<uint32_t>(i)));
                NiNode* v2 = spRoot.m_pObject;
                v2->AttachChild(spClouds[i].m_pObject, 1);
                ++apMemory_16;
            }
        }
        SetNumLayers(apMemory_16);
    }
}

void Clouds::Update(Sky* apSky, float afSecondsPassed) {
    SkyObject::Update(apSky, afSecondsPassed);
    TESWeather* pCurrentWeather = apSky->pCurrentWeather;
    TESWeather* pLastWeather = apSky->pLastWeather;
    if (pCurrentWeather) {
        for (int32_t i = 0; i < GetNumLayers(); ++i) {
            if (!spClouds[i].m_pObject) {
                return;
            }
        }
        if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
            for (int32_t i_2 = 0;; ++i_2) {
                SkyShaderProperty* Shade;
                int32_t v25;
                int32_t NumLayers = GetNumLayers();
                if (i_2 >= NumLayers) {
                    break;
                }
                if ((&spClouds[i_2].m_pObject->m_PropertyState)->GetShade()) {
                    v25 = (&spClouds[i_2].m_pObject->m_PropertyState)->GetShade()->iShaderPropertyType == 13;
                }
                else {
                    v25 = 0;
                }
                if (v25) {
                    Shade = static_cast<SkyShaderProperty*>((&spClouds[i_2].m_pObject->m_PropertyState)->GetShade());
                }
                else {
                    Shade = nullptr;
                }
                SkyShaderProperty* pshadeProperty = Shade;
                if (Shade) {
                    NiColor kcolor = GetColor(i_2);
                    NiColorA apMemory(kcolor.r, kcolor.g, kcolor.b, 1.0f);
                    NiColorA* v4 = &apMemory;
                    pshadeProperty->VertexColor = *v4;
                }
                float* v5 = &fWeatherCloudSpeedMax.Float();
                float fCloudSpeed = pCurrentWeather->GetCloudLayerSpeed(i_2, *v5, 0.0f);
                if (pLastWeather) {
                    double WeatherPercent = apSky->GetWeatherPercent();
                    fCloudSpeed = WeatherPercent * fCloudSpeed;
                    double v23 = 1.0 - apSky->GetWeatherPercent();
                    float* v7 = &fWeatherCloudSpeedMax.Float();
                    double CloudLayerSpeed = pLastWeather->GetCloudLayerSpeed(i_2, *v7, 0.0f);
                    fCloudSpeed = CloudLayerSpeed * v23 + fCloudSpeed;
                }
                double DefaultWaterHeight = apSky->fWindSpeed;
                float fdelta = DefaultWaterHeight * afSecondsPassed * fCloudSpeed;
                SkyShader::SetCloudTexCoordOffset(i_2, fdelta);
            }
        }
        if (apSky->GetWeatherUpdateFlag() || bForceUpdate) {
            bForceUpdate = false;
            if (pLastWeather) {
                for (int32_t i_3 = 0;; ++i_3) {
                    int32_t v10 = GetNumLayers();
                    if (i_3 >= v10) {
                        break;
                    }
                    BSStringT<char> strfilename;
                    SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spClouds[i_3].m_pObject->GetProperty(3));
                    const TESTexture* ptestexture = pCurrentWeather->GetCloudTexture(i_3);
                    if (ptestexture && ptestexture->GetTextureNameLength()) {
                        const char* v11 = ptestexture->GetDefaultPath();
                        strfilename = v11;
                        const char* Model_0 = ptestexture->GetTextureName();
                        strfilename += Model_0;
                        char* fWeatherPercent = strfilename.pString;
                        SkyTextureManager::LoadTextureInBackground(&spTransTexture[i_3], fWeatherPercent);
                    }
                    else {
                        spTransTexture[i_3] = static_cast<NiTexture*>(nullptr);
                    }
                }
            }
            else {
                if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
                    for (int32_t i_4 = 0;; ++i_4) {
                        int32_t v13 = GetNumLayers();
                        if (i_4 >= v13) {
                            break;
                        }
                        BSStringT<char> strfilename_2;
                        SkyShaderProperty* pprop_2 =
                            static_cast<SkyShaderProperty*>(spClouds[i_4].m_pObject->GetProperty(3));
                        const TESTexture* ptestexture_2 = pCurrentWeather->GetCloudTexture(i_4);
                        if ((ptestexture_2) && (ptestexture_2->GetTextureNameLength())) {
                            const char* v14 = ptestexture_2->GetDefaultPath();
                            strfilename_2 = v14;
                            const char* v15 = ptestexture_2->GetTextureName();
                            strfilename_2 += v15;
                            NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
                            BSShaderManager::GetTexture(strfilename_2.pString, true, spTex, true, false);
                            NiTexture* ptex = spTex.m_pObject;
                            if (ptex) {
                                pprop_2->SetTexture(static_cast<NiSourceTexture*>(ptex));
                                pprop_2->SetTextureFile(strfilename_2.pString);
                                pprop_2->SetBlendValue(0.0f);
                            }
                        }
                    }
                }
            }
        }
        for (int32_t i_5 = 0;; ++i_5) {
            int32_t v16 = GetNumLayers();
            if (i_5 >= v16) {
                break;
            }
            NiProperty* pprop_3 = spClouds[i_5].m_pObject->GetProperty(3);
            SkyShaderProperty* pskyprop =
                static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pprop_3));
            if (spTransTexture[i_5].m_pObject) {
                if (pskyprop) {
                    NiTexture* fWeatherPercenta = spTransTexture[i_5].m_pObject;
                    pskyprop->SetBlendTexture(fWeatherPercenta);
                    float fWeatherPercentb = apSky->GetWeatherPercent();
                    pskyprop->SetBlendValue(fWeatherPercentb);
                }
                if (!pLastWeather) {
                    NiTexture* fWeatherPercentc = spTransTexture[i_5].m_pObject;
                    pskyprop->SetTexture(static_cast<NiSourceTexture*>(fWeatherPercentc));
                    spTransTexture[i_5] = static_cast<NiTexture*>(nullptr);
                    pskyprop->SetBlendTexture(nullptr);
                    pskyprop->SetBlendValue(0.0f);
                }
            }
            bool v21 = !pskyprop || !pskyprop->GetBaseTexture();
            spClouds[i_5].m_pObject->SetAppCulled(v21);
        }
    }
}

int32_t Clouds::GetNumLayers() const {
    return usNumLayers;
}

void Clouds::SetNumLayers(int32_t aiNumLayers) {
    if (aiNumLayers > MAX_LAYERS) {
        aiNumLayers = MAX_LAYERS;
    }

    usNumLayers = static_cast<uint16_t>(aiNumLayers);
}

NiColor& Clouds::GetColor(int32_t aiLayer) {
    return pColors[aiLayer];
}

void Clouds::SetColor(int32_t aiLayer, NiColor& aColor) {
    pColors[aiLayer] = aColor;
}

NiGeometry* Clouds::GetCloudsGeo(int32_t aiLayer) {
    return spClouds[aiLayer].m_pObject;
}

void Clouds::ZeroGeo() {
    for (int32_t i = 0; i < MAX_LAYERS; ++i) {
        spClouds[i] = nullptr;
    }
}

void Clouds::ClearTransTextures() {
    for (int32_t i = 0; i < GetNumLayers(); ++i) {
        spTransTexture[i] = nullptr;
        if (spClouds[i]) {
            NiProperty* pProperty = spClouds[i]->GetProperty(NiProperty::SHADE);
            SkyShaderProperty* pShader =
                static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty));
            if (pShader) {
                pShader->SetBlendTexture(nullptr);
                pShader->SetBlendValue(0.0f);
            }
        }
    }
}

void Clouds::RemoveTextures() {
    for (int32_t i = 0; i < GetNumLayers(); ++i) {
        if (spClouds[i]) {
            NiProperty* pProperty = spClouds[i]->GetProperty(NiProperty::SHADE);
            SkyShaderProperty* pShader =
                static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty));
            if (pShader) {
                pShader->SetTexture(nullptr);
                pShader->SetTextureFile(EmptyFS);
            }
        }
    }
}

void Clouds::ForceTransTextureUpdate() {
    bForceUpdate = true;
}
