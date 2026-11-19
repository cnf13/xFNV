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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "SDK/MSVC/ehvec.h"

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
    fLastPosUpdate(3.4028235e+38f) {
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
    if (spRoot.m_pObject->m_pkParent) {
        spRoot.m_pObject->m_pkParent->DetachChild(spRoot.m_pObject);
    }
}

void Moon::Update(Sky* apSky, float afSecondsPassed) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Moon.cpp", 292
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
        SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spMoonMesh.m_pObject->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture(strTextures[Moon::eCurrentPhase].pString, true, spTex, true, false);
        NiTexture* ptex = spTex.m_pObject;
        if (ptex) {
            pprop->SetTexture(static_cast<NiSourceTexture*>(ptex));
        }
        spMoonMesh.m_pObject->SetDisplayObject(ptex != nullptr);
        eUpdateMoonTexture = Moon::US_NOT_REQUIRED;
    }
    if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
        NiMatrix3 matrixMoonZ;
        NiMatrix3 matrixMoonX;
        if (!spRoot.m_pObject->GetDisplayObject()) {
            spRoot.m_pObject->SetAppCulled(true);
            return;
        }
        float v12 = -fPos * 0.01745329238474369;
        matrixMoonX.MakeXRotation(v12);
        float v11 = fZOffset * 0.01745329238474369;
        matrixMoonZ.MakeZRotation(v11);
        spRoot.m_pObject->SetRotate(matrixMoonX * matrixMoonZ);
        if (!(fvisMoon <= 0.0) || !(fvisShadow <= 0.0)) {
            spRoot.m_pObject->SetAppCulled(false);
            if (!(fvisMoon <= 0.0) && spMoonMesh.m_pObject->GetDisplayObject()) {
                SkyShaderProperty* pshadeProperty;
                int32_t v10;
                spMoonNode.m_pObject->SetAppCulled(false);
                if (spMoonMesh.m_pObject->GetProperty(3)) {
                    NiProperty* Property = spMoonMesh.m_pObject->GetProperty(3);
                    v10 = static_cast<NiShadeProperty*>(Property)->iShaderPropertyType ==
                        BSShaderManager::BSSM_PROPERTYTYPE_SKY;
                }
                else {
                    v10 = 0;
                }
                if (v10) {
                    pshadeProperty = static_cast<SkyShaderProperty*>(spMoonMesh.m_pObject->GetProperty(3));
                }
                else {
                    pshadeProperty = nullptr;
                }
                if (pshadeProperty) {
                    pshadeProperty->VertexColor.a = fvisMoon;
                }
            }
            else {
                spMoonNode.m_pObject->SetAppCulled(true);
            }
            if (!(fvisShadow <= 0.0) && spShadowMesh.m_pObject->GetDisplayObject()) {
                SkyShaderProperty* pshadeProperty_2;
                int32_t bIsSkyShader;
                spShadowNode.m_pObject->SetAppCulled(false);
                if (spShadowMesh.m_pObject->GetProperty(3)) {
                    NiProperty* v5 = spShadowMesh.m_pObject->GetProperty(3);
                    bIsSkyShader = static_cast<NiShadeProperty*>(v5)->iShaderPropertyType ==
                        BSShaderManager::BSSM_PROPERTYTYPE_SKY;
                }
                else {
                    bIsSkyShader = 0;
                }
                if (bIsSkyShader) {
                    pshadeProperty_2 = static_cast<SkyShaderProperty*>(spShadowMesh.m_pObject->GetProperty(3));
                }
                else {
                    pshadeProperty_2 = nullptr;
                }
                if (pshadeProperty_2) {
                    NiColorA v14(
                        (*&apSky->SkyColor)[0].r, (*&apSky->SkyColor)[0].g, (*&apSky->SkyColor)[0].b, fvisShadow
                    );
                    NiColorA* v6 = &v14;
                    pshadeProperty_2->VertexColor = *v6;
                }
            }
            else {
                spShadowNode.m_pObject->SetAppCulled(true);
            }
        }
        else {
            spRoot.m_pObject->SetAppCulled(true);
        }
    }
}

