#include "Fallout/Interface/FaderManager.h"

#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

FaderDataArray FaderData =
    {FADER_DATA(true, "Black.dds"), FADER_DATA(false, "Black.dds"), FADER_DATA(false, "White.dds")};

FaderManager::FaderManager() : pRootBelowMenus(nullptr), pRootAboveMenus(nullptr) {
    for (FADER_DATA& rFader : FaderData) {
        rFader.bFadingIn = false;
        rFader.pRoot = nullptr;
        rFader.fAlpha = 0.0f;
        rFader.spTileShaderProp = nullptr;
    }
}

FaderManager::~FaderManager() {
    for (FADER_DATA& rFader : FaderData) {
        rFader.bFadingIn = false;
        rFader.pRoot = nullptr;
        rFader.fAlpha = 0.0f;
        rFader.spTileShaderProp = nullptr;
    }
}

void FaderManager::AddRoot(NiNode* apNode, float afOneToOneDistance, bool abBelowMenus) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp",
        80
    );
    NiNode* pNode = new NiNode(0);
    if (abBelowMenus) {
        pRootBelowMenus = pNode;
        NiFixedString kName("FaderNode Below Menus");
        pNode->SetName(kName);
    }
    else {
        pRootAboveMenus = pNode;
        NiFixedString kName("FaderNode Above Menus");
        pNode->SetName(kName);
        afOneToOneDistance = afOneToOneDistance - 20.0;
    }
    NiDirectionalLight* pLight = new NiDirectionalLight;
    NiFixedString kLightName("FaderNodeLight");
    pLight->SetName(kLightName);
    pLight->SetAmbientColor(NiColor(1.0f, 1.0f, 1.0f));
    pNode->AttachChild(pLight, true);
    float fWidth = static_cast<float>(Renderer::GetScreenWidth());
    float fHeight = static_cast<float>(Renderer::GetScreenHeight());
    float fScale = 1.0f;
    if (fHeight / fWidth != 0.75) {
        fScale = 1.3333334f;
    }
    if (fWidth != 1280.0) {
        fScale = 1280.0 / fWidth * fScale;
    }
    if (Renderer::is_bEyeFinity()) {
        fScale = 1.0f;
    }
    pNode->SetScale(fScale);
    NiPoint3 kTranslate(0.0f, afOneToOneDistance, 0.0f);
    pNode->SetTranslate(kTranslate);
    apNode->AttachChild(pNode, true);
}

void FaderManager::CreateFader(FADER_ENUM aeFader, float afFadeSpeed, bool abBeginBlack) {
    TileShaderProperty* ptsp;
    NiTriShape* ptri;
    NiAlphaProperty* palphaProp;
    NiNode* faderNode;
    NiPoint2 kUV3;
    NiPoint2 kUV2;
    NiPoint2 kUV1;
    NiPoint2 kUV0;
    NiPoint3 kCorner3;
    NiPoint3 kCorner2;
    NiPoint3 kCorner1;
    NiPoint3 kCorner0;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp",
        135
    );
    if (FaderData[aeFader].pRoot) {
        if (FaderData[aeFader].bFadingIn) {
            return;
        }
        RemoveFader(aeFader, true);
    }
    NiPoint3* pvertices = new NiPoint3[4];
    NiPoint2* ptextures = new NiPoint2[4];
    uint16_t* pvertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    float fhalfScreenWidth = static_cast<double>(Renderer::GetScreenWidth()) * 0.5;
    float fhalfScreenHeight = static_cast<double>(Renderer::GetScreenHeight()) * 0.5;
    pvertices[0] = *::new (&kCorner0) NiPoint3(-fhalfScreenWidth, 0.0f, fhalfScreenHeight);
    pvertices[1] = *::new (&kCorner1) NiPoint3(-fhalfScreenWidth, 0.0f, -fhalfScreenHeight);
    pvertices[2] = *::new (&kCorner2) NiPoint3(fhalfScreenWidth, 0.0f, fhalfScreenHeight);
    pvertices[3] = *::new (&kCorner3) NiPoint3(fhalfScreenWidth, 0.0f, -fhalfScreenHeight);
    NiPoint2* puv = ::new (&kUV0) NiPoint2(0.0f, 0.0f);
    float x = puv->x;
    float y = puv->y;
    ptextures[0].x = x;
    ptextures[0].y = y;
    puv = ::new (&kUV1) NiPoint2(0.0f, 1.0f);
    x = puv->x;
    y = puv->y;
    ptextures[1].x = x;
    ptextures[1].y = y;
    puv = ::new (&kUV2) NiPoint2(1.0f, 0.0f);
    x = puv->x;
    y = puv->y;
    ptextures[2].x = x;
    ptextures[2].y = y;
    puv = ::new (&kUV3) NiPoint2(1.0f, 1.0f);
    x = puv->x;
    y = puv->y;
    ptextures[3].x = x;
    ptextures[3].y = y;
    pvertOrder[0] = 0;
    pvertOrder[1] = 1;
    pvertOrder[2] = 2;
    pvertOrder[3] = 2;
    pvertOrder[4] = 1;
    pvertOrder[5] = 3;
    faderNode = new NiNode(0);
    FaderData[aeFader].pRoot = faderNode;
    FaderData[aeFader].pRoot->SetName(FaderData[aeFader].pFaderTextureFilename);
    palphaProp = new NiAlphaProperty;
    palphaProp->SetAlphaBlending(true);
    FaderData[aeFader].pRoot->AttachProperty(palphaProp);
    ptri = new NiTriShape(4, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2, pvertOrder);
    FaderData[aeFader].pRoot->AttachChild(ptri, 1);
    BSStringT<char> textureFilename;
    textureFilename.SPrintF("Data\\Textures\\Interface\\Faders\\%s", FaderData[aeFader].pFaderTextureFilename);
    ptri->SetName(static_cast<const char*>(textureFilename.pString));
    NiPointer<NiSourceTexture> ptexture(
        NiSourceTexture::Create(
            static_cast<const char*>(textureFilename.pString),
            NiSourceTexture::ms_kDefaultPrefs,
            true,
            false
        )
    );
    float falpha = 0.0f;
    if (abBeginBlack || afFadeSpeed == 0.0) {
        falpha = 1.0f;
        cFaderManagerSkipFadeOutFrames = 3;
    }
    ptsp = new TileShaderProperty(false);
    ptsp->SetTileTexture(ptexture.m_pObject);
    ptsp->TexClampMode = NiTexturingProperty::WRAP_S_WRAP_T;
    ptsp->SetTileAlpha(falpha);
    ptri->AttachProperty(ptsp);
    FaderData[aeFader].spTileShaderProp = ptsp;
    BSShaderManager::PrepareObject(ptri, 0, 0);
    NiBound* pModelBound = &ptri->GetModelBound();
    pModelBound->ComputeFromData(4, pvertices);
    NiNode* pparent = pRootAboveMenus;
    if (FaderData[aeFader].bBelowMenus) {
        pparent = pRootBelowMenus;
    }
    pparent->AttachChild(FaderData[aeFader].pRoot, 1);
    pparent->UpdateProperties();
    NiUpdateData UpdateData(0.0f, false, false);
    pparent->Update(UpdateData);
    FaderData[aeFader].fFadeSpeed = afFadeSpeed;
    FaderData[aeFader].bFadingIn = !abBeginBlack;
    FaderData[aeFader].fAlpha = falpha;
}

