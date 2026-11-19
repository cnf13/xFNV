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

NiRTTI MagicShaderHitEffect::ms_RTTI("MagicShaderHitEffect", &MagicHitEffect::ms_RTTI);

using ParticleShaderPropertyArray = BSSimpleArray<NiPointer<ParticleShaderProperty>>;

MagicShaderHitEffect::MagicShaderHitEffect()
  : MagicHitEffect(), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(nullptr), fAlphaTimer(0.0f),
    spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f), spTexEffectData(nullptr) {}

MagicShaderHitEffect::MagicShaderHitEffect(TESObjectREFR* a1, TESEffectShader* a2, float a3)
  : MagicHitEffect(a1, nullptr), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(a2), fAlphaTimer(0.0f),
    spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f), spTexEffectData(nullptr) {
    if (pTarget == pPlayer) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        b3rdPerson = p3D && !p3D->GetAppCulled();
    }
    if (!(a3 < 0.0)) {
        fLifetime = a3;
    }
    else {
        fLifetime = 3.4028235e+38f;
    }
    if (pEffectData && pEffectData->Data.pAddonModels) {
        for (BSSimpleList<BGSDebrisData*>* pdebrisList = pEffectData->Data.pAddonModels->GetData();
             pdebrisList && !pdebrisList->IsEmpty(); pdebrisList = pdebrisList->m_pkNext) {
            pModelLoader->LoadFile(pdebrisList->m_item->pFileName, LOD_MULT_NONE, true, false, false, false);
        }
    }
}

MagicShaderHitEffect::MagicShaderHitEffect(TESObjectREFR* a1, ActiveEffect* a2)
  : MagicHitEffect(a1, a2), bWeaponEnchantment(false), pWornObject(nullptr), pEffectData(nullptr), fAlphaTimer(0.0f),
    spParticleNode(nullptr), b3rdPerson(false), fAddonAlpha(1.0f), fAddonScale(1.0f), spTexEffectData(nullptr) {
    EffectSetting* AVEffect = a2->pSpell->GetAVEffect(MagicSystem::SELF);
    pEffectData = AVEffect->data.pEffectShader;
    if (pTarget == pPlayer) {
        NiAVObject* p3D = pPlayer->Get3D(false);
        b3rdPerson = p3D && !p3D->GetAppCulled();
    }
    if (pEffectData && pEffectData->Data.pAddonModels) {
        for (BSSimpleList<BGSDebrisData*>* pdebrisList = pEffectData->Data.pAddonModels->GetData();
             pdebrisList && !pdebrisList->IsEmpty(); pdebrisList = pdebrisList->m_pkNext) {
            pModelLoader->LoadFile(pdebrisList->m_item->pFileName, LOD_MULT_NONE, true, false, false, false);
        }
    }
}

MagicShaderHitEffect::~MagicShaderHitEffect() {
    if (pTarget != pPlayer && pCell == nullptr) {
        pTarget = nullptr;
    }
    if (spTexEffectData.m_pObject) {
        if (pTarget == pPlayer) {
            NiAVObject* pav = pPlayer->Get3D(true);
            if (pav) {
                pEffectData->StopTextureShader(pav, spTexEffectData.m_pObject);
            }
            NiAVObject* pav_2 = pPlayer->Get3D(false);
            if (pav_2) {
                pEffectData->StopTextureShader(pav_2, spTexEffectData.m_pObject);
            }
        }
        else if (pTarget) {
            NiAVObject* pav_3 = pTarget->Get3D();
            if (pav_3) {
                pEffectData->StopTextureShader(pav_3, spTexEffectData.m_pObject);
            }
        }
    }
    if (spParticleNode.m_pObject) {
        for (int32_t i = 0; i < ParticlePropertyArray.iSize; ++i) {
            pEffectData->StopParticleShader(spParticleNode.m_pObject, ParticlePropertyArray[i].m_pObject);
        }
    }
    for (int32_t i = 0; i < AddOnObjects.iSize; ++i) {
        NiAVObject* pobject = AddOnObjects[i].m_pObject;
        if (pobject->m_pkParent) {
            pobject->m_pkParent->DetachChild(pobject);
        }
    }
    if (spParticleNode.m_pObject) {
        spParticleNode.m_pObject->AttachParent(nullptr);
    }
    if (pEffectData) {
        char apDest[264];
        // The two effect shaders whose moving sounds stop with the effect.
        if (pTarget && (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51)) {
            (&BSAudio::QInstance())->StopMovingSounds(pTarget->Get3D(), 1.0f, 0);
        }
        if (pEffectData->ParticleShaderTexture.GetTextureName()) {
            BSsprintf(apDest, 260, "%s\\%s", "Textures", pEffectData->ParticleShaderTexture.GetTextureName());
            TES_FreeTexture(TES::pInstance, apDest, 0);
        }
        if (pEffectData->GetTextureShaderTexture()->GetTextureNameLength()) {
            BSsprintf(apDest, 260, "%s\\%s", "Textures", pEffectData->GetTextureShaderTexture()->GetTextureName());
            TES_FreeTexture(TES::pInstance, apDest, 0);
        }
        if (pEffectData->Data.pAddonModels) {
            for (BSSimpleList<BGSDebrisData*>* pdebrisList = pEffectData->Data.pAddonModels->GetData();
                 pdebrisList && !pdebrisList->IsEmpty(); pdebrisList = pdebrisList->m_pkNext) {
                pModelLoader->ReleaseFile(pdebrisList->m_item->pFileName);
            }
        }
    }
}