void Moon::Initialize(NiNode* apSkyRoot, const char* apName) {
    Sky* v57;
    NiNode* v68;
    NiNode* v69;
    NiAlphaProperty* NiAlphaProperty_;
    NiTriShape* NiTriShape_;
    NiTriShape* v72;
    NiColorA* pColorsShadow;
    NiColorA* pColorsMoon;
    NiPoint2* pTexturesShadow;
    NiPoint2* pTexturesMoon;
    NiPoint3* pVerticesShadow;
    NiPoint3* pNormalsShadow;
    NiPoint3* pVerticesMoon;
    NiPoint3* pNormalsMoon;
    NiPoint2 v95;
    NiPoint2 v96;
    NiPoint2 v97;
    NiPoint2 v98;
    NiPoint2 v99;
    NiPoint2 v100;
    NiPoint2 v101;
    NiPoint2 v102;
    NiPoint3 v107;
    NiPoint3 v108;
    NiPoint3 v109;
    NiPoint3 v110;
    NiPoint3 v111;
    NiPoint3 v112;
    NiPoint3 v113;
    NiPoint3 dst;
    NiMatrix3 matrixMoon;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Moon.cpp", 102
    );
    SkyObject::Initialize(apSkyRoot);
    void* ptr = ourMalloc(4 * sizeof(NiPoint3));
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pNormalsMoon = static_cast<NiPoint3*>(ptr);
    }
    else {
        pNormalsMoon = nullptr;
    }
    void* v119 = ourMalloc(4 * sizeof(NiPoint3));
    if (v119) {
        rt::eh_vector_constructor_iterator(v119, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pVerticesMoon = static_cast<NiPoint3*>(v119);
    }
    else {
        pVerticesMoon = nullptr;
    }
    void* v117 = ourMalloc(4 * sizeof(NiPoint3));
    if (v117) {
        rt::eh_vector_constructor_iterator(v117, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pNormalsShadow = static_cast<NiPoint3*>(v117);
    }
    else {
        pNormalsShadow = nullptr;
    }
    void* v115 = ourMalloc(4 * sizeof(NiPoint3));
    if (v115) {
        rt::eh_vector_constructor_iterator(v115, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pVerticesShadow = static_cast<NiPoint3*>(v115);
    }
    else {
        pVerticesShadow = nullptr;
    }
    float a4 = static_cast<float>(iSize);
    float v82 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v3 = ::new (&dst) NiPoint3(v82, a4, 0.0f);
    *pVerticesMoon = *v3;
    float v81 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v4 = ::new (&v113) NiPoint3(v81, v81, 0.0f);
    pVerticesMoon[1] = *v4;
    float a4a = static_cast<float>(iSize);
    NiPoint3* v5 = ::new (&v112) NiPoint3(a4a, a4a, 0.0f);
    pVerticesMoon[2] = *v5;
    float v80 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v6 = ::new (&v111) NiPoint3(static_cast<float>(iSize), v80, 0.0f);
    pVerticesMoon[3] = *v6;
    float a4b = static_cast<float>(iSize);
    float v79 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v7 = ::new (&v110) NiPoint3(v79, a4b, 0.0f);
    *pVerticesShadow = *v7;
    float v78 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v8 = ::new (&v109) NiPoint3(v78, v78, 0.0f);
    pVerticesShadow[1] = *v8;
    float a4c = static_cast<float>(iSize);
    NiPoint3* v9 = ::new (&v108) NiPoint3(a4c, a4c, 0.0f);
    pVerticesShadow[2] = *v9;
    float v77 = static_cast<double>(iSize) * -1.0;
    NiPoint3* v10 = ::new (&v107) NiPoint3(static_cast<float>(iSize), v77, 0.0f);
    pVerticesShadow[3] = *v10;
    void* v105 = ourMalloc(4 * sizeof(NiPoint2));
    if (v105) {
        rt::eh_vector_constructor_iterator(v105, sizeof(NiPoint2), 4, &emptyPointConstructor);
        pTexturesMoon = static_cast<NiPoint2*>(v105);
    }
    else {
        pTexturesMoon = nullptr;
    }
    void* v103 = ourMalloc(4 * sizeof(NiPoint2));
    if (v103) {
        rt::eh_vector_constructor_iterator(v103, sizeof(NiPoint2), 4, &emptyPointConstructor);
        pTexturesShadow = static_cast<NiPoint2*>(v103);
    }
    else {
        pTexturesShadow = nullptr;
    }
    NiPoint2* v11 = ::new (&v102) NiPoint2(0.0f, 0.0f);
    float x = v11->x;
    float y = v11->y;
    pTexturesMoon->x = x;
    pTexturesMoon->y = y;
    NiPoint2* v15 = ::new (&v101) NiPoint2(0.0f, 1.0f);
    float v16 = v15->x;
    float v17 = v15->y;
    pTexturesMoon[1].x = v16;
    pTexturesMoon[1].y = v17;
    NiPoint2* v19 = ::new (&v100) NiPoint2(1.0f, 0.0f);
    float v20 = v19->x;
    float v21 = v19->y;
    pTexturesMoon[2].x = v20;
    pTexturesMoon[2].y = v21;
    NiPoint2* v23 = ::new (&v99) NiPoint2(1.0f, 1.0f);
    float v24 = v23->x;
    float v25 = v23->y;
    pTexturesMoon[3].x = v24;
    pTexturesMoon[3].y = v25;
    NiPoint2* v27 = ::new (&v98) NiPoint2(0.0f, 0.0f);
    float v28 = v27->x;
    float v29 = v27->y;
    pTexturesShadow->x = v28;
    pTexturesShadow->y = v29;
    NiPoint2* v31 = ::new (&v97) NiPoint2(0.0f, 1.0f);
    float v32 = v31->x;
    float v33 = v31->y;
    pTexturesShadow[1].x = v32;
    pTexturesShadow[1].y = v33;
    NiPoint2* v35 = ::new (&v96) NiPoint2(1.0f, 0.0f);
    float v36 = v35->x;
    float v37 = v35->y;
    pTexturesShadow[2].x = v36;
    pTexturesShadow[2].y = v37;
    NiPoint2* v39 = ::new (&v95) NiPoint2(1.0f, 1.0f);
    float v40 = v39->x;
    float v41 = v39->y;
    pTexturesShadow[3].x = v40;
    pTexturesShadow[3].y = v41;
    NiColorA* v94 = static_cast<NiColorA*>(ourMalloc(4 * sizeof(NiColorA)));
    if (v94) {
        ::new (static_cast<void*>(v94)) NiColorA[4];
        pColorsMoon = v94;
    }
    else {
        pColorsMoon = nullptr;
    }
    NiColorA* v93 = static_cast<NiColorA*>(ourMalloc(4 * sizeof(NiColorA)));
    if (v93) {
        ::new (static_cast<void*>(v93)) NiColorA[4];
        pColorsShadow = v93;
    }
    else {
        pColorsShadow = nullptr;
    }
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
    *sVertOrderMoon = 0;
    sVertOrderMoon[1] = 1;
    sVertOrderMoon[2] = 2;
    sVertOrderMoon[3] = 2;
    sVertOrderMoon[4] = 1;
    sVertOrderMoon[5] = 3;
    *sVertOrderShadow = 0;
    sVertOrderShadow[1] = 1;
    sVertOrderShadow[2] = 2;
    sVertOrderShadow[3] = 2;
    sVertOrderShadow[4] = 1;
    sVertOrderShadow[5] = 3;
    NiTriShape* v92 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v92) {
        v72 = ::new (v92) NiTriShape(
            4, pVerticesMoon, pNormalsMoon, pColorsMoon, pTexturesMoon, 1, NiGeometryData::NBT_METHOD_NONE, 2,
            sVertOrderMoon
        );
    }
    else {
        v72 = nullptr;
    }
    spMoonMesh.operator=(v72);
    spMoonMesh.m_pObject->SetSelectiveUpdate(true);
    NiTriShape* v91 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v91) {
        NiTriShape_ = ::new (v91) NiTriShape(
            4, pVerticesShadow, pNormalsShadow, pColorsShadow, pTexturesShadow, 1, NiGeometryData::NBT_METHOD_NONE, 2,
            sVertOrderShadow
        );
    }
    else {
        NiTriShape_ = nullptr;
    }
    spShadowMesh.operator=(NiTriShape_);
    spShadowMesh.m_pObject->SetSelectiveUpdate(true);
    NiAlphaProperty* v90 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v90) {
        NiAlphaProperty_ = ::new (v90) NiAlphaProperty;
    }
    else {
        NiAlphaProperty_ = nullptr;
    }
    {
        NiPointer<NiAlphaProperty> spAlpha(NiAlphaProperty_);
        spAlpha.m_pObject->SetAlphaBlending(true);
        spAlpha.m_pObject->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        spAlpha.m_pObject->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
        spAlpha.m_pObject->SetNoSorter(true);
        spMoonMesh.m_pObject->AttachProperty(spAlpha.m_pObject);
        spAlpha.operator=(static_cast<NiAlphaProperty*>(nullptr));
    }
    BSShaderData bsshaderdata;
    bsshaderdata.eLightingShader = BSShaderData::LS_SKY;
    bsshaderdata.iSkyTYpe = 6;
    bsshaderdata.Convert(spMoonMesh.m_pObject, 1u);
    bsshaderdata.iSkyTYpe = 7;
    bsshaderdata.Convert(spShadowMesh.m_pObject, 1u);
    Sky* Instance = Sky::GetInstance();
    if (Instance->eMode == Sky::SM_FULL || (v57 = Sky::GetInstance(), v57->eMode == Sky::SM_FAKEEXTERIOR)) {
        SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(spShadowMesh.m_pObject->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture("Textures\\Sky\\MoonShadow.dds", true, spTex, true, false);
        NiTexture* ptex = spTex.m_pObject;
        if (ptex) {
            pprop->SetTexture(static_cast<NiSourceTexture*>(ptex));
        }
        spShadowMesh.m_pObject->SetDisplayObject(ptex != nullptr);
        eUpdateMoonTexture = Moon::US_INITIALIZE;
    }
    NiNode* v89 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
    if (v89) {
        v69 = ::new (v89) NiNode(0);
    }
    else {
        v69 = nullptr;
    }
    spMoonNode.operator=(v69);
    NiNode* v88 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
    if (v88) {
        v68 = ::new (v88) NiNode(0);
    }
    else {
        v68 = nullptr;
    }
    spShadowNode.operator=(v68);
    spMoonNode.m_pObject->SetSelectiveUpdate(true);
    spShadowNode.m_pObject->SetSelectiveUpdate(true);
    NiNode* v67 = spMoonNode.m_pObject;
    v67->AttachChild(spMoonMesh.m_pObject, 1);
    NiNode* v66 = spShadowNode.m_pObject;
    v66->AttachChild(spShadowMesh.m_pObject, 1);
    NiNode* v65 = spRoot.m_pObject;
    v65->AttachChild(spShadowNode.m_pObject, 1);
    NiNode* v64 = spRoot.m_pObject;
    v64->AttachChild(spMoonNode.m_pObject, 1);
    matrixMoon.MakeXRotation(-1.5707964f);
    spMoonMesh.m_pObject->SetTranslate(0.0f, 512.0f, 0.0f);
    spMoonMesh.m_pObject->SetRotate(matrixMoon);
    spShadowMesh.m_pObject->SetTranslate(0.0f, 512.0f, 0.0f);
    spShadowMesh.m_pObject->SetRotate(matrixMoon);
    BSShaderManager::PrepareObject(spRoot.m_pObject, 0, 0);
}

