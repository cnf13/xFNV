#include "Fallout/Magic/MagicShaderHitEffect.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSWeightedRandom.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSCloningProcess.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/DisintegrateArmorEffect.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <limits>

NiRTTI MagicShaderHitEffect::ms_RTTI("MagicShaderHitEffect", &MagicHitEffect::ms_RTTI);

using ParticleShaderPropertyArray = BSSimpleArray<NiPointer<ParticleShaderProperty>>;

MagicShaderHitEffect::MagicShaderHitEffect()
  : MagicHitEffect(), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(nullptr), fAlphaTimer(0.0f),
    spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f), spTexEffectData(nullptr) {}

MagicShaderHitEffect::MagicShaderHitEffect(TESObjectREFR* apTarget, TESEffectShader* apShader, float afLifetime)
  : MagicHitEffect(apTarget, nullptr), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(apShader),
    fAlphaTimer(0.0f), spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f),
    spTexEffectData(nullptr) {
    if (pTarget == pPlayer) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        b3rdPerson = p3D && !p3D->GetAppCulled();
    }
    if (!(afLifetime < 0.0)) {
        fLifetime = afLifetime;
    }
    else {
        fLifetime = std::numeric_limits<float>::max();
    }
    if (pEffectData && pEffectData->Data.pAddonModels) {
        for (BSSimpleList<BGSDebrisData*>* pDebrisList = pEffectData->Data.pAddonModels->GetData();
             pDebrisList && !pDebrisList->IsEmpty();
             pDebrisList = pDebrisList->m_pkNext) {
            pModelLoader->LoadFile(pDebrisList->m_item->pFileName, LOD_MULT_NONE, true, false, false, false);
        }
    }
}

MagicShaderHitEffect::MagicShaderHitEffect(TESObjectREFR* apTarget, ActiveEffect* apEffect)
  : MagicHitEffect(apTarget, apEffect), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(nullptr),
    fAlphaTimer(0.0f), spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f),
    spTexEffectData(nullptr) {
    EffectSetting* pAVEffect = apEffect->pSpell->GetAVEffect(MagicSystem::SELF);
    pEffectData = pAVEffect->data.pEffectShader;
    if (pTarget == pPlayer) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        b3rdPerson = p3D && !p3D->GetAppCulled();
    }
    if (pEffectData && pEffectData->Data.pAddonModels) {
        for (BSSimpleList<BGSDebrisData*>* pDebrisList = pEffectData->Data.pAddonModels->GetData();
             pDebrisList && !pDebrisList->IsEmpty();
             pDebrisList = pDebrisList->m_pkNext) {
            pModelLoader->LoadFile(pDebrisList->m_item->pFileName, LOD_MULT_NONE, true, false, false, false);
        }
    }
}

MagicShaderHitEffect::~MagicShaderHitEffect() {
    if (pTarget != pPlayer && pCell == nullptr) {
        pTarget = nullptr;
    }
    if (spTexEffectData.m_pObject) {
        if (pTarget == pPlayer) {
            NiAVObject* p3D = pPlayer->Get3D(true);
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
            p3D = pPlayer->Get3D(false);
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
        }
        else if (pTarget) {
            NiAVObject* p3D = pTarget->Get3D();
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
        }
    }
    if (spParticleNode.m_pObject) {
        for (ParticleShaderProperty* pProperty : ParticlePropertyArray) {
            pEffectData->StopParticleShader(spParticleNode.m_pObject, pProperty);
        }
    }
    for (NiAVObject* pObject : AddOnObjects) {
        if (pObject->m_pkParent) {
            pObject->m_pkParent->DetachChild(pObject);
        }
    }
    if (spParticleNode.m_pObject) {
        spParticleNode->AttachParent(nullptr);
    }
    if (pEffectData) {
        char cPath[264];
        // The two effect shaders whose moving sounds stop with the effect.
        if (pTarget && (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51)) {
            BSAudio::QInstance().StopMovingSounds(pTarget->Get3D(), 1.0f, 0);
        }
        if (pEffectData->ParticleShaderTexture.GetTextureName()) {
            BSsprintf(cPath, 260, "%s\\%s", "Textures", pEffectData->ParticleShaderTexture.GetTextureName());
            TES::pInstance->RemoveTextureImage(cPath, 0);
        }
        if (pEffectData->GetTextureShaderTexture()->GetTextureNameLength()) {
            BSsprintf(cPath, 260, "%s\\%s", "Textures", pEffectData->GetTextureShaderTexture()->GetTextureName());
            TES::pInstance->RemoveTextureImage(cPath, 0);
        }
        if (pEffectData->Data.pAddonModels) {
            for (BSSimpleList<BGSDebrisData*>* pDebrisList = pEffectData->Data.pAddonModels->GetData();
                 pDebrisList && !pDebrisList->IsEmpty();
                 pDebrisList = pDebrisList->m_pkNext) {
                pModelLoader->ReleaseFile(pDebrisList->m_item->pFileName);
            }
        }
    }
}

