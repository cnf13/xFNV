#include "Fallout/Magic/MagicBallProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/bhkCharacterListenerSpell.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <limits>

MagicBallProjectile::MagicBallProjectile() : pLight(nullptr) {}

MagicBallProjectile::MagicBallProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    EffectSetting* apAVEffect,
    NiPoint3 aOrigin,
    NiPoint3 aRotation,
    NiAVObject* apData3D,
    MagicLight* apLight
)
  : MagicProjectile(apCaster, apSpell, apDominantEffect, apAVEffect, aOrigin, aRotation), fExplodingAge(0.0f),
    eState(MagicBallProjectile::BALL_STATE_CREATION), fScale(1.0f), pLight(nullptr) {
    double dSpeed = pAVEffect->data.fSpeed;
    fSpeed = fMagicProjectileBaseSpeed.Float() * dSpeed;
    if (apData3D) {
        Set3DSimple(apData3D);
    }
    else {
        MagicProjectile::Load3D(false);
    }
    Init3D();
    MagicBallProjectile::InitHavok();
    hkVector4 kVelocity(-std::numeric_limits<float>::max(), 0.0f, 0.0f, 0.0f);
    GetCharController()->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    if (pAVEffect->GetBoltSound()) {
        uint32_t isoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        Sound = PlaySound(
            isoundFileID,
            true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
    if (apLight) {
        apLight->Transfer(this);
        pLight = apLight;
    }
    CheckShotBehindWall();
    PlayerCharacter* pcaster = GetCaster() ? static_cast<PlayerCharacter*>(GetCaster()->GetCasterStatsObject())
                                           : nullptr;
    if (pcaster != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBall.Float());
    }
    TESForm::AddChange(CHANGE_REFR_ANIMATION);
}

// The sound handle's own destructor does nothing, so the sound is stopped and released here.
MagicBallProjectile::~MagicBallProjectile() {
    TESObjectREFR* pcaster = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pcaster != pPlayer) {
        MagicProjectile::RemoveTrackedProjectile(fMagicTrackingMultBall.Float());
    }
    if (Sound.IsValid()) {
        Sound.Stop();
        if (Sound.IsValid()) {
            Sound.Release();
        }
    }
    delete pLight;
    pLight = nullptr;
}

uint16_t MagicBallProjectile::GetSaveSize(uint32_t aiFlags) {
    int16_t ibaseSize = static_cast<int16_t>(MagicProjectile::GetSaveSize(aiFlags) + 8);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x64u) {
        ibaseSize += 4;
    }
    uint16_t isize = static_cast<uint16_t>(ibaseSize + 8);
    if (eState == 2) {
        isize += 4;
    }
    return isize;
}

void MagicBallProjectile::SaveGame(uint32_t aiFlags) {
    float ffadeTimer;
    MagicProjectile::SaveGame(aiFlags);
    SaveGameDataOLD(&fExplodingAge, sizeof(fExplodingAge));
    SaveGameDataOLD(&fSpeed, sizeof(fSpeed));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 100u) {
        SaveGameDataOLD(&fDistanceMoved, sizeof(fDistanceMoved));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, sizeof(eState));
    SaveGameDataOLD(&fScale, sizeof(fScale));
    if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
        ffadeTimer = 0.0f;
        if (pLight) {
            ffadeTimer = pLight->fFadeTimer;
        }
        SaveGameDataOLD(&ffadeTimer, sizeof(ffadeTimer));
    }
}

void MagicBallProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    InitHavok();
    MagicProjectile::LoadGame(aiFlags, aiCurrentFlags);
    LoadGameDataOLD(&fExplodingAge, sizeof(fExplodingAge));
    LoadGameDataOLD(&fSpeed, sizeof(fSpeed));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 100u) {
        LoadGameDataOLD(&fDistanceMoved, sizeof(fDistanceMoved));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, sizeof(eState));
    LoadGameDataOLD(&fScale, sizeof(fScale));
    if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
        char acFadeTimer[4];
        LoadGameDataOLD(acFadeTimer, sizeof(acFadeTimer));
        ourMemcpy(&pLight, acFadeTimer, sizeof(pLight));
    }
}

void MagicBallProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    float ffadeTimer;
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    Init3D();
    GetParentCell()->AddReference(this, false);
    if (eState != MagicBallProjectile::BALL_STATE_EXPLODING && pAVEffect->GetBoltSound()) {
        uint32_t isoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        Sound = PlaySound(
            isoundFileID,
            true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }

    if (Get3D()) {
        float fcurrentScale = fScale;
        Get3D()->SetScale(fcurrentScale);
    }
    ourMemcpy(&ffadeTimer, &pLight, sizeof(ffadeTimer));
    pLight = nullptr;
    if (Get3D() && pAVEffect->data.pLight) {
        pLight = new MagicLight(pAVEffect->data.pLight->iFormID, Get3D());
        if (eState == MagicBallProjectile::BALL_STATE_EXPLODING && pLight) {
            NiAVObject* ptopav = Get3D();
            NiTimeController* po = ptopav ? ptopav->GetControllers() : nullptr;
            NiControllerManager* pmanager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
            );
            if (pmanager) {
                NiControllerSequence* pseqArea = pmanager->GetSequenceByName("SpecialIdle_AreaEffect");
                if (pseqArea) {
                    float ffadeTime = pseqArea->m_fEndKeyTime * 0.75;
                    pLight->Fade(false, ffadeTime);
                    pLight->SetFadeTimer(ffadeTimer);
                }
            }
        }
    }
}

void MagicBallProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    CFilter kBodyFilter;
    CFilter kCasterFilter;
    CFilter filter;
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        uint32_t igroup;
        if (GetCaster() && GetCaster()->GetCasterStatsObject()) {
            TESObjectREFR* pref = GetCaster()->GetCasterStatsObject();
            Actor* pactor = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
            if (pactor) {
                CFilter kActorFilter;
                // GetGroup is called on the pointer GetCollisionFilter returns, which is &kActorFilter.
                pactor->GetCollisionFilter(&kActorFilter.iFilter);
                igroup = kActorFilter.GetGroup();
            }
            else {
                bhkCollisionObject* pcol = FindFirstCollisionObject(pref->Get3D());
                bhkRigidBody* prb = pcol ? pcol->GetRigidBody() : nullptr;
                if (prb) {
                    igroup = prb->GetCollisionFilter(&kCasterFilter)->GetGroup();
                }
                else {
                    igroup = bhkCollisionFilter::GetNewSystemGroup();
                }
            }
        }
        else {
            bhkCollisionObject* pcol = FindFirstCollisionObject(Get3D());
            bhkRigidBody* prb = pcol ? pcol->GetRigidBody() : nullptr;
            if (prb) {
                igroup = prb->GetCollisionFilter(&kBodyFilter)->GetGroup();
            }
            else {
                igroup = bhkCollisionFilter::GetNewSystemGroup();
            }
        }
        pcon->GetCollisionFilter(&filter);
        filter.SetLayer(L_SPELL);
        filter.SetGroup(igroup);
        pcon->SetCollisionFilter(filter);
    }
    TESObjectREFR* pcaster = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pcaster != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBall.Float());
    }
}

