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

#include <limits>

FlameProjectile::FlameProjectile(bool abAdd) : Projectile(abAdd), fExpirationTimer(0.0f), fConeAngle(0.0f) {
    SetFormType(PFLA_ID);
}

FlameProjectile::FlameProjectile(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : Projectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle), fExpirationTimer(0.0f),
    fConeAngle(0.0f) {
    SetFormType(PFLA_ID);
}

void FlameProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fExpirationTimer), sizeof(fExpirationTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fConeAngle), sizeof(fConeAngle), 0);
}

void FlameProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fExpirationTimer), sizeof(fExpirationTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fConeAngle), sizeof(fConeAngle));
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
            NiUpdateData kUpdateData(0.0f, false, false);
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
    NiMatrix3 kRotate;
    if (!GetDelete()) {
        NiAVObject* pfireNode = GetShooter() ? GetShooter()->GetFireNode() : nullptr;
        NiAVObject* p3d = Get3D();
        if (pfireNode && p3d) {
            p3d->SetRotate(kFollowOffset.m_Rotate * pfireNode->GetWorldTransform().m_Rotate);
            kRotate = kFollowOffset.m_Rotate * pfireNode->GetWorldTransform().m_Rotate;
            SetRotation(&kRotate);
            p3d->SetTranslate(pfireNode->GetWorldTranslate());
            SetLocationOnReference(pfireNode->GetWorldTranslate());
        }

        if (fExpirationTimer == 0.0) {
            RunTargetPick();
            CheckExplosion(afDelta);
        }
        if (fExpirationTimer == 0.0) {
            NiNode* pNode = Get3D()->IsNode();
            TESObjectREFR::AddMasterParticleAddonNodes(pNode);
            Get3D()->SetAppCulled(false);
        }
        else if (fExpirationTimer > 0.30000001192092896) {
            Get3D()->SetAppCulled(true);
            NiNode* pNode = Get3D()->IsNode();
            TESObjectREFR::RemoveMasterParticleAddonNodes(pNode);
        }
        fExpirationTimer = fExpirationTimer + afDelta;
        if (fExpirationTimer > 1.0 && (ImpactList.IsEmpty() || bImpactProcessed)) {
            Kill();
        }
    }
}

// The flame casts four rays from its position to points on its cone (around the forward direction of its heading and
// looking), with the character controller's filter or, without one, the shooter's group (or a new one) on the
// projectile layer; true when any ray reaches a target. The rays go through the player's cell world.
bool FlameProjectile::RunTargetPick() {
    if (!GetParentCell() || !GetParentCell()->GetbhkWorld()) {
        return false;
    }
    bool bHit = false;
    bhkPickData kPickData;
    NiPoint3 kOrigin;
    NiPoint3 kForward;
    kOrigin = GetLocationOnReference();
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(GetHeading(false));
    NiMatrix3 kLooking;
    kLooking.MakeXRotation(GetLooking());
    kRotation = kRotation * kLooking;
    kRotation.GetCol(1, kForward);
    kForward.Unitize();
    CFilter kFilter(0);
    if (GetCharController()) {
        CFilter kControllerFilter;
        kFilter.iFilter = static_cast<bhkCharacterController*>(GetCharController())
                              ->GetCollisionFilter(&kControllerFilter)
                              ->iFilter;
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
    float fRotation = RandomFloatTwoPi();
    float fRange = GetProjectileBase()->GetRange();
    float fConeAngle = GetConeAngle();
    float fLooking = GetLooking();
    float fHeading = GetHeading(false);
    Projectile::GetConeEndpoints(akPoints, 3, kOrigin, fHeading, fLooking, fConeAngle, fRange, fRotation);
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
            bPicked = ProcessTargetPick(&kPickData, this, nullptr, nullptr, false, std::numeric_limits<float>::max()) !=
                nullptr;
        }
        if (bPicked) {
            bHit = true;
        }
        if (bProjectileDebug.Bool()) {
            NiColorA kColor = kPickData.GetPickFailed() ? NiColorA(1.0f, 0.0f, 0.0f, 1.0f)
                                                        : NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
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
        ImpactData* pImpact = pIter->GetItem();
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
        Projectile::ImpactData* pImpact = ImpactList.m_item;
        if (pImpact) {
            delete pImpact;
        }
        ImpactList.RemoveHead();
    }
    SpawnMuzzleFlash();
    bImpactProcessed = false;
}

float FlameProjectile::FindFlameConeAngle(NiAVObject* apObject) {
    float fDeclinationVar = 0.0f;
    if (!apObject) {
        return fDeclinationVar;
    }
    if (NiObject::IsKindOf_0(&BSStripParticleSystem::ms_RTTI, apObject)) {
        for (uint32_t uiIndex = 0;
             fDeclinationVar == 0.0 && uiIndex < static_cast<NiParticleSystem*>(apObject)->GetModifierCount();
             ++uiIndex) {
            NiPSysModifier* pModifier = static_cast<NiParticleSystem*>(apObject)->GetModifierAt(uiIndex);
            if (pModifier && NiObject::IsKindOf_0(&NiPSysEmitter::ms_RTTI, pModifier)) {
                fDeclinationVar = static_cast<NiPSysEmitter*>(pModifier)->m_fDeclinationVar;
            }
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiParticleSystem* pChild = static_cast<NiParticleSystem*>(pNode->GetAt(i));
            float fFlameConeAngle = FlameProjectile::FindFlameConeAngle(pChild);
            if (fDeclinationVar < static_cast<double>(fFlameConeAngle)) {
                fDeclinationVar = fFlameConeAngle;
            }
        }
    }
    return fDeclinationVar;
}

ProjectilePickCache* Projectile::GetPickCache() {
    return spPickCache;
}