bool MagicShaderHitEffect::Update(float afTimeDelta) {
    bool bdone;
    float falpha;
    if (!MagicHitEffect::Update(afTimeDelta) || (!pTarget || !pTarget->Get3D())) {
        goto LABEL_4;
    }
    fAlphaTimer = fAlphaTimer + afTimeDelta;
    bdone = bFinished;
    UpdatePosition();
    falpha = 1.0f;
    if (pTarget->IsActor() && pEffectData != MagicSystem::GetDetectLifeShaderEffect()) {
        if ((spTexEffectData.m_pObject) && (spTexEffectData.m_pObject->spBlockOutTexture.m_pObject) &&
            (pTarget->IsActor() && static_cast<Actor*>(pTarget)->Actor::GetAlphaMult() > 0.9900000095367432)) {
            static_cast<Actor*>(pTarget)->Actor::SetAlphaMult(0.99f);
            static_cast<Actor*>(pTarget)->Actor::UpdateAlpha();
        }

        BSFadeNode* pfadeNode = static_cast<BSFadeNode*>(pTarget->Get3D());
        if (NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pfadeNode)) {
            falpha = pfadeNode->GetLastFadePercent();
        }
        else {
            falpha = static_cast<Actor*>(pTarget)->GetAlpha();
        }
        float fmult = static_cast<Actor*>(pTarget)->Actor::GetAlphaMult();
        if (fmult > 0.0) {
            falpha = falpha / fmult;
        }
    }
    if ((ParticlePropertyArray.iSize) && (spParticleNode != static_cast<NiNode*>(nullptr))) {
        float flastParticleCount = 0.0f;
        for (int32_t i = 0; i < ParticlePropertyArray.iSize; ++i) {
            double BranchDimming = (&ParticlePropertyArray[i])->m_pObject->fMaxParticleRatio;
            flastParticleCount =
                static_cast<double>(ParticleShader::GetMaxInstanceSize()) * BranchDimming + flastParticleCount;
        }
        float fparticleCount =
            pEffectData->AnimateParticleEffectBirthrate(flastParticleCount, afTimeDelta, fAlphaTimer, bFinished);
        if (fparticleCount > 0.0) {
            bdone = false;
        }
        for (int32_t i_2 = 0; i_2 < ParticlePropertyArray.iSize; ++i_2) {
            float a2a = static_cast<float>(ParticleShader::GetMaxInstanceSize());
            float fparticleCountForThisPass = BSMIN(fparticleCount, a2a);
            fparticleCount = fparticleCount - fparticleCountForThisPass;
            ParticleShaderProperty* pparticleProp = (&ParticlePropertyArray[i_2])->m_pObject;
            float v18 = fparticleCountForThisPass / static_cast<double>(ParticleShader::GetMaxInstanceSize());
            pparticleProp->SetMaxParticleRatio(v18);
            pparticleProp->SetFade(falpha);
            pparticleProp->UpdateParticles(fAlphaTimer, false, true);
            if (pparticleProp->iParticleCount > 0) {
                bdone = false;
            }
        }
        if (AddOnObjects.iSize) {
            float a2;
            double addonScaleOutTime;
            double addonFadeOutTime;
            if (bFinished) {
                addonFadeOutTime = pEffectData->Data.fAddOnFadeOutTime * -1.0;
            }
            else {
                addonFadeOutTime = pEffectData->Data.fAddOnFadeInTime;
            }
            float v23 = afTimeDelta / addonFadeOutTime;
            float v16 = fAddonAlpha + v23;
            float a2b = BSMIN(1.0f, v16);
            fAddonAlpha = BSMAX(0.0f, a2b);
            if (fAddonAlpha > 0.0) {
                bdone = false;
            }
            if (bFinished) {
                addonScaleOutTime = pEffectData->GetAddOnScaleOutTime() * -1.0;
            }
            else {
                addonScaleOutTime = pEffectData->Data.fAddOnScaleInTime;
            }
            float v24 = afTimeDelta / addonScaleOutTime;
            float v14 = fAddonScale + v24;
            float a2c = BSMIN(1.0f, v14);
            fAddonScale = BSMAX(0.0f, a2c);
            if (bFinished) {
                a2 = pEffectData->Data.fAddOnScaleEnd;
            }
            else {
                a2 = pEffectData->Data.fAddOnScaleStart;
            }
            float fscaleLow = BSMAX(0.01f, a2);
            float fscale = (1.0f - fscaleLow) * fAddonScale + fscaleLow;
            for (uint32_t j = 0; j < AddOnObjects.iSize; ++j) {
                NiAVObject* v12 = (&AddOnObjects[j])->m_pObject;
                float a2d = fAddonAlpha;
                NiNode* v3 = v12->IsNode();
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(v3, a2d);
                (&AddOnObjects[j])->m_pObject->SetScale(fscale);
            }
        }
    }

    if (spTexEffectData.m_pObject) {
        pEffectData->AnimateTextureEffect(
            spTexEffectData.m_pObject, afTimeDelta, fAlphaTimer, this->MagicHitEffect::fAge, bFinished
        );
        float* p_a = &spTexEffectData.m_pObject->CurrentFillColor.a;
        *p_a = *p_a * falpha;
        float* v10 = &spTexEffectData.m_pObject->CurrentRimColor.a;
        *v10 = *v10 * falpha;
        if (spTexEffectData.m_pObject->RimColor.a > 0.0 || spTexEffectData.m_pObject->FillColor.a > 0.0) {
            bdone = false;
        }
    }
    if (bFinished && bdone) {
        if ((pTarget->IsActor()) && (static_cast<Actor*>(pTarget)->GetCurrentAIProcess())) {
            BaseProcess* CurrentAIProcess = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
            CurrentAIProcess->SetShouldRefreshMagicShadersEffects(true, true, false);
        }

    LABEL_4:;
        ClearTarget();
        return false;
    }
    return true;
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWeaponEnchantment), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAlphaTimer), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAddonAlpha), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAddonScale), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&b3rdPerson), 1u, 0);
    apSaveGameBuffer->SaveFormID(pWornObject, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveFormID(pEffectData, static_cast<uint32_t>(0));
}

