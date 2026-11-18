#include "Fallout/AI/Combat/FlameProjectile.h"

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "SDK/MSVC/ehvec.h"

FlameProjectile::FlameProjectile(bool abAdd) : Projectile(abAdd), fExpirationTimer(0.0f), fConeAngle(0.0f) {
    SetFormType(PFLA_ID);
}

FlameProjectile::FlameProjectile(
    BGSProjectile* a1,
    TESObjectREFR* a2,
    TESObjectWEAP* a3,
    NiPoint3 a4,
    float a5,
    float a6
)
  : Projectile(a1, a2, a3, a4, a5, a6), fExpirationTimer(0.0f), fConeAngle(0.0f) {
    SetFormType(PFLA_ID);
}

void FlameProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fExpirationTimer), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fConeAngle), 4, 0);
}

void FlameProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fExpirationTimer), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fConeAngle), 4);
}

void FlameProjectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::InitLoadGame(apLoadGameBuffer);
}

void FlameProjectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::Revert(apLoadGameBuffer);
    fExpirationTimer = 0.0f;
    fConeAngle = 0.0f;
}

void FlameProjectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::FinishLoadGame(apLoadGameBuffer);
}

Projectile::PROJECTILE_TYPE FlameProjectile::GetProjectileType() {
    return PROJECTILE_TYPE_FLAME;
}

void FlameProjectile::Initialize() {
    Projectile::SetFlag(Projectile::TRACER, false);
    Projectile::SetFlag(Projectile::HITSCAN_MODE, false);
    Projectile::SetFlag(Projectile::NO_3D, false);
    Projectile::SetFlag(Projectile::NO_COLLISION, true);
    Projectile::SetFlag(Projectile::FADING, false);
    Projectile::SetFlag(Projectile::POST_PROCESS, true);
    Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, true);
    Projectile::Initialize();
}

// Stores the root's rotation relative to the shooter's fire node, then narrows the cone to 0.9 of the flame's.
void FlameProjectile::Process3D() {
    NiNode* pRootNode = Get3D();
    if (pRootNode) {
        NiAVObject* pProjectileNode = GetShooter()->GetFireNode();
        if (pProjectileNode) {
            NiUpdateData kUpdateData;
            kUpdateData.~NiUpdateData();
            ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
            pRootNode->UpdateWorldData(kUpdateData);
            NiMatrix3 kInverse;
            const NiMatrix3* pInverse;
            pInverse = pProjectileNode->GetWorldTransform().m_Rotate.Inverse(&kInverse);
            kFollowOffset.m_Rotate = Get3D()->GetWorldTransform().m_Rotate * *pInverse;
        }
        // Multiplies by the double 0.8999999761581421.
        fConeAngle = static_cast<float>(FindFlameConeAngle(pRootNode) * 0.8999999761581421);
    }
}

void FlameProjectile::UpdateProjectile(float afDelta) {
    NiMatrix3 v14;
    if (!GetDelete()) {
        NiAVObject* pfireNode;
        if (GetShooter()) {
            TESObjectREFR* Shooter = GetShooter();
            pfireNode = Shooter->GetFireNode();
        }
        else {
            pfireNode = nullptr;
        }
        NiAVObject* p3d = Get3D();
        if ((pfireNode) && (p3d)) {
            p3d->SetRotate(kFollowOffset.m_Rotate * (&pfireNode->GetWorldTransform())->m_Rotate);
            NiMatrix3* v3 = &(v14 = kFollowOffset.m_Rotate * (&pfireNode->GetWorldTransform())->m_Rotate);
            SetRotation(v3);
            const NiPoint3* afDelta = &pfireNode->GetWorldTranslate();
            p3d->SetTranslate(*afDelta);
            SetLocationOnReference(pfireNode->GetWorldTranslate());
        }

        if (fExpirationTimer == 0.0) {
            RunTargetPick();
            CheckExplosion(afDelta);
        }
        if (fExpirationTimer == 0.0) {
            NiNode* v10 = Get3D();
            NiNode* v4 = v10->IsNode();
            TESObjectREFR::AddMasterParticleAddonNodes(v4);
            NiNode* v5 = Get3D();
            v5->SetAppCulled(false);
        }
        else {
            if (fExpirationTimer > 0.30000001192092896) {
                NiNode* v6 = Get3D();
                v6->SetAppCulled(true);
                NiNode* v9 = Get3D();
                NiNode* v7 = v9->IsNode();
                TESObjectREFR::RemoveMasterParticleAddonNodes(v7);
            }
        }
        fExpirationTimer = fExpirationTimer + afDelta;
        if (fExpirationTimer > 1.0 && (ImpactList.IsEmpty() || bImpactProcessed)) {
            Kill();
        }
    }
}

