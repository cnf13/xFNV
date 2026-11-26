#include "Fallout/Magic/MagicFogProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/NewOverloads.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <cmath>
#include <limits>

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
    alignas(16) hkVector4 kVelocity(-std::numeric_limits<float>::max(), 0.0f, 0.0f, 0.0f);
    static_cast<bhkCharacterController*>(GetCharController())->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    if (GetAVEffect()->GetBoltSound()) {
        uint32_t uiSoundID = GetAVEffect()->GetBoltSound()->GetFormID();
        if (Sound.IsValid()) {
            Sound.Release();
        }
        Sound = PlaySound(
            uiSoundID,
            true,
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
    uint16_t usSize = static_cast<uint16_t>(MagicProjectile::GetSaveSize(aiFlags) + 20);
    if (eState == 1) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71u) {
        usSize += 6;
        for (MagicFogProjectile::SpellTarget* pList = pTargetList; pList; pList = pList->pNext) {
            usSize += 5;
        }
    }
    return usSize;
}

void MagicFogProjectile::SaveGame(uint32_t aiFlags) {
    uint8_t ceffectItemIndex;
    uint32_t itargetID;
    float ffadeTimer;
    MagicProjectile::SaveGame(aiFlags);
    SaveGameDataOLD(&this->MagicFogProjectile::fAge, sizeof(this->MagicFogProjectile::fAge));
    SaveGameDataOLD(&fSpeed, sizeof(fSpeed));
    SaveGameDataOLD(&fProjectileDuration, sizeof(fProjectileDuration));
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, sizeof(eState));
    SaveGameDataOLD(&fScale, sizeof(fScale));
    if (eState == MagicFogProjectile::FOG_STATE_PERSISTANT) {
        ffadeTimer = 0.0f;
        if (pLight) {
            ffadeTimer = pLight->fFadeTimer;
        }
        SaveGameDataOLD(&ffadeTimer, sizeof(ffadeTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        uint16_t usCount;
        SaveGameDataOLD(&fProjectileArea, sizeof(fProjectileArea));
        usCount = 0;
        uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveGameDataOLD(&usCount, sizeof(usCount));
        for (MagicFogProjectile::SpellTarget* plist = pTargetList; plist; plist = plist->pNext) {
            itargetID = 0;
            if (plist->pTarget) {
                itargetID = plist->pTarget->iFormID;
            }
            SaveNumericID(&itargetID, 4);
            ceffectItemIndex = 0;
            if (pSpell && plist->pEffect) {
                ceffectItemIndex = pSpell->GetEffectItemIndex(plist->pEffect);
            }

            SaveGameDataOLD(&ceffectItemIndex, sizeof(ceffectItemIndex));
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
    LoadGameDataOLD(&this->MagicFogProjectile::fAge, sizeof(this->MagicFogProjectile::fAge));
    LoadGameDataOLD(&fSpeed, sizeof(fSpeed));
    LoadGameDataOLD(&fProjectileDuration, sizeof(fProjectileDuration));
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, sizeof(eState));
    LoadGameDataOLD(&fScale, sizeof(fScale));
    if (eState == MagicFogProjectile::FOG_STATE_PERSISTANT) {
        char cFadeTimer[4];
        LoadGameDataOLD(cFadeTimer, sizeof(cFadeTimer));
        ourMemcpy(&pLight, cFadeTimer, sizeof(pLight));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&fProjectileArea, sizeof(fProjectileArea));
        LoadGameDataOLD(&usCount, sizeof(usCount));
        for (uint32_t i = 0; i < usCount; ++i) {
            MagicFogProjectile::SpellTarget* pnewEntry;
            uint32_t itargetID;
            LoadNumericID(&itargetID, 4);
            LoadGameDataOLD(&ceffectItemIndex, sizeof(ceffectItemIndex));
            // The target's form ID and the effect's index are kept in the pointer fields until InitLoadGame
            // resolves them.
            pnewEntry = new MagicFogProjectile::SpellTarget(
                reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(itargetID)),
                reinterpret_cast<EffectItem*>(static_cast<uintptr_t>(ceffectItemIndex)),
                pTargetList
            );
            pTargetList = pnewEntry;
        }
    }
}

void MagicFogProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    float ffadeTimer;
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    Init3D();
    GetParentCell()->AddReference(this, false);
    if (eState != MagicFogProjectile::FOG_STATE_PERSISTANT && pAVEffect->GetBoltSound()) {
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

    ourMemcpy(&ffadeTimer, &pLight, sizeof(ffadeTimer));
    pLight = nullptr;
    if (Get3D() && pAVEffect->data.pLight) {
        pLight = new MagicLight(pAVEffect->data.pLight->iFormID, Get3D());
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
                TESForm* pForm = TESForm::GetFormByNumericID(itargetID);
                plist->pTarget = dynamic_cast<TESObjectREFR*>(pForm);
            }
            if (pSpell) {
                plist->pEffect = pSpell->GetEffectItemByIndex(ceffectItemIndex);
            }
        }
    }
}

void MagicFogProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESObjectREFR* pCaster;
    CFilter filter;
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        uint32_t uiGroup;
        if (GetCaster() && GetCaster()->GetCasterStatsObject()) {
            TESObjectREFR* pref = GetCaster()->GetCasterStatsObject();
            Actor* pActor = dynamic_cast<Actor*>(pref);
            if (pActor) {
                // GetGroup is called on the returned pointer, which is &kFilter.
                CFilter kFilter;
                pActor->GetCollisionFilter(&kFilter.iFilter);
                uiGroup = kFilter.GetGroup();
            }
            else {
                bhkRigidBody* pBody;
                bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(pref->Get3D());
                if (pCollisionObject) {
                    pBody = pCollisionObject->GetRigidBody();
                }
                else {
                    pBody = nullptr;
                }
                if (pBody) {
                    CFilter kFilter;
                    uiGroup = pBody->GetCollisionFilter(&kFilter)->GetGroup();
                }
                else {
                    uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                }
            }
        }
        else {
            bhkRigidBody* pBody;
            bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(Get3D());
            if (pCollisionObject) {
                pBody = pCollisionObject->GetRigidBody();
            }
            else {
                pBody = nullptr;
            }
            if (pBody) {
                CFilter kFilter;
                uiGroup = pBody->GetCollisionFilter(&kFilter)->GetGroup();
            }
            else {
                uiGroup = bhkCollisionFilter::GetNewSystemGroup();
            }
        }
        pcon->GetCollisionFilter(&filter);
        filter.SetLayer(L_SPELL);
        filter.SetGroup(uiGroup);
        pcon->SetCollisionFilter(filter);
    }
    if (GetCaster()) {
        pCaster = GetCaster()->GetCasterStatsObject();
    }
    else {
        pCaster = nullptr;
    }
    if (pCaster != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultFog.Float());
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
            float fHavokWaterHeight = GetCharController()->QWaterHeight();
            double dWaterHeight = HK2NI(fHavokWaterHeight);
            if (GetLocationOnReference().z <= dWaterHeight) {
                NiPoint3 kLocation = GetLocationOnReference();
                Collide(kLocation, nullptr, nullptr, true);
            }
            NiPoint3 rdeltaMove(0.0f, fSpeed * afDelta, 0.0f);
            float fFallTime = 1.0 - afDelta * fMagicCloudSlowdownRate.Float();
            GetCharController()->SetFallTime(fFallTime);
            NiPoint3 oldLoc = GetLocationOnReference();
            Move(afDelta, rdeltaMove, 15u);
            if (IsActive()) {
                NiPoint3 newLoc = GetLocationOnReference();
                Get3D()->SetTranslate(newLoc);
                float fdistance = (newLoc - oldLoc).Length();
                fDistanceMoved = fDistanceMoved + fdistance;
                if (fMagicProjectileMaxDistance.Float() < static_cast<double>(fDistanceMoved)) {
                    SetDelete(true);
                }
                if (fdistance / afDelta < 5.0) {
                    Collide(NiPoint3::ZERO, nullptr, nullptr, false);
                }
            }
        }
        float flastAge = MagicFogProjectile::fAge;
        MagicFogProjectile::fAge = MagicFogProjectile::fAge + afDelta;
        if (fProjectileDuration < static_cast<double>(MagicFogProjectile::fAge)) {
            SetDelete(true);
        }
        else {
            if (pLight) {
                pLight->Update(afDelta);
            }
            if (Sound.IsValid()) {
                float fhalfDuration = fProjectileDuration / 2.0;
                if (fhalfDuration < static_cast<double>(MagicFogProjectile::fAge)) {
                    float fVolume = (1.0 - (MagicFogProjectile::fAge - fhalfDuration) / fhalfDuration) * 1.0;
                    Sound.SetVolume(fVolume);
                }
            }
            float fLastPeriods = flastAge / fMagicCloudFindTargetTime.Float();
            int32_t iLastPeriod = static_cast<int32_t>(std::floor(fLastPeriods));
            float fPeriods = MagicFogProjectile::fAge / fMagicCloudFindTargetTime.Float();
            if (iLastPeriod != static_cast<int32_t>(std::floor(fPeriods))) {
                float fpower = (fProjectileDuration - MagicFogProjectile::fAge) / fProjectileDuration;
                if (fpower > 0.0) {
                    NiPoint3 kLocation = GetLocationOnReference();
                    GetCaster()->FindTargets_0(pSpell, GetParentCell(), kLocation, this, nullptr, false, fpower, 1.0f);
                }
            }
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
    GetCharController()->GetCollisionFilter(&filter);
    filter.SetDisabled(1);
    GetCharController()->SetCollisionFilter(filter);
    eState = MagicFogProjectile::FOG_STATE_PERSISTANT;
    float fpower = (fProjectileDuration - MagicFogProjectile::fAge) / fProjectileDuration;
    if (!abFizzle && fpower > 0.0) {
        NiPoint3 kLocation = GetLocationOnReference();
        GetCaster()->FindTargets_0(pSpell, GetParentCell(), kLocation, this, apCollidee, false, fpower, 1.0f);
    }
    if (pAVEffect->data.pAreaSound && !abFizzle) {
        uint32_t uiSoundID = pAVEffect->data.pAreaSound->iFormID;
        BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 258u);
        NiPoint3 kLocation = GetLocationOnReference();
        soundHand.SetPosition(kLocation.x, kLocation.y, kLocation.z);
        soundHand.Play(false);
    }
}