void MagicShaderHitEffect::SaveGame() {
    uint32_t apFormID;
    int32_t a2;
    a2 = pTarget->GetFormID();
    pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&a2), 4u);
    apFormID = pEffectData->GetFormID();
    pSaveLoadGameOLD->SaveNumericID(&apFormID, 4u);
    SaveGame(static_cast<ActiveEffect*>(nullptr), pTarget);
}

void MagicShaderHitEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWeaponEnchantment), 1u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAlphaTimer), 4u);
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAddonAlpha), 4u);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAddonScale), 4u);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&b3rdPerson), 1u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pWornObject = dynamic_cast<TESBoundObject*>(FormByNumericID);
    uint32_t v4 = apLoadGameBuffer->LoadFormID();
    TESForm* v5 = TESForm::GetFormByNumericID(v4);
    pEffectData = dynamic_cast<TESEffectShader*>(v5);
}

bool MagicShaderHitEffect::LoadGame() {
    TESObjectCELL* ParentCell;
    uint32_t aiSearchID;
    uint32_t apFormID;
    bool v9 = true;
    pSaveLoadGameOLD->LoadNumericID(&apFormID, 4u);
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(apFormID);
    pTarget = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    if (!pTarget ||
        (!pTarget->GetParentCell() || (ParentCell = pTarget->GetParentCell(), !ParentCell->GetAttached()))) {
        v9 = false;
    }
    pSaveLoadGameOLD->LoadNumericID(&aiSearchID, 4u);
    TESForm* v3 = TESForm::GetFormByNumericID(aiSearchID);
    char* v7 = reinterpret_cast<char*>(dynamic_cast<TESEffectShader*>(v3));
    if (!v7) {
        v9 = false;
    }
    LoadGame(static_cast<ActiveEffect*>(nullptr), pTarget);
    if (v9) {
        InitLoadGame(static_cast<ActiveEffect*>(nullptr), pTarget);
        FinishInitLoadGame(nullptr, pTarget, v7);
    }
    return v9;
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
    char path_1[260];
    char v47[260];
    char path[260];
    if (pEffect) {
        this->MagicHitEffect::fAge = pEffect->fElapsedSeconds;
        EffectSetting* EffectSetting_ = pEffect->GetEffectSetting();
        bFinished = !EffectSetting_->IsFlagSet(EffectSetting::PERSIST);
        if (pEffect->IsFlagSet(ActiveEffect::NO_INTIAL_FLARE)) {
            fAlphaTimer = 100.0f;
        }
    }
    bool bsword = false;
    bool bactorRoot = false;
    float fswordLength = 0.0f;
    bool switching = false;
    if ((pTarget == pPlayer) && (pPlayer->Get3D(false))) {
        NiAVObject* v2 = pPlayer->Get3D(false);
        bool b3rd = !v2->GetAppCulled();
        bool v32 = this->MagicHitEffect::fAge > 0.0 && b3rd != b3rdPerson;
        switching = v32;
        b3rdPerson = b3rd;
    }

    NiAVObject* pmembraneRoot = nullptr;
    NiAVObject* pparticleRoot = nullptr;
    GetAffectedScenegraph(bactorRoot, bsword, fswordLength, pmembraneRoot, pparticleRoot);
    if (!pmembraneRoot || (!pparticleRoot || !pEffectData)) {
        return false;
    }
    // The form IDs of two effect shaders that play the fire loop.
    if (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51) {
        BSAudio* v3 = &BSAudio::QInstance();
        BSSoundHandle handle = v3->GetSoundHandleByName("AMBFireMediumLP", 18u);
        NiPoint3 v61 = pTarget->GetLocationOnReference();
        handle.SetPosition(v61.x, v61.y, v61.z);
        handle.SetVolume(1.0f);
        handle.Play(true);
        NiNode* v4 = pTarget->Get3D();
        handle.SetObjectToFollow(v4);
    }
    bool bdoParticles = true;
    if ((bWeaponEnchantment && pTarget->IsActor()) && (static_cast<Actor*>(pTarget)->GetCurrentAIProcess())) {
        BaseProcess* CurrentAIProcess = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
        if (CurrentAIProcess->IsCurrentWeaponGrenadeOrMine()) {
            bdoParticles = false;
        }
        BaseProcess* v30 = static_cast<Actor*>(pTarget)->GetCurrentAIProcess();
        if (!v30->GetWeaponDrawn() && pPlayer->GetMagicShaderHitEffect() != this) {
            bdoParticles = false;
        }
    }

    if (!bdoParticles || pEffectData->IsFlagSet(TESEffectShader::DISABLE_PARTICLE_SHADER)) {
        if ((ParticlePropertyArray.iSize) && (spParticleNode.m_pObject)) {
            for (int32_t i_2 = 0; i_2 < ParticlePropertyArray.iSize; ++i_2) {
                pEffectData->StopParticleShader(spParticleNode.m_pObject, (&ParticlePropertyArray[i_2])->m_pObject);
            }
            ParticlePropertyArray.Clear(true);
            if (spParticleNode.m_pObject) {
                spParticleNode.m_pObject->AttachParent(nullptr);
            }
            spParticleNode = static_cast<NiNode*>(nullptr);
        }
    }
    else {
        if (spParticleNode == static_cast<NiNode*>(nullptr)) {
            NiNode* v29;
            v29 = new NiNode(0);
            spParticleNode = v29;
            spParticleNode.m_pObject->SetName("ParticleShader Geometry");
            spParticleNode.m_pObject->SetRotate(NiMatrix3_IDENTITY);
        }
        if (!ParticlePropertyArray.iSize) {
            NiPointer<NiSourceTexture> sptexture(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->ParticleShaderTexture.GetTextureNameLength()) {
                BSsprintf(path, 260, "%s\\%s", "Textures", pEffectData->ParticleShaderTexture.GetTextureName());
                TES::pInstance->CreateTextureImage(path, sptexture, false, false);
                float arPos_8 = pEffectData->Data.fParticlePersistent;
                float arPos_4 = pEffectData->Data.fParticleFull;
                int32_t imaxParticles = static_cast<int32_t>(BSMAX(arPos_4, arPos_8));
                int32_t iparticlesPerPass = ParticleShader::GetMaxInstanceSize();
                for (int32_t ipassCount = 0; iparticlesPerPass * ipassCount < imaxParticles; ++ipassCount) {
                    float arPos_8a = fAlphaTimer;
                    NiSourceTexture* arPos_4a = sptexture.m_pObject;
                    NiPointer<ParticleShaderProperty> particleProperty(
                        pEffectData->StartParticleShader(spParticleNode.m_pObject, pparticleRoot, arPos_4a, arPos_8a)
                    );
                    ParticlePropertyArray.Add(particleProperty);
                }
            }
        }
        for (int32_t i = 0;; ++i) {
            uint32_t FormFlags = ParticlePropertyArray.iSize;
            if (i >= FormFlags) {
                break;
            }
            ParticleShaderProperty* particleProperty_2 = (&ParticlePropertyArray[i])->m_pObject;
            if (bsword) {
                particleProperty_2->SetEmitterTypeRay(fswordLength);
            }
            if (bWeaponEnchantment) {
                particleProperty_2->SetByte6C(false);
            }
            if (pTarget) {
                TESObjectACTI* pactivator = dynamic_cast<TESObjectACTI*>(pTarget->GetObjectReference());
                if (bactorRoot || pactivator) {
                    if (pTarget != pPlayer || b3rdPerson) {
                        particleProperty_2->SetEmitterTypeCollision();
                    }
                    else {
                        particleProperty_2->SetEmitterTypeGeometry();
                    }
                }
            }
            particleProperty_2->CollectTargets(pparticleRoot);
            if (switching) {
                particleProperty_2->InitParticleData(0.0f);
                float v27 = fAlphaTimer - 0.16599999368190765;
                particleProperty_2->UpdateParticles(v27, false, true);
                particleProperty_2->UpdateParticles(fAlphaTimer, false, true);
            }
        }
        UpdatePosition();
    }
    if (!pEffectData->IsFlagSet(TESEffectShader::DISABLE_TEXTURE_SHADER)) {
        if (spTexEffectData == static_cast<BSShaderPPLightingProperty::TextureEffectData*>(nullptr)) {
            NiPointer<NiSourceTexture> sptexture_2(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->GetTextureShaderTexture()->GetTextureNameLength()) {
                BSsprintf(v47, 260, "%s\\%s", "Textures", pEffectData->GetTextureShaderTexture()->GetTextureName());
                TES::pInstance->CreateTextureImage(v47, sptexture_2, false, false);
            }
            NiPointer<NiSourceTexture> spblockoutTexture(static_cast<NiSourceTexture*>(nullptr));
            if (pEffectData->GetBlockOutTexture()->GetTextureNameLength()) {
                BSsprintf(path_1, 260, "%s\\%s", "Textures", pEffectData->GetBlockOutTexture()->GetTextureName());
                TES::pInstance->CreateTextureImage(path_1, spblockoutTexture, false, false);
            }
            NiSourceTexture* arPos_8b = spblockoutTexture.m_pObject;
            NiSourceTexture* arPos_4b = sptexture_2.m_pObject;
            spTexEffectData = pEffectData->CreateTextureShaderData(pmembraneRoot, arPos_4b, arPos_8b);
            if ((spTexEffectData.m_pObject) && (spTexEffectData.m_pObject->spBlockOutTexture.m_pObject) && (pTarget) &&
                (pTarget->IsActor() && static_cast<Actor*>(pTarget)->Actor::GetAlphaMult() > 0.9900000095367432)) {
                static_cast<Actor*>(pTarget)->Actor::SetAlphaMult(0.99f);
                static_cast<Actor*>(pTarget)->Actor::UpdateAlpha();
            }
        }
        BSShaderPPLightingProperty::TextureEffectData* arPos_8c = spTexEffectData.m_pObject;
        pEffectData->StartTextureShader(pmembraneRoot, arPos_8c);
    }
    if ((!switching && !AddOnObjects.iSize) && (pEffectData) && (pEffectData->Data.pAddonModels) &&
        (ParticlePropertyArray.iSize) && ((&ParticlePropertyArray[0])->m_pObject)) {
        NiTObjectArray<NiPointer<NiAVObject>>* TargetArray = (&ParticlePropertyArray[0])->m_pObject->GetTargetArray();
        if (TargetArray->m_usSize) {
            BSWeightedRandom<char*> randomDebris;
            for (BSSimpleList<BGSDebrisData*>* pdebrisList = pEffectData->Data.pAddonModels->GetData();
                 pdebrisList && !pdebrisList->IsEmpty(); pdebrisList = pdebrisList->m_pkNext) {
                char* pFileName = pdebrisList->m_item->pFileName;
                pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
                uint32_t arPos_8d = pdebrisList->m_item->cPercentage;
                randomDebris.AddWeightedResult(pdebrisList->m_item->pFileName, arPos_8d);
            }
            ParticleShaderProperty* pparticleProp = (&ParticlePropertyArray[0])->m_pObject;
            for (int32_t i_3 = 0;; ++i_3) {
                NiTObjectArray<NiPointer<NiAVObject>>* v9 = pparticleProp->GetTargetArray();
                uint32_t Size = v9->m_usSize;
                if (i_3 >= Size) {
                    break;
                }
                NiAVObject* pobject = (&(*pparticleProp->GetTargetArray())[i_3])->m_pObject;
                if (pobject->IsNode()) {
                    NiPointer<Model> spmodel;
                    char* v11 = randomDebris.PickResult();
                    pModelLoader->FindModel(v11, spmodel);
                    if ((spmodel.m_pObject) && (spmodel.m_pObject->GetModel())) {
                        BSCloningProcess clone(1.0f);
                        NiAVObject* paddon = static_cast<NiAVObject*>(spmodel.m_pObject->GetModel()->Clone(clone));
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            NiNode* arPos = pobject->IsNode();
                            NiAVObject* v15 = paddon;
                            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                            Instance->QueueNodeAttach(v15, arPos, true, false);
                        }
                        else {
                            NiNode* fScale = pobject->IsNode();
                            fScale->AttachChild(paddon, 1);
                        }
                        fAddonAlpha = 0.0f;
                        float arPos_8e = pEffectData->Data.fAddOnScaleStart;
                        fAddonScale = BSMAX(0.01f, arPos_8e);
                        paddon->SetScale(fAddonScale);
                        NiTimeController::StartAnimations(paddon);
                        NiPointer<NiAVObject> spaddon(paddon);
                        AddOnObjects.Add(spaddon);
                    }
                }
            }
        }
    }

    return ParticlePropertyArray.iSize || spTexEffectData.m_pObject;
}

