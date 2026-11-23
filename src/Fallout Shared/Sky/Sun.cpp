#include "Fallout Shared/Sky/Sun.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderData.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>

SettingT<GameSettingCollection> Sun::fSunMinimumGlareScale("fSunMinimumGlareScale", 0.25f);
SettingT<GameSettingCollection> Sun::fSunReduceGlareSpeed("fSunReduceGlareSpeed", 4.0f);
SettingT<GameSettingCollection> Sun::fSunAlphaTransTime("fSunAlphaTransTime", 2.0f);
SettingT<GameSettingCollection> Sun::fSunXExtreme("fSunXExtreme", -400.0f);
SettingT<GameSettingCollection> Sun::fSunYExtreme("fSunYExtreme", 25.0f);
SettingT<GameSettingCollection> gs_fSunZExtreme("fSunZExtreme", -100.0f);
SettingT<INISettingCollection> Sun::fSunBaseSize("fSunBaseSize:Weather", 250.0f);
SettingT<INISettingCollection> Sun::fSunGlareSize("fSunGlareSize:Weather", 350.0f);

Sun::Sun()
  : spSunBaseNode(nullptr), spSunGlareNode(nullptr), spSunBase(nullptr), spSunGlare(nullptr), pSunGlarePick(nullptr),
    spLight(nullptr), fGlareScale(0.0f), bDoOcclusionTests(false), spSunAccumulator(nullptr) {
    spSunAccumulator = new BSShaderAccumulator(100, 1u, 1u);
}

Sun::~Sun() {
    delete pSunGlarePick;
}

