#include "Fallout/AI/Combat/GrenadeProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

GrenadeProjectile::GrenadeProjectile(bool abAdd) : Projectile(abAdd), bCollisionGroupReset(false) {
    SetFormType(PGRE_ID);
}

// The base projectile's construction, then the grenade's own vtables, its state byte and its form type.
GrenadeProjectile::GrenadeProjectile(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : Projectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle), bCollisionGroupReset(false) {
    SetFormType(PGRE_ID);
}

void GrenadeProjectile::Process3D() {
    uint8_t v2 = (uint8_t)(!Projectile::IsFlagSet(Projectile::TURNED_OFF));
    NiNode* v1 = Get3D();
    TESObjectREFR::SetLight(v1, v2);
}

bool GrenadeProjectile::TurnOff(Actor* apActionActor, bool abSilent) {
    int8_t v5 = (int8_t)Projectile::TurnOff(apActionActor, abSilent);
    Process3D();
    return (bool)v5;
}

bool GrenadeProjectile::IsPermanent() {
    return IsMine() || Projectile::IsPermanent();
}

void GrenadeProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
}

void GrenadeProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
}

void GrenadeProjectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::InitLoadGame(apLoadGameBuffer);
}

void GrenadeProjectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::Revert(apLoadGameBuffer);
    bCollisionGroupReset = false;
}

void GrenadeProjectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::FinishLoadGame(apLoadGameBuffer);
}

Projectile::PROJECTILE_TYPE GrenadeProjectile::GetProjectileType() {
    return PROJECTILE_TYPE_GRENADE;
}

void GrenadeProjectile::Initialize() {
    Projectile::SetFlag(Projectile::NO_CHARCONTROLLER, true);
    if (Projectile::IsFlagSet(Projectile::ACTIVE_IN_INVENTORY)) {
        Projectile::SetFlag(Projectile::NO_3D, true);
        Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, false);
    }
    else {
        Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, true);
    }
    if (!GetProjectileBase()->GetHasExplosion() || GetProjectileBase()->GetHasExplosionAltTrigger() ||
        GetProjectileBase()->GetHasExplosionDetonates()) {
        Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, false);
    }
    else {
        Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, true);
    }
    Projectile::SetFlag(Projectile::KILL_ON_EXPLOSION, true);
    Projectile::SetFlag(Projectile::WHOLE_BODY_TARGETED, true);
    Projectile::Initialize();
}

void GrenadeProjectile::UpdateProjectile(float afDelta) {
    FNVD_INLINED_LOCAL(BSSoundHandle, sndHandle, std::nullopt, std::nullopt);
    if (!GetDelete()) {
        if (!bCollisionGroupReset && GetAge() > 1.0) {
            uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
            NiNode* v2 = Get3D();
            bhkWorld::SetGroup(v2, NewSystemGroup, true, false);
            bCollisionGroupReset = true;
            if ((GetProjectileBase()) && (GetProjectileBase()->Data.pActiveSoundLoop)) {
                TESSound* WorldSpace = GetProjectileBase()->Data.pActiveSoundLoop;
                if (!TESSound::ConvertLoopFlag(WorldSpace->GetData().iFlags)) {
                    void* afDelta = GetProjectileBase()->Data.pActiveSoundLoop;
                    TESSound* v5 = GetProjectileBase()->Data.pActiveSoundLoop;
                    const char* Filename = v5->GetFilename();
                    BSAudio* v6 = &BSAudio::QInstance();
                    v6->GetSoundHandleByFilename(&sndHandle, Filename, 258u, afDelta);
                    const NiPoint3* v7 = &GetLocationOnReference();
                    sndHandle.SetPosition(*v7);
                    sndHandle.Play(false);
                }
            }
        }
        float fdeathAge = 0.0f;
        if ((GetProjectileBase()->GetHasExplosionAltTrigger() && GetProjectileBase()->Data.fExplosionProximity > 0.0) ||
            GetProjectileBase()->GetHasExplosionDetonates()) {
            fdeathAge = fMineAgeMax.Float();
        }
        else {
            fdeathAge = fGrenadeAgeMax.Float();
        }
        if (fdeathAge != 0.0) {
            double Age = GetAge();
            if (fdeathAge < Age) {
                uint32_t CastingType = iFormID;
                if (pDataHandler->IsFormIDCreated(CastingType)) {
                    Kill();
                }
            }
        }
        if ((IsFlagSet(Projectile::ACTIVE_IN_INVENTORY)) && (GetDesiredTarget())) {
            HandleInventoryGrenadeDialogue();
            TESObjectREFR* DesiredTarget = GetDesiredTarget();
            const NiPoint3* v10 = &DesiredTarget->GetLocationOnReference();
            SetLocationOnReference(*v10);
        }

        CheckExplosion(afDelta);
    }
}

