#include "Fallout/Magic/MagicLight.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

MagicLight::MagicLight(uint32_t aiLightID, NiAVObject* apObj)
  : pParent(nullptr), pLightObject(nullptr), fFadeTimer(0.0f), fFadeTargetTime(0.0f), bFadeIn(false) {
    LightData.fWantDimmer = 1.0f;
    Init(aiLightID, apObj);
}

MagicLight::~MagicLight() {
    if (pParent.m_pObject) {
        pParent->DetachChild(LightData.spLight.m_pObject);
    }
    ShadowSceneNode* pNode = BSShaderManager::GetShadowSceneNode(0);
    if (pNode) {
        pNode->RemoveLight(LightData.spLight.m_pObject);
    }
    pLightObject = nullptr;
}

bool MagicLight::IsFading() {
    return fFadeTargetTime > 0.0;
}

void MagicLight::Fade(bool abIn, float afTime) {
    bFadeIn = abIn;
    fFadeTargetTime = afTime;
    fFadeTimer = 0.0f;
}

void MagicLight::SetFadeTimer(float afTimer) {
    fFadeTimer = afTimer;
}

void MagicLight::Transfer(TESObjectREFR* apRef) {
    if (apRef->Get3D()) {
        Transfer(apRef->Get3D());
    }
}

void MagicLight::Update(float afDelta) {
    if (pLightObject) {
        if (fFadeTargetTime > 0.0) {
            fFadeTimer = fFadeTimer + afDelta;
            uint32_t uiColor = pLightObject->GetColor();
            float fBlue = static_cast<float>(static_cast<uint8_t>(uiColor >> 16));
            float fGreen = static_cast<float>(static_cast<uint8_t>(uiColor >> 8));
            NiColor color(static_cast<float>(static_cast<uint8_t>(uiColor)), fGreen, fBlue);
            // Both branches store the scale to one float slot.
            float fLightScale;
            if (bFadeIn) {
                fLightScale = fFadeTimer / fFadeTargetTime;
            }
            else {
                fLightScale = 1.0 - fFadeTimer / fFadeTargetTime;
            }
            if (fLightScale > 1.0) {
                fFadeTargetTime = 0.0f;
                fLightScale = 1.0f;
            }
            else if (fLightScale < 0.0) {
                fFadeTargetTime = 0.0f;
                fLightScale = 0.0f;
            }
            float fColorScale = fLightScale / 255.0;
            color *= fColorScale;
            LightData.spLight->SetDiffuseColor(color);
        }
        pLightObject->Update(&LightData, nullptr);
    }
}

void MagicLight::Init(uint32_t aiLightID, NiAVObject* apObj) {
    TESForm* pLightForm = TESForm::GetFormByNumericID(aiLightID);
    pLightObject = dynamic_cast<TESObjectLIGH*>(pLightForm);
    if (pLightObject) {
        NiPointLight* pLight = new NiPointLight;
        LightData.spLight = static_cast<NiLight*>(pLight);
        LightData.fWantDimmer = 1.0f;
        uint32_t uiColor = pLightObject->GetColor();
        float fBlue = static_cast<float>(static_cast<uint8_t>(uiColor >> 16));
        float fGreen = static_cast<float>(static_cast<uint8_t>(uiColor >> 8));
        NiColor color(static_cast<float>(static_cast<uint8_t>(uiColor)), fGreen, fBlue);
        color /= 255.0f;
        LightData.spLight->SetDiffuseColor(color);
        NiColor fakeSpecular(500.0f, 0.0f, 0.0f);
        LightData.spLight->SetSpecularColor(fakeSpecular);
        LightData.spLight->SetDimmer(pLightObject->fFade);
        ShadowSceneNode* pNode = BSShaderManager::GetShadowSceneNode(0);
        if (pNode) {
            pNode->AddLight(LightData.spLight.m_pObject, true);
        }
        pParent = static_cast<NiNode*>(apObj);
        pParent->AttachChild(LightData.spLight.m_pObject, 1);
    }
}

void MagicLight::Transfer(NiAVObject* apNode) {
    if (pLightObject) {
        pParent = static_cast<NiNode*>(apNode);
        pParent->AttachChild(LightData.spLight.m_pObject, 1);
    }
}

NiColor& NiColor::operator*=(float fScalar) {
    r *= fScalar;
    g *= fScalar;
    b *= fScalar;
    return *this;
}