void Moon::RemoveTextures() {
    NiTriShape* v1 = (&spMoonMesh)->m_pObject;
    NiProperty* pkObject = v1->GetProperty(3);
    SkyShaderProperty* v6 =
        static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pkObject));
    if (v6) {
        v6->SetTexture(nullptr);
        v6->SetTextureFile(EmptyFS);
    }
    NiTriShape* v2 = (&spShadowMesh)->m_pObject;
    NiProperty* pkObjecta = v2->GetProperty(3);
    SkyShaderProperty* v7 =
        static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pkObjecta));
    if (v7) {
        v7->SetTexture(nullptr);
        v7->SetTextureFile(EmptyFS);
    }
}

float Moon::GetMoonVisibility() const {
    float v4 = (float)(180.0 - fAngleFadeStart);
    float v5 = (float)(180.0 - fAngleFadeEnd);
    if (fPos > 180.0) {
        return 0.0;
    }
    if ((!(fAngleFadeStart >= ((double)fPos))) || (!(fAngleFadeEnd <= ((double)fPos)))) {
        if ((!(v4 <= ((double)fPos))) || (!(v5 >= ((double)fPos)))) {
            if ((!(fAngleFadeStart < ((double)fPos))) || (!(v4 > ((double)fPos)))) {
                return 0.0;
            }

            return 1.0;
        }

        return ((v5 - fPos) / (v5 - v4));
    }

    return ((fPos - fAngleFadeEnd) / (fAngleFadeStart - fAngleFadeEnd));
}

float Moon::GetMoonShadowVisibility() const {
    float fAngleFadeEnd;
    fAngleFadeEnd = this->fAngleFadeEnd;
    float v6 = fAngleFadeEnd - fAngleShadowEarlyFade;
    float v5 = (float)(180.0 - fAngleFadeEnd);
    float v7 = (float)(180.0 - v6);
    if (fPos > 180.0) {
        return 0.0;
    }
    if ((!(fAngleFadeEnd >= ((double)fPos))) || (!(v6 <= ((double)fPos)))) {
        if ((!(v5 <= ((double)fPos))) || (!(v7 >= ((double)fPos)))) {
            if ((!(fAngleFadeEnd < ((double)fPos))) || (!(v5 > ((double)fPos)))) {
                return 0.0;
            }

            return 1.0;
        }

        return ((v7 - fPos) / (v7 - v5));
    }

    return ((fPos - v6) / (fAngleFadeEnd - v6));
}

// The phase is (days passed mod 8 phase lengths) / phase length; each operand is fetched again, as the game does.
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

uint32_t NiAVObject::GetDisplayObject() {
    return GetFlag(NiAVObject::DISPLAY_OBJECT);
}
