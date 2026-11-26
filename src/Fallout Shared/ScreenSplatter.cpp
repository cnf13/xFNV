#include "Fallout Shared/ScreenSplatter.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderBloodSplatterProperty.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BSSplatterData.h"
#include "Fallout Shared/BSSplatterExtraData.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

NiRTTI BSSplatterExtraData::ms_RTTI("BSSplatterExtraData", &NiExtraData::ms_RTTI);
SettingT<INISettingCollection> bBloodSplatterEnabled("bBloodSplatterEnabled:ScreenSplatter", true);
NiPointer<NiCamera> ScreenSplatter::spCamera;
NiPointer<NiNode> ScreenSplatter::spRoot;
ScreenSplatter::spTextures_t ScreenSplatter::spTextures;

void ScreenSplatter::Activate(uint32_t auiCount, float afDuration, ScreenSplatterType aeType) {
    BSSplatterData splatterData;
    if (ScreenSplatter::bEnabled) {
        uint32_t uicount = BSMIN(auiCount, iBloodSplatterMaxCount.Int() - ScreenSplatter::uiCount);
        if (uicount) {
            NiNode* pnode = ScreenSplatter::CreateGeometry(uicount);
            ScreenSplatter::uiCount += uicount;
            splatterData.fAlpha = 1.0f;
            splatterData.fAge = 0.0;
            if (afDuration > 0.0) {
                splatterData.fDuration = afDuration;
            }
            else {
                splatterData.fDuration = fBloodSplatterDuration.Float();
            }
            splatterData.uiCount = uicount;
            NiExtraData* ped = new BSSplatterExtraData(splatterData);
            BSSplatterExtraData* pSplatterExtraData = static_cast<BSSplatterExtraData*>(ped);
            pnode->AddExtraData(ped);
            pnode->SetName("ScreenSplatterRoot");
            ScreenSplatter::spRoot->AttachChild(pnode, 1);
            BSShaderBloodSplatterProperty::SetFadeSourceRecurse(pnode, &pSplatterExtraData->SplatterData.fAlpha);
            NiUpdateData UpdateData(0.0f, false, false);
            ScreenSplatter::spRoot->Update(UpdateData);
        }
    }
}

void ScreenSplatter::Clear() {
    if (ScreenSplatter::bEnabled) {
        ScreenSplatter::spRoot->RemoveAllChildren();
        ScreenSplatter::uiCount = 0;
    }
}

bool ScreenSplatter::IsActiveSplatter() {
    if (!ScreenSplatter::spRoot) {
        return false;
    }
    NiNode* pRoot = ScreenSplatter::spRoot;
    return pRoot->GetChildCount() != 0;
}

NiCamera* ScreenSplatter::GetCamera() {
    return ScreenSplatter::spCamera;
}

NiNode* ScreenSplatter::GetRoot() {
    return ScreenSplatter::spRoot;
}

void ScreenSplatter::Update() {
    if (ScreenSplatter::bEnabled) {
        uint32_t uiChildCount = ScreenSplatter::spRoot->GetArrayCount();
        float fElapsedTime = appTimer.fDelta;
        for (uint32_t ui = 0; ui < uiChildCount; ++ui) {
            NiAVObject* pChild = ScreenSplatter::spRoot->GetAt(ui);
            if (pChild) {
                NiFixedString* pTagName = BSSplatterExtraData::GetTag();
                BSSplatterExtraData* pExtraData = static_cast<BSSplatterExtraData*>(pChild->GetExtraData(*pTagName));
                BSSplatterData* pSplatterData = &pExtraData->SplatterData;
                pSplatterData->fAge = pSplatterData->fAge + fElapsedTime;
                if (pSplatterData->fDuration <= static_cast<double>(pSplatterData->fAge)) {
                    ScreenSplatter::uiCount -= pSplatterData->uiCount;
                    ScreenSplatter::spRoot->DetachChildAt(ui);
                }
                else {
                    double dAgeRatio = pSplatterData->fAge / pSplatterData->fDuration;
                    if (fBloodSplatterFadeStart.Float() <= dAgeRatio) {
                        float fAge = pSplatterData->fAge;
                        float fDuration = pSplatterData->fDuration;
                        float fFadeStart = fBloodSplatterFadeStart.Float() * pSplatterData->fDuration;
                        double dFade = FLerp(0.0f, 1.0f, fFadeStart, fDuration, fAge);
                        pSplatterData->fAlpha = 1.0 - dFade;
                    }
                    NiUpdateData UpdateData(pSplatterData->fAge, true, false);
                    pChild->Update(UpdateData);
                }
            }
        }
        BSShaderBloodSplatter::SetFlareOffsetScale(fBloodSplatterFlareOffsetScale.Float());
        BSShaderBloodSplatter::SetFlareMult(fBloodSplatterFlareMult.Float());
        float fGlobalAlpha = BSShaderBloodSplatter::GetGlobalAlpha();
        if (fGlobalAlpha < 1.0) {
            fGlobalAlpha = fElapsedTime / 0.4 + fGlobalAlpha;
            fGlobalAlpha = BSMIN(1.0f, fGlobalAlpha);
            BSShaderBloodSplatter::SetGlobalAlpha(fGlobalAlpha);
        }
    }
}