// Builds the sun base and glare quads (each under a billboard node facing the camera), the sun light, the glare's
// occlusion pick, and gives both quads sky shader properties. The quads are fSunBaseSize and fSunGlareSize half-wide.
// Each NiPoint3 argument list reads its y before its x, and the negated halves are rounded to float locals.
void Sun::Initialize(NiNode* apSkyRoot) {
    SkyObject::Initialize(apSkyRoot);

    NiPoint3* psunVertices = new NiPoint3[4];
    float fY = fSunBaseSize.Float();
    float fX = static_cast<float>(fSunBaseSize.Float() * -1.0);
    psunVertices[0] = NiPoint3(fX, fY, 0.0f);
    fY = static_cast<float>(fSunBaseSize.Float() * -1.0);
    fX = static_cast<float>(fSunBaseSize.Float() * -1.0);
    psunVertices[1] = NiPoint3(fX, fY, 0.0f);
    fY = fSunBaseSize.Float();
    fX = fSunBaseSize.Float();
    psunVertices[2] = NiPoint3(fX, fY, 0.0f);
    fY = static_cast<float>(fSunBaseSize.Float() * -1.0);
    fX = fSunBaseSize.Float();
    psunVertices[3] = NiPoint3(fX, fY, 0.0f);

    NiPoint3* psunNormals = new NiPoint3[4];
    psunNormals[0] = NiPoint3::ZERO;
    psunNormals[1] = NiPoint3::ZERO;
    psunNormals[2] = NiPoint3::ZERO;
    psunNormals[3] = NiPoint3::ZERO;

    NiPoint3* pglareVertices = new NiPoint3[4];
    fY = fSunGlareSize.Float();
    fX = static_cast<float>(fSunGlareSize.Float() * -1.0);
    pglareVertices[0] = NiPoint3(fX, fY, 0.0f);
    fY = static_cast<float>(fSunGlareSize.Float() * -1.0);
    fX = static_cast<float>(fSunGlareSize.Float() * -1.0);
    pglareVertices[1] = NiPoint3(fX, fY, 0.0f);
    fY = fSunGlareSize.Float();
    fX = fSunGlareSize.Float();
    pglareVertices[2] = NiPoint3(fX, fY, 0.0f);
    fY = static_cast<float>(fSunGlareSize.Float() * -1.0);
    fX = fSunGlareSize.Float();
    pglareVertices[3] = NiPoint3(fX, fY, 0.0f);

    NiPoint3* pglareNormals = new NiPoint3[4];
    pglareNormals[0] = NiPoint3::ZERO;
    pglareNormals[1] = NiPoint3::ZERO;
    pglareNormals[2] = NiPoint3::ZERO;
    pglareNormals[3] = NiPoint3::ZERO;

    {
        alignas(NiColorA) uint8_t aucDefaultColor[sizeof(NiColorA)];
        const NiColorA& defaultColor = *::new (aucDefaultColor) NiColorA(1.0f, 0.0f, 0.0f, 1.0f);

        NiColorA* psunColors = new NiColorA[4];
        psunColors[0] = defaultColor;
        psunColors[1] = defaultColor;
        psunColors[2] = defaultColor;
        psunColors[3] = defaultColor;

        NiColorA* pglareColors = new NiColorA[4];
        pglareColors[0] = defaultColor;
        pglareColors[1] = defaultColor;
        pglareColors[2] = defaultColor;
        pglareColors[3] = defaultColor;

        NiPoint2* psunTexture = new NiPoint2[4];
        psunTexture[0] = NiPoint2(0.0f, 0.0f);
        psunTexture[1] = NiPoint2(0.0f, 1.0f);
        psunTexture[2] = NiPoint2(1.0f, 0.0f);
        psunTexture[3] = NiPoint2(1.0f, 1.0f);

        NiPoint2* pglareTexture = new NiPoint2[4];
        pglareTexture[0] = NiPoint2(0.0f, 0.0f);
        pglareTexture[1] = NiPoint2(0.0f, 1.0f);
        pglareTexture[2] = NiPoint2(1.0f, 0.0f);
        pglareTexture[3] = NiPoint2(1.0f, 1.0f);

        // Two triangles per quad.
        uint16_t* ssunVertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
        ssunVertOrder[0] = 0;
        ssunVertOrder[1] = 1;
        ssunVertOrder[2] = 2;
        ssunVertOrder[3] = 2;
        ssunVertOrder[4] = 1;
        ssunVertOrder[5] = 3;
        uint16_t* sglareVertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
        sglareVertOrder[0] = 0;
        sglareVertOrder[1] = 1;
        sglareVertOrder[2] = 2;
        sglareVertOrder[3] = 2;
        sglareVertOrder[4] = 1;
        sglareVertOrder[5] = 3;

        NiTriShape* pSunBase = new NiTriShape(
            4,
            psunVertices,
            psunNormals,
            psunColors,
            psunTexture,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            2,
            ssunVertOrder
        );
        spSunBase = pSunBase;
        spSunBase->SetSelectiveUpdate(true);

        NiBillboardNode* pSunBaseNode = new NiBillboardNode;
        spSunBaseNode = pSunBaseNode;
        spSunBaseNode->SetSelectiveUpdate(true);
        spSunBaseNode->SetMode(NiBillboardNode::RIGID_FACE_CAMERA);
        NiNode* pParent = spSunBaseNode;
        pParent->AttachChild(spSunBase, true);

        NiTriShape* pSunGlare = new NiTriShape(
            4,
            pglareVertices,
            pglareNormals,
            pglareColors,
            pglareTexture,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            2,
            sglareVertOrder
        );
        spSunGlare = pSunGlare;
        spSunGlare->SetSelectiveUpdate(true);
    }

    NiBillboardNode* pSunGlareNode = new NiBillboardNode;
    spSunGlareNode = pSunGlareNode;
    spSunGlareNode->SetSelectiveUpdate(true);
    spSunGlareNode->SetMode(NiBillboardNode::RIGID_FACE_CAMERA);
    float fScale = fGlareScale;
    spSunGlareNode->SetScale(fScale);
    NiNode* pParent = spSunGlareNode;
    pParent->AttachChild(spSunGlare, true);
    // SkyObject keeps spRoot as a raw pointer.
    pParent = spRoot;
    pParent->AttachChild(spSunBaseNode, true);

    NiDirectionalLight* pLight = new NiDirectionalLight;
    spLight = pLight;
    spLight->SetDimmer(1.0f);
    spLight->SetAmbientColor(NiColor::WHITE);
    spLight->SetDiffuseColor(NiColor::WHITE);
    // SkyObject keeps spRoot as a raw pointer.
    pParent = spRoot;
    pParent->AttachChild(spLight, true);

    pSunGlarePick = ::new NiPick(0, 8);
    pSunGlarePick->SetPickType(NiPick::FIND_FIRST);
    pSunGlarePick->SetIntersectType(NiPick::TRIANGLE_INTERSECT);
    pSunGlarePick->SetFrontOnly(true);
    pSunGlarePick->SetObserveAppCullFlag(true);

    alignas(BSShaderData) uint8_t aShaderDataStorage[sizeof(BSShaderData)];
    BSShaderData* pBSShaderData = ::new (aShaderDataStorage) BSShaderData;
    pBSShaderData->eLightingShader = BSShaderData::LS_SKY;
    pBSShaderData->iSkyTYpe = SkyShaderProperty::SO_SUN;
    pBSShaderData->Convert(spSunBase, BSShaderData::PROP_SHADER);
    pBSShaderData->iSkyTYpe = SkyShaderProperty::SO_SUN_GLARE;
    pBSShaderData->Convert(spSunGlare, BSShaderData::PROP_SHADER);
}

