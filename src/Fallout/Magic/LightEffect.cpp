#include "Fallout/Magic/LightEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

// Whether the light archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::LIGHT, &LightEffect::Instantiate);

LightEffect::LightEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), spPointLight(nullptr) {}

// The point light's release is spPointLight's own destructor.
LightEffect::~LightEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* LightEffect::Copy() {
    LightEffect* pCopy = new LightEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void LightEffect::Update(float afSecondsElapsed) {
    if (spPointLight.m_pObject) {
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        if (pShadowSceneNode) {
            if (!pShadowSceneNode->GetLight(spPointLight.m_pObject)) {
                pShadowSceneNode->AddLight(spPointLight.m_pObject, true);
            }
        }
    }
}

void LightEffect::FinishInitLoadGame(Actor* apActor) {
    ActiveEffect::FinishInitLoadGame(apActor);
    Start();
}

void LightEffect::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActiveEffect::Revert(apLoadGameBuffer);
    DetachLight();
}

// Reverts the base effect and takes the light off the target.
void LightEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    DetachLight();
}

void LightEffect::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActiveEffect::FinishLoadGame(apLoadGameBuffer);
    Start();
}

void LightEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
    LightEffect* pdestination = dynamic_cast<LightEffect*>(apDestination);
    if (pdestination) {
        pdestination->spPointLight = spPointLight;
    }
}

bool LightEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor;
    if (apTarget) {
        pTargetActor = apTarget->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    return pTargetActor != nullptr;
}

// Attaches the best light of the target actor.
void LightEffect::Start() {
    Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    if (pActor) {
        AttachBestLightEffect(pActor);
    }
}

// Takes this light off and attaches the target actor's best remaining light.
void LightEffect::Finish() {
    DetachLight();
    Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    if (pActor) {
        AttachBestLightEffect(pActor);
    }
}

void LightEffect::DetachLight() {
    Actor* pTargetActor;
    if (GetTarget()) {
        pTargetActor = GetTarget()->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    if (spPointLight.m_pObject) {
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        if (pShadowSceneNode) {
            pShadowSceneNode->RemoveLight(spPointLight.m_pObject);
        }
        NiNode* pParent = spPointLight->GetParent();
        if (pParent) {
            pParent->DetachChild(spPointLight.m_pObject);
        }
        if (pTargetActor) {
            pTargetActor->KillLight(true);
        }
        spPointLight = static_cast<NiPointLight*>(nullptr);
        if (--LightEffect::iLightCount < 0) {
            LightEffect::iLightCount = 0;
        }
    }
}

void LightEffect::AttachLight() {
    Actor* ptargetActor;
    NiPoint3 boundMax;
    if (pTarget) {
        ptargetActor = pTarget->GetTargetAsActor();
    }
    else {
        ptargetActor = nullptr;
    }
    if (!spPointLight.m_pObject && ptargetActor && !ptargetActor->GetSpellEffectLight()) {
        if (LightEffect::iLightCount <= iMagicLightMaxCount.Int()) {
            TESObjectLIGH* plight = GetEffectSetting()->data.pLight;
            if (plight) {
                NiNode* p3D;
                NiPointLight* pPointLight = new NiPointLight;
                spPointLight = pPointLight;
                uint32_t ucolor = plight->GetColor();
                float b = static_cast<float>(static_cast<uint8_t>(ucolor >> 16));
                float g = static_cast<float>(static_cast<uint8_t>(ucolor >> 8));
                NiColor color(static_cast<float>(static_cast<uint8_t>(ucolor)), g, b);
                color /= 255.0f;
                spPointLight->SetDiffuseColor(color);
                double dradius = fMagnitude + fMagicLightRadiusBase.Float();
                float fradius = fMagicUnitsPerFoot.Float() * dradius;
                NiColor fakeSpecular(fradius, 0.0f, 0.0f);
                spPointLight->SetSpecularColor(fakeSpecular);
                float ffade = plight->fFade;
                spPointLight->SetDimmer(ffade);
                ShadowSceneNode* pnode = BSShaderManager::GetShadowSceneNode(0);
                if (pnode) {
                    pnode->AddLight(spPointLight.m_pObject, true);
                }
                float fX = ptargetActor->GetScale() * fMagicLightSideOffset.Float();
                NiPoint3* pboundMax = ::new (&boundMax) NiPoint3(ptargetActor->GetBoundMax());
                double dforward = pboundMax->y + fMagicLightForwardOffset.Float();
                float fY = ptargetActor->GetScale() * dforward;
                double dheight = ptargetActor->GetHeight();
                float fZ = ptargetActor->GetScale() * fMagicLightHeightOffset.Float() + dheight;
                spPointLight->SetTranslate(fX, fY, fZ);
                if (ptargetActor == pPlayer) {
                    p3D = static_cast<NiNode*>(pPlayer->Get3D(false));
                }
                else {
                    p3D = ptargetActor->Get3D();
                }
                NiPointer<NiNode> pparent(p3D);
                pparent->AttachChild(spPointLight.m_pObject, 1);
                ptargetActor->SetSpellEffectLight(spPointLight.m_pObject);
                ++LightEffect::iLightCount;
            }
            else {
                Warning(
                    "EDITOR: Light Effect has no Light object associated with it. Use the editor to select "
                    "a light in the Magic Effects window for the Light effect."
                );
            }
        }
    }
}

void LightEffect::AttachBestLightEffect(MagicTarget* apTarget) {
    if (apTarget) {
        LightEffect* pbestLight = nullptr;
        for (BSSimpleList<ActiveEffect*>* plist = apTarget->GetActiveEffectList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            ActiveEffect* peffect = plist->m_item;
            if (peffect) {
                EffectSetting* psetting = peffect->GetEffectSetting();
                if (psetting->GetEffectArchetype() == 13 && !peffect->bDone) {
                    double dbestMagnitude;
                    LightEffect* plightEffect = dynamic_cast<LightEffect*>(peffect);
                    if (pbestLight &&
                        (dbestMagnitude = pbestLight->GetMagnitude(),
                         !(plightEffect->GetMagnitude() > dbestMagnitude))) {
                        plightEffect->DetachLight();
                    }
                    else {
                        if (pbestLight) {
                            pbestLight->DetachLight();
                        }
                        pbestLight = plightEffect;
                    }
                }
            }
        }
        if (pbestLight) {
            pbestLight->AttachLight();
        }
    }
}

ActiveEffect* LightEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new LightEffect(apCaster, apSpell, apEffect);
}

NiColor& NiColor::operator/=(float fScalar) {
    r = r / fScalar;
    g = g / fScalar;
    b = b / fScalar;
    return *this;
}
