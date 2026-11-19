#include "Fallout/Magic/MagicBoltProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BoltShader.h"
#include "BSShader/BoltShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/bhkCharacterListenerSpell.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/rtti.h"

BoltSavedData::BoltSavedData() : pAreaBoltData(nullptr) {}

MagicBoltProjectile::AreaBolt::AreaBolt()
  : spBoltProperty(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr) {}

void MagicBoltProjectile::AreaBolt::operator delete(void* apMemory) {
    ourFree(apMemory);
}

MagicBoltProjectile::MagicBoltProjectile()
  : spBoltProperty(nullptr), pAreaBolts(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr),
    spSoundNode(nullptr), pTarget(nullptr), fCollideTime(0.0f) {
    SetState(MagicBoltProjectile::TRUNK);
    fSpeed = 4096.0f;
}

// A bolt the visible player casts starts fShockCastVOffset higher. The 3D cloned from the effect is dropped at once.
// The bolt starts with the lowest possible x velocity; the effect's bolt sound follows the sound node, looping.
MagicBoltProjectile::MagicBoltProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    EffectSetting* apAVEffect,
    NiPoint3 aOrigin,
    NiPoint3 aRotation
)
  : MagicProjectile(apCaster, apSpell, apDominantEffect, apAVEffect, aOrigin, aRotation), spBoltProperty(nullptr),
    pAreaBolts(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr), spSoundNode(nullptr),
    pTarget(nullptr), fCollideTime(0.0f) {
    BoltShader::SetSubSegmentCount(iShockSubSegments.UInt());
    SetState(TRUNK);
    double dProjectileSpeed = GetAVEffect()->GetProjectileSpeed();
    fSpeed = static_cast<float>(fMagicProjectileBaseSpeed.Float() * dProjectileSpeed);
    if (apCaster->GetCasterStatsObject() == pPlayer && pPlayer->Get3D(true) && !pPlayer->Get3D(true)->GetAppCulled()) {
        NiPoint3 kLocation = *QLocationOnReference();
        kLocation.z += fShockCastVOffset.Float();
        SetLocationOnReference(kLocation);
    }
    { NiPointer<NiAVObject> spClone(MagicProjectile::Clone3D_0(GetAVEffect())); }
    Init3D();
    Set3DSimple(spBoltNode);
    MagicBoltProjectile::InitHavok();
    alignas(16) hkVector4 kVelocity(-3.40282347e+38f, 0.0f, 0.0f, 0.0f);
    static_cast<bhkCharacterController*>(GetCharController())->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    SetState(TRUNK);
    if (GetAVEffect()->GetBoltSound()) {
        uint32_t uiSoundID = GetAVEffect()->GetBoltSound()->GetFormID();
        if (Sound.IsValid()) {
            Sound.Release();
        }
        if (spSoundNode) {
            Sound = BSAudio::QInstance().GetSoundHandleByNumericID(
                uiSoundID, BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            NiPoint3 kPosition = *QLocationOnReference();
            Sound.SetPosition(kPosition.x, kPosition.y, kPosition.z);
            Sound.SetObjectToFollow(spSoundNode);
            Sound.Play(true);
        }
    }
    CheckShotBehindWall();
    TESObjectREFR* pCasterRef = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pCasterRef == pPlayer) {
        ReferenceProcessLists.ProjectilePostProcessList.AddHead(this);
    }
    else {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
}

// The pointer members release their objects in their own destructors; each area bolt is stopped and detached here,
// then deleted.
MagicBoltProjectile::~MagicBoltProjectile() {
    MagicCaster* pPlayerCaster;
    if (pPlayer) {
        pPlayerCaster = &static_cast<MagicCaster&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
    }
    else {
        pPlayerCaster = nullptr;
    }
    if (GetCaster() != pPlayerCaster) {
        MagicProjectile::RemoveTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
    ReferenceProcessLists.ProjectilePostProcessList.Remove(static_cast<MobileObject*>(this));
    BoltShader::Stop(spBoltNode.m_pObject, spBoltProperty.m_pObject);
    if (spBoltNode.m_pObject) {
        NiNode* pparent = spBoltNode.m_pObject->m_pkParent;
        if (pparent) {
            pparent->DetachChild(spBoltNode.m_pObject);
        }
    }
    while (pAreaBolts) {
        MagicBoltProjectile::AreaBolt* pnextBolt = pAreaBolts->pNext;
        BoltShader::Stop(pAreaBolts->spBoltNode.m_pObject, pAreaBolts->spBoltProperty.m_pObject);
        if (pAreaBolts->spBoltNode.m_pObject) {
            NiNode* pparent_2 = pAreaBolts->spBoltNode.m_pObject->m_pkParent;
            if (pparent_2) {
                pparent_2->DetachChild(pAreaBolts->spBoltNode.m_pObject);
            }
        }
        delete pAreaBolts;
        pAreaBolts = pnextBolt;
    }
    if (Sound.IsValid()) {
        Sound.Stop();
        Sound.Release();
    }
}

uint16_t MagicBoltProjectile::GetSaveSize(uint32_t aiFlags) {
    MagicBoltProjectile::AreaBolt* i;
    int16_t v4 = (int16_t)(MagicProjectile::GetSaveSize(aiFlags) + 8);
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x30u) {
        v4 += 32;
    }
    uint16_t v5 = (uint16_t)(v4 + 6);
    for (i = pAreaBolts; i; i = i->pNext) {
        v5 += 4;
    }
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x71u) {
        v5 += 4;
    }
    return v5;
}