void ScreenSplatter::Init() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ScreenSplatter.cpp",
        181
    );
    ScreenSplatter::bEnabled = bBloodSplatterEnabled.Bool();
    // Tests the whole byte of bEnabled, not its low bit.
    if (static_cast<uint8_t>(ScreenSplatter::bEnabled)) {
        NiRenderer* pRenderer = NiRenderer::GetRenderer();
        double dScreenWidth = static_cast<double>(pRenderer->GetScreenWidth());
        float fAspectRatio = dScreenWidth / static_cast<double>(pRenderer->GetScreenHeight());
        NiCamera* pCamera = new NiCamera;
        ScreenSplatter::spCamera = pCamera;
        NiPoint3 kTranslate(0.0f, 0.0f, 1.00001f);
        ScreenSplatter::spCamera->SetTranslate(kTranslate);
        NiUpdateData UpdateData1(0.0f, false, false);
        ScreenSplatter::spCamera->Update(UpdateData1);
        ScreenSplatter::spCamera->LookAtWorldPoint(NiPoint3::ZERO, NiPoint3::UNIT_Y);
        NiFrustum frustum(false);
        frustum.m_bOrtho = false;
        frustum.m_fTop = 0.5f;
        frustum.m_fBottom = -0.5f;
        frustum.m_fLeft = fAspectRatio * -0.5;
        frustum.m_fRight = fAspectRatio * 0.5;
        frustum.m_fFar = 10000.0f;
        frustum.m_fNear = 1.0f;
        ScreenSplatter::spCamera->SetViewFrustum(frustum);
        NiUpdateData UpdateData2(0.0f, false, false);
        ScreenSplatter::spCamera->Update(UpdateData2);
        ScreenSplatter::fWRadius = frustum.m_fRight;
        ScreenSplatter::fHRadius = frustum.m_fTop;
        NiNode* pRoot = new NiNode(0);
        ScreenSplatter::spRoot = pRoot;
        ScreenSplatter::uiCount = 0;
        const char* pcFilename = sBloodSplatterAlpha01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(&ScreenSplatter::spTextures[1], pcFilename);
        pcFilename = sBloodSplatterColor01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(ScreenSplatter::spTextures, pcFilename);
        if (BSShaderManager::bHDR) {
            pcFilename = sBloodSplatterFlare01Filename.String();
            SkyTextureManager::LoadTextureInBackground(&ScreenSplatter::spTextures[2], pcFilename);
        }
    }
}

void ScreenSplatter::Shutdown() {
    if (ScreenSplatter::bEnabled) {
        ScreenSplatter::spRoot->RemoveAllChildren();
        ScreenSplatter::spRoot = static_cast<NiNode*>(nullptr);
        ScreenSplatter::spCamera = static_cast<NiCamera*>(nullptr);
    }
    ScreenSplatter::bEnabled = false;
    for (NiPointer<NiTexture>& spTexture : ScreenSplatter::spTextures) {
        spTexture = static_cast<NiTexture*>(nullptr);
    }
}

