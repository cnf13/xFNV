#include "Fallout Shared/Sky/Moon.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderData.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout Shared/Sky/Stars.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Interface/Menus/RaceSexMenu.h"
#include "Fallout/Misc/Calendar.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <limits>

Moon::Moon(
    const char* apMoonName,
    float afAngleFadeStart,
    float afAngleFadeEnd,
    float afAngleShadowEarlyFade,
    float afSpeed,
    float afZOffset,
    uint32_t aiSize
)
  : spMoonNode(nullptr), spShadowNode(nullptr), spMoonMesh(nullptr), spShadowMesh(nullptr),
    fAngleFadeStart(afAngleFadeStart), fAngleFadeEnd(afAngleFadeEnd), fAngleShadowEarlyFade(afAngleShadowEarlyFade),
    fSpeed(afSpeed), fZOffset(afZOffset), iSize(aiSize), eUpdateMoonTexture(US_NOT_REQUIRED), fPos(0.0f),
    fLastPosUpdate(std::numeric_limits<float>::max()) {
    AutoMemContext kMemContext(MEM_CONTEXT_SKY);
    strTextures[3].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "one_wan");
    strTextures[5].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "one_wax");
    strTextures[2].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "half_wan");
    strTextures[6].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "half_wax");
    strTextures[1].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "three_wan");
    strTextures[7].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "three_wax");
    strTextures[0].SPrintF("\\Textures\\Sky\\%s_%s.dds", apMoonName, "full");
}

Moon::~Moon() {
    if (spRoot->m_pkParent) {
        spRoot->m_pkParent->DetachChild(spRoot.m_pObject);
    }
}

void Moon::Update(Sky* apSky, float afSecondsPassed) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Moon.cpp",
        292
    );
    SkyObject::Update(apSky, afSecondsPassed);
    if (fLastPosUpdate == 3.4028234663852886e+38) {
        fLastPosUpdate = 0.0f;
        fPos = 90.0f;
    }
    if (pSaveLoadGameOLD->GetLoading()) {
        fLastPosUpdate = -(static_cast<double>(appCalendar.GetDaysPassed() - 1) * 24.0);
        fPos = 90.0f;
    }
    float fTimePassed = apSky->GetCurrentGameHour() - fLastPosUpdate;
    if (fTimePassed < 0.0) {
        fTimePassed = fTimePassed + 24.0;
    }
    for (fPos = fSpeed * 60.0 * fTimePassed + fPos; fPos < 0.0; fPos = fPos + 360.0) {
        ;
    }
    while (fPos >= 360.0) {
        fPos = fPos - 360.0;
    }
    fLastPosUpdate = apSky->GetCurrentGameHour();
    float fvisMoon = GetMoonVisibility();
    float fvisShadow = GetMoonShadowVisibility();
    float fvisStars = apSky->pStars->fAlpha;
    fvisShadow = BSMIN(fvisShadow, fvisStars);
    if (eUpdateMoonTexture == Moon::US_INITIALIZE ||
        (eUpdateMoonTexture == Moon::US_WHEN_CULLED && (fvisMoon <= 0.0 && fvisShadow <= 0.0))) {
        SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spMoonMesh->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture(strTextures[Moon::eCurrentPhase].pString, true, spTex, true, false);
        NiTexture* ptex = spTex.m_pObject;
        if (ptex) {
            pprop->SetTexture(static_cast<NiSourceTexture*>(ptex));
        }
        spMoonMesh->SetDisplayObject(ptex != nullptr);
        eUpdateMoonTexture = Moon::US_NOT_REQUIRED;
    }
    if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
        NiMatrix3 matrixMoonZ;
        NiMatrix3 matrixMoonX;
        if (!spRoot->GetDisplayObject()) {
            spRoot->SetAppCulled(true);
            return;
        }
        float fXAngle = -fPos * 0.01745329238474369;
        matrixMoonX.MakeXRotation(fXAngle);
        float fZAngle = fZOffset * 0.01745329238474369;
        matrixMoonZ.MakeZRotation(fZAngle);
        spRoot->SetRotate(matrixMoonX * matrixMoonZ);
        if (fvisMoon <= 0.0 && fvisShadow <= 0.0) {
            spRoot->SetAppCulled(true);
        }
        else {
            spRoot->SetAppCulled(false);
            if (!(fvisMoon <= 0.0) && spMoonMesh->GetDisplayObject()) {
                SkyShaderProperty* pshadeProperty;
                int32_t bIsSkyShader;
                spMoonNode->SetAppCulled(false);
                if (spMoonMesh->GetProperty(3)) {
                    NiProperty* pProperty = spMoonMesh->GetProperty(3);
                    bIsSkyShader = static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType ==
                        BSShaderManager::BSSM_PROPERTYTYPE_SKY;
                }
                else {
                    bIsSkyShader = 0;
                }
                if (bIsSkyShader) {
                    pshadeProperty = static_cast<SkyShaderProperty*>(spMoonMesh->GetProperty(3));
                }
                else {
                    pshadeProperty = nullptr;
                }
                if (pshadeProperty) {
                    pshadeProperty->VertexColor.a = fvisMoon;
                }
            }
            else {
                spMoonNode->SetAppCulled(true);
            }
            if (!(fvisShadow <= 0.0) && spShadowMesh->GetDisplayObject()) {
                SkyShaderProperty* pshadeProperty;
                int32_t bIsSkyShader;
                spShadowNode->SetAppCulled(false);
                if (spShadowMesh->GetProperty(3)) {
                    NiProperty* pProperty = spShadowMesh->GetProperty(3);
                    bIsSkyShader = static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType ==
                        BSShaderManager::BSSM_PROPERTYTYPE_SKY;
                }
                else {
                    bIsSkyShader = 0;
                }
                if (bIsSkyShader) {
                    pshadeProperty = static_cast<SkyShaderProperty*>(spShadowMesh->GetProperty(3));
                }
                else {
                    pshadeProperty = nullptr;
                }
                if (pshadeProperty) {
                    NiColorA kColor(
                        (*&apSky->SkyColor)[0].r,
                        (*&apSky->SkyColor)[0].g,
                        (*&apSky->SkyColor)[0].b,
                        fvisShadow
                    );
                    pshadeProperty->VertexColor = kColor;
                }
            }
            else {
                spShadowNode->SetAppCulled(true);
            }
        }
    }
}