bool MagicShaderHitEffect::Update(float afTimeDelta) {
    bool bDone;
    float fAlpha;
    if (MagicHitEffect::Update(afTimeDelta) && pTarget && pTarget->Get3D()) {
        fAlphaTimer = fAlphaTimer + afTimeDelta;
        bDone = bFinished;
        UpdatePosition();
        fAlpha = 1.0f;
        if (pTarget->IsActor() && pEffectData != MagicSystem::GetDetectLifeShaderEffect()) {
            if (spTexEffectData.m_pObject &&
                spTexEffectData->spBlockOutTexture.m_pObject &&
                pTarget->IsActor() &&
                static_cast<Actor*>(pTarget)->Actor::GetAlphaMult() > 0.9900000095367432) {
                static_cast<Actor*>(pTarget)->Actor::SetAlphaMult(0.99f);
                static_cast<Actor*>(pTarget)->Actor::UpdateAlpha();
            }

            BSFadeNode* pFadeNode = static_cast<BSFadeNode*>(pTarget->Get3D());
            if (NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pFadeNode)) {
                fAlpha = pFadeNode->GetLastFadePercent();
            }
            else {
                fAlpha = static_cast<Actor*>(pTarget)->GetAlpha();
            }
            float fMult = static_cast<Actor*>(pTarget)->Actor::GetAlphaMult();
            if (fMult > 0.0) {
                fAlpha = fAlpha / fMult;
            }
        }
        if (ParticlePropertyArray.iSize && spParticleNode != static_cast<NiNode*>(nullptr)) {
            float fLastParticleCount = 0.0f;
            for (ParticleShaderProperty* pProperty : ParticlePropertyArray) {
                double dMaxParticleRatio = pProperty->fMaxParticleRatio;
                fLastParticleCount = static_cast<double>(ParticleShader::GetMaxInstanceSize()) * dMaxParticleRatio +
                    fLastParticleCount;
            }
            float fParticleCount = pEffectData->AnimateParticleEffectBirthrate(
                fLastParticleCount,
                afTimeDelta,
                fAlphaTimer,
                bFinished
            );
            if (fParticleCount > 0.0) {
                bDone = false;
            }
            for (const NiPointer<ParticleShaderProperty>& spProperty : ParticlePropertyArray) {
                float fMaxInstanceSize = static_cast<float>(ParticleShader::GetMaxInstanceSize());
                float fParticleCountForThisPass = BSMIN(fParticleCount, fMaxInstanceSize);
                fParticleCount = fParticleCount - fParticleCountForThisPass;
                ParticleShaderProperty* pParticleProp = spProperty.m_pObject;
                float fRatio = fParticleCountForThisPass / static_cast<double>(ParticleShader::GetMaxInstanceSize());
                pParticleProp->SetMaxParticleRatio(fRatio);
                pParticleProp->SetFade(fAlpha);
                pParticleProp->UpdateParticles(fAlphaTimer, false, true);
                if (pParticleProp->iParticleCount > 0) {
                    bDone = false;
                }
            }
            if (AddOnObjects.iSize) {
                float fScaleLimit;
                double dAddonScaleTime;
                double dAddonFadeTime;
                if (bFinished) {
                    dAddonFadeTime = pEffectData->Data.fAddOnFadeOutTime * -1.0;
                }
                else {
                    dAddonFadeTime = pEffectData->Data.fAddOnFadeInTime;
                }
                float fAlphaStep = afTimeDelta / dAddonFadeTime;
                float fNewAddonAlpha = fAddonAlpha + fAlphaStep;
                float fCappedAddonAlpha = BSMIN(1.0f, fNewAddonAlpha);
                fAddonAlpha = BSMAX(0.0f, fCappedAddonAlpha);
                if (fAddonAlpha > 0.0) {
                    bDone = false;
                }
                if (bFinished) {
                    dAddonScaleTime = pEffectData->GetAddOnScaleOutTime() * -1.0;
                }
                else {
                    dAddonScaleTime = pEffectData->Data.fAddOnScaleInTime;
                }
                float fScaleStep = afTimeDelta / dAddonScaleTime;
                float fNewAddonScale = fAddonScale + fScaleStep;
                float fCappedAddonScale = BSMIN(1.0f, fNewAddonScale);
                fAddonScale = BSMAX(0.0f, fCappedAddonScale);
                if (bFinished) {
                    fScaleLimit = pEffectData->Data.fAddOnScaleEnd;
                }
                else {
                    fScaleLimit = pEffectData->Data.fAddOnScaleStart;
                }
                float fScaleLow = BSMAX(0.01f, fScaleLimit);
                float fScale = (1.0f - fScaleLow) * fAddonScale + fScaleLow;
                for (NiAVObject* pAddOnObject : AddOnObjects) {
                    float fAddonFade = fAddonAlpha;
                    BSShaderUtil::RecursiveSetPropertyFadeAlpha(pAddOnObject->IsNode(), fAddonFade);
                    pAddOnObject->SetScale(fScale);
                }
            }
        }

        if (spTexEffectData.m_pObject) {
            pEffectData->AnimateTextureEffect(
                spTexEffectData.m_pObject,
                afTimeDelta,
                fAlphaTimer,
                this->MagicHitEffect::fAge,
                bFinished
            );
            spTexEffectData->CurrentFillColor.a = spTexEffectData->CurrentFillColor.a * fAlpha;
            spTexEffectData->CurrentRimColor.a = spTexEffectData->CurrentRimColor.a * fAlpha;
            if (spTexEffectData->RimColor.a > 0.0 || spTexEffectData->FillColor.a > 0.0) {
                bDone = false;
            }
        }
        if (!bFinished || !bDone) {
            return true;
        }
        if (pTarget->IsActor() && static_cast<Actor*>(pTarget)->GetCurrentAIProcess()) {
            BaseProcess* pProcess = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
            pProcess->SetShouldRefreshMagicShadersEffects(true, true, false);
        }
    }
    ClearTarget();
    return false;
}