// The flame casts four rays from its position to points on its cone (around the forward direction of its heading and
// looking), with the character controller's filter or, without one, the shooter's group (or a new one) on the
// projectile layer; true when any ray reaches a target. Its own cell world is fetched but the rays go through the
// player's cell world, as the game does.
bool FlameProjectile::RunTargetPick() {
    if (!GetParentCell() || !GetParentCell()->GetbhkWorld()) {
        return false;
    }
    bool bHit = false;
    [[maybe_unused]] bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
    bhkPickData kPickData;
    kPickData.~bhkPickData();
    ::new (&kPickData) bhkPickData;
    NiPoint3 kOrigin;
    kOrigin.~NiPoint3();
    ::new (&kOrigin) NiPoint3;
    NiPoint3 kForward;
    kForward.~NiPoint3();
    ::new (&kForward) NiPoint3;
    kOrigin = GetLocationOnReference();
    NiMatrix3 kRotation;
    kRotation.~NiMatrix3();
    ::new (&kRotation) NiMatrix3;
    kRotation.MakeZRotation(GetHeading(false));
    NiMatrix3 kLooking;
    kLooking.~NiMatrix3();
    ::new (&kLooking) NiMatrix3;
    kLooking.MakeXRotation(GetLooking());
    kRotation = kRotation * kLooking;
    kRotation.GetCol(1, kForward);
    kForward.Unitize();
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
    if (GetCharController()) {
        CFilter kControllerFilter;
        kFilter.iFilter =
            static_cast<bhkCharacterController*>(GetCharController())->GetCollisionFilter(&kControllerFilter)->iFilter;
    }
    else {
        kFilter.SetLayer(L_PROJECTILE);
        if (GetShooter() && GetShooter()->IsActor()) {
            // GetGroup is called on the returned filter word, which is this local.
            CFilter kShooterFilter;
            static_cast<MobileObject*>(GetShooter())->GetCollisionFilter(&kShooterFilter.iFilter);
            kFilter.SetGroup(kShooterFilter.GetGroup());
        }
        else {
            kFilter.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
        }
    }
    if (GetPickCache() && GetPickCache()->GetCache()) {
        kPickData.SetCache(GetPickCache()->GetCache());
    }
    NiPoint3 akPoints[4];
    rt::eh_vector_constructor_iterator(akPoints, sizeof(NiPoint3), 4, &emptyPointConstructor);
    float fRotation = RandomFloatTwoPi();
    float fRange = GetProjectileBase()->GetRange();
    float fConeAngle = GetConeAngle();
    float fLooking = GetLooking();
    float fHeading = GetHeading(false);
    Projectile::GetConeEndpoints(akPoints, 3, kOrigin, fHeading, fLooking, fConeAngle, fRange, fRotation);
    float fLength = GetRange() - GetDistanceMoved();
    // The end point is computed and never read, as the game does.
    [[maybe_unused]] NiPoint3 kEnd = kOrigin + kForward * fLength;
    kPickData.SetFilter(kFilter);
    kPickData.NiSetFrom(kOrigin);
    for (int32_t i = 0; i < 4; ++i) {
        kPickData.NiSetLength(akPoints[i] - kOrigin);
        alignas(16) uint8_t acCollector[sizeof(hkpAllRayHitCollector)];
        hkpAllRayHitCollector* pCollector = ::new (acCollector) hkpAllRayHitCollector;
        kPickData.SetAllCollector(pCollector);
        bhkWorld* pPlayerWorld = pPlayer->GetParentCell()->GetbhkWorld();
        bool bPicked = pPlayerWorld->PickObject(kPickData);
        if (bPicked) {
            bPicked = ProcessTargetPick(&kPickData, this, nullptr, nullptr, false, 3.4028235e+38f) != nullptr;
        }
        if (bPicked) {
            bHit = true;
        }
        if (bProjectileDebug.Bool()) {
            NiColorA kColor =
                kPickData.GetPickFailed() ? NiColorA(1.0f, 0.0f, 0.0f, 1.0f) : NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
            NiLines* pLine = MakeLine(kOrigin, kColor, akPoints[i], kColor, true);
            TES::pInstance->AddTempDebugObject(pLine, fProjectileDebugDuration.Float());
        }
        pCollector->~hkpAllRayHitCollector();
    }
    return bHit;
}