// Gives a high-process ball a character controller (a 1-unit sphere in the spell layer, in the caster's collision
// group, without gravity) when it has none, then puts the controller in the cell's world at the ball's position, 8
// units higher when it changes worlds.
void MagicBallProjectile::InitHavok() {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_HIGH) {
        return;
    }
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    TESObjectCELL* pCell = GetParentCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    if (!pController) {
        alignas(16) uint8_t aCinfoMemory[sizeof(bhkCharacterControllerCinfo)];
        bhkCharacterControllerCinfo* pCinfo = ::new (aCinfoMemory) bhkCharacterControllerCinfo(true);
        pCinfo->SetAcrobatics(1.0f);
        pCinfo->SetObject(Get3D());
        pCinfo->SetPosition(GetLocationOnReference());
        pCinfo->SetWorld(pWorld);
        pCinfo->SetGravity(0.0f);
        // NiNew bhkSphereShape: 0x14 bytes.
        bhkSphereShape* pShape = new bhkSphereShape(1.0f, true);
        pCinfo->SetShape(pShape);
        pCinfo->SetAvoidBox(false);
        if (pSaveLoadGameOLD->GetLoading()) {
            pCinfo->SetColFilter(L_SPELL, 0);
        }
        else {
            TESObjectREFR* pCasterRef = nullptr;
            if (GetCaster()) {
                pCasterRef = GetCaster()->GetCasterStatsObject();
            }
            if (pCasterRef) {
                if (pCasterRef->IsActor()) {
                    CFilter kFilter;
                    static_cast<MobileObject*>(pCasterRef)->GetCollisionFilter(&kFilter.iFilter);
                    pCinfo->SetColFilter(L_SPELL, kFilter.GetGroup());
                }
                else {
                    bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(pCasterRef->Get3D());
                    bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
                    uint32_t uiGroup;
                    if (pBody) {
                        CFilter kFilter;
                        pBody->GetCollisionFilter(&kFilter);
                        uiGroup = kFilter.GetGroup();
                    }
                    else {
                        uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                    }
                    pCinfo->SetColFilter(L_SPELL, uiGroup);
                }
            }
            else {
                bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(Get3D());
                bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
                uint32_t uiGroup;
                if (pBody) {
                    CFilter kFilter;
                    pBody->GetCollisionFilter(&kFilter);
                    uiGroup = kFilter.GetGroup();
                }
                else {
                    uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                }
                pCinfo->SetColFilter(L_SPELL, uiGroup);
            }
        }
        pCinfo->SetStartState(HK_CHARACTER_USER_STATE_1);
        pController = new bhkCharacterListenerSpell(*pCinfo, this);
        pController->SetFallTime(1.0f);
        pController->SetQuickSimulate(Main::GetSceneIsComplex());
        pController->SetIsProjectile(true);
        pCurrentProcess->SetCharController(NiPointer<bhkCharacterController>(pController));
        pController->SetMOBObject(this);
        pCinfo->~bhkCharacterControllerCinfo();
    }
    if (!pController) {
        return;
    }
    if (pCell && pWorld) {
        pController->SetWaterHeight(pCell->GetWaterHeight());
    }
    else {
        pController->SetWaterHeight(-std::numeric_limits<float>::max());
    }
    bhkWorld* pOldWorld = pController->GetWorld();
    pController->SetWorld(pWorld);
    NiNode* p3D = Get3D();
    NiAVObject* pOldObject = pController->GetNiObject();
    bool bAddToWorld = true;
    if (pOldObject != p3D) {
        pController->SetNiObject(p3D);
        if (pWorld && pWorld->QDebugDisplay()) {
            pController->BuildDisplayGeometry(nullptr);
            BSShaderUtil::RecursiveSetPropertyWireframe(pController->GetNiObject(), 1);
        }
        bhkWorld::SetMotion(p3D, hkpMotion::MOTION_KEYFRAMED, true, false, true);
        bAddToWorld = true;
    }
    NiPoint3 kPosition = GetLocationOnReference();
    if (pOldWorld != pWorld) {
        if (pOldWorld) {
            bhkWorld::RemoveObjects(p3D, true, false);
        }
        kPosition.z = static_cast<float>(kPosition.z + 8.0);
        bAddToWorld = true;
    }
    if (!pWorld) {
        return;
    }
    pController->SetPosition(kPosition);
    if (bAddToWorld) {
        CFilter kFilter;
        pController->GetCollisionFilter(&kFilter);
        uint32_t uiGroup = kFilter.GetGroup();
        pWorld->AddObjects(p3D, true, false, uiGroup, false);
    }
    else {
        bhkWorld::UpdatePosition(p3D, true, false);
    }
}