bool MagicShaderHitEffect::CheckShouldSave() {
    if (pEffect) {
        return false;
    }
    if (!pTarget || !pEffectData || bWeaponEnchantment) {
        return false;
    }
    // Calls the getter first and reads pEffectData after it.
    TESEffectShader* pDetectLife = MagicSystem::GetDetectLifeShaderEffect();
    return pEffectData != pDetectLife;
}

uint16_t MagicShaderHitEffect::GetSaveSize() {
    return GetSaveSize(static_cast<ActiveEffect*>(nullptr), pTarget) + 8;
}

void MagicShaderHitEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    MagicHitEffect::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWeaponEnchantment), sizeof(bWeaponEnchantment), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAlphaTimer), sizeof(fAlphaTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAddonAlpha), sizeof(fAddonAlpha), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAddonScale), sizeof(fAddonScale), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&b3rdPerson), sizeof(b3rdPerson), 0);
    apSaveGameBuffer->SaveFormID(pWornObject, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveFormID(pEffectData, static_cast<uint32_t>(0));
}

void MagicShaderHitEffect::SaveGame() {
    int32_t iTargetID = pTarget->GetFormID();
    pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&iTargetID), 4u);
    uint32_t uiShaderID = pEffectData->GetFormID();
    pSaveLoadGameOLD->SaveNumericID(&uiShaderID, 4u);
    SaveGame(static_cast<ActiveEffect*>(nullptr), pTarget);
}

void MagicShaderHitEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWeaponEnchantment), sizeof(bWeaponEnchantment));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAlphaTimer), sizeof(fAlphaTimer));
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAddonAlpha), sizeof(fAddonAlpha));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAddonScale), sizeof(fAddonScale));
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&b3rdPerson), sizeof(b3rdPerson));
    uint32_t uiWornID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiWornID);
    pWornObject = dynamic_cast<TESBoundObject*>(pForm);
    uint32_t uiShaderID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiShaderID);
    pEffectData = dynamic_cast<TESEffectShader*>(pForm);
}