void Moon::Initialize(NiNode* apSkyRoot, const char* apName) {
    NiMatrix3 matrixMoon;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Moon.cpp",
        102
    );
    SkyObject::Initialize(apSkyRoot);
    NiPoint3* pNormalsMoon = new NiPoint3[4];
    NiPoint3* pVerticesMoon = new NiPoint3[4];
    NiPoint3* pNormalsShadow = new NiPoint3[4];
    NiPoint3* pVerticesShadow = new NiPoint3[4];
    float fSize = static_cast<float>(iSize);
    float fNegSize = static_cast<double>(iSize) * -1.0;
    pVerticesMoon[0] = NiPoint3(fNegSize, fSize, 0.0f);
    pVerticesMoon[1] = NiPoint3(fNegSize, fNegSize, 0.0f);
    pVerticesMoon[2] = NiPoint3(fSize, fSize, 0.0f);
    pVerticesMoon[3] = NiPoint3(fSize, fNegSize, 0.0f);
    pVerticesShadow[0] = NiPoint3(fNegSize, fSize, 0.0f);
    pVerticesShadow[1] = NiPoint3(fNegSize, fNegSize, 0.0f);
    pVerticesShadow[2] = NiPoint3(fSize, fSize, 0.0f);
    pVerticesShadow[3] = NiPoint3(fSize, fNegSize, 0.0f);
    NiPoint2* pTexturesMoon = new NiPoint2[4];
    NiPoint2* pTexturesShadow = new NiPoint2[4];
    pTexturesMoon[0] = NiPoint2(0.0f, 0.0f);
    pTexturesMoon[1] = NiPoint2(0.0f, 1.0f);
    pTexturesMoon[2] = NiPoint2(1.0f, 0.0f);
    pTexturesMoon[3] = NiPoint2(1.0f, 1.0f);
    pTexturesShadow[0] = NiPoint2(0.0f, 0.0f);
    pTexturesShadow[1] = NiPoint2(0.0f, 1.0f);
    pTexturesShadow[2] = NiPoint2(1.0f, 0.0f);
    pTexturesShadow[3] = NiPoint2(1.0f, 1.0f);
    NiColorA* pColorsMoon = new NiColorA[4];
    NiColorA* pColorsShadow = new NiColorA[4];
    // Copies the colour into all eight entries.
    NiColorA defaultColor(1.0f, 0.0f, 0.0f, 1.0f);
    pColorsMoon[0] = defaultColor;
    pColorsMoon[1] = defaultColor;
    pColorsMoon[2] = defaultColor;
    pColorsMoon[3] = defaultColor;
    pColorsShadow[0] = defaultColor;
    pColorsShadow[1] = defaultColor;
    pColorsShadow[2] = defaultColor;
    pColorsShadow[3] = defaultColor;
    uint16_t* sVertOrderMoon = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    uint16_t* sVertOrderShadow = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    sVertOrderMoon[0] = 0;
    sVertOrderMoon[1] = 1;
    sVertOrderMoon[2] = 2;
    sVertOrderMoon[3] = 2;
    sVertOrderMoon[4] = 1;
    sVertOrderMoon[5] = 3;
    sVertOrderShadow[0] = 0;
    sVertOrderShadow[1] = 1;
    sVertOrderShadow[2] = 2;
    sVertOrderShadow[3] = 2;
    sVertOrderShadow[4] = 1;
    sVertOrderShadow[5] = 3;
    spMoonMesh = new NiTriShape(
        4,
        pVerticesMoon,
        pNormalsMoon,
        pColorsMoon,
        pTexturesMoon,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2,
        sVertOrderMoon
    );
    spMoonMesh->SetSelectiveUpdate(true);
    spShadowMesh = new NiTriShape(
        4,
        pVerticesShadow,
        pNormalsShadow,
        pColorsShadow,
        pTexturesShadow,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2,
        sVertOrderShadow
    );
    spShadowMesh->SetSelectiveUpdate(true);
    {
        NiPointer<NiAlphaProperty> spAlpha(new NiAlphaProperty);
        spAlpha->SetAlphaBlending(true);
        spAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        spAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
        spAlpha->SetNoSorter(true);
        spMoonMesh->AttachProperty(spAlpha.m_pObject);
        spAlpha = static_cast<NiAlphaProperty*>(nullptr);
    }
    BSShaderData bsshaderdata;
    bsshaderdata.eLightingShader = BSShaderData::LS_SKY;
    bsshaderdata.iSkyTYpe = 6;
    bsshaderdata.Convert(spMoonMesh.m_pObject, 1u);
    bsshaderdata.iSkyTYpe = 7;
    bsshaderdata.Convert(spShadowMesh.m_pObject, 1u);
    if (Sky::GetInstance()->eMode == Sky::SM_FULL || Sky::GetInstance()->eMode == Sky::SM_FAKEEXTERIOR) {
        SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spShadowMesh->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture("Textures\\Sky\\MoonShadow.dds", true, spTex, true, false);
        NiTexture* ptex = spTex.m_pObject;
        if (ptex) {
            pprop->SetTexture(static_cast<NiSourceTexture*>(ptex));
        }
        spShadowMesh->SetDisplayObject(ptex != nullptr);
        eUpdateMoonTexture = Moon::US_INITIALIZE;
    }
    spMoonNode = new NiNode(0);
    spShadowNode = new NiNode(0);
    spMoonNode->SetSelectiveUpdate(true);
    spShadowNode->SetSelectiveUpdate(true);
    spMoonNode->AttachChild(spMoonMesh.m_pObject, 1);
    spShadowNode->AttachChild(spShadowMesh.m_pObject, 1);
    spRoot->AttachChild(spShadowNode.m_pObject, 1);
    spRoot->AttachChild(spMoonNode.m_pObject, 1);
    matrixMoon.MakeXRotation(-1.5707964f);
    spMoonMesh->SetTranslate(0.0f, 512.0f, 0.0f);
    spMoonMesh->SetRotate(matrixMoon);
    spShadowMesh->SetTranslate(0.0f, 512.0f, 0.0f);
    spShadowMesh->SetRotate(matrixMoon);
    BSShaderManager::PrepareObject(spRoot.m_pObject, 0, 0);
}