void MagicShaderHitEffect::ClearTarget() {
    if ((pTarget) && (pEffectData) && (pEffectData->iFormID == 0x852FE || pEffectData->iFormID == 0x84A51)) {
        NiNode* v1 = pTarget->Get3D();
        (&BSAudio::QInstance())->StopMovingSounds(v1, 1.0f, 0);
    }

    if (spTexEffectData.m_pObject) {
        if (pTarget == pPlayer) {
            NiAVObject* pav = pPlayer->Get3D(true);
            if (pav) {
                pEffectData->StopTextureShader(pav, spTexEffectData.m_pObject);
            }
            NiAVObject* pav_2 = pPlayer->Get3D(false);
            if (pav_2) {
                pEffectData->StopTextureShader(pav_2, spTexEffectData.m_pObject);
            }
        }
        else {
            if (pTarget) {
                NiAVObject* pav_3 = pTarget->Get3D();
                if (pav_3) {
                    pEffectData->StopTextureShader(pav_3, spTexEffectData.m_pObject);
                }
            }
        }
    }
    if (spParticleNode != static_cast<NiNode*>(nullptr)) {
        for (int32_t i = 0; i < ParticlePropertyArray.iSize; ++i) {
            pEffectData->StopParticleShader(spParticleNode.m_pObject, (&ParticlePropertyArray[i])->m_pObject);
        }
    }
    ParticlePropertyArray.Clear(true);
    if (spParticleNode != static_cast<NiNode*>(nullptr)) {
        spParticleNode.m_pObject->AttachParent(nullptr);
    }
    spParticleNode = static_cast<NiNode*>(nullptr);
    spTexEffectData = static_cast<BSShaderPPLightingProperty::TextureEffectData*>(nullptr);
    pTarget = nullptr;
}