bool MagicShaderHitEffect::LoadGame() {
    uint32_t uiShaderID;
    uint32_t uiTargetID;
    bool bLoaded = true;
    pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4u);
    TESForm* pForm = TESForm::GetFormByNumericID(uiTargetID);
    pTarget = dynamic_cast<TESObjectREFR*>(pForm);
    if (!pTarget || !pTarget->GetParentCell() || !pTarget->GetParentCell()->GetAttached()) {
        bLoaded = false;
    }
    pSaveLoadGameOLD->LoadNumericID(&uiShaderID, 4u);
    pForm = TESForm::GetFormByNumericID(uiShaderID);
    char* pShader = reinterpret_cast<char*>(dynamic_cast<TESEffectShader*>(pForm));
    if (!pShader) {
        bLoaded = false;
    }
    LoadGame(static_cast<ActiveEffect*>(nullptr), pTarget);
    if (bLoaded) {
        InitLoadGame(static_cast<ActiveEffect*>(nullptr), pTarget);
        FinishInitLoadGame(nullptr, pTarget, pShader);
    }
    return bLoaded;
}

void MagicShaderHitEffect::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::InitLoadGame(apLoadGameBuffer);
}

// Restores the hit effect after a load: the base's step, then Init with the age and finished flag kept, then
// an update of no time.
void MagicShaderHitEffect::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::FinishLoadGame(apLoadGameBuffer);
    float ftempAge = fAge;
    bool btempFinished = bFinished;
    Init();
    fAge = ftempAge;
    bFinished = btempFinished;
    Update(0.0f);
}