// A flame hits each reference once: a repeated hit on an actor is refused, any other repeated hit is accepted
// without adding an impact.
bool FlameProjectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase())) {
        return false;
    }
    BSSimpleList<ImpactData*>* pIter = &ImpactList;
    while (pIter && !pIter->IsEmpty()) {
        ImpactData* pImpact;
        pImpact = pIter->GetItem();
        if (apCollidee == pImpact->pCollidee) {
            return !(apCollidee && apCollidee->IsActor());
        }
        pIter = pIter->GetNext();
    }
    return Projectile::AddImpact(apCollidee, aContactPoint, aContactNormal, apTargetCollidable, aiKey, aeForceMaterial);
}

void FlameProjectile::ResetExpirationTimer() {
    fExpirationTimer = 0.0f;
    while (!ImpactList.IsEmpty()) {
        Projectile::ImpactData* apThis = ImpactList.m_item;
        if (apThis) {
            delete apThis;
        }
        ImpactList.RemoveHead();
    }
    SpawnMuzzleFlash();
    bImpactProcessed = false;
}

float FlameProjectile::FindFlameConeAngle(NiAVObject* apObject) {
    NiParticleSystem* v1;
    float flameConeAngle;
    uint32_t a1;
    NiPSysModifier* modifierAt;
    uint32_t uiIndex;
    float fDeclinationVar = (float)0.0;
    if (!apObject) {
        return fDeclinationVar;
    }
    if (NiObject::IsKindOf_0(&BSStripParticleSystem::ms_RTTI, apObject)) {
        for (uiIndex = 0; (fDeclinationVar == 0.0) &&
             (uiIndex < ((uint32_t)((int32_t)static_cast<NiParticleSystem*>(apObject)->GetModifierCount())));
             ++uiIndex) {
            modifierAt = static_cast<NiParticleSystem*>(apObject)->GetModifierAt(uiIndex);
            if ((modifierAt) && (NiObject::IsKindOf_0(&NiPSysEmitter::ms_RTTI, modifierAt))) {
                fDeclinationVar = static_cast<NiPSysEmitter*>(modifierAt)->m_fDeclinationVar;
            }
        }
    }
    NiNode* v7 = apObject->IsNode();
    if (v7) {
        for (a1 = 0; a1 < v7->GetArrayCount(); ++a1) {
            v1 = static_cast<NiParticleSystem*>(v7->GetAt(a1));
            flameConeAngle = FlameProjectile::FindFlameConeAngle(v1);
            if (fDeclinationVar < ((double)flameConeAngle)) {
                fDeclinationVar = flameConeAngle;
            }
        }
    }
    return fDeclinationVar;
}

ProjectilePickCache* Projectile::GetPickCache() {
    return spPickCache.operator->();
}
