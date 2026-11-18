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
#include "SDK/MSVC/ehvec.h"

FaderDataArray FaderData = {
    FADER_DATA(true, "Black.dds"), FADER_DATA(false, "Black.dds"), FADER_DATA(false, "White.dds")
};

FaderManager::FaderManager() : pRootBelowMenus(nullptr), pRootAboveMenus(nullptr) {
    for (int32_t n = 0; n < 3; ++n) {
        FaderData[n].bFadingIn = false;
        FaderData[n].pRoot = nullptr;
        FaderData[n].fAlpha = 0.0f;
        FaderData[n].spTileShaderProp = nullptr;
    }
}

FaderManager::~FaderManager() {
    for (int32_t n = 0; n < 3; ++n) {
        FaderData[n].bFadingIn = false;
        FaderData[n].pRoot = nullptr;
        FaderData[n].fAlpha = 0.0f;
        FaderData[n].spTileShaderProp = nullptr;
    }
}

void FaderManager::AddRoot(NiNode* apNode, float afOneToOneDistance, bool abBelowMenus) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp", 80
    );
    void* pNodeMemory = NiMemObject::operator new(sizeof(NiNode));
    NiNode* pNode = pNodeMemory ? ::new (pNodeMemory) NiNode(0) : nullptr;
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
    void* pLightMemory = NiMemObject::operator new(sizeof(NiDirectionalLight));
    NiDirectionalLight* pLight = pLightMemory ? ::new (pLightMemory) NiDirectionalLight : nullptr;
    NiFixedString kLightName("FaderNodeLight");
    pLight->SetName(kLightName);
    NiColor kAmbient;
    pLight->SetAmbientColor(*XFNV_RECONSTRUCT(NiColor)(&kAmbient, 1.0f, 1.0f, 1.0f));
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
    NiPoint2* ptextures;
    NiPoint3* pvertices;
    XFNV_INLINED_LOCAL(NiFixedString, v42, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v43, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiPoint2 v51;
    NiPoint2 v52;
    NiPoint2 v53;
    NiPoint2 v54;
    NiPoint3 v55;
    NiPoint3 v56;
    NiPoint3 v57;
    NiPoint3 dst;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp", 135
    );
    if (FaderData[aeFader].pRoot) {
        if (FaderData[aeFader].bFadingIn) {
            return;
        }
        RemoveFader(aeFader, true);
    }
    void* ptr = ourMalloc(4 * sizeof(NiPoint3));
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pvertices = static_cast<NiPoint3*>(ptr);
    }
    else {
        pvertices = nullptr;
    }
    void* v59 = ourMalloc(4 * sizeof(NiPoint2));
    if (v59) {
        rt::eh_vector_constructor_iterator(v59, sizeof(NiPoint2), 4, &emptyPointConstructor);
        ptextures = static_cast<NiPoint2*>(v59);
    }
    else {
        ptextures = nullptr;
    }
    uint16_t* pvertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    float fhalfScreenWidth = static_cast<double>(Renderer::GetScreenWidth()) * 0.5;
    float fhalfScreenHeight = static_cast<double>(Renderer::GetScreenHeight()) * 0.5;
    NiPoint3* v4 = ::new (&dst) NiPoint3(-fhalfScreenWidth, 0.0f, fhalfScreenHeight);
    *pvertices = *v4;
    NiPoint3* v5 = ::new (&v57) NiPoint3(-fhalfScreenWidth, 0.0f, -fhalfScreenHeight);
    pvertices[1] = *v5;
    NiPoint3* v6 = ::new (&v56) NiPoint3(fhalfScreenWidth, 0.0f, fhalfScreenHeight);
    pvertices[2] = *v6;
    NiPoint3* v7 = ::new (&v55) NiPoint3(fhalfScreenWidth, 0.0f, -fhalfScreenHeight);
    pvertices[3] = *v7;
    NiPoint2* v8 = ::new (&v54) NiPoint2(0.0f, 0.0f);
    float x = v8->x;
    float y = v8->y;
    ptextures->x = x;
    ptextures->y = y;
    NiPoint2* v12 = ::new (&v53) NiPoint2(0.0f, 1.0f);
    float v13 = v12->x;
    float v14 = v12->y;
    ptextures[1].x = v13;
    ptextures[1].y = v14;
    NiPoint2* v16 = ::new (&v52) NiPoint2(1.0f, 0.0f);
    float v17 = v16->x;
    float v18 = v16->y;
    ptextures[2].x = v17;
    ptextures[2].y = v18;
    NiPoint2* v20 = ::new (&v51) NiPoint2(1.0f, 1.0f);
    float v21 = v20->x;
    float v22 = v20->y;
    ptextures[3].x = v21;
    ptextures[3].y = v22;
    *pvertOrder = 0;
    pvertOrder[1] = 1;
    pvertOrder[2] = 2;
    pvertOrder[3] = 2;
    pvertOrder[4] = 1;
    pvertOrder[5] = 3;
    NiNode* v49 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
    if (v49) {
        faderNode = ::new (v49) NiNode(0);
    }
    else {
        faderNode = nullptr;
    }
    FaderData[aeFader].pRoot = faderNode;
    NiFixedString* kName = ::new (static_cast<void*>(&apString))
        NiFixedString(static_cast<const char*>(FaderData[aeFader].pFaderTextureFilename));
    FaderData[aeFader].pRoot->SetName(*kName);
    apString.~NiFixedString();
    NiAlphaProperty* v46 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v46) {
        palphaProp = ::new (v46) NiAlphaProperty;
    }
    else {
        palphaProp = nullptr;
    }
    palphaProp->SetAlphaBlending(true);
    FaderData[aeFader].pRoot->AttachProperty(palphaProp);
    NiTriShape* v44 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v44) {
        ptri = ::new (v44)
            NiTriShape(4, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2, pvertOrder);
    }
    else {
        ptri = nullptr;
    }
    FaderData[aeFader].pRoot->AttachChild(ptri, 1);
    BSStringT<char> textureFilename;
    textureFilename.SPrintF("Data\\Textures\\Interface\\Faders\\%s", FaderData[aeFader].pFaderTextureFilename);
    NiFixedString* v33 =
        ::new (static_cast<void*>(&v43)) NiFixedString(static_cast<const char*>(textureFilename.pString));
    ptri->SetName(*v33);
    v43.~NiFixedString();
    NiFixedString* kFilename =
        ::new (static_cast<void*>(&v42)) NiFixedString(static_cast<const char*>(textureFilename.pString));
    NiPointer<NiSourceTexture> ptexture(
        NiSourceTexture::Create(*kFilename, NiSourceTexture::ms_kDefaultPrefs, true, false)
    );
    v42.~NiFixedString();
    float falpha = 0.0f;
    if (abBeginBlack || afFadeSpeed == 0.0) {
        falpha = 1.0f;
        cFaderManagerSkipFadeOutFrames = 3;
    }
    TileShaderProperty* v41 = static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
    if (v41) {
        ptsp = ::new (v41) TileShaderProperty(false);
    }
    else {
        ptsp = nullptr;
    }
    NiSourceTexture* negativeHeightb = ptexture.m_pObject;
    ptsp->SetTileTexture(negativeHeightb);
    ptsp->TexClampMode = NiTexturingProperty::WRAP_S_WRAP_T;
    ptsp->SetTileAlpha(falpha);
    ptri->AttachProperty(ptsp);
    FaderData[aeFader].spTileShaderProp.operator=(ptsp);
    BSShaderManager::PrepareObject(ptri, 0, 0);
    NiBound* ModelBound = &ptri->GetModelBound();
    ModelBound->ComputeFromData(4, pvertices);
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
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\FaderManager.cpp", 266
    );
    for (int32_t i = 0; i < FADER_COUNT; ++i) {
        FADER_DATA* pFader = &FaderData[i];
        if (!pFader->pRoot) {
            continue;
        }
        if (pFader->bFadingIn) {
            if (pFader->fAlpha < 1.0) {
                pFader->fAlpha = appTimer.GetRealTimeSeconds() / pFader->fFadeSpeed + pFader->fAlpha;
                if (pFader->fAlpha > 1.0) {
                    pFader->fAlpha = 1.0f;
                }
                if (pFader->spTileShaderProp.operator->()) {
                    float fAlpha = pFader->fAlpha;
                    pFader->spTileShaderProp.operator->()->SetTileAlpha(fAlpha);
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
            if (pFader->spTileShaderProp.operator->()) {
                float fAlpha = pFader->fAlpha;
                pFader->spTileShaderProp.operator->()->SetTileAlpha(fAlpha);
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
            NiNode* Parent = FaderData[aeFader].pRoot->m_pkParent;
            Parent->DetachChild(FaderData[aeFader].pRoot);
            FaderData[aeFader].pRoot = nullptr;
            FaderData[aeFader].fAlpha = 0.0f;
            FaderData[aeFader].spTileShaderProp.operator=(static_cast<TileShaderProperty*>(nullptr));
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
    FADER_DATA* v3 = &FaderData[aeFader];
    return v3->pRoot && (v3->bFadingIn && v3->fAlpha < 1.0);
}

void FaderManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}