void Moon::RemoveTextures() {
    NiProperty* pProperty = spMoonMesh->GetProperty(3);
    SkyShaderProperty* pShader = static_cast<SkyShaderProperty*>(
        NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty)
    );
    if (pShader) {
        pShader->SetTexture(nullptr);
        pShader->SetTextureFile(EmptyFS);
    }
    pProperty = spShadowMesh->GetProperty(3);
    pShader = static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty));
    if (pShader) {
        pShader->SetTexture(nullptr);
        pShader->SetTextureFile(EmptyFS);
    }
}

float Moon::GetMoonVisibility() const {
    float fSetFadeStart = static_cast<float>(180.0 - fAngleFadeStart);
    float fSetFadeEnd = static_cast<float>(180.0 - fAngleFadeEnd);
    if (fPos > 180.0) {
        return 0.0;
    }
    if (fAngleFadeStart >= static_cast<double>(fPos) && fAngleFadeEnd <= static_cast<double>(fPos)) {
        return (fPos - fAngleFadeEnd) / (fAngleFadeStart - fAngleFadeEnd);
    }
    if (fSetFadeStart <= static_cast<double>(fPos) && fSetFadeEnd >= static_cast<double>(fPos)) {
        return (fSetFadeEnd - fPos) / (fSetFadeEnd - fSetFadeStart);
    }
    if (fAngleFadeStart < static_cast<double>(fPos) && fSetFadeStart > static_cast<double>(fPos)) {
        return 1.0;
    }
    return 0.0;
}

