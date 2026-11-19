#include "Fallout/Magic/MagicFogProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
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
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <cmath>

MagicFogProjectile::SpellTarget::SpellTarget(
    TESObjectREFR* apTarget,
    EffectItem* apEffect,
    MagicFogProjectile::SpellTarget* apNext
)
  : pTarget(apTarget), pEffect(apEffect), pNext(apNext) {}

MagicFogProjectile::MagicFogProjectile() : pTargetList(nullptr), pLight(nullptr) {}

// The duration and area are read as signed; the fog starts with the lowest possible x velocity. The area sound is
// the effect's bolt sound, played looping.
MagicFogProjectile::MagicFogProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    EffectSetting* apAVEffect,
    NiPoint3 aOrigin,
    NiPoint3 aRotation,
    NiAVObject* apData3D,
    MagicLight* apLight
)
  : MagicProjectile(apCaster, apSpell, apDominantEffect, apAVEffect, aOrigin, aRotation), fAge(0.0f),
    eState(MagicFogProjectile::FOG_STATE_SHOOTING), fScale(1.0f), pTargetList(nullptr), pLight(nullptr) {
    float fDurationMin = fMagicCloudDurationMin.Float();
    int32_t iDuration = GetDominantEffect()->GetDuration();
    fProjectileDuration = BSMAX(static_cast<float>(iDuration), fDurationMin);
    float fAreaMin = fMagicCloudAreaMin.Float();
    int32_t iArea = GetDominantEffect()->GetArea();
    fProjectileArea = BSMAX(static_cast<float>(iArea), fAreaMin);
    double dProjectileSpeed = GetAVEffect()->GetProjectileSpeed();
    fSpeed = static_cast<float>(fMagicProjectileBaseSpeed.Float() * dProjectileSpeed);
    if (apData3D) {
        Set3DSimple(apData3D);
    }
    else {
        MagicProjectile::Load3D(false);
    }
    Init3D();
    MagicFogProjectile::InitHavok();
    alignas(16) hkVector4 kVelocity(-3.40282347e+38f, 0.0f, 0.0f, 0.0f);
    static_cast<bhkCharacterController*>(GetCharController())->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    if (GetAVEffect()->GetBoltSound()) {
        uint32_t uiSoundID = GetAVEffect()->GetBoltSound()->GetFormID();
        if (Sound.IsValid()) {
            Sound.Release();
        }
        Sound = PlaySound(
            uiSoundID, true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
        Sound.SetVolume(1.0f);
    }
    if (apLight) {
        apLight->Transfer(this);
        pLight = apLight;
        pLight->Fade(false, fProjectileDuration);
    }
    CheckShotBehindWall();
    TESObjectREFR* pCasterRef = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pCasterRef != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultFog.Float());
    }
}

// The sound handle's own destructor does nothing, so the sound is stopped and released here.
MagicFogProjectile::~MagicFogProjectile() {
    TESObjectREFR* pcaster = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pcaster != pPlayer) {
        MagicProjectile::RemoveTrackedProjectile(fMagicTrackingMultFog.Float());
    }
    ClearTargetList();
    if (Sound.IsValid()) {
        Sound.Stop();
        Sound.Release();
    }
    delete pLight;
    pLight = nullptr;
}

uint16_t MagicFogProjectile::GetSaveSize(uint32_t aiFlags) {
    MagicFogProjectile::SpellTarget* i;
    uint16_t v5 = (uint16_t)(MagicProjectile::GetSaveSize(aiFlags) + 20);
    if (eState == 1) {
        v5 += 4;
    }
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x71u) {
        v5 += 6;
        for (i = pTargetList; i; i = i->pNext) {
            v5 += 5;
        }
    }
    return v5;
}

