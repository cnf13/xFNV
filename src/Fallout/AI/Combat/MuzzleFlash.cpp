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
    if (spAVObject.m_pObject && spAVObject.m_pObject->m_pkParent) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueNodeDetach(spAVObject.m_pObject);
        }
        else {
            spAVObject.m_pObject->m_pkParent->DetachChild(spAVObject.m_pObject);
        }
    }
    spAVObject = nullptr;
    DetachLight();
}

void MuzzleFlash::Update(float afDelta, TESObjectREFR* apParentRef) {
    if (spAVObject.m_pObject) {
        Position3D(apParentRef);
        spAVObject.m_pObject->SetAppCulled(!bEnabled);
        if (bEnabled && !bMPSEnabled) {
            NiNode* v6 = static_cast<NiNode*>(spAVObject.m_pObject);
            NiNode* v3 = v6->IsNode();
            TESObjectREFR::AddMasterParticleAddonNodes(v3);
            bMPSEnabled = true;
        }
        if (!bEnabled && bMPSEnabled) {
            NiAVObject* v5 = spAVObject.m_pObject;
            NiNode* v4 = v5->IsNode();
            TESObjectREFR::RemoveMasterParticleAddonNodes(v4);
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

void MuzzleFlash::SetDuration(float a2) {
    fDurationTimer = a2;
}

void MuzzleFlash::UpdateLight() {
    ShadowSceneNode* shadowSceneNode;
    ShadowSceneNode* v2;
    NiLight* v3;
    NiLight* v4;
    NiLight* v5;
    float v6;
    if (spLight.m_pObject) {
        if (bEnabled) {
            v5 = spLight.m_pObject;
            shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
            if (!shadowSceneNode->GetLight(v5)) {
                v4 = spLight.m_pObject;
                v2 = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
                v2->AddLight(v4, true);
            }
        }
        if (bEnabled) {
            v6 = (float)1.0;
        }
        else {
            v6 = (float)0.0;
        }
        v3 = spLight.m_pObject;
        v3->SetDimmer(v6);
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
    else {
        return nullptr;
    }
}

void MuzzleFlash::DetachLight() {
    ShadowSceneNode* shadowSceneNode;
    NiLight* v2;
    if ((&spLight)->m_pObject) {
        v2 = (&spLight)->m_pObject;
        shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        shadowSceneNode->RemoveLight(v2);
        (&spLight)->operator=(nullptr);
    }
}

void MuzzleFlash::Init(TESObjectREFR* apParentRef) {
    TESModel* pModel = GetMuzzleFlashModel();
    if (pModel) {
        if (pModel->GetModelLength()) {
            const char* v2 = pModel->GetModel();
            NiNode* pObject = pModelLoader->LoadFile(v2, LOD_MULT_NONE, true, false, false, false);
            if (pObject) {
                spAVObject.operator=(static_cast<NiAVObject*>(pObject->Clone()));
            }
        }
    }
    if (spAVObject.m_pObject) {
        TESHavokUtilities::RemoveHavokFromSceneGraph(spAVObject.m_pObject);
        Position3D(apParentRef);
        AttachLight();
        NiTimeController::StartAnimations(spAVObject.m_pObject);
        spAVObject.m_pObject->UpdateProperties();
        float fPrevTime = fCellAppTime - 0.032999999821186066;
        NiUpdateData kUpdateData(fPrevTime, true, false);
        spAVObject.m_pObject->Update(kUpdateData);
        kUpdateData.fTime = fCellAppTime;
        spAVObject.m_pObject->Update(kUpdateData);
    }
}

void MuzzleFlash::Position3D(TESObjectREFR* apParentRef) {
    NiTransform kDest;
    NiMatrix3 result;
    if (spAVObject.m_pObject) {
        if (apParentRef) {
            if (apParentRef->GetParentCell()) {
                if (apParentRef->Get3D()) {
                    NiAVObject* pprojectileNode;
                    if (apParentRef->IsActor()) {
                        pprojectileNode = apParentRef->GetFireNode();
                    }
                    else {
                        NiAVObject* pprojectileNode_2;
                        if (pWeaponSource) {
                            NiNode* v2 = apParentRef->Get3D();
                            pprojectileNode_2 = pWeaponSource->GetFireNode(v2);
                        }
                        else {
                            pprojectileNode_2 = nullptr;
                        }
                        pprojectileNode = pprojectileNode_2;
                    }
                    bool bupdate = false;
                    float fupdateTime_2 = 0.0f;
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
                                kplayerInvTransform.m_Rotate =
                                    *::new (&result)
                                        NiMatrix3((&psceneParent->GetWorldTransform())->m_Rotate.Transpose());
                                kplayerInvTransform.m_fScale = 1.0 / (&psceneParent->GetWorldTransform())->m_fScale;
                                kplayerInvTransform.m_Translate = -(&psceneParent->GetWorldTransform())->m_Translate;
                                kplayerInvTransform.m_Translate =
                                    kplayerInvTransform.m_Rotate * kplayerInvTransform.m_Translate;
                                kplayerInvTransform.m_Translate.operator*=(kplayerInvTransform.m_fScale);
                            }
                            ktrans =
                                *::new (&kDest) NiTransform(kplayerInvTransform * pprojectileNode->GetWorldTransform());
                            bupdate = true;
                            if (pPlayer->IsActually3rdPerson()) {
                                float fupdateTime;
                                if (pPlayer->GetAnimation()) {
                                    Animation* v4 = pPlayer->GetAnimation();
                                    fupdateTime = v4->GetTime();
                                }
                                else {
                                    fupdateTime = 0.0f;
                                }
                                fupdateTime_2 = fupdateTime;
                            }
                            else {
                                float fupdateTime_3;
                                if (pPlayer->GetAnimation(true)) {
                                    Animation* Animation_0 = pPlayer->GetAnimation(true);
                                    fupdateTime_3 = Animation_0->GetTime();
                                }
                                else {
                                    fupdateTime_3 = 0.0f;
                                }
                                fupdateTime_2 = fupdateTime_3;
                            }
                        }
                        else {
                            ktrans = pprojectileNode->GetWorldTransform();
                            TESObjectCELL* pcell = apParentRef->GetParentCell();
                            psceneParent = static_cast<NiNode*>(pcell->GetDynamicNode());
                        }
                        if (psceneParent) {
                            NiNode* Parent = spAVObject.m_pObject->m_pkParent;
                            if (psceneParent != Parent) {
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    NiAVObject* v8 = spAVObject.m_pObject;
                                    TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                                    Instance->QueueNodeAttach(v8, psceneParent, true, false);
                                }
                                else {
                                    NiAVObject* v10 = spAVObject.m_pObject;
                                    psceneParent->AttachChild(v10, 1);
                                }
                                NiNode* v11 = static_cast<NiNode*>(spAVObject.m_pObject);
                                NiNode* v7 = v11->IsNode();
                                BSShaderUtil::RecursiveSetPropertyFadeAlpha(v7, 1.0f);
                            }
                        }
                        spAVObject.m_pObject->SetTranslate(ktrans.m_Translate);
                        spAVObject.m_pObject->SetRotate(ktrans.m_Rotate);
                        spAVObject.m_pObject->SetScale(ktrans.m_fScale);
                        if (bupdate) {
                            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                if (bwarnOnce) {
                                    Warning("Trying to update a Muzzle Flash when not in the main thread");
                                    bwarnOnce = false;
                                }
                            }
                            else {
                                NiUpdateData UpdateData(fupdateTime_2, false, false);
                                spAVObject.m_pObject->Update(UpdateData);
                            }
                        }
                    }
                }
            }
        }
    }
}

