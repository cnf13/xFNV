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
    BSStream kStream;
    if (kStream.BSStream::Load("Meshes\\Sky\\Clouds.nif", nullptr)) {
        Model kModel("Meshes\\Sky\\Clouds.nif", &kStream, false, false);
        NiNode* pModelRoot = kModel.GetModel();
        apSkyRoot->AttachChild(spRoot.m_pObject, 1);
        int32_t iNumLayers = 0;
        int32_t iChildCount = pModelRoot->GetChildCount();
        for (int32_t i = 0; i < iChildCount; ++i) {
            NiAVObject* pChild = pModelRoot->GetAt(static_cast<uint32_t>(i));
            if (pChild->IsGeometry()) {
                spClouds[i] = static_cast<NiGeometry*>(pModelRoot->GetAt(static_cast<uint32_t>(i)));
                spRoot->AttachChild(spClouds[i].m_pObject, 1);
                ++iNumLayers;
            }
        }
        SetNumLayers(iNumLayers);
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
            for (int32_t i = 0; i < GetNumLayers(); ++i) {
                int32_t iIsSkyShader;
                if (spClouds[i]->m_PropertyState.GetShade()) {
                    iIsSkyShader = spClouds[i]->m_PropertyState.GetShade()->iShaderPropertyType == 13;
                }
                else {
                    iIsSkyShader = 0;
                }
                SkyShaderProperty* pshadeProperty;
                if (iIsSkyShader) {
                    pshadeProperty = static_cast<SkyShaderProperty*>(spClouds[i]->m_PropertyState.GetShade());
                }
                else {
                    pshadeProperty = nullptr;
                }
                if (pshadeProperty) {
                    NiColor kcolor = GetColor(i);
                    NiColorA kVertexColor(kcolor.r, kcolor.g, kcolor.b, 1.0f);
                    pshadeProperty->VertexColor = kVertexColor;
                }
                float fCloudSpeed = pCurrentWeather->GetCloudLayerSpeed(i, fWeatherCloudSpeedMax.Float(), 0.0f);
                if (pLastWeather) {
                    double dWeatherPercent = apSky->GetWeatherPercent();
                    fCloudSpeed = dWeatherPercent * fCloudSpeed;
                    double dLastPercent = 1.0 - apSky->GetWeatherPercent();
                    double dLastCloudSpeed = pLastWeather->GetCloudLayerSpeed(i, fWeatherCloudSpeedMax.Float(), 0.0f);
                    fCloudSpeed = dLastCloudSpeed * dLastPercent + fCloudSpeed;
                }
                double dWindSpeed = apSky->fWindSpeed;
                float fdelta = dWindSpeed * afSecondsPassed * fCloudSpeed;
                SkyShader::SetCloudTexCoordOffset(i, fdelta);
            }
        }
        if (apSky->GetWeatherUpdateFlag() || bForceUpdate) {
            bForceUpdate = false;
            if (pLastWeather) {
                for (int32_t i = 0; i < GetNumLayers(); ++i) {
                    BSStringT<char> strfilename;
                    const TESTexture* ptestexture = pCurrentWeather->GetCloudTexture(i);
                    if (ptestexture && ptestexture->GetTextureNameLength()) {
                        strfilename = ptestexture->GetDefaultPath();
                        strfilename += ptestexture->GetTextureName();
                        SkyTextureManager::LoadTextureInBackground(&spTransTexture[i], strfilename.pString);
                    }
                    else {
                        spTransTexture[i] = static_cast<NiTexture*>(nullptr);
                    }
                }
            }
            else if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
                for (int32_t i = 0; i < GetNumLayers(); ++i) {
                    BSStringT<char> strfilename;
                    SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spClouds[i]->GetProperty(3));
                    const TESTexture* ptestexture = pCurrentWeather->GetCloudTexture(i);
                    if (ptestexture && ptestexture->GetTextureNameLength()) {
                        strfilename = ptestexture->GetDefaultPath();
                        strfilename += ptestexture->GetTextureName();
                        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
                        BSShaderManager::GetTexture(strfilename.pString, true, spTex, true, false);
                        NiTexture* ptex = spTex.m_pObject;
                        if (ptex) {
                            pprop->SetTexture(static_cast<NiSourceTexture*>(ptex));
                            pprop->SetTextureFile(strfilename.pString);
                            pprop->SetBlendValue(0.0f);
                        }
                    }
                }
            }
        }
        for (int32_t i = 0; i < GetNumLayers(); ++i) {
            NiProperty* pProperty = spClouds[i]->GetProperty(3);
            SkyShaderProperty* pskyprop = static_cast<SkyShaderProperty*>(
                NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
            );
            if (spTransTexture[i].m_pObject) {
                if (pskyprop) {
                    pskyprop->SetBlendTexture(spTransTexture[i].m_pObject);
                    pskyprop->SetBlendValue(apSky->GetWeatherPercent());
                }
                if (!pLastWeather) {
                    pskyprop->SetTexture(static_cast<NiSourceTexture*>(spTransTexture[i].m_pObject));
                    spTransTexture[i] = static_cast<NiTexture*>(nullptr);
                    pskyprop->SetBlendTexture(nullptr);
                    pskyprop->SetBlendValue(0.0f);
                }
            }
            bool bCulled = !pskyprop || !pskyprop->GetBaseTexture();
            spClouds[i]->SetAppCulled(bCulled);
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
    for (NiPointer<NiGeometry>& spCloud : spClouds) {
        spCloud = nullptr;
    }
}

void Clouds::ClearTransTextures() {
    for (int32_t i = 0; i < GetNumLayers(); ++i) {
        spTransTexture[i] = nullptr;
        if (spClouds[i]) {
            NiProperty* pProperty = spClouds[i]->GetProperty(NiProperty::SHADE);
            SkyShaderProperty* pShader = static_cast<SkyShaderProperty*>(
                NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
            );
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
            SkyShaderProperty* pShader = static_cast<SkyShaderProperty*>(
                NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
            );
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
