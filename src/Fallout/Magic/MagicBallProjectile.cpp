#include "Fallout/Magic/MagicBallProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
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
    double MaximumLeafAngle = pAVEffect->data.fSpeed;
    fSpeed = fMagicProjectileBaseSpeed.Float() * MaximumLeafAngle;
    if (apData3D) {
        Set3DSimple(apData3D);
    }
    else {
        MagicProjectile::Load3D(false);
    }
    Init3D();
    MagicBallProjectile::InitHavok();
    hkVector4 kVelocity(-3.4028235e+38f, 0.0f, 0.0f, 0.0f);
    GetCharController()->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    if (pAVEffect->GetBoltSound()) {
        uint32_t CastingType = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        BSSoundHandle kSound;
        Sound = *PlaySound(
            &kSound, CastingType, true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
    if (apLight) {
        apLight->Transfer(this);
        pLight = apLight;
    }
    CheckShotBehindWall();
    PlayerCharacter* pcaster =
        GetCaster() ? static_cast<PlayerCharacter*>(GetCaster()->GetCasterStatsObject()) : nullptr;
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
    int16_t v4 = (int16_t)(MagicProjectile::GetSaveSize(aiFlags) + 8);
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) < 0x64u) {
        v4 += 4;
    }
    uint16_t v5 = (uint16_t)(v4 + 8);
    if (eState == 2) {
        v5 += 4;
    }
    return v5;
}

void MagicBallProjectile::SaveGame(uint32_t aiFlags) {
    float a1;
    MagicProjectile::SaveGame(aiFlags);
    SaveGameDataOLD(&fExplodingAge, 4);
    SaveGameDataOLD(&fSpeed, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 100u) {
        SaveGameDataOLD(&fDistanceMoved, 4);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, 4u);
    SaveGameDataOLD(&fScale, 4);
    if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
        a1 = 0.0f;
        if (pLight) {
            a1 = pLight->fFadeTimer;
        }
        SaveGameDataOLD(&a1, 4);
    }
}

void MagicBallProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    InitHavok();
    MagicProjectile::LoadGame(aiFlags, aiCurrentFlags);
    LoadGameDataOLD(&fExplodingAge, 4);
    LoadGameDataOLD(&fSpeed, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 100u) {
        LoadGameDataOLD(&fDistanceMoved, 4);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, 4u);
    LoadGameDataOLD(&fScale, 4);
    if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
        char apSrc[4];
        LoadGameDataOLD(apSrc, 4);
        ourMemcpy(&pLight, apSrc, sizeof(pLight));
    }
}

void MagicBallProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, arHandleOut, std::nullopt, std::nullopt);
    float ffadeTimer;
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    Init3D();
    GetParentCell()->AddReference(this, false);
    if ((eState != MagicBallProjectile::BALL_STATE_EXPLODING) && (pAVEffect->GetBoltSound())) {
        uint32_t isoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        BSSoundHandle* arHandle = PlaySound(
            &arHandleOut, isoundFileID, true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
        Sound.operator=(*arHandle);
    }

    if (Get3D()) {
        float a3 = fScale;
        NiNode* v3 = Get3D();
        v3->SetScale(a3);
    }
    ourMemcpy(&ffadeTimer, &pLight, sizeof(ffadeTimer));
    pLight = nullptr;
    if ((Get3D()) && (pAVEffect->data.pLight)) {
        MagicLight* v10;
        MagicLight* v14 = static_cast<MagicLight*>(ourMalloc(sizeof(MagicLight)));
        if (v14) {
            NiNode* a3a = Get3D();
            uint32_t CastingType = pAVEffect->data.pLight->iFormID;
            v10 = ::new (v14) MagicLight(CastingType, a3a);
        }
        else {
            v10 = nullptr;
        }
        pLight = v10;
        if ((eState == MagicBallProjectile::BALL_STATE_EXPLODING) && (pLight)) {
            NiTimeController* po;
            NiAVObject* ptopav = (NiAVObject*)Get3D();
            if (ptopav) {
                po = ptopav->GetControllers();
            }
            else {
                po = nullptr;
            }
            NiObject* pkObject = (NiObject*)po;
            NiControllerManager* pmanager =
                static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po));
            if (pmanager) {
                NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("SpecialIdle_AreaEffect");
                NiControllerSequence* pseqArea = pmanager->GetSequenceByName(*a2);
                apString.~NiFixedString();
                if (pseqArea) {
                    float v7 = pseqArea->m_fEndKeyTime * 0.75;
                    pLight->Fade(false, v7);
                    pLight->SetFadeTimer(ffadeTimer);
                }
            }
        }
    }
}

void MagicBallProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESObjectREFR* pcaster;
    CFilter v18;
    CFilter apResult;
    CFilter filter;
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        MagicCaster* v16;
        uint32_t igroup;
        if (GetCaster() && (v16 = GetCaster(), v16->GetCasterStatsObject())) {
            MagicCaster* Caster = GetCaster();
            TESObjectREFR* pref = Caster->GetCasterStatsObject();
            Actor* pactor = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
            if (pactor) {
                CFilter v20;
                // GetGroup is called on the pointer GetCollisionFilter returns, which is &v20.
                pactor->GetCollisionFilter(&v20.iFilter);
                igroup = v20.GetGroup();
            }
            else {
                uint32_t igroup_2;
                bhkRigidBody* prb;
                NiNode* v4 = pref->Get3D();
                bhkCollisionObject* pcol = FindFirstCollisionObject(v4);
                if (pcol) {
                    prb = pcol->GetRigidBody();
                }
                else {
                    prb = nullptr;
                }
                if (prb) {
                    CFilter* v5 = prb->GetCollisionFilter(&apResult);
                    igroup_2 = v5->GetGroup();
                }
                else {
                    igroup_2 = bhkCollisionFilter::GetNewSystemGroup();
                }
                igroup = igroup_2;
            }
        }
        else {
            uint32_t igroup_3;
            bhkRigidBody* prb_2;
            NiNode* v6 = Get3D();
            bhkCollisionObject* pcol_2 = FindFirstCollisionObject(v6);
            if (pcol_2) {
                prb_2 = pcol_2->GetRigidBody();
            }
            else {
                prb_2 = nullptr;
            }
            if (prb_2) {
                CFilter* v7 = prb_2->GetCollisionFilter(&v18);
                igroup_3 = v7->GetGroup();
            }
            else {
                igroup_3 = bhkCollisionFilter::GetNewSystemGroup();
            }
            igroup = igroup_3;
        }
        pcon->GetCollisionFilter(&filter);
        filter.SetLayer(L_SPELL);
        filter.SetGroup(igroup);
        pcon->SetCollisionFilter(filter);
    }
    if (GetCaster()) {
        MagicCaster* v10 = GetCaster();
        pcaster = v10->GetCasterStatsObject();
    }
    else {
        pcaster = nullptr;
    }
    if (pcaster != pPlayer) {
        float* v8 = &fMagicTrackingMultBall.Float();
        MagicProjectile::AddTrackedProjectile(*v8);
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
    bhkWorld* pWorld;
    if (pCell) {
        pWorld = pCell->GetbhkWorld();
    }
    else {
        pWorld = nullptr;
    }
    if (!pController) {
        alignas(16) uint8_t aCinfoMemory[sizeof(bhkCharacterControllerCinfo)];
        bhkCharacterControllerCinfo* pCinfo = ::new (aCinfoMemory) bhkCharacterControllerCinfo(true);
        pCinfo->SetAcrobatics(1.0f);
        pCinfo->SetObject(Get3D());
        pCinfo->SetPosition(GetLocationOnReference());
        pCinfo->SetWorld(pWorld);
        pCinfo->SetGravity(0.0f);
        // NiNew bhkSphereShape: 0x14 bytes.
        void* pShapeMemory = NiMemObject::operator new(sizeof(bhkSphereShape));
        bhkSphereShape* pShape;
        if (pShapeMemory) {
            pShape = ::new (pShapeMemory) bhkSphereShape(1.0f, true);
        }
        else {
            pShape = nullptr;
        }
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
                    bhkRigidBody* pBody;
                    if (pCollisionObject) {
                        pBody = pCollisionObject->GetRigidBody();
                    }
                    else {
                        pBody = nullptr;
                    }
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
                bhkRigidBody* pBody;
                if (pCollisionObject) {
                    pBody = pCollisionObject->GetRigidBody();
                }
                else {
                    pBody = nullptr;
                }
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
        // The listener is allocated with the 16-aligned allocator it shares with bhkRagdollController.
        void* pListenerMemory = bhkRagdollController::operator new(sizeof(bhkCharacterListenerSpell));
        bhkCharacterListenerSpell* pListener;
        if (pListenerMemory) {
            pListener = ::new (pListenerMemory) bhkCharacterListenerSpell(*pCinfo, this);
        }
        else {
            pListener = nullptr;
        }
        pController = pListener;
        pController->SetFallTime(1.0f);
        pController->SetQuickSimulate(Main::GetSceneIsComplex());
        pController->SetIsProjectile(true);
        // SWEEP-INLINE 0x633C90
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
        pController->SetWaterHeight(-3.4028235e+38f);
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
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    if (afDelta >= 60.0) {
        SetDelete(true);
    }
    if (afDelta >= 0.0) {
        MagicBallProjectile::State eState = this->eState;
        if (eState) {
            if (eState != MagicBallProjectile::BALL_STATE_FLYING) {
                if (eState == MagicBallProjectile::BALL_STATE_EXPLODING) {
                    NiTimeController* po;
                    NiAVObject* ptopav = (NiAVObject*)Get3D();
                    if (ptopav) {
                        po = ptopav->GetControllers();
                    }
                    else {
                        po = nullptr;
                    }
                    NiObject* pkObject = (NiObject*)po;
                    NiControllerManager* pmanager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
                    );
                    if (pmanager) {
                        double v8;
                        NiFixedString* kName =
                            ::new (static_cast<void*>(&apString)) NiFixedString("SpecialIdle_AreaEffect");
                        NiControllerSequence* pseqStop = pmanager->GetSequenceByName(*kName);
                        apString.~NiFixedString();
                        if (!pseqStop ||
                            ((v8 = pseqStop->m_fLastTime, pseqStop->m_fEndKeyTime < v8) || !pseqStop->GetState())) {
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
            bhkCharacterController* CharController = GetCharController();
            float z = CharController->QWaterHeight();
            double v13 = HK2NI(z);
            if ((&GetLocationOnReference())->z <= v13) {
                Collide(GetLocationOnReference(), nullptr, nullptr, true);
            }
            float v12 = fSpeed * afDelta;
            NiPoint3 rdeltaMove(0.0f, v12, 0.0f);
            NiPoint3 oldLoc = GetLocationOnReference();
            Move(afDelta, rdeltaMove, 15u);
            if (IsActive()) {
                NiPoint3 newLoc = GetLocationOnReference();
                if (Get3D()) {
                    NiNode* v3 = Get3D();
                    v3->SetTranslate(newLoc);
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
    Actor* v20;
    FNVD_INLINED_LOCAL(NiFixedString, v22, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
    CFilter filter;
    if (apCollidee && apCollidee->IsActor()) {
        v20 = static_cast<Actor*>(apCollidee);
    }
    else {
        v20 = nullptr;
    }
    if (!v20 || !v20->GetGhost()) {
        NiTimeController* po;
        double Area = static_cast<double>(GetDominantEffect()->GetArea());
        fScale = fMagicAreaScale.Float() * Area;
        if (!(fMagicAreaScaleMax.Float() < static_cast<double>(fScale))) {
            if (fMagicAreaScaleMin.Float() > static_cast<double>(fScale)) {
                fScale = fMagicAreaScaleMin.Float();
            }
        }
        else {
            fScale = fMagicAreaScaleMax.Float();
        }
        SetLocationOnReference(aContactPoint);
        NiAVObject* pav = Get3D();
        if (pav) {
            NiPoint3 v38 = (&pav->GetWorldTransform())->m_Rotate * NiPoint3_UNIT_Y;
            ApplyCollisionForce(aContactPoint, v38, apTargetBody);
        }
        if ((pav) && (!abFizzle)) {
            pav->SetTranslate(aContactPoint);
            NiFixedString* v18 = ::new (static_cast<void*>(&apString)) NiFixedString("AreaEffect");
            NiAVObject* pareaParticles = pav->GetObjectByName(*v18);
            apString.~NiFixedString();
            if (pareaParticles) {
                pareaParticles->SetScale(fScale);
            }
        }

        if (Sound.IsValid()) {
            Sound.Stop();
        }
        if ((GetCharController()) && (!abFizzle)) {
            CFilter filter_2;
            bhkCharacterController* CharController = GetCharController();
            if (CharController->GetFlag(bhkCharacterController::DEBUGDISP)) {
                BuildAreaEffectGeometry();
            }
            bhkCharacterController* v6 = GetCharController();
            v6->GetCollisionFilter(&filter);
            filter.SetDisabled(1);
            filter_2.iFilter = filter.iFilter;
            bhkCharacterController* v7 = GetCharController();
            v7->SetCollisionFilter(filter_2);
        }

        eState = MagicBallProjectile::BALL_STATE_EXPLODING;
        if (!abFizzle) {
            NiPoint3 v37 = GetLocationOnReference();
            GetCaster()->FindTargets_0(pSpell, GetParentCell(), v37, this, apCollidee, false, 1.0f, 1.0f);
        }
        NiAVObject* ptopav = Get3D();
        if (ptopav) {
            po = ptopav->GetControllers();
        }
        else {
            po = nullptr;
        }
        NiObject* pkObject = po;
        NiControllerManager* pmanager =
            static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po));
        if (pmanager) {
            NiFixedString* kName = ::new (static_cast<void*>(&v22)) NiFixedString("SpecialIdle_AreaEffect");
            NiControllerSequence* pseqArea = pmanager->GetSequenceByName(*kName);
            v22.~NiFixedString();
            if (pseqArea) {
                pmanager->DeactivateAll(0.0f);
                pmanager->ActivateSequence(pseqArea, 0, false, 1.0f, 0.0f, nullptr);
                pmanager->SetActive(true);
                pseqArea->SetOffset(-3.4028235e+38f);
                if (pLight) {
                    float v15 = pseqArea->m_fEndKeyTime * 0.75;
                    pLight->Fade(false, v15);
                }
            }
        }
        if ((pAVEffect->data.pAreaSound) && (!abFizzle)) {
            TESSound* OpenSound = pAVEffect->data.pAreaSound;
            int32_t auiFormID = OpenSound->iFormID;
            uint32_t arPos_4 = auiFormID;
            BSAudio* v9 = &BSAudio::QInstance();
            v9->GetSoundHandleByNumericID(&soundHand, arPos_4, 258u);
            NiPoint3 v27 = GetLocationOnReference();
            soundHand.SetPosition(v27.x, v27.y, v27.z);
            soundHand.Play(false);
        }
    }
}

bool MagicBallProjectile::IsActive() {
    return (bool)((uint8_t)((eState != 2) && (!TESForm::GetDelete())));
}

void MagicBallProjectile::BuildAreaEffectGeometry() {
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, v13, std::nullopt, std::nullopt);
    char v9 = 0;
    NiAVObject* pav = (NiAVObject*)Get3D();
    bool v6 = true;
    if (pav) {
        NiFixedString* v7 = ::new (static_cast<void*>(&v13)) NiFixedString("MagicAreaDisplay");
        v9 = 1;
        if (!pav->GetObjectByName(*v7)) {
            v6 = false;
        }
    }
    if ((v9 & 1) != 0) {
        v13.~NiFixedString();
    }
    if (!v6) {
        NiNode* pnode = pav->IsNode();
        if (pnode) {
            NiMaterialProperty* pmat;
            CFilter kFilter;
            double Area = static_cast<double>(GetDominantEffect()->GetArea());
            double v4 = fMagicUnitsPerFoot.Float() * Area;
            float farea = v4 / pav->m_kLocal.m_fScale;
            NiAVObject* pdebugSphere = BSTestObjects::CreateSphere(farea, 10, 10, nullptr);
            NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("MagicAreaDisplay");
            pdebugSphere->SetName(*a2);
            apString.~NiFixedString();
            pdebugSphere->SetTranslate(NiPoint3_ZERO);
            NiColor clr(0.0f, 0.0f, 0.0f);
            GetCollisionFilter(&kFilter.iFilter);
            bhkCollisionFilter::GetDebugColor(kFilter, clr);
            NiMaterialProperty* apMemory =
                static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
            if (apMemory) {
                pmat = ::new (apMemory) NiMaterialProperty;
            }
            else {
                pmat = nullptr;
            }
            pmat->SetEmittance(clr);
            pdebugSphere->AttachProperty(pmat);
            pnode->AttachChild(pdebugSphere, 1);
            pnode->UpdateProperties();
        }
    }
}

void MagicBallProjectile::Init3D() {
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiAVObject* pn = Get3D();
    if (pn) {
        NiMatrix3 rot;
        pn->SetScale(fScale);
        const NiPoint3* v1 = &GetLocationOnReference();
        pn->SetTranslate(*v1);
        NiPoint3 v10 = GetAngleOnReference();
        rot.FromEulerAnglesZXY(v10.z, v10.x, v10.y);
        pn->SetRotate(rot);
        BSUtilities::ProcessSelectiveUpdate(pn);
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pn->GetControllers())
        );
        if (pmanager) {
            NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("SpecialIdle_Projectile");
            NiControllerSequence* pseq = pmanager->GetSequenceByName(*kName);
            apString.~NiFixedString();
            if (pseq) {
                pmanager->DeactivateAll(0.0f);
                pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
                pmanager->SetActive(true);
                pseq->SetOffset(-3.4028235e+38f);
                float v3 = fCellAppTime - 0.10000000149011612;
                NiUpdateData UpdateData(v3, true, false);
                pn->Update(UpdateData);
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