void FaderManager::UpdateFaders(bool abUpdateLoading) {
    alignas(AutoMemContext) uint8_t kMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (kMemContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp",
        266
    );
    for (FADER_DATA& rFader : FaderData) {
        FADER_DATA* pFader = &rFader;
        if (!pFader->pRoot) {
            continue;
        }
        if (pFader->bFadingIn) {
            if (pFader->fAlpha < 1.0) {
                pFader->fAlpha = appTimer.GetRealTimeSeconds() / pFader->fFadeSpeed + pFader->fAlpha;
                if (pFader->fAlpha > 1.0) {
                    pFader->fAlpha = 1.0f;
                }
                if (pFader->spTileShaderProp) {
                    float fAlpha = pFader->fAlpha;
                    pFader->spTileShaderProp->SetTileAlpha(fAlpha);
                }
            }
        }
        else if (!(pFader->fAlpha <= 0.0)) {
            // Steps when the alpha is above zero or unordered (NaN).
            if (cFaderManagerSkipFadeOutFrames) {
                --cFaderManagerSkipFadeOutFrames;
            }
            else {
                pFader->fAlpha = pFader->fAlpha - appTimer.GetRealTimeSeconds() / pFader->fFadeSpeed;
            }
            if (pFader->fAlpha < 0.0) {
                pFader->fAlpha = 0.0f;
            }
            if (pFader->spTileShaderProp) {
                float fAlpha = pFader->fAlpha;
                pFader->spTileShaderProp->SetTileAlpha(fAlpha);
            }
        }
        else {
            if (pFader->pRoot && pFader->pRoot->GetParent()) {
                NiNode* pParent = pFader->pRoot->GetParent();
                pParent->DetachChild(pFader->pRoot);
            }
            pFader->pRoot = nullptr;
            pFader->fAlpha = 0.0f;
        }
    }
    pMemContext->~AutoMemContext();
}

bool FaderManager::IsFaderVisible(FADER_ENUM aeFader) {
    FADER_DATA* pFader = &FaderData[aeFader];
    return pFader->pRoot && pFader->bFadingIn && pFader->fAlpha == 1.0;
}

float FaderManager::GetFaderAlpha(FADER_ENUM aeFader) {
    return FaderData[aeFader].fAlpha;
}

// bFadingIn is read as a raw byte and any nonzero value counts as true; a bool read would hand back the byte itself, so
// it is read as uint8_t.
bool FaderManager::IsFaderActive(FADER_ENUM aeFader) {
    return FaderData[aeFader].pRoot && static_cast<uint8_t>(FaderData[aeFader].bFadingIn) != 0;
}

void FaderManager::RemoveFader(FADER_ENUM aeFader, bool abForce) {
    if (FaderData[aeFader].pRoot) {
        if (abForce || bClosingAllSupportMenus) {
            NiNode* pParent = FaderData[aeFader].pRoot->m_pkParent;
            pParent->DetachChild(FaderData[aeFader].pRoot);
            FaderData[aeFader].pRoot = nullptr;
            FaderData[aeFader].fAlpha = 0.0f;
            FaderData[aeFader].spTileShaderProp = static_cast<TileShaderProperty*>(nullptr);
            TES::pInstance->ShowLoadingMenu(false, nullptr, false);
        }
        else {
            FaderData[aeFader].bFadingIn = false;
            if (FaderData[aeFader].fAlpha == 1.0) {
                FaderData[aeFader].fAlpha = 0.9999f;
            }
        }
    }
}

bool FaderManager::IsFadingIn(FADER_ENUM aeFader) {
    FADER_DATA* pFader = &FaderData[aeFader];
    return pFader->pRoot && pFader->bFadingIn && pFader->fAlpha < 1.0;
}

void FaderManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}