// A projectile that detonates, or has a proximity alternate trigger.
bool GrenadeProjectile::IsMine() {
    if (GetProjectileBase()->GetHasExplosionDetonates()) {
        return true;
    }
    if (!GetProjectileBase()->GetHasExplosionAltTrigger()) {
        return false;
    }
    return GetProjectileBase()->GetExplosionProximity() > 0.0;
}

// A mine (a detonating projectile, or one with a proximity alternate trigger) is fixed in place once Havok
// deactivates it.
void GrenadeProjectile::ReportHavokDeactivation() {
    if (GetProjectileBase()->GetHasExplosionDetonates() ||
        (GetProjectileBase()->GetHasExplosionAltTrigger() && GetProjectileBase()->GetExplosionProximity() > 0.0)) {
        bhkWorld::SetMotion(Get3D(), hkpMotion::MOTION_FIXED, true, false, true);
    }
}

GrenadeProjectile::~GrenadeProjectile() {}

// In VATS playback, first slows the grenade's dynamic rigid body by the player's Explosives skill (90% of the
// velocity at skill 0, 10% at 100). A grenade takes only its first impact, and only a valid one the base class
// accepts: hitting an actor stops it dead; any other first impact at age 0 bounces it at once and marks the impact
// processed.
bool GrenadeProjectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(Get3D());
        if (pcol) {
            bhkRigidBody* prb = pcol->GetRigidBody();
            if (prb) {
                hkpMotion::MotionType eMotion = static_cast<hkpMotion::MotionType>(prb->GetMotionType());
                if (bhkRigidBody::IsMotionTypeDynamic(eMotion)) {
                    hkVector4 velocity;
                    float fMult = 0.9f;
                    fMult = static_cast<double>(100 - pPlayer->GetActorValue(ActorValue::EXPLOSIVES)) / 100.0 * fMult;
                    fMult = fMult + 0.10000000149011612;
                    velocity.set(fMult, fMult, fMult, 0.0f);
                    velocity.mul4(prb->GetLinearVelocity());
                    prb->SetLinearVelocity(velocity);
                }
            }
        }
    }
    if (!ImpactList.IsEmpty()) {
        return false;
    }
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase()) ||
        !Projectile::AddImpact(apCollidee, aContactPoint, aContactNormal, apTargetCollidable, aiKey, aeForceMaterial)) {
        return false;
    }
    if (apCollidee && apCollidee->IsActor()) {
        TESHavokUtilities::AddVelocity(Get3D(), NiPoint3_ZERO, true);
    }
    else if (GetAge() == 0.0) {
        ProcessBounce();
        bImpactProcessed = true;
    }
    return true;
}