void MagicBallProjectile::Update(float afDelta) {
    if (afDelta >= 60.0) {
        SetDelete(true);
    }
    if (afDelta >= 0.0) {
        MagicBallProjectile::State eState = this->eState;
        if (eState) {
            if (eState != MagicBallProjectile::BALL_STATE_FLYING) {
                if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
                    NiAVObject* ptopav = Get3D();
                    NiTimeController* po = ptopav ? ptopav->GetControllers() : nullptr;
                    NiControllerManager* pmanager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
                    );
                    if (pmanager) {
                        NiControllerSequence* pseqStop = pmanager->GetSequenceByName("SpecialIdle_AreaEffect");
                        if (!pseqStop || pseqStop->m_fEndKeyTime < pseqStop->m_fLastTime || !pseqStop->GetState()) {
                            pmanager->DeactivateAll(0.0f);
                            SetDisabled(true);
                            SetDelete(true);
                        }
                    }
                    else {
                        SetDelete(true);
                        SetDisabled(true);
                    }
                    if (pLight) {
                        pLight->Update(afDelta);
                    }
                }
                return;
            }
        }
        else {
            this->eState = MagicBallProjectile::BALL_STATE_FLYING;
        }
        if (!GetDelete()) {
            float fhavokWaterHeight = GetCharController()->QWaterHeight();
            double dWaterHeight = HK2NI(fhavokWaterHeight);
            if (GetLocationOnReference().z <= dWaterHeight) {
                Collide(GetLocationOnReference(), nullptr, nullptr, true);
            }
            float fstep = fSpeed * afDelta;
            NiPoint3 rdeltaMove(0.0f, fstep, 0.0f);
            NiPoint3 oldLoc = GetLocationOnReference();
            Move(afDelta, rdeltaMove, 15u);
            if (IsActive()) {
                NiPoint3 newLoc = GetLocationOnReference();
                if (Get3D()) {
                    Get3D()->SetTranslate(newLoc);
                }
                fDistanceMoved = (newLoc - oldLoc).Length() + fDistanceMoved;
                if (fMagicProjectileMaxDistance.Float() < static_cast<double>(fDistanceMoved)) {
                    SetDelete(true);
                }
                if (pLight) {
                    pLight->Update(afDelta);
                }
            }
        }
    }
}

void MagicBallProjectile::Collide(
    NiPoint3 aContactPoint,
    bhkRigidBody* apTargetBody,
    TESObjectREFR* apCollidee,
    bool abFizzle
) {
    Actor* pactor;
    CFilter filter;
    if (apCollidee && apCollidee->IsActor()) {
        pactor = static_cast<Actor*>(apCollidee);
    }
    else {
        pactor = nullptr;
    }
    if (!pactor || !pactor->GetGhost()) {
        double dArea = static_cast<double>(GetDominantEffect()->GetArea());
        fScale = fMagicAreaScale.Float() * dArea;
        if (fMagicAreaScaleMax.Float() < static_cast<double>(fScale)) {
            fScale = fMagicAreaScaleMax.Float();
        }
        else if (fMagicAreaScaleMin.Float() > static_cast<double>(fScale)) {
            fScale = fMagicAreaScaleMin.Float();
        }
        SetLocationOnReference(aContactPoint);
        NiAVObject* pav = Get3D();
        if (pav) {
            NiPoint3 kDirection = pav->GetWorldTransform().m_Rotate * NiPoint3::UNIT_Y;
            ApplyCollisionForce(aContactPoint, kDirection, apTargetBody);
        }
        if (pav && !abFizzle) {
            pav->SetTranslate(aContactPoint);
            NiAVObject* pareaParticles = pav->GetObjectByName("AreaEffect");
            if (pareaParticles) {
                pareaParticles->SetScale(fScale);
            }
        }

        if (Sound.IsValid()) {
            Sound.Stop();
        }
        if (GetCharController() && !abFizzle) {
            CFilter disabledFilter;
            if (GetCharController()->GetFlag(bhkCharacterController::DEBUGDISP)) {
                BuildAreaEffectGeometry();
            }
            GetCharController()->GetCollisionFilter(&filter);
            filter.SetDisabled(1);
            disabledFilter.iFilter = filter.iFilter;
            GetCharController()->SetCollisionFilter(disabledFilter);
        }

        eState = MagicBallProjectile::BALL_STATE_EXPLODING;
        if (!abFizzle) {
            NiPoint3 kLocation = GetLocationOnReference();
            GetCaster()->FindTargets_0(pSpell, GetParentCell(), kLocation, this, apCollidee, false, 1.0f, 1.0f);
        }
        NiAVObject* ptopav = Get3D();
        NiTimeController* po = ptopav ? ptopav->GetControllers() : nullptr;
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
        );
        if (pmanager) {
            NiControllerSequence* pseqArea = pmanager->GetSequenceByName("SpecialIdle_AreaEffect");
            if (pseqArea) {
                pmanager->DeactivateAll(0.0f);
                pmanager->ActivateSequence(pseqArea, 0, false, 1.0f, 0.0f, nullptr);
                pmanager->SetActive(true);
                pseqArea->SetOffset(-std::numeric_limits<float>::max());
                if (pLight) {
                    float ffadeTime = pseqArea->m_fEndKeyTime * 0.75;
                    pLight->Fade(false, ffadeTime);
                }
            }
        }
        if (pAVEffect->data.pAreaSound && !abFizzle) {
            uint32_t isoundFileID = pAVEffect->data.pAreaSound->iFormID;
            BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(isoundFileID, 258u);
            NiPoint3 kLocation = GetLocationOnReference();
            soundHand.SetPosition(kLocation.x, kLocation.y, kLocation.z);
            soundHand.Play(false);
        }
    }
}