bool MagicShaderHitEffect::Init() {
    char cBlockOutPath[260];
    char cTexturePath[260];
    char cParticlePath[260];
    if (pEffect) {
        this->MagicHitEffect::fAge = pEffect->fElapsedSeconds;
        EffectSetting* pEffectSetting = pEffect->GetEffectSetting();
        bFinished = !pEffectSetting->IsFlagSet(EffectSetting::PERSIST);
        if (pEffect->IsFlagSet(ActiveEffect::NO_INTIAL_FLARE)) {
            fAlphaTimer = 100.0f;
        }
    }
    bool bSword = false;
    bool bActorRoot = false;
    float fSwordLength = 0.0f;
    bool bSwitching = false;
    if (pTarget == pPlayer && pPlayer->Get3D(false)) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        bool b3rd = !p3D->GetAppCulled();
        bSwitching = this->MagicHitEffect::fAge > 0.0 && b3rd != b3rdPerson;
        b3rdPerson = b3rd;
    }

    NiAVObject* pMembraneRoot = nullptr;
    NiAVObject* pParticleRoot = nullptr;
    GetAffectedScenegraph(bActorRoot, bSword, fSwordLength, pMembraneRoot, pParticleRoot);
    if (!pMembraneRoot || !pParticleRoot || !pEffectData) {
        return false;
    }
    // The form IDs of two effect shaders that play the fire loop.
    if (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51) {
        BSSoundHandle kHandle = BSAudio::QInstance().GetSoundHandleByName("AMBFireMediumLP", 18u);
        NiPoint3 kLocation = pTarget->GetLocationOnReference();
        kHandle.SetPosition(kLocation.x, kLocation.y, kLocation.z);
        kHandle.SetVolume(1.0f);
        kHandle.Play(true);
        kHandle.SetObjectToFollow(pTarget->Get3D());
    }
    bool bDoParticles = true;
    if (bWeaponEnchantment && pTarget->IsActor() && static_cast<Actor*>(pTarget)->GetCurrentAIProcess()) {
        BaseProcess* pProcess = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
        if (pProcess->IsCurrentWeaponGrenadeOrMine()) {
            bDoParticles = false;
        }
        pProcess = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
        if (!pProcess->GetWeaponDrawn() && pPlayer->GetMagicShaderHitEffect() != this) {
            bDoParticles = false;
        }
    }

    if (!bDoParticles || pEffectData->IsFlagSet(TESEffectShader::DISABLE_PARTICLE_SHADER)) {
        if (ParticlePropertyArray.iSize && spParticleNode.m_pObject) {
            for (ParticleShaderProperty* pProperty : ParticlePropertyArray) {
                pEffectData->StopParticleShader(spParticleNode.m_pObject, pProperty);
            }
            ParticlePropertyArray.Clear(true);
            if (spParticleNode.m_pObject) {
                spParticleNode->AttachParent(nullptr);
            }
            spParticleNode = static_cast<NiNode*>(nullptr);
        }
    }
    else {
        if (spParticleNode == static_cast<NiNode*>(nullptr)) {
            spParticleNode = new NiNode(0);
            spParticleNode->SetName("ParticleShader Geometry");
            spParticleNode->SetRotate(NiMatrix3::IDENTITY);
        }
        if (!ParticlePropertyArray.iSize) {
            NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->ParticleShaderTexture.GetTextureNameLength()) {
                BSsprintf(
                    cParticlePath,
                    260,
                    "%s\\%s",
                    "Textures",
                    pEffectData->ParticleShaderTexture.GetTextureName()
                );
                TES::pInstance->CreateTextureImage(cParticlePath, spTexture, false, false);
                float fParticlePersistent = pEffectData->Data.fParticlePersistent;
                float fParticleFull = pEffectData->Data.fParticleFull;
                int32_t iMaxParticles = static_cast<int32_t>(BSMAX(fParticleFull, fParticlePersistent));
                int32_t iParticlesPerPass = ParticleShader::GetMaxInstanceSize();
                for (int32_t iPassCount = 0; iParticlesPerPass * iPassCount < iMaxParticles; ++iPassCount) {
                    NiPointer<ParticleShaderProperty> spParticleProperty(pEffectData->StartParticleShader(
                        spParticleNode.m_pObject,
                        pParticleRoot,
                        spTexture.m_pObject,
                        fAlphaTimer
                    ));
                    ParticlePropertyArray.Add(spParticleProperty);
                }
            }
        }
        for (int32_t i = 0; i < ParticlePropertyArray.iSize; ++i) {
            ParticleShaderProperty* pParticleProperty = ParticlePropertyArray[i].m_pObject;
            if (bSword) {
                pParticleProperty->SetEmitterTypeRay(fSwordLength);
            }
            if (bWeaponEnchantment) {
                pParticleProperty->SetByte6C(false);
            }
            if (pTarget) {
                TESObjectACTI* pActivator = dynamic_cast<TESObjectACTI*>(pTarget->GetObjectReference());
                if (bActorRoot || pActivator) {
                    if (pTarget != pPlayer || b3rdPerson) {
                        pParticleProperty->SetEmitterTypeCollision();
                    }
                    else {
                        pParticleProperty->SetEmitterTypeGeometry();
                    }
                }
            }
            pParticleProperty->CollectTargets(pParticleRoot);
            if (bSwitching) {
                pParticleProperty->InitParticleData(0.0f);
                float fRewindTime = fAlphaTimer - 0.16599999368190765;
                pParticleProperty->UpdateParticles(fRewindTime, false, true);
                pParticleProperty->UpdateParticles(fAlphaTimer, false, true);
            }
        }
        UpdatePosition();
    }
    if (!pEffectData->IsFlagSet(TESEffectShader::DISABLE_TEXTURE_SHADER)) {
        if (spTexEffectData == static_cast<BSShaderPPLightingProperty::TextureEffectData*>(nullptr)) {
            NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->GetTextureShaderTexture()->GetTextureNameLength()) {
                BSsprintf(
                    cTexturePath,
                    260,
                    "%s\\%s",
                    "Textures",
                    pEffectData->GetTextureShaderTexture()->GetTextureName()
                );
                TES::pInstance->CreateTextureImage(cTexturePath, spTexture, false, false);
            }
            NiPointer<NiSourceTexture> spBlockOutTexture(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->GetBlockOutTexture()->GetTextureNameLength()) {
                BSsprintf(
                    cBlockOutPath,
                    260,
                    "%s\\%s",
                    "Textures",
                    pEffectData->GetBlockOutTexture()->GetTextureName()
                );
                TES::pInstance->CreateTextureImage(cBlockOutPath, spBlockOutTexture, false, false);
            }
            spTexEffectData = pEffectData->CreateTextureShaderData(
                pMembraneRoot,
                spTexture.m_pObject,
                spBlockOutTexture.m_pObject
            );
            if (spTexEffectData.m_pObject &&
                spTexEffectData->spBlockOutTexture.m_pObject &&
                pTarget &&
                pTarget->IsActor() &&
                static_cast<Actor*>(pTarget)->Actor::GetAlphaMult() > 0.9900000095367432) {
                static_cast<Actor*>(pTarget)->Actor::SetAlphaMult(0.99f);
                static_cast<Actor*>(pTarget)->Actor::UpdateAlpha();
            }
        }
        pEffectData->StartTextureShader(pMembraneRoot, spTexEffectData.m_pObject);
    }
    if (!bSwitching &&
        !AddOnObjects.iSize &&
        pEffectData &&
        pEffectData->Data.pAddonModels &&
        ParticlePropertyArray.iSize &&
        ParticlePropertyArray[0].m_pObject) {
        NiTObjectArray<NiPointer<NiAVObject>>* pTargetArray = ParticlePropertyArray[0]->GetTargetArray();
        if (pTargetArray->m_usSize) {
            BSWeightedRandom<char*> kRandomDebris;
            for (BSSimpleList<BGSDebrisData*>* pDebrisList = pEffectData->Data.pAddonModels->GetData();
                 pDebrisList && !pDebrisList->IsEmpty();
                 pDebrisList = pDebrisList->m_pkNext) {
                char* pFileName = pDebrisList->m_item->pFileName;
                pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
                kRandomDebris.AddWeightedResult(pDebrisList->m_item->pFileName, pDebrisList->m_item->cPercentage);
            }
            ParticleShaderProperty* pParticleProp = ParticlePropertyArray[0].m_pObject;
            for (int32_t i = 0; i < static_cast<uint32_t>(pParticleProp->GetTargetArray()->m_usSize); ++i) {
                NiAVObject* pObject = (*pParticleProp->GetTargetArray())[i].m_pObject;
                if (pObject->IsNode()) {
                    NiPointer<Model> spModel;
                    char* pFileName = kRandomDebris.PickResult();
                    pModelLoader->FindModel(pFileName, spModel);
                    if (spModel.m_pObject && spModel->GetModel()) {
                        BSCloningProcess kClone(1.0f);
                        NiAVObject* pAddon = static_cast<NiAVObject*>(spModel->GetModel()->Clone(kClone));
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            NiNode* pParentNode = pObject->IsNode();
                            TaskQueueInterface::GetInstance()->QueueNodeAttach(pAddon, pParentNode, true, false);
                        }
                        else {
                            pObject->IsNode()->AttachChild(pAddon, 1);
                        }
                        fAddonAlpha = 0.0f;
                        fAddonScale = BSMAX(0.01f, pEffectData->Data.fAddOnScaleStart);
                        pAddon->SetScale(fAddonScale);
                        NiTimeController::StartAnimations(pAddon);
                        NiPointer<NiAVObject> spAddon(pAddon);
                        AddOnObjects.Add(spAddon);
                    }
                }
            }
        }
    }

    return ParticlePropertyArray.iSize || spTexEffectData.m_pObject;
}