void Sun::Update(Sky* apSky, float afSecondsPassed) {
    NiMatrix3 interiorRotmatCombined;
    NiMatrix3 rotmatCombined;
    NiMatrix3 newSunRot;
    NiMatrix3 interiorLightDir;
    NiMatrix3 lightDir;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp",
        300
    );
    NiPoint3 newLoc = NiPoint3::ZERO;
    float fHour = apSky->GetCurrentGameHour();
    SkyObject::Update(apSky, afSecondsPassed);
    if (apSky->eMode) {
        if (apSky->eMode == Sky::SM_INTERIOR) {
            interiorLightDir = NiMatrix3::IDENTITY;
            TESObjectCELL* pCell = TES::pInstance ? TES::pInstance->pInteriorCell : nullptr;
            if (pCell) {
                NiMatrix3 rotmat;
                float fZRotation = static_cast<double>(pCell->GetDataDirectionalXY()) * 0.01745329238474369;
                interiorLightDir.MakeZRotation(fZRotation);
                float fYRotation = static_cast<double>(pCell->GetDataDirectionalZ()) * 0.01745329238474369;
                rotmat.MakeYRotation(fYRotation);
                interiorLightDir = *::new (&interiorRotmatCombined) NiMatrix3(interiorLightDir * rotmat);
            }
            else {
                interiorLightDir.SetEntry(1u, 0, -0.7853982f);
                interiorLightDir.SetEntry(2u, 0, -0.7853982f);
            }
            spLight->SetRotate(interiorLightDir);
            // SkyColor[3] is the ambient light color.
            spLight->SetAmbientColor(apSky->SkyColor[3]);
            spLight->SetDiffuseColor(*apSky->GetSunLightColor());
            spLight->SetSpecularColor(apSky->SunSpecularColor);
        }
        else if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
            SkyShaderProperty* pShadeProperty;
            int32_t iSkyShader;
            float fLastGlare;
            float fCurrGlare;
            float fTemp;
            bool bDisplayObject = spRoot->GetDisplayObject();
            spRoot->SetAppCulled(!bDisplayObject);
            double dSkySunriseEnd = apSky->GetSunriseEnd();
            float fSunriseBegin = (apSky->GetSunriseBegin() + dSkySunriseEnd) / 2.0;
            double dSkySunsetEnd = apSky->GetSunsetEnd();
            float fSunsetEnd = (apSky->GetSunsetBegin() + dSkySunsetEnd) / 2.0;
            float fHalfTrans = Sun::fSunAlphaTransTime.Float() * 0.5;
            float fAlphaInBegin = fSunriseBegin - fHalfTrans;
            float fAlphaInEnd = fSunriseBegin + fHalfTrans;
            float fAlphaOutBegin = fSunsetEnd - fHalfTrans;
            float fAlphaOutEnd = fSunsetEnd + fHalfTrans;
            float fDiscAlpha = 1.0f;
            if (fHour >= static_cast<double>(fAlphaInBegin) && fAlphaOutEnd >= static_cast<double>(fHour)) {
                if (fHour > static_cast<double>(fAlphaInBegin) && fAlphaInEnd > static_cast<double>(fHour)) {
                    fDiscAlpha = (fHour - fAlphaInBegin) / (fAlphaInEnd - fAlphaInBegin);
                }
                else if (fHour > static_cast<double>(fAlphaOutBegin) && fAlphaOutEnd > static_cast<double>(fHour)) {
                    fDiscAlpha = 1.0 - (fHour - fAlphaOutBegin) / (fAlphaOutEnd - fAlphaOutBegin);
                }
            }
            else {
                fDiscAlpha = 0.0f;
            }
            TESWeather* pCurr = apSky->pCurrentWeather;
            TESWeather* pLast = apSky->pLastWeather;
            float fCurrPct = apSky->GetWeatherPercent();
            if (pCurr) {
                float fCurrAlpha = fCurrPct * fDiscAlpha;
                fCurrGlare = pCurr->GetWeatherData(TESWeather::WD_SUN_GLARE, fCurrAlpha, 0.0f);
            }
            else {
                fCurrGlare = fDiscAlpha;
            }
            if (pLast) {
                float fLastAlpha = (1.0 - fCurrPct) * fDiscAlpha;
                fLastGlare = pLast->GetWeatherData(TESWeather::WD_SUN_GLARE, fLastAlpha, 0.0f);
            }
            else {
                fLastGlare = 0.0f;
            }
            float fGlareAlpha = fCurrGlare + fLastGlare;
            if (spSunBase->GetProperty(3)) {
                NiProperty* pProperty = spSunBase->GetProperty(3);
                iSkyShader = static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType ==
                    BSShaderManager::BSSM_PROPERTYTYPE_SKY;
            }
            else {
                iSkyShader = 0;
            }
            if (iSkyShader) {
                pShadeProperty = static_cast<SkyShaderProperty*>(spSunBase->GetProperty(3));
            }
            else {
                pShadeProperty = nullptr;
            }
            if (pShadeProperty) {
                // SkyColor[5] is the sun color; the colour is built from it and copied into the property's vertex
                // colour.
                NiColorA kSunColor(apSky->SkyColor[5].r, apSky->SkyColor[5].g, apSky->SkyColor[5].b, fDiscAlpha);
                pShadeProperty->VertexColor = kSunColor;
            }
            if (spSunGlare->GetProperty(3)) {
                NiProperty* pProperty = spSunGlare->GetProperty(3);
                iSkyShader = static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType ==
                    BSShaderManager::BSSM_PROPERTYTYPE_SKY;
            }
            else {
                iSkyShader = 0;
            }
            if (iSkyShader) {
                pShadeProperty = static_cast<SkyShaderProperty*>(spSunGlare->GetProperty(3));
            }
            else {
                pShadeProperty = nullptr;
            }
            if (pShadeProperty) {
                NiColorA kGlareColor(apSky->SkyColor[5].r, apSky->SkyColor[5].g, apSky->SkyColor[5].b, fGlareAlpha);
                pShadeProperty->VertexColor = kGlareColor;
            }
            float fxNewAim = 0.0f;
            newSunRot = NiMatrix3::IDENTITY;
            if (fHour > static_cast<double>(fAlphaInBegin) && fAlphaOutEnd > static_cast<double>(fHour)) {
                fTemp = (fHour - fAlphaInBegin) / (fAlphaOutEnd - fAlphaInBegin);
                double dAim = 1.0 - fTemp * 2.0;
                fxNewAim = Sun::fSunXExtreme.Float() * dAim;
            }
            else if (fAlphaOutEnd <= static_cast<double>(fHour)) {
                fTemp = (fHour - fAlphaOutEnd) / (24.0 - (fAlphaOutEnd - fAlphaInBegin));
                double dAim = fTemp * 2.0 - 1.0;
                fxNewAim = Sun::fSunXExtreme.Float() * dAim;
            }
            else if (fAlphaInBegin >= static_cast<double>(fHour)) {
                fTemp = (24.0 - fAlphaOutEnd + fHour) / (24.0 - (fAlphaOutEnd - fAlphaInBegin));
                double dAim = fTemp * 2.0 - 1.0;
                fxNewAim = Sun::fSunXExtreme.Float() * dAim;
            }
            newLoc.x = fxNewAim;
            newLoc.y = Sun::fSunYExtreme;
            double dAbsXExtreme = std::fabsf(Sun::fSunXExtreme);
            newLoc.z = dAbsXExtreme - std::fabsf(fxNewAim);
            spRoot->SetTranslate(newLoc);
            spSunGlareNode->SetTranslate(newLoc);
            float fZExtreme = gs_fSunZExtreme;
            float fMaxb = Sun::fSunYExtreme;
            NiPoint3 kSunDirection(-fxNewAim, fMaxb, fZExtreme);
            kSunDirection.Unitize();
            newSunRot.SetCol(0, kSunDirection);
            spLight->SetRotate(newSunRot);
            spLight->SetAmbientColor(apSky->SkyColor[3]);
            spLight->SetDiffuseColor(*apSky->GetSunLightColor());
            spLight->SetSpecularColor(apSky->SunSpecularColor);
            if (TES::pInstance && TES::pInstance->pObjRoot) {
                NiPoint3 direction = newLoc;
                float fScaleChange = Sun::fSunReduceGlareSpeed.Float() * afSecondsPassed;
                direction.Unitize();
                bool bHideGlare = true;
                if (!(GetSunOcclusionPercent() >= 1.0) && !TESWaterSystem::GetPlayerUnderwater()) {
                    bHideGlare = false;
                }

                float fVisible = 1.0 - GetSunOcclusionPercent();
                float fScale = std::pow(fVisible, 2.0f);
                StopSunOcclusion();
                if (bHideGlare || Sun::fSunMinimumGlareScale.Float() > static_cast<double>(fScale)) {
                    double dNewScale = fGlareScale - fScaleChange;
                    if (Sun::fSunMinimumGlareScale.Float() > dNewScale) {
                        spSunGlareNode->SetAppCulled(true);
                        fGlareScale = 0.0f;
                    }
                    else {
                        spSunGlareNode->SetAppCulled(false);
                        fGlareScale = fGlareScale - fScaleChange;
                        spSunGlareNode->SetScale(fGlareScale);
                    }
                }
                else {
                    if (!(fScale <= fGlareScale + fScaleChange) && !(fScale <= static_cast<double>(fGlareScale))) {
                        fGlareScale = fGlareScale + fScaleChange;
                    }
                    else {
                        fGlareScale = fScale;
                    }
                    spSunGlareNode->SetAppCulled(false);
                    spSunGlareNode->SetScale(fGlareScale);
                }
                if (!spSunGlareNode->GetDisplayObject()) {
                    spSunGlareNode->SetAppCulled(true);
                }
            }

            if (apSky->IsUnderwater()) {
                GetSunBaseNode()->SetAppCulled(true);
            }
            else {
                GetSunBaseNode()->SetAppCulled(false);
            }
        }
    }
    else {
        lightDir = NiMatrix3::IDENTITY;
        TESObjectCELL* pCell = TES::pInstance ? TES::pInstance->pInteriorCell : nullptr;
        if (pCell) {
            NiMatrix3 rotmat;
            float fZRotation = static_cast<double>(pCell->GetDataDirectionalXY()) * 0.01745329238474369;
            lightDir.MakeZRotation(fZRotation);
            float fYRotation = static_cast<double>(pCell->GetDataDirectionalZ()) * 0.01745329238474369;
            rotmat.MakeYRotation(fYRotation);
            lightDir = *::new (&rotmatCombined) NiMatrix3(lightDir * rotmat);
        }
        else {
            lightDir.SetEntry(1u, 0, -0.7853982f);
            lightDir.SetEntry(2u, 0, -0.7853982f);
        }
        spLight->SetRotate(lightDir);
        spLight->SetAmbientColor(apSky->SkyColor[3]);
        spLight->SetDiffuseColor(*apSky->GetSunLightColor());
        spLight->SetSpecularColor(apSky->SunSpecularColor);
    }
}