// Finds the references behind both bodies of the contact; when one of them is a projectile, adds the impact on the
// other with the contact's point and normal (the normal flipped when the projectile is body A). When the projectile is
// body B, the contact's normal is deliberately read for the point too, as the game does. A contact between an actor and
// a projectile that both have rigid bodies is rejected.
void GrenadeProjectile::ReportCollision(hkpContactPointAddedEvent& arEvent) {
    TESObjectREFR* prefB;
    NiAVObject* pavB;
    bhkCollisionObject* pcolB;
    TESObjectREFR* prefA;
    NiAVObject* pavA;
    bhkCollisionObject* pcolA;
    hkpRigidBody* pbodyA = hkpGetRigidBody(arEvent.m_bodyA->getRootCollidable());
    if (pbodyA) {
        pcolA = bhkCollisionObject::Getbhk(pbodyA);
    }
    else {
        pcolA = nullptr;
    }
    if (pcolA) {
        pavA = pcolA->m_pkSceneObject;
    }
    else {
        pavA = nullptr;
    }
    if (pavA) {
        prefA = TESObjectREFR::FindReferenceFor3D(pavA);
    }
    else {
        prefA = nullptr;
    }
    hkpRigidBody* pbodyB = hkpGetRigidBody(arEvent.m_bodyB->getRootCollidable());
    if (pbodyB) {
        pcolB = bhkCollisionObject::Getbhk(pbodyB);
    }
    else {
        pcolB = nullptr;
    }
    if (pcolB) {
        pavB = pcolB->m_pkSceneObject;
    }
    else {
        pavB = nullptr;
    }
    if (pavB) {
        prefB = TESObjectREFR::FindReferenceFor3D(pavB);
    }
    else {
        prefB = nullptr;
    }
    TESObjectREFR* pcollidee = nullptr;
    NiPoint3 contactPoint = NiPoint3_ZERO;
    NiPoint3 contactNormal = NiPoint3_ZERO;
    const hkpCollidable* pcollidable = nullptr;
    uint32_t ikey = -1;
    if (prefA && prefA->IsProjectile()) {
        pcollidee = prefB;
        HK2NI(contactPoint, arEvent.m_contactPoint->getPosition());
        HK2NI(contactNormal, arEvent.m_contactPoint->getNormal());
        contactNormal.operator*=(-1.0f);
        pcollidable = arEvent.m_bodyB->getRootCollidable();
        ikey = arEvent.m_bodyB->m_shapeKey;
    }
    else if (prefB && prefB->IsProjectile()) {
        pcollidee = prefA;
        HK2NI(contactPoint, arEvent.m_contactPoint->getNormal());
        HK2NI(contactNormal, arEvent.m_contactPoint->getNormal());
        pcollidable = arEvent.m_bodyA->getRootCollidable();
        ikey = arEvent.m_bodyA->m_shapeKey;
    }
    AddImpact(pcollidee, contactPoint, contactNormal, pcollidable, ikey, BHK_MATERIAL_MAX);
    if (pcollidee && pcollidee->IsActor() && pbodyA && pbodyB) {
        arEvent.m_status = HK_CONTACT_POINT_REJECT;
    }
}

// Adds an impact on apController's reference at akPos.
void GrenadeProjectile::ReportCollision(bhkCharacterController* apController, NiPoint3 akPos, NiPoint3 akNormal) {
    if ((apController) && (apController->GetNiObject())) {
        NiAVObject* NiObject_ = apController->GetNiObject();
        TESObjectREFR* ReferenceFor3D = TESObjectREFR::FindReferenceFor3D(NiObject_);
        hkpShapePhantom* v9 = apController->GethkShapePhantom();
        AddImpact(ReferenceFor3D, akPos, akNormal, v9->getCollidable(), 0xFFFFFFFFu, BHK_MATERIAL_MAX);
    }
}

bool GrenadeProjectile::ProcessBounce() {
    if (!ImpactList.IsEmpty()) {
        NiNode* p3D = Get3D();
        ImpactData* pImpact = ImpactList.GetItem();
        if (!pSaveLoadGameOLD->GetLoading()) {
            bhkWorld::UpdatePosition(p3D, true, false);
            float fRadius = p3D->GetWorldBoundRadius();
            p3D->SetTranslate(pImpact->kLocation + pImpact->kNormal * fRadius);
            NiUpdateData kData;
            kData.~NiUpdateData();
            ::new (&kData) NiUpdateData(0.0f, false, false);
            p3D->Update(kData);
            NiPoint3 kVelocity = NiPoint3_ZERO;
            NiPoint3 kDown;
            kDown.~NiPoint3();
            ::new (&kDown) NiPoint3;
            p3D->GetWorldTransform().m_Rotate.GetCol(1, kDown);
            kDown *= -1.0f;
            NiPoint3::UnitizeVector(kDown);
            NiPoint3::UnitizeVector(pImpact->kNormal);
            float fDot = pImpact->kNormal.Dot(kDown);
            kVelocity = kDown + 2.0f * (fDot * pImpact->kNormal - kDown);
            kVelocity *= GetProjectileBase()->GetSpeed();
            if (GetProjectileBase()->GetBounceMultiplier() > 0.0) {
                kVelocity *= GetProjectileBase()->GetBounceMultiplier();
            }
            TESHavokUtilities::AddVelocity(p3D, kVelocity, true);
        }
    }
    return true;
}