void MagicFogProjectile::SaveGame(uint32_t aiFlags) {
    uint8_t ceffectItemIndex;
    uint32_t itargetID;
    float ffadeTimer;
    MagicProjectile::SaveGame(aiFlags);
    SaveGameDataOLD(&this->MagicFogProjectile::fAge, 4);
    SaveGameDataOLD(&fSpeed, 4);
    SaveGameDataOLD(&fProjectileDuration, 4);
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, 4u);
    SaveGameDataOLD(&fScale, 4);
    if (eState == MagicFogProjectile::FOG_STATE_PERSISTANT) {
        ffadeTimer = 0.0f;
        if (pLight) {
            ffadeTimer = pLight->fFadeTimer;
        }
        SaveGameDataOLD(&ffadeTimer, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        uint16_t usCount;
        SaveGameDataOLD(&fProjectileArea, 4);
        usCount = 0;
        uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveGameDataOLD(&usCount, 2);
        for (MagicFogProjectile::SpellTarget* plist = pTargetList; plist; plist = plist->pNext) {
            itargetID = 0;
            if (plist->pTarget) {
                itargetID = plist->pTarget->iFormID;
            }
            SaveNumericID(&itargetID, 4);
            ceffectItemIndex = 0;
            if ((pSpell) && (plist->pEffect)) {
                ceffectItemIndex = pSpell->GetEffectItemIndex(plist->pEffect);
            }

            SaveGameDataOLD(&ceffectItemIndex, 1);
            ++usCount;
        }
        *pcount = usCount;
    }
}

void MagicFogProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint8_t ceffectItemIndex;
    uint16_t usCount;
    ClearTargetList();
    InitHavok();
    MagicProjectile::LoadGame(aiFlags, aiCurrentFlags);
    LoadGameDataOLD(&this->MagicFogProjectile::fAge, 4);
    LoadGameDataOLD(&fSpeed, 4);
    LoadGameDataOLD(&fProjectileDuration, 4);
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, 4u);
    LoadGameDataOLD(&fScale, 4);
    if (eState == MagicFogProjectile::FOG_STATE_PERSISTANT) {
        char apSrc[4];
        LoadGameDataOLD(apSrc, 4);
        ourMemcpy(&pLight, apSrc, sizeof(pLight));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&fProjectileArea, 4);
        LoadGameDataOLD(&usCount, 2);
        for (uint32_t i = 0; i < usCount; ++i) {
            MagicFogProjectile::SpellTarget* pnewEntry;
            uint32_t itargetID;
            LoadNumericID(&itargetID, 4);
            LoadGameDataOLD(&ceffectItemIndex, 1);
            MagicFogProjectile::SpellTarget* apMemory =
                static_cast<MagicFogProjectile::SpellTarget*>(ourMalloc(sizeof(MagicFogProjectile::SpellTarget)));
            if (apMemory) {
                // The target's form ID and the effect's index are kept in the pointer fields until InitLoadGame
                // resolves them.
                pnewEntry = ::new (apMemory) MagicFogProjectile::SpellTarget(
                    reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(itargetID)),
                    reinterpret_cast<EffectItem*>(static_cast<uintptr_t>(ceffectItemIndex)), pTargetList
                );
            }
            else {
                pnewEntry = nullptr;
            }
            pTargetList = pnewEntry;
        }
    }
}

void MagicFogProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    float ffadeTimer;
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    Init3D();
    GetParentCell()->AddReference(this, false);
    if ((eState != MagicFogProjectile::FOG_STATE_PERSISTANT) && (pAVEffect->GetBoltSound())) {
        uint32_t isoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        Sound = PlaySound(
            isoundFileID, true,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }

    ourMemcpy(&ffadeTimer, &pLight, sizeof(ffadeTimer));
    pLight = nullptr;
    if ((Get3D()) && (pAVEffect->data.pLight)) {
        MagicLight* v8;
        MagicLight* v11 = static_cast<MagicLight*>(ourMalloc(sizeof(MagicLight)));
        if (v11) {
            NiNode* a3 = Get3D();
            uint32_t CastingType = pAVEffect->data.pLight->iFormID;
            v8 = ::new (v11) MagicLight(CastingType, a3);
        }
        else {
            v8 = nullptr;
        }
        pLight = v8;
        if (pLight) {
            pLight->Fade(false, fProjectileDuration);
            pLight->SetFadeTimer(ffadeTimer);
        }
    }

    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        for (MagicFogProjectile::SpellTarget* plist = pTargetList; plist; plist = plist->pNext) {
            uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->pTarget));
            // pEffect holds the effect's index here (stored by LoadGame) rather than an EffectItem*, and only its low
            // byte is read.
            uint8_t ceffectItemIndex = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(plist->pEffect));
            if (itargetID) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(itargetID);
                TESObjectREFR* v5 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
                plist->pTarget = v5;
            }
            if (pSpell) {
                EffectItem* EffectItemByIndex = pSpell->GetEffectItemByIndex(ceffectItemIndex);
                plist->pEffect = EffectItemByIndex;
            }
        }
    }
}

void MagicFogProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
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
            Actor* pactor = dynamic_cast<Actor*>(pref);
            if (pactor) {
                // GetGroup is called on the returned pointer, which is &kFilter.
                CFilter kFilter;
                pactor->GetCollisionFilter(&kFilter.iFilter);
                igroup = kFilter.GetGroup();
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
        float* v8 = &fMagicTrackingMultFog.Float();
        MagicProjectile::AddTrackedProjectile(*v8);
    }
}

// Gives a high-process fog a character controller (a 1-unit sphere in the spell layer, in the caster's collision group,
// without gravity, with the fog's acrobatics) when it has none, then puts the controller in the cell's world at the
// fog's position, 8 units higher when it changes worlds. MagicBallProjectile::InitHavok's steps, except that the caster
// is looked up before the loading test and the area geometry is rebuilt after the debug wireframe.
void MagicFogProjectile::InitHavok() {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_HIGH) {
        return;
    }
    MagicProjectile* pThis = this;
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
        pCinfo->SetAcrobatics(pThis->GetAcrobatics());
        pCinfo->SetObject(Get3D());
        pCinfo->SetPosition(GetLocationOnReference());
        pCinfo->SetWorld(pWorld);
        pCinfo->SetGravity(0.0f);
        // NiNew bhkSphereShape: 0x14 bytes.
        bhkSphereShape* pShape = new bhkSphereShape(1.0f, true);
        pCinfo->SetShape(pShape);
        pCinfo->SetAvoidBox(false);
        TESObjectREFR* pCasterRef = nullptr;
        if (GetCaster()) {
            pCasterRef = GetCaster()->GetCasterStatsObject();
        }
        if (pSaveLoadGameOLD->GetLoading()) {
            pCinfo->SetColFilter(L_SPELL, 0);
        }
        else if (pCasterRef) {
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
            pThis->BuildAreaEffectGeometry();
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

float MagicFogProjectile::GetAcrobatics() {
    return 0.0f;
}

void MagicFogProjectile::Update(float afDelta) {
    if (afDelta >= 60.0) {
        SetDelete(true);
    }
    if (afDelta >= 0.0) {
        if (!eState) {
            if (GetDelete()) {
                return;
            }
            bhkCharacterController* CharController = GetCharController();
            float y = CharController->QWaterHeight();
            double v15 = HK2NI(y);
            if (GetLocationOnReference().z <= v15) {
                NiPoint3 v26 = GetLocationOnReference();
                Collide(v26, nullptr, nullptr, true);
            }
            float v14 = fSpeed * afDelta;
            NiPoint3 rdeltaMove(0.0f, v14, 0.0f);
            float v13 = 1.0 - afDelta * fMagicCloudSlowdownRate.Float();
            bhkCharacterController* v3 = GetCharController();
            v3->SetFallTime(v13);
            NiPoint3 oldLoc = GetLocationOnReference();
            Move(afDelta, rdeltaMove, 15u);
            if (IsActive()) {
                NiPoint3 newLoc = GetLocationOnReference();
                NiNode* v4 = Get3D();
                v4->SetTranslate(newLoc);
                float fdistance = (newLoc - oldLoc).Length();
                fDistanceMoved = fDistanceMoved + fdistance;
                if (fMagicProjectileMaxDistance.Float() < static_cast<double>(fDistanceMoved)) {
                    SetDelete(true);
                }
                if (fdistance / afDelta < 5.0) {
                    Collide(NiPoint3_ZERO, nullptr, nullptr, false);
                }
            }
        }
        float flastAge = MagicFogProjectile::fAge;
        MagicFogProjectile::fAge = MagicFogProjectile::fAge + afDelta;
        if (!(fProjectileDuration < static_cast<double>(MagicFogProjectile::fAge))) {
            if (pLight) {
                pLight->Update(afDelta);
            }
            if (Sound.IsValid()) {
                float fhalfDuration = fProjectileDuration / 2.0;
                if (fhalfDuration < static_cast<double>(MagicFogProjectile::fAge)) {
                    float v12 = (1.0 - (MagicFogProjectile::fAge - fhalfDuration) / fhalfDuration) * 1.0;
                    Sound.SetVolume(v12);
                }
            }
            float* v6 = &fMagicCloudFindTargetTime.Float();
            float v11 = flastAge / *v6;
            int32_t v7 = static_cast<int32_t>(std::floor(v11));
            float v10 = MagicFogProjectile::fAge / fMagicCloudFindTargetTime.Float();
            if (v7 != static_cast<int32_t>(std::floor(v10))) {
                float fpower = (fProjectileDuration - MagicFogProjectile::fAge) / fProjectileDuration;
                if (fpower > 0.0) {
                    NiPoint3 fY = GetLocationOnReference();
                    GetCaster()->FindTargets_0(pSpell, GetParentCell(), fY, this, nullptr, false, fpower, 1.0f);
                }
            }
        }
        else {
            SetDelete(true);
        }
    }
}

void MagicFogProjectile::Collide(
    NiPoint3 aContactPoint,
    bhkRigidBody* apTargetBody,
    TESObjectREFR* apCollidee,
    bool abFizzle
) {
    CFilter filter;
    bhkCharacterController* CharController = GetCharController();
    CharController->GetCollisionFilter(&filter);
    filter.SetDisabled(1);
    bhkCharacterController* v6 = GetCharController();
    v6->SetCollisionFilter(filter);
    eState = MagicFogProjectile::FOG_STATE_PERSISTANT;
    float fpower = (fProjectileDuration - MagicFogProjectile::fAge) / fProjectileDuration;
    if (!abFizzle && fpower > 0.0) {
        NiPoint3 v17 = GetLocationOnReference();
        GetCaster()->FindTargets_0(pSpell, GetParentCell(), v17, this, apCollidee, false, fpower, 1.0f);
    }
    if ((pAVEffect->data.pAreaSound) && (!abFizzle)) {
        TESSound* OpenSound = pAVEffect->data.pAreaSound;
        uint32_t a3 = OpenSound->iFormID;
        BSAudio* v8 = &BSAudio::QInstance();
        BSSoundHandle soundHand = v8->GetSoundHandleByNumericID(a3, 258u);
        NiPoint3 v13 = GetLocationOnReference();
        soundHand.SetPosition(v13.x, v13.y, v13.z);
        soundHand.Play(false);
    }
}

bool MagicFogProjectile::IsActive() {
    return (bool)((uint8_t)((eState != 1) && (!TESForm::GetDelete())));
}

void MagicFogProjectile::BuildAreaEffectGeometry() {
    NiAVObject* pav = (NiAVObject*)Get3D();
    bool v6 = true;
    if (pav) {
        NiFixedString v13("MagicAreaDisplay");
        if (!pav->GetObjectByName(v13)) {
            v6 = false;
        }
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
            pdebugSphere->SetName("MagicAreaDisplay");
            pdebugSphere->SetTranslate(NiPoint3_ZERO);
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

bool MagicFogProjectile::CheckAddTarget(MagicTarget* apSpellTarget, ActiveEffect* apEffect) {
    MagicFogProjectile::SpellTarget* pnewEntry;
    if (!apSpellTarget || !apEffect) {
        return false;
    }
    MagicItem* Spell = apEffect->pSpell;
    if (Spell != pSpell) {
        return true;
    }
    TESObjectREFR* ptarget = apSpellTarget->GetTargetStatsObject();
    EffectItem* peffect = apEffect->pEffect;
    for (MagicFogProjectile::SpellTarget* plist = pTargetList; plist; plist = plist->pNext) {
        if (plist->pTarget == ptarget && plist->pEffect == peffect) {
            return false;
        }
    }
    MagicFogProjectile::SpellTarget* apMemory =
        static_cast<MagicFogProjectile::SpellTarget*>(ourMalloc(sizeof(MagicFogProjectile::SpellTarget)));
    if (apMemory) {
        pnewEntry = ::new (apMemory) MagicFogProjectile::SpellTarget(ptarget, peffect, pTargetList);
    }
    else {
        pnewEntry = nullptr;
    }
    pTargetList = pnewEntry;
    return true;
}

void MagicFogProjectile::ClearTargetList() {
    MagicFogProjectile::SpellTarget* pTargetList;
    while (this->pTargetList) {
        pTargetList = this->pTargetList;
        this->pTargetList = pTargetList->pNext;
        ourFree(pTargetList);
    }
    this->pTargetList = nullptr;
}

void MagicFogProjectile::Init3D() {
    NiAVObject* pn = Get3D();
    if (pn) {
        NiMatrix3 rot;
        pn->SetScale(fScale);
        const NiPoint3* v1 = &GetLocationOnReference();
        pn->SetTranslate(*v1);
        NiPoint3 v12 = GetAngleOnReference();
        rot.FromEulerAnglesZXY(v12.z, v12.x, v12.y);
        pn->SetRotate(rot);
        BSUtilities::ProcessSelectiveUpdate(pn);
        NiAVObject* pareaParticles = pn->GetObjectByName("AreaEffect");
        if (pareaParticles) {
            AdjustParticles(pareaParticles);
        }
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pn->GetControllers())
        );
        if (pmanager) {
            NiControllerSequence* pseq = pmanager->GetSequenceByName("SpecialIdle_Projectile");
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

void MagicFogProjectile::AdjustParticles(NiAVObject* apav) {
    float v15 = fProjectileDuration * fMagicCloudLifeScale.Float();
    float v17 = fProjectileArea * fMagicCloudSizeScale.Float();
    float v16 = fProjectileArea * fMagicCloudSpeedScale.Float();
    if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apav)) {
        uint32_t i_2 = 0;
        for (uint32_t i = static_cast<NiParticleSystem*>(apav)->NiParticleSystem::GetModifierCount(); i_2 < i;
             i = static_cast<NiParticleSystem*>(apav)->NiParticleSystem::GetModifierCount()) {
            NiPSysModifier* pmod = static_cast<NiParticleSystem*>(apav)->NiParticleSystem::GetModifierAt(i_2);
            NiPSysSpawnModifier* pspawn =
                static_cast<NiPSysSpawnModifier*>(NiObject::DynamicCastStatic(&NiPSysSpawnModifier::ms_RTTI, pmod));
            if (pspawn) {
                float v7 = pspawn->m_fLifeSpan * v15;
                pspawn->SetLifeSpan(v7);
            }
            NiPSysEmitter* pemit =
                static_cast<NiPSysEmitter*>(NiObject::DynamicCastStatic(&NiPSysEmitter::ms_RTTI, pmod));
            if (pemit) {
                float v6 = pemit->m_fLifeSpan * v15;
                pemit->SetLifeSpan(v6);
                float v5 = pemit->m_fSpeed * v16;
                pemit->SetSpeed(v5);
                float v4 = pemit->m_fInitialRadius * v17;
                pemit->SetInitialRadius(v4);
            }
            ++i_2;
        }
    }
    else {
        if ((apav) && (apav->IsNode())) {
            uint32_t I = 0;
            for (uint32_t j = static_cast<NiNode*>(apav)->NiNode::GetArrayCount(); I < j;
                 j = static_cast<NiNode*>(apav)->NiNode::GetArrayCount()) {
                NiAVObject* pchild = static_cast<NiNode*>(apav)->NiNode::GetAt(I);
                AdjustParticles(pchild);
                ++I;
            }
        }
    }
}