void MagicShaderHitEffect::ClearTarget() {
    if (pTarget && pEffectData && (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51)) {
        NiNode* pTarget3D = pTarget->Get3D();
        BSAudio::QInstance().StopMovingSounds(pTarget3D, 1.0f, 0);
    }

    if (spTexEffectData.m_pObject) {
        if (pTarget == pPlayer) {
            NiAVObject* p3D = pPlayer->Get3D(true);
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
            p3D = pPlayer->Get3D(false);
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
        }
        else if (pTarget) {
            NiAVObject* p3D = pTarget->Get3D();
            if (p3D) {
                pEffectData->StopTextureShader(p3D, spTexEffectData.m_pObject);
            }
        }
    }
    if (spParticleNode != static_cast<NiNode*>(nullptr)) {
        for (ParticleShaderProperty* pProperty : ParticlePropertyArray) {
            pEffectData->StopParticleShader(spParticleNode.m_pObject, pProperty);
        }
    }
    ParticlePropertyArray.Clear(true);
    if (spParticleNode != static_cast<NiNode*>(nullptr)) {
        spParticleNode->AttachParent(nullptr);
    }
    spParticleNode = static_cast<NiNode*>(nullptr);
    spTexEffectData = static_cast<BSShaderPPLightingProperty::TextureEffectData*>(nullptr);
    pTarget = nullptr;
}