void MuzzleFlash::AttachLight() {
    if (spAVObject.m_pObject) {
        if (pProjectile->Data.pMuzzleFlashLight) {
            NiAVObject* v2 = spAVObject.m_pObject;
            TESObjectLIGH* FormOne = pProjectile->Data.pMuzzleFlashLight;
            spLight.operator=(FormOne->GenDynamic(nullptr, v2, true));
        }
    }
}

void MuzzleFlash::SpawnMuzzleFlash(TESObjectREFR* apParentRef, Projectile* apProjectile) {
    if (apParentRef) {
        if (apProjectile) {
            if (apParentRef->IsActor()) {
                BaseProcess* pprocess = static_cast<Actor*>(apParentRef)->GetCurrentAIProcess();
                if (pprocess) {
                    BGSProjectile* Time;
                    TESObjectWEAP* Count;
                    if (!pprocess->GetCurrentMuzzleFlash() ||
                        ((Time = pprocess->GetCurrentMuzzleFlash()->pProjectile,
                          Time != apProjectile->GetProjectileBase()) ||
                         (Count = pprocess->GetCurrentMuzzleFlash()->pWeaponSource,
                          Count != apProjectile->pWeaponSource))) {
                        pprocess->ClearMuzzleFlash();
                        pprocess->CreateMuzzleFlash(apProjectile, apParentRef);
                    }
                    if (pprocess->GetCurrentMuzzleFlash()) {
                        MuzzleFlash* v4 = pprocess->GetCurrentMuzzleFlash();
                        v4->Enable();
                    }
                }
            }
            else {
                ReferenceProcessLists.SpawnReferenceMuzzleFlash(apProjectile, apParentRef);
            }
        }
    }
}
