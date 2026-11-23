#include "Fallout/AI/Combat/MuzzleFlash.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

MuzzleFlash::MuzzleFlash(Projectile* apProjectile, TESObjectREFR* apPlayer)
  : bEnabled(false), bMPSEnabled(false), bUpdateLight(false), fEnableTimer(0.0f), spAVObject(nullptr), spLight(nullptr),
    pProjectile(apProjectile ? apProjectile->GetProjectileBase() : nullptr),
    pWeaponSource(apProjectile ? apProjectile->pWeaponSource : nullptr), pParentReference(apPlayer) {
    fDurationTimer = pProjectile->Data.fMuzzleFlashDuration;
    Init(apPlayer);
}

MuzzleFlash::~MuzzleFlash() {
    TESModel* pModel = GetMuzzleFlashModel();
    if (pModel->GetModelLength()) {
        pModelLoader->ReleaseFile(pModel->GetModel());
    }
    if (spAVObject.m_pObject && spAVObject->m_pkParent) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueNodeDetach(spAVObject.m_pObject);
        }
        else {
            spAVObject->m_pkParent->DetachChild(spAVObject.m_pObject);
        }
    }
    spAVObject = nullptr;
    DetachLight();
}

void MuzzleFlash::Update(float afDelta, TESObjectREFR* apParentRef) {
    if (spAVObject.m_pObject) {
        Position3D(apParentRef);
        spAVObject->SetAppCulled(!bEnabled);
        if (bEnabled && !bMPSEnabled) {
            NiNode* pNode = static_cast<NiNode*>(spAVObject.m_pObject)->IsNode();
            TESObjectREFR::AddMasterParticleAddonNodes(pNode);
            bMPSEnabled = true;
        }
        if (!bEnabled && bMPSEnabled) {
            NiNode* pNode = spAVObject->IsNode();
            TESObjectREFR::RemoveMasterParticleAddonNodes(pNode);
            bMPSEnabled = false;
        }
        if (bUpdateLight) {
            UpdateLight();
        }
        if ((pVATS.eMode == VATS::VATS_PLAYBACK && BSTimer::QGlobalTimeMultiplier() > 0.0) ||
            (apParentRef->IsActor() && static_cast<Actor*>(apParentRef)->GetActorValue(ActorValue::TURBO) > 0)) {
            afDelta = afDelta / BSTimer::QGlobalTimeMultiplier();
        }
        fEnableTimer = fEnableTimer - afDelta;
        if (bEnabled) {
            fDurationTimer = fDurationTimer - afDelta;
            if (fDurationTimer <= 0.0) {
                fDurationTimer = pProjectile->Data.fMuzzleFlashDuration;
                fEnableTimer = 0.1f;
                bEnabled = false;
                bUpdateLight = true;
            }
        }
    }
}

bool MuzzleFlash::GetEnabled() {
    return bEnabled;
}

TESObjectREFR* MuzzleFlash::GetParentRef() {
    return pParentReference;
}

void MuzzleFlash::SetDuration(float afDuration) {
    fDurationTimer = afDuration;
}

void MuzzleFlash::UpdateLight() {
    if (spLight.m_pObject) {
        if (bEnabled) {
            NiLight* pLight = spLight.m_pObject;
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
            if (!pShadowSceneNode->GetLight(pLight)) {
                pLight = spLight.m_pObject;
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddLight(pLight, true);
            }
        }
        float fDimmer = bEnabled ? 1.0f : 0.0f;
        spLight->SetDimmer(fDimmer);
    }
}

void MuzzleFlash::Enable() {
    if (fEnableTimer <= 0.0) {
        bEnabled = true;
        bUpdateLight = true;
        fDurationTimer = pProjectile->Data.fMuzzleFlashDuration;
    }
}

TESModel* MuzzleFlash::GetMuzzleFlashModel() {
    if (pProjectile && pProjectile->GetHasMuzzleFlash()) {
        return &pProjectile->GetMuzzleFlashModel();
    }

    return nullptr;
}

void MuzzleFlash::DetachLight() {
    if (spLight.m_pObject) {
        NiLight* pLight = spLight.m_pObject;
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveLight(pLight);
        spLight = nullptr;
    }
}

void MuzzleFlash::Init(TESObjectREFR* apParentRef) {
    TESModel* pModel = GetMuzzleFlashModel();
    if (pModel && pModel->GetModelLength()) {
        const char* pFileName = pModel->GetModel();
        NiNode* pObject = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
        if (pObject) {
            spAVObject = static_cast<NiAVObject*>(pObject->Clone());
        }
    }

    if (spAVObject.m_pObject) {
        TESHavokUtilities::RemoveHavokFromSceneGraph(spAVObject.m_pObject);
        Position3D(apParentRef);
        AttachLight();
        NiTimeController::StartAnimations(spAVObject.m_pObject);
        spAVObject->UpdateProperties();
        float fPrevTime = fCellAppTime - 0.032999999821186066;
        NiUpdateData kUpdateData(fPrevTime, true, false);
        spAVObject->Update(kUpdateData);
        kUpdateData.fTime = fCellAppTime;
        spAVObject->Update(kUpdateData);
    }
}