void MagicShaderHitEffect::UpdatePosition() {
    NiPoint3 kOffset;
    NiMatrix3 kInverseRotate2;
    NiMatrix3 kInverseRotate;
    NiMatrix3 kTargetRot;
    PlayerCharacter* pPlayerTarget = static_cast<PlayerCharacter*>(pTarget);
    bool bPlayer = pPlayerTarget == pPlayer;
    bool bPlayerEffect = pPlayerTarget == pPlayer && pPlayer && pPlayer->GetMagicShaderHitEffect() == this;
    bool bFirstPerson = false;
    if (bPlayer && pPlayer && pPlayer->Get3D(false)) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        if (p3D->GetAppCulled()) {
            bFirstPerson = true;
        }
    }

    NiAVObject* pTarget3D = nullptr;
    if (bWeaponEnchantment) {
        NiAVObject* pParticleTarget;
        NiAVObject* pMembraneTarget;
        bool bUnused;
        float fUnused;
        GetAffectedScenegraph(bUnused, bUnused, fUnused, pMembraneTarget, pParticleTarget);
        pTarget3D = pParticleTarget;
    }
    else if (bPlayer) {
        pTarget3D = pPlayer->Get3D(false);
    }
    else {
        pTarget3D = pTarget->Get3D();
    }
    if (spParticleNode != static_cast<NiNode*>(nullptr) && pTarget3D) {
        const void* pSourceRotate;
        if (bWeaponEnchantment) {
            pSourceRotate = &pTarget3D->GetWorldTransform();
        }
        else {
            pSourceRotate = &NiMatrix3::IDENTITY;
        }
        rt::memcpy(&kTargetRot, pSourceRotate, sizeof(kTargetRot));
        if (!bPlayerEffect) {
            if (bFirstPerson && pPlayer->Get3D(true)) {
                NiAVObject* pPlayer3D = pPlayer->Get3D(true);
                NiNode* pNode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pPlayer3D));
                if (pNode && pNode != spParticleNode->m_pkParent) {
                    pNode->AttachChild(spParticleNode.m_pObject, 1);
                }

                if (bFirstPerson) {
                    NiMatrix3* pInverse = spParticleNode->m_pkParent->GetWorldTransform().m_Rotate.Inverse(
                        &kInverseRotate
                    );
                    spParticleNode->SetRotate(*pInverse * kTargetRot);
                    const NiPoint3* pParentTranslate = &spParticleNode->m_pkParent->GetWorldTranslate();
                    NiPoint3* pOffset = ::new (&kOffset) NiPoint3(pTarget3D->GetWorldTranslate() - *pParentTranslate);
                    pInverse = spParticleNode->m_pkParent->GetWorldTransform().m_Rotate.Inverse(&kInverseRotate2);
                    spParticleNode->SetTranslate(*pInverse * *pOffset);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    spParticleNode->Update(kUpdateData);
                }
                else {
                    spParticleNode->SetRotate(kTargetRot);
                    spParticleNode->SetTranslate(pTarget3D->GetWorldTranslate());
                }
            }
            else {
                spParticleNode->SetTranslate(pTarget3D->GetWorldTranslate());
                spParticleNode->SetRotate(kTargetRot);
            }
        }
    }

    if (bFirstPerson || bPlayerEffect) {
        UpdateParentCell(nullptr);
    }
    else {
        UpdateParentCell(spParticleNode.m_pObject);
    }
}

// Adds 4 bytes of its own; save version 0x37 added a byte and a 4-byte field.
uint16_t MagicShaderHitEffect::GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    uint16_t usSize = MagicHitEffect::GetSaveSize(apEffect, apTarget);
    usSize += 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        usSize += 1;
        usSize += 4;
    }
    return usSize;
}

// Save version 0x37 added the weapon enchantment flag and the worn object's form ID.
void MagicShaderHitEffect::SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    MagicHitEffect::SaveGame(apEffect, apTarget);
    pSaveLoadGameOLD->SaveGameDataOLD(&fAlphaTimer, sizeof(fAlphaTimer));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bWeaponEnchantment, sizeof(bWeaponEnchantment));
        uint32_t uiWornID = 0;
        if (pWornObject) {
            uiWornID = pWornObject->GetFormID();
        }
        pSaveLoadGameOLD->SaveNumericID(&uiWornID, 4);
    }
}

// A worn object ID of 0 leaves pWornObject as it is.
void MagicShaderHitEffect::LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    MagicHitEffect::LoadGame(apEffect, apTarget);
    pSaveLoadGameOLD->LoadGameDataOLD(&fAlphaTimer, sizeof(fAlphaTimer));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bWeaponEnchantment, sizeof(bWeaponEnchantment));
        uint32_t uiWornID;
        pSaveLoadGameOLD->LoadNumericID(&uiWornID, 4);
        if (uiWornID) {
            pWornObject = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(uiWornID));
        }
    }
}

