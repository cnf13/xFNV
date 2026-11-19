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
        pParent.m_pObject->DetachChild(LightData.spLight.m_pObject);
    }
    ShadowSceneNode* pnode = BSShaderManager::GetShadowSceneNode(0);
    if (pnode) {
        pnode->RemoveLight(LightData.spLight.m_pObject);
    }
    pLightObject = nullptr;
}

bool MagicLight::IsFading() {
    return (bool)((uint8_t)(fFadeTargetTime > 0.0));
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
        NiNode* v2 = apRef->Get3D();
        Transfer(v2);
    }
}

void MagicLight::Update(float afDelta) {
    if (pLightObject) {
        if (fFadeTargetTime > 0.0) {
            fFadeTimer = fFadeTimer + afDelta;
            uint32_t ucolor = pLightObject->GetColor();
            float a2 = static_cast<float>(static_cast<uint8_t>(ucolor >> 16));
            float y = static_cast<float>(static_cast<uint8_t>(ucolor >> 8));
            NiColor color(static_cast<float>(static_cast<uint8_t>(ucolor)), y, a2);
            // Both branches store the scale to one float slot.
            float flightScale;
            if (bFadeIn) {
                flightScale = fFadeTimer / fFadeTargetTime;
            }
            else {
                flightScale = 1.0 - fFadeTimer / fFadeTargetTime;
            }
            if (!(flightScale > 1.0)) {
                if (flightScale < 0.0) {
                    fFadeTargetTime = 0.0f;
                    flightScale = 0.0f;
                }
            }
            else {
                fFadeTargetTime = 0.0f;
                flightScale = 1.0f;
            }
            float v6 = flightScale / 255.0;
            color *= v6;
            LightData.spLight.m_pObject->SetDiffuseColor(color);
        }
        pLightObject->Update(&LightData, nullptr);
    }
}

void MagicLight::Init(uint32_t aiLightID, NiAVObject* apObj) {
    TESForm* plightForm = TESForm::GetFormByNumericID(aiLightID);
    pLightObject = dynamic_cast<TESObjectLIGH*>(plightForm);
    if (pLightObject) {
        NiPointLight* v8;
        NiPointLight* apMemory = static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
        if (apMemory) {
            v8 = ::new (apMemory) NiPointLight;
        }
        else {
            v8 = nullptr;
        }
        LightData.spLight = static_cast<NiLight*>(v8);
        LightData.fWantDimmer = 1.0f;
        uint32_t ucolor = pLightObject->GetColor();
        float z = static_cast<float>(static_cast<uint8_t>(ucolor >> 16));
        float y = static_cast<float>(static_cast<uint8_t>(ucolor >> 8));
        NiColor color(static_cast<float>(static_cast<uint8_t>(ucolor)), y, z);
        color /= 255.0f;
        LightData.spLight.m_pObject->SetDiffuseColor(color);
        NiColor fakeSpecular(500.0f, 0.0f, 0.0f);
        LightData.spLight.m_pObject->SetSpecularColor(fakeSpecular);
        float za = pLightObject->fFade;
        LightData.spLight.m_pObject->SetDimmer(za);
        ShadowSceneNode* pnode = BSShaderManager::GetShadowSceneNode(0);
        if (pnode) {
            pnode->AddLight(LightData.spLight.m_pObject, true);
        }
        pParent = static_cast<NiNode*>(apObj);
        NiNode* v7 = pParent.m_pObject;
        v7->AttachChild(LightData.spLight.m_pObject, 1);
    }
}

void MagicLight::Transfer(NiAVObject* apNode) {
    if (pLightObject) {
        pParent = static_cast<NiNode*>(apNode);
        NiNode* v3 = pParent.m_pObject;
        v3->AttachChild(LightData.spLight.m_pObject, 1);
    }
}

NiColor& NiColor::operator*=(float fScalar) {
    r *= fScalar;
    g *= fScalar;
    b *= fScalar;
    return *this;
}