// The area bolts' count is written as a placeholder first and patched through the save buffer's position taken before
// that write.
void MagicBoltProjectile::SaveGame(uint32_t aiFlags) {
    uint32_t imagicTargetID;
    NiPoint3 startingLocation;
    NiQuaternion rotation;
    float fboltTimer;
    uint16_t icount;
    uint32_t itargetID;
    MagicProjectile::SaveGame(aiFlags);
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, 4u);
    SaveGameDataOLD(&fSpeed, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 48u) {
        startingLocation = Get3D()->GetTranslate();
        SaveGameDataOLD(&startingLocation, 12);
        NiNode* v2 = Get3D();
        v2->GetRotate(rotation);
        pSaveLoadGameOLD->SaveGameDataOLD(&rotation, 16u);
        fboltTimer = 0.0f;
        if (spBoltProperty.m_pObject) {
            fboltTimer = spBoltProperty.m_pObject->fTimer;
        }
        SaveGameDataOLD(&fboltTimer, 4);
    }
    itargetID = 0;
    if (pTarget) {
        itargetID = pTarget->iFormID;
    }
    SaveNumericID(&itargetID, 4);
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    SaveGameDataOLD(&icount, 2);
    for (MagicBoltProjectile::AreaBolt* pbolts = pAreaBolts; pbolts; pbolts = pbolts->pNext) {
        imagicTargetID = 0;
        if (pbolts->spTarget.m_pObject) {
            TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pbolts->spTarget.m_pObject);
            if (pref) {
                imagicTargetID = pref->iFormID;
            }
        }
        SaveNumericID(&imagicTargetID, 4);
        ++icount;
    }
    *pcount = icount;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        SaveGameDataOLD(&fCollideTime, 4);
    }
}

// The saved bolt state is read into a BoltSavedData kept in pAreaBolts until FinishInitLoadGame applies and frees it.
// The area bolts' targets are read as a count, then stored as a count followed by that many form IDs.
void MagicBoltProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    BoltSavedData* psavedData;
    uint16_t icount;
    uint32_t itargetID;
    InitHavok();
    MagicProjectile::LoadGame(aiFlags, aiCurrentFlags);
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, 4u);
    LoadGameDataOLD(&fSpeed, 4);
    BoltSavedData* pmemory = static_cast<BoltSavedData*>(ourMalloc(sizeof(BoltSavedData)));
    if (pmemory) {
        psavedData = ::new (pmemory) BoltSavedData;
    }
    else {
        psavedData = nullptr;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 48u) {
        LoadGameDataOLD(&psavedData->StartingLocation, 12);
        pSaveLoadGameOLD->LoadGameDataOLD(&psavedData->Rotation, 16u);
        LoadGameDataOLD(&psavedData->fBoltTimer, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 48u) {
        psavedData->StartingLocation = pPlayer->GetLocationOnReference();
        psavedData->Rotation = NiQuaternion_IDENTITY;
    }
    LoadNumericID(&itargetID, 4);
    pTarget = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(itargetID));
    LoadGameDataOLD(&icount, 2);
    if (icount) {
        uint32_t uiEntries = icount + 1u;
        psavedData->pAreaBoltData = static_cast<uint32_t*>(
            ourMalloc(uiEntries > SIZE_MAX / sizeof(uint32_t) ? SIZE_MAX : sizeof(uint32_t) * uiEntries)
        );
        psavedData->pAreaBoltData[0] = icount;
        LoadNumericID(&psavedData->pAreaBoltData[1], icount * sizeof(uint32_t));
    }
    pSavedData = psavedData;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&fCollideTime, 4);
    }
}

void MagicBoltProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget));
    if (itargetID) {
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(itargetID);
        pTarget = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (pTarget) {
            NiAVObject* pav = pTarget->Get3D();
            if (pav) {
                NiFixedString apString("Bip01 Spine2");
                NiFixedString* v7 = &apString;
                spTarget = pav->GetObjectByName(*v7);
            }
        }
    }
    Init3D();
    Set3DSimple(spBoltNode.m_pObject);
    GetParentCell()->AddReference(this, false);
    if (pAVEffect->GetBoltSound()) {
        uint32_t isoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        if (Get3D()) {
            BSAudio* v4 = &BSAudio::QInstance();
            Sound = v4->GetSoundHandleByNumericID(isoundFileID, 258u);
            NiPoint3 v11 = GetLocationOnReference();
            Sound.SetPosition(v11.x, v11.y, v11.z);
            Sound.SetObjectToFollow(spSoundNode.m_pObject);
            Sound.Play(true);
        }
    }
}

// Applies the state LoadGame left in pAreaBolts as a BoltSavedData: the bolt node's place and rotation, the bolt's
// shape towards the target (or the projectile), the area targets; then frees it (its target array stays allocated,
// deliberately, as the game does). The actor's filter result is the filter word itself
// (MobileObject::GetCollisionFilter returns its argument), so GetGroup runs on that filter.
void MagicBoltProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESObjectREFR* pcaster;
    CFilter v30;
    CFilter apResult;
    CFilter filter_2;
    CFilter filter;
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        MagicCaster* v27;
        uint32_t igroup;
        if (GetCaster() && (v27 = GetCaster(), v27->GetCasterStatsObject())) {
            MagicCaster* Caster = GetCaster();
            TESObjectREFR* pref = Caster->GetCasterStatsObject();
            Actor* pactor = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
            if (pactor) {
                CFilter v32;
                pactor->GetCollisionFilter(&v32.iFilter);
                igroup = v32.GetGroup();
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
                CFilter* v7 = prb_2->GetCollisionFilter(&v30);
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
        MagicCaster* v21 = GetCaster();
        pcaster = v21->GetCasterStatsObject();
    }
    else {
        pcaster = nullptr;
    }
    if (pcaster != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
    BoltSavedData* ptempData = pSavedData;
    if (ptempData) {
        if (spBoltNode.m_pObject) {
            spBoltNode.m_pObject->SetTranslate(ptempData->StartingLocation);
            spBoltNode.m_pObject->SetRotate_0(ptempData->Rotation);
            bool bpostHit = eState != MagicBoltProjectile::TRUNK;
            if (spTarget.m_pObject) {
                NiMatrix3 rot;
                NiPoint3 toTarget = spTarget.m_pObject->GetWorldTranslate() - ptempData->StartingLocation;
                spBoltProperty.m_pObject->UpdateBolt(ptempData->fBoltTimer, toTarget.Length(), fSpeed, bpostHit);
                float fz = toTarget.z;
                toTarget.z = 0.0f;
                float fxy = toTarget.Length();
                float fYAngle = -NiATan2(fz, fxy);
                float fZAngle = NiATan2(toTarget.x, toTarget.y);
                rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
                spBoltNode.m_pObject->SetRotate(rot);
            }
            else {
                NiPoint3 endLocation = GetLocationOnReference();
                float fboltLength = (ptempData->StartingLocation - endLocation).Length();
                spBoltProperty.m_pObject->UpdateBolt(ptempData->fBoltTimer, fboltLength, fSpeed, bpostHit);
                if (spBoltProperty.m_pObject->IsAtMaxLength() || eState == MagicBoltProjectile::PERSIST) {
                    SetState(MagicBoltProjectile::PERSIST);
                    GetCharController()->GetCollisionFilter(&filter_2);
                    filter_2.SetDisabled(1);
                    GetCharController()->SetCollisionFilter(filter_2);
                }
            }
        }
        pAreaBolts = nullptr;
        if (ptempData->pAreaBoltData) {
            // Only the count's low 16 bits are read.
            uint16_t icount = static_cast<uint16_t>(ptempData->pAreaBoltData[0]);
            for (uint32_t i = 0; i < icount; ++i) {
                uint32_t imagicTargetID = ptempData->pAreaBoltData[i + 1];
                MagicTarget* pmagicTarget = MagicTarget::GetMagicTargetByNumericID(imagicTargetID);
                if (pmagicTarget) {
                    AddAreaTarget(pmagicTarget, nullptr);
                }
            }
        }
        ourFree(ptempData);
    }
}

// Gives a high-process bolt a character controller (a 1-unit sphere in the spell layer, in the caster's collision
// group, without gravity) when it has none, then puts the controller in the cell's world at the bolt's position, 8
// units higher when it changes worlds. MagicBallProjectile::InitHavok's steps, except that the projectile flag is set
// before the quick-simulate one.
void MagicBoltProjectile::InitHavok() {
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
        // Allocates the listener with the 16-aligned allocator it shares with bhkRagdollController.
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
        pController->SetIsProjectile(true);
        pController->SetQuickSimulate(Main::GetSceneIsComplex());
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

void MagicBoltProjectile::Update(float afDelta) {
    static char bstop;
    CFilter filter;
    NiPoint3 dst_12;
    if (afDelta != 0.0 && !GetDelete()) {
        bhkCharacterController* CharController;
        float Y;
        float v26;
        double v27;
        NiPoint3 dst;
        NiPoint3 v47;
        NiPoint3 v48 = Get3D()->GetTranslate();
        if (!spBoltProperty.m_pObject || eState) {
            if (eState == MagicBoltProjectile::BRANCH) {
                MagicBoltProjectile::AreaBolt* pbranch = pAreaBolts;
                MagicBoltProjectile::AreaBolt* pprevious = nullptr;
                MagicBoltProjectile::AreaBolt* v43 = nullptr;
                while (pbranch) {
                    UpdateBranch(afDelta, pbranch);
                    MagicBoltProjectile::AreaBolt* pnext = pbranch->pNext;
                    if (pbranch->spBoltProperty.m_pObject->IsFinished()) {
                        NiNode* pparent = pbranch->spBoltNode.m_pObject->m_pkParent;
                        if (pparent) {
                            pparent->DetachChild(pbranch->spBoltNode.m_pObject);
                        }
                        if (pprevious) {
                            pprevious->pNext = pbranch->pNext;
                        }
                        else {
                            pAreaBolts = pbranch->pNext;
                        }
                        pbranch->spBoltNode = static_cast<NiNode*>(nullptr);
                        pbranch->spBoltProperty = static_cast<BoltShaderProperty*>(nullptr);
                        pbranch->spSource = static_cast<NiAVObject*>(nullptr);
                        pbranch->spTarget = static_cast<NiAVObject*>(nullptr);
                        MagicBoltProjectile::AreaBolt* v30 = pbranch;
                        MagicBoltProjectile::AreaBolt* pbranch_2 = pbranch;
                        if (pbranch) {
                            delete pbranch_2;
                        }
                    }
                    else {
                        pprevious = pbranch;
                    }
                    pbranch = pnext;
                }
            }
            goto LABEL_29;
        }
        CharController = GetCharController();
        Y = CharController->QWaterHeight();
        v27 = HK2NI(Y);
        if ((&GetLocationOnReference())->z <= v27) {
            Collide(GetLocationOnReference(), nullptr, nullptr, true);
        }
        v26 = fSpeed * afDelta;
        ::new (&dst_12) NiPoint3(0.0f, v26, 0.0f);
        v47 = NiPoint3_ZERO;
        if (GetCharController()) {
            bhkCharacterController* v3 = GetCharController();
            v3->GetPosition(v47);
        }
        Move(afDelta, dst_12, 15u);
        dst = NiPoint3_ZERO;
        if (GetCharController()) {
            bhkCharacterController* v4 = GetCharController();
            v4->GetPosition(dst);
        }
        fDistanceMoved = (dst - v47).Length() + fDistanceMoved;
        if (fMagicProjectileMaxDistance.Float() < static_cast<double>(fDistanceMoved)) {
            SetDelete(true);
        }
        if (IsActive()) {
        LABEL_29:;
            if (iShockDebug.Int() <= 0 && !bstop) {
                bool bpostHit = eState != MagicBoltProjectile::TRUNK;
                if (spTarget.m_pObject) {
                    double CurrentFurniture;
                    double v25;
                    NiMatrix3 rot;
                    NiPoint3 toTarget = spTarget.m_pObject->GetWorldTranslate() - v48;
                    if (!bpostHit ||
                        (v25 = fAge * fSpeed,
                         (CurrentFurniture = static_cast<double>(spBoltProperty.m_pObject->iSegmentsPerBolt),
                          spBoltProperty.m_pObject->fSegmentLength * CurrentFurniture < v25))) {
                        float X = fSpeed;
                        float fxy = toTarget.Length();
                        spBoltProperty.m_pObject->UpdateBolt(afDelta, fxy, X, bpostHit);
                    }
                    float fz = toTarget.z;
                    toTarget.z = 0.0f;
                    float fxy_2 = toTarget.Length();
                    float Ya = -NiATan2(fz, fxy_2);
                    float xa = NiATan2(toTarget.x, toTarget.y);
                    rot.FromEulerAnglesZYX(xa, 0.0f, Ya);
                    spBoltNode.m_pObject->SetRotate(rot);
                }
                else {
                    if (GetCharController()) {
                        double v20;
                        double v23;
                        NiPoint3 kPosition;
                        bhkCharacterController* v7 = GetCharController();
                        v7->GetPosition(kPosition);
                        float flength = (v48 - kPosition).Length();
                        if (!bpostHit ||
                            (v23 = fAge * fSpeed,
                             (v20 = static_cast<double>(spBoltProperty.m_pObject->iSegmentsPerBolt),
                              spBoltProperty.m_pObject->fSegmentLength * v20 < v23))) {
                            spBoltProperty.m_pObject->UpdateBolt(afDelta, flength, fSpeed, bpostHit);
                        }
                        if (spBoltProperty.m_pObject->IsAtMaxLength()) {
                            CFilter filter_2;
                            SetState(MagicBoltProjectile::PERSIST);
                            bhkCharacterController* v9 = GetCharController();
                            v9->GetCollisionFilter(&filter);
                            filter.SetDisabled(1);
                            filter_2.iFilter = filter.iFilter;
                            bhkCharacterController* v10 = GetCharController();
                            v10->SetCollisionFilter(filter_2);
                        }
                    }
                }
            }
            if (spSoundNode.m_pObject) {
                const NiPoint3* v11 = &GetLocationOnReference();
                spSoundNode.m_pObject->SetWorldTranslate(*v11);
            }
            if (spBoltProperty.m_pObject->IsFinished() ||
                (eState == MagicBoltProjectile::BRANCH && fLastUpdate - fCollideTime > 3.0)) {
                if (Sound.IsValid()) {
                    Sound.Stop();
                    Sound.Release();
                }
                if (!pAreaBolts) {
                    SetDelete(true);
                }
            }
        }
    }
}

void MagicBoltProjectile::Collide(
    NiPoint3 aContactPoint,
    bhkRigidBody* apTargetBody,
    TESObjectREFR* apCollidee,
    bool abFizzle
) {
    Actor* v23;
    NiPoint3 v25;
    CFilter filter;
    if (apCollidee && apCollidee->IsActor()) {
        v23 = static_cast<Actor*>(apCollidee);
    }
    else {
        v23 = nullptr;
    }
    if (!v23 || !v23->GetGhost()) {
        fCollideTime = fLastUpdate;
        if (GetCharController()) {
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
        SetState(MagicBoltProjectile::BRANCH);
        NiAVObject* pav = Get3D();
        if (pav) {
            NiPoint3 afTimeDelta = (&pav->GetWorldTransform())->m_Rotate * NiPoint3_UNIT_Y;
            ApplyCollisionForce(aContactPoint, afTimeDelta, apTargetBody);
        }
        if ((apCollidee) && (apCollidee != pPlayer || pPlayer->Is3rdPerson())) {
            pTarget = apCollidee;
            NiAVObject* pav_2 = pTarget->Get3D();
            if (pav_2) {
                NiFixedString apString("Bip01 Spine2");
                NiFixedString* v22 = &apString;
                spTarget = pav_2->GetObjectByName(*v22);
            }
        }

        NiPoint3 startingLocation = Get3D()->GetTranslate();
        float fBoltLength = (startingLocation - aContactPoint).Length();
        spBoltProperty.m_pObject->UpdateBolt(0.01f, fBoltLength, fSpeed, false);
        if (!abFizzle) {
            NiPoint3 v44 = GetLocationOnReference();
            GetCaster()->FindTargets_0(pSpell, GetParentCell(), v44, this, apCollidee, false, 1.0f, 1.0f);
        }
        pTarget = nullptr;
        if (pAVEffect->GetModel()) {
            NiPoint3* v18;
            double Area = static_cast<double>(GetDominantEffect()->GetArea());
            float fscale = fMagicAreaScale.Float() * Area;
            float* v9 = &fMagicAreaScaleMax.Float();
            if (!(*v9 < static_cast<double>(fscale))) {
                float* v10 = &fMagicAreaScaleMin.Float();
                if (*v10 > static_cast<double>(fscale)) {
                    fscale = fMagicAreaScaleMin.Float();
                }
            }
            else {
                fscale = fMagicAreaScaleMax.Float();
            }
            const char* pfilename = pAVEffect->GetModel();
            if (Get3D()) {
                NiNode* v11 = Get3D();
                v18 = ::new (&v25) NiPoint3((&v11->m_kLocal)->m_Rotate * NiPoint3_UNIT_Y);
            }
            else {
                v18 = &NiPoint3_UNIT_Y;
            }
            NiPoint3 orient = *v18;
            BSTempEffectParticle* pareaEffect =
                BSTempEffectParticle::Spawn(GetParentCell(), 5.0f, pfilename, orient, aContactPoint, 1.0f, 7u, nullptr);
            pareaEffect->PlaySequence("SpecialIdle_AreaEffect");
        }
        if ((pAVEffect->data.pAreaSound) && (!abFizzle)) {
            TESSound* OpenSound = pAVEffect->data.pAreaSound;
            uint32_t isoundFileID = OpenSound->iFormID;
            BSAudio* v13 = &BSAudio::QInstance();
            BSSoundHandle soundHand = v13->GetSoundHandleByNumericID(isoundFileID, 258u);
            NiPoint3 v32 = GetLocationOnReference();
            soundHand.SetPosition(v32.x, v32.y, v32.z);
            soundHand.Play(false);
        }
    }
}

void MagicBoltProjectile::AddAreaTarget(MagicTarget* apTarget, ActiveEffect* apActiveEffect) {
    EffectItem* EffectItem_;
    if (!apActiveEffect ||
        ((EffectItem_ = apActiveEffect->pEffect, EffectItem_ == GetDominantEffect()) &&
         pTarget != apTarget->GetTargetStatsObject())) {
        NiNode* pav;
        NiNode* pnode;
        NiPoint3 location;
        void* apMemory = NiMemObject::operator new(sizeof(NiNode));
        if (apMemory) {
            pnode = ::new (apMemory) NiNode(0);
        }
        else {
            pnode = nullptr;
        }
        NiNode* pparentNode = static_cast<NiNode*>(GetParentCell()->GetDynamicNode());
        pparentNode->AttachChild(pnode, 1);
        BoltShaderProperty* pprop = BoltShader::Start(pnode, false);
        if (pprop) {
            pprop->SetAbsorbNumBolts(iShockBranchNumBolts.Int());
            float* v4 = &fShockBranchBoltsRadius.Float();
            pprop->SetBoltsRadius(*v4);
            float* v5 = &fShockBranchBoltsRadiusStrength.Float();
            pprop->SetBoltsRadiusStrength(*v5);
            float* v6 = &fShockBoltSmallWidth.Float();
            pprop->SetBoltSmallWidth(*v6);
            float* v7 = &fShockBoltGrowWidth.Float();
            pprop->SetBoltGrowWidth(*v7);
            float a4 = fShockCoreColorB.Float();
            float a3 = fShockCoreColorG.Float();
            float* v8 = &fShockCoreColorR.Float();
            NiColorA kCoreColor(*v8, a3, a4, 1.0f);
            NiColorA* v9 = &kCoreColor;
            pprop->SetAbsorbCoreColor(v9->r, v9->g, v9->b, v9->a);
            float a4a = fShockGlowColorB.Float();
            float a3a = fShockGlowColorG.Float();
            float* v10 = &fShockGlowColorR.Float();
            NiColorA kGlowColor(*v10, a3a, a4a, 1.0f);
            NiColorA* v11 = &kGlowColor;
            pprop->SetAbsorbGlowColor(v11->r, v11->g, v11->b, v11->a);
            float* v12 = &fShockBranchSegmentLength.Float();
            pprop->SetAbsorbSegmentLength(*v12);
            int32_t a2 = iShockBranchSegmentsPerBolt.Int();
            pprop->iSegmentsPerBolt = a2;
            float* v13 = &fShockBranchSegmentVariance.Float();
            pprop->SetSegmentVariance(*v13);
            float* v14 = &gs_fSubSegmentVariance.Float();
            pprop->SetAbsorbTortuosityVariance(*v14);
            pprop->SetStartFromPoint(false);
            pprop->InitSegmentData();
        }
        bhkCharacterController* CharController = GetCharController();
        CharController->GetPosition(location);
        NiAVObject* psource = nullptr;
        if (pTarget) {
            pav = pTarget->Get3D();
        }
        else {
            pav = nullptr;
        }
        NiAVObject* pav_2 = (NiAVObject*)pav;
        if (pav) {
            NiFixedString apString("Bip01 Spine2");
            NiFixedString* v25 = &apString;
            psource = pav_2->GetObjectByName(*v25);
        }
        NiRefObject* v36 = nullptr;
        TESObjectREFR* v24 = apTarget->GetTargetStatsObject();
        pav_2 = (NiAVObject*)v24->Get3D();
        if (pav_2) {
            MagicBoltProjectile::AreaBolt* pnewBolt;
            NiAVObject* ptarget = pav_2->GetObjectByName("Bip01 Spine2");
            if (!ptarget) {
                ptarget = pav_2;
            }
            MagicBoltProjectile::AreaBolt* v16 =
                static_cast<MagicBoltProjectile::AreaBolt*>(ourMalloc(sizeof(MagicBoltProjectile::AreaBolt)));
            if (v16) {
                pnewBolt = ::new (v16) MagicBoltProjectile::AreaBolt;
            }
            else {
                pnewBolt = nullptr;
            }
            pnewBolt->spBoltProperty = pprop;
            pnewBolt->spBoltNode = pnode;
            pnewBolt->spSource = psource;
            pnewBolt->spTarget = ptarget;
            pnewBolt->SourceLoc = location;
            pnewBolt->pNext = pAreaBolts;
            UpdateBranch(0.0f, pnewBolt);
            pAreaBolts = pnewBolt;
        }
    }
}

bool MagicBoltProjectile::PostProcess() {
    NiNode* pmagicNode;
    TESObjectREFR* v5;
    TESObjectREFR* pcasterRef;
    MagicCaster* pcaster = GetCaster();
    if (pcaster) {
        pcasterRef = pcaster->GetCasterStatsObject();
    }
    else {
        pcasterRef = nullptr;
    }
    if (pcasterRef && pcasterRef->IsActor()) {
        v5 = pcasterRef;
    }
    else {
        v5 = nullptr;
    }
    PlayerCharacter* pcasterActor = static_cast<PlayerCharacter*>(v5);
    if (v5) {
        pmagicNode = pcasterActor->GetMagicNode();
    }
    else {
        pmagicNode = nullptr;
    }
    bhkCharacterController* pchar = GetCharController();
    if (!pmagicNode || (!pchar || !spBoltNode.m_pObject)) {
        return true;
    }
    if (fAge < 0.5 && pcasterActor == pPlayer && !pPlayer->Is3rdPerson()) {
        NiPoint3 startLocation = pmagicNode->GetWorldTranslate();
        spBoltNode.m_pObject->SetTranslate(startLocation);
    }
    if (fAge > 0.0) {
        NiPoint3 endLocation;
        NiMatrix3 rot;
        NiPoint3 startLocation_2 = spBoltNode.m_pObject->GetWorldTranslate();
        pchar->GetPosition(endLocation);
        NiPoint3 toTarget = endLocation - startLocation_2;
        float fz = toTarget.z;
        toTarget.z = 0.0f;
        float fxy = toTarget.Length();
        float Y = -NiATan2(fz, fxy);
        float fZAngle = NiATan2(toTarget.x, toTarget.y);
        rot.FromEulerAnglesZYX(fZAngle, 0.0f, Y);
        spBoltNode.m_pObject->SetRotate(rot);
    }
    NiUpdateData UpdateData(0.0f, false, false);
    spBoltNode.m_pObject->Update(UpdateData);
    return false;
}

void MagicBoltProjectile::SetState(State aeNewState) {
    eState = aeNewState;
}

void MagicBoltProjectile::UpdateBranch(float afDelta, MagicBoltProjectile::AreaBolt* apBranch) {
    NiPoint3 toTarget;
    NiMatrix3 rot;
    NiNode* pnode = apBranch->spBoltNode.m_pObject;
    if (apBranch->spSource.m_pObject) {
        apBranch->SourceLoc = apBranch->spSource.m_pObject->GetWorldTranslate();
    }
    pnode->SetTranslate(apBranch->SourceLoc);
    toTarget = apBranch->spTarget.m_pObject->GetWorldTranslate() - apBranch->SourceLoc;
    double v9 = fLastUpdate - fCollideTime;
    bool bfinish = fShockBranchLifetime.Float() < v9;
    float X = fSpeed;
    float fZAngle = toTarget.Length();
    BoltShaderProperty* v3 = apBranch->spBoltProperty.m_pObject;
    v3->UpdateAndMoveBolt(afDelta, fZAngle, X, bfinish);
    float fz = toTarget.z;
    toTarget.z = 0.0f;
    float fxy = toTarget.Length();
    float Ya = -NiATan2(fz, fxy);
    float fZAnglea = NiATan2(toTarget.x, toTarget.y);
    rot.FromEulerAnglesZYX(fZAnglea, 0.0f, Ya);
    pnode->SetRotate(rot);
}

void MagicBoltProjectile::Init3D() {
    NiNode* v21;
    NiNode* v22;
    NiMatrix3 rot;
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        v22 = ::new (apMemory) NiNode(0);
    }
    else {
        v22 = nullptr;
    }
    NiRefObject* v31 = (NiRefObject*)v22;
    spSoundNode = v22;
    const NiPoint3* v1 = &GetLocationOnReference();
    spSoundNode.m_pObject->SetWorldTranslate(*v1);
    NiNode* v28 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
    if (v28) {
        v21 = ::new (v28) NiNode(0);
    }
    else {
        v21 = nullptr;
    }
    NiRefObject* v29 = (NiRefObject*)v21;
    spBoltNode = v21;
    const NiPoint3* v2 = &GetLocationOnReference();
    spBoltNode.m_pObject->SetTranslate(*v2);
    const NiFixedString* v3 = &BSXFlags::Tag();
    BSXFlags* pextra = static_cast<BSXFlags*>(spBoltNode.m_pObject->GetExtraData(*v3));
    if (!pextra) {
        BSXFlags* pextra_2;
        BSXFlags* v26 = static_cast<BSXFlags*>(NiMemObject::operator new(sizeof(BSXFlags)));
        if (v26) {
            pextra_2 = ::new (v26) BSXFlags;
        }
        else {
            pextra_2 = nullptr;
        }
        pextra = pextra_2;
        const NiFixedString* v4 = &BSXFlags::Tag();
        spBoltNode.m_pObject->AddExtraData(*v4, pextra_2);
    }
    pextra->SetFlags(1u, true);
    spBoltProperty = static_cast<BoltShaderProperty*>(nullptr);
    NiPoint3 v33 = GetAngleOnReference();
    rot.FromEulerAnglesZXY(v33.z, v33.x, v33.y);
    spBoltNode.m_pObject->SetRotate(rot);
    spBoltProperty = BoltShader::Start(spBoltNode.m_pObject, true);
    if (spBoltProperty.m_pObject) {
        spBoltProperty.m_pObject->SetAbsorbNumBolts(iShockNumBolts.Int());
        float* v5 = &fShockBoltsRadius.Float();
        spBoltProperty.m_pObject->SetBoltsRadius(*v5);
        float* v6 = &fShockBoltsRadiusStrength.Float();
        spBoltProperty.m_pObject->SetBoltsRadiusStrength(*v6);
        float* v7 = &fShockBoltSmallWidth.Float();
        spBoltProperty.m_pObject->SetBoltSmallWidth(*v7);
        float* v8 = &fShockBoltGrowWidth.Float();
        spBoltProperty.m_pObject->SetBoltGrowWidth(*v8);
        float a4 = fShockCoreColorB.Float();
        float a3 = fShockCoreColorG.Float();
        float* v9 = &fShockCoreColorR.Float();
        NiColorA kCoreColor(*v9, a3, a4, 1.0f);
        NiColorA* v10 = &kCoreColor;
        spBoltProperty.m_pObject->SetAbsorbCoreColor(v10->r, v10->g, v10->b, v10->a);
        float a4a = fShockGlowColorB.Float();
        float a3a = fShockGlowColorG.Float();
        float* v11 = &fShockGlowColorR.Float();
        NiColorA kGlowColor(*v11, a3a, a4a, 1.0f);
        NiColorA* v12 = &kGlowColor;
        spBoltProperty.m_pObject->SetAbsorbGlowColor(v12->r, v12->g, v12->b, v12->a);
        float* v13 = &fShockSegmentLength.Float();
        spBoltProperty.m_pObject->SetAbsorbSegmentLength(*v13);
        spBoltProperty.m_pObject->iSegmentsPerBolt = iShockSegmentsPerBolt.Int();
        float* v14 = &fShockSegmentVariance.Float();
        spBoltProperty.m_pObject->SetSegmentVariance(*v14);
        float* v15 = &fShockSubSegmentVariance.Float();
        spBoltProperty.m_pObject->SetAbsorbTortuosityVariance(*v15);
        spBoltProperty.m_pObject->SetStartFromPoint(true);
        spBoltProperty.m_pObject->InitSegmentData();
    }
}

void BoltShader::SetSubSegmentCount(int32_t aiSubSegments) {
    iSubSegments = aiSubSegments;
}

bool BoltShaderProperty::IsFinished() {
    return !bWrapped && iBoltStartSegment >= iBoltEndSegment;
}