NiAVObject* Sun::GetBase() {
    return spSunBase;
}

NiDirectionalLight* Sun::GetSunLight() {
    return spLight;
}

NiBillboardNode* Sun::GetSunGlareNode() {
    return spSunGlareNode;
}

NiTriShape* Sun::GetSunGlareGeo() const {
    return spSunGlare;
}

float Sun::GetIntensity() {
    int32_t iSkyShader;
    SkyShaderProperty* pShadeProperty;
    if (spSunGlare->GetProperty(3)) {
        NiShadeProperty* pProperty = static_cast<NiShadeProperty*>(spSunGlare->GetProperty(3));
        iSkyShader = pProperty->GetShaderPropertyType() == BSShaderManager::BSSM_PROPERTYTYPE_SKY;
    }
    else {
        iSkyShader = 0;
    }
    if (iSkyShader) {
        pShadeProperty = static_cast<SkyShaderProperty*>(spSunGlare->GetProperty(3));
    }
    else {
        pShadeProperty = nullptr;
    }
    if (pShadeProperty) {
        return pShadeProperty->VertexColor.a;
    }

    return 0.0;
}

// Clears the texture and texture file of the sky shader property on the sun base and on the glare.
void Sun::RemoveTextures() {
    NiProperty* pProperty = spSunBase->GetProperty(NiProperty::SHADE);
    SkyShaderProperty* pShaderProperty;
    pShaderProperty = static_cast<SkyShaderProperty*>(
        NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
    );
    if (pShaderProperty) {
        pShaderProperty->SetTexture(nullptr);
        pShaderProperty->SetTextureFile(EmptyFS);
    }
    pProperty = spSunGlare->GetProperty(NiProperty::SHADE);
    pShaderProperty = static_cast<SkyShaderProperty*>(
        NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
    );
    if (pShaderProperty) {
        pShaderProperty->SetTexture(nullptr);
        pShaderProperty->SetTextureFile(EmptyFS);
    }
}