void MagicShaderHitEffect::UpdatePosition() {
    NiPoint3 v15;
    NiMatrix3 v16;
    NiMatrix3 apResult;
    NiMatrix3 targetRot;
    PlayerCharacter* Target = static_cast<PlayerCharacter*>(pTarget);
    bool bplayer = Target == pPlayer;
    bool v12 = Target == pPlayer && (pPlayer && pPlayer->GetMagicShaderHitEffect() == this);
    bool v11 = false;
    if ((bplayer) && (pPlayer) && (pPlayer->Get3D(false))) {
        NiAVObject* ptarget = pPlayer->Get3D(false);
        if (ptarget->GetAppCulled()) {
            v11 = true;
        }
    }

    NiAVObject* ptarget_2 = nullptr;
    if (bWeaponEnchantment) {
        NiAVObject* pparticleTarget;
        NiAVObject* pmembraneTarget;
        bool bunused;
        float funused;
        GetAffectedScenegraph(bunused, bunused, funused, pmembraneTarget, pparticleTarget);
        ptarget_2 = pparticleTarget;
    }
    else {
        if (bplayer) {
            ptarget_2 = pPlayer->Get3D(false);
        }
        else {
            ptarget_2 = pTarget->Get3D();
        }
    }
    if ((spParticleNode != static_cast<NiNode*>(nullptr)) && (ptarget_2)) {
        const void* WorldTransform;
        if (bWeaponEnchantment) {
            WorldTransform = &ptarget_2->GetWorldTransform();
        }
        else {
            WorldTransform = &NiMatrix3_IDENTITY;
        }
        rt::memcpy(&targetRot, WorldTransform, sizeof(targetRot));
        if (!v12) {
            if (v11 && pPlayer->Get3D(true)) {
                NiAVObject* v3 = pPlayer->Get3D(true);
                NiNode* pnode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, v3));
                if ((pnode) && (pnode != spParticleNode.m_pObject->m_pkParent)) {
                    pnode->AttachChild(spParticleNode.m_pObject, 1);
                }

                if (v11) {
                    NiMatrix3* v4 =
                        (&spParticleNode.m_pObject->m_pkParent->GetWorldTransform())->m_Rotate.Inverse(&apResult);
                    spParticleNode.m_pObject->SetRotate(*v4 * targetRot);
                    const NiPoint3* WorldTranslate = &spParticleNode.m_pObject->m_pkParent->GetWorldTranslate();
                    NiPoint3* v9 = ::new (&v15) NiPoint3(ptarget_2->GetWorldTranslate() - *WorldTranslate);
                    NiMatrix3* v6 =
                        (&spParticleNode.m_pObject->m_pkParent->GetWorldTransform())->m_Rotate.Inverse(&v16);
                    spParticleNode.m_pObject->SetTranslate(*v6 * *v9);
                    NiUpdateData UpdateData(0.0f, false, false);
                    spParticleNode.m_pObject->Update(UpdateData);
                }
                else {
                    spParticleNode.m_pObject->SetRotate(targetRot);
                    spParticleNode.m_pObject->SetTranslate(ptarget_2->GetWorldTranslate());
                }
            }
            else {
                spParticleNode.m_pObject->SetTranslate(ptarget_2->GetWorldTranslate());
                spParticleNode.m_pObject->SetRotate(targetRot);
            }
        }
    }

    if (v11 || v12) {
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
    pSaveLoadGameOLD->SaveGameDataOLD(&fAlphaTimer, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bWeaponEnchantment, 1);
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
    pSaveLoadGameOLD->LoadGameDataOLD(&fAlphaTimer, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bWeaponEnchantment, 1);
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
        EffectSetting* AVEffect = apEffect->pSpell->GetAVEffect(MagicSystem::SELF);
        pEffectData = AVEffect->data.pEffectShader;
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

void MagicShaderHitEffect::SetLifeTime(float a2) {
    fAlphaTimer = a2;
}

void MagicShaderHitEffect::SetFinished(bool a1) {
    bFinished = a1;
}

void MagicShaderHitEffect::GetAffectedScenegraph(
    bool& abActorRoot,
    bool& abSword,
    float& afSwordLength,
    NiAVObject*& apMembraneRoot,
    NiAVObject*& apParticleRoot
) {
    bool bdisintigrateWeapon = false;
    bool disintigrateArmor = false;
    NiAVObject* pav = nullptr;
    bool binventoryEnchantment = false;
    Actor* pactor = dynamic_cast<Actor*>(pTarget);
    BipedAnim* pbiped = nullptr;
    if (pactor) {
        if (pactor == pPlayer) {
            if (pPlayer->GetInventory3D() && pPlayer->GetMagicShaderHitEffect() == this) {
                binventoryEnchantment = true;
            }
            else {
                pbiped = pPlayer->GetBiped(!b3rdPerson);
            }
        }
        else {
            pbiped = pactor->GetBiped();
        }
    }
    if (bWeaponEnchantment) {
        if ((pTarget) && (pTarget->IsActor())) {
            Actor* pactor_2 = static_cast<Actor*>(pTarget);
            BaseProcess* pprocess = pactor_2->pCurrentProcess;
            if (pprocess) {
                pprocess->GetCurrentWeapon();
            }
        }
    }
    else {
        if (pTarget) {
            pav = pTarget->Get3D();
            if (pTarget->IsActor()) {
                if (pTarget == pPlayer) {
                    pav = pPlayer->Get3D(!b3rdPerson);
                }
                abActorRoot = true;
            }
            else {
                if ((dynamic_cast<TESObjectWEAP*>(pTarget->GetObjectReference())) && (pav)) {
                    abSword = true;
                    const NiBound* WorldBound = &pav->GetWorldBound();
                    if (WorldBound->m_fRadius <= 0.0) {
                        NiUpdateData UpdateData(0.0f, false, false);
                        pav->Update(UpdateData);
                    }
                    const NiPoint3* WorldTranslate = &pav->GetWorldTranslate();
                    const NiBound* v7 = &pav->GetWorldBound();
                    const NiPoint3& kCenter = v7->m_kCenter;
                    NiPoint3 diff = kCenter - *WorldTranslate;
                    double v10 = diff.Length();
                    const NiBound* v8 = &pav->GetWorldBound();
                    afSwordLength = v8->m_fRadius + v10;
                }
            }
        }
    }
    apMembraneRoot = pav;
    apParticleRoot = pav;
}

void MagicShaderHitEffect::ResetAlphaTimer() {
    float v4 = 0.0f;
    if (spTexEffectData.m_pObject) {
        v4 = BSMAX(spTexEffectData.m_pObject->FillColor.a, spTexEffectData.m_pObject->RimColor.a);
    }
    if (bWeaponEnchantment) {
        v4 = 0.0f;
        spTexEffectData.m_pObject->FillColor.a = 0.0f;
        spTexEffectData.m_pObject->RimColor.a = 0.0f;
    }
    if (!(static_cast<double>(v4) < pEffectData->Data.fFillAlphaFullPercent)) {
        fAlphaTimer = pEffectData->Data.fFillAlphaFadeInTime;
    }
    else {
        double BeginKeyTime = pEffectData->Data.fFillAlphaFadeInTime;
        fAlphaTimer = v4 / pEffectData->Data.fFillAlphaFullPercent * BeginKeyTime;
    }
    for (uint32_t auiIndex = 0; auiIndex < ParticlePropertyArray.iSize; ++auiIndex) {
        (&ParticlePropertyArray[auiIndex])->m_pObject->RewindTimer(fAlphaTimer);
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