bool MagicFogProjectile::IsActive() {
    return eState != FOG_STATE_PERSISTANT && !TESForm::GetDelete();
}

void MagicFogProjectile::BuildAreaEffectGeometry() {
    NiAVObject* pav = Get3D();
    bool bHasDisplay = true;
    if (pav) {
        NiFixedString kDisplayName("MagicAreaDisplay");
        if (!pav->GetObjectByName(kDisplayName)) {
            bHasDisplay = false;
        }
    }
    if (!bHasDisplay) {
        NiNode* pnode = pav->IsNode();
        if (pnode) {
            NiMaterialProperty* pmat;
            CFilter kFilter;
            double dArea = static_cast<double>(GetDominantEffect()->GetArea());
            double dAreaUnits = fMagicUnitsPerFoot.Float() * dArea;
            float farea = dAreaUnits / pav->m_kLocal.m_fScale;
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

bool MagicFogProjectile::CheckAddTarget(MagicTarget* apSpellTarget, ActiveEffect* apEffect) {
    MagicFogProjectile::SpellTarget* pnewEntry;
    if (!apSpellTarget || !apEffect) {
        return false;
    }
    if (apEffect->pSpell != pSpell) {
        return true;
    }
    TESObjectREFR* ptarget = apSpellTarget->GetTargetStatsObject();
    EffectItem* peffect = apEffect->pEffect;
    for (MagicFogProjectile::SpellTarget* plist = pTargetList; plist; plist = plist->pNext) {
        if (plist->pTarget == ptarget && plist->pEffect == peffect) {
            return false;
        }
    }
    pnewEntry = new MagicFogProjectile::SpellTarget(ptarget, peffect, pTargetList);
    pTargetList = pnewEntry;
    return true;
}

void MagicFogProjectile::ClearTargetList() {
    while (pTargetList) {
        MagicFogProjectile::SpellTarget* pTarget = pTargetList;
        pTargetList = pTarget->pNext;
        ourFree(pTarget);
    }
    pTargetList = nullptr;
}

void MagicFogProjectile::Init3D() {
    NiAVObject* pn = Get3D();
    if (pn) {
        NiMatrix3 rot;
        pn->SetScale(fScale);
        pn->SetTranslate(GetLocationOnReference());
        NiPoint3 kAngle = GetAngleOnReference();
        rot.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
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
                pseq->SetOffset(-std::numeric_limits<float>::max());
                float fUpdateTime = fCellAppTime - 0.10000000149011612;
                NiUpdateData kUpdateData(fUpdateTime, true, false);
                pn->Update(kUpdateData);
            }
        }
    }
}

void MagicFogProjectile::AdjustParticles(NiAVObject* apav) {
    float fLifeScale = fProjectileDuration * fMagicCloudLifeScale.Float();
    float fSizeScale = fProjectileArea * fMagicCloudSizeScale.Float();
    float fSpeedScale = fProjectileArea * fMagicCloudSpeedScale.Float();
    if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apav)) {
        for (uint32_t i = 0; i < static_cast<NiParticleSystem*>(apav)->NiParticleSystem::GetModifierCount(); ++i) {
            NiPSysModifier* pmod = static_cast<NiParticleSystem*>(apav)->NiParticleSystem::GetModifierAt(i);
            NiPSysSpawnModifier* pspawn = static_cast<NiPSysSpawnModifier*>(
                NiObject::DynamicCastStatic(&NiPSysSpawnModifier::ms_RTTI, pmod)
            );
            if (pspawn) {
                pspawn->SetLifeSpan(pspawn->m_fLifeSpan * fLifeScale);
            }
            NiPSysEmitter* pemit = static_cast<NiPSysEmitter*>(
                NiObject::DynamicCastStatic(&NiPSysEmitter::ms_RTTI, pmod)
            );
            if (pemit) {
                pemit->SetLifeSpan(pemit->m_fLifeSpan * fLifeScale);
                pemit->SetSpeed(pemit->m_fSpeed * fSpeedScale);
                pemit->SetInitialRadius(pemit->m_fInitialRadius * fSizeScale);
            }
        }
    }
    else if (apav && apav->IsNode()) {
        for (uint32_t i = 0; i < static_cast<NiNode*>(apav)->NiNode::GetArrayCount(); ++i) {
            NiAVObject* pchild = static_cast<NiNode*>(apav)->NiNode::GetAt(i);
            AdjustParticles(pchild);
        }
    }
}