void MagicShaderHitEffect::InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    MagicHitEffect::InitLoadGame(apEffect, apTarget);
}

void MagicShaderHitEffect::FinishInitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget, void* apData) {
    if (apEffect) {
        EffectSetting* pAVEffect = apEffect->pSpell->GetAVEffect(MagicSystem::SELF);
        pEffectData = pAVEffect->data.pEffectShader;
    }
    else {
        pEffectData = static_cast<TESEffectShader*>(apData);
    }
    float ftempAge = MagicHitEffect::fAge;
    bool btempFinished = bFinished;
    Init();
    MagicHitEffect::fAge = ftempAge;
    bFinished = btempFinished;
    Update(0.0f);
}

void MagicShaderHitEffect::SetLifeTime(float afLifeTime) {
    fAlphaTimer = afLifeTime;
}

void MagicShaderHitEffect::SetFinished(bool abFinished) {
    bFinished = abFinished;
}

void MagicShaderHitEffect::GetAffectedScenegraph(
    bool& abActorRoot,
    bool& abSword,
    float& afSwordLength,
    NiAVObject*& apMembraneRoot,
    NiAVObject*& apParticleRoot
) {
    NiAVObject* p3D = nullptr;
    Actor* pActor = dynamic_cast<Actor*>(pTarget);
    if (bWeaponEnchantment) {
        if (pTarget && pTarget->IsActor()) {
            pActor = static_cast<Actor*>(pTarget);
            BaseProcess* pProcess = pActor->pCurrentProcess;
            if (pProcess) {
                pProcess->GetCurrentWeapon();
            }
        }
    }
    else if (pTarget) {
        p3D = pTarget->Get3D();
        if (pTarget->IsActor()) {
            if (pTarget == pPlayer) {
                p3D = pPlayer->Get3D(!b3rdPerson);
            }
            abActorRoot = true;
        }
        else if (dynamic_cast<TESObjectWEAP*>(pTarget->GetObjectReference()) && p3D) {
            abSword = true;
            if (p3D->GetWorldBound().m_fRadius <= 0.0) {
                NiUpdateData kUpdateData(0.0f, false, false);
                p3D->Update(kUpdateData);
            }
            const NiPoint3* pTranslate = &p3D->GetWorldTranslate();
            NiPoint3 kDiff = p3D->GetWorldBound().m_kCenter - *pTranslate;
            double dLength = kDiff.Length();
            afSwordLength = p3D->GetWorldBound().m_fRadius + dLength;
        }
    }
    apMembraneRoot = p3D;
    apParticleRoot = p3D;
}

void MagicShaderHitEffect::ResetAlphaTimer() {
    float fAlpha = 0.0f;
    if (spTexEffectData.m_pObject) {
        fAlpha = BSMAX(spTexEffectData->FillColor.a, spTexEffectData->RimColor.a);
    }
    if (bWeaponEnchantment) {
        fAlpha = 0.0f;
        spTexEffectData->FillColor.a = 0.0f;
        spTexEffectData->RimColor.a = 0.0f;
    }
    if (!(static_cast<double>(fAlpha) < pEffectData->Data.fFillAlphaFullPercent)) {
        fAlphaTimer = pEffectData->Data.fFillAlphaFadeInTime;
    }
    else {
        double dFadeInTime = pEffectData->Data.fFillAlphaFadeInTime;
        fAlphaTimer = fAlpha / pEffectData->Data.fFillAlphaFullPercent * dFadeInTime;
    }
    for (ParticleShaderProperty* pProperty : ParticlePropertyArray) {
        pProperty->RewindTimer(fAlphaTimer);
    }
}

ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeRay(float afRayLength) {
    eEmitterType = EMIT_RAY;
    fRayLength = afRayLength;
    return this;
}

ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeCollision() {
    eEmitterType = EMIT_COLLISION;
    return this;
}

ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeGeometry() {
    eEmitterType = EMIT_GEOMETRY;
    return this;
}

NiTObjectArray<NiPointer<NiAVObject>>* ParticleShaderProperty::GetTargetArray() {
    return &TargetArray;
}

float TESEffectShader::GetAddOnScaleOutTime() {
    return Data.fAddOnScaleOutTime;
}