bool MagicBallProjectile::IsActive() {
    return eState != BALL_STATE_EXPLODING && !TESForm::GetDelete();
}

void MagicBallProjectile::BuildAreaEffectGeometry() {
    NiAVObject* pav = Get3D();
    bool bhasDisplay = true;
    if (pav) {
        if (!pav->GetObjectByName("MagicAreaDisplay")) {
            bhasDisplay = false;
        }
    }
    if (!bhasDisplay) {
        NiNode* pnode = pav->IsNode();
        if (pnode) {
            NiMaterialProperty* pmat;
            CFilter kFilter;
            double dArea = static_cast<double>(GetDominantEffect()->GetArea());
            double dRadius = fMagicUnitsPerFoot.Float() * dArea;
            float farea = dRadius / pav->m_kLocal.m_fScale;
            NiAVObject* pdebugSphere = BSTestObjects::CreateSphere(farea, 10, 10, nullptr);
            pdebugSphere->SetName("MagicAreaDisplay");
            pdebugSphere->SetTranslate(NiPoint3::ZERO);
            NiColor clr(0.0f, 0.0f, 0.0f);
            GetCollisionFilter(&kFilter.iFilter);
            bhkCollisionFilter::GetDebugColor(kFilter, clr);
            pmat = new NiMaterialProperty;
            pmat->SetEmittance(clr);
            pdebugSphere->AttachProperty(pmat);
            pnode->AttachChild(pdebugSphere, 1);
            pnode->UpdateProperties();
        }
    }
}

void MagicBallProjectile::Init3D() {
    NiAVObject* pn = Get3D();
    if (pn) {
        NiMatrix3 rot;
        pn->SetScale(fScale);
        pn->SetTranslate(GetLocationOnReference());
        NiPoint3 kAngle = GetAngleOnReference();
        rot.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
        pn->SetRotate(rot);
        BSUtilities::ProcessSelectiveUpdate(pn);
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pn->GetControllers())
        );
        if (pmanager) {
            NiControllerSequence* pseq = pmanager->GetSequenceByName("SpecialIdle_Projectile");
            if (pseq) {
                pmanager->DeactivateAll(0.0f);
                pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
                pmanager->SetActive(true);
                pseq->SetOffset(-std::numeric_limits<float>::max());
                float ftime = fCellAppTime - 0.10000000149011612;
                NiUpdateData kUpdateData(ftime, true, false);
                pn->Update(kUpdateData);
            }
        }
    }
}

void bhkCharacterControllerCinfo::SetPosition(const NiPoint3& arPos) {
    NI2HK(m_position, arPos);
}

void bhkCharacterControllerCinfo::SetShape(bhkShape* apShape) {
    spShape = apShape;
}

EffectItem* MagicProjectile::GetDominantEffect() {
    return pDominantEffect;
}

EffectSetting* MagicProjectile::GetAVEffect() {
    return pAVEffect;
}

void MagicProjectile::AddTrackedProjectile(float afProjectileWeight) {
    fProjectileTracker = fProjectileTracker + afProjectileWeight;
}

void MagicProjectile::RemoveTrackedProjectile(float afProjectileWeight) {
    fProjectileTracker = fProjectileTracker - BSMAX(0.0f, afProjectileWeight);
}