NiNode* ScreenSplatter::CreateGeometry(uint32_t auiCount) {
    NiNode* pNode = new NiNode(0);
    NiPoint3* pVerticesA = new NiPoint3[4 * auiCount];
    NiPoint2* pTexcoordsA = new NiPoint2[4 * auiCount];
    uint16_t* pIndicesA = static_cast<uint16_t*>(
        ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
    );
    NiPoint3* pFlareVerticesA = nullptr;
    NiPoint2* pFlareTexcoordsA = nullptr;
    uint16_t* pFlareIndicesA = nullptr;
    if (BSShaderManager::bHDR) {
        pFlareVerticesA = new NiPoint3[4 * auiCount];
        pFlareTexcoordsA = new NiPoint2[4 * auiCount];
        pFlareIndicesA = static_cast<uint16_t*>(
            ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
        );
    }
    for (uint32_t i = 0; i < auiCount; ++i) {
        NiMatrix3 rotationMatrix;
        float fx = RandomFloatNeg1To1() * ScreenSplatter::fWRadius;
        float fy = RandomFloatNeg1To1() * ScreenSplatter::fHRadius;
        NiPoint3 translate(fx, fy, 0.0f);
        float fMaxSize = fBloodSplatterMaxSize.Float();
        float fScale = RandomFloat(fBloodSplatterMinSize.Float(), fMaxSize);
        float fMaxOpacity = fBloodSplatterMaxOpacity.Float();
        float fAlpha1 = RandomFloat(fBloodSplatterMinOpacity.Float(), fMaxOpacity);
        float fMaxOpacity2 = fBloodSplatterMaxOpacity2.Float();
        float fAlpha2 = RandomFloat(fBloodSplatterMinOpacity2.Float(), fMaxOpacity2);
        rotationMatrix.MakeIdentity();
        if (RandomBool(fBloodSplatterOpacityChance.Float())) {
            fAlpha1 = fAlpha2;
        }
        double dUOffset;
        if (RandomFloatNeg1To1() > 0.0) {
            dUOffset = 0.0;
        }
        else {
            dUOffset = 0.5;
        }
        float fU = dUOffset;
        double dVOffset;
        if (RandomFloatNeg1To1() > 0.0) {
            dVOffset = 0.0;
        }
        else {
            dVOffset = 0.5;
        }
        float fV = dVOffset;
        NiPoint2 texTranslate(fU, fV);
        float fFlareSize = fBloodSplatterFlareSize.Float();
        uint32_t uiBase = 4 * i;
        pVerticesA[uiBase] = rotationMatrix * NiPoint3(-fScale, fScale, fAlpha1) + translate;
        pVerticesA[uiBase + 1] = rotationMatrix * NiPoint3(fScale, fScale, fAlpha1) + translate;
        float fY = -fScale;
        pVerticesA[uiBase + 2] = rotationMatrix * NiPoint3(fY, fY, fAlpha1) + translate;
        pVerticesA[uiBase + 3] = rotationMatrix * NiPoint3(fScale, -fScale, fAlpha1) + translate;
        if (BSShaderManager::bHDR) {
            pFlareVerticesA[uiBase] = rotationMatrix * NiPoint3(-fFlareSize, fFlareSize, fAlpha1) + translate;
            pFlareVerticesA[uiBase + 1] = rotationMatrix * NiPoint3(fFlareSize, fFlareSize, fAlpha1) + translate;
            float fFlareY = -fFlareSize;
            pFlareVerticesA[uiBase + 2] = rotationMatrix * NiPoint3(fFlareY, fFlareY, fAlpha1) + translate;
            pFlareVerticesA[uiBase + 3] = rotationMatrix * NiPoint3(fFlareSize, -fFlareSize, fAlpha1) + translate;
        }
        NiPoint2 kTexCoord = NiPoint2(0.5f, 0.5f) + texTranslate;
        pTexcoordsA[uiBase].x = kTexCoord.x;
        pTexcoordsA[uiBase].y = kTexCoord.y;
        kTexCoord = NiPoint2(0.0f, 0.5f) + texTranslate;
        pTexcoordsA[uiBase + 1].x = kTexCoord.x;
        pTexcoordsA[uiBase + 1].y = kTexCoord.y;
        kTexCoord = NiPoint2(0.5f, 0.0f) + texTranslate;
        pTexcoordsA[uiBase + 2].x = kTexCoord.x;
        pTexcoordsA[uiBase + 2].y = kTexCoord.y;
        kTexCoord = NiPoint2(0.0f, 0.0f) + texTranslate;
        pTexcoordsA[uiBase + 3].x = kTexCoord.x;
        pTexcoordsA[uiBase + 3].y = kTexCoord.y;
        if (BSShaderManager::bHDR) {
            kTexCoord = NiPoint2(1.0f, 1.0f);
            pFlareTexcoordsA[uiBase].x = kTexCoord.x;
            pFlareTexcoordsA[uiBase].y = kTexCoord.y;
            kTexCoord = NiPoint2(0.0f, 1.0f);
            pFlareTexcoordsA[uiBase + 1].x = kTexCoord.x;
            pFlareTexcoordsA[uiBase + 1].y = kTexCoord.y;
            kTexCoord = NiPoint2(1.0f, 0.0f);
            pFlareTexcoordsA[uiBase + 2].x = kTexCoord.x;
            pFlareTexcoordsA[uiBase + 2].y = kTexCoord.y;
            kTexCoord = NiPoint2(0.0f, 0.0f);
            pFlareTexcoordsA[uiBase + 3].x = kTexCoord.x;
            pFlareTexcoordsA[uiBase + 3].y = kTexCoord.y;
        }
        uint32_t uiIndexBase = 6 * i;
        pIndicesA[6 * i] = uiBase;
        pIndicesA[uiIndexBase + 1] = uiBase + 2;
        pIndicesA[uiIndexBase + 2] = uiBase + 1;
        pIndicesA[uiIndexBase + 3] = uiBase + 1;
        pIndicesA[uiIndexBase + 4] = uiBase + 2;
        pIndicesA[uiIndexBase + 5] = uiBase + 3;
        if (BSShaderManager::bHDR) {
            pFlareIndicesA[uiIndexBase] = uiBase;
            pFlareIndicesA[uiIndexBase + 1] = uiBase + 2;
            pFlareIndicesA[uiIndexBase + 2] = uiBase + 1;
            pFlareIndicesA[uiIndexBase + 3] = uiBase + 1;
            pFlareIndicesA[uiIndexBase + 4] = uiBase + 2;
            pFlareIndicesA[uiIndexBase + 5] = uiBase + 3;
        }
    }
    NiTriShape* pGeom = new NiTriShape(
        4 * auiCount,
        pVerticesA,
        nullptr,
        nullptr,
        pTexcoordsA,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2 * auiCount,
        pIndicesA
    );
    BSShaderBloodSplatterProperty* pShaderProp = new BSShaderBloodSplatterProperty;
    pShaderProp->SetTexture(0, ScreenSplatter::spTextures[1].m_pObject);
    pShaderProp->SetTexture(1u, ScreenSplatter::spTextures->m_pObject);
    pGeom->AttachProperty(pShaderProp);
    BSShaderManager::PrepareObject(pGeom, 0, 0);
    pNode->AttachChild(pGeom, 1);
    if (BSShaderManager::bHDR) {
        pGeom = new NiTriShape(
            4 * auiCount,
            pFlareVerticesA,
            nullptr,
            nullptr,
            pFlareTexcoordsA,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            2 * auiCount,
            pFlareIndicesA
        );
        pShaderProp = new BSShaderBloodSplatterProperty;
        pShaderProp->SetTexture(0, ScreenSplatter::spTextures[2].m_pObject);
        pGeom->AttachProperty(pShaderProp);
        BSShaderManager::PrepareObject(pGeom, 0, 0);
        pNode->AttachChild(pGeom, 1);
    }
    return pNode;
}

BSSplatterExtraData::BSSplatterExtraData(const BSSplatterData& aSplatterData)
  : NiExtraData(BSSplatterExtraData::GetTag()), SplatterData(aSplatterData) {}

BSSplatterExtraData::~BSSplatterExtraData() {}

NiFixedString* BSSplatterExtraData::GetTag() {
    return BSSplatterExtraData::pTag;
}

void BSSplatterExtraData::InitTag() {
    NiFixedString* pTagString;
    pTagString = new NiFixedString(const_cast<char*>("SPD"));
    BSSplatterExtraData::pTag = pTagString;
}

void BSSplatterExtraData::KillSDM() {
    if (BSSplatterExtraData::pTag) {
        delete BSSplatterExtraData::pTag;
    }
    BSSplatterExtraData::pTag = nullptr;
}