void Sun::StartSunOcclusion() {
    bDoOcclusionTests = true;
    BSShaderAccumulator* pAccumulator = spSunAccumulator;
    if (pAccumulator) {
        pAccumulator->ResetSunOcclusion();
    }
}

void Sun::StopSunOcclusion() {
    bDoOcclusionTests = false;
}

void Sun::CalculateSunGlare(uint32_t aiTestNumber, BSShaderAccumulator* apAccumulator, NiCamera* apCamera) {
    AutoMemContext
        kMemContext(MEM_CONTEXT_SKY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp", 546);
    if (GetBase() && bDoOcclusionTests && apAccumulator) {
        spSunAccumulator = apAccumulator;
        NiTriShape* pSunBase = static_cast<NiTriShape*>(GetBase());
        float fReducedScale = pSunBase->GetWorldScale() / 25.0;
        pSunBase->SetWorldScale(fReducedScale);
        apAccumulator->DoSunOcclusionQuery(pSunBase, aiTestNumber, apCamera);
        float fRestoredScale = pSunBase->GetWorldScale() * 25.0;
        pSunBase->SetWorldScale(fRestoredScale);
    }
}

float Sun::GetSunOcclusionPercent() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp",
        565
    );
    BSShaderAccumulator* pAccum = spSunAccumulator.m_pObject;
    if (pAccum) {
        return pAccum->fPercentSunOccludedStored;
    }

    return 0.0f;
}