void MuzzleFlash::Position3D(TESObjectREFR* apParentRef) {
    NiTransform kDest;
    NiMatrix3 kInvRotate;
    if (spAVObject.m_pObject && apParentRef && apParentRef->GetParentCell() && apParentRef->Get3D()) {
        NiAVObject* pprojectileNode;
        if (apParentRef->IsActor()) {
            pprojectileNode = apParentRef->GetFireNode();
        }
        else if (pWeaponSource) {
            NiNode* p3D = apParentRef->Get3D();
            pprojectileNode = pWeaponSource->GetFireNode(p3D);
        }
        else {
            pprojectileNode = nullptr;
        }
        bool bupdate = false;
        float fupdateTime = 0.0f;
        if (pprojectileNode) {
            NiNode* psceneParent;
            NiTransform ktrans;
            if (apParentRef == pPlayer) {
                if (pPlayer->IsActually3rdPerson()) {
                    psceneParent = pPlayer->Get3D();
                }
                else {
                    psceneParent = static_cast<NiNode*>(pPlayer->Get3D(true));
                }
                NiTransform kplayerInvTransform;
                if (psceneParent) {
                    kplayerInvTransform.m_Rotate = *::new (&kInvRotate) NiMatrix3(
                        psceneParent->GetWorldTransform().m_Rotate.Transpose()
                    );
                    kplayerInvTransform.m_fScale = 1.0 / psceneParent->GetWorldTransform().m_fScale;
                    kplayerInvTransform.m_Translate = -psceneParent->GetWorldTransform().m_Translate;
                    kplayerInvTransform.m_Translate = kplayerInvTransform.m_Rotate * kplayerInvTransform.m_Translate;
                    kplayerInvTransform.m_Translate *= kplayerInvTransform.m_fScale;
                }
                ktrans = *::new (&kDest) NiTransform(kplayerInvTransform * pprojectileNode->GetWorldTransform());
                bupdate = true;
                if (pPlayer->IsActually3rdPerson()) {
                    if (pPlayer->GetAnimation()) {
                        fupdateTime = pPlayer->GetAnimation()->GetTime();
                    }
                    else {
                        fupdateTime = 0.0f;
                    }
                }
                else if (pPlayer->GetAnimation(true)) {
                    fupdateTime = pPlayer->GetAnimation(true)->GetTime();
                }
                else {
                    fupdateTime = 0.0f;
                }
            }
            else {
                ktrans = pprojectileNode->GetWorldTransform();
                TESObjectCELL* pcell = apParentRef->GetParentCell();
                psceneParent = static_cast<NiNode*>(pcell->GetDynamicNode());
            }
            if (psceneParent) {
                if (psceneParent != spAVObject->m_pkParent) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        NiAVObject* pObject = spAVObject.m_pObject;
                        TaskQueueInterface::GetInstance()->QueueNodeAttach(pObject, psceneParent, true, false);
                    }
                    else {
                        psceneParent->AttachChild(spAVObject.m_pObject, 1);
                    }
                    NiNode* pNode = static_cast<NiNode*>(spAVObject.m_pObject)->IsNode();
                    BSShaderUtil::RecursiveSetPropertyFadeAlpha(pNode, 1.0f);
                }
            }
            spAVObject->SetTranslate(ktrans.m_Translate);
            spAVObject->SetRotate(ktrans.m_Rotate);
            spAVObject->SetScale(ktrans.m_fScale);
            if (bupdate) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    if (bwarnOnce) {
                        Warning("Trying to update a Muzzle Flash when not in the main thread");
                        bwarnOnce = false;
                    }
                }
                else {
                    NiUpdateData kUpdateData(fupdateTime, false, false);
                    spAVObject->Update(kUpdateData);
                }
            }
        }
    }
}

void MuzzleFlash::AttachLight() {
    if (spAVObject.m_pObject && pProjectile->Data.pMuzzleFlashLight) {
        spLight = pProjectile->Data.pMuzzleFlashLight->GenDynamic(nullptr, spAVObject.m_pObject, true);
    }
}

void MuzzleFlash::SpawnMuzzleFlash(TESObjectREFR* apParentRef, Projectile* apProjectile) {
    if (apParentRef && apProjectile) {
        if (apParentRef->IsActor()) {
            BaseProcess* pprocess = static_cast<Actor*>(apParentRef)->GetCurrentAIProcess();
            if (pprocess) {
                BGSProjectile* pBaseProjectile;
                TESObjectWEAP* pWeapon;
                if (!pprocess->GetCurrentMuzzleFlash() ||
                    (pBaseProjectile = pprocess->GetCurrentMuzzleFlash()->pProjectile,
                     pBaseProjectile != apProjectile->GetProjectileBase()) ||
                    (pWeapon = pprocess->GetCurrentMuzzleFlash()->pWeaponSource,
                     pWeapon != apProjectile->pWeaponSource)) {
                    pprocess->ClearMuzzleFlash();
                    pprocess->CreateMuzzleFlash(apProjectile, apParentRef);
                }
                if (pprocess->GetCurrentMuzzleFlash()) {
                    pprocess->GetCurrentMuzzleFlash()->Enable();
                }
            }
        }
        else {
            ReferenceProcessLists.SpawnReferenceMuzzleFlash(apProjectile, apParentRef);
        }
    }
}