float Moon::GetMoonShadowVisibility() const {
    float fShadowFadeStart = fAngleFadeEnd;
    float fShadowFadeEnd = fShadowFadeStart - fAngleShadowEarlyFade;
    float fSetFadeStart = static_cast<float>(180.0 - fShadowFadeStart);
    float fSetFadeEnd = static_cast<float>(180.0 - fShadowFadeEnd);
    if (fPos > 180.0) {
        return 0.0;
    }
    if (fShadowFadeStart >= static_cast<double>(fPos) && fShadowFadeEnd <= static_cast<double>(fPos)) {
        return (fPos - fShadowFadeEnd) / (fShadowFadeStart - fShadowFadeEnd);
    }
    if (fSetFadeStart <= static_cast<double>(fPos) && fSetFadeEnd >= static_cast<double>(fPos)) {
        return (fSetFadeEnd - fPos) / (fSetFadeEnd - fSetFadeStart);
    }
    if (fShadowFadeStart < static_cast<double>(fPos) && fSetFadeStart > static_cast<double>(fPos)) {
        return 1.0;
    }
    return 0.0;
}

// The phase is (days passed mod 8 phase lengths) / phase length; each operand is fetched again.
bool Moon::UpdatePhase(const Sky* apSky) {
    if (!apSky->GetCurrentClimate() || !apSky->GetCurrentClimate()->GetMoonPhaseDays()) {
        return false;
    }
    uint32_t uiDaysPassed = appCalendar.GetDaysPassed();
    uint32_t uiCycleDay = uiDaysPassed % (8 * apSky->GetCurrentClimate()->GetMoonPhaseDays());
    uint32_t uiPhase = uiCycleDay / apSky->GetCurrentClimate()->GetMoonPhaseDays();
    if (uiPhase == static_cast<uint32_t>(eCurrentPhase)) {
        return false;
    }
    eCurrentPhase = uiPhase;
    return true;
}
